# Run doomkit from Python (ctypes)

Python's standard-library FFI is **ctypes** — no `pip install` needed. The whole
binding is one short file.

## The one rule

Keep the callback objects alive. `CFUNCTYPE(...)` wrappers must not be
garbage-collected while the engine holds their pointers, so we store them in the
module-level `_cb` struct for the program's lifetime.

## Run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)).
2. Make it loadable and run:
   ```sh
   # macOS
   DYLD_LIBRARY_PATH=/path/to/lib python3 doom.py -iwad /path/to/doom1.wad
   # Linux
   LD_LIBRARY_PATH=/path/to/lib   python3 doom.py -iwad /path/to/doom1.wad
   ```
   It writes `frame.ppm` at frame 100.

## Notes

- `lib.dg_screen_buffer()` returns a `POINTER(c_uint32)`; index it like an array
  (`buf[i]`, each `0x00RRGGBB`).
- The per-pixel Python loop is fine for a demo; for real-time display use `numpy`
  (`np.ctypeslib.as_array`) to copy the whole framebuffer at once.
- Real input: return `1` from `on_get_key` and write through the pointer args,
  e.g. `pressed[0] = 1; key[0] = 0xa3` (KEY_FIRE).
