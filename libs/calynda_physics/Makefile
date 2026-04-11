CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g -O2
LDFLAGS = -lm

MATH_DIR  = ../calynda_math
MATH_SRC  = $(MATH_DIR)/calynda_math.c

SRC   = calynda_physics.c
HDR   = calynda_physics.h
TEST  = test_physics.c

.PHONY: all test clean

all: test_physics

test_physics: $(TEST) $(SRC) $(HDR) $(MATH_SRC)
	$(CC) $(CFLAGS) -I$(MATH_DIR) -o $@ $(TEST) $(SRC) $(MATH_SRC) $(LDFLAGS)

test: test_physics
	./test_physics

clean:
	rm -f test_physics
