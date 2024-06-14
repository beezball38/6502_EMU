SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := bin/obj

CC := gcc
CFLAGS := -Wall -Wextra -Werror -g -std=c11
TEST_FLAGS := -w -g

DEPS := $(SRC_DIR)/cpu.h $(SRC_DIR)/cpu.c

TARGET := $(BIN_DIR)/cpu_emulator

all: $(TARGET) 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
$(TARGET): $(OBJ_DIR)/cpu.o $(OBJ_DIR)/cpu_emulator.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET) 