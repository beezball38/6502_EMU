CC = gcc
CFLAGS = -Wall -Wextra
DEPS = $(wildcard src/*.h)
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, bin/obj/%.o, $(SRCS))
#src/tests will contain source code for test programs
TESTS = $(wildcard src/tests/*.c)
#bin/tests will contain executables for test programs
TESTS_OBJS = $(patsubst src/tests/%.c, bin/tests/%.o, $(TESTS))
TESTS_EXECS = $(patsubst src/tests/%.c, bin/tests/%, $(TESTS))
LIBS = -lm
TARGET = bin/cpu_emulator


all: $(TARGET) $(TESTS_EXECS)

bin/obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

#test programs
$(TESTS_OBJS): bin/tests/%.o: src/tests/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TESTS_EXECS): bin/tests/%: bin/tests/%.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET) $(TESTS_OBJS) $(TESTS_EXECS)