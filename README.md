# CHIP-8 Interpreter

A CHIP-8 interpreter written in C99, rendered with [raylib](https://www.raylib.com/).

CHIP-8 is a virtual machine from the mid-1970s — 4KB of memory, 16 registers, a
64×32 monochrome display, and 35 instructions. Writing an interpreter for it means
implementing a fetch–decode–execute loop, which is the same shape as a real CPU
emulator, just small enough to fit in one file.

```
┌────────────────────────────────────────┐
│                                        │
│    ████  ██  ██  ██  ████     64×32    │
│   ██     ██  ██  ██  ██       pixels   │
│   ██     ██████  ██  ████     @ 12x    │
│   ██     ██  ██  ██  ██       scale    │
│    ████  ██  ██  ██  ████              │
│                                        │
└────────────────────────────────────────┘
```

---

## How it works

The interpreter is a straight fetch–decode–execute loop.

**Fetch.** CHIP-8 instructions are 2 bytes, big-endian. Every cycle reads the two
bytes at the program counter and joins them into one 16-bit opcode, then advances
`pc` by 2 *immediately* — so any instruction that jumps just overwrites `pc`
afterwards, and no instruction has to remember to advance it.

```c
uint16_t opcode = (c->mem[c->pc] << 8) | c->mem[c->pc + 1];
c->pc += 2;
```

**Decode.** Every opcode packs its operands into the same fixed nibble positions,
so all five possible operands get pulled out up front and each case uses whichever
it needs:

| Name  | Bits             | Meaning                        |
| :---- | :--------------- | :----------------------------- |
| `x`   | `0x0F00 >> 8`    | register index (`Vx`)          |
| `y`   | `0x00F0 >> 4`    | register index (`Vy`)          |
| `n`   | `0x000F`         | 4-bit value (sprite height)    |
| `nn`  | `0x00FF`         | 8-bit constant                 |
| `nnn` | `0x0FFF`         | 12-bit address                 |

**Execute.** A `switch` on the top nibble (`opcode & 0xF000`) picks the instruction
family. Families that share a top nibble — `0x0`, `0x5`, `0x8`, `0x9`, `0xE`, `0xF` —
fall into a nested `switch` on `n` or `nn` to pick the exact instruction. Anything
unrecognised lands on a shared `unimplemented` label that logs the opcode and its
address instead of crashing, which made debugging half-finished ROMs a lot easier.

### The machine's state

Everything lives in one `Chip8` struct ([dev/chip8.h](dev/chip8.h)) — no globals, so the
interpreter is reentrant and easy to test:

| Field                | Size        | Purpose                                       |
| :------------------- | :---------- | :-------------------------------------------- |
| `mem`                | 4096 bytes  | RAM; ROMs load at `0x200`, font at `0x50`     |
| `V[16]`              | 16 bytes    | Registers `V0`–`VF` (`VF` doubles as a flag)  |
| `I`                  | 16-bit      | Index register — points at memory             |
| `pc`                 | 16-bit      | Program counter                               |
| `stack[16]` / `sp`   | 16 × 16-bit | Call stack for `CALL`/`RET`                   |
| `delay_timer`        | 8-bit       | Counts down at 60 Hz; games poll it           |
| `sound_timer`        | 8-bit       | Counts down at 60 Hz; nonzero = beep          |
| `display`            | 64 × 32     | One byte per pixel, on or off                 |
| `keys[16]`           | 16 bools    | Which hex keys are held this frame            |

The first 512 bytes (`0x000`–`0x1FF`) were where the original interpreter itself
lived, which is why ROMs start at `0x200`. That space now holds the built-in font:
16 five-byte sprites for the hex digits `0`–`F`, copied in by `chip8_init` so that
`FX29` has something to point at.

### Timing

`main.c` runs at a 60 FPS target. Each frame it polls the keyboard, runs
**10 interpreter cycles** (`CYCLES_PER_FRAME`), then ticks both timers down once —
giving roughly 600 instructions/sec with correctly-timed 60 Hz timers. Keys are
polled once per frame, before the cycle batch, so all 10 cycles see a consistent
snapshot of the keyboard.

---

## Opcodes

All 34 standard CHIP-8 instructions are implemented, in [dev/chip8.c](dev/chip8.c).

### Flow control

| Opcode | Name             | What it does                    |
| :----- | :--------------- | :------------------------------ |
| `00EE` | `RET`            | Return from subroutine          |
| `1NNN` | `JP nnn`         | Jump to address                 |
| `2NNN` | `CALL nnn`       | Call subroutine                 |
| `BNNN` | `JP V0, nnn`     | Jump to `nnn + V0`              |

### Conditional skips

| Opcode | Name             | What it does                    |
| :----- | :--------------- | :------------------------------ |
| `3XNN` | `SE Vx, nn`      | Skip if `Vx == nn`              |
| `4XNN` | `SNE Vx, nn`     | Skip if `Vx != nn`              |
| `5XY0` | `SE Vx, Vy`      | Skip if `Vx == Vy`              |
| `9XY0` | `SNE Vx, Vy`     | Skip if `Vx != Vy`              |
| `EX9E` | `SKP Vx`         | Skip if key held                |
| `EXA1` | `SKNP Vx`        | Skip if key released            |

### Registers and math

| Opcode | Name             | What it does                    |
| :----- | :--------------- | :------------------------------ |
| `6XNN` | `LD Vx, nn`      | Set `Vx = nn`                   |
| `7XNN` | `ADD Vx, nn`     | Add constant (no carry)         |
| `8XY0` | `LD Vx, Vy`      | Copy `Vy` into `Vx`             |
| `8XY1` | `OR Vx, Vy`      | Bitwise OR                      |
| `8XY2` | `AND Vx, Vy`     | Bitwise AND                     |
| `8XY3` | `XOR Vx, Vy`     | Bitwise XOR                     |
| `8XY4` | `ADD Vx, Vy`     | Add, carry into `VF`            |
| `8XY5` | `SUB Vx, Vy`     | Subtract, borrow flag           |
| `8XY6` | `SHR Vx`         | Shift right, lost bit           |
| `8XY7` | `SUBN Vx, Vy`    | Reverse subtract, borrow        |
| `8XYE` | `SHL Vx`         | Shift left, lost bit            |
| `CXNN` | `RND Vx, nn`     | Random byte, masked             |

### Memory and index

| Opcode | Name             | What it does                    |
| :----- | :--------------- | :------------------------------ |
| `ANNN` | `LD I, nnn`      | Set index register              |
| `FX1E` | `ADD I, Vx`      | Add `Vx` to `I`                 |
| `FX29` | `LD F, Vx`       | Point `I` at digit              |
| `FX33` | `LD B, Vx`       | Store binary-coded decimal      |
| `FX55` | `LD [I], Vx`     | Dump registers to memory        |
| `FX65` | `LD Vx, [I]`     | Load registers from memory      |

### Display, timers, input

| Opcode | Name             | What it does                    |
| :----- | :--------------- | :------------------------------ |
| `00E0` | `CLS`            | Clear the screen                |
| `DXYN` | `DRW Vx, Vy, n`  | Draw sprite, XOR                |
| `FX07` | `LD Vx, DT`      | Read delay timer                |
| `FX15` | `LD DT, Vx`      | Set delay timer                 |
| `FX18` | `LD ST, Vx`      | Set sound timer                 |
| `FX0A` | `LD Vx, K`       | Block until keypress            |

### The interesting three

Most of the table is one-liners. Three are worth a closer look:

**`DXYN` — draw.** Sprites are 8 pixels wide and `n` rows tall, read from memory at
`I`. Drawing is **XOR**: a set pixel flips whatever's already on screen, so drawing
the same sprite twice erases it — that's how CHIP-8 games do animation. If any lit
pixel gets turned *off* by the XOR, `VF` is set to 1 as a collision flag, which is
how games detect hits without any geometry. The start position wraps around the
screen edges, but the sprite body clips rather than wrapping.

**`FX33` — BCD.** Splits `Vx` into its three decimal digits and writes them to
`mem[I]`, `mem[I+1]`, `mem[I+2]`. There's no way to print a number otherwise: games
call this, then use `FX29` + `DXYN` three times to draw each digit's font sprite.

**`FX0A` — wait for key.** Rather than blocking, this decrements `pc` by 2 when no
key is held, so the same instruction re-executes next cycle. The interpreter keeps
running and drawing while it waits.

### Quirk choices

CHIP-8 has no single authoritative spec, and interpreters disagree on a few
instructions. This one follows the modern (CHIP-48/SUPER-CHIP) behaviour, which is
what the test ROMs below check:

- **`8XY6` / `8XYE`** shift `Vx` in place. (The original COSMAC VIP shifted `Vy` into `Vx`.)
- **`FX55` / `FX65`** leave `I` unchanged. (The original incremented `I` past the copied range.)
- **`8XY4`–`8XY7`** compute the carry/borrow *before* the write, so `VF` is still correct when `x == 0xF`.
- **`BNNN`** offsets by `V0`, the original behaviour.

---

## Keypad

CHIP-8 machines had a 16-key hex keypad. It maps onto the left of a QWERTY keyboard:

```
   CHIP-8              Keyboard
  ┌─┬─┬─┬─┐           ┌─┬─┬─┬─┐
  │1│2│3│C│           │1│2│3│4│
  ├─┼─┼─┼─┤           ├─┼─┼─┼─┤
  │4│5│6│D│    ──>    │Q│W│E│R│
  ├─┼─┼─┼─┤           ├─┼─┼─┼─┤
  │7│8│9│E│           │A│S│D│F│
  ├─┼─┼─┼─┤           ├─┼─┼─┼─┤
  │A│0│B│F│           │Z│X│C│V│
  └─┴─┴─┴─┘           └─┴─┴─┴─┘
```

The mapping lives in `keymap[16]` in [dev/main.c](dev/main.c), indexed by CHIP-8 key
value — `keymap[0]` is `X`, `keymap[1]` is `1`, and so on.

Games pick their own keys out of those 16. The usual bindings are `1`/`Q` and
`4`/`R` for Pong's two paddles, and `W`/`Q`/`E`/`A` to rotate, move, and drop in
Tetris — if a game feels unresponsive, it's worth trying the other keys in the grid.

---

## Building

**Requirements:** a C99 compiler (MinGW-w64 GCC), `mingw32-make`, and raylib
installed where the compiler can find it. The Makefile links
`-lraylib -lopengl32 -lgdi32 -lwinmm`, so it's set up for Windows; on Linux or macOS
swap those for `-lraylib -lm -ldl -lpthread` or the macOS frameworks.

```powershell
cd dev
mingw32-make
```

That produces `chip8.exe` in `dev/`. `mingw32-make clean` removes the objects and
binary.

---

## Running

Pass a ROM path as the first argument. With no argument, it runs a tiny built-in
demo ROM that draws the font's `0` sprite across the screen — useful for checking
the display path works before trusting a real ROM.

**PowerShell, direct:**

```powershell
.\chip8.exe ..\roms\corax.ch8
.\chip8.exe ..\roms\4-flags.ch8
.\chip8.exe ..\roms\pong.ch8
.\chip8.exe ..\roms\tetris.ch8
```

**Make:**

```powershell
mingw32-make run ROM=../roms/corax.ch8
mingw32-make run ROM=../roms/4-flags.ch8
mingw32-make run ROM=../roms/pong.ch8
mingw32-make run ROM=../roms/tetris.ch8
```

Note that `make run` launches the built-in demo first and the ROM second — close the
demo window to get to the ROM.

Close the window or press `ESC` to quit.

### Included ROMs

| ROM            | What it is                                                        |
| :------------- | :---------------------------------------------------------------- |
| `corax.ch8`    | Test ROM — checks most opcodes, prints a grid of pass/fail codes   |
| `4-flags.ch8`  | Test ROM — focuses on `VF` carry/borrow behaviour in the `8XY_` family |
| `pong.ch8`     | Two-player Pong                                                    |
| `tetris.ch8`   | Tetris                                                             |

The two test ROMs come from [Timendus's CHIP-8 test suite](https://github.com/Timendus/chip8-test-suite),
which is the fastest way to find out which opcode you got subtly wrong.

---

## Layout

```
chip8/
├── dev/
│   ├── chip8.h      State struct and public API
│   ├── chip8.c      Interpreter core — init, ROM loading, the opcode switch
│   ├── main.c       raylib window, input polling, timers, render loop
│   └── Makefile
└── roms/            Test ROMs and games
```

The split is deliberate: `chip8.c` has no raylib dependency at all and doesn't know
how it's being displayed or where its key states come from. `main.c` owns everything
platform-specific — the window, the 60 FPS loop, the keyboard, and drawing
`chip.display` as scaled rectangles.
