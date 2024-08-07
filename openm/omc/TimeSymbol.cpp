/**
* @file    TimeSymbol.cpp
* Definitions for the TimeSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "TimeSymbol.h"
#include "CodeBlock.h"

using namespace std;

bool TimeSymbol::is_wrapped()
{
    return time_type == token::TK_float || time_type == token::TK_double || time_type == token::TK_ldouble;
}

CodeBlock TimeSymbol::cxx_declaration_global()
{
    // Hook into the hierarchical calling chain
    CodeBlock h = super::cxx_declaration_global();

    // Perform operations specific to this level in the Symbol hierarchy.

    // The NumericSymbol upwards in the hierarchy does not generated a 
    // typedef for Time, because it requires special treatment.
    // 
    string typedef_string;
    if (is_wrapped()) {
        // wrap the floating point type in the typedef
        typedef_string = "fixed_precision<" + token_to_string(time_type) + ">";
    }
    else {
        typedef_string = token_to_string(time_type);
    }

    h += "typedef " + typedef_string + " Time;";
    h += "typedef " + exposed_type() + " Time_t; // For use in model code";
    h += "typedef " + typedef_string + " TIME;";
    h += "typedef " + exposed_type() + " TIME_t; // For use in model code";
    // Time 'literals'
    h += "extern const Time_t time_infinite;";
    h += "extern const Time_t TIME_INFINITE; // For Modgen source compatibility";
    h += "extern const Time_t time_undef;";
    h += "extern const Time_t TIME_UNDEF; // For Modgen source compatibility";

    return h;
}

CodeBlock TimeSymbol::cxx_definition_global()
{
    // Hook into the hierarchical calling chain
    CodeBlock c = super::cxx_definition_global();

    // Perform operations specific to this level in the Symbol hierarchy.

    // Time 'literals'
    if (is_floating() && !Symbol::option_time_infinite_is_32767) {
        string typ = token_to_string(time_type);
        c += "const Time_t time_infinite = std::numeric_limits<" + typ + ">::infinity();";
        c += "const Time_t TIME_INFINITE = std::numeric_limits<" + typ + ">::infinity(); // for Modgen compatibility";
    }
    else {
        c += "const Time_t time_infinite = 32767;";
        c += "const Time_t TIME_INFINITE = 32767; // for Modgen compatibility";
    }
    if (is_floating() && !Symbol::option_time_undef_is_minus_one) {
        string typ = token_to_string(time_type);
        c += "const Time_t time_undef = std::numeric_limits<" + typ + ">::quiet_NaN();";
        c += "const Time_t TIME_UNDEF = std::numeric_limits<" + typ + ">::quiet_NaN(); // for Modgen compatibility";
    }
    else {
        c += "const Time_t time_undef = -1;";
        c += "const Time_t TIME_UNDEF = -1; // for Modgen compatibility";
    }
    c += "";

    return c;
}

// static
TimeSymbol * TimeSymbol::find()
{
    auto ts = dynamic_cast<TimeSymbol *>(get_symbol(token_to_string(token::TK_Time)));
    assert(ts); // only called when valid
    return ts;
}

Constant * TimeSymbol::make_constant(const string & i_value) const
{
    return TimeLiteral::is_valid_literal(i_value.c_str()) ? new Constant(new TimeLiteral(i_value)) : nullptr;
}

string TimeSymbol::format_for_storage(const Constant & k) const
{
    if (openm::equalNoCase(k.value().c_str(), "time_undef")) {
        return (is_floating() && !Symbol::option_time_undef_is_minus_one) ? "NULL" : "-1";
    }
    
    if (openm::equalNoCase(k.value().c_str(), "time_infinite")) {
        if (is_floating() && !Symbol::option_time_infinite_is_32767) {
            // using max instead of infinity because it must be float value and not 'inf' string
            switch (time_type) {
            case token::TK_float:
                return to_string(std::numeric_limits<float>::max());
            case token::TK_double:
                return to_string(std::numeric_limits<double>::max());
            case token::TK_ldouble:
                return to_string(std::numeric_limits<long double>::max());
            default:
                return k.value();    // unexpected Time floating type
            }
            // return k.value();   // unexpected Time floating type
        }
        else {
            return "32767";
        }
    }

    return NumericSymbol::format_for_storage(k);
}
