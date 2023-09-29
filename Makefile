# Directories
SRC_DIR = src
OBJ_DIR = bin/obj
TESTS_DIR = src/tests
TESTS_BIN_DIR = bin/tests

# Files
DEPS = $(wildcard $(SRC_DIR)/*.h)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
EXECS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))
TESTS = $(wildcard $(TESTS_DIR)/*.c)
TESTS_OBJS = $(patsubst $(TESTS_DIR)/%.c, $(OBJ_DIR)/%.o, $(TESTS))
TESTS_EXECS = $(patsubst $(TESTS_DIR)/%.c, $(TESTS_BIN_DIR)/%, $(TESTS))

# Compiler settings
CC = gcc

#-I$(SRC_DIR): adds the src directory to the include path
CFLAGS = -Wall -Wextra -std=c99 -pedantic -I$(SRC_DIR)

# Libraries
LIBS = -lm

# Targets
TARGET = bin/cpu_emulator

all: $(TARGET) $(TESTS_EXECS)

$(OBJS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(TESTS_OBJS): $(OBJ_DIR)/%.o: $(TESTS_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TESTS_EXECS): $(TESTS_BIN_DIR)/%: $(OBJ_DIR)/%.o $(OBJS)


.PHONY: clean
clean:
	rm -f $(OBJS) $(TESTS_OBJS) $(TESTS_EXECS) $(TARGET)