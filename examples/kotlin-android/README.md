# Run genericdoom on Android (Kotlin + NDK/JNI)

Android's built-in way to use native code is the **NDK + JNI**. Instead of
loading a prebuilt `libdoomgeneric` and registering callbacks (the desktop
pattern), Android apps compile the engine *into the app's own `.so`* and
implement the six `DG_*` functions directly in C++. That C++ renders into a
`Surface` and reads keys Kotlin forwards.

```
Kotlin (MainActivity) ──JNI──▶ native-doom.cpp ──defines──▶ DG_* ◀──calls── DOOM engine
        Surface  ───────────────▶  ANativeWindow (DG_DrawFrame draws here)
        key events ─────────────▶  dg_keyqueue  (DG_GetKey reads here)
```

## Files

| File | Role |
|------|------|
| `app/src/main/cpp/native-doom.cpp` | JNI bridge; implements `DG_*`, renders to the Surface |
| `app/src/main/cpp/CMakeLists.txt` | builds engine + bridge into `libdoom.so` |
| `app/src/main/java/com/example/doom/DoomNative.kt` | `external fun` declarations + `System.loadLibrary` |
| `app/src/main/java/com/example/doom/MainActivity.kt` | SurfaceView, render thread, input mapping |
| `app/build.gradle.kts` | wires Gradle to the CMake build |

## Setup (in Android Studio)

1. Create a new **Empty Activity** project, language **Kotlin**, with **C++
   support** (or add the NDK + CMake to an existing one).
2. Copy these files into the matching paths under your module.
3. Drop the upstream DOOM engine sources into
   `app/src/main/cpp/engine/`, and make sure this package's `include/` and
   `src/dg_keyqueue.c` are reachable (the `PKG_DIR` path in `CMakeLists.txt`).
4. Ship `doom1.wad` with the app (e.g. in `assets/`) and copy it to
   `filesDir/doom1.wad` on first run — `MainActivity` points the engine there.
5. Build & run on a device/emulator.

> ⚠️ This is a faithful **skeleton**, not a turnkey project — it needs the
> Android SDK/NDK and the engine sources, so it cannot be built from this repo
> alone. The JNI symbol names in `native-doom.cpp` are tied to the
> `com.example.doom.DoomNative` package/class; keep them in sync if you rename.

## Notes

- The `package`/class in Kotlin **must** match the `Java_com_example_doom_..._native*`
  names in C++ — that is how JNI links them.
- Run `nativeTick()` on a **background thread**; never block the UI thread.
- `native-doom.cpp` repacks `0x00RRGGBB` into Android's RGBA_8888 byte order so
  colours are correct.
- Input reuses this package's tested `dg_keyqueue`; the Kotlin side maps Android
  `KeyEvent` codes to DOOM key codes (see `dg_keys.h`).
