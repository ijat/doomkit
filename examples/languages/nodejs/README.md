# Run doomkit from Node.js

Node can drive doomkit three ways. This example uses **koffi**, the modern Node
FFI ("ctypes for Node") — it loads the prebuilt `libdoomgeneric` and registers
the six callbacks with no native compilation. The other two options are noted at
the bottom.

## The one rule

Keep the registered callbacks alive. `koffi.register(...)` returns a pointer the
engine stores and calls later; if the JS functions/pointers get
garbage-collected, the engine calls freed memory. We keep them in the
module-level `callbacks` object for the whole run.

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)):
   ```sh
   make lib ENGINE=/path/to/doomgeneric/doomgeneric   # from the project root
   ```
2. Install the FFI dependency and place the library where the script looks:
   ```sh
   cd examples/languages/nodejs
   npm install
   mkdir -p lib && cp /path/to/libdoomgeneric.* lib/   # or set DOOMKIT_LIB=/abs/path
   ```
3. Run:
   ```sh
   node doom.js -iwad /path/to/doom1.wad               # writes frame.ppm at frame 100
   ```

## How it maps

| koffi piece | Role |
|-------------|------|
| `koffi.load(...)` | open the shared library |
| `koffi.struct('dg_callbacks', {...})` | the 6-function-pointer struct |
| `koffi.proto(...)` + `koffi.register(fn, ptr)` | turn JS functions into C callbacks |
| `lib.func('void dg_create(int, const char **)')` | declare an exported function |
| `koffi.decode(ptr, koffi.array('uint32_t', w*h))` | read the framebuffer (`0x00RRGGBB`) |

## Notes

- `onSleepMs` busy-waits to keep this a single synchronous file. A real app would
  drive `dg_tick()` from a timer (`setInterval`/`setImmediate`) and never block
  the event loop.
- Real input: return `1` from `onGetKey` and write the DOOM key code / pressed
  flag into the pointer args with `koffi.encode(...)`.

## Other ways to use doomkit from Node

- **Native N-API addon** (`node-gyp` + `node-addon-api`): compile a `.node` that
  links the engine and exposes JS methods. More setup, but the most "official"
  native path and lets C call JS via thread-safe functions.
- **Run the WebAssembly build in Node**: `make wasm` output (`doom.js`) runs
  under Node's `WebAssembly` too — headless, so you'd save frames to disk
  instead of a `<canvas>`. See [`examples/platforms/wasm/`](../../platforms/wasm/).
