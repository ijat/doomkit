# Run doomkit from Go (cgo)

Go's built-in FFI is **cgo**. The interesting part here is that C must call *back*
into Go for the six callbacks — cgo supports that via `//export`.

## File layout

| File | Role |
|------|------|
| `main.go` | the callbacks (`//export goDG*`), framebuffer read, and the loop |
| `bridge.c` | the one bit of C that bundles the exported Go funcs into `dg_callbacks` (cgo forbids defining this in `main.go`'s preamble) |
| `go.mod` | module file |

## Why two files?

A Go file that uses `//export` may only **declare** C things in its preamble,
not **define** them. Building the `dg_callbacks` struct is a definition, so it
lives in `bridge.c`, which includes the cgo-generated `_cgo_export.h` to see the
`goDG*` symbols.

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)).
2. Put the library where the `LDFLAGS` in `main.go` expect it — a `lib/`
   subfolder here:
   ```sh
   mkdir -p lib && cp /path/to/libdoomgeneric.* lib/
   ```
3. Build and run:
   ```sh
   go build -o doomdemo .
   ./doomdemo -iwad /path/to/doom1.wad   # writes frame.ppm at frame 100
   ```

The `#cgo` lines in `main.go` set the include path (to `bindings/`) and the
linker path (to `./lib`), including an `-rpath` so the loader finds the library
at runtime.

## Notes

- `unsafe.Slice(...)` views the C framebuffer as a `[]uint32` with no copy — read
  it inside `goDGDrawFrame`.
- `cArgv()` converts `os.Args` into a C `argv` so engine flags like `-iwad`
  reach the engine; it frees the C memory when done.
- Real input: return `1` from `goDGGetKey` and set `*pressed` / `*key` (a DOOM
  key code) per event you've queued from your window toolkit.
