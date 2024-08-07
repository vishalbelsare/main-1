#!/bin/bash
#
# Xcode project to build models: compile model *.ompp and *.mpp using omc compiler
#
set -x
set -e

echo Compile model: ${MODEL_NAME}

OMC_EXE=${OM_ROOT}/bin/omc

if [ ! -x ${OMC_EXE} ]; then
  echo Omc compiler not found: ${OMC_EXE}
  exit 1
fi

OMC_SCENARIO_OPT=
if [ -n ${OMC_SCENARIO_PARAM_DIR} ]; then
  OMC_SCENARIO_OPT="-p ${OMC_SCENARIO_PARAM_DIR}"
fi

OMC_FIXED_OPT=
if [ -d ${OMC_FIXED_PARAM_DIR} ] && [ -n "$(ls ${OMC_FIXED_PARAM_DIR})" ]; then
  OMC_FIXED_OPT="-f ${OMC_FIXED_PARAM_DIR}"
fi

OMC_CODE_PAGE_OPT=
if [ -n "$OMC_CODE_PAGE" ]; then
  OMC_CODE_PAGE_OPT="-Omc.CodePage ${OMC_CODE_PAGE}"
fi

OMC_NO_LINE_OPT=
if [ -n "$OMC_NO_LINE" ]; then
  OMC_NO_LINE_OPT="-Omc.NoLineDirectives ${OMC_NO_LINE}"
fi

# if not disabled then create model documentation
#
OMC_MODEL_DOC_OPTS=
if [ -z "${MODEL_DOC_DISABLE}" ]; then

  [ -z "${MODEL_INDOC_DIR}" ]  && MODEL_INDOC_DIR="doc"
  [ -z "${MODEL_OUTDOC_DIR}" ] && MODEL_OUTDOC_DIR="${TARGET_BUILD_DIR}/doc"

  OMC_MODEL_DOC_OPTS="-Omc.ModelDoc true -d ${MODEL_INDOC_DIR} -omc.OutDocDir ${MODEL_OUTDOC_DIR}"

  if [ ! -d "${MODEL_OUTDOC_DIR}" ]; then
    mkdir "${MODEL_OUTDOC_DIR}"
  fi
fi

# do omc compile

${OMC_EXE} \
 -m ${MODEL_NAME} \
 -s ${SCENARIO_NAME} \
 -i ${MODEL_CODE_DIR} \
 -o ${OMC_OUT_DIR} \
 -u ${OMC_USE_DIR} \
 -Omc.SqlDir ${OM_SQL_DIR} \
 ${OMC_SCENARIO_OPT} \
 ${OMC_FIXED_OPT} \
 ${OMC_NO_LINE_OPT} \
 ${OMC_CODE_PAGE_OPT} \
 ${OMC_MODEL_DOC_OPTS}

# done
