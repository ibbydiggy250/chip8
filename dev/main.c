#include "raylib.h"
#include "chip8.h"
#include <stdio.h>
#include <string.h>

#define SCALE 12
#define CYCLES_PER_FRAME 10

// Exercises 00E0, 6XNN, 7XNN, ANNN, DXYN, 1NNN: draws the font '0' sprite,
// then loops drawing it again 8px to the right each pass, wrapping forever.
static const uint8_t demo_rom[] = {
    0x00, 0xE0, // clear
    0x60, 0x0A, // V0 = 10
    0x61, 0x0A, // V1 = 10
    0xA0, 0x50, // I = font '0'
    0xD0, 0x15, // draw 5-row sprite at (V0, V1)
    0x70, 0x08, // V0 += 8
    0x12, 0x06, // jump back to the ANNN instruction
};

int main(int argc, char *argv[])
{
    Chip8 chip;
    chip8_init(&chip);

    if (argc > 1) {
        if (!chip8_load_rom(&chip, argv[1])) {
            fprintf(stderr, "failed to load ROM: %s\n", argv[1]);
            return 1;
        }
    } else {
        memcpy(chip.mem + CHIP8_ROM_START, demo_rom, sizeof(demo_rom));
    }

    InitWindow(CHIP8_DISPLAY_W * SCALE, CHIP8_DISPLAY_H * SCALE, "CHIP-8");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        for (int i = 0; i < CYCLES_PER_FRAME; i++)
            chip8_cycle(&chip);

        if (chip.delay_timer > 0)
            chip.delay_timer--;
        if (chip.sound_timer > 0)
            chip.sound_timer--;

        BeginDrawing();
            ClearBackground(BLACK);

            for (int y = 0; y < CHIP8_DISPLAY_H; y++) {
                for (int x = 0; x < CHIP8_DISPLAY_W; x++) {
                    if (chip.display[y * CHIP8_DISPLAY_W + x])
                        DrawRectangle(x * SCALE, y * SCALE, SCALE, SCALE, WHITE);
                }
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
