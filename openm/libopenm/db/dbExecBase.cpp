/**
 * @file
 * OpenM++ data library: base class for db connection wrapper
 */
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "dbExec.h"
#include "dbExecBase.h"
using namespace openm;

// mutex to lock database operations
recursive_mutex openm::dbMutex;

/** prepare to open new db-connection. */
DbExecBase::DbExecBase(const string & i_connectionStr)
{
    try {
        lock_guard<recursive_mutex> lck(dbMutex);
        connProps = parseConnectionStr(i_connectionStr);
    }
    catch (DbException & ex) {
        theLog->logErr(ex, OM_FILE_LINE);
        throw;
    }
    catch (exception & ex) {
        theLog->logErr(ex, OM_FILE_LINE);
        throw DbException(ex.what());
    }
}

/** return true in transaction scope. */
bool DbExecBase::isTransaction(void)
{
    try {
        lock_guard<recursive_mutex> lck(dbMutex);
        return
            trxThreadId != thread::id();
    }
    catch (DbException & ex) {
        theLog->logErr(ex, OM_FILE_LINE);
        throw;
    }
    catch (exception & ex) {
        theLog->logErr(ex, OM_FILE_LINE);
        throw DbException(ex.what());
    }
}

/** return true if other thread have active transaction. */
bool DbExecBase::isTransactionNonOwn(void)
{
    lock_guard<recursive_mutex> lck(dbMutex);
    return
        trxThreadId != thread::id() && trxThreadId != this_thread::get_id();
}

/** set transaction ownership status. */
void DbExecBase::setTransactionActive(void)
{
    lock_guard<recursive_mutex> lck(dbMutex);
    if (isTransactionNonOwn()) throw DbException("db transaction active on other thread");

    trxThreadId = this_thread::get_id();
}

/** release transaction: clean active transaction status. */
void DbExecBase::releaseTransaction(void)
{
    lock_guard<recursive_mutex> lck(dbMutex);
    if (isTransactionNonOwn()) throw DbException("db transaction active on other thread");

    trxThreadId = thread::id();
}

/** return string value of connection property by key (case neutral) or empty "" string if key not found. */
string DbExecBase::strConnProperty(const string & i_key) const
{
    return (connProps.find(i_key) != connProps.end()) ? connProps.at(i_key) : "";
}

/** return true if value of connection property is 'true', 'yes', '1' (case neutral). */
bool DbExecBase::boolConnProperty(const string & i_key) const
{
    string sVal = strConnProperty(i_key);
    return 
        equalNoCase(sVal.c_str(), "1") || equalNoCase(sVal.c_str(), "yes") || equalNoCase(sVal.c_str(), "true");
}

/** return long value of connection property or default value if key not found (case neutral). */
long long DbExecBase::longConnProperty(const string & i_key, long i_default) const
{
    try {
        string sVal = strConnProperty(i_key);
        return 
            (sVal != "") ? std::stoll(sVal) : i_default;
    }
    catch (exception & ex) {
        throw DbException("Invalid connection string value of: %s. %s", i_key.c_str(), ex.what());
    }
}

/**   
* parse connection string and return case-neutral map of key+value string pairs.
*
* connection string consist of key=value pairs separated by semicolon \n
* leading and trailing spaces are trimmed from key and value \n
* value can be in 'single' or "double" quotes to include ; semicolon or leading or trailing spaces \n
* for example: Database=my.data; Password = " spaces included "
*/
NoCaseMap DbExecBase::parseConnectionStr(const string & i_connectionStr)
{
    string connStr = trim(i_connectionStr);
    NoCaseMap connMap;

    string sKey;
    string sVal;
    string::size_type keyPos = 0;
    string::size_type eqPos = 0;
    string::size_type valPos = 0;
    bool isKey = true;
    bool isInQuote = false;
    char chQuote = '\0';

    for (string::size_type nowPos = 0; nowPos < connStr.length(); nowPos++) {

        // skip empty chars before key or value
        char chNow = connStr[nowPos];

        // get key: delimited by = equal sign
        if (isKey && chNow == '=') {

            sKey = trim(connStr.substr(keyPos, nowPos - keyPos));

            while (sKey.length() > 0 && sKey[0] == ';') {     // cleanup duplicate ;; semicolons
                sKey = trim(sKey.substr(1));
            }
            if (sKey == "") throw DbException("Invalid (empty) key in connection string at position: %d", nowPos);

            isKey = false;
            eqPos = nowPos;
            valPos = nowPos + 1;
            continue;   // skip = equal sign
        }

        // get value: delimited by ; semicolon outside of "quotes"
        if (!isKey && !isInQuote && chNow == ';') {

            sVal = trim(connStr.substr(valPos, nowPos - valPos));

            if (sVal.length() >= 2 && (sVal[0] == '"' || sVal[0] == '\'') && sVal[sVal.length() - 1] == sVal[0]) 
                sVal = sVal.substr(1, sVal.length() - 2);

            // add key and value to collection
            connMap[sKey] = sVal;
            sKey = sVal = "";

            isKey = true;
            keyPos = nowPos + 1;
            continue;   // skip ; semicolon
        }

        // check for begin of "quoted" value
        if (!isKey && !isInQuote && (chNow == '"' || chNow == '\'')) {
            isInQuote = true;
            chQuote = chNow;
            continue;
        }

        // check for end of "quoted" value
        if (!isKey && isInQuote && chNow == chQuote) {
            isInQuote = false;
            chQuote = '\0';
            continue;
        }
    }

    // check for any non-space chars at the end of connection string
    if (keyPos >= eqPos && keyPos < connStr.length()) {
        if (trim(connStr.substr(keyPos)) != "") 
            throw DbException("Invalid connection string starting from position: %d", keyPos);
    }

    // add last key and value to collection
    if (sKey != "") {

            sVal = (valPos < connStr.length()) ? trim(connStr.substr(valPos)) : "";

            if (sVal.length() >= 2 && (sVal[0] == '"' || sVal[0] == '\'') && sVal[sVal.length() - 1] == sVal[0]) 
                sVal = sVal.substr(1, sVal.length() - 2);

            connMap[sKey] = sVal;
    }

    return connMap;
}