CC = gcc
CFLAGS = -Wall -Wextra
DEPS = $(wildcard src/*.h)
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, bin/obj/%.o, $(SRCS))
LIBS = -lm
TARGET = bin/cpu_emulator


all: $(TARGET)

bin/obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY : clean
clean:
	rm -f $(OBJS) $(TARGET)