# Compiler and flags

# Build mode: debug or release
BUILD_MODE ?= debug
WASM ?= 0

# Add -fsanitize=undefined to enable undefined behavior sanitizer
CWARNINGS := \
    -Wfatal-errors \
    -Wextra \
    -Wshadow \
    -Wundef \
    -Wwrite-strings \
    -Wredundant-decls \
    -Wdisabled-optimization \
    -Wdouble-promotion \
    -Wmissing-declarations \
    -Wconversion \
    -Wsign-conversion \
    -Wstrict-overflow=2 \
    -pedantic-errors

WASM_FLAGS := \
    -sEXPORTED_RUNTIME_METHODS=ccall \
    -sENVIRONMENT=worker \
    -sEXPORT_NAME=createModule \
    -sEXPORT_ES6=1

ifeq ($(WASM), 0)
	CC := clang
else
	CC := emcc
endif
AR := llvm-ar

# Build-specific flags
ifeq ($(WASM), 0)

# non-wasm
ifeq ($(BUILD_MODE),release)
    BUILD_FLAGS := -O3 -flto -fvisibility=hidden -DNDEBUG
else
    BUILD_FLAGS := -g --debug -DSEMI_DEBUG -DSEMI_DEBUG_MSG -fsanitize=address
endif

else

# wasm
ifeq ($(BUILD_MODE),release)
    BUILD_FLAGS := -Oz -fvisibility=hidden -DNDEBUG
else
    BUILD_FLAGS := -g3 --debug -DSEMI_DEBUG -DSEMI_DEBUG_MSG --profiling
endif

endif

CFLAGS := -std=c11 -Isrc -Iinclude $(CWARNINGS) $(BUILD_FLAGS)
ifeq ($(BUILD_MODE),release)
		LDFLAGS :=
else
		LDFLAGS := -fsanitize=address
endif

# Dependency flags
DEPFLAGS := -MMD -MP

# Library name
LIB_NAME := semi

# Source and object files
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := tests
AMALGAMATE_DIR := amalgamated

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEPS := $(OBJ:.o=.d)
LIB := $(BUILD_DIR)/lib$(LIB_NAME).a

# Disassembler settings
DIS_SRC := $(BIN_DIR)/dis/dis.cpp
DIS_OBJ := $(BUILD_DIR)/dis.o
ifeq ($(WASM), 0)
	DIS_EXECUTABLE := $(BUILD_DIR)/dis
else
	DIS_EXECUTABLE := $(BUILD_DIR)/dis.js
endif

# REPL settings
REPL_SRC := $(BIN_DIR)/semi/semi.cpp
REPL_OBJ := $(BUILD_DIR)/semi.o
ifeq ($(WASM), 0)
	REPL_EXECUTABLE := $(BUILD_DIR)/semi
else
	REPL_EXECUTABLE := $(BUILD_DIR)/semi.js
endif

# WASM settings
WASM_SRC := $(BIN_DIR)/wasm/wasm.c
WASM_OBJ := $(BUILD_DIR)/wasm.o
ifeq ($(WASM), 0)
	WASM_EXECUTABLE := $(BUILD_DIR)/wasm
else
	WASM_EXECUTABLE := $(BUILD_DIR)/wasm.js
endif

# GoogleTest settings
GTEST_INC := /opt/homebrew/opt/googletest/include
GTEST_LIB_DIR := /opt/homebrew/opt/googletest/lib
GTEST_LIBS := -lgtest -lgtest_main -lpthread
ifeq ($(WASM), 0)
	CXX := clang++
else
	CXX := em++
endif

CXXFLAGS := -std=c++17 -Isrc -Iinclude $(BUILD_FLAGS)
LDFLAGS_TEST := -L$(GTEST_LIB_DIR) $(GTEST_LIBS)

# Test files
TEST_SRC := $(wildcard $(TEST_DIR)/*.cpp)
TEST_C_OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.test.o,$(SRC))
TEST_OBJ := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/%.test.o,$(TEST_SRC))
TEST_DEPS := $(TEST_OBJ:.o=.d) $(TEST_C_OBJ:.o=.d)
TEST_CFLAGS := $(CFLAGS) -DSEMI_TEST -Wno-character-conversion
TEST_CXXFLAGS := -std=c++17 -Isrc -I$(GTEST_INC) -DSEMI_TEST -Wno-character-conversion -Iinclude $(BUILD_FLAGS)
TEST_RUNNER := $(BUILD_DIR)/test_runner


# --- Targets ---

all: $(LIB) $(DIS_EXECUTABLE) $(REPL_EXECUTABLE)

wasm: $(WASM_EXECUTABLE)

$(LIB): $(OBJ) | $(BUILD_DIR)
	@$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(DIS_EXECUTABLE): $(DIS_OBJ) $(OBJ) | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(DIS_OBJ): $(DIS_SRC) | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(REPL_EXECUTABLE): $(REPL_OBJ) $(OBJ) | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(REPL_OBJ): $(REPL_SRC) | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(WASM_EXECUTABLE): $(WASM_OBJ) $(OBJ) | $(BUILD_DIR)
	@$(CC) $(WASM_FLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(WASM_OBJ): $(WASM_SRC) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@


# --- Test Targets ---
$(TEST_RUNNER): $(TEST_OBJ) $(TEST_C_OBJ) | $(BUILD_DIR)
	@$(CXX) $(TEST_CXXFLAGS) $(LDFLAGS_TEST) -o $@ $^

# Static pattern rule for C++ test objects
# This rule applies ONLY to the files listed in $(TEST_OBJ)
$(TEST_OBJ): $(BUILD_DIR)/%.test.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	@$(CXX) $(TEST_CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Static pattern rule for C test objects
# This rule applies ONLY to the files listed in $(TEST_C_OBJ)
$(TEST_C_OBJ): $(BUILD_DIR)/%.test.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(TEST_CFLAGS) $(DEPFLAGS) -c $< -o $@

test: $(TEST_RUNNER)
	@$(TEST_RUNNER) --gtest_brief=1


# --- Aux Targets ---

-include $(DEPS) $(TEST_DEPS)

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean test dis semi wasm amalgamate

dis: $(DIS_EXECUTABLE)

semi: $(REPL_EXECUTABLE)

wasm: $(WASM_EXECUTABLE)

amalgamate:
	@python3 amalgamate.py
