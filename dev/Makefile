CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -g
LDLIBS  := -lraylib -lopengl32 -lgdi32 -lwinmm

SRC := main.c chip8.c
OBJ := $(SRC:.c=.o)
BIN := chip8.exe

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c chip8.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all run clean