# Run doomkit from Zig

Zig's C FFI is built into the language — no packages needed. You declare the
library's functions as `extern`, describe the callback bundle as an
`extern struct` of `callconv(.C)` function pointers, write the callbacks as
plain `callconv(.C)` functions, and let `build.zig` do the linking.

Tested against **Zig 0.14**. On Zig 0.15+ the calling-convention literal became
lowercase, so replace every `callconv(.C)` with `callconv(.c)`.

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)).
2. Put the library in a `lib/` folder next to `build.zig`:

   ```sh
   mkdir -p lib && cp /path/to/libdoomgeneric.* lib/
   ```

3. Build and run:

   ```sh
   zig build run -- -iwad /path/to/doom1.wad   # writes frame.ppm at frame 100
   ```

Like the other language examples, this is a headless demo: it boots the engine,
runs 200 frames, and dumps `frame.ppm` at frame 100 so you can confirm real
pixels came out.

## Two things to watch

1. **Pixel format is `0x00RRGGBB`.** `onDrawFrame` unpacks each pixel as
   R/G/B before writing the PPM. If red and blue look swapped on your display,
   that's the byte order to fix.
2. **Keep the callbacks alive.** Zig has no GC, so the file-scope `callbacks`
   value lives for the whole program — the engine's pointers into it stay valid.
