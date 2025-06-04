CC=gcc
CFLAGS=-Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LDFLAGS=$(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lm
OBJS=ai.o chess.o ui.o main.o
TARGET=chess_game

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	 rm -f $(OBJS) $(TARGET)

.PHONY: all clean
