SRC_DIR := src
TEST_DIR := test
BIN_DIR := bin
OBJ_DIR := bin/obj

CC := gcc
CFLAGS := -Wall -Wextra -Werror -g -std=c11
TEST_FLAGS := -w -g

DEPS := $(SRC_DIR)/cpu.h $(SRC_DIR)/cpu.c
MUNIT_DEPS := $(TEST_DIR)/munit/munit.h $(TEST_DIR)/munit/munit.c

TEST_DEPS := $(TEST_DIR)/test_instructions.c $(DEPS)

TARGET := $(BIN_DIR)/cpu_emulator

all: $(TARGET) test_instructions

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
$(TARGET): $(OBJ_DIR)/cpu.o $(OBJ_DIR)/cpu_emulator.o
	$(CC) $(CFLAGS) $^ -o $@

munit.o : $(TEST_DIR)/munit/munit.c $(TEST_DIR)/munit/munit.h
	$(CC) $(TEST_FLAGS) -c $< -o $@

test_instructions.o : $(TEST_DIR)/test_instructions.c $(DEPS) $(MUNIT_DEPS)
	$(CC) $(TEST_FLAGS) -c $< -o $@

test_instructions: test_instructions.o $(OBJ_DIR)/cpu.o munit.o
	$(CC) $(TEST_FLAGS) $^ -o $(BIN_DIR)/tests/test_instructions

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET) $(BIN_DIR)/tests/test_instructions ./test_instructions.o ./munit.o