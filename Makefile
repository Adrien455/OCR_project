CC = gcc

CFLAGS = -Wall -Wextra

LDFLAGS = -lSDL2 -lSDL2_image -lm

TARGET = main

SRCS = $(shell find src -name "*.c")

HEADERS = $(shell find src -name "*.h")

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
