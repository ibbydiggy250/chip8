#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

#define CHIP8_MEM_SIZE   4096
#define CHIP8_DISPLAY_W  64
#define CHIP8_DISPLAY_H  32
#define CHIP8_ROM_START  0x200
#define CHIP8_FONT_START 0x50

typedef struct {
    uint8_t  mem[CHIP8_MEM_SIZE];
    uint8_t  V[16];
    uint16_t I;
    uint16_t pc;

    uint16_t stack[16];
    uint8_t  sp;

    uint8_t  delay_timer;
    uint8_t  sound_timer;

    uint8_t  display[CHIP8_DISPLAY_W * CHIP8_DISPLAY_H];
    bool     keys[16];
} Chip8;

void chip8_init(Chip8 *c);

#endif