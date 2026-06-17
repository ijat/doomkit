# Run doomkit in a window with C# + Avalonia

The plain [`../csharp`](../csharp) example drives the engine over P/Invoke and
writes a `.ppm` file. This one is the same binding, but it **draws each frame to
a desktop window** using [Avalonia](https://avaloniaui.net) and wires up enough
keys to actually play:

| Key | Action | DOOM code |
|-----|--------|-----------|
| `↑` / `↓` | move forward / back | `KEY_UPARROW` / `KEY_DOWNARROW` |
| `←` / `→` | turn left / right | `KEY_LEFTARROW` / `KEY_RIGHTARROW` |
| `E` | use — open doors, flip switches | `KEY_USE` (`0xa2`) |
| `Space` | shoot | `KEY_FIRE` (`0xa3`) |
| `Enter` | confirm menu selections | `KEY_ENTER` (`13`) |

(Press <kbd>Enter</kbd> through the menu to start a game. This is a minimal
input example — strafe, run, and weapon-select keys are left for you to add.)

## How it works

```
 background thread                         UI thread (Avalonia)
 ───────────────                           ────────────────────
 dg_create / dg_tick loop                  Window ▸ Image ▸ WriteableBitmap
        │  every frame:                          ▲
        ▼  OnDrawFrame                           │ Dispatcher.UIThread.Invoke
 copy dg_screen_buffer() ─────────── blit pixels ┘  + InvalidateVisual
```

- **DOOM owns its own loop** (`dg_tick` blocks via your `SleepMs`), so it runs on
  a background thread; the UI thread stays free for Avalonia.
- Each `OnDrawFrame`, the engine's `dg_screen_buffer()` (`0x00RRGGBB` pixels) is
  copied into a `WriteableBitmap` and shown in an `Image`.
- `Dispatcher.UIThread.Invoke` bridges the two threads. **Invoke** (not `Post`)
  blocks DOOM until the frame is on screen — no buffer race, and it paces the
  engine to the display.

## The three rules you must not forget

1. **Keep the delegates alive.** The library stores the function pointers and
   calls them later; if the managed `Callbacks` struct is collected, those
   pointers dangle and the engine crashes. Here it lives in the `_cb` field.
2. **Force opaque alpha.** DOOM pixels are `0x00RRGGBB` — in memory `[B,G,R,A]`,
   already `Bgra8888` byte order, but with alpha `0` (transparent). We OR in
   `0xFF000000` so the window isn't see-through.
3. **Cross threads for input.** `KeyDown`/`KeyUp` fire on the UI thread, but the
   engine calls `GetKey` on the DOOM thread. A `ConcurrentQueue` carries
   `(pressed, doomKey)` events across; `GetKey` drains one per call (returns `0`
   when empty). A `HashSet` of held keys drops OS auto-repeat so each physical
   press is one down + one up.

## Build & run

1. Build `libdoomgeneric` (see [`bindings/README.md`](../../../bindings/README.md)):
   ```sh
   make lib                       # from the repo root; outputs build/lib/libdoomgeneric.dylib
   ```
2. Make the native library loadable — copy it next to this example:
   ```sh
   cp ../../../build/lib/libdoomgeneric.dylib .      # macOS  (.so on Linux, doomgeneric.dll on Windows)
   ```
   The `.csproj` copies it to the output folder on build. (Alternatively, set
   `DYLD_LIBRARY_PATH` / `LD_LIBRARY_PATH` to `build/lib`.)
3. Run with a WAD:
   ```sh
   dotnet run -- -iwad /path/to/doom1.wad
   ```
   A window opens playing the DOOM demo.

## Add more keys

`TryMap` in `Program.cs` is the whole keymap — add `case Key.W => KEY_UPARROW`
etc. from [`dg_keys.h`](../../../include/doomkit/dg_keys.h) for movement, weapon
digits, `Enter`/`Escape` for menus, and so on. (In a C/C++ port you'd use the
[`dg_keymap`](../../../include/doomkit/dg_keymap.h) and
[`dg_keyqueue`](../../../include/doomkit/dg_keyqueue.h) helpers for exactly this;
they aren't exported over the C ABI, so here we do the same job in a few lines of
C#.)
