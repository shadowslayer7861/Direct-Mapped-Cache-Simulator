CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDLIBS =
ifeq ($(OS),Windows_NT)
EXE = .exe
else
EXE =
endif

SRC_SOURCES = $(wildcard src/*.c)
LIB_SOURCES = $(filter-out src/main.c,$(SRC_SOURCES))
TEST_SOURCES = $(wildcard tests/test_*.c)
TEST_BINS = $(patsubst tests/%.c,build/%$(EXE),$(TEST_SOURCES))
PROGRAM = build/cachesim$(EXE)

.PHONY: all test run clean

all: $(PROGRAM)

$(PROGRAM): $(SRC_SOURCES) | build
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC_SOURCES) $(LDLIBS) -o $@

build/test_%$(EXE): tests/test_%.c $(LIB_SOURCES) | build
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB_SOURCES) $(LDLIBS) -o $@

build:
	mkdir -p build

test: $(TEST_BINS)
	@set -e; for test in $(TEST_BINS); do ./$$test; done

run: all
	./$(PROGRAM)

clean:
	rm -rf build
