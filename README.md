# genericdoom-cleancode

A **clean, fully-documented, fully-tested reference of the DOOM porting layer** —
the small, elegant idea hiding inside [doomgeneric](https://github.com/ozkl/doomgeneric):
you can run all of DOOM on a new platform by implementing just **six functions**.

This package takes that porting layer, rewrites it as clean and heavily-commented
C, extracts its testable logic into pure modules with **100% unit-test coverage**,
relocates the platform implementations into `examples/`, and documents the whole
thing from first principles for someone who has never seen DOOM's internals.

> **What this is:** a teaching-quality, test-backed *reference + scaffold* for the
> genericdoom porting interface, plus an architecture map of the engine.
>
> **What this is NOT:** a fork of the DOOM engine. The ~73,000-line engine is
> battle-tested GPL code from 1993; rewriting it would add bugs and value to no
> one. To run *real* DOOM you drop the upstream engine in behind this interface
> (see [Running real DOOM](#running-real-doom)). Everything here is the seam, not
> the engine.

---

## Why this exists

The original engine mixes the genuinely reusable porting logic (a key queue, a
keycode map, palette conversion, framebuffer centring) into platform files and a
2,000-line `i_video.c`, with terse 1990s comments. That makes the *easy* part —
the part you actually touch when porting — look intimidating. This package pulls
that part out, names it, comments every line in plain English, and proves it
correct with tests.

---

## At a glance

```
genericdoom-cleancode/
├── include/genericdoom/      ← THE CONTRACT (read genericdoom.h first)
│   ├── genericdoom.h         · the six callbacks + lifecycle + framebuffer
│   ├── dg_keys.h             · DOOM key codes, explained
│   ├── dg_keyqueue.h         · input ring-buffer API
│   ├── dg_keymap.h           · host-key → DOOM-key API
│   ├── dg_palette.h          · paletted → RGB API
│   └── dg_framebuffer.h      · centring/scaling math API
├── src/                      ← PURE, TESTABLE LOGIC (100% covered)
│   ├── dg_keyqueue.c · dg_keymap.c · dg_palette.c · dg_framebuffer.c
├── bindings/                 ← flat C ABI so OTHER languages can drive the engine
│   └── doomgeneric_capi.{h,c} · register 6 callbacks at runtime; build a shared lib
├── examples/                 ← THE "IMPLEMENTATION CODE", as ports
│   ├── null/    · zero-dependency headless port — runnable! (make run-null)
│   ├── sdl/     · reference SDL2 port
│   ├── template/· copy-me skeleton for a new platform
│   ├── minimal_main.c
│   ├── kotlin-android/· Android (Kotlin + NDK/JNI) skeleton
│   └── languages/· C, C++, Go, C#, Java, Python, Rust — one binding each
├── tests/                    ← Unity test suites (+ vendored Unity)
├── docs/                     ← ARCHITECTURE.md · CONTRACT.md · PORTING.md
├── Makefile                  ← make test · make coverage · make run-null
├── LICENSE · NOTICE.md       ← GPLv2 (+ Unity MIT) and attribution
```

---

## Quick start

```sh
# 1. Run the unit tests (needs only a C compiler + make)
make test

# 2. See 100% coverage of the helper modules
make coverage

# 3. Build & run the dependency-free demo; it writes build/frame.ppm
make run-null
```

Verified on this machine:

```
make test      →  4 suites, 26 tests, 0 failures
make coverage  →  dg_keyqueue/keymap/palette/framebuffer: 100% lines, 100% branches
make run-null  →  translates keys, centres a 320×200 image into 640×400, writes frame.ppm
```

---

## The whole idea in one screen

Implement these six functions for your platform and DOOM runs:

```c
#include "genericdoom/genericdoom.h"

void     DG_Init(void);                                   // open display + input
void     DG_DrawFrame(void);                              // blit DG_ScreenBuffer
void     DG_SleepMs(uint32_t ms);                         // sleep
uint32_t DG_GetTicksMs(void);                             // monotonic ms clock
int      DG_GetKey(int *pressed, unsigned char *doomKey); // next key event, or 0
void     DG_SetWindowTitle(const char *title);            // optional

int main(int argc, char **argv) {
    doomgeneric_Create(argc, argv);     // boots the engine, calls your DG_Init
    for (;;) doomgeneric_Tick();         // one frame per call, ends in DG_DrawFrame
}
```

The two helpers that save you the most effort:

```c
// 1) Translate a host key, then queue it (do this while pumping OS events):
dg_keyqueue_push(&q, is_down, dg_keymap_from_sdl(host_key));

// 2) DG_GetKey then becomes a one-liner:
int DG_GetKey(int *pressed, unsigned char *doomKey) {
    return dg_keyqueue_pop(&q, pressed, doomKey);
}
```

That's the entire input path. The full worked version is
[`examples/null/platform_null.c`](examples/null/platform_null.c) (runnable) and
[`examples/sdl/platform_sdl.c`](examples/sdl/platform_sdl.c) (real window).

---

## The four pure helper modules

These are the only files that claim test coverage, because they are the only
parts with real, isolated logic. Each was extracted from the engine and made
side-effect-free so it can be tested in isolation.

| Module | Job | Extracted from |
|--------|-----|----------------|
| **dg_keyqueue** | fixed ring buffer carrying key events from the OS to the engine | the queue in doomgeneric's SDL port |
| **dg_keymap** | turn a host key symbol into a DOOM key code | `convertToDoomKey()` |
| **dg_palette** | expand DOOM's 8-bit paletted pixels into 32-bit / RGB565, with gamma | `cmap_to_fb()` / `I_SetPalette()` |
| **dg_framebuffer** | compute where to place & centre the image in a larger buffer | offset math in `I_FinishUpdate()` |

> One deliberate fix: the upstream vertical-centring offset didn't account for
> the framebuffer width and so didn't truly centre. `dg_framebuffer` computes the
> correct, directly-usable offset (and a test pins the centred result). See the
> comment in `src/dg_framebuffer.c`.

---

## Testing & coverage

Tests use **Unity** (vendored under `tests/vendor/unity/`, MIT — so `make test`
works with no install). Every branch of every helper is exercised:

```sh
make test       # build each suite and run it; non-zero exit on any failure
make coverage   # LLVM source-based coverage report for src/ (needs Apple/LLVM toolchain)
```

`make coverage` prints a per-file table; the target — and current result — is
**100%** across regions, functions, lines, and branches for all of `src/`.

---

## Running real DOOM

This package is the interface, not the engine. To get a playable build:

1. Get the upstream engine sources from <https://github.com/ozkl/doomgeneric>
   (the `doomgeneric/` folder of `.c`/`.h` files). They provide
   `doomgeneric_Create()` / `doomgeneric_Tick()` and the entire game.
2. Get a WAD (the free `doom1.wad` is fine).
3. Compile your platform file + this package's `src/` + the engine, then run:

```sh
cc -Iinclude -I<engine_dir> \
   examples/template/platform_myplatform.c \
   src/dg_keyqueue.c src/dg_keymap.c \
   <engine_dir>/*.c <platform libs> -o doom
./doom -iwad doom1.wad
```

Full walkthrough: **[docs/PORTING.md](docs/PORTING.md)**.

---

## Other languages (Go, C#, Java, Kotlin, Python, Rust, ...)

DOOM is C, but you can drive it from almost anything. The
[`bindings/`](bindings/) folder adds a flat C ABI (`dg_set_callbacks`,
`dg_create`, `dg_tick`, `dg_screen_buffer`, ...) so you build `libdoomgeneric`
once (`make lib ENGINE=/path/to/doomgeneric/doomgeneric`) and load it from any
language's FFI. There is one worked, documented example per language in
[`examples/languages/`](examples/languages/), plus an Android (Kotlin + NDK/JNI)
skeleton in [`examples/kotlin-android/`](examples/kotlin-android/).

| Language | FFI | Language | FFI |
|----------|-----|----------|-----|
| C / C++ | link + register | Java | Panama FFM (JDK 22+) |
| Go | cgo | Python | ctypes |
| C# | P/Invoke | Rust | `extern "C"` |
| Kotlin/Android | NDK + JNI | | |

Start with [`bindings/README.md`](bindings/README.md) (build the shared library),
then [`examples/languages/README.md`](examples/languages/README.md) (pick a
language).

## Documentation

| Doc | Read it for |
|-----|-------------|
| [`include/genericdoom/genericdoom.h`](include/genericdoom/genericdoom.h) | the contract itself, with the "power socket" mental model |
| [`docs/PORTING.md`](docs/PORTING.md) | beginner, step-by-step: port DOOM in six functions |
| [`docs/CONTRACT.md`](docs/CONTRACT.md) | precise semantics, threading, timing of every callback |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | the engine map: data flow of a frame + subsystem inventory |
| [`examples/README.md`](examples/README.md) | what each example demonstrates |

---

## License

GPLv2 (`LICENSE`), because the helpers are derived from GPLv2 DOOM / doomgeneric
source. Attribution and the per-file derivation table are in
[`NOTICE.md`](NOTICE.md). The vendored Unity test framework is MIT.
