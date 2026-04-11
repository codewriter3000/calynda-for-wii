# calynda_math — 3D Math Foundation

CC      ?= gcc
CFLAGS  := -Wall -Wextra -pedantic -std=c11 -I.
LDFLAGS := -lm

.PHONY: all test clean

all: test_math

calynda_math.o: calynda_math.c calynda_math.h
	$(CC) $(CFLAGS) -c -o $@ $<

test_math: test_math.c calynda_math.o calynda_math.h
	$(CC) $(CFLAGS) -o $@ test_math.c calynda_math.o $(LDFLAGS)

test: test_math
	./test_math

clean:
	rm -f *.o test_math
