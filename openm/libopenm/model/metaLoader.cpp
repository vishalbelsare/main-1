/**
* @file
* OpenM++ modeling library: model metadata loader to read and broadcast metadata and run options.
*/
// Copyright (c) 2013-2015 OpenM++
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "model.h"

using namespace std;
using namespace openm;

namespace openm
{
    /** compatibility short name for options file name: -s fileName.ini */
    const char * RunShortKey::optionsFile = "s";

    /** short name for options file name: -ini fileName.ini */
    const char * RunShortKey::optionsFileIni = "ini";

    /** parameters started with "Parameter." treated as value of model scalar input parameters */
    const char * RunOptionsKey::parameterPrefix = "Parameter";

    /** number of sub-samples */
    const char * RunOptionsKey::subSampleCount = "General.Subsamples";

    /** number of modeling threads */
    const char * RunOptionsKey::threadCount = "General.Threads";

    /** database connection string */
    const char * RunOptionsKey::dbConnStr = "OpenM.Database";

    /** model run id */
    const char * RunOptionsKey::runId = "OpenM.RunId";

    /** working set id to get input parameters */
    const char * RunOptionsKey::setId = "OpenM.SetId";

    /** working set name to get input parameters */
    const char * RunOptionsKey::setName = "OpenM.SetName";

    /** modelling task id */
    const char * RunOptionsKey::taskId = "OpenM.TaskId";

    /** modelling task name */
    const char * RunOptionsKey::taskName = "OpenM.TaskName";

    /** profile name to get run options, default is model name */
    const char * RunOptionsKey::profile = "OpenM.OptionsProfile";

    /** use sparse output tables */
    const char * RunOptionsKey::useSparse = "OpenM.SparseOutput";

    /** sparse NULL value */
    const char * RunOptionsKey::sparseNull = "OpenM.SparseNullValue";

    /** trace log to console */
    const char * RunOptionsKey::traceToConsole = "OpenM.TraceToConsole";

    /** trace log to file */
    const char * RunOptionsKey::traceToFile = "OpenM.TraceToFile";

    /** trace log file path */
    const char * RunOptionsKey::traceFilePath = "OpenM.TraceFilePath";

    /** trace log to "stamped" file */
    const char * RunOptionsKey::traceToStamped = "OpenM.TraceToStampedFile";

    /** trace use time-stamp in log "stamped" file name */
    const char * RunOptionsKey::traceUseTs = "OpenM.TraceUseTimeStamp";

    /** trace use pid-stamp in log "stamped" file name */
    const char * RunOptionsKey::traceUsePid = "OpenM.TraceUsePidStamp";

    /** do not prefix trace log messages with date-time */
    const char * RunOptionsKey::traceNoMsgTime = "OpenM.TraceNoMsgTime";
}

/** array of model run option keys. */
static const char * runOptKeyArr[] = {
    RunOptionsKey::subSampleCount,
    RunOptionsKey::threadCount,
    RunOptionsKey::dbConnStr,
    RunOptionsKey::runId,
    RunOptionsKey::setId,
    RunOptionsKey::setName,
    RunOptionsKey::taskId,
    RunOptionsKey::taskName,
    RunOptionsKey::profile,
    RunOptionsKey::useSparse,
    RunOptionsKey::sparseNull,
    RunOptionsKey::traceToConsole,
    RunOptionsKey::traceToFile,
    RunOptionsKey::traceFilePath,
    RunOptionsKey::traceToStamped,
    RunOptionsKey::traceUseTs,
    RunOptionsKey::traceUsePid,
    RunOptionsKey::traceNoMsgTime,
    ArgKey::optionsFile,
    ArgKey::logToConsole,
    ArgKey::logToFile,
    ArgKey::logFilePath,
    ArgKey::logToStamped,
    ArgKey::logUseTs,
    ArgKey::logUsePid,
    ArgKey::logNoMsgTime,
    ArgKey::logSql
};
static const size_t runOptKeySize = sizeof(runOptKeyArr) / sizeof(const char *);

/** array of short and full option names, used to find full option name by short. */
static const pair<const char *, const char *> shortPairArr[] = 
{
    make_pair(RunShortKey::optionsFile, ArgKey::optionsFile),
    make_pair(RunShortKey::optionsFileIni, ArgKey::optionsFile)
};
static const size_t shortPairSize = sizeof(shortPairArr) / sizeof(const pair<const char *, const char *>);

/** create metadata loader, initialize run options from command line and ini-file. */
MetaLoader::MetaLoader(int argc, char ** argv) :
    modelId(0),
    subSampleCount(0),
    processCount(1),
    threadCount(1),
    taskId(0),
    taskRunId(0)
{
    // get command line options
    argStore.parseCommandLine(argc, argv, false, true, runOptKeySize, runOptKeyArr, shortPairSize, shortPairArr);

    // load options from ini-file and append parameters section
    argStore.loadIniFile(
        argStore.strOption(ArgKey::optionsFile).c_str(), runOptKeySize, runOptKeyArr, RunOptionsKey::parameterPrefix
        );

    // dependency in log options: if LogToFile is true then file name must be non-empty else must be empty
    argStore.adjustLogSettings(argc, argv);
}

/** read metadata from db, deterine number of subsamples, merge command line and ini-file options with db profile table.
*
* (a) determine number of subsamples, processes, threads
* ------------------------------------------------------
*
* Number of subsamples by default = 1 and it can be specified
* using command-line argument, ini-file or profile table in database
*
* Number of modelling threads by default = 1 and subsamples run sequentially in single thread.
* If more threads specified (i.e. by command-line argument) then subsamples run in parallel.
*
* For example: \n
*   model.exe -General.Subsamples 8 \n
*   model.exe -General.Subsamples 8 -General.Threads 4 \n
*   mpiexec -n 2 model.exe -General.Subsamples 31 -General.Threads 7
*
* If MPI is used then
*
*   number of modelling processes = MPI world size \n
*   number of subsamples per process = total number of subsamples / number of processes \n
*   if total number of subsamples % number of processes != 0 then remainder calculated at root process \n
*
* (b) load metadata tables from database and broadcast it to child processes
* --------------------------------------------------------------------------
*
* (c) merge command line and ini-file options with db profile table
*------------------------------------------------------------------
* model run options can be specified as (in order of priority):
*
*   - command line arguments, i.e.: \n
*       model.exe -Parameter.Population 1234
*
*   - through ini-file: \n
*       model.exe -s modelOne.ini
*
*   - as rows of profile_option table, default profile_name='model name'
*     profile name also can be command line argument or ini-file entry, i.e.: \n
*       model.exe -OpenM.OptionsProfile TestProfile
*
*   - some options have hard coded default values which used if nothing above is specified
*
*   run options which name starts with "Parameter." are treated as
*   value of scalar input parameter (see below). \n
*   if there is no input parameter with such name then exception raised. \n
*   for example, if command line is: \n
*       model.exe -Parameter.Population 1234 \n
*   and model does not have "Population" parameter then execution aborted.
*/
MetaRunHolder * MetaLoader::init(bool i_isMpiUsed, IDbExec * i_dbExec, IMsgExec * i_msgExec)
{
    if (i_msgExec == nullptr) throw MsgException("invalid (NULL) message passing interface");

    // create new metadata rows storage
    unique_ptr<MetaRunHolder> metaStore(new MetaRunHolder);

    // if this is main (root) process then read run options and metadata from database
    if (!i_isMpiUsed || i_msgExec->isRoot()) {
        initRoot(i_dbExec, metaStore.get());
    }

    // get main run control values: number of subsamples, processes, threads
    subSampleCount = argStore.intOption(RunOptionsKey::subSampleCount, 1);  // number of subsamples from command line or ini-file
    processCount = i_isMpiUsed ? i_msgExec->worldSize() : 1;                // number of processes: MPI world size
    threadCount = argStore.intOption(RunOptionsKey::threadCount, 1);        // max number of modeling threads

    // basic validation
    if (subSampleCount <= 0) throw ModelException("Invalid number of subsamples: %d", subSampleCount);
    if (processCount <= 0 || threadCount <= 0)
        throw ModelException("Invalid number of modelling processes: %d or threads: %d", processCount, threadCount);
    if (subSampleCount < processCount)
        throw ModelException("Error: number of subsamples: %d less than number of processes: %d", subSampleCount, processCount);

    // broadcast metadata: subsample count and meta tables from root to all other processes
    if (i_isMpiUsed) {
        broadcastMetaData(i_msgExec, metaStore.get());
    }

    return metaStore.release();
}

// initialization for main (root) process
void MetaLoader::initRoot(IDbExec * i_dbExec, MetaRunHolder * io_metaStore)
{
    // validate arguments: it must be root process and valid db connection
    if (i_dbExec == nullptr) throw ModelException("invalid (NULL) database connection");

    // load metadata table rows, except of run_option, which is may not created yet
    const ModelDicRow * mdRow = readMetaTables(i_dbExec, io_metaStore);

    // merge command line and ini-file arguments with profile_option table values
    mergeProfile(i_dbExec, mdRow, io_metaStore);

    // if this is modelling task then find it in database
    findTask(i_dbExec, mdRow);

    if (taskId > 0) {
        readTask(i_dbExec, mdRow);

        // create task run entry in database
        createTaskRun(i_dbExec);
    }
}

// read metadata tables from db, except of run_option table
const ModelDicRow * MetaLoader::readMetaTables(IDbExec * i_dbExec, MetaRunHolder * io_metaStore)
{
    // find model and set model id
    io_metaStore->modelDic.reset(IModelDicTable::create(i_dbExec, OM_MODEL_NAME, OM_MODEL_TIMESTAMP));

    const ModelDicRow * mdRow = io_metaStore->modelDic->byNameTimeStamp(OM_MODEL_NAME, OM_MODEL_TIMESTAMP);
    if (mdRow == nullptr) throw DbException("model %s not found in the database", OM_MODEL_NAME);

    modelId = mdRow->modelId;

    // read metadata tables
    io_metaStore->typeDic.reset(ITypeDicTable::create(i_dbExec, modelId));
    io_metaStore->typeEnumLst.reset(ITypeEnumLstTable::create(i_dbExec, modelId));
    io_metaStore->paramDic.reset(IParamDicTable::create(i_dbExec, modelId));
    io_metaStore->paramDims.reset(IParamDimsTable::create(i_dbExec, modelId));
    io_metaStore->tableDic.reset(ITableDicTable::create(i_dbExec, modelId));
    io_metaStore->tableDims.reset(ITableDimsTable::create(i_dbExec, modelId));
    io_metaStore->tableAcc.reset(ITableAccTable::create(i_dbExec, modelId));
    io_metaStore->tableExpr.reset(ITableExprTable::create(i_dbExec, modelId));

    return mdRow;
}

// broadcast metadata: run id, subsample count, subsample number and meta tables from root to all modelling processes
void MetaLoader::broadcastMetaData(IMsgExec * i_msgExec, MetaRunHolder * io_metaStore)
{
    if (i_msgExec == nullptr) throw ModelException("invalid (NULL) message passing interface");

    // broadcast model id, run id, subsample count and thread from root to all model processes
    i_msgExec->bcast(i_msgExec->rootRank, typeid(int), 1, &modelId);
    i_msgExec->bcast(i_msgExec->rootRank, typeid(int), 1, &subSampleCount);
    i_msgExec->bcast(i_msgExec->rootRank, typeid(int), 1, &threadCount);

    // broadcast metadata tables
    broadcastMetaTable<IModelDicTable>(io_metaStore->modelDic, i_msgExec, MsgTag::modelDic);
    broadcastMetaTable<ITypeDicTable>(io_metaStore->typeDic, i_msgExec, MsgTag::typeDic);
    broadcastMetaTable<ITypeEnumLstTable>(io_metaStore->typeEnumLst, i_msgExec, MsgTag::typeEnumLst);
    broadcastMetaTable<IParamDicTable>(io_metaStore->paramDic, i_msgExec, MsgTag::parameterDic);
    broadcastMetaTable<IParamDimsTable>(io_metaStore->paramDims, i_msgExec, MsgTag::parameterDims);
    broadcastMetaTable<ITableDicTable>(io_metaStore->tableDic, i_msgExec, MsgTag::tableDic);
    broadcastMetaTable<ITableDimsTable>(io_metaStore->tableDims, i_msgExec, MsgTag::tableDims);
    broadcastMetaTable<ITableAccTable>(io_metaStore->tableAcc, i_msgExec, MsgTag::tableAcc);
    broadcastMetaTable<ITableExprTable>(io_metaStore->tableExpr, i_msgExec, MsgTag::tableExpr);
}

// temporary: work-in-progress
// broadcast run id from root to all modelling processes
void MetaLoader::broadcastRunId(IMsgExec * i_msgExec, int * io_runId)
{
    if (i_msgExec == nullptr) throw ModelException("invalid (NULL) message passing interface");

    i_msgExec->bcast(i_msgExec->rootRank, typeid(int), 1, io_runId);
}

// temporary: work-in-progress
// broadcast run options from root to all modelling processes
void MetaLoader::broadcastRunOptions(IMsgExec * i_msgExec, unique_ptr<IRunOptionTable> & i_runOptionTbl)
{
    if (i_msgExec == nullptr) throw ModelException("invalid (NULL) message passing interface");

    broadcastMetaTable<IRunOptionTable>(i_runOptionTbl, i_msgExec, MsgTag::runOption);
}

// broadcast meta table db rows
template <typename MetaTbl>
void MetaLoader::broadcastMetaTable(unique_ptr<MetaTbl> & i_tableUptr, IMsgExec * i_msgExec, MsgTag i_msgTag)
{
    unique_ptr<IPackedAdapter> packAdp(IPackedAdapter::create(i_msgTag));

    if (i_msgExec->isRoot()) {
        IRowBaseVec & rv = i_tableUptr->rowsRef();
        i_msgExec->bcastPacked(i_msgExec->rootRank, rv, *packAdp);
    }
    else {
        IRowBaseVec rv;
        i_msgExec->bcastPacked(i_msgExec->rootRank, rv, *packAdp);
        i_tableUptr.reset(MetaTbl::create(rv));
    }
}

// merge command line and ini-file arguments with profile_option table values
// use default values for basic run options, i.e. SparseOutput = false
// raise exception if any of "Parameter." run option name 
// is not in the list of model input parameters or is not scalar
void MetaLoader::mergeProfile(
    IDbExec * i_dbExec, const ModelDicRow * i_mdRow, const MetaRunHolder * i_metaStore
    )
{
    // hard-coded default run options
    RunOptions emptyOpts;
    NoCaseMap defaultOpt;

    defaultOpt[RunOptionsKey::useSparse] = emptyOpts.useSparse ? "true" : "false";
    defaultOpt[RunOptionsKey::sparseNull] = toString(emptyOpts.nullValue);
    defaultOpt[RunOptionsKey::threadCount] = "1";

    // load default run options from profile options, default profile name = model name
    string profileName = argStore.strOption(RunOptionsKey::profile, OM_MODEL_NAME);
    unique_ptr<IProfileOptionTable> profileTbl(
        IProfileOptionTable::create(i_dbExec, profileName)
        );

    // update run options: merge command line and ini-file with profile and hard-coded default values
    for (size_t nOpt = 0; nOpt < runOptKeySize; nOpt++) {

        if (argStore.isOptionExist(runOptKeyArr[nOpt])) continue;   // option specified at command line or ini-file

        // find option in profile_option table
        const ProfileOptionRow * optRow = profileTbl->byKey(profileName, runOptKeyArr[nOpt]);
        if (optRow != nullptr) {
            argStore.args[runOptKeyArr[nOpt]] = optRow->value;      // add option from database
        }
        else {  // no database value for that option key, use hard-coded default
            NoCaseMap::const_iterator defIt = defaultOpt.find(runOptKeyArr[nOpt]);
            if (defIt != defaultOpt.cend()) argStore.args[runOptKeyArr[nOpt]] = defIt->second;
        }
    }

    // update "Parameter." options: merge command line and ini-file with profile table
    vector<ParamDicRow> paramVec = i_metaStore->paramDic->rows();

    for (vector<ParamDicRow>::const_iterator paramIt = paramVec.cbegin(); paramIt != paramVec.cend(); ++paramIt) {

        string argName = string(RunOptionsKey::parameterPrefix) + "." + paramIt->paramName;

        if (argStore.isOptionExist(argName.c_str())) continue;  // parameter specified at command line or ini-file

        // find option in profile_option table
        const ProfileOptionRow * optRow = profileTbl->byKey(profileName, argName.c_str());
        if (optRow != nullptr) {
            argStore.args[argName] = optRow->value;             // add option from database
        }
    }

    // validate "Parameter." options: it must be name of scalar input parameter
    string parPrefix = string(RunOptionsKey::parameterPrefix) + ".";
    size_t nPrefix = parPrefix.length();

    for (NoCaseMap::const_iterator propIt = argStore.args.cbegin(); propIt != argStore.args.cend(); propIt++) {

        if (!equalNoCase(propIt->first.c_str(), parPrefix.c_str(), nPrefix)) continue;  // it is not a "Parameter."

        if (propIt->first.length() <= nPrefix) throw ModelException("invalid (empty) parameter name argument specified");

        // find parameter by name: it must be a model parameter
        string sName = propIt->first.substr(nPrefix);

        const ParamDicRow * paramRow = i_metaStore->paramDic->byModelIdName(i_mdRow->modelId, sName);
        if (paramRow == nullptr)
            throw DbException("parameter %s is not an input parameter of model %s, id: %d", sName.c_str(), i_mdRow->name.c_str(), i_mdRow->modelId);

        // check the rank: it must scalar
        if (paramRow->rank != 0) throw DbException("parameter %s is not a scalar", sName.c_str());

        // parameter value can not be empty
        if (propIt->second.empty()) throw ModelException("invalid (empty) value specified for parameter %s", sName.c_str());
    }
}

// read modelling task definition
void MetaLoader::readTask(IDbExec * i_dbExec, const ModelDicRow * i_mdRow)
{
    // initialize pairs of (set, run) for that task
    vector<TaskSetRow> taskSetRows = ITaskSetTable::select(i_dbExec, taskId);
    if (taskSetRows.size() <= 0) throw DbException("no input data sets found for the task: %d of model %s, id: %d", taskId, i_mdRow->name.c_str(), i_mdRow->modelId);

    for (const TaskSetRow & tsRow : taskSetRows) {
        taskRunLst.push_back(TaskItem(tsRow.setId));
    }
}

// create task run entry in database
void MetaLoader::createTaskRun(IDbExec * i_dbExec) 
{ 
    // update in transaction scope
    unique_lock<recursive_mutex> lck = i_dbExec->beginTransactionThreaded();

    // get next task run id
    i_dbExec->update("UPDATE id_lst SET id_value = id_value + 1 WHERE id_key = 'task_run_id'");

    taskRunId = i_dbExec->selectToInt("SELECT id_value FROM id_lst WHERE id_key = 'task_run_id'", 0);
    if (taskRunId <= 0)
        throw DbException("invalid task run id: %d", taskRunId);

    string dtStr = toDateTimeString(theLog->timeStampSuffix()); // get log date-time as string

    // create new run
    i_dbExec->update(
        "INSERT INTO task_run_lst (task_run_id, task_id, sub_count, create_dt, status, update_dt)" \
        " VALUES (" +
        to_string(taskRunId) + ", " +
        to_string(taskId) + ", " +
        to_string(subSampleCount) + ", " +
        toQuoted(dtStr) + ", "
        "'i', " +
        toQuoted(dtStr) + ")"
        );

    // completed: commit the changes
    i_dbExec->commit();
}

// find modelling task, if specified
void MetaLoader::findTask(IDbExec * i_dbExec, const ModelDicRow * i_mdRow)
{
    // find task id or name if specified as run options
    taskId = argStore.intOption(RunOptionsKey::taskId, 0);
    string taskName = argStore.strOption(RunOptionsKey::taskName);

    if (taskId > 0) {
        int cnt = i_dbExec->selectToInt(
            "SELECT COUNT(*) FROM task_lst WHERE task_id = " + to_string(taskId), 0
            );
        if (cnt <= 0) throw DbException("task id not found in database: %d", taskId);
    }
    else {  // find task id by name, if taskName option specified 

        if (!taskName.empty()) {
            taskId = i_dbExec->selectToInt(
                "SELECT MIN(task_id) FROM task_lst WHERE model_id = " + to_string(i_mdRow->modelId) +
                " AND task_name = " + toQuoted(taskName),
                0);
            if (taskId <= 0)
                throw DbException("model %s, id: %d does not contain task with name: %s", i_mdRow->name.c_str(), i_mdRow->modelId, taskName.c_str());
        }
    }

    // if task found then add it to run options
    if (taskId > 0) {

        if (taskName.empty()) {
            taskName = i_dbExec->selectToStr(
                "SELECT task_name FROM task_lst WHERE task_id = " + to_string(taskId)
                );
        }

        argStore.args[RunOptionsKey::taskId] = to_string(taskId);
        argStore.args[RunOptionsKey::taskName] = taskName;
    }
}

// find id and name of source working set for input parameters:
//   if set id specified as run option then use such set id
//   if set name specified as run option then find set id by name
//   else use min(set id) as default set of model parameters
int MetaLoader::findWorkset(IDbExec * i_dbExec, const ModelDicRow * i_mdRow)
{
    // find set id of parameters workset, default is first set id for that model
    int setId = argStore.intOption(RunOptionsKey::setId, 0);
    string setName = argStore.strOption(RunOptionsKey::setName);

    if (setId > 0) {
        int cnt = i_dbExec->selectToInt(
            "SELECT COUNT(*) FROM workset_lst WHERE set_id = " + to_string(setId), 0
            );
        if (cnt <= 0) throw DbException("working set id not found in database: %d", setId);
    }
    else {  // find set id by name, if setName option specified 

        if (!setName.empty()) {
            setId = i_dbExec->selectToInt(
                "SELECT MIN(set_id) FROM workset_lst WHERE model_id = " + to_string(i_mdRow->modelId) +
                " AND set_name = " + toQuoted(setName),
                0);
            if (setId <= 0)
                throw DbException("model %s, id: %d does not contain working set with name: %s", i_mdRow->name.c_str(), i_mdRow->modelId, setName.c_str());
        }
    }

    // if set id not defined then use default working set for the model
    if (setId <= 0) {
        setId = i_dbExec->selectToInt(
            "SELECT MIN(set_id) FROM workset_lst WHERE model_id = " + to_string(i_mdRow->modelId), 0
            );
        if (setId <= 0)
            throw DbException("model %s, id: %d must have at least one working set", i_mdRow->name.c_str(), i_mdRow->modelId);
    }

    // find working set name, if not explictly specified
    if (setName.empty()) {
        setName = i_dbExec->selectToStr(
            "SELECT set_name FROM workset_lst WHERE set_id = " + to_string(setId)
            );
    }

    argStore.args[RunOptionsKey::setId] = to_string(setId);     // add set id to run options
    argStore.args[RunOptionsKey::setName] = setName;            // add set name to run options
    return setId;
}