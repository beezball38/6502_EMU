SRC_DIR := src
TEST_DIR := test
BIN_DIR := bin
OBJ_DIR := bin/obj

CC := gcc
CFLAGS := -Wall -Wextra -Werror -g

DEPS := $(SRC_DIR)/cpu.h $(SRC_DIR)/cpu.c

TEST_DEPS := $(TEST_DIR)/test_instructions.c $(DEPS)

TARGET := $(BIN_DIR)/cpu_emulator

all: $(TARGET) test_instructions

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
$(TARGET): $(OBJ_DIR)/cpu.o $(OBJ_DIR)/cpu_emulator.o
	$(CC) $(CFLAGS) $^ -o $@

test_instructions.o : $(TEST_DIR)/test_instructions.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

test_instructions: test_instructions.o $(OBJ_DIR)/cpu.o
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/tests/test_instructions

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET) $(BIN_DIR)/tests/test_instructions