CC = gcc

CFLAGS = -Wall -Wextra `sdl2-config --cflags`

LDFLAGS = `sdl2-config --libs` -lSDL2_image

TARGET = main

SRCS = main.c $(shell find . -mindepth 1 -name "*.c" ! -name "main.c")

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
