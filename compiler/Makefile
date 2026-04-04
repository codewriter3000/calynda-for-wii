CC      = gcc

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Subdirectory layout
TOKENIZER_DIR    = $(SRC_DIR)/tokenizer
AST_DIR          = $(SRC_DIR)/ast
PARSER_DIR       = $(SRC_DIR)/parser
SEMA_DIR         = $(SRC_DIR)/sema
HIR_DIR          = $(SRC_DIR)/hir
RUNTIME_DIR      = $(SRC_DIR)/runtime
CLI_DIR          = $(SRC_DIR)/cli
C_EMIT_DIR       = $(SRC_DIR)/c_emit
C_RUNTIME_DIR    = $(SRC_DIR)/c_runtime

INCLUDES = -I$(TOKENIZER_DIR) -I$(AST_DIR) -I$(PARSER_DIR) -I$(SEMA_DIR) \
	-I$(HIR_DIR) -I$(RUNTIME_DIR) -I$(CLI_DIR) \
	-I$(C_EMIT_DIR) -I$(C_RUNTIME_DIR)

CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g $(INCLUDES)

# Source files
SRCS      = $(TOKENIZER_DIR)/tokenizer.c $(TOKENIZER_DIR)/tokenizer_keywords.c $(TOKENIZER_DIR)/tokenizer_literals.c $(TOKENIZER_DIR)/tokenizer_punctuation.c $(AST_DIR)/ast.c $(AST_DIR)/ast_expressions.c $(AST_DIR)/ast_statements.c $(AST_DIR)/ast_declarations.c $(PARSER_DIR)/parser.c $(PARSER_DIR)/parser_utils.c $(PARSER_DIR)/parser_types.c $(PARSER_DIR)/parser_decl.c $(PARSER_DIR)/parser_union.c $(PARSER_DIR)/parser_stmt.c $(PARSER_DIR)/parser_expr.c $(PARSER_DIR)/parser_binary.c $(PARSER_DIR)/parser_postfix.c $(PARSER_DIR)/parser_literals.c $(PARSER_DIR)/parser_lookahead.c $(AST_DIR)/ast_dump.c $(AST_DIR)/ast_dump_names.c $(AST_DIR)/ast_dump_types.c $(AST_DIR)/ast_dump_expr.c $(AST_DIR)/ast_dump_stmt.c $(AST_DIR)/ast_dump_decl.c $(SEMA_DIR)/symbol_table.c $(SEMA_DIR)/symbol_table_query.c $(SEMA_DIR)/symbol_table_core.c $(SEMA_DIR)/symbol_table_registry.c $(SEMA_DIR)/symbol_table_imports.c $(SEMA_DIR)/symbol_table_predecl.c $(SEMA_DIR)/symbol_table_analyze.c $(SEMA_DIR)/symbol_table_analyze_expr.c $(SEMA_DIR)/type_resolution.c $(SEMA_DIR)/type_resolution_helpers.c $(SEMA_DIR)/type_resolution_resolve.c $(SEMA_DIR)/type_resolution_expr.c $(SEMA_DIR)/type_checker.c $(SEMA_DIR)/type_checker_check.c $(SEMA_DIR)/type_checker_names.c $(SEMA_DIR)/type_checker_types.c $(SEMA_DIR)/type_checker_convert.c $(SEMA_DIR)/type_checker_util.c $(SEMA_DIR)/type_checker_resolve.c $(SEMA_DIR)/type_checker_lambda.c $(SEMA_DIR)/type_checker_block.c $(SEMA_DIR)/type_checker_ops.c $(SEMA_DIR)/type_checker_expr.c $(SEMA_DIR)/type_checker_expr_ext.c $(SEMA_DIR)/type_checker_expr_more.c $(SEMA_DIR)/semantic_dump.c $(SEMA_DIR)/semantic_dump_builder.c $(SEMA_DIR)/semantic_dump_types.c $(SEMA_DIR)/semantic_dump_scope.c $(SEMA_DIR)/semantic_dump_symbols.c $(HIR_DIR)/hir.c $(HIR_DIR)/hir_memory.c $(HIR_DIR)/hir_helpers.c $(HIR_DIR)/hir_lower.c $(HIR_DIR)/hir_lower_stmt.c $(HIR_DIR)/hir_lower_expr.c $(HIR_DIR)/hir_lower_expr_ext.c $(HIR_DIR)/hir_lower_decls.c $(HIR_DIR)/hir_dump.c $(HIR_DIR)/hir_dump_helpers.c $(HIR_DIR)/hir_dump_expr.c $(HIR_DIR)/hir_dump_expr_ext.c $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_union.c $(C_EMIT_DIR)/c_emit.c $(C_EMIT_DIR)/c_emit_type.c $(C_EMIT_DIR)/c_emit_names.c $(C_EMIT_DIR)/c_emit_expr.c $(C_EMIT_DIR)/c_emit_stmt.c $(C_EMIT_DIR)/c_emit_closure.c $(C_RUNTIME_DIR)/calynda_gc_refcount.c $(C_RUNTIME_DIR)/calynda_runtime.c
TEST_SRCS = $(TEST_DIR)/test_tokenizer.c $(TEST_DIR)/test_ast.c $(TEST_DIR)/test_parser.c $(TEST_DIR)/test_ast_dump.c $(TEST_DIR)/test_symbol_table.c $(TEST_DIR)/test_type_resolution.c $(TEST_DIR)/test_type_checker.c $(TEST_DIR)/test_semantic_dump.c $(TEST_DIR)/test_hir_dump.c $(TEST_DIR)/test_runtime.c $(TEST_DIR)/test_calynda_cli.c $(TEST_DIR)/test_c_emit.c

# Object files
TOKENIZER_OBJS       = $(BUILD_DIR)/tokenizer.o $(BUILD_DIR)/tokenizer_keywords.o $(BUILD_DIR)/tokenizer_literals.o $(BUILD_DIR)/tokenizer_punctuation.o
AST_OBJS             = $(BUILD_DIR)/ast.o $(BUILD_DIR)/ast_expressions.o $(BUILD_DIR)/ast_statements.o $(BUILD_DIR)/ast_declarations.o
PARSER_OBJS          = $(BUILD_DIR)/parser.o $(BUILD_DIR)/parser_utils.o $(BUILD_DIR)/parser_types.o $(BUILD_DIR)/parser_decl.o $(BUILD_DIR)/parser_union.o $(BUILD_DIR)/parser_stmt.o $(BUILD_DIR)/parser_expr.o $(BUILD_DIR)/parser_binary.o $(BUILD_DIR)/parser_postfix.o $(BUILD_DIR)/parser_literals.o $(BUILD_DIR)/parser_lookahead.o
AST_DUMP_OBJS        = $(BUILD_DIR)/ast_dump.o $(BUILD_DIR)/ast_dump_names.o $(BUILD_DIR)/ast_dump_types.o $(BUILD_DIR)/ast_dump_expr.o $(BUILD_DIR)/ast_dump_stmt.o $(BUILD_DIR)/ast_dump_decl.o
SYMBOL_TABLE_OBJS    = $(BUILD_DIR)/symbol_table.o $(BUILD_DIR)/symbol_table_query.o $(BUILD_DIR)/symbol_table_core.o $(BUILD_DIR)/symbol_table_registry.o $(BUILD_DIR)/symbol_table_imports.o $(BUILD_DIR)/symbol_table_predecl.o $(BUILD_DIR)/symbol_table_analyze.o $(BUILD_DIR)/symbol_table_analyze_expr.o
TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/type_resolution.o $(BUILD_DIR)/type_resolution_helpers.o $(BUILD_DIR)/type_resolution_resolve.o $(BUILD_DIR)/type_resolution_expr.o
TYPE_CHECKER_OBJS    = $(BUILD_DIR)/type_checker.o $(BUILD_DIR)/type_checker_check.o $(BUILD_DIR)/type_checker_names.o $(BUILD_DIR)/type_checker_types.o $(BUILD_DIR)/type_checker_convert.o $(BUILD_DIR)/type_checker_util.o $(BUILD_DIR)/type_checker_resolve.o $(BUILD_DIR)/type_checker_lambda.o $(BUILD_DIR)/type_checker_block.o $(BUILD_DIR)/type_checker_ops.o $(BUILD_DIR)/type_checker_expr.o $(BUILD_DIR)/type_checker_expr_ext.o $(BUILD_DIR)/type_checker_expr_more.o
SEMANTIC_DUMP_OBJS   = $(BUILD_DIR)/semantic_dump.o $(BUILD_DIR)/semantic_dump_builder.o $(BUILD_DIR)/semantic_dump_types.o $(BUILD_DIR)/semantic_dump_scope.o $(BUILD_DIR)/semantic_dump_symbols.o
HIR_OBJS             = $(BUILD_DIR)/hir.o $(BUILD_DIR)/hir_memory.o $(BUILD_DIR)/hir_helpers.o $(BUILD_DIR)/hir_lower.o $(BUILD_DIR)/hir_lower_stmt.o $(BUILD_DIR)/hir_lower_expr.o $(BUILD_DIR)/hir_lower_expr_ext.o $(BUILD_DIR)/hir_lower_decls.o
HIR_DUMP_OBJS        = $(BUILD_DIR)/hir_dump.o $(BUILD_DIR)/hir_dump_helpers.o $(BUILD_DIR)/hir_dump_expr.o $(BUILD_DIR)/hir_dump_expr_ext.o
RUNTIME_OBJS         = $(BUILD_DIR)/runtime.o $(BUILD_DIR)/runtime_names.o $(BUILD_DIR)/runtime_format.o $(BUILD_DIR)/runtime_ops.o $(BUILD_DIR)/runtime_union.o
C_EMIT_OBJS          = $(BUILD_DIR)/c_emit.o $(BUILD_DIR)/c_emit_type.o $(BUILD_DIR)/c_emit_names.o $(BUILD_DIR)/c_emit_expr.o $(BUILD_DIR)/c_emit_stmt.o $(BUILD_DIR)/c_emit_closure.o
C_RUNTIME_OBJS       = $(BUILD_DIR)/calynda_gc_refcount.o $(BUILD_DIR)/calynda_runtime.o
DUMP_AST_OBJS        = $(BUILD_DIR)/dump_ast.o
DUMP_SEMANTICS_OBJS  = $(BUILD_DIR)/dump_semantics.o
CALYNDA_CLI_OBJS     = $(BUILD_DIR)/calynda.o $(BUILD_DIR)/calynda_compile.o $(BUILD_DIR)/calynda_utils.o
TEST_TOKENIZER_OBJS  = $(BUILD_DIR)/test_tokenizer.o
TEST_AST_OBJS        = $(BUILD_DIR)/test_ast.o
TEST_PARSER_OBJS     = $(BUILD_DIR)/test_parser.o
TEST_AST_DUMP_OBJS   = $(BUILD_DIR)/test_ast_dump.o
TEST_SYMBOL_TABLE_OBJS = $(BUILD_DIR)/test_symbol_table.o
TEST_TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/test_type_resolution.o
TEST_TYPE_CHECKER_OBJS = $(BUILD_DIR)/test_type_checker.o
TEST_SEMANTIC_DUMP_OBJS = $(BUILD_DIR)/test_semantic_dump.o
TEST_HIR_DUMP_OBJS   = $(BUILD_DIR)/test_hir_dump.o
TEST_RUNTIME_OBJS    = $(BUILD_DIR)/test_runtime.o
TEST_CALYNDA_CLI_OBJS = $(BUILD_DIR)/test_calynda_cli.o
TEST_C_EMIT_OBJS     = $(BUILD_DIR)/test_c_emit.o

# Test binaries
TEST_TOKENIZER_BIN = $(BUILD_DIR)/test_tokenizer
TEST_AST_BIN       = $(BUILD_DIR)/test_ast
TEST_PARSER_BIN    = $(BUILD_DIR)/test_parser
TEST_AST_DUMP_BIN  = $(BUILD_DIR)/test_ast_dump
TEST_SYMBOL_TABLE_BIN = $(BUILD_DIR)/test_symbol_table
TEST_TYPE_RESOLUTION_BIN = $(BUILD_DIR)/test_type_resolution
TEST_TYPE_CHECKER_BIN = $(BUILD_DIR)/test_type_checker
TEST_SEMANTIC_DUMP_BIN = $(BUILD_DIR)/test_semantic_dump
TEST_HIR_DUMP_BIN   = $(BUILD_DIR)/test_hir_dump
TEST_RUNTIME_BIN   = $(BUILD_DIR)/test_runtime
TEST_CALYNDA_CLI_BIN = $(BUILD_DIR)/test_calynda_cli
TEST_C_EMIT_BIN    = $(BUILD_DIR)/test_c_emit
TEST_BINS          = $(TEST_TOKENIZER_BIN) $(TEST_AST_BIN) $(TEST_PARSER_BIN) $(TEST_AST_DUMP_BIN) $(TEST_SYMBOL_TABLE_BIN) $(TEST_TYPE_RESOLUTION_BIN) $(TEST_TYPE_CHECKER_BIN) $(TEST_SEMANTIC_DUMP_BIN) $(TEST_HIR_DUMP_BIN) $(TEST_RUNTIME_BIN) $(TEST_CALYNDA_CLI_BIN) $(TEST_C_EMIT_BIN)

# Tool binaries
DUMP_AST_BIN       = $(BUILD_DIR)/dump_ast
DUMP_SEMANTICS_BIN = $(BUILD_DIR)/dump_semantics
CALYNDA_BIN        = $(BUILD_DIR)/calynda

PREFIX ?= /usr/local
INSTALL_BIN_DIR ?= $(PREFIX)/bin

.PHONY: all clean test dump_ast dump_semantics calynda install

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/tokenizer.o: $(TOKENIZER_DIR)/tokenizer.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_keywords.o: $(TOKENIZER_DIR)/tokenizer_keywords.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_literals.o: $(TOKENIZER_DIR)/tokenizer_literals.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_punctuation.o: $(TOKENIZER_DIR)/tokenizer_punctuation.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(AST_DIR)/ast.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_expressions.o: $(AST_DIR)/ast_expressions.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_statements.o: $(AST_DIR)/ast_statements.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_declarations.o: $(AST_DIR)/ast_declarations.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(PARSER_DIR)/parser.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_utils.o: $(PARSER_DIR)/parser_utils.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_types.o: $(PARSER_DIR)/parser_types.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_decl.o: $(PARSER_DIR)/parser_decl.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_union.o: $(PARSER_DIR)/parser_union.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_stmt.o: $(PARSER_DIR)/parser_stmt.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_expr.o: $(PARSER_DIR)/parser_expr.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_binary.o: $(PARSER_DIR)/parser_binary.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_postfix.o: $(PARSER_DIR)/parser_postfix.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_literals.o: $(PARSER_DIR)/parser_literals.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_lookahead.o: $(PARSER_DIR)/parser_lookahead.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump.o: $(AST_DIR)/ast_dump.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_names.o: $(AST_DIR)/ast_dump_names.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_types.o: $(AST_DIR)/ast_dump_types.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_expr.o: $(AST_DIR)/ast_dump_expr.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_stmt.o: $(AST_DIR)/ast_dump_stmt.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_decl.o: $(AST_DIR)/ast_dump_decl.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table.o: $(SEMA_DIR)/symbol_table.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_query.o: $(SEMA_DIR)/symbol_table_query.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_core.o: $(SEMA_DIR)/symbol_table_core.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_registry.o: $(SEMA_DIR)/symbol_table_registry.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_imports.o: $(SEMA_DIR)/symbol_table_imports.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_predecl.o: $(SEMA_DIR)/symbol_table_predecl.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_analyze.o: $(SEMA_DIR)/symbol_table_analyze.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_analyze_expr.o: $(SEMA_DIR)/symbol_table_analyze_expr.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution.o: $(SEMA_DIR)/type_resolution.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_helpers.o: $(SEMA_DIR)/type_resolution_helpers.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_resolve.o: $(SEMA_DIR)/type_resolution_resolve.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_expr.o: $(SEMA_DIR)/type_resolution_expr.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker.o: $(SEMA_DIR)/type_checker.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_check.o: $(SEMA_DIR)/type_checker_check.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_names.o: $(SEMA_DIR)/type_checker_names.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_types.o: $(SEMA_DIR)/type_checker_types.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_convert.o: $(SEMA_DIR)/type_checker_convert.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_util.o: $(SEMA_DIR)/type_checker_util.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_resolve.o: $(SEMA_DIR)/type_checker_resolve.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_lambda.o: $(SEMA_DIR)/type_checker_lambda.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_block.o: $(SEMA_DIR)/type_checker_block.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_ops.o: $(SEMA_DIR)/type_checker_ops.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr.o: $(SEMA_DIR)/type_checker_expr.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_ext.o: $(SEMA_DIR)/type_checker_expr_ext.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_more.o: $(SEMA_DIR)/type_checker_expr_more.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump.o: $(SEMA_DIR)/semantic_dump.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_builder.o: $(SEMA_DIR)/semantic_dump_builder.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_types.o: $(SEMA_DIR)/semantic_dump_types.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_scope.o: $(SEMA_DIR)/semantic_dump_scope.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_symbols.o: $(SEMA_DIR)/semantic_dump_symbols.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir.o: $(HIR_DIR)/hir.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_memory.o: $(HIR_DIR)/hir_memory.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_helpers.o: $(HIR_DIR)/hir_helpers.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower.o: $(HIR_DIR)/hir_lower.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_stmt.o: $(HIR_DIR)/hir_lower_stmt.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr.o: $(HIR_DIR)/hir_lower_expr.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr_ext.o: $(HIR_DIR)/hir_lower_expr_ext.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_decls.o: $(HIR_DIR)/hir_lower_decls.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump.o: $(HIR_DIR)/hir_dump.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_helpers.o: $(HIR_DIR)/hir_dump_helpers.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_expr.o: $(HIR_DIR)/hir_dump_expr.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_expr_ext.o: $(HIR_DIR)/hir_dump_expr_ext.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime.o: $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_names.o: $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_format.o: $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_ops.o: $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_union.o: $(RUNTIME_DIR)/runtime_union.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/c_emit.o: $(C_EMIT_DIR)/c_emit.c $(C_EMIT_DIR)/c_emit_internal.h $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/c_emit_type.o: $(C_EMIT_DIR)/c_emit_type.c $(C_EMIT_DIR)/c_emit_internal.h $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/c_emit_names.o: $(C_EMIT_DIR)/c_emit_names.c $(C_EMIT_DIR)/c_emit_internal.h $(C_EMIT_DIR)/c_emit.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/c_emit_expr.o: $(C_EMIT_DIR)/c_emit_expr.c $(C_EMIT_DIR)/c_emit_internal.h $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/c_emit_stmt.o: $(C_EMIT_DIR)/c_emit_stmt.c $(C_EMIT_DIR)/c_emit_internal.h $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/c_emit_closure.o: $(C_EMIT_DIR)/c_emit_closure.c $(C_EMIT_DIR)/c_emit_internal.h $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_gc_refcount.o: $(C_RUNTIME_DIR)/calynda_gc_refcount.c $(C_RUNTIME_DIR)/calynda_gc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_runtime.o: $(C_RUNTIME_DIR)/calynda_runtime.c $(C_RUNTIME_DIR)/calynda_runtime.h $(C_RUNTIME_DIR)/calynda_gc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_ast.o: $(CLI_DIR)/dump_ast.c $(AST_DIR)/ast_dump.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_semantics.o: $(CLI_DIR)/dump_semantics.c $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda.o: $(CLI_DIR)/calynda.c $(CLI_DIR)/calynda_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_compile.o: $(CLI_DIR)/calynda_compile.c $(CLI_DIR)/calynda_internal.h $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_utils.o: $(CLI_DIR)/calynda_utils.c $(CLI_DIR)/calynda_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer.o: $(TEST_DIR)/test_tokenizer.c $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast.o: $(TEST_DIR)/test_ast.c $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: $(TEST_DIR)/test_parser.c $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump.o: $(TEST_DIR)/test_ast_dump.c $(AST_DIR)/ast_dump.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table.o: $(TEST_DIR)/test_symbol_table.c $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution.o: $(TEST_DIR)/test_type_resolution.c $(SEMA_DIR)/type_resolution.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker.o: $(TEST_DIR)/test_type_checker.c $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_semantic_dump.o: $(TEST_DIR)/test_semantic_dump.c $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump.o: $(TEST_DIR)/test_hir_dump.c $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_runtime.o: $(TEST_DIR)/test_runtime.c $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli.o: $(TEST_DIR)/test_calynda_cli.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_c_emit.o: $(TEST_DIR)/test_c_emit.c $(C_EMIT_DIR)/c_emit.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TOKENIZER_BIN): $(TOKENIZER_OBJS) $(TEST_TOKENIZER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_AST_BIN): $(AST_OBJS) $(TEST_AST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_PARSER_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(TEST_PARSER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_AST_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(AST_DUMP_OBJS) $(TEST_AST_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_SYMBOL_TABLE_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TEST_SYMBOL_TABLE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_TYPE_RESOLUTION_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(TYPE_RESOLUTION_OBJS) $(TEST_TYPE_RESOLUTION_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_TYPE_CHECKER_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(TEST_TYPE_CHECKER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_SEMANTIC_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(SEMANTIC_DUMP_OBJS) $(TEST_SEMANTIC_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_HIR_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(HIR_DUMP_OBJS) $(TEST_HIR_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_RUNTIME_BIN): $(RUNTIME_OBJS) $(TEST_RUNTIME_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_CALYNDA_CLI_BIN): $(TEST_CALYNDA_CLI_OBJS) $(CALYNDA_BIN)
	$(CC) $(CFLAGS) $(TEST_CALYNDA_CLI_OBJS) -o $@

$(TEST_C_EMIT_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(C_EMIT_OBJS) $(C_RUNTIME_OBJS) $(TEST_C_EMIT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(DUMP_AST_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(AST_DUMP_OBJS) $(DUMP_AST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(DUMP_SEMANTICS_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(SEMANTIC_DUMP_OBJS) $(DUMP_SEMANTICS_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/calynda_runtime.a: $(RUNTIME_OBJS)
	ar rcs $@ $^

$(CALYNDA_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(HIR_DUMP_OBJS) $(C_RUNTIME_OBJS) $(C_EMIT_OBJS) $(CALYNDA_CLI_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_BINS)
	./$(TEST_TOKENIZER_BIN)
	./$(TEST_AST_BIN)
	./$(TEST_PARSER_BIN)
	./$(TEST_AST_DUMP_BIN)
	./$(TEST_SYMBOL_TABLE_BIN)
	./$(TEST_TYPE_RESOLUTION_BIN)
	./$(TEST_TYPE_CHECKER_BIN)
	./$(TEST_SEMANTIC_DUMP_BIN)
	./$(TEST_HIR_DUMP_BIN)
	./$(TEST_RUNTIME_BIN)
	./$(TEST_CALYNDA_CLI_BIN)
	./$(TEST_C_EMIT_BIN)

dump_ast: $(DUMP_AST_BIN)

dump_semantics: $(DUMP_SEMANTICS_BIN)

calynda: $(CALYNDA_BIN)

install: $(CALYNDA_BIN)
	mkdir -p $(INSTALL_BIN_DIR)
	install -m 755 $(CALYNDA_BIN) $(INSTALL_BIN_DIR)/calynda

clean:
	rm -rf $(BUILD_DIR)
