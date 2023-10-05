# Directories
SRC_DIR = src
TEST_SRC_DIR = test	
OBJ_DIR = bin/obj
TEST_OBJ_DIR = test/obj	
TEST_DIR = test
MUNIT_DIR = test/munit

# Files
DEPS = $(wildcard $(SRC_DIR)/*.h)
SRCS = $(wildcard $(SRC_DIR)/*.c)
#OBJS BESIDES TARGET_OBJ
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(filter-out $(TARGET_OBJ), $(SRCS)))
# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -I$(SRC_DIR)

# Libraries
LIBS = -lm

# Targets
TARGET = bin/cpu_emulator
TARGET_OBJ = $(OBJ_DIR)/cpu_emulator.o
BINARIES= $(TARGET)
TEST_DEPS = $(wildcard $(TEST_DIR)/*.h) $(wildcard $(MUNIT_DIR)/*.h)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c) $(wildcard $(MUNIT_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(TEST_DIR)/%.o, $(TEST_SRCS))
TEST_BINS = test/test


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS) $(TARGET_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(TEST_DIR)/%.o: $(TEST_DIR)/%.c $(TEST_DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BINS): $(TEST_OBJS) src/cpu.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)


.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET_OBJ) $(TEST_OBJS) $(TEST_BINS) $(BINARIES)
