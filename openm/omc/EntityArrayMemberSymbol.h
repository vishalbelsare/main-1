/**
* @file    EntityArrayMemberSymbol.h
* Declarations for the EntityArrayMemberSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#pragma once
#include <string>
#include "EntityDataMemberSymbol.h"
#include "EnumerationSymbol.h"

using namespace std;

/**
* Internal data symbol.
* Abstracts a member of an agent class which stores information
* used internally by om.  These members are not accessible to
* developer code, and have no side-effects.
*/

class EntityArrayMemberSymbol : public EntityDataMemberSymbol
{
private:
    typedef EntityDataMemberSymbol super;

public:
    bool is_base_symbol() const { return false; }

    /**
     * Morphing constructor.
     *
     * @param [in,out] sym If non-null, the symbol.
     * @param agent        The agent.
     * @param type         The type.
     */
    EntityArrayMemberSymbol(Symbol *sym, const Symbol *agent, const Symbol *type, yy::location decl_loc = yy::location())
        : EntityDataMemberSymbol(sym, agent, type, decl_loc)
    {
    }

    CodeBlock cxx_initialization_expression(bool type_default) const;

    void post_parse(int pass);

    CodeBlock cxx_declaration_agent();

    /**
     * List of dimensions.
     */
    list<Symbol **> dimension_list;

    /**
     * List of symbols (post-parse)
     */
    list<EnumerationSymbol *> pp_dimension_list;
};
