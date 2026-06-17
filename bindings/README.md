# bindings — the shared library every language example uses

The desktop language examples (C, C++, Go, C#, Java, Python, Rust) all load the
**same shared library**, `libdoomgeneric`, and talk to it through the flat C ABI
declared in [`doomgeneric_capi.h`](doomgeneric_capi.h). This folder is that ABI.
Build the library **once**, then point any example at it.

(Android is the exception — it compiles the engine into the app's own `.so` via
the NDK instead of loading a prebuilt one. See `examples/platforms/kotlin-android/`.)

## Why a registration API instead of plain `DG_*`?

The base doomkit contract wants the platform to *define* six link-time
symbols (`DG_Init`, ...). Managed languages (C#, Java, ...) cannot define raw C
symbols. So this shim **inverts** the wiring: you *register* six function
pointers at runtime with `dg_set_callbacks()`, and the shim provides the real
`DG_*` symbols that simply forward to whatever you registered.

```
engine ──calls──▶ DG_DrawFrame (in the shim) ──calls──▶ your registered callback
```

Every FFI understands "call a function" and "pass a function pointer", so this
one indirection makes the engine reachable from all of them.

## The ABI (full reference in the header)

```c
void      dg_set_callbacks(const dg_callbacks *cb); // register 6 fn pointers (call first)
void      dg_create(int argc, char **argv);         // boot engine + load WAD
void      dg_tick(void);                             // advance one frame
uint32_t *dg_screen_buffer(void);                    // RESX*RESY pixels, 0x00RRGGBB
int       dg_resx(void);
int       dg_resy(void);
```

## Building `libdoomgeneric`

You need the upstream DOOM engine sources (this package does not vendor them):

```sh
git clone https://github.com/ozkl/doomgeneric
```

### The easy way: `make lib`

From the package root, point the `ENGINE` variable at the upstream
`doomgeneric/` folder (the one with `d_main.c`):

```sh
make lib ENGINE=/path/to/doomgeneric/doomgeneric
# -> build/lib/libdoomgeneric.{dylib,so}
```

That target compiles the verified portable engine set + this shim and links the
shared library for your OS. The rest of this section explains what it does under
the hood (and how to do it by hand on Windows, which the Makefile does not
cover).

### By hand

Compile every engine `.c` **except** the desktop platform files
(`doomgeneric_*.c` — they define `DG_*` and would clash), plus this shim, into a
shared library. The set of engine files is the Makefile's `SRC_DOOM` list minus
`doomgeneric_xlib.o`.

### macOS (verified build command)

```sh
mkdir -p out
# 1) compile engine objects (skip platform files) + the shim
for f in "$ENGINE"/*.c; do
  case "$f" in *doomgeneric_*.c) continue;; esac      # skip platform impls
  cc -fPIC -fvisibility=hidden -w -I"$ENGINE" -Ibindings -c "$f" -o "out/$(basename "$f" .c).o"
done
cc -fPIC -fvisibility=hidden -w -I"$ENGINE" -Ibindings -c bindings/doomgeneric_capi.c -o out/capi.o
# 2) link the dylib
cc -dynamiclib -o out/libdoomgeneric.dylib out/*.o
```

> The upstream `SRC_DOOM` list also omits a few `i_*` files that need SDL/Allegro
> (e.g. `i_sdlsound.c`). The glob above skips only `doomgeneric_*.c`; if your
> checkout includes SDL-only `i_*` files, exclude those too, or just compile the
> exact `SRC_DOOM` list (minus `doomgeneric_xlib.o`).

### Linux

Same, but produce a `.so`:

```sh
cc -shared -o out/libdoomgeneric.so out/*.o
```

### Windows (MinGW)

```sh
cc -shared -o doomgeneric.dll out/*.o -Wl,--out-implib,libdoomgeneric.a
```

`DG_API` in the header already adds `__declspec(dllexport)` on Windows.

## Smoke-test it

A 20-line C program that `dlopen`s the library and calls `dg_resx()` /
`dg_set_callbacks()` (neither needs a WAD) is enough to confirm the symbols
resolve before you bring a whole language runtime into the picture. Expected
output: `640` / `400`.

## Then pick a language

Each folder under [`../examples/languages/`](../examples/languages/) loads this
library and registers callbacks in that language's idiom. Start with whichever
you know best — they all do the same thing.
