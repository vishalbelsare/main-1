/**
* @file    omc_file.cpp
 * omc file helper functions and translation functions.
*/
// Copyright (c) 2013-2016 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "omc_file.h"

using namespace std;
using namespace openm;
namespace fs = std::filesystem;

namespace omc
{
    /** actual instance of translated message store: empty, loaded in main */
    static TranslatedStore defaultTranslatedStore;

    /** translated messages for all languages from omc.message.ini */
    TranslatedStore * theAllTranslated = &defaultTranslatedStore;
}

// find language-specific message by source non-translated message and language
const string omc::TranslatedStore::getTranslated(const char * i_lang, const char * i_source) const noexcept
{
    try {
        if (i_lang == nullptr || i_source == nullptr) return "";    // empty result if language code empty or source on null

        // if language exist in message.ini
        auto msgMapIt = std::find_if(
            allMsg.cbegin(),
            allMsg.cend(),
            [&i_lang](const pair<string, unordered_map<string, string>> & i_msgPair) -> bool { return equalNoCase(i_lang, i_msgPair.first.c_str()); }
        );
        if (msgMapIt == allMsg.cend()) return i_source;   // language not found

        // if translation exist then return copy of translated message else return original message (no translation)
        const unordered_map<string, string>::const_iterator msgIt = msgMapIt->second.find(i_source);
        return
            (msgIt != msgMapIt->second.cend()) ? msgIt->second.c_str() : i_source;
    }
    catch (...) {
        return "";
    }
}

// get list of files matching extension list from specified directory or current directory if source path is empty
// each file name in result is a relative path and include source directory
list<string> omc::listSourceFiles(const string & i_srcPath, const list<string> & i_extensions)
{
    list<string> pathLst;
    
    // open source directory or current directory if source path is empty
    string srcPath = !i_srcPath.empty() ? i_srcPath : ".";

    // collect list of .mpp, .ompp or .dat files
    for (const fs::directory_entry & de : fs::directory_iterator(srcPath)) {
        
        if (!de.is_regular_file()) continue;    // skip directories and special files

        const string p = de.path().generic_string();

        for (auto e : i_extensions) {
            if (endWithNoCase(p, e.c_str())) {
                pathLst.push_back(p);
                break;
            }
        }
    }
    
    // sort source files in alphabetical order for reproducibility
    pathLst.sort();
    return pathLst;
}

// get extension of path normalized to lowercase
string omc::getPathExtension(const string& path)
{
    string ext = fs::path(path).extension().generic_string();
    openm::toLower(ext);
    return ext;
}

// get stem of path
string omc::getPathStem(const string& path)
{
    return fs::path(path).stem().generic_string();
}

// get filename of path
string omc::getPathFilename(const string& path)
{
    return fs::path(path).filename().generic_string();
}

bool omc::skipPathModule(const string& path)
{
    string stem = omc::getPathStem(path);
    if (openm::equalNoCase(stem.c_str(), "modgen_", 7)) {
        return true;
    }
    else {
        return false;
    }
}

// create output/modelName.message.ini file by merging model messages and languages with existing code/modelName.message.ini 
void omc::buildMessageIni(
    const MetaModelHolder & i_metaRows,
    const string & i_inpDir,
    const string i_outDir,
    const char * i_codePageName,
    const unordered_set<string> & i_msgSet
)
{
    if (i_msgSet.empty()) return;   // exit: no messages to translate

    // cleanup line breaks
    unordered_set<string> msgSet;
    for (string msg : i_msgSet) {
        blankCrLf(msg);
        msgSet.insert(msg);
    }
    locale loc("");         // current user locale

    map<string, map<string, string>, LessNoCase> iniMap;  // output ini-file, section names are case-neutral

    // if exist code/modelName.message.ini then read it and 
    // for each language merge new model messages with existing message translations
    string srcPath = makeFilePath(i_inpDir.c_str(), i_metaRows.modelDic.name.c_str(), ".message.ini");

    if (isFileExists(srcPath.c_str())) {    // if exist code/modelName.message.ini

        // read existing translation file, languages are section names
        list<pair<string, unordered_map<string, string>>> langMsgLst = IniFileReader::loadAllMessages(srcPath.c_str(), i_codePageName);

        // for all languages from message.ini
        for (const auto & langMsg : langMsgLst) {

            auto sectIt = iniMap.insert(pair<string, map<string, string>>(langMsg.first, map<string, string>()));
            map<string, string> & sectMsg = sectIt.first->second;

            for (const string & msg : msgSet) {
                const auto mIt = langMsg.second.find(msg);
                sectMsg[msg] = (mIt != langMsg.second.cend()) ? mIt->second : "";
            }
        }

        // BEGIN of optional code: can be commented
        //
        // count deleted translations for each model language
        for (const LangLstRow & langRow : i_metaRows.langLst) {

            const auto lc = trim(langRow.code, loc);
            const auto sectIt = iniMap.find(lc);        // language section in new ini-file
            if (sectIt == iniMap.cend()) continue;      // skip: no such model language

            const auto rdIt = find_if(
                langMsgLst.cbegin(),
                langMsgLst.cend(),
                [&lc](const pair<string, unordered_map<string, string>> i_lm) -> bool { return equalNoCase(i_lm.first.c_str(), lc.c_str()); });

            if (rdIt == langMsgLst.cend()) continue;    // protect from internal bug

            int nDel = 0;

            for (const auto & rd : rdIt->second) {
                if (sectIt->second.find(rd.first) == sectIt->second.end()) nDel++;
            }
            if (nDel > 0) theLog->logFormatted("Deleted %d translated message(s) from language %s", nDel, lc.c_str());
        }
        // END of optional code
    }
    // merge done for existing ini-file messages

    // BEGIN of optional code: can be commented
    //
    // do the counts and report translation status
    // assume no translations if only one model language
    if (i_metaRows.langLst.size() > 1) {

        // count missing model languages
        // count missing translations for each model language
        int nLangMissing = 0;   // count missing translations for each model language
        int nMsgMissing = 0;    // total count of count missing translations

        for (const LangLstRow & langRow : i_metaRows.langLst) {

            const auto sectIt = iniMap.find(trim(langRow.code, loc));   // language section in new ini-file
            if (sectIt == iniMap.end()) {
                nLangMissing++;
                theLog->logFormatted("Missing translated messages for language %s", langRow.code.c_str());
                continue;           // missing model language
            }

            int nMissing = 0;    // count missing translations for current model language

            for (const auto & ct : sectIt->second) {
                if (ct.second.empty()) nMissing++;      // message exist in in-file, but translation is empty
            }
            for (const string & msg : msgSet) {
                if (sectIt->second.find(msg) == sectIt->second.end()) nMissing++;   // message not exist in ini-file
            }
            nMsgMissing += nMissing;
            if (nMsgMissing > 0) theLog->logFormatted("Missing %d translated message(s) for language %s", nMsgMissing, langRow.code.c_str());
        }

        if (nMsgMissing > 0) theLog->logFormatted("Missing %d translated message(s)", nMsgMissing);
        if (nLangMissing > 0) theLog->logFormatted("Missing translated messages for %d language(s)", nLangMissing);
    }
    // END of optional code

    // for missing model languages append to output ini-file empty translations
    // assume no translations if only one model language
    if (i_metaRows.langLst.size() > 1) {

        for (const LangLstRow & langRow : i_metaRows.langLst) {

            // if language section exist in new ini-file then skip it
            string lc = trim(langRow.code, loc);
            if (iniMap.find(lc) != iniMap.end()) continue;

            auto sectIt = iniMap.insert(pair<string, map<string, string>>(lc, map<string, string>()));
            map<string, string> & sectMsg = sectIt.first->second;

            for (const string & msg : msgSet) { // insert empty translation
                sectMsg[msg] = "";
            }
        }
    }

    // write updated version of output/modelName.message.ini
    if (!iniMap.empty()) {

        string dstPath = makeFilePath(i_outDir.c_str(), i_metaRows.modelDic.name.c_str(), ".message.ini");

        ofstream iniFs(dstPath, ios::out | ios::trunc | ios::binary);
        exit_guard<ofstream> onExit(&iniFs, &ofstream::close);   // close on exit
        if (iniFs.fail()) throw HelperException(LT("error : unable to open %s for writing"), dstPath.c_str());

        iniFs << "; Translated messages for model: " << i_metaRows.modelDic.name << "\r\n" <<
            "; \r\n" <<
            "; Each message must be Key = Value \r\n" <<
            "; Value can be a multi-line with \\ at the end for continuation \r\n" <<
            "; If message starts or end with blank space then put it inside of \"quotes\" or 'single' quotes \r\n" <<
            "; Part of the line after ; or # is just a comment \r\n" <<
            "; Multi-line \"quoted values\" cannot have ; or # comment at the end \r\n" <<
            "; Examples: \r\n" <<
            "; some message = message in English                                    ; this part is a comment \r\n" <<
            "; \" space can be preserved \" = \" space can be preserved in English \" \r\n" <<
            "; ' you can use single quote ' = ' you can use single quote or '' combine \"quotes\" ' \r\n" <<
            "; it can be =\\ \r\n" <<
            ";             \" a multilpe lines \\ \r\n" <<
            ";             value\" \r\n" <<
            "\r\n";

        for (const auto & iniSect : iniMap) {

            iniFs << "[" << iniSect.first << "]\r\n";
            if (iniFs.fail()) throw HelperException(LT("error : unable to write into: %s"), dstPath.c_str());

            for (const auto & ct : iniSect.second) {

                // cleanup: remove empty messages (empty message code)
                if (ct.first.empty()) continue;

                // if code or value start from or end with space
                // put "quotes" or 'single quotes' around of code and value
                if ((isspace<char>(ct.first.front(), loc) || isspace<char>(ct.first.back(), loc)) ||
                    (!ct.second.empty() && (isspace<char>(ct.second.front(), loc) || isspace<char>(ct.second.back(), loc)))) {
                
                    if (ct.first.find("\"") != string::npos) {
                        iniFs << "'" << ct.first << "' = '" << ct.second << "'\r\n";
                    }
                    else {
                        iniFs << "\"" << ct.first << "\" = \"" << ct.second << "\"\r\n";
                    }
                }
                else {
                    iniFs << ct.first << " = " << ct.second << "\r\n";
                }
                if (iniFs.fail()) throw HelperException(LT("error : unable to write into: %s"), dstPath.c_str());
            }

            iniFs << "\r\n";
            if (iniFs.fail()) throw HelperException(LT("error : unable to write into: %s"), dstPath.c_str());
        }
    }
}
