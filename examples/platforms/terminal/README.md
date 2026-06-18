# Terminal port — real DOOM as colored ASCII

Yes, it runs DOOM. In your terminal. As text.

This is the same six `DG_*` callbacks as every other port, linked against the
**real DOOM engine** — but the "screen" is your terminal window and the
"keyboard" is raw stdin. No SDL, no X11, no GPU: just POSIX `termios` + `ioctl`,
which every Unix already has.

It is the sibling of [`../null/platform_null_engine.c`](../null/platform_null_engine.c)
(same real engine, no GUI toolkit) — except instead of dumping one frame to a
`.ppm`, it draws **every** frame to the terminal and lets you actually play.

## Run it

Like `run-null-engine`, this needs the upstream engine sources and a WAD:

```sh
make run-terminal ENGINE=/path/to/doomgeneric/doomgeneric WAD=/path/to/doom1.wad
```

Use a **truecolor** terminal (iTerm2, modern Terminal.app, most Linux terminals,
Windows Terminal) at roughly **80×50 or larger** — the bigger the window, the
more "pixels" you get. Quit with **Ctrl-C** (the terminal is always restored on
the way out) or from DOOM's own menu.

## Controls

| Key | Action |
|-----|--------|
| Arrow keys / `W` `A` `S` `D` | move & turn |
| `,` `.` | strafe left / right |
| `Space` or `E` | use (doors, switches) |
| `F` or `X` | fire |
| `Enter` | confirm menu item |
| `Esc` | menu / back |
| `Tab` | automap |
| `-` `=` | shrink / enlarge view |
| digits, `Y`/`N` | menu input & prompts |

## How it works (the two interesting bits)

**Picture (`DG_DrawFrame`).** The engine fills a 640×400 buffer of `0x00RRGGBB`
pixels. Each frame we ask the terminal its current size (`ioctl(TIOCGWINSZ)`),
average each source block down to one cell, choose an ASCII glyph from a
brightness ramp (`" .:-=+*#%@"`) for that cell's luma, and set its 24-bit ANSI
foreground colour to the block's mean colour. The whole frame is built in one
buffer and written after a cursor-home (`\033[H`) — one `write()`, no flicker.
Row count is capped to keep DOOM's aspect ratio, since terminal cells are ~2×
taller than they are wide.

**Key releases (`DG_GetKey`) — the one non-obvious thing.** A real keyboard
sends key-**down** *and* key-**up**, and DOOM needs both. A terminal only sends
the character; there is no key-up. So we synthesize one: each keystroke queues a
press immediately and schedules a matching release a few frames later. The result
is tap-to-step movement — and holding a key works because the terminal's own
auto-repeat keeps re-pressing it. This is the standard compromise every terminal
DOOM port makes.

Read [`platform_terminal.c`](platform_terminal.c) — it's heavily commented and
calls out exactly which lines are "just the contract" versus "terminal-specific."
