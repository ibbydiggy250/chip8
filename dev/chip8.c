#include "chip8.h"
#include <stdio.h>
#include <string.h>

static const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

void chip8_init(Chip8 *c)
{
    memset(c, 0, sizeof(*c));
    c->pc = CHIP8_ROM_START;
    memcpy(c->mem + CHIP8_FONT_START, fontset, sizeof(fontset));
}

bool chip8_load_rom(Chip8 *c, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    size_t max = CHIP8_MEM_SIZE - CHIP8_ROM_START;
    size_t n = fread(c->mem + CHIP8_ROM_START, 1, max, f);
    fclose(f);

    return n > 0;
}

void chip8_cycle(Chip8 *c)
{
    uint16_t opcode = (c->mem[c->pc] << 8) | c->mem[c->pc + 1];
    c->pc += 2;

    uint8_t  x   = (opcode >> 8) & 0xF;
    uint8_t  y   = (opcode >> 4) & 0xF;
    uint8_t  n   = opcode & 0xF;
    uint8_t  nn  = opcode & 0xFF;
    uint16_t nnn = opcode & 0xFFF;

    switch (opcode & 0xF000) {
    case 0x0000:
        if (opcode == 0x00E0)
            memset(c->display, 0, sizeof(c->display));
        break;

    case 0x1000:
        c->pc = nnn;
        break;

    case 0x6000:
        c->V[x] = nn;
        break;

    case 0x7000:
        c->V[x] += nn;
        break;

    case 0xA000:
        c->I = nnn;
        break;

    case 0xD000: {
        uint8_t sx = c->V[x] % CHIP8_DISPLAY_W;
        uint8_t sy = c->V[y] % CHIP8_DISPLAY_H;
        c->V[0xF] = 0;

        for (int row = 0; row < n; row++) {
            if (sy + row >= CHIP8_DISPLAY_H)
                break;
            uint8_t sprite_byte = c->mem[c->I + row];

            for (int col = 0; col < 8; col++) {
                if (sx + col >= CHIP8_DISPLAY_W)
                    break;
                if (!(sprite_byte & (0x80 >> col)))
                    continue;

                int idx = (sy + row) * CHIP8_DISPLAY_W + (sx + col);
                if (c->display[idx])
                    c->V[0xF] = 1;
                c->display[idx] ^= 1;
            }
        }
        break;
    }

    default:
        fprintf(stderr, "unimplemented opcode 0x%04X at 0x%03X\n", opcode, c->pc - 2);
        break;
    }
}
