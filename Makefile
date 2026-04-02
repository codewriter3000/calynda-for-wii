CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Source files
SRCS      = $(SRC_DIR)/tokenizer.c
TEST_SRCS = $(TEST_DIR)/test_tokenizer.c

# Object files
OBJS      = $(BUILD_DIR)/tokenizer.o
TEST_OBJS = $(BUILD_DIR)/test_tokenizer.o

# Test binary
TEST_BIN  = $(BUILD_DIR)/test_tokenizer

.PHONY: all clean test

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/tokenizer.o: $(SRC_DIR)/tokenizer.c $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer.o: $(TEST_DIR)/test_tokenizer.c $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
