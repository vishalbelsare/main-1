/**
* @file    RangeSymbol.cpp
* Definitions for the RangeSymbol class.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include <cassert>
#include "RangeSymbol.h"
#include "LanguageSymbol.h"
#include "CodeBlock.h"
#include "libopenm/db/metaModelHolder.h"

#include "omc_location.hh"

using namespace std;
using namespace openm;

const string RangeSymbol::default_initial_value() const {
    return to_string(lower_bound);
}

void RangeSymbol::post_parse(int pass)
{
    // Hook into the post_parse hierarchical calling chain
    super::post_parse(pass);

    // Perform post-parse operations specific to this level in the Symbol hierarchy.
    switch (pass) {
    case eCreateMissingSymbols:
        {
            // Create enumerators for each integer within range
            //for (int i=lower_bound, j=0; i<=upper_bound; ++i, ++j) {
            //    string enumerator_name = name + "_";
            //    if (i < 0) enumerator_name += "_" + to_string(-i);
            //    else  enumerator_name += to_string(i);
            //    auto sym = new RangeEnumeratorSymbol(enumerator_name, this, j, lower_bound);
            //}

            // Information to compute storage type is known in this pass
            storage_type = optimized_storage_type(lower_bound, upper_bound);

            // Semantic errors for range can be detected in this pass
            if (lower_bound > upper_bound) {
                pp_error(LT("error : minimum of range is greater than maximum"));
            }
        }
        break;
    default:
        break;
    }
}

CodeBlock RangeSymbol::cxx_declaration_global()
{
    // Hook into the hierarchical calling chain
    CodeBlock h = super::cxx_declaration_global();

    // Perform operations specific to this level in the Symbol hierarchy.

    h += "";
    h += doxygen("Range: " + name);

    h += "extern const std::string om_name_" + name + ";";
    h += doxygen_short("Range {" + to_string(lower_bound) + "..." + to_string(upper_bound) + "}: " + label());
    h += "typedef Range<"
        + token_to_string(storage_type) + ", "
        + to_string(lower_bound) + ", "
        + to_string(upper_bound) + ", "
        + "&om_name_" + name
        + "> "
        + name + ";";
    h += doxygen_short("C-type of " + name + " (" + exposed_type() + ")");
    h += "typedef " + exposed_type() + " " + name + "_t;";

    return h;
}

CodeBlock RangeSymbol::cxx_definition_global()
{
    // Hook into the hierarchical calling chain
    CodeBlock c = super::cxx_definition_global();

    // Perform operations specific to this level in the Symbol hierarchy.

    c += "";
    c += doxygen_short(name);

    c += "const std::string om_name_" + name + " = \"" + pretty_name() + "\";";

    return c;
}

void RangeSymbol::populate_metadata(openm::MetaModelHolder & metaRows)
{
    // Hook into the hierarchical calling chain
    super::populate_metadata(metaRows);

    // Perform operations specific to this level in the Symbol hierarchy.

    // Only generate metadata if the enumeration has been marked
    // as being used in a table or external parameter.

    if (!metadata_needed) return;

    // Unlike classifications, etc. (children of EnumerationWithEnumerators),
	// range enumerations do not populate an associated C++ collection of enumerators.
	// This is because a range may contain 100,000 or more levels in some models,
	// causing performance issues during model compilation and publishing.
    // Instead, enumerator metadata for ranges is created directly at this level (RangeSymbol) 
    // of the hierarchical calling chain.

    // resize range vectors to reduce reallocations
    // 2024-02-09:
    // by final memory size resize() slightly better than just push_back(), reserve() increase final memory size +20%
    size_t nEnum = metaRows.typeEnum.size();
    metaRows.typeEnum.resize(nEnum + pp_size());

    size_t nTxt = metaRows.typeEnumTxt.size();
    metaRows.typeEnumTxt.resize(nTxt + (pp_size() * Symbol::pp_all_languages.size()));
    size_t tIdx = nTxt;

    for (int i = lower_bound, ordinal = 0; i <= upper_bound; ++i, ++ordinal) {
		// The enumerator 'name' in the data store is set to the integer range value.
        string enumerator_name = to_string(i);

        {
            TypeEnumLstRow typeEnum;
            typeEnum.typeId = type_id;
            typeEnum.enumId = lower_bound + ordinal;
            typeEnum.name = enumerator_name;
            metaRows.typeEnum[nEnum + ordinal] = typeEnum;
        }

        for (const auto& langSym : Symbol::pp_all_languages) {
            const string& lang = langSym->name; // e.g. "EN" or "FR"
            TypeEnumTxtLangRow typeEnumTxt;
            typeEnumTxt.typeId = type_id;
            typeEnumTxt.enumId = lower_bound + ordinal;
            typeEnumTxt.langCode = lang;
            typeEnumTxt.descr = to_string(lower_bound + ordinal);
            typeEnumTxt.note = "";
            metaRows.typeEnumTxt[tIdx++] = typeEnumTxt;
        }
    }

    // adjust total enum id for range: range enum id's are not zero based
    vector<TypeDicRow>::iterator it = find_if(
        metaRows.typeDic.begin(),
        metaRows.typeDic.end(),
        [this](TypeDicRow & i_row) -> bool { return i_row.typeId == type_id; });

    if (it == metaRows.typeDic.cend()) throw DbException(LT("invalid type id: %d for range type: %s"), type_id, name.c_str());

    it->totalEnumId = upper_bound + 1;
}

bool RangeSymbol::is_valid_constant(const Constant &k) const
{
    // named enumerators are invalid for a range (only integers allowed)
    if (k.is_enumerator) return false;

    // floating point literals are invalid for range
    if (is_floating()) return false;

    return is_valid_literal(k.value().c_str());
}

bool RangeSymbol::is_valid_literal(const char * i_value) const
{
    if (i_value == nullptr) return false;
    
    // floating point literals are invalid for range
    if (!IntegerLiteral::is_valid_literal(i_value)) return false;

    // value must fall with range bounds
    long v = stol(i_value);
    return lower_bound <= v && v <= upper_bound;
}

Constant * RangeSymbol::make_constant(const string & i_value) const
{
    return (is_valid_literal(i_value.c_str())) ? new Constant(new Literal(i_value)) : nullptr;
}

string RangeSymbol::format_for_storage(const Constant &k) const
{
    long value = stol(k.value());

    string result = to_string(value); // range enum id's are not zero-based
    return result;
}
