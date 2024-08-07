// OpenM++ data library: output table writer
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "dbValue.h"
#include "dbOutputTable.h"

using namespace openm;

namespace openm
{
    // output table writer implementation
    class OutputTableWriter : public IOutputTableWriter
    {
    public:
        OutputTableWriter(
            int i_runId,
            const char * i_name, 
            IDbExec * i_dbExec, 
            const MetaHolder * i_metaStore, 
            int i_subCount,
            const char * i_doubleFormat = "",
            bool i_isSparse = false,
            double i_nullValue = FLT_MIN
            );

        // Output table writer cleanup
        ~OutputTableWriter(void) noexcept { }

        // return total number of values for each accumulator
        size_t sizeOf(void) const noexcept override { return totalSize; }

        // return number of output aggregated expressions
        int expressionCount(void) const noexcept override { return exprCount; }

        // write output table accumulator values
        void writeAccumulator(IDbExec * i_dbExec, int i_subId, int i_accId, size_t i_size, const double * i_valueArr) override;

        // write all output table values: aggregate sub-values using table expressions
        void writeAllExpressions(IDbExec * i_dbExec) override;

        // calculate output table values digest and store only single copy of output values
        void digestOutput(IDbExec * i_dbExec) override;

    private:
        int runId;                      // destination run id
        int modelId;                    // model id in database
        int tableId;                    // output table id
        int subCount;                   // number of sub-values in model run
        int dimCount;                   // number of dimensions
        int accCount;                   // number of accumulators
        int exprCount;                  // number of aggregated expressions
        size_t totalSize;               // total number of values in the table
        string doubleFmt;               // printf format for float and double
        bool isSparseTable;             // if true then use sparse output to database
        double nullValue;               // if is sparse and abs(value) <= nullValue then value not stored
        string accDbTable;              // db table name for accumulators
        string valueDbTable;            // db table name for aggregated expressions
        const TableDicRow * tableRow;   // table db row
        vector<TableDimsRow> tableDims; // table dimensions
        vector<TableAccRow> tableAcc;   // table accumulators
        vector<TableExprRow> tableExpr; // table aggregation expressions
        vector<vector<int>> dimEnumIds; // enums for each dimension, including "total" enum

        /**
         * write output table value: aggregated output expression value
         *
         * @param[in] i_dbExec      database connection
         * @param[in] i_nExpression aggregation expression number
         */
        void writeExpression(IDbExec * i_dbExec, int i_nExpression);

    private:
        OutputTableWriter(const OutputTableWriter & i_writer) = delete;
        OutputTableWriter & operator=(const OutputTableWriter & i_writer) = delete;
    };
}

// Output table writer cleanup
IOutputTableWriter::~IOutputTableWriter(void) noexcept { }

// Output table writer factory: create new accumulators writer
IOutputTableWriter * IOutputTableWriter::create(
    int i_runId,
    const char * i_name, 
    IDbExec * i_dbExec, 
    const MetaHolder * i_metaStore, 
    int i_subCount, 
    bool i_isSparseGlobal, 
    double i_nullValue
    )
{
    return new OutputTableWriter(i_runId, i_name, i_dbExec, i_metaStore, i_subCount, "", i_isSparseGlobal, i_nullValue);
}

// Output table writer factory: create new writer
IOutputTableWriter * IOutputTableWriter::create(
    int i_runId,
    const char * i_name, 
    IDbExec * i_dbExec, 
    const MetaHolder * i_metaStore, 
    int i_subCount, 
    const char * i_doubleFormat
    )
{
    return new OutputTableWriter(i_runId, i_name, i_dbExec, i_metaStore, i_subCount, i_doubleFormat);
}

// New output table writer
OutputTableWriter::OutputTableWriter(
    int i_runId,
    const char * i_name,
    IDbExec * i_dbExec,
    const MetaHolder * i_metaStore,
    int i_subCount,
    const char * i_doubleFormat,
    bool i_isSparseGlobal,
    double i_nullValue
) :
    runId(i_runId),
    modelId(0),
    tableId(0),
    subCount(i_subCount),
    dimCount(0),
    accCount(0),
    exprCount(0),
    totalSize(0),
    isSparseTable(i_isSparseGlobal),
    nullValue(i_nullValue >= 0.0 ? i_nullValue : FLT_MIN),
    tableRow(nullptr)
{
    // check parameters
    if (i_dbExec == nullptr) throw DbException("invalid (NULL) database connection");
    if (i_metaStore == nullptr) throw DbException("invalid (NULL) model metadata");
    if (i_name == nullptr || i_name[0] == '\0') throw DbException("Invalid (empty) output table name");

    doubleFmt = (i_doubleFormat != nullptr) ? i_doubleFormat : "";

    // get table dimensions, accumulators and aggregation expressions
    modelId = i_metaStore->modelRow->modelId;

    tableRow = i_metaStore->tableDic->byModelIdName(modelId, i_name);
    if (tableRow == nullptr) throw DbException("output table not found in tables dictionary: %s", i_name);

    tableId = tableRow->tableId;
    dimCount = tableRow->rank;

    accDbTable = tableRow->dbAccTable;
    valueDbTable = tableRow->dbExprTable;

    tableDims = i_metaStore->tableDims->byModelIdTableId(modelId, tableId);
    if (dimCount < 0 || dimCount != (int)tableDims.size()) throw DbException("invalid table rank or dimensions not found for table: %s", i_name);

    tableAcc = i_metaStore->tableAcc->byModelIdTableId(modelId, tableId);
    accCount = (int)tableAcc.size();
    if (accCount <= 0) throw DbException("output table accumulators not found for table: %s", i_name);

    tableExpr = i_metaStore->tableExpr->byModelIdTableId(modelId, tableId);
    exprCount = (int)tableExpr.size();

    // calculate total number of values for each accumulator
    // for each dimension collect all enum id's
    // include "total" enum id if total enabled for that output table dimension
    for (const TableDimsRow & dim : tableDims) {

        size_t nSize = dim.dimSize;
        if (nSize <= 0) throw DbException("invalid size of dimension %s for table: %s", dim.name.c_str(), i_name);

        // collect enum id's for that dimension
        vector<TypeEnumLstRow> de = i_metaStore->typeEnumLst->byModelIdTypeId(modelId, dim.typeId);
        if (nSize != de.size() + (dim.isTotal ? 1 : 0))
             throw DbException("invalid size of dimension %s for table: %s, expected: %zd", dim.name.c_str(), i_name, nSize);

        vector<int> idArr(nSize);
        for (size_t k = 0; k < de.size(); k++) {
            idArr[k] = de[k].enumId;
        }

        // if that output table dimension has "total" enabled then append total enum id
        if (dim.isTotal) idArr[nSize - 1] = i_metaStore->typeDic->byKey(modelId, dim.typeId)->totalEnumId;

        dimEnumIds.push_back(std::move(idArr));     // store enum id's for that dimension
    }

    totalSize = i_metaStore->accumulatorSize(*tableRow);    // total number of accumulator values as product of dimensions size, including total item
}

// calculate output table size: total number of values for each accumulator
size_t IOutputTableWriter::sizeOf(const MetaHolder * i_metaStore, int i_tableId)
{ 
    if (i_metaStore == nullptr) throw DbException("invalid (NULL) model metadata");

    // get dimensions size, including expr dimension
    int mId = i_metaStore->modelRow->modelId;

    const TableDicRow * tblRow = i_metaStore->tableDic->byKey(mId, i_tableId);
    if (tblRow == nullptr) throw DbException("table not found in table dictionary, id: %d", i_tableId);

    int dimCount = tblRow->rank;

    vector<TableDimsRow> tblDimVec = i_metaStore->tableDims->byModelIdTableId(mId, i_tableId);
    if (dimCount < 0 || dimCount != (int)tblDimVec.size()) throw DbException("invalid table rank or dimensions not found for table: %s", tblRow->tableName.c_str());

    // calculate table total size
    long long nTotal = 1;
    for (const TableDimsRow & dim : tblDimVec) {
        if (dim.dimSize <= 0) throw DbException("invalid size of dimension %s for table: %s", dim.name.c_str(), tblRow->tableName.c_str());
        nTotal *= dim.dimSize;
    }
    if (nTotal <= 0 || nTotal > INT_MAX) throw DbException("invalid size of the table: %s", tblRow->tableName.c_str());

    return (size_t)nTotal;
}

// write output table accumulator values
void OutputTableWriter::writeAccumulator(IDbExec * i_dbExec, int i_subId, int i_accId, size_t i_size, const double * i_valueArr)
{
    // validate parameters
    if (i_dbExec == nullptr) throw DbException("invalid (NULL) database connection");
    if (i_subId < 0 || i_subId >= subCount) throw DbException("invalid sub-value index: %d for output table: %s", i_subId, tableRow->tableName.c_str());
    if (i_accId < 0 || i_accId >= accCount) throw DbException("invalid accumulator number: %d for output table: %s", i_accId, tableRow->tableName.c_str());
    if (i_size <= 0 || totalSize != i_size) throw DbException("invalid value array size: %zd for output table: %s", i_size, tableRow->tableName.c_str());

    if (i_valueArr == nullptr) throw DbException("invalid value array: it can not be NULL for output table: %s", tableRow->tableName.c_str());

    // build sql:
    // INSERT INTO salarySex_a201208171604590148
    //   (run_id, acc_id, sub_id, dim0, dim1, acc_value) VALUES (2, 15, 4, ?, ?, ?)
    string sql = "INSERT INTO " + accDbTable + " (run_id, acc_id, sub_id";

    for (const TableDimsRow & dim : tableDims) {
        sql += ", " + dim.columnName();
    }

    sql += ", acc_value) VALUES (" + to_string(runId) + ", " + to_string(i_accId) + ", " + to_string(i_subId);

    // build sql, append: , ?, ?, ?)
    // as dimensions parameter placeholder(s), value placeholder
    for (int nDim = 0; nDim < dimCount; nDim++) {
        sql += ", ?";
    }
    sql += ", ?)";

    // set parameters type: dimensions and accumulator value
    vector<const type_info *> tv;
    for (int nDim = 0; nDim < dimCount; nDim++) {
        tv.push_back(&typeid(int));
    }
    tv.push_back(&typeid(double));  // set parameters type: accumulator value
    
    // begin update transaction
    unique_lock<recursive_mutex> lck = i_dbExec->beginTransactionThreaded();

    // prepare statement
    {
        exit_guard<IDbExec> onExit(i_dbExec, &IDbExec::releaseStatement);
        i_dbExec->createStatement(sql, (int)tv.size(), tv.data());

        // storage for dimension enum indexes
        unique_ptr<int[]> cellArrUptr(new int[dimCount]);
        int * cellArr = cellArrUptr.get();

        for (int k = 0; k < dimCount; k++) {
            cellArr[k] = 0;
        }

        // storage for dimension items and db row values
        int rowSize = dimCount + 1;
        unique_ptr<DbValue[]> valVecUptr(new DbValue[rowSize]);
        DbValue * valVec = valVecUptr.get();

        // loop through all dimensions and store cell values
        for (size_t cellOffset = 0; cellOffset < i_size; cellOffset++) {

            // set sql parameter values: dimension enum id by enum index
            for (int nDim = 0; nDim < dimCount; nDim++) {
                valVec[nDim] = DbValue(dimEnumIds[nDim][cellArr[nDim]]);
            }

            // if table is not "sparse" then store NULL value rows:
            // if no "sparse" flag set for that output table or value is finite and greater than "sparse null"
            if (!isSparseTable ||
                (isfinite(i_valueArr[cellOffset]) && fabs(i_valueArr[cellOffset]) > nullValue)) {

                // set parameter value: accumulator value
                valVec[dimCount] = DbValue(i_valueArr[cellOffset]);

                // insert cell value into output table
                i_dbExec->executeStatement(rowSize, valVec);
            }

            // get next cell indices
            for (int nDim = dimCount - 1; nDim >= 0; nDim--) {
                if (nDim > 0 && cellArr[nDim] >= tableDims[nDim].dimSize - 1) {
                    cellArr[nDim] = 0;
                }
                else {
                    cellArr[nDim]++;
                    break;
                }
            }
            if (cellOffset + 1 < i_size && dimCount > 0 && cellArr[0] >= tableDims[0].dimSize) throw DbException("Invalid value array size");
        }
    }   // done with insert

    // commit: done with sub-value
    i_dbExec->commit();
}

// write all output table values: aggregate sub-values using table expressions
void OutputTableWriter::writeAllExpressions(IDbExec * i_dbExec)
{
    if (i_dbExec == nullptr) throw DbException("invalid (NULL) database connection");

    unique_lock<recursive_mutex> lck = i_dbExec->beginTransactionThreaded();

    for (int nExpr = 0; nExpr < exprCount; nExpr++) {
        writeExpression(i_dbExec, nExpr);
    }
    i_dbExec->commit();
}

// write output table value: aggregated output expression value
void OutputTableWriter::writeExpression(IDbExec * i_dbExec, int i_nExpression)
{
    // validate parameters
    if (i_dbExec == nullptr) throw DbException("invalid (NULL) database connection");
    if (i_nExpression < 0 || i_nExpression >= exprCount) throw DbException("invalid expression index: %d for output table: %s", i_nExpression, tableRow->tableName.c_str());

    // check if update is in transaction scope
    if (!i_dbExec->isTransaction()) throw DbException("output table update must be in transaction scope");
    
    // build sql:
    // INSERT INTO salarySex_v201208171604590148 
    //  (run_id, dim0, dim1, expr_id, expr_value)
    // SELECT
    //  F.run_id, F.dim0, F.dim1, 2, F.expr2
    // FROM
    // (
    //  SELECT
    //    M1.run_id, M1.dim0, M1.dim1, SUM(M1.acc0) AS expr2
    //  FROM salarySex_a201208171604590148 M1
    //  GROUP BY M1.run_id, M1.dim0, M1.dim1
    // ) F
    // WHERE F.run_id = 15
    // 
    string sql = "INSERT INTO " + valueDbTable + " (run_id";

    for (const TableDimsRow & dim : tableDims) {
        sql += ", " + dim.columnName();
    }

    sql += ", expr_id, expr_value) SELECT F.run_id";

    for (const TableDimsRow & dim : tableDims) {
        sql += ", F." + dim.columnName();
    }
    sql += ", " + to_string(i_nExpression) + ", F." + tableExpr[i_nExpression].columnName() +
        " FROM (" +
        tableExpr[i_nExpression].sqlExpr +
        ") F WHERE F.run_id = " + to_string(runId);

    // do the insert
    i_dbExec->update(sql);
}

// calculate output table values digest and store only single copy of output values
void OutputTableWriter::digestOutput(IDbExec * i_dbExec)
{
    if (i_dbExec == nullptr) throw DbException("invalid (NULL) database connection");

    unique_lock<recursive_mutex> lck = i_dbExec->beginTransactionThreaded();

    // insert run_table dictionary record
    string sHid = to_string(tableRow->tableHid);
    string sRunId = to_string(runId);

    i_dbExec->update(
        "INSERT INTO run_table (run_id, table_hid, base_run_id, value_digest)" \
        " VALUES (" + sRunId + ", " + sHid + ", " + sRunId + ", NULL)"
        );

    // build sql to select accumulators and expressions values:
    //
    // SELECT acc_id, sub_id, dim0, dim1, acc_value FROM salarySex_a201208171 WHERE run_id = 11 ORDER BY 1, 2, 3, 4
    //
    string accSql = "SELECT acc_id, sub_id, ";

    for (const TableDimsRow & dim : tableDims) {
        accSql += dim.columnName() + ", ";
    }
    accSql += "acc_value FROM " + accDbTable + " WHERE run_id = " + to_string(runId);

    accSql += " ORDER BY 1, 2";
    for (int nDim = 0; nDim < dimCount; nDim++) {
        accSql += ", " + to_string(nDim + 3);
    }

    // build sql to expressions values:
    //
    // SELECT expr_id, dim0, dim1, expr_value FROM salarySex_v201208171 WHERE run_id = 11 ORDER BY 1, 2, 3
    //
    string exprSql = "SELECT expr_id, ";

    for (const TableDimsRow & dim : tableDims) {
        exprSql += dim.columnName() + ", ";
    }
    exprSql += "expr_value FROM " + valueDbTable + " WHERE run_id = " + to_string(runId);

    exprSql += " ORDER BY 1";
    for (int nDim = 0; nDim < dimCount; nDim++) {
        exprSql += ", " + to_string(nDim + 2);
    }

    // select accumulator values and calculate digest
    MD5 md5;

    // start from metadata digest
    string sLine = "table_name,table_digest\n" + tableRow->tableName + "," + tableRow->digest + "\n";
    md5.add(sLine.c_str(), sLine.length());

    // append accumulators header
    sLine = "acc_id,sub_id,";
    for (const TableDimsRow & dim : tableDims) {
        sLine += dim.name + ",";
    }
    sLine += "acc_value\n";

    md5.add(sLine.c_str(), sLine.length());

    // +2 columns: acc_id, sub_id
    ValueRowDigester md5AccRd(2 + dimCount, typeid(double), &md5, doubleFmt.c_str());
    ValueRowAdapter accAdp(2 + dimCount, typeid(double));

    i_dbExec->selectToRowProcessor(accSql, accAdp, md5AccRd);

    // select expression values and append to the digest
    sLine = "expr_id,";

    for (const TableDimsRow & dim : tableDims) {
        sLine += dim.name + ",";
    }
    sLine += "expr_value\n";

    md5.add(sLine.c_str(), sLine.length()); // append expressions header

    // +1 column: expr_id
    ValueRowDigester md5ExprRd(1 + dimCount, typeid(double), &md5, doubleFmt.c_str());
    ValueRowAdapter exprAdp(1 + dimCount, typeid(double));

    i_dbExec->selectToRowProcessor(exprSql, exprAdp, md5ExprRd);

    string sDigest = md5.getHash();     // digest of metadata and values of accumulators and expressions

    // update digest and base run id
    //
    // UPDATE run_table SET value_digest = '22ee44cc' WHERE run_id = 11 table_hid = 456
    //
    // UPDATE run_table SET 
    //   base_run_id =
    //   (
    //     SELECT MIN(E.run_id) FROM run_table E
    //     WHERE E.table_hid = 456
    //     AND E.value_digest = '22ee44cc'
    //   )
    // WHERE run_id = 11 AND table_hid = 456
    //
    i_dbExec->update(
        "UPDATE run_table SET value_digest = " + toQuoted(sDigest) + 
        " WHERE run_id = " + sRunId + " AND table_hid = " + sHid
        );

    i_dbExec->update(
        "UPDATE run_table SET base_run_id =" \
        " (" \
        " SELECT MIN(E.run_id) FROM run_table E" \
        " WHERE E.table_hid = " + sHid +
        " AND E.value_digest = " + toQuoted(sDigest) +
        " )" \
        " WHERE run_id = " + sRunId + " AND table_hid = " + sHid
        );

    // if same digest already exists then delete current run value rows
    int nBase = i_dbExec->selectToInt(
        "SELECT base_run_id FROM run_table WHERE run_id = " + sRunId + " AND table_hid = " + sHid,
        0);

    if (nBase > 0 && nBase != runId) {
        i_dbExec->update(
            "DELETE FROM " + accDbTable + " WHERE run_id = " + sRunId
            );
        i_dbExec->update(
            "DELETE FROM " + valueDbTable + " WHERE run_id = " + sRunId
            );
    }

    i_dbExec->commit();     // completed
}
