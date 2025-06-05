#!/usr/bin/env bash

set -e

# Determine sudo usage
if [ "$EUID" -eq 0 ]; then
  SUDO=""
else
  SUDO="sudo"
fi

# Determine package manager and required packages
if command -v apt-get >/dev/null; then
  PM="apt-get"
  INSTALL="$SUDO apt-get install -y"
  UPDATE="$SUDO apt-get update"
  PACKAGES="build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev"
elif command -v dnf >/dev/null; then
  PM="dnf"
  INSTALL="$SUDO dnf install -y"
  UPDATE="$SUDO dnf makecache"
  PACKAGES="gcc make SDL2-devel SDL2_image-devel SDL2_ttf-devel"
elif command -v pacman >/dev/null; then
  PM="pacman"
  INSTALL="$SUDO pacman -S --noconfirm"
  UPDATE="$SUDO pacman -Sy"
  PACKAGES="base-devel sdl2 sdl2_image sdl2_ttf"
else
  echo "Unsupported package manager. Please install SDL2, SDL2_image, SDL2_ttf, and a C compiler." >&2
  exit 1
fi

# Install dependencies
if [ -n "$UPDATE" ]; then
  echo "Updating package lists using $PM..."
  eval $UPDATE
fi

echo "Installing required packages..."
eval $INSTALL $PACKAGES

# Build the project
if [ -f Makefile ]; then
  make clean
  make
else
  gcc -Iinclude src/*.c -o chess_game $(sdl2-config --cflags --libs) -lSDL2_ttf -lSDL2_image -lm
fi

# Run the game
./chess_game
