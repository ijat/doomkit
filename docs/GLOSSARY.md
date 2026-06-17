# Glossary — the jargon, in plain English

The other docs try to use plain language, but DOOM and C porting come with their
own vocabulary. If a term in any doc made you pause, it's probably here. Ordered
roughly from "DOOM data" → "the porting contract" → "C / FFI plumbing".

---

### WAD / IWAD / PWAD
The game's data file. **WAD** = "Where's All the Data". An **IWAD** is a complete
game (you need exactly one); a **PWAD** is an optional add-on layered on top. Full
guide: [WAD.md](WAD.md).

### Lump
One named chunk *inside* a WAD — a single texture, sound, map, etc. The WAD is
basically a zip of lumps.

### The engine
The ~73,000 lines of original id Software / doomgeneric C that *are* DOOM: input
handling, the game simulation, the 3D renderer. doomkit does **not** include or
modify it — you link it in unchanged. See [ARCHITECTURE.md](ARCHITECTURE.md).

### Porting layer / porting seam / "the contract"
The tiny boundary between the engine and your platform: six callbacks, two
lifecycle calls, one framebuffer. Implementing it is the *entire* job of a port.
This is what doomkit documents and tests. See [CONTRACT.md](CONTRACT.md).

### Callback
A function *you* write that the *engine* calls (the `DG_*` functions). The engine
is in charge of the loop; it "calls you back" when it needs a frame drawn, a key,
or the time. The inverse of a normal library call where you call it.

### Framebuffer
A flat block of memory holding one screen image as pixels, row by row. DOOM fills
`DG_ScreenBuffer`; your `DG_DrawFrame` copies it to the actual display.

### Pixel format (`0x00RRGGBB`)
How one pixel's colour is packed into a number. doomkit's default is a 32-bit
value with one byte each for red, green, blue (top byte unused) — what almost
every modern display wants. See `doomkit.h`.

### Paletted / 8-bit / palette index / CMAP256
The 1993 way to store colour: each pixel is a small number (0–255) that *indexes*
a 256-colour table (the "palette"), instead of carrying full RGB. DOOM renders
internally this way; the `dg_palette` helper expands it to `0x00RRGGBB`. Building
with `-DCMAP256` keeps the 8-bit form for tiny/paletted displays.

### Blit
"Block image transfer" — jargon for *copy a rectangle of pixels* from one buffer
to another (e.g. `DG_ScreenBuffer` → your window's surface).

### Pitch / stride
The number of **bytes** from the start of one pixel row to the start of the next.
Usually `width × bytes-per-pixel` (for doomkit, `DOOMGENERIC_RESX * 4`). Getting
it wrong produces a skewed/diagonal image. Mentioned in [PORTING.md](PORTING.md)
troubleshooting.

### Centring / scaling offset
DOOM renders at 320×200; the output buffer may be larger (default 640×400). The
`dg_framebuffer` helper computes *where* to place the image so it's centred
instead of stuck in a corner.

### Tic (a.k.a. tick)
DOOM's fixed simulation step: the game world advances **35 times per second**
(35 Hz), every step the same size, regardless of how fast you draw frames. Note:
this is *not* the same as `doomgeneric_Tick()` — that function may render several
display frames and advance zero or more tics per call. See [CONTRACT.md](CONTRACT.md).

### Monotonic clock
A time source that **only ever increases** and never jumps backward (unlike a
wall clock that can be adjusted by NTP or DST). `DG_GetTicksMs` must be monotonic
or the game's timing breaks. The absolute starting value doesn't matter.

### BSP (Binary Space Partitioning)
The data structure DOOM precomputes for each map so the software renderer can
draw walls in the right front-to-back order, fast, without a GPU. You never touch
it — it's deep inside the `r_*` renderer files.

### Software renderer
A 3D renderer that runs entirely on the CPU, writing pixels into a memory buffer
— no GPU, no OpenGL/Vulkan. That's why DOOM only needs you to provide a place to
copy finished pixels, and why it ran on 1993 hardware (and your toaster).

---

## C / language-binding terms

### Shared library (`.so` / `.dylib` / `.dll`)
A compiled blob of code loaded at runtime instead of baked into your program. The
language examples build the engine once as `libdoomgeneric` and load *that* from
Go, Python, etc. `make lib` produces it. See [bindings/README.md](../bindings/README.md).

### FFI (Foreign Function Interface)
A language's mechanism for calling functions written in another language —
specifically calling C from Go/Python/Rust/.... Each language has its own FFI
(cgo, ctypes, P/Invoke, …); the table in the root README lists them.

### ABI (Application Binary Interface)
The binary-level contract for *how* a compiled function is called: argument
layout, calling convention, symbol names. The `bindings/` shim exposes a **flat C
ABI** (`dg_create`, `dg_tick`, …) because every FFI understands plain C calls.

### Symbol
The name of a function or variable as it exists in a compiled object/library
(e.g. `DG_DrawFrame`). "Duplicate symbol" link errors mean two files defined the
same name — the reason [PORTING.md](PORTING.md) tells you to exclude the engine's
own `doomgeneric_*.c` platform files when you provide your own `DG_*`.

### Callback registration (`dg_set_callbacks`)
Because managed languages can't *define* raw C symbols like `DG_Init`, the
bindings flip it around: you hand the shim six **function pointers** at runtime
and it forwards the engine's calls to them. See [bindings/README.md](../bindings/README.md).

### Toolchain
The set of tools that turns source into a runnable program: a compiler (`cc`,
`clang`, `gcc`, `emcc`), `make`, and any platform SDK (SDL2, Emscripten, the JDK,
the Android NDK). [PORTING.md](PORTING.md#prerequisites) lists what each target needs.

### PPM
A trivially simple, uncompressed image format (plain pixel values with a tiny
header). The `null` demo writes one (`frame.ppm`) so you can *see* its output
with no display backend. Most image viewers open it; if yours won't, convert it
(`magick frame.ppm frame.png`).
