# platform name: Linux or Darwin
PLATFORM_UNAME := $(shell uname -s)

ifeq ($(OM_MSG_USE), MPI)
  CXX = mpic++
  CC = mpicc
  OM_MSG_DEF = OM_MSG_MPI
  MSG_POSTFIX = _mpi
else
  CXX = g++
  CC = gcc
  OM_MSG_DEF = OM_MSG_EMPTY
  MSG_POSTFIX =
endif
CPP = $(CC)
AR = ar

ifndef OUT_PREFIX
  OUT_PREFIX = ompp-linux
  ifeq ($(PLATFORM_UNAME), Darwin)
    OUT_PREFIX = ompp-mac
  endif
endif

ifndef OM_ROOT
  OM_ROOT = ../..
endif

OM_INC_DIR = $(OM_ROOT)/include
OM_BIN_DIR = $(OM_ROOT)/bin
OM_LIB_DIR = $(OM_ROOT)/lib
OM_SQL_DIR = $(OM_ROOT)/sql
OM_SQLITE_DIR = $(OM_SQL_DIR)/sqlite
OM_MODELS_DIR = $(OM_ROOT)/models

#
# model name: current dir name by default
#
CUR_SUBDIR = $(notdir $(CURDIR))
ifndef MODEL_NAME
  MODEL_NAME = $(CUR_SUBDIR)
endif

#
# model ini file to run the model
#
RUN_OPT_INI =
ifdef MODEL_INI
  RUN_OPT_INI = -ini $(MODEL_INI)
endif

# set model root, for example: OM_modelOne=/home/user/modelOne
#
model_root_name = OM_$(MODEL_NAME)
model_root_value = $(CURDIR)

#
# build directories
# if model build directory defined globally 
#   then assume shared some/build/ location and use model name to avoid conflicts
#
ifndef BUILD_DIR
  MODEL_BUILD_DIR = $(OUT_PREFIX)/build
else
  MODEL_BUILD_DIR = $(BUILD_DIR)/$(CUR_SUBDIR)
endif

ifndef RELEASE
  BD_CFLAGS = -g -D_DEBUG -Og
  ifeq ($(PLATFORM_UNAME), Linux)
    BD_CFLAGS = -g -D_DEBUG -O0
  endif
  DEPS_DIR = $(MODEL_BUILD_DIR)/debug/deps
  OBJ_DIR = $(MODEL_BUILD_DIR)/debug/obj
  BIN_POSTFIX = D
else
  BD_CFLAGS = -DNDEBUG -O3
  DEPS_DIR = $(MODEL_BUILD_DIR)/release/deps
  OBJ_DIR = $(MODEL_BUILD_DIR)/release/obj
  BIN_POSTFIX =
endif

OUT_BIN_DIR = $(OUT_PREFIX)/bin

ifndef OM_DB_LIB
#  OM_DB_LIB = sqlite3
  OM_DB_LIB = sqlite$(BIN_POSTFIX)
endif

LIBOPENM_A = libopenm$(BIN_POSTFIX)$(MSG_POSTFIX).a
LIBSQLITE_A = libsqlite$(BIN_POSTFIX).a

L_UCVT_FLAG =
STDC_FS_LIB = stdc++fs
WL_PIE_FLAG = -pie
INFO_PLIST_FLAG =
ifeq ($(PLATFORM_UNAME), Darwin)
  L_UCVT_FLAG = -liconv
  STDC_FS_LIB = stdc++
  WL_PIE_FLAG = -Wl,-pie
  ifdef RELEASE
    INFO_PLIST_FLAG = -sectcreate __TEXT __info_plist $(MODEL_BUILD_DIR)/Info.plist
  endif
  ifndef BUNDLE_VERSION
    BUNDLE_VERSION = 0.0.1
  endif
endif

# address sanitizer
CC_ASAN_FLAGS =
LD_ASAN_FLAGS =
ifdef USE_ASAN
  CC_ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer
  LD_ASAN_FLAGS = -fsanitize=address
endif

# recognize dependency files
SUFFIXES += .d

CXXFLAGS = -Wall -std=c++20 -pthread -fPIE -I$(OM_INC_DIR) $(CC_ASAN_FLAGS) $(BD_CFLAGS)
CPPFLAGS = $(CXXFLAGS)

MODEL_EXE = $(MODEL_NAME)$(BIN_POSTFIX)$(MSG_POSTFIX)

#MODEL_PATH = $(CURDIR)
MODEL_CPPLIST = \
    modelOne.cpp \
    modelOne_om.cpp

sources = $(MODEL_CPPLIST)

OBJS := $(foreach root,$(sources:.cpp=.o),$(OBJ_DIR)/$(root))
DEPS := $(foreach root,$(sources:.cpp=.d),$(DEPS_DIR)/$(root))

# vpath %.cpp $(MODEL_PATH)

.PHONY : all
all: model

.PHONY : model
model: prepare $(OUT_BIN_DIR)/$(MODEL_EXE)

.PHONY : install
install : all publish

$(OBJS): | prepare
$(DEPS): | prepare

$(DEPS_DIR)/%.d : %.cpp
	$(CPP) -MM $(CPPFLAGS) $< -MF $@

$(OBJ_DIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_BIN_DIR)/$(MODEL_EXE) : $(OBJS) $(OM_LIB_DIR)/$(LIBOPENM_A) $(OM_LIB_DIR)/$(LIBSQLITE_A)
	$(CXX) \
	-pthread -L$(OM_LIB_DIR) $(WL_PIE_FLAG) $(LD_ASAN_FLAGS) $(INFO_PLIST_FLAG) \
	-o $@ \
	$(OBJS) \
	-lopenm$(BIN_POSTFIX)$(MSG_POSTFIX) -l$(OM_DB_LIB) -l$(STDC_FS_LIB) $(L_UCVT_FLAG)

#
# create model SQLite database
#
SQLITE_EXE = sqlite3
MODEL_SQLITE = $(OUT_BIN_DIR)/$(MODEL_NAME).sqlite

.PHONY : publish
publish : $(MODEL_SQLITE) copy_ini

$(MODEL_SQLITE) : \
  $(OM_SQL_DIR)/create_db.sql \
  $(MODEL_NAME)_create_model.sql \
  $(MODEL_NAME)_create_entity.sql \
  $(MODEL_NAME)_insert_param.sql
	rm -f $(MODEL_SQLITE)
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(OM_SQL_DIR)/create_db.sql
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(OM_SQL_DIR)/insert_default.sql
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(MODEL_NAME)_create_model.sql
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(MODEL_NAME)_create_entity.sql
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(MODEL_NAME)_insert_param.sql

.PHONY : publish-views
publish-views : \
  $(MODEL_SQLITE) \
  $(OM_SQLITE_DIR)/optional_meta_views_sqlite.sql \
  $(MODEL_NAME)_optional_views.sql
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(OM_SQLITE_DIR)/optional_meta_views_sqlite.sql
	$(SQLITE_EXE) $(MODEL_SQLITE) < $(MODEL_NAME)_optional_views.sql

.PHONY : copy_ini
copy_ini:
	@if [ -e modelOne.ini ] ; then cp -pf modelOne.ini $(OUT_BIN_DIR) ; fi
	@if [ -e modelOne.message.ini ] ; then cp -pf modelOne.message.ini $(OUT_BIN_DIR) ; fi

#
# run the model
#
.PHONY : run
run:
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.RunName Default
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-OpenM.Profile modelOne \
		-OpenM.RunName Default-4
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-Parameter.baseSalary Full \
		-OpenM.RunName "Default and Full base salary"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.TaskName taskOne \
		-OpenM.TaskRunName "First Task Run"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 2 -SubFrom.salaryFull csv -OpenM.ParamDir ../../csv \
		-OpenM.RunName "Sub-values_2_from_csv" \
		-EN.RunDescription "Parameter sub-values 2 from csv" \
		-FR.RunDescription "(FR) Parameter sub-values 2 from csv" \
		-EN.RunNotesPath ../../run_notes-en.md \
		-FR.RunNotesPath ../../run_notes-fr.md
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-SubFrom.baseSalary db -SubFrom.salaryFull db -SubFrom.filePath db -SubFrom.isOldAge db \
		-OpenM.RunName "Sub-values_4" \
		-EN.RunDescription "Parameter sub-values 4"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 2 \
		-SubFrom.AgeSexParameters csv -SubValues.AgeSexParameters 2,3 -OpenM.ParamDir ../../csv -SubFrom.salaryFull db \
		-OpenM.RunName "Group_sub-values_2_from_csv" \
		-EN.RunDescription "Parameter group sub-values 2 from csv"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-ImportRunName.modelOne Default \
		-OpenM.RunName "Import_from_Default_run" \
		-EN.RunDescription "Import parameters from Default run"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.BaseRunName "Sub-values_2_from_csv" \
		-OpenM.SubValues 2 -SubFrom.salaryFull db \
		-OpenM.RunName "Base_run_is_Sub-values_2_from_csv" \
		-EN.RunDescription "Parameters from base run Sub-values_2_from_csv"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.BaseRunName "Sub-values_2_from_csv" \
		-OpenM.SetName modelOne_partial \
		-OpenM.RunName "Base_run_and_partial_input_set" \
		-EN.RunDescription "Parameters from base run and from partial input set"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.TaskName taskOne \
		-OpenM.TaskRunName "Task Run with Suppressed Tables" \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Tables.Suppress ageSexIncome,AdditionalTables
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.TaskName taskOne \
		-OpenM.TaskRunName "Task Run with NotSuppressed Tables" \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Tables.Retain ageSexIncome,AdditionalTables
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToDb -Microdata.All \
		-OpenM.RunName "Microdata in database" \
		-EN.RunDescription "Write microdata into database"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToCsv -Microdata.All -Microdata.Events Birth,Retire,Death \
		-OpenM.RunName "Microdata in CSV" \
		-EN.RunDescription "Write microdata into CSV files"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SetName modelOne_other \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToDb -Microdata.All \
		-OpenM.RunName "Microdata other in database" \
		-EN.RunDescription "Write other microdata into database"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToDb -Microdata.All \
		-Parameter.StartingSeed 2047 \
		-OpenM.RunName "Microdata version in database" \
		-EN.RunDescription "Write another microdata version into database"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToDb -Microdata.Person Sex,Age,Income \
		-OpenM.RunName "Microdata Person Age,Sex,Income in database" \
		-EN.RunDescription "Write microdata Person Age,Sex,Income into database"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToDb -Microdata.Person crc,Sex,Age,Age,Income -Microdata.UseInternal \
		-OpenM.RunName "Microdata Person Age,Sex,Income,crc in database" \
		-EN.RunDescription "Write microdata Person Age,Sex,Income,crc into database"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-Microdata.ToDb -Microdata.Person Sex,Age,Income,OldAge,AgeSex,Internal \
		-OpenM.RunName "Microdata Person Age,Sex,Income,OldAge,AgeSex in database" \
		-EN.RunDescription "Write microdata Person Age,Sex,Income and OldAge,AgeSex groups into database"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-SubFrom.AllParameters csv -OpenM.ParamDir ../../csv \
		-OpenM.RunName "All_4_Sub-values_from_csv" \
		-EN.RunDescription "All parameters from csv, 4 sub-values"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-SubFrom.AllParameters csv -OpenM.ParamDir ../../tsv \
		-OpenM.RunName "All_4_Sub-values_from_TSV" \
		-EN.RunDescription "All parameters from TSV, 4 sub-values"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-SubFrom.AllParameters csv -OpenM.ParamDir ../../csv_id \
		-OpenM.IdCsv \
		-OpenM.RunName "All_4_Sub-values_from_Id_csv" \
		-EN.RunDescription "All parameters from id.csv, 4 sub-values"
	cd $(OUT_BIN_DIR) && $(model_root_name)=$(model_root_value) ./$(MODEL_EXE) $(RUN_OPT_INI) \
		-OpenM.SubValues 4 -OpenM.Threads 4 \
		-SubFrom.AllParameters csv -OpenM.ParamDir ../../tsv_id \
		-OpenM.RunName "All_4_Sub-values_from_Id_TSV" \
		-EN.RunDescription "All parameters from id.TSV, 4 sub-values"

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(DEPS_DIR)/*.d

.PHONY: clean-all
clean-all: clean
	rm -rf $(MODEL_BUILD_DIR)
	rm -f $(OUT_BIN_DIR)/$(MODEL_EXE)
	rm -f $(MODEL_SQLITE)
	rm -f $(OUT_BIN_DIR)/*.ini
	rm -f $(OUT_BIN_DIR)/*.log

.PHONY: prepare
prepare:
	@if [ ! -d $(DEPS_DIR) ] ; then mkdir -p $(DEPS_DIR) ; fi
	@if [ ! -d $(OBJ_DIR) ] ; then mkdir -p $(OBJ_DIR) ; fi
	@if [ ! -d $(OUT_BIN_DIR) ] ; then mkdir -p $(OUT_BIN_DIR) ; fi
ifeq ($(PLATFORM_UNAME), Darwin)
	@sed \
	-e "s_<string>0.0.1</string>_<string>$(BUNDLE_VERSION)</string>_" \
	-e "s|<string>org.openmpp.MODEL</string>|<string>org.openmpp.$(MODEL_NAME)</string>|" \
	-e "s|<string>MODEL</string>|<string>$(MODEL_NAME)</string>|" \
	< $(OM_MODELS_DIR)/Model.Info.plist.txt \
	> $(MODEL_BUILD_DIR)/Info.plist
endif

# include dependencies for each .cpp file
# if target is not clean or prepare
ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean clean-all prepare)))
    -include $(DEPS)
endif

