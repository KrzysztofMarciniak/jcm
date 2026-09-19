CC ?= cc
CPPFLAGS ?= -Isrc -D_POSIX_C_SOURCE=200809L
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -O2
LDLIBS += -lm

TARGET := jcm
SRC := src/main.c src/ast.c src/lex.c src/eval.c src/codegen.c
OBJ := $(SRC:.c=.o)
TEST_DIR := src/tests
TEST_SUPPORT_SRC := $(TEST_DIR)/test_support.c
TEST_SUPPORT_OBJ := $(TEST_SUPPORT_SRC:.c=.o)
TEST_SRC := $(filter-out $(TEST_SUPPORT_SRC),$(wildcard $(TEST_DIR)/test_*.c))
TEST_BIN := $(TEST_SRC:.c=)
TEST_CORE_OBJ := src/ast.o src/lex.o src/eval.o
.PHONY: all test run install clean
all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(TEST_SUPPORT_OBJ): $(TEST_SUPPORT_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(TEST_BIN): %: %.c $(TEST_SUPPORT_OBJ) $(TEST_CORE_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(TEST_SUPPORT_OBJ) $(TEST_CORE_OBJ) $(LDLIBS)
test: $(TEST_BIN)
	@for test in $(TEST_BIN); do ./$$test; done
run: $(TARGET)
	./$(TARGET) --eval
install: $(TARGET)
	install -d $(BINDIR)
	install $(TARGET) $(BINDIR)/$(TARGET)
clean:
	rm -f $(TARGET) $(OBJ) $(TEST_SUPPORT_OBJ) $(TEST_BIN)
