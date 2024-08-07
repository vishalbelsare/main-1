/**
* @file    DimensionSymbol.h
* Definitions for the DimensionSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include <cassert>
#include "DimensionSymbol.h"
#include "AttributeSymbol.h"
#include "LinkToAttributeSymbol.h"
#include "EnumerationSymbol.h"
#include "LanguageSymbol.h"
#include "libopenm/common/omHelper.h"

using namespace openm;

// static
string DimensionSymbol::symbol_name(const Symbol* symbol_with_dimensions, int index, bool after_expression_dim)
{
    assert(symbol_with_dimensions);
    return symbol_with_dimensions->name + "::Dim" + to_string(index);
}

// static
string DimensionSymbol::modgen_symbol_name(const Symbol* symbol_with_dimensions, int index, bool after_expression_dim)
{
    assert(symbol_with_dimensions);
    // Modgen counts the the expression dimension in the "Dim" suffix,
    // so 1 must be added to dimension index for dimensions which follow the expression dimension
    // to contruct the Modgen symbol name.
    return symbol_with_dimensions->name + "::Dim" + to_string(index + after_expression_dim);
}

// Update dimension name to be suitable as database column name: it must be unique, alphanumeric and not longer than 255 chars
void DimensionSymbol::to_column_name(const string & i_ownerName, const list<DimensionSymbol *> i_dimLst, DimensionSymbol * io_dim)
{
    assert(io_dim);
    size_t maxlen = std::min<size_t>(short_name_max_length, OM_NAME_DB_MAX);
    string colName = openm::toAlphaNumeric(io_dim->short_name, (int)maxlen);  // make dimension name alphanumeric and truncate it to 255 chars

    for (auto pIt = i_dimLst.cbegin(); pIt != i_dimLst.cend() && *pIt != io_dim; ++pIt) {
        if (colName == (*pIt)->short_name) {
            string sId = to_string(io_dim->index);
            if (colName.length() + sId.length() <= maxlen) {
                // append disambiguating unique numeric suffix
                colName += sId;
            }
            else {
                // replace trailing characters of name with numeric suffix
                colName = colName.replace(colName.length() - sId.length(), sId.length(), sId);
            }
            break;
        }
    }
    if (io_dim->short_name != colName) {
        // to do: use pp_warning() if we can apply LT without string + concatenation
        theLog->logFormatted(LT("warning: %s dimension [%d] name %s updated to %s"), i_ownerName.c_str(), io_dim->index, io_dim->short_name.c_str(), colName.c_str());

        io_dim->short_name = colName;      // change dimension short name
    }
}

void DimensionSymbol::post_parse(int pass)
{
    // Hook into the post_parse hierarchical calling chain
    super::post_parse(pass);

    // Perform post-parse operations specific to this level in the Symbol hierarchy.
    switch (pass) {
    case eAssignLabel:
    {
        // If an explicit short name was given by //NAME, use it
        auto search = explicit_names.find(unique_name);
        if (search != explicit_names.end()) {
            auto text = (search->second).first;
            auto loc = (search->second).second;
            short_name_explicit = text;
            short_name = short_name_explicit;
        }
        // If an explicit short name was provided, use it to look for and process if found
        // a LABEL or NOTE which uses it as key, e.g. //LABEL(MyTable.Prov,FR)
        if (short_name_explicit != "") {
            // note that pp_containing_symbol is not yet available in this pass
            string key = (**containing_symbol).name + "::" + short_name;
            associate_explicit_label_or_note(key);
        }
        break;
    }
    case eAssignMembers:
    {
        pp_containing_symbol = pp_symbol(containing_symbol);
        if (attribute) {
            // Attribute was assigned during parsing.
            // The dimension symbol is in an entity table or entity set.
            // assign direct pointer to attribute for post-parse use
            pp_attribute = dynamic_cast<AttributeSymbol *> (pp_symbol(attribute));
            if (!pp_attribute) {
                pp_error(LT("error : '") + (*attribute)->name + LT("' must be an attribute to be used as a dimension"));
            }
        }
        else {
            assert(enumeration); // grammar guarnatee
            // Attribute was not assigned during parsing.
            // The dimension symbol is in a derived table.
            // There is no attribute associated with this dimension.
            // An enumeration was specified for this dimension.
            pp_enumeration = dynamic_cast<EnumerationSymbol *> (pp_symbol(enumeration));
            if (!pp_enumeration) {
                pp_error(LT("error : '") + (*enumeration)->name + LT("' must be an enumeration to be used as a dimension"));
            }
        }
        break;
    }
    case ePopulateCollections:
    {
        if (pp_attribute) {
            // Assign direct pointer to enumeration for post-parse use
            pp_enumeration = dynamic_cast<EnumerationSymbol *>(pp_attribute->pp_data_type);
            if (!pp_enumeration) {
                pp_error(LT("error : the data-type of '") + pp_attribute->name + LT("' must be an enumeration to be used as a dimension"));
            }
        }
        {
            // Create fall-back label if not given explicitly
            for (const auto& langSym : Symbol::pp_all_languages) {
                int lang_index = langSym->language_id; // 0-based
                if (!pp_labels_explicit[lang_index]) {
                    // no explicit label for this language
                    assert(pp_enumeration); // logic guarantee
                    if (pp_enumeration->pp_labels_explicit[lang_index]) {
                        // the underlying enumeration has an explicit label for this language, use it
                        pp_labels[lang_index] = pp_enumeration->pp_labels[lang_index];
                    }
                    else {
                        // use Dim0, etc.
                        pp_labels[lang_index] = short_name_default;
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

string DimensionSymbol::heuristic_short_name(void) const
{
    string hn; // heuristic name

    // Get the dimension's label for the model's default language.
    string lbl = pp_labels[0];
    string unm = unique_name;
    if (lbl != unm && lbl.length() <= short_name_max_length) {
        // Use label from model source code, if not too long.
        hn = lbl;
    }
    else {
        // Get the dimension's attribute, if present (if dimension is in an entity table).
        auto attr = pp_attribute;
        if (attr) {
            // Dimension has an attribute.
            // Base the heuristic name on the dimension's attribute.
            string lbl = attr->pp_labels[0];
            string unm = attr->unique_name;
            if (lbl != unm && lbl.length() <= short_name_max_length) {
                // Use explicit label from model source code.
                hn = lbl;
            }
            else {
                // No explicit label in source code, or is too long.
                // Use the same name as model source (unqualified by entity name).
                hn = attr->name;
            }
        }
        else {
            // Base the name on the dimension's underlying enumeration.
            assert(pp_enumeration); // logic guarantee
            string lbl = pp_enumeration->pp_labels[0];
            string unm = pp_enumeration->unique_name;
            if (lbl != unm && lbl.length() <= short_name_max_length) {
                // Use explicit label from model source code.
                hn = lbl;
            }
            else {
                // No explicit label in source code, or is too long
                // Use the same name as model source code.
                hn = unm;
            }
        }
    }

    assert(hn.length() > 0); // logic guarantee

    // trim off leading "om_" prefix if present
    //if (hn.starts_with("om_")) { // c++20
    if (hn.length() >= 3 && hn.substr(0, 3) == "om_") {
        hn.erase(0,3);
    }

    if (!std::isalpha(hn[0], locale::classic())) {
        // First character is not alphabetic.
        // Prepend X_ to make valid name
        hn = "X_" + hn;
    }

    // trim off trailing "_" if present
    //if (hn.ends_with("_")) { // c++20
    if (hn[hn.length() - 1] == '_') {
        hn.erase(hn.length() - 1, 1);
    }

    // if name is subject to truncation, remove characters from middle rather than truncating from end,
    // because long descriptions often disambiguate at both beginning and end.
    if (hn.length() > short_name_max_length) {
        // replacement string for snipped section
        string r = "_x_";
        // number of characters to snip from middle (excess plus length of replacement)
        size_t n = hn.length() - short_name_max_length + r.length();
        // start of snipped section (middle section of original string)
        size_t p = (hn.length() - n) / 2;
        // replace snipped section with _
        if (p > 0 && p < hn.length()) {
            hn.replace(p, n, r);
        }
    }

    // Make name alphanumeric and truncate it to maximum length.
    hn = openm::toAlphaNumeric(hn, (int)short_name_max_length);

    // trim off trailing "_" if present
    //if (hn.ends_with("_")) { // c++20
    if (hn[hn.length() - 1] == '_') {
        hn.erase(hn.length() - 1, 1);
    }

    return hn;
}
