# Solite/Wii Runtime — Build System
# ═════════════════════════════════
#
# Targets:
#   make              — build host test library + test binaries
#   make test         — run all tests
#   make clean        — remove build artifacts
#   make wii          — cross-compile for Wii (requires devkitPRO)
#
# The reactive runtime is pure C. The bridge is C++ (links libwiigui).

CC       = gcc
CXX      = g++
AR       = ar

# Paths
RUNTIME_SRC = src
RUNTIME_INC = include
BRIDGE_SRC  = bridge
TEST_SRC    = tests
BUILD_DIR   = build

# Calynda runtime (for CalyndaRtWord, GC, etc.)
CALYNDA_RT  = ../compiler/src/c_runtime

CFLAGS   = -Wall -Wextra -std=c11 -g \
           -I$(RUNTIME_INC) \
           -I$(CALYNDA_RT)
CXXFLAGS = -Wall -Wextra -std=c++11 -g \
           -I$(RUNTIME_INC) \
           -I$(CALYNDA_RT)

# Source files
C_SOURCES = $(RUNTIME_SRC)/solite_signal.c \
            $(RUNTIME_SRC)/solite_effect.c \
            $(RUNTIME_SRC)/solite_runtime.c \
            $(RUNTIME_SRC)/solite_control_flow.c \
            $(RUNTIME_SRC)/solite_style.c

CPP_SOURCES = $(BRIDGE_SRC)/solite_gui_bridge.cpp

# Calynda runtime sources we need to link
CALYNDA_SOURCES = $(CALYNDA_RT)/calynda_runtime.c \
                  $(CALYNDA_RT)/calynda_gc_refcount.c

# Object files
C_OBJECTS   = $(patsubst $(RUNTIME_SRC)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CPP_OBJECTS = $(patsubst $(BRIDGE_SRC)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
CAL_OBJECTS = $(BUILD_DIR)/calynda_runtime.o $(BUILD_DIR)/calynda_gc_refcount.o

ALL_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS) $(CAL_OBJECTS)

# Output library
LIB_HOST = $(BUILD_DIR)/libsolite_wii_host.a

# Test binaries
TEST_SIGNAL  = $(BUILD_DIR)/test_solite_signal
TEST_EFFECT  = $(BUILD_DIR)/test_solite_effect
TEST_STYLE   = $(BUILD_DIR)/test_solite_style

# Game / example binaries
GAME_SRC     = ../game/src
COUNTER_GUI  = $(BUILD_DIR)/counter_gui

# ── Default target ──────────────────────────────────────────────────
.PHONY: all clean test wii counter

all: $(BUILD_DIR) $(LIB_HOST) $(TEST_SIGNAL) $(TEST_EFFECT) $(TEST_STYLE) $(COUNTER_GUI)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ── Compile rules ───────────────────────────────────────────────────

$(BUILD_DIR)/%.o: $(RUNTIME_SRC)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BRIDGE_SRC)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_runtime.o: $(CALYNDA_RT)/calynda_runtime.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_gc_refcount.o: $(CALYNDA_RT)/calynda_gc_refcount.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Library ─────────────────────────────────────────────────────────

$(LIB_HOST): $(ALL_OBJECTS)
	$(AR) rcs $@ $^

# ── Test binaries ───────────────────────────────────────────────────

$(TEST_SIGNAL): $(TEST_SRC)/test_solite_signal.c $(LIB_HOST)
	$(CXX) $(CXXFLAGS) $< -L$(BUILD_DIR) -lsolite_wii_host -o $@

$(TEST_EFFECT): $(TEST_SRC)/test_solite_effect.c $(LIB_HOST)
	$(CXX) $(CXXFLAGS) $< -L$(BUILD_DIR) -lsolite_wii_host -o $@

$(TEST_STYLE): $(TEST_SRC)/test_solite_style.c $(LIB_HOST)
	$(CXX) $(CXXFLAGS) $< -L$(BUILD_DIR) -lsolite_wii_host -o $@

# ── Counter GUI app ─────────────────────────────────────────────────

$(COUNTER_GUI): $(GAME_SRC)/counter_gui.c $(LIB_HOST)
	$(CXX) $(CXXFLAGS) $< -L$(BUILD_DIR) -lsolite_wii_host -o $@

counter: $(BUILD_DIR) $(LIB_HOST) $(COUNTER_GUI)
	@echo "Built counter_gui → $(COUNTER_GUI)"

# ── Test runner ─────────────────────────────────────────────────────

test: all
	@echo "=== Running Solite/Wii runtime tests ==="
	$(TEST_SIGNAL)  && echo "  PASS: test_solite_signal"
	$(TEST_EFFECT)  && echo "  PASS: test_solite_effect"
	$(TEST_STYLE)   && echo "  PASS: test_solite_style"
	@echo "=== All tests passed ==="

# ── Clean ───────────────────────────────────────────────────────────

clean:
	rm -rf $(BUILD_DIR)

# ── Wii cross-compilation ──────────────────────────────────────────
# Requires devkitPRO with devkitPPC installed.

WII_CC      = powerpc-eabi-gcc
WII_CXX     = powerpc-eabi-g++
WII_AR      = powerpc-eabi-ar
WII_CFLAGS  = -DCALYNDA_WII_BUILD -DSOLITE_HAS_LIBWIIGUI \
              -Wall -Wextra -std=c11 -O2 \
              -I$(RUNTIME_INC) -I$(CALYNDA_RT) \
              -I$(DEVKITPRO)/libogc/include
WII_CXXFLAGS = -DCALYNDA_WII_BUILD -DSOLITE_HAS_LIBWIIGUI \
               -Wall -Wextra -std=c++11 -O2 \
               -I$(RUNTIME_INC) -I$(CALYNDA_RT) \
               -I$(DEVKITPRO)/libogc/include

WII_BUILD_DIR = build/wii
WII_LIB       = $(WII_BUILD_DIR)/libsolite_wii.a

wii: $(WII_BUILD_DIR)
	$(WII_CC) $(WII_CFLAGS) -c $(RUNTIME_SRC)/solite_signal.c -o $(WII_BUILD_DIR)/solite_signal.o
	$(WII_CC) $(WII_CFLAGS) -c $(RUNTIME_SRC)/solite_effect.c -o $(WII_BUILD_DIR)/solite_effect.o
	$(WII_CC) $(WII_CFLAGS) -c $(RUNTIME_SRC)/solite_runtime.c -o $(WII_BUILD_DIR)/solite_runtime.o
	$(WII_CC) $(WII_CFLAGS) -c $(RUNTIME_SRC)/solite_control_flow.c -o $(WII_BUILD_DIR)/solite_control_flow.o
	$(WII_CC) $(WII_CFLAGS) -c $(RUNTIME_SRC)/solite_style.c -o $(WII_BUILD_DIR)/solite_style.o
	$(WII_CXX) $(WII_CXXFLAGS) -c $(BRIDGE_SRC)/solite_gui_bridge.cpp -o $(WII_BUILD_DIR)/solite_gui_bridge.o
	$(WII_AR) rcs $(WII_LIB) $(WII_BUILD_DIR)/*.o

$(WII_BUILD_DIR):
	mkdir -p $(WII_BUILD_DIR)
