#!/bin/sh
# =============================================================================
#  examples/wasm/build.sh  --  Compile DOOM to WebAssembly with Emscripten.
# -----------------------------------------------------------------------------
#  Same result as `make wasm` from the project root, as a standalone script so
#  you can read exactly what emcc is invoked with. GPLv2. See LICENSE.
#
#  Usage:
#    ENGINE=/path/to/doomgeneric/doomgeneric ./build.sh
#  Requires the Emscripten SDK on PATH (emcc). Output goes to ./out/.
# =============================================================================
set -e

ENGINE="${ENGINE:-../../../../doomgeneric/doomgeneric}"  # upstream engine sources
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$HERE/../../.."                                    # doomkit project root
OUT="$HERE/out"

command -v emcc >/dev/null 2>&1 || { echo "ERROR: emcc not found. Install the Emscripten SDK (emsdk) and 'source emsdk_env.sh'."; exit 1; }
test -f "$ENGINE/doomgeneric.h" || { echo "ERROR: engine not found at ENGINE=$ENGINE (override: ENGINE=/path/to/doomgeneric/doomgeneric)"; exit 1; }

mkdir -p "$OUT"

# Engine sources = every .c except the desktop platform files and the
# SDL/Allegro/GUS-only files (same portable set as `make lib`). platform_wasm.c
# provides the DG_* symbols, so no doomgeneric_*.c platform file is included.
ENGINE_SRCS=""
for f in "$ENGINE"/*.c; do
  case "$(basename "$f")" in
    doomgeneric_*.c|i_sdl*.c|i_allegro*.c|gusconf.c|mus2mid.c|icon.c) continue ;;
  esac
  ENGINE_SRCS="$ENGINE_SRCS $f"
done

emcc -O2 -w \
  -I"$ENGINE" -I"$ROOT/include" \
  "$HERE/platform_wasm.c" "$ROOT/src/dg_keyqueue.c" $ENGINE_SRCS \
  -sINVOKE_RUN=0 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS=_main,_wasm_push_key \
  -sEXPORTED_RUNTIME_METHODS=callMain,FS,HEAPU32 \
  -o "$OUT/doom.js"

cp "$HERE/index.html" "$OUT/"
echo "built $OUT/doom.{js,wasm} + index.html"
echo "serve it (browsers block file://), e.g.:  (cd '$OUT' && python3 -m http.server 8000)"
echo "then open http://localhost:8000/ and pick a WAD."
