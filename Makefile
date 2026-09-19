CC = cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2

TARGET = jcm

SRC = \
	src/main.c \
	src/ast.c \
	src/lex.c \
	src/eval.c \
	src/codegen.c

OBJ = $(SRC:.c=.o)

TEST_DIR = src/tests
TEST_SRC = $(wildcard $(TEST_DIR)/test_*.c)
TEST_BIN = $(TEST_SRC:.c=)

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(filter-out src/main.o,$(OBJ))
	$(CC) $(CFLAGS) -o $@ $< $(filter-out src/main.o,$(OBJ))

test: $(TEST_BIN)
	@for test in $(TEST_BIN); do \
		echo "==> $$test"; \
		./$$test || exit 1; \
	done

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(TEST_BIN) $(OBJ)

