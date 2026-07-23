#include "raylib.h"
#include "chip8.h"

#define SCALE 12

int main(void)
{
    Chip8 chip;
    chip8_init(&chip);

    for (int y = 0; y < CHIP8_DISPLAY_H; y++)
        for (int x = 0; x < CHIP8_DISPLAY_W; x++)
            chip.display[y * CHIP8_DISPLAY_W + x] = ((x / 4) + (y / 4)) & 1;

    InitWindow(CHIP8_DISPLAY_W * SCALE, CHIP8_DISPLAY_H * SCALE, "CHIP-8");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
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