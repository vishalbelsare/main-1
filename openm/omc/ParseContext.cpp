/**
 * @file    ParseContext.cpp
 * Implements some functions in the parse context class.
 */
// Copyright (c) 2013-2016 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "ParseContext.h"

string ParseContext::cxx_process_token(token_type tok, const string tok_str, yy::location * loc)
{
    // Maintain the ordered list of all tokens in C++ code
    cxx_tokens.push_back(make_pair(tok, tok_str));

    // return value (possibly modified in code below)
    string result = tok_str;

    //
    // Note all symbols used in C++ code.
    // 
    if (tok == token::SYMBOL) {
        Symbol::identifiers_in_model_source.insert(tok_str);
    }

    //
    // Handle beginning of a member function definition.
    // 
    if (tok == token::TK_LEFT_BRACE && brace_level == 1 && parenthesis_level == 0) {
        // { at level 0 is the possible start of the body of a member function definition
        bool is_memfunc = false;
        int pos = 0;
        int plev = 0;
        bool in_parmlist = false;
        vector<string> parmlist;
        bool name2_coming = false;
        string name2;
        bool scope_coming = false;
        bool name1_coming = false;
        string name1;
        // Iterate backwards through the token list,
        // looking for the pattern name1::name2(parmlist) {
        for (auto it = cxx_tokens.rbegin(); it != cxx_tokens.rend(); ++it, ++pos) {
            // token 0 is {
            if (pos == 0) continue;
            // if token -1 is not ), then not a function definition
            else if (pos == 1) {
                if (it->first != token::TK_RIGHT_PAREN) break;
                ++plev;
                in_parmlist = true;
                continue;
            }
            else if (in_parmlist) {
                // look for the matching (
                if (it->first == token::TK_RIGHT_PAREN) {
                    ++plev;
                }
                if (it->first == token::TK_LEFT_PAREN) {
                    --plev;
                    if (plev == 0) {
                        in_parmlist = false;
                        name2_coming = true;
                        continue;
                    }
                }
                // build the parameter list (reversed)
                parmlist.push_back(it->second);
                continue;
            }
            else if (name2_coming) {
                // snaffle name2
                if (it->first != token::SYMBOL) break;
                name2 = it->second;
                name2_coming = false;
                scope_coming = true;
                continue;
            }
            else if (scope_coming) {
                if (it->first != token::TK_SCOPE_RESOLUTION) break;
                scope_coming = false;
                name1_coming = true;
                continue;
            }
            else if (name1_coming) {
                // snaffle name1
                if (it->first != token::SYMBOL) break;
                // pattern matches
                name1 = it->second;
                is_memfunc = true;
                break;
            }
        }
        if (is_memfunc) {
            cxx_memfunc_gather = true;
            cxx_memfunc_name = name1 + "::" + name2;
            reverse(parmlist.begin(), parmlist.end());
            Symbol::memfunc_parmlist.emplace(cxx_memfunc_name, parmlist);
            Symbol::memfunc_defn_loc.emplace(cxx_memfunc_name, *loc);
        }
    }

    //
    // Handle interior of member functions.
    // 
    if (cxx_memfunc_gather) {
        if (tok == token::TK_RIGHT_BRACE && brace_level == 0) {
            // end of member function
            cxx_memfunc_gather = false;
            cxx_memfunc_name = "";
        }
        else if (tok == token::SYMBOL) {
            // Add identifier to map of all identifiers in member function
            assert(cxx_memfunc_name != "");
            Symbol::memfunc_bodyids.emplace(cxx_memfunc_name, tok_str);
        }
    }

    //
    // Handle special global functions (PreSimulation, etc.)
    // 
    if (tok == token::SYMBOL && brace_level == 0) {
        string word = tok_str;
        for (auto sg : {&Symbol::pre_simulation, &Symbol::post_simulation, &Symbol::derived_tables}) {
            if (sg->prefix == word.substr(0, sg->prefix.length())) {
                /* starts with prefix */
                if (word == sg->prefix) {
                    /* No suffix, so substitute a disambiguated name based on order encountered in code */
                    word = sg->disambiguated_name(sg->ambiguous_count);
                    ++sg->ambiguous_count;
                }
                else {
                    /* Has a suffix */
                    string suffix = word.substr(sg->prefix.length());
                    sg->suffixes.push_back(suffix);
                }
            }
        }
        result = word;
    }

    // Return possibly modified version of the token.
    return result;
}

void ParseContext::process_cxx_comment(const string& cmt, const yy::location& loc)
{
    // Parse //LABEL comments
    if (cmt.length() >= 5 && cmt.substr(0, 5) == "LABEL") {
        //TODO use regex!
        std::string::size_type p = 5;
        std::string::size_type q = 5;

        // Extract symbol name
        p = cmt.find_first_not_of("( \t", p);
        if (p == std::string::npos) {
            warning(loc, "warning : problem (#1) with LABEL comment - not processed");
            return;
        }
        q = cmt.find_first_of(", \t", p);
        if (q == std::string::npos) {
            warning(loc, "warning : problem (#2) with LABEL comment - not processed");
            return;
        }
        string sym_name = cmt.substr(p, q - p);
        // Change . to :: to align with Symbol unique_name
        // e.g. Person.age to Person::age
        auto r = sym_name.find(".");
        if (r != string::npos) {
            sym_name.replace(r, 1, "::");
        }

        // Extract language code
        p = cmt.find_first_not_of(", \t", q);
        if (p == std::string::npos) {
            warning(loc, "warning : problem (#3) with LABEL comment - not processed");
            return;
        }
        q = cmt.find_first_of(") \t", p);
        if (q == std::string::npos) {
            warning(loc, "warning : problem (#4) with LABEL comment - not processed");
            return;
        }
        string lang_code = cmt.substr(p, q - p);
        string key = sym_name + "," + lang_code;

        // Extract label
        p = cmt.find_first_not_of(") \t", q);
        if (p == std::string::npos) {
            // ignore empty label
            return;
        }
        string lab = cmt.substr(p);
        
        // Insert label into map of all explicit //LABEL comments
        Symbol::explicit_labels.emplace(key, lab);

        // Don't place //LABEL comments into list of all cxx_comments,
        // which is used for //EN, etc.
        return;
    }

    // Construct key based on the beginning of the line containing the comment.
    yy::position pos(loc.begin.filename, loc.begin.line, 0);
    comment_map_value_type element(pos, cmt);
    Symbol::cxx_comments.insert(element);
}

void ParseContext::process_c_comment(const string& cmt, const yy::location& loc)
{
    // Parse NOTE comments.  Assume starts as /*NOTE or /* NOTE
    if ((cmt.length() > 4) && ((cmt.substr(0, 4) == "NOTE") || (cmt.substr(0, 5) == " NOTE"))) {
        //TODO use regex!
        std::string::size_type p = 4;
        std::string::size_type q = 4;
        if ((cmt.substr(0, 5) == " NOTE")) {
            p = 5;
            q = 5;
        }

        // Extract symbol name
        p = cmt.find_first_not_of("( \t", p);
        if (p == std::string::npos) {
            warning(loc, "warning : problem (#1) with NOTE comment - not processed");
            return;
        }
        q = cmt.find_first_of(", \t", p);
        if (q == std::string::npos) {
            warning(loc, "warning : problem (#2) with NOTE comment - not processed");
            return;
        }
        string sym_name = cmt.substr(p, q - p);
        // Change . to :: to align with Symbol unique_name
        // e.g. Person.age to Person::age
        auto r = sym_name.find(".");
        if (r != string::npos) {
            sym_name.replace(r, 1, "::");
        }

        // Extract language code
        p = cmt.find_first_not_of(", \t", q);
        if (p == std::string::npos) {
            warning(loc, "warning : problem (#3) with NOTE comment - not processed");
            return;
        }
        q = cmt.find_first_of(") \t", p);
        if (q == std::string::npos) {
            warning(loc, "warning : problem (#4) with NOTE comment - not processed");
            return;
        }
        string lang_code = cmt.substr(p, q - p);
        string key = sym_name + "," + lang_code;

        // Extract note
        p = cmt.find_first_not_of(") \t\n", q);
        if (p == std::string::npos) {
            // ignore empty label
            return;
        }
        string note = normalize_note(cmt.substr(p));
        
        // Insert note into appropriate map of NOTE comments
        if (is_scenario_parameter_value || is_fixed_parameter_value) {
            Symbol::notes_input.emplace(key, note);
        }
        else {
            Symbol::notes_source.emplace(key, note);
        }
    }

    // Construct key based on the start position of the comment.
    yy::position pos(loc.begin);
    comment_map_value_type element(pos, cmt);
    Symbol::c_comments.insert(element);
}

string ParseContext::normalize_note(const string & txt)
{
    string result;
    result.reserve(txt.length());

    auto lines = openm::splitCsv(txt, "\n");
    // trim leading and trailing white space
    for (auto & line : lines) {
        line = openm::trim(line);
    }

    bool prev_empty = true; // previous line is empty
    for (auto & line : lines) {
        if (line[0] == '>') {
            if (!prev_empty) {
                // terminate previous text block
                result += '\n';
            }
            // start new text block
            // append contents following '>'
            result += line.substr(1);
            prev_empty = false;
        }
        else if (line[0] == '-') {
            if (!prev_empty) {
                // terminate previous text block
                result += '\n';
            }
            // start new text block
            // append contents
            result += line;
            prev_empty = false;
        }
        else if (line.length() == 0) {
            if (!prev_empty) {
                // terminate previous text block
                result += '\n';
            }
            prev_empty = true;
        }
        else {
            if (!prev_empty) {
                // append line to previous text block
                result += ' ' + line;
            }
            else {
                // start new text block
                result += line;
            }
            prev_empty = false;
        }
    }

    return result;
}
