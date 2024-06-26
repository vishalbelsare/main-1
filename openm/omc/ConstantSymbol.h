/**
* @file    ConstantSymbol.h
* Declarations for the ConstantSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#pragma once
#include "Symbol.h"

class EnumeratorSymbol;


/**
* ConstantSymbol - Refers to either a Literal or an EnumeratorSymbol
*
*/
class ConstantSymbol : public Symbol
{
private:
    typedef Symbol super;

public:
    bool is_base_symbol() const { return false; }

    /**
    * Constructor for a ConstantSymbol representing an enumerator
    *
    * @param [in,out]  sym The symbol to be morphed.
    */
    ConstantSymbol(Symbol *enumerator, omc::location decl_loc = omc::location())
        : Symbol(next_symbol_name(), decl_loc)
        , is_enumerator(true)
        , enumerator(enumerator->stable_pp())
        , pp_enumerator(nullptr)
        , is_literal(false)
        , literal(nullptr)
    {
        code_label_allowed = false; // constants are not declared in model code so can have no label from model code
    }

    /**
    * Constructor for a ConstantSymbol representing a Literal
    *
    * @param [in,out]  sym The symbol to be morphed.
    */
    ConstantSymbol(Literal *literal, omc::location decl_loc = omc::location())
        : Symbol(next_symbol_name(), decl_loc)
        , is_enumerator(false)
        , enumerator(nullptr)
        , pp_enumerator(nullptr)
        , is_literal(true)
        , literal(literal)
    {
        code_label_allowed = false; // constants are not declared in model code so can have no label from model code
    }

    /**
     * true if this object represents an Enumerator.
     * 
     * If true, then is_literal is false.
     */
    const bool is_enumerator;

    /**
     * Pointer to pointer to enumerator
     * 
     * Stable to symbol morphing during parse phase.
     */
    Symbol** enumerator;

    /**
     * Direct pointer to enumerator
     * 
     * For use post-parse.
     */
    EnumeratorSymbol *pp_enumerator;

    /**
     * true if this object represents a Literal.
     * 
     * If true, then is_enumerator is false.
     */
    const bool is_literal;

    /**
     * The literal.
     */
    Literal *literal;

    /**
     * Gets the constant value as a string
     *
     * @return A string.
     */
    const string value() const;

    /**
     * Transform the constant value to a string which can be part of an identifier
     *
     * @return A string.
     */
    const string value_as_name() const;

    /**
     * Generated name for the next instance of a ConstantSymbol
     *
     * @return A string.
     */
    static string next_symbol_name();

    void post_parse(int pass);

    /**
     * Counter of instances used to generate unique names
     */
    static int instance_counter;
};
