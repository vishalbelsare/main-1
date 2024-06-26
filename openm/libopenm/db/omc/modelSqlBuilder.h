/**
 * @file
 * OpenM++ data library: class to produce sql creation script for new model
 */
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#ifndef MODEL_SQL_BUILDER_H
#define MODEL_SQL_BUILDER_H

#include <fstream>
#include <streambuf>
#include "helper.h"
#include "crc32.h"
#include "md5.h"
#include "dbExec.h"
#include "libopenm/common/omFile.h"
#include "libopenm/common/iniReader.h"
#include "libopenm/db/modelBuilder.h"
#include "dbMetaTable.h"
#include "modelSqlWriter.h"
#include "modelInsertSql.h"
#include "modelExpressionSql.h"

using namespace std;

namespace openm
{
    /** class to build model creation sql from metadata rows. */
    class ModelSqlBuilder : public IModelBuilder
    {
    public:
        /** create new model builder. */
        ModelSqlBuilder(const string & i_providerNames, const string & i_sqlDir, const string & i_outputDir, const string & i_sqliteDir);

        /** release builder resources. */
        ~ModelSqlBuilder() noexcept { }

        /** if true then create SQLite database */
        bool isSqliteDb(void) const override { return isSqlite; }

        /** set meta rows values and calculate digests for types, parameters, tables and model */
        void setModelMetaRows(MetaModelHolder & io_metaRows) const override;

        /** update metadata and write sql script to create new model from supplied metadata rows */
        void build(MetaModelHolder & io_metaRows) override;

        /** start sql script to create new working set */
        void beginWorkset(const MetaModelHolder & i_metaRows, MetaSetLangHolder & io_metaSet) override;

        /** append scalar parameter value to sql script for new working set creation */
        void addWorksetParameter(
            const MetaModelHolder & i_metaRows, const MetaSetLangHolder & i_metaSet, const string & i_name, int i_subId, const string & i_value
        ) override;

        /** append parameter values to sql script for new working set creation */
        void addWorksetParameter(
            const MetaModelHolder & i_metaRows, const MetaSetLangHolder & i_metaSet, const string & i_name, int i_subId, const list<string> & i_valueLst
        ) override;

        /** finish sql script to create new working set */
        void endWorkset(const MetaModelHolder & i_metaRows, const MetaSetLangHolder & i_metaSet) override;

        /** write sql script to create backward compatibility views */
        void buildCompatibilityViews(const MetaModelHolder & i_metaRows) const override;

    private:
        string sqlDir;                  // sql scripts directory to create database
        string sourceDir;               // source code directory
        string outputDir;               // output directory to write sql script files
        string sqliteDir;               // output directory to create model.sqlite database
        bool isSqlite;                  // if true then create SQLite database
        list<string> dbProviderLst;     // list of db provider names, ie: sqlite,postgresql,mysql
        unique_ptr<IDbExec> dbExec;     // database connection

        /** helper struct to collect info for db table */
        struct DbTblInfo
        {
            /** model id */
            int modelId;

            /** parameter id or output table id */
            int id;

            /** parameter name or output table name */
            string name;

            /** dimension names */
            vector<string> dimVec;

            /** dimension column names */
            vector<string> colVec;

            /** dimension type id's */
            vector<int> typeIdVec;

            DbTblInfo(void) : id(-1) { }
        };

        /** helper struct to collect info for db parameter table */
        struct ParamTblInfo : DbTblInfo
        {
            /** parameter value type name */
            vector<TypeDicRow>::const_iterator valueTypeIt;

            /** if true then parameter added to workset */
            bool isAdded;

            /** if true then parameter value can be NULL */
            bool isNullable;
        };

        /** helper struct to collect info for db accumulators table and value view */
        struct OutTblInfo : DbTblInfo
        {
            /** accumulator ids */
            vector<int> accIdVec;

            /** accumulator names */
            vector<string> accNameVec;

            /** accumulator columns */
            vector<string> accColVec;
        };

        /** vector of db parameter tables info */
        vector<ParamTblInfo> paramInfoVec;

        /** vector of db accumulator tables and value views info */
        vector<OutTblInfo> outInfoVec;

        /** sort and validate metadata rows for uniqueness and referential integrity */
        void prepare(MetaModelHolder & io_metaRows) const;

        /** write sql script to insert new model metadata */
        void buildCreateModel(const string & i_sqlProvider, const MetaModelHolder & i_metaRows, const string & i_filePath) const;

        /** insert new model metadata into database and update id's with actual db values */
        static void createModel(IDbExec * i_dbExec, MetaModelHolder & io_metaRows);

        /** write sql script to create new model tables and views */
        void buildCreateModelTables(const string & i_sqlProvider, const MetaModelHolder & i_metaRows, const string & i_filePath) const;

        /** write sql script to drop model tables and views */
        void buildDropModelTables(const MetaModelHolder & i_metaRows, const string & i_filePath) const;

        /** sort and validate workset metadata for uniqueness and referential integrity */
        void prepareWorkset(const MetaModelHolder & i_metaRows, MetaSetLangHolder & io_metaSet) const;

        /** impelementation of append scalar parameter value to sql script */
        void doAddScalarWorksetParameter(
            int i_setId,
            const string & i_name, 
            const string & i_dbTableName, 
            const vector<ParamTblInfo>::const_iterator & i_paramInfo,
            int i_subId,
            const string & i_value
        );

        /** create table sql for parameter */
        const void paramCreateTable(
            const string & i_sqlProvider, 
            const string & i_dbTableName, 
            const string & i_runSetId, 
            const ParamTblInfo & i_tblInfo, 
            ModelSqlWriter & io_wr
        ) const;

        /** create table sql for accumulator table */
        const void accCreateTable(
            const string & i_sqlProvider, 
            const string & i_dbTableName, 
            const OutTblInfo & i_tblInfo, 
            ModelSqlWriter & io_wr
        ) const;

        /** create table sql for value table: aggregated table expressions */
        const void valueCreateTable(
            const string & i_sqlProvider, 
            const string & i_dbTableName, 
            const OutTblInfo & i_tblInfo, 
            ModelSqlWriter & io_wr
        ) const;

        /** create sql for all accumulators view: */
        const void accAllCreateView(
            const string & i_sqlProvider, 
            const string & i_viewName, 
            const OutTblInfo & i_tblInfo,
            const string & i_accTablName,
            const vector<TableAccRow> & i_accVec,
            ModelSqlWriter & io_wr
        ) const;

        /** create view sql for parameter compatibility view */
        const void paramCompatibilityView(
            const string & i_sqlProvider, 
            const ModelDicRow & i_modelRow,
            const string & i_viewName, 
            const string & i_srcTableName, 
            const vector<string> & i_colNames,
            const vector<int> & i_dimsLowerBound,
            int i_valueLowerBound,
            ModelSqlWriter & io_wr
        ) const;

        /** create view sql for output table compatibility view */
        const void outputCompatibilityView(
            const string & i_sqlProvider, 
            const ModelDicRow & i_modelRow, 
            const string & i_viewName, 
            const string & i_srcTableName, 
            const vector<string> & i_colNames,
            const vector<int> & i_dimsLowerBound,
            ModelSqlWriter & io_wr
        ) const;

        /** set field values for workset_lst table row */
        void setWorksetRow(WorksetLstRow & io_wsRow) const;

        /** setup type rows, i.e. type digest */
        void setTypeDicRows(MetaModelHolder & io_metaRows) const;

        /** collect info for db parameter tables */
        void setParamTableInfo(MetaModelHolder & io_metaRows);

        /** collect info for db accumulator tables and value views */
        void setOutTableInfo(MetaModelHolder & io_metaRows);

        /** set parameter meta rows and calculate digests */
        void setParamDicRows(MetaModelHolder & io_metaRows) const;

        /** set output tables meta rows and calculate digests */
        void setTableDicRows(MetaModelHolder & io_metaRows) const;

        /** set entity meta rows and calculate digests */
        void setEntityDicRows(MetaModelHolder& io_metaRows) const;

        /** set default model profile rows */
        void setModelProfileRows(MetaModelHolder & io_metaRows) const;

        /** trim string fields in model metadata rows */
        static void trimModelRows(MetaModelHolder & io_metaRows);

        /** sort model metadata rows */
        static void sortModelRows(MetaModelHolder & io_metaRows);

        /** sort workset metadata rows */
        static void sortWorksetRows(MetaSetLangHolder & io_metaSet);

        /** set names for dimension, accumulator, expression columns */
        static void setColumnNames(MetaModelHolder & io_metaRows);

        /** set field values for model_dic table row */
        void setModelDicRow(ModelDicRow & io_mdRow) const;

        /** calculate type digest */
        static const string makeTypeDigest(const TypeDicRow & i_typeRow, const MetaModelHolder & i_metaRows);

        /** calculate parameter metadata digest and parameter import digest */
        static const tuple<string, string> makeParamDigest(const ParamDicRow & i_paramRow, const MetaModelHolder & i_metaRows);
        
        /** calculate output table metadata digest */
        static const tuple<string, string> makeOutTableDigest(const TableDicRow i_tableRow, const MetaModelHolder & i_metaRows);

        /** calculate entity metadata digest */
        static const string makeEntityDigest(const EntityDicRow i_entityRow, const MetaModelHolder& i_metaRows);

        /** calculate model metadata digest and table import digest */
        static const string makeModelDigest(const MetaModelHolder & i_metaRows);
    };
}

#endif  // MODEL_SQL_BUILDER_H
