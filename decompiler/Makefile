CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g -Iinclude -Isrc

BUILD_DIR = build
SRC_DIR   = src
TEST_DIR  = tests

PPC_OBJS  = $(BUILD_DIR)/ppc/decode.o \
            $(BUILD_DIR)/ppc/decode_ext.o \
            $(BUILD_DIR)/ppc/format.o \
            $(BUILD_DIR)/ppc/mnemonic.o

CORE_OBJS = $(BUILD_DIR)/dol.o $(PPC_OBJS) $(BUILD_DIR)/iso.o \
            $(BUILD_DIR)/aes.o $(BUILD_DIR)/wii_crypto.o

CLI_OBJS  = $(BUILD_DIR)/cli/main.o \
            $(BUILD_DIR)/cli/symbols.o \
            $(BUILD_DIR)/cli/extract.o \
            $(BUILD_DIR)/cli/analysis.o \
            $(BUILD_DIR)/cli/analysis_data.o \
            $(BUILD_DIR)/cli/all_functions.o \
            $(BUILD_DIR)/cli/stats.o \
            $(BUILD_DIR)/cli/diff.o \
            $(BUILD_DIR)/cli/cfg/cfg_build.o \
            $(BUILD_DIR)/cli/cfg/cfg_dominance.o \
            $(BUILD_DIR)/cli/cfg/cfg_regions.o \
            $(BUILD_DIR)/cli/cfg/cfg_switch.o \
            $(BUILD_DIR)/cli/cfg/cfg_print.o \
            $(BUILD_DIR)/cli/ssa/ssa_build.o \
            $(BUILD_DIR)/cli/ssa/ssa_defuse.o \
            $(BUILD_DIR)/cli/ssa/ssa_phi.o \
            $(BUILD_DIR)/cli/ssa/ssa_fold.o \
            $(BUILD_DIR)/cli/ssa/ssa_expr.o \
            $(BUILD_DIR)/cli/ssa/ssa_stack.o \
            $(BUILD_DIR)/cli/ssa/ssa_calls.o \
            $(BUILD_DIR)/cli/ssa/ssa_print.o \
            $(BUILD_DIR)/cli/types/types_build.o \
            $(BUILD_DIR)/cli/types/types_width.o \
            $(BUILD_DIR)/cli/types/types_ptr.o \
            $(BUILD_DIR)/cli/types/types_signature.o \
            $(BUILD_DIR)/cli/types/types_sign.o \
            $(BUILD_DIR)/cli/types/types_array.o \
            $(BUILD_DIR)/cli/types/types_conflict.o \
            $(BUILD_DIR)/cli/classes/classes_build.o \
            $(BUILD_DIR)/cli/classes/classes_scan.o \
            $(BUILD_DIR)/cli/classes/classes_hier.o \
            $(BUILD_DIR)/cli/classes/classes_fields.o \
            $(BUILD_DIR)/cli/classes/classes_map.o \
            $(BUILD_DIR)/cli/demangle/demangle.o \
            $(BUILD_DIR)/cli/demangle/demangle_type.o \
            $(BUILD_DIR)/cli/demangle/demangle_ops.o \
            $(BUILD_DIR)/cli/demangle/demangle_calynda.o \
            $(BUILD_DIR)/cli/externs/externs_classify.o \
            $(BUILD_DIR)/cli/externs/externs_types.o \
            $(BUILD_DIR)/cli/externs/externs_build.o \
            $(BUILD_DIR)/cli/externs/externs_emit.o \
            $(BUILD_DIR)/cli/emit_cal/emit_cal_file.o \
            $(BUILD_DIR)/cli/emit_cal/emit_cal_header.o \
            $(BUILD_DIR)/cli/emit_cal/emit_cal_globals.o \
            $(BUILD_DIR)/cli/emit_cal/emit_cal_func.o \
            $(BUILD_DIR)/cli/emit_cal/emit_cal_build.o \
            $(BUILD_DIR)/cli/run.o \
            $(BUILD_DIR)/cli/run_iso.o

TEST_DOL_BIN  = $(BUILD_DIR)/test_dol
TEST_PPC_BIN  = $(BUILD_DIR)/test_ppc
TEST_ISO_BIN  = $(BUILD_DIR)/test_iso
TEST_AES_BIN  = $(BUILD_DIR)/test_aes
TEST_WII_BIN  = $(BUILD_DIR)/test_wii_crypto
TEST_CFG_BIN  = $(BUILD_DIR)/test_cfg
TEST_SSA_BIN  = $(BUILD_DIR)/test_ssa
TEST_TY_BIN   = $(BUILD_DIR)/test_types
TEST_CL_BIN   = $(BUILD_DIR)/test_classes
TEST_DM_BIN   = $(BUILD_DIR)/test_demangle
TEST_EX_BIN   = $(BUILD_DIR)/test_externs
TEST_EC_BIN   = $(BUILD_DIR)/test_emit_cal
TEST_BINS     = $(TEST_DOL_BIN) $(TEST_PPC_BIN) $(TEST_ISO_BIN) \
                $(TEST_AES_BIN) $(TEST_WII_BIN) $(TEST_CFG_BIN) \
                $(TEST_SSA_BIN) $(TEST_TY_BIN) $(TEST_CL_BIN) \
                $(TEST_DM_BIN) $(TEST_EX_BIN) $(TEST_EC_BIN)

CLI_BIN       = $(BUILD_DIR)/decompile

.PHONY: all test clean

all: test $(CLI_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/cli $(BUILD_DIR)/ppc

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/%.o: $(SRC_DIR)/cli/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/cfg/%.o: $(SRC_DIR)/cli/cfg/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/ssa/%.o: $(SRC_DIR)/cli/ssa/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/types/%.o: $(SRC_DIR)/cli/types/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/classes/%.o: $(SRC_DIR)/cli/classes/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/demangle/%.o: $(SRC_DIR)/cli/demangle/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/externs/%.o: $(SRC_DIR)/cli/externs/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cli/emit_cal/%.o: $(SRC_DIR)/cli/emit_cal/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ppc/%.o: $(SRC_DIR)/ppc/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_DOL_BIN): $(BUILD_DIR)/test_dol.o $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_PPC_BIN): $(BUILD_DIR)/test_ppc.o $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_ISO_BIN): $(BUILD_DIR)/test_iso.o $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_AES_BIN): $(BUILD_DIR)/test_aes.o $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_WII_BIN): $(BUILD_DIR)/test_wii_crypto.o $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

CFG_OBJS = $(BUILD_DIR)/cli/cfg/cfg_build.o \
           $(BUILD_DIR)/cli/cfg/cfg_dominance.o \
           $(BUILD_DIR)/cli/cfg/cfg_regions.o \
           $(BUILD_DIR)/cli/cfg/cfg_switch.o \
           $(BUILD_DIR)/cli/cfg/cfg_print.o

SSA_OBJS = $(BUILD_DIR)/cli/ssa/ssa_build.o \
           $(BUILD_DIR)/cli/ssa/ssa_defuse.o \
           $(BUILD_DIR)/cli/ssa/ssa_phi.o \
           $(BUILD_DIR)/cli/ssa/ssa_fold.o \
           $(BUILD_DIR)/cli/ssa/ssa_expr.o \
           $(BUILD_DIR)/cli/ssa/ssa_stack.o \
           $(BUILD_DIR)/cli/ssa/ssa_calls.o \
           $(BUILD_DIR)/cli/ssa/ssa_print.o

TY_OBJS = $(BUILD_DIR)/cli/types/types_build.o \
          $(BUILD_DIR)/cli/types/types_width.o \
          $(BUILD_DIR)/cli/types/types_ptr.o \
          $(BUILD_DIR)/cli/types/types_signature.o \
          $(BUILD_DIR)/cli/types/types_sign.o \
          $(BUILD_DIR)/cli/types/types_array.o \
          $(BUILD_DIR)/cli/types/types_conflict.o

$(TEST_CFG_BIN): $(BUILD_DIR)/test_cfg.o $(CFG_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_SSA_BIN): $(BUILD_DIR)/test_ssa.o $(SSA_OBJS) $(CFG_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_TY_BIN): $(BUILD_DIR)/test_types.o $(BUILD_DIR)/cli/symbols.o \
                $(TY_OBJS) $(SSA_OBJS) $(CFG_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

CL_OBJS = $(BUILD_DIR)/cli/classes/classes_build.o \
          $(BUILD_DIR)/cli/classes/classes_scan.o \
          $(BUILD_DIR)/cli/classes/classes_hier.o \
          $(BUILD_DIR)/cli/classes/classes_fields.o \
          $(BUILD_DIR)/cli/classes/classes_map.o

$(TEST_CL_BIN): $(BUILD_DIR)/test_classes.o $(BUILD_DIR)/cli/symbols.o \
                $(CL_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

DM_OBJS = $(BUILD_DIR)/cli/demangle/demangle.o \
          $(BUILD_DIR)/cli/demangle/demangle_type.o \
          $(BUILD_DIR)/cli/demangle/demangle_ops.o \
          $(BUILD_DIR)/cli/demangle/demangle_calynda.o

$(TEST_DM_BIN): $(BUILD_DIR)/test_demangle.o $(DM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

EX_OBJS = $(BUILD_DIR)/cli/externs/externs_classify.o \
          $(BUILD_DIR)/cli/externs/externs_types.o \
          $(BUILD_DIR)/cli/externs/externs_build.o \
          $(BUILD_DIR)/cli/externs/externs_emit.o

EC_OBJS = $(BUILD_DIR)/cli/emit_cal/emit_cal_file.o \
          $(BUILD_DIR)/cli/emit_cal/emit_cal_header.o \
          $(BUILD_DIR)/cli/emit_cal/emit_cal_globals.o \
          $(BUILD_DIR)/cli/emit_cal/emit_cal_func.o \
          $(BUILD_DIR)/cli/emit_cal/emit_cal_build.o

$(TEST_EX_BIN): $(BUILD_DIR)/test_externs.o $(BUILD_DIR)/cli/symbols.o \
                $(EX_OBJS) $(DM_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_EC_BIN): $(BUILD_DIR)/test_emit_cal.o $(BUILD_DIR)/cli/symbols.o \
                $(EC_OBJS) $(EX_OBJS) $(DM_OBJS) $(SSA_OBJS) $(CFG_OBJS) \
                $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(CLI_BIN): $(CLI_OBJS) $(CORE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_BINS)
	@echo "Running decompiler tests..."
	@for t in $(TEST_BINS); do echo "-- $$t --"; ./$$t || exit 1; done

clean:
	rm -rf $(BUILD_DIR)
