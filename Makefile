# Directories
SRC_DIR = src
OBJ_DIR = bin/obj

# Files
DEPS = $(wildcard $(SRC_DIR)/*.h)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -I$(SRC_DIR)

# Libraries
LIBS = -lm

# Targets
TARGET = bin/cpu_emulator
BINARIES= $(TARGET)

# Build rules
all: $(BINARIES)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_BINS)
