CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Icore/include -Ivendor -Ivendor/tree-sitter/lib/include -D_GNU_SOURCE
LDFLAGS = -lpthread -ldl -lm

CORE_SRCS = $(wildcard core/*.c)
VENDOR_SRCS = vendor/sqlite3.c vendor/cJSON.c
TS_SRCS = vendor/tree-sitter/lib/src/lib.c \
          vendor/tree-sitter/grammars/tree-sitter-c/src/parser.c \
          vendor/tree-sitter/grammars/tree-sitter-python/src/parser.c \
          vendor/tree-sitter/grammars/tree-sitter-python/src/scanner.c \
          vendor/tree-sitter/grammars/tree-sitter-javascript/src/parser.c \
          vendor/tree-sitter/grammars/tree-sitter-javascript/src/scanner.c
CLI_SRCS = $(wildcard cli/*.c)

CORE_OBJS = $(CORE_SRCS:.c=.o)
VENDOR_OBJS = $(VENDOR_SRCS:.c=.o)
TS_OBJS = $(TS_SRCS:.c=.o)
CLI_OBJS = $(CLI_SRCS:.c=.o)

LIB = libradar.a
BIN = bin/memspace

.PHONY: all clean test

all: $(BIN)

$(TS_OBJS): CFLAGS = -std=c11 -Ivendor/tree-sitter/lib/include -D_GNU_SOURCE -w
vendor/tree-sitter/lib/src/lib.o: CFLAGS += -Ivendor/tree-sitter/lib/src
vendor/sqlite3.o: CFLAGS = -std=c11 -D_GNU_SOURCE -w

$(LIB): $(CORE_OBJS) $(VENDOR_OBJS) $(TS_OBJS)
	ar rcs $@ $^

$(BIN): $(CLI_OBJS) $(LIB)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(CLI_OBJS) $(LIB) -o $@ $(LDFLAGS)

test: $(LIB) tests/test_parser.o tests/test_index.o
	$(CC) $(CFLAGS) tests/test_parser.o $(LIB) -o tests/test_runner_parser $(LDFLAGS)
	./tests/test_runner_parser
	$(CC) $(CFLAGS) tests/test_index.o $(LIB) -o tests/test_runner_index $(LDFLAGS)
	./tests/test_runner_index

clean:
	rm -f $(CORE_OBJS) $(VENDOR_OBJS) $(TS_OBJS) $(CLI_OBJS) $(LIB) $(BIN) tests/test_parser.o tests/test_runner_parser tests/test_index.o tests/test_runner_index
	rm -rf bin/
