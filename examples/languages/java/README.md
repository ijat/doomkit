# Run doomkit from pure Java (Project Panama / FFM)

"Pure Java" means **no JNI and no hand-written C glue**. Java 22's Foreign
Function & Memory API (`java.lang.foreign`) can both call native functions
(downcalls) and expose Java methods as C function pointers (upcalls) — which is
exactly what doomkit's callback design needs.

## Concepts in `Doom.java`

| FFM piece | Used for |
|-----------|----------|
| `SymbolLookup.libraryLookup(...)` | open `libdoomgeneric` |
| `Linker.downcallHandle(...)` | call `dg_create`, `dg_tick`, `dg_resx`, ... |
| `Linker.upcallStub(...)` | turn `onInit`, `onDrawFrame`, ... into C function pointers |
| `Arena` / `MemorySegment` | the callback struct, the `argv` array, and reading the framebuffer |

## Requirements

- **JDK 22 or newer** (FFM is stable there; on 21 it was preview and a couple of
  method names differ).

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)).
2. Single-file source launch (no compile step needed):
   ```sh
   java --enable-native-access=ALL-UNNAMED \
        -Djava.library.path=/path/to/lib \
        Doom.java -iwad /path/to/doom1.wad
   ```
   It writes `frame.ppm` at frame 100. `--enable-native-access` silences the
   restricted-method warning; `-Djava.library.path` is where it looks for the
   native library.

## Notes

- The `Arena` (`ARENA`) is **shared and never closed** on purpose: it owns the
  upcall stubs, so it must outlive every engine callback.
- `dg_screen_buffer()` returns a zero-length segment; `.reinterpret(w*h*4)` gives
  it a real size so you can read pixels (`0x00RRGGBB`).
- Real input: return `1` from `onGetKey` and write the DOOM key code / pressed
  flag into the two `MemorySegment` out-parameters with `set(JAVA_INT/JAVA_BYTE...)`.
