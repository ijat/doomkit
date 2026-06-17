# Run doomkit from C++

C++ talks to the library through the same `extern "C"` API as C — no wrapper
layer needed. This example just adds a small `Framebuffer` class for idiomatic
access and uses `<chrono>`/`<thread>` for timing.

## One C++ gotcha

Callbacks you hand to the C API must be **plain functions or non-capturing
lambdas**. A *capturing* lambda holds state and cannot be converted to a C
function pointer, so it won't compile in `dg_callbacks`. Keep per-run state in
namespace-scope variables (as here) or a singleton.

## Build & run

```sh
# build libdoomgeneric first — see bindings/README.md
c++ -std=c++17 -I../../../bindings main.cpp -L/path/to/lib -ldoomgeneric -o doomdemo
./doomdemo -iwad /path/to/doom1.wad   # writes frame.ppm at frame 100
```

On Linux add `-Wl,-rpath,/path/to/lib` so the loader finds the `.so` at runtime.

## Notes

- `dg_screen_buffer()` returns `uint32_t*` to `dg_resx()*dg_resy()` pixels in
  `0x00RRGGBB`. The `Framebuffer` class wraps it; read it inside `on_draw_frame`.
- A real app would blit to a window (SDL/Qt/Win32) instead of saving a PPM, and
  feed real key events through `on_get_key` (translate host keys with the same
  idea as `dg_keymap`).
