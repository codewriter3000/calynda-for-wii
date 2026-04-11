CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g -O2
LDFLAGS = -lm

MATH_DIR  = ../calynda_math
MATH_SRC  = $(MATH_DIR)/calynda_math.c

SRC   = calynda_motion.c
HDR   = calynda_motion.h
TEST  = test_motion.c

.PHONY: all test clean

all: test_motion

test_motion: $(TEST) $(SRC) $(HDR) $(MATH_SRC)
	$(CC) $(CFLAGS) -I$(MATH_DIR) -o $@ $(TEST) $(SRC) $(MATH_SRC) $(LDFLAGS)

test: test_motion
	./test_motion

clean:
	rm -f test_motion
