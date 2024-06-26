/**
* @file    EntityHookSymbol.cpp
* Definitions for the EntityHookSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include <cassert>
#include <algorithm>
#include "Symbol.h"
#include "EntityHookSymbol.h"
#include "EntitySymbol.h"
#include "DerivedAttributeSymbol.h"

using namespace std;

// static
string EntityHookSymbol::symbol_name(const Symbol* from, const Symbol* to)
{
    return "om_hook_" + from->name + "_TO_" + to->name;
}

//static
bool EntityHookSymbol::exists(const Symbol* ent, const Symbol* from, const Symbol* to)
{
    return Symbol::exists(symbol_name(from, to), ent);
}


void EntityHookSymbol::create_auxiliary_symbols()
{
    // The hook_<func> member function
    {
        auto ss = dynamic_cast<DerivedAttributeSymbol *>(to);
        if (ss) {
            assert(ss->is_self_scheduling()); // logic guarantee
            to_is_self_scheduling = true;
            ss->any_to_hooks = true;
            pp_to_ss = ss; // can do early, so do now rather than in post-parse phase
            // For hooks to ss, a single placeholder name is used for all hooks
            // to make them hook together.
            to_name = "self_scheduling"; 
        }
        else {
            to_is_self_scheduling = false;
            to_name = to->name; // name of the function being hooked to
            string fn_name = EntityFuncSymbol::hook_implement_name(to->name);
            auto sym = Symbol::get_symbol(fn_name, entity);

            EntityFuncSymbol *fn_sym = nullptr;
            if (sym) {
                // cast it to derived type
                fn_sym = dynamic_cast<EntityFuncSymbol *>(sym);
                if (!fn_sym) {
                    pp_error(LT("error : symbol '") + fn_name + LT("' is reserved for implementing hooks"));
                    return;
                }
            }
            else {
                // create it
                fn_sym = new EntityFuncSymbol(fn_name, entity, "void", "");
                fn_sym->doc_block = doxygen_short("Call the functions hooked to the function '" + to->name + "'");
            }
            // store it
            hook_fn = fn_sym;
        }
    }
}

void EntityHookSymbol::post_parse(int pass)
{
    // Hook into the post_parse hierarchical calling chain
    super::post_parse(pass);

    // Perform post-parse operations specific to this level in the Symbol hierarchy.
    switch (pass) {
    case eAssignMembers:
    {
        // assign direct pointer to entity for use post-parse
        pp_entity = dynamic_cast<EntitySymbol *> (pp_symbol(entity));
        assert(pp_entity); // parser guarantee
        // assign direct pointer to the 'from' entity function symbol for use post-parse
        pp_from = dynamic_cast<EntityFuncSymbol *> (pp_symbol(from));
        if (!pp_from) {
            pp_error(LT("error : '") + from->name + LT("' must be a function member of entity '") + pp_entity->name + LT("'"));
            break;
        }
        if (!to_is_self_scheduling) {
            pp_to_fn = dynamic_cast<EntityFuncSymbol *> (pp_symbol(to));
            if (!pp_to_fn) {
                pp_error(LT("error : '") + to->name + LT("' must be a function member of entity '") + pp_entity->name + LT("'"));
                break;
            }
        }

        if (to_is_self_scheduling) {
            // hook function is the implement function of the self-scheduling event of the entity
            hook_fn = pp_entity->ss_implement_fn;
            assert(hook_fn);
        }

        // Create entry in entity multimap of all hooks.
        // The key is the name of the 'to' function
        // (or a placeholder string if the hook is to a self-scheduling attribute).
        pp_entity->pp_hooks.emplace(to_name, pp_from->name);

        // Create entry in entity multimap of all hooks (additionally distinguished by order).
        // The key is constructed using two parts to allow subsequent testing for 
        // ties in hook order.
        string key = to->name + " order=" + to_string(order);
        pp_entity->pp_hooks_with_order.emplace(key, pp_from->name);
        break;
    }
    case ePopulateDependencies:
    {
        if (!to_is_self_scheduling) {
            assert(hook_fn); // logic guarantee
            string nm = hook_fn->name;
            // verify that hook_fn is actually used somewhere in the body of the 'to' hook function
            if (!any_of(
                        pp_to_fn->body_identifiers.begin(),
                        pp_to_fn->body_identifiers.end(),
                        [nm](string id){ return nm == id; })) {
                pp_error(LT("error : the target function '") + pp_to_fn->unique_name + LT("' of the hook contains no call to '") + hook_fn->name + LT("'"));
                Symbol::pp_logmsg(pp_to_fn->defn_loc, LT("note : see definition of target function '") + pp_to_fn->unique_name + LT("'"));
            }
        }

        // Test for ambiguous hook order and emit warning if found.
        {
            string key = to->name + " order=" + to_string(order); // NB copied from line above
            if (pp_entity->pp_hooks_with_order.count(key) > 1) {
                pp_warning(LT("warning : one or more functions hooking to '") + to->pretty_name() + LT("' are ordered ambiguously with respect to '") + pp_from->name + LT("'."));
            }
        }

        CodeBlock & cxx = hook_fn->func_body;
        cxx += injection_description();
        if (to_is_self_scheduling) {
            // The target of code injection for ss hooks is the self-scheduling event implement function of the entity.
            // Code injection order (sorting_group) is low to ensure that injection occurs last,
            // because the code makes use of the local-scoped variable flag_name,
            // set earlier within the ss event implement function.
            assert(pp_to_ss); // logic guarantee
            cxx += "if (" + pp_to_ss->flag_name() + ") {";
            cxx += pp_from->name + "();";
            cxx += "}";
        }
        else {
            cxx += pp_from->name + "();";
        }
        break;
    }
    default:
        break;
    }
}



