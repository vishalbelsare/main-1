/**
* @file    EntityArrayMemberSymbol.cpp
* Definitions for the EntityArrayMemberSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "EntityArrayMemberSymbol.h"
#include "TypeSymbol.h"
#include "EnumerationSymbol.h"
#include "CodeBlock.h"

using namespace std;

CodeBlock EntityArrayMemberSymbol::cxx_initialization_expression(bool type_default) const
{
    CodeBlock c;
    // example:              om_in_DurationOfLife_alive = false;\n
    c += " // TODO EntityArrayMemberSymbol " + name;
    //c += name + " = " + initialization_value(type_default) + ";";
    return c;
}

CodeBlock EntityArrayMemberSymbol::cxx_declaration_entity()
{
    // Hook into the hierarchical calling chain
    CodeBlock h = super::cxx_declaration_entity();

    // Perform operations specific to this level in the Symbol hierarchy.
    string dims;
    for (auto es : pp_dimension_list) dims += "[" + es->name + "::size]";
    h += pp_data_type->name + " " + name + dims + ";";
    return h;
}

size_t EntityArrayMemberSymbol::cell_count() const
{
    size_t cells = 1;
    for (auto dim : pp_dimension_list) {
        cells *= dim->pp_size();
    }
    return cells;
}


void EntityArrayMemberSymbol::post_parse(int pass)
{
    // Hook into the post_parse hierarchical calling chain
    super::post_parse(pass);

    // Perform post-parse operations specific to this level in the Symbol hierarchy.
    switch (pass) {
    case ePopulateCollections:
    {
        // create post-parse list of dimensions
        for (auto sym : dimension_list) {
            auto pp_sym = pp_symbol(sym);
            assert(pp_sym);
            auto es = dynamic_cast<EnumerationSymbol *>(pp_sym);
            if (!es) {
                pp_error(decl_loc, LT("error : invalid dimension '") + pp_sym->name + LT("' in '") + name + LT("'"));
            }
            else {
                pp_dimension_list.push_back(es);
            }
        }
        break;
    }
    default:
        break;
    }
}




