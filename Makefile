CC = gcc
CFLAGS = -Wall -g
#c files and headers will be in directory /src
#object files will be in directory /bin/obj
#executable will be in directory /bin
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, bin/obj/%.o, $(SRCS))
DEPS = $(wildcard src/*.h)
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