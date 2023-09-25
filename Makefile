CC = gcc
CFLAGS = -Wall -g
TARGET = main
LIBS = -lm
DEPS = cpu.h
OBJS = main.o cpu.o
SRC = main.c cpu.c

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY : clean
clean:
	rm -f $(TARGET) $(OBJS)