CC = gcc

CFLAGS = -Wall -Wextra

LDFLAGS = -lSDL2 -lSDL2_image -lm -lSDL2_gfx

TARGET = main

SRCS = main.c $(shell find . -mindepth 1 -name "*.c" ! -name "main.c")

HEADERS = $(shell find . -mindepth 1 -name "*.h")

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
