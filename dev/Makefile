CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -g
LDLIBS  := -lraylib -lopengl32 -lgdi32 -lwinmm

SRC := main.c chip8.c
OBJ := $(SRC:.c=.o)
BIN := chip8.exe
ROM ?= ../roms/corax.ch8
all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c chip8.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) $(ROM)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all run clean