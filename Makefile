CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Source files
SRCS      = $(SRC_DIR)/tokenizer.c $(SRC_DIR)/ast.c $(SRC_DIR)/parser.c $(SRC_DIR)/ast_dump.c $(SRC_DIR)/symbol_table.c $(SRC_DIR)/type_resolution.c $(SRC_DIR)/type_checker.c $(SRC_DIR)/semantic_dump.c
TEST_SRCS = $(TEST_DIR)/test_tokenizer.c $(TEST_DIR)/test_ast.c $(TEST_DIR)/test_parser.c $(TEST_DIR)/test_ast_dump.c $(TEST_DIR)/test_symbol_table.c $(TEST_DIR)/test_type_resolution.c $(TEST_DIR)/test_type_checker.c $(TEST_DIR)/test_semantic_dump.c

# Object files
TOKENIZER_OBJS       = $(BUILD_DIR)/tokenizer.o
AST_OBJS             = $(BUILD_DIR)/ast.o
PARSER_OBJS          = $(BUILD_DIR)/parser.o
AST_DUMP_OBJS        = $(BUILD_DIR)/ast_dump.o
SYMBOL_TABLE_OBJS    = $(BUILD_DIR)/symbol_table.o
TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/type_resolution.o
TYPE_CHECKER_OBJS    = $(BUILD_DIR)/type_checker.o
SEMANTIC_DUMP_OBJS   = $(BUILD_DIR)/semantic_dump.o
DUMP_AST_OBJS        = $(BUILD_DIR)/dump_ast.o
DUMP_SEMANTICS_OBJS  = $(BUILD_DIR)/dump_semantics.o
TEST_TOKENIZER_OBJS  = $(BUILD_DIR)/test_tokenizer.o
TEST_AST_OBJS        = $(BUILD_DIR)/test_ast.o
TEST_PARSER_OBJS     = $(BUILD_DIR)/test_parser.o
TEST_AST_DUMP_OBJS   = $(BUILD_DIR)/test_ast_dump.o
TEST_SYMBOL_TABLE_OBJS = $(BUILD_DIR)/test_symbol_table.o
TEST_TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/test_type_resolution.o
TEST_TYPE_CHECKER_OBJS = $(BUILD_DIR)/test_type_checker.o
TEST_SEMANTIC_DUMP_OBJS = $(BUILD_DIR)/test_semantic_dump.o

# Test binaries
TEST_TOKENIZER_BIN = $(BUILD_DIR)/test_tokenizer
TEST_AST_BIN       = $(BUILD_DIR)/test_ast
TEST_PARSER_BIN    = $(BUILD_DIR)/test_parser
TEST_AST_DUMP_BIN  = $(BUILD_DIR)/test_ast_dump
TEST_SYMBOL_TABLE_BIN = $(BUILD_DIR)/test_symbol_table
TEST_TYPE_RESOLUTION_BIN = $(BUILD_DIR)/test_type_resolution
TEST_TYPE_CHECKER_BIN = $(BUILD_DIR)/test_type_checker
TEST_SEMANTIC_DUMP_BIN = $(BUILD_DIR)/test_semantic_dump
TEST_BINS          = $(TEST_TOKENIZER_BIN) $(TEST_AST_BIN) $(TEST_PARSER_BIN) $(TEST_AST_DUMP_BIN) $(TEST_SYMBOL_TABLE_BIN) $(TEST_TYPE_RESOLUTION_BIN) $(TEST_TYPE_CHECKER_BIN) $(TEST_SEMANTIC_DUMP_BIN)

# Tool binaries
DUMP_AST_BIN       = $(BUILD_DIR)/dump_ast
DUMP_SEMANTICS_BIN = $(BUILD_DIR)/dump_semantics

.PHONY: all clean test dump_ast dump_semantics

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/tokenizer.o: $(SRC_DIR)/tokenizer.c $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast.c $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump.o: $(SRC_DIR)/ast_dump.c $(SRC_DIR)/ast_dump.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table.o: $(SRC_DIR)/symbol_table.c $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution.o: $(SRC_DIR)/type_resolution.c $(SRC_DIR)/type_resolution.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker.o: $(SRC_DIR)/type_checker.c $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump.o: $(SRC_DIR)/semantic_dump.c $(SRC_DIR)/semantic_dump.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_ast.o: $(SRC_DIR)/dump_ast.c $(SRC_DIR)/ast_dump.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_semantics.o: $(SRC_DIR)/dump_semantics.c $(SRC_DIR)/semantic_dump.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer.o: $(TEST_DIR)/test_tokenizer.c $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/test_ast.o: $(TEST_DIR)/test_ast.c $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: $(TEST_DIR)/test_parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump.o: $(TEST_DIR)/test_ast_dump.c $(SRC_DIR)/ast_dump.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table.o: $(TEST_DIR)/test_symbol_table.c $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution.o: $(TEST_DIR)/test_type_resolution.c $(SRC_DIR)/type_resolution.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker.o: $(TEST_DIR)/test_type_checker.c $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_semantic_dump.o: $(TEST_DIR)/test_semantic_dump.c $(SRC_DIR)/semantic_dump.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
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

$(DUMP_AST_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(AST_DUMP_OBJS) $(DUMP_AST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(DUMP_SEMANTICS_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(SEMANTIC_DUMP_OBJS) $(DUMP_SEMANTICS_OBJS)
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

dump_ast: $(DUMP_AST_BIN)

dump_semantics: $(DUMP_SEMANTICS_BIN)

clean:
	rm -rf $(BUILD_DIR)
