# Run genericdoom from C# (.NET / P/Invoke)

.NET's built-in interop is **P/Invoke** (`[DllImport]`). Delegates marshal to C
function pointers automatically, so the six callbacks are just methods.

## The one rule you must not forget

**Keep the delegates alive.** The library stores the function pointers and calls
them later. If the managed delegate objects get garbage-collected, those
pointers dangle and the engine crashes. Here we store the whole `Callbacks`
struct (which holds the delegates) in a `static` field, which roots them for the
program's lifetime.

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)).
2. Make the native library loadable. Easiest options:
   - copy it next to the built binary (`bin/Release/net8.0/`), **or**
   - macOS: `export DYLD_LIBRARY_PATH=/path/to/lib`
   - Linux: `export LD_LIBRARY_PATH=/path/to/lib`
3. Build and run:
   ```sh
   dotnet run -c Release -- -iwad /path/to/doom1.wad
   ```
   It writes `frame.ppm` at frame 100.

`[DllImport("doomgeneric")]` resolves to `libdoomgeneric.so` / `.dylib` /
`doomgeneric.dll` per OS automatically.

## Notes

- `string[] argv` marshals to C's `char**`, so engine flags like `-iwad` work.
  We prepend a fake `argv[0]` ("doomdemo") because C programs expect it.
- `dg_screen_buffer()` returns an `IntPtr`; `Marshal.Copy` pulls the pixels into
  an `int[]` (each `0x00RRGGBB`) with no `unsafe` code.
- Real input: return `1` from `OnGetKey` and set `pressed`/`doomKey` (a DOOM key
  code) from events your UI framework delivers.
