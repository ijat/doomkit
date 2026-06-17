#!/usr/bin/env python3
# =============================================================================
#  examples/languages/python/doom.py
# -----------------------------------------------------------------------------
#  Drive the doomkit shared library from Python using ctypes (the built-in
#  FFI in the standard library -- no pip packages). GPLv2. See LICENSE.
# =============================================================================
#
#  ctypes makes this short. The only thing to remember:
#    * Keep references to the CFUNCTYPE callback objects alive (here: in a module
#      list) so Python's GC does not free the function pointers the engine holds.
#
#  RUN (after building libdoomgeneric -- see bindings/README.md):
#      DYLD_LIBRARY_PATH=/path/to/lib python3 doom.py -iwad doom1.wad   # macOS
#      LD_LIBRARY_PATH=/path/to/lib   python3 doom.py -iwad doom1.wad   # Linux
# =============================================================================

import ctypes as C
import ctypes.util
import sys
import time

# ---- load the library ------------------------------------------------------
# Try the platform's normal lookup first; fall back to a few common names.
_name = ctypes.util.find_library("doomgeneric")
for cand in ([_name] if _name else []) + ["libdoomgeneric.dylib",
                                          "libdoomgeneric.so",
                                          "doomgeneric.dll"]:
    try:
        lib = C.CDLL(cand)
        break
    except OSError:
        continue
else:
    sys.exit("could not load libdoomgeneric; set DYLD_LIBRARY_PATH/LD_LIBRARY_PATH")

# ---- callback function types (match doomgeneric_capi.h) --------------------
INIT_FN       = C.CFUNCTYPE(None)
DRAW_FRAME_FN = C.CFUNCTYPE(None)
SLEEP_MS_FN   = C.CFUNCTYPE(None, C.c_uint32)
GET_TICKS_FN  = C.CFUNCTYPE(C.c_uint32)
GET_KEY_FN    = C.CFUNCTYPE(C.c_int, C.POINTER(C.c_int), C.POINTER(C.c_ubyte))
SET_TITLE_FN  = C.CFUNCTYPE(None, C.c_char_p)

class Callbacks(C.Structure):
    _fields_ = [
        ("init",             INIT_FN),
        ("draw_frame",       DRAW_FRAME_FN),
        ("sleep_ms",         SLEEP_MS_FN),
        ("get_ticks_ms",     GET_TICKS_FN),
        ("get_key",          GET_KEY_FN),
        ("set_window_title", SET_TITLE_FN),
    ]

# ---- declare the exported functions ----------------------------------------
lib.dg_set_callbacks.argtypes = [C.POINTER(Callbacks)]
lib.dg_create.argtypes        = [C.c_int, C.POINTER(C.c_char_p)]
lib.dg_tick.argtypes          = []
lib.dg_screen_buffer.restype  = C.POINTER(C.c_uint32)
lib.dg_resx.restype           = C.c_int
lib.dg_resy.restype           = C.c_int

# ---- the six callbacks ------------------------------------------------------
_start = time.monotonic()
_state = {"frame": 0}

def on_init():
    print(f"[py] init: framebuffer {lib.dg_resx()}x{lib.dg_resy()}")

def on_draw_frame():
    _state["frame"] += 1
    if _state["frame"] != 100:
        return
    w, h = lib.dg_resx(), lib.dg_resy()
    buf = lib.dg_screen_buffer()
    with open("frame.ppm", "wb") as f:
        f.write(f"P6\n{w} {h}\n255\n".encode("ascii"))
        out = bytearray(w * h * 3)
        for i in range(w * h):
            p = buf[i]                      # 0x00RRGGBB
            out[i * 3]     = (p >> 16) & 0xFF
            out[i * 3 + 1] = (p >> 8) & 0xFF
            out[i * 3 + 2] = p & 0xFF
        f.write(out)
    print(f"[py] wrote frame.ppm at frame {_state['frame']}")

def on_sleep_ms(ms):
    time.sleep(ms / 1000.0)

def on_get_ticks_ms():
    return int((time.monotonic() - _start) * 1000) & 0xFFFFFFFF

def on_get_key(pressed, key):
    return 0  # no input in this headless demo

def on_set_title(title):
    print(f"[py] title: {title.decode('ascii', 'replace')}")

# Keep the C-callable wrappers alive for the whole run (GC safety).
_cb = Callbacks(
    INIT_FN(on_init), DRAW_FRAME_FN(on_draw_frame), SLEEP_MS_FN(on_sleep_ms),
    GET_TICKS_FN(on_get_ticks_ms), GET_KEY_FN(on_get_key), SET_TITLE_FN(on_set_title),
)

def main():
    lib.dg_set_callbacks(C.byref(_cb))

    # Build argv: program name + user args, as a C char**.
    args = [b"doomdemo"] + [a.encode() for a in sys.argv[1:]]
    argv = (C.c_char_p * len(args))(*args)
    lib.dg_create(len(args), argv)

    for _ in range(200):
        lib.dg_tick()
    print("[py] done.")

if __name__ == "__main__":
    main()
