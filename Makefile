CC ?= gcc
WINDOWS_CC ?= x86_64-w64-mingw32-gcc
CFLAGS=-Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LDFLAGS=$(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lm
WINDOWS_LDFLAGS=$(LDFLAGS) -mwindows
OBJS=ai.o chess.o ui.o settings.o main.o
TARGET=chess_game

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) chess_game.exe

windows: $(OBJS)
	$(WINDOWS_CC) $(OBJS) -o chess_game.exe $(WINDOWS_LDFLAGS)

.PHONY: all clean windows
