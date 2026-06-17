# Run doomkit in the browser (WebAssembly)

DOOM compiled to WebAssembly with **Emscripten**, rendering to a `<canvas>`.
This is a normal doomkit platform port — `platform_wasm.c` implements the six
`DG_*` callbacks — that happens to target the browser. There is no shared
library and no FFI here: Emscripten compiles the engine, this port, and
`dg_keyqueue` into one `.wasm` module.

```
 browser
 ├─ index.html ── canvas + key handling + WAD picker (the "screen & keyboard")
 └─ doom.js/.wasm
     └─ platform_wasm.c ── DG_* ◀── DOOM engine    (+ dg_keyqueue for input)
        DG_DrawFrame ──EM_ASM──▶ dgDrawFrame(ptr,w,h) paints the canvas
        wasm_push_key ◀──────────  key events from index.html
```

## What you need

1. **The Emscripten SDK** (`emcc` on your PATH):
   ```sh
   git clone https://github.com/emscripten-core/emsdk
   cd emsdk && ./emsdk install latest && ./emsdk activate latest
   source ./emsdk_env.sh
   ```
2. **The upstream DOOM engine sources** (this package does not vendor them) —
   the `doomgeneric/` folder with `d_main.c`, `i_video.c`, …
3. **A WAD** (e.g. the free shareware `doom1.wad`). You do **not** bundle it into
   the build — you pick it in the page at runtime.

## Build

Either from the project root:

```sh
make wasm ENGINE=/path/to/doomgeneric/doomgeneric
# -> build/wasm/{doom.js,doom.wasm,index.html}
```

…or with the standalone script (it prints the exact `emcc` command it runs):

```sh
cd examples/wasm
ENGINE=/path/to/doomgeneric/doomgeneric ./build.sh   # -> examples/wasm/out/
```

## Run

Browsers block `file://` for WASM + `fetch`, so serve the output folder over
HTTP:

```sh
cd build/wasm        # or examples/wasm/out
python3 -m http.server 8000
```

Open <http://localhost:8000/>, click **Choose File**, pick your `doom.wad`, and
play. Arrows move, **Ctrl** fires, **Space** opens doors, **Enter/Esc** drive the
menus.

## How the key pieces work

| Piece | What it does |
|-------|--------------|
| `-sINVOKE_RUN=0` | `main()` does not auto-run; the page calls `Module.callMain(['-iwad','/doom.wad'])` **after** writing the WAD into the virtual FS |
| `Module.FS.writeFile('/doom.wad', bytes)` | puts the user's picked WAD where the engine can open it |
| `emscripten_set_main_loop(doomgeneric_Tick, 0, 1)` | hands the per-frame loop to the browser (no blocking) |
| `EM_ASM({ dgDrawFrame(...) }, ...)` | C → JS each frame to paint `DG_ScreenBuffer` onto the canvas |
| `_wasm_push_key` (exported) | JS → C: queue a DOOM key event into `dg_keyqueue` |

## Notes & limits

- **Pixel order:** `DG_ScreenBuffer` is `0x00RRGGBB`; `dgDrawFrame` unpacks each
  word into the canvas's R,G,B,A bytes, so colours are correct regardless of
  endianness.
- **Sound is off** in this demo (no SDL/Web Audio backend wired). The engine's
  no-op sound stub is used.
- **Not built in this repo's CI** — it needs the Emscripten SDK + engine sources.
  The C is a standard doomkit port; the build was validated by syntax-checking
  `platform_wasm.c` against the contract headers.
- For a zero-pick experience you can instead preload the WAD at build time with
  `--preload-file your.wad@/doom.wad` and call `Module.callMain` immediately.
