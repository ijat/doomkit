# doomkit

[![CI](https://github.com/ijat/doomkit/actions/workflows/ci.yml/badge.svg)](https://github.com/ijat/doomkit/actions/workflows/ci.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](LICENSE)

A **clean, fully-documented, fully-tested reference of the DOOM porting layer** —
the small, elegant idea hiding inside [doomgeneric](https://github.com/ozkl/doomgeneric):
you can run all of DOOM on a new platform by implementing just **six functions**.

This package takes that porting layer, rewrites it as clean and heavily-commented
C, extracts its testable logic into pure modules with **100% unit-test coverage**,
relocates the platform implementations into `examples/`, and documents the whole
thing from first principles for someone who has never seen DOOM's internals.

> **What this is:** a teaching-quality, test-backed *reference + scaffold* for the
> doomkit porting interface, plus an architecture map of the engine.
>
> **What this is NOT:** a fork of the DOOM engine. The ~73,000-line engine is
> battle-tested GPL code from 1993; rewriting it would add bugs and value to no
> one. To run *real* DOOM you drop the upstream engine in behind this interface
> (see [Running real DOOM](#running-real-doom)). Everything here is the seam, not
> the engine.
>
> **No sound (by design).** doomkit covers the six video/input/timing callbacks;
> it does **not** provide audio. DOOM's sound is engine-internal, not part of the
> porting seam, so it sits outside doomkit's scope. If your platform has SDL or an
> OS audio API you can switch on the engine's own backend — see
> [docs/SOUND.md](docs/SOUND.md) for why, and how.

---

## But can it run on my ______?

Probably. Yes. Almost certainly yes.

DOOM has a near-religious tradition of being ported to anything with a CPU and a
pulse — calculators, pregnancy tests, ATMs, treadmills, a literal potato (okay,
that one had help). doomkit exists to make *you* the next person in that meme.

The bar is hilariously low: **if your thing can compile C, it can run DOOM.**

- 🧊 **Your fridge?** If it has a smart panel, it has a CPU. Rip and tear between
  grocery reminders.
- 🍞 **Your toaster?** Add a microcontroller and you've got DOOM at 35 tics per
  slice.
- 🔋 **Your powerbank?** The fancy ones have screens *and* a chip now. No excuse.
- 📟 **That weird industrial gadget at work?** Don't. (But you could.)

**"But it doesn't even have a screen!"** *Glorious.* DOOM doesn't care. The
[`null` port](examples/platforms/null) runs the entire engine and writes the
frame to a file — no display, no window, no GPU, nothing. Your screenless gizmo
is busy fragging demons in the dark; it just can't brag about it. Pipe those
pixels to an OLED, an LED matrix, a thermal printer, a row of servo-driven flip
dots — whatever you've got. Six functions stand between your hardware and
eternal internet glory.

So: stop reading, pick a victim from your junk drawer, and
[port DOOM to it](docs/PORTING.md). The world needs to know your smart kettle
can survive Hell.

> Legal-but-fun footnote: you still need the game data (a WAD). Use the free,
> redistributable [Freedoom](docs/WAD.md) so your toaster ships *legally*.

---

## Pick your platform — or your language

Two ways to use doomkit, depending on where you want DOOM to run:

- **Platform port** — implement the six `DG_*` callbacks in a file compiled
  *together with* the engine (a desktop window, the browser, a phone).
- **Language binding** — build the engine once as a shared library
  (`make lib`) and drive it from another language over its FFI.

The first block of rows is platforms; the second is languages. Find your row,
open that folder, follow its README.

| I want to run DOOM… | Example | How it connects | Start with | Status |
|----------------------|---------|-----------------|------------|--------|
| **Headless / CI / to learn** | [`examples/platforms/null/`](examples/platforms/null) | zero-dependency port | `make run-null` | ✅ runs |
| **In a desktop window** | [`examples/platforms/sdl/`](examples/platforms/sdl) | SDL2 port | `examples/platforms/sdl/` README | ◦ ref |
| **On a brand-new platform** | [`examples/platforms/template/`](examples/platforms/template) | fill in 6 TODOs | copy the file | — skeleton |
| **In a web browser** | [`examples/platforms/wasm/`](examples/platforms/wasm) | Emscripten → `<canvas>` | `make wasm` | ◦ ref |
| **On Android** | [`examples/platforms/kotlin-android/`](examples/platforms/kotlin-android) | Kotlin + NDK/JNI | Android Studio | ◦ skeleton |
| **From C / C++** | [`…/languages/c`](examples/languages/c) · [`cpp`](examples/languages/cpp) | link + register | `make lib` | ✅ verified |
| **From Go** | [`…/languages/go`](examples/languages/go) | cgo | `make lib` | ◦ ref |
| **From C# / .NET** | [`…/languages/csharp`](examples/languages/csharp) | P/Invoke | `make lib` | ✅ verified |
| **From Java** | [`…/languages/java`](examples/languages/java) | Panama FFM (JDK 22+) | `make lib` | ◦ ref |
| **From Python** | [`…/languages/python`](examples/languages/python) | ctypes | `make lib` | ✅ verified |
| **From Rust** | [`…/languages/rust`](examples/languages/rust) | `extern "C"` | `make lib` | ✅ verified |
| **From Node.js** | [`…/languages/nodejs`](examples/languages/nodejs) | koffi (FFI) | `make lib` | ✅ verified |

**Status:** ✅ built & exercised in this repo · ◦ correct reference code, needs
that toolchain (SDL / Emscripten / Go / JDK 22 / Android NDK) · — a starting
skeleton. Every row except `null` needs the [upstream engine](#running-real-doom)
and a WAD to actually *play* — they are ports/bindings, not a packaged game.

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
doomkit/
├── include/doomkit/      ← THE CONTRACT (read doomkit.h first)
│   ├── doomkit.h         · the six callbacks + lifecycle + framebuffer
│   ├── dg_keys.h             · DOOM key codes, explained
│   ├── dg_keyqueue.h         · input ring-buffer API
│   ├── dg_keymap.h           · host-key → DOOM-key API
│   ├── dg_palette.h          · paletted → RGB API
│   └── dg_framebuffer.h      · centring/scaling math API
├── src/                      ← PURE, TESTABLE LOGIC (100% covered)
│   ├── dg_keyqueue.c · dg_keymap.c · dg_palette.c · dg_framebuffer.c
├── bindings/                 ← flat C ABI so OTHER languages can drive the engine
│   └── doomgeneric_capi.{h,c} · register 6 callbacks at runtime; build a shared lib
├── examples/                 ← THE "IMPLEMENTATION CODE"
│   ├── platforms/  ← implement the 6 DG_*, compiled WITH the engine
│   │   ├── null/        · zero-dependency headless port — runnable! (make run-null)
│   │   ├── sdl/         · reference SDL2 desktop port
│   │   ├── template/    · copy-me skeleton for a new platform
│   │   ├── wasm/        · DOOM in the browser via Emscripten — runnable! (make wasm)
│   │   └── kotlin-android/· Android (Kotlin + NDK/JNI) skeleton
│   ├── languages/  ← FFI to a prebuilt libdoomgeneric (make lib)
│   │   └── c · cpp · go · csharp · csharp-avalonia (windowed) · java · python · rust · nodejs
│   └── minimal_main.c · the canonical Create()/Tick() loop
├── tests/                    ← Unity test suites (+ vendored Unity)
├── docs/                     ← PORTING.md · CONTRACT.md · ARCHITECTURE.md · WAD.md · SOUND.md · GLOSSARY.md
├── Makefile                  ← make test · coverage · run-null · lib · wasm
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
# open build/frame.ppm in any image viewer (or: magick build/frame.ppm out.png)
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
#include "doomkit/doomkit.h"

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
[`examples/platforms/null/platform_null.c`](examples/platforms/null/platform_null.c) (runnable) and
[`examples/platforms/sdl/platform_sdl.c`](examples/platforms/sdl/platform_sdl.c) (real window).

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
2. Get a WAD — the free, redistributable **Freedoom** or the shareware
   `doom1.wad`. Where to get one legally and how to use it: [docs/WAD.md](docs/WAD.md).
3. Compile your platform file + this package's `src/` + the engine, then run:

```sh
cc -Iinclude -I<engine_dir> \
   examples/platforms/template/platform_myplatform.c \
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
skeleton in [`examples/platforms/kotlin-android/`](examples/platforms/kotlin-android/).

| Language | FFI | Language | FFI |
|----------|-----|----------|-----|
| C / C++ | link + register | Java | Panama FFM (JDK 22+) |
| Go | cgo | Python | ctypes |
| C# | P/Invoke | Rust | `extern "C"` |
| Node.js | koffi (FFI) | Kotlin/Android | NDK + JNI |

Start with [`bindings/README.md`](bindings/README.md) (build the shared library),
then [`examples/languages/README.md`](examples/languages/README.md) (pick a
language).

## In the browser (WebAssembly)

`make wasm ENGINE=/path/to/doomgeneric/doomgeneric` compiles DOOM to WebAssembly
with Emscripten and renders it to a `<canvas>` — serve `build/wasm/` and pick a
WAD in the page. See [`examples/platforms/wasm/`](examples/platforms/wasm/).

## Documentation

| Doc | Read it for |
|-----|-------------|
| [`include/doomkit/doomkit.h`](include/doomkit/doomkit.h) | the contract itself, with the "power socket" mental model |
| [`docs/PORTING.md`](docs/PORTING.md) | beginner, step-by-step: port DOOM in six functions |
| [`docs/CONTRACT.md`](docs/CONTRACT.md) | precise semantics, threading, timing of every callback |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | the engine map: data flow of a frame + subsystem inventory |
| [`docs/SOUND.md`](docs/SOUND.md) | why sound is not in the porting seam, and how to enable the engine's own audio |
| [`docs/WAD.md`](docs/WAD.md) | where to get the game data (WAD) legally, and how to point the engine at it |
| [`docs/GLOSSARY.md`](docs/GLOSSARY.md) | plain-English definitions of every term used in these docs |
| [`examples/README.md`](examples/README.md) | what each example demonstrates |

---

## License

GPLv2 (`LICENSE`), because the helpers are derived from GPLv2 DOOM / doomgeneric
source. Attribution and the per-file derivation table are in
[`NOTICE.md`](NOTICE.md). The vendored Unity test framework is MIT.
