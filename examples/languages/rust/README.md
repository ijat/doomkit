# Run genericdoom from Rust

Rust's FFI is built into the language — no crates needed. You declare the C
functions in an `extern "C"` block, write the callbacks as `extern "C" fn`, and
link the library via `build.rs`.

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)).
2. Put the library in a `lib/` folder next to `Cargo.toml`:
   ```sh
   mkdir -p lib && cp /path/to/libdoomgeneric.* lib/
   ```
3. Build and run:
   ```sh
   cargo run -- -iwad /path/to/doom1.wad   # writes frame.ppm at frame 100
   ```
   `build.rs` adds the link search path and an rpath so the loader finds the
   library at runtime.

## Notes

- `dg_screen_buffer()` returns `*mut u32`; `std::slice::from_raw_parts` views it
  as a `&[u32]` (each `0x00RRGGBB`) with no copy.
- The demo uses `static mut` for brevity because it is single-threaded; a real
  app should prefer `OnceCell`/atomics or pass state another way.
- Real input: return `1` from `on_get_key` and write through the raw pointers,
  e.g. `*pressed = 1; *key = 0xa3;` (KEY_FIRE) inside an `unsafe` block.
