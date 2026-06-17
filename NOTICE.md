# Notices & attribution

This package is **GPLv2** (see `LICENSE`), because its helper modules are derived
from GPLv2 source.

## What is derived from what

| Part of this package | Derived from | License |
|----------------------|--------------|---------|
| `include/doomkit/doomkit.h`, `dg_keys.h` | doomgeneric `doomgeneric.h`, `doomkeys.h` (id Software DOOM) | GPLv2 |
| `src/dg_keyqueue.c`, `src/dg_keymap.c` | the key queue & `convertToDoomKey()` in doomgeneric's SDL port | GPLv2 |
| `src/dg_palette.c` | `cmap_to_fb()` / `I_SetPalette()` in doomgeneric `i_video.c` | GPLv2 |
| `src/dg_framebuffer.c` | offset math in `I_FinishUpdate()` in doomgeneric `i_video.c` | GPLv2 |
| docs, examples, tests | original to this package | GPLv2 (same project) |
| `tests/vendor/unity/` | ThrowTheSwitch **Unity** v2.6.0 | MIT (see `tests/vendor/unity/LICENSE.txt`) |

## Upstream credits

- **DOOM** © 1993–1996 id Software, Inc.
- **Chocolate Doom** lineage © 2005–2014 Simon Howard and contributors.
- **doomgeneric** © its authors — <https://github.com/ozkl/doomgeneric>.
- **Unity** test framework © Mike Karlesky, Mark VanderVoord, Greg Williams (MIT).

The MIT license of Unity is compatible with GPLv2; Unity is used only as a test
dependency and is not part of the shipped library.
