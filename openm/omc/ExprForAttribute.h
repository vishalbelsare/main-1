/**
 * @file    ExprForAttribute.h
 * Declares and implements the class ExprForAttribute which represents elements in an openM++ agentvar expression
 */
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#pragma once
#include "Symbol.h"

class Literal;

using namespace std;


/**
 * AgentVar expression tree element.
 * 
 * An expression in an openM++ expression agentvar specification or filter is parsed into a tree
 * whose nodes and leaves are instances of ExprForAttribute.  Each such instance represents an
 * operator or a terminal leaf of the tree.
 */

class ExprForAttribute {
public:

	virtual ~ExprForAttribute()
    {
    	//TODO traverse tree recursively and destroy (but why bother?)
    }
};

class ExprForAgentVarUnaryOp : public ExprForAttribute {
public:

    /**
     * Constructor for a unary operator in an agentvar expression tree.
     *
     * @param   op      The token for the unary operator, e.g. token::TK_MINUS.
     * @param   right   The right argument.
     */
	ExprForAgentVarUnaryOp(token_type op, ExprForAttribute *right)
	    : op ( op )
	    , right ( right )
    {
    }

	const token_type op;
	ExprForAttribute *right;
};

class ExprForAgentVarBinaryOp : public ExprForAttribute {
public:

    /**
     * Constructor for a binary operator in an agentvar expression tree.
     *
     * @param   op      The token for the binary operator, e.g. token::TK_PLUS.
     * @param   left    The left argument.
     * @param   right   The right argument.
     */
	ExprForAgentVarBinaryOp(token_type op, ExprForAttribute *left, ExprForAttribute *right)
	    : op ( op )
	    , left ( left )
	    , right ( right )
    {
    }

	const token_type op;
	ExprForAttribute *left;
	ExprForAttribute *right;
};

class ExprForAgentVarTernaryOp : public ExprForAttribute {
public:

    /**
     * Constructor for the ternary operator in an agentvar expression tree.
     *
     * @param [in,out] cond   The condition argumnet of the C ternary opertor.
     * @param [in,out] first  The result if true.
     * @param [in,out] second The result if false.
     */
	ExprForAgentVarTernaryOp(ExprForAttribute *cond, ExprForAttribute *first, ExprForAttribute *second)
	    : cond ( cond )
	    , first ( first )
	    , second ( second )
    {
    }

	ExprForAttribute *cond;
	ExprForAttribute *first;
	ExprForAttribute *second;
};

class ExprForAgentVarSymbol : public ExprForAttribute {
public:

    /**
     * Constructor for a symbol (terminal) in an agentvar expression tree.
     *
     */
	ExprForAgentVarSymbol(const Symbol *symbol)
	    : symbol ( symbol->stable_rp() )
        , pp_symbol ( nullptr )
    {
    }

	Symbol*& symbol;
	Symbol* pp_symbol;
};

class ExprForAgentVarLiteral : public ExprForAttribute {
public:

    /**
     * Constructor for a constant literal (terminal) in an agentvar expression tree.
     *
     * @param constant 
     */
	ExprForAgentVarLiteral(const Literal *constant)
	    : constant (constant)
    {
    }

    const Literal *constant;
};