# platform name: Linux or Darwin
PLATFORM_UNAME := $(shell uname -s)

CXX = g++
CC = gcc
CPP = $(CC)
AR = ar
ifndef BISON
  BISON = bison
endif
ifndef FLEX
  FLEX = flex
endif

ifndef BUILD_DIR
  BUILD_DIR = ../../build
endif

ifndef OUT_PREFIX
  OUT_PREFIX = ../..
endif

BUILD_PROJ_DIR = $(BUILD_DIR)/omc
DEPS_DIR = $(BUILD_PROJ_DIR)/deps
BUILD_SRC_DIR = $(BUILD_PROJ_DIR)/src

ifndef RELEASE
  BD_CFLAGS = -g -D_DEBUG -Og
  ifeq ($(PLATFORM_UNAME), Linux)
    BD_CFLAGS = -g -D_DEBUG -O0
  endif
  OUT_BIN_DIR = $(OUT_PREFIX)/bin
  OUT_LIB_DIR = $(OUT_PREFIX)/lib
  OBJ_DIR = $(BUILD_PROJ_DIR)/debug
  BIN_POSTFIX = D
else
  BD_CFLAGS = -DNDEBUG -O3
  OUT_BIN_DIR = $(OUT_PREFIX)/bin
  OUT_LIB_DIR = $(OUT_PREFIX)/lib
  OBJ_DIR = $(BUILD_PROJ_DIR)/release
  BIN_POSTFIX =
endif

INCLUDE_DIR = ../../include

ifndef OM_DB_LIB
#  OM_DB_LIB = sqlite3
  OM_DB_LIB = sqlite$(BIN_POSTFIX)
endif

LIBSQLITE_A = libsqlite$(BIN_POSTFIX).a
LIB_OMC_A = libopenm_omc_db$(BIN_POSTFIX).a

L_UCVT_FLAG =
STDC_FS_LIB = stdc++fs
WL_PIE_FLAG = -pie
INFO_PLIST_FLAG =
ifeq ($(PLATFORM_UNAME), Darwin)
  L_UCVT_FLAG = -liconv
  STDC_FS_LIB = stdc++
  WL_PIE_FLAG = -Wl,-pie
  ifdef RELEASE
    INFO_PLIST_FLAG = -sectcreate __TEXT __info_plist $(BUILD_PROJ_DIR)/Info.plist
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

CXXFLAGS = -Wall -std=c++20 -pthread -fPIE -fdiagnostics-color=auto -I$(INCLUDE_DIR) -I$(BUILD_SRC_DIR) -I. $(CC_ASAN_FLAGS) $(BD_CFLAGS) -Wno-register
CPPFLAGS = $(CXXFLAGS)

OMC_EXE = omc$(BIN_POSTFIX)

OMC_CPPLIST = \
AggregationSymbol.cpp \
AttributeGroupSymbol.cpp \
AnonGroupSymbol.cpp \
AttributeSymbol.cpp \
BoolSymbol.cpp \
BuiltinAttributeSymbol.cpp \
ClassificationSymbol.cpp \
CodeBlock.cpp \
CodeGen.cpp \
Constant.cpp \
ConstantSymbol.cpp \
DependencyGroupSymbol.cpp \
DerivedAttributeSymbol.cpp \
DerivedTableSymbol.cpp \
DimensionSymbol.cpp \
Driver.cpp \
EntityArrayMemberSymbol.cpp \
EntityDataMemberSymbol.cpp \
EntityEventSymbol.cpp \
EntityForeignMemberSymbol.cpp \
EntityFuncSymbol.cpp \
EntityHookSymbol.cpp \
EntityIncrementSymbol.cpp \
EntityInternalSymbol.cpp \
EntityMemberSymbol.cpp \
EntityMultilinkSymbol.cpp \
EntitySetSymbol.cpp \
EntitySymbol.cpp \
EntityTableAccumulatorSymbol.cpp \
EntityTableMeasureAttributeSymbol.cpp \
EntityTableMeasureSymbol.cpp \
EntityTableSymbol.cpp \
EnumerationSymbol.cpp \
EnumerationWithEnumeratorsSymbol.cpp \
EnumeratorSymbol.cpp \
ExtendParameterSymbol.cpp \
GlobalFuncSymbol.cpp \
GroupSymbol.cpp \
HeaderCheck.cpp \
IdentityAttributeSymbol.cpp \
ImportSymbol.cpp \
LanguageSymbol.cpp \
LinkAttributeSymbol.cpp \
LinkToAttributeSymbol.cpp \
Literal.cpp \
MaintainedAttributeSymbol.cpp \
MeasureDimensionSymbol.cpp \
ModelSymbol.cpp \
ModelTypeSymbol.cpp \
ModuleSymbol.cpp \
MultilinkAttributeSymbol.cpp \
NumericSymbol.cpp \
omc.cpp \
omc_csv.cpp \
omc_file.cpp \
omc_markup.cpp \
omc_missing_documentation.cpp \
omc_model_doc.cpp \
omc_model_metrics.cpp \
ParameterGroupSymbol.cpp \
ParameterSymbol.cpp \
ParseContext.cpp \
PartitionSymbol.cpp \
RangeSymbol.cpp \
RealSymbol.cpp \
ScenarioSymbol.cpp \
SimpleAttributeEnumSymbol.cpp \
SimpleAttributeSymbol.cpp \
StringSymbol.cpp \
StringTypeSymbol.cpp \
Symbol.cpp \
Symbol_options.cpp \
TableGroupSymbol.cpp \
TableMeasureSymbol.cpp \
TableSymbol.cpp \
TimeSymbol.cpp \
TypeOfLinkSymbol.cpp \
TypeSymbol.cpp \
UnknownTypeSymbol.cpp \
VersionSymbol.cpp \
  $(BUILD_SRC_DIR)/parser.cpp \
  $(BUILD_SRC_DIR)/scanner.cpp

sources = $(OMC_CPPLIST)

OBJS := $(foreach root,$(sources:.cpp=.o),$(OBJ_DIR)/$(notdir $(root)))
DEPS := $(foreach root,$(sources:.cpp=.d),$(DEPS_DIR)/$(notdir $(root)))

vpath %.cpp $(CURDIR)
vpath %.cpp $(BUILD_SRC_DIR)

.PHONY : all
all: omc

.PHONY : omc
omc: prepare $(OUT_BIN_DIR)/$(OMC_EXE) copy_ini

$(BUILD_SRC_DIR)/parser.cpp: | prepare
$(DEPS): $(BUILD_SRC_DIR)/parser.cpp | prepare
$(OBJS): | prepare

$(BUILD_SRC_DIR)/parser.cpp : parser.y
	$(BISON) -b parser -o $@ $<

$(BUILD_SRC_DIR)/scanner.cpp : scanner.l $(BUILD_SRC_DIR)/parser.cpp
	$(FLEX) -Cem -o $@ scanner.l

# $(BUILD_SRC_DIR)/scanner.cpp : scanner.l
#	$(FLEX) -Cem -o $@ $<

$(DEPS_DIR)/%.d : %.cpp
	$(CPP) -MM $(CPPFLAGS) $< -MF $@

$(DEPS_DIR)/parser.d : $(BUILD_SRC_DIR)/parser.cpp
	$(CPP) -MM $(CPPFLAGS) $< -MF $@

$(DEPS_DIR)/scanner.d : $(BUILD_SRC_DIR)/scanner.cpp
	$(CPP) -MM $(CPPFLAGS) $< -MF $@

$(OBJ_DIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_BIN_DIR)/$(OMC_EXE) : $(OBJS) $(OUT_LIB_DIR)/$(LIB_OMC_A) $(OUT_LIB_DIR)/$(LIBSQLITE_A)
	$(CXX) \
	-pthread -L$(OUT_LIB_DIR) $(WL_PIE_FLAG) $(LD_ASAN_FLAGS) $(INFO_PLIST_FLAG) \
	-o $@ \
	$(OBJS) \
	-lopenm_omc_db$(BIN_POSTFIX) -l$(OM_DB_LIB) -l$(STDC_FS_LIB) $(L_UCVT_FLAG)

.PHONY : copy_ini
copy_ini:
	@if [ -e omc.ini ] ; then cp -pf omc.ini $(OUT_BIN_DIR) ; fi
	@if [ -e omc.message.ini ] ; then cp -pf omc.message.ini $(OUT_BIN_DIR) ; fi

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(DEPS_DIR)/*.d
	rm -f $(BUILD_SRC_DIR)/*

.PHONY: clean-all
clean-all: clean
	rm -f $(OUT_BIN_DIR)/$(OMC_EXE)
	rm -f $(OUT_BIN_DIR)/omc.ini
	rm -f $(OUT_BIN_DIR)/omc.message.ini
	rm -rf $(BUILD_PROJ_DIR)
	@if [ -e $(OUT_BIN_DIR)/omc ] ;  then rm -f $(OUT_BIN_DIR)/omc ; fi 
	@if [ -e $(OUT_BIN_DIR)/omcD ] ; then rm -f $(OUT_BIN_DIR)/omcD ; fi 

.PHONY: prepare
prepare:
	@if [ ! -d $(BUILD_SRC_DIR) ] ; then mkdir -p $(BUILD_SRC_DIR) ; fi
	@if [ ! -d $(DEPS_DIR) ] ; then mkdir -p $(DEPS_DIR) ; fi
	@if [ ! -d $(OBJ_DIR) ] ; then mkdir -p $(OBJ_DIR) ; fi
	@if [ ! -d $(OUT_BIN_DIR) ] ; then mkdir -p $(OUT_BIN_DIR) ; fi
ifeq ($(PLATFORM_UNAME), Darwin)
	@sed \
	-e "s_<string>0.0.1</string>_<string>$(BUNDLE_VERSION)</string>_" \
	< omc.Info.plist.txt \
	> $(BUILD_PROJ_DIR)/Info.plist
endif

# include dependencies for each .cpp file
# if target is not clean or prepare
ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean clean-all prepare)))
    -include $(DEPS)
endif
