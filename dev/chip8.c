#include "chip8.h"
#include <string.h>

void chip8_init(Chip8 *c)
{
    memset(c, 0, sizeof(*c));
    c->pc = CHIP8_ROM_START;
}