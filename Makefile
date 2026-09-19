CC ?= cc
CPPFLAGS ?= -Isrc -D_POSIX_C_SOURCE=200809L
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -O2

TARGET := jcm

SRC := \
	src/main.c \
	src/ast.c \
	src/lex.c \
	src/eval.c \
	src/codegen.c

OBJ := $(SRC:.c=.o)

TEST_DIR := src/tests
TEST_SUPPORT_SRC := $(TEST_DIR)/test_support.c
TEST_SUPPORT_OBJ := $(TEST_DIR)/test_support.o
TEST_SRC := $(filter-out $(TEST_SUPPORT_SRC),$(wildcard $(TEST_DIR)/test_*.c))
TEST_BIN := $(TEST_SRC:.c=)
TEST_CORE_OBJ := src/ast.o src/lex.o src/eval.o

PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

.PHONY: all test run install clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_SUPPORT_OBJ): $(TEST_SUPPORT_SRC) $(TEST_DIR)/test_support.h src/ast.h src/eval.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(TEST_SUPPORT_OBJ) $(TEST_CORE_OBJ) $(TEST_DIR)/test_support.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(TEST_SUPPORT_OBJ) $(TEST_CORE_OBJ) $(LDLIBS)

test: $(TEST_BIN)
	@set -e; for test in $(TEST_BIN); do \
		echo "==> $$test"; \
		./$$test; \
	done

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(TEST_BIN) $(OBJ) $(TEST_SUPPORT_OBJ)
