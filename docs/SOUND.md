# Sound — why it's not in the porting seam, and how to get it anyway

**Short version:** doomkit does not support sound, and that is deliberate, not an
oversight. The six `DG_*` callbacks doomkit documents are video, input, and
timing only. DOOM's audio lives *inside* the engine, behind a different seam, and
on the kinds of constrained platforms doomkit targets it is usually unwanted (or
physically impossible) anyway. This document explains the architecture so the
absence makes sense, and points you at the engine's own backend if your platform
*can* have sound.

---

## Why sound isn't one of the six functions

The doomkit contract is six callbacks: `DG_Init`, `DG_DrawFrame`, `DG_SleepMs`,
`DG_GetTicksMs`, `DG_GetKey`, `DG_SetWindowTitle`. None of them touch audio —
because DOOM's sound was never part of the platform porting layer in the first
place. It is handled *within* the engine by `i_sound.c`, which dispatches through
its own internal interface (`sound_module_t` / `music_module_t` in
`i_sound.h`) to backends like SDL or Allegro.

So sound and video sit behind two different seams:

```
   VIDEO / INPUT / TIMING                 SOUND
   ──────────────────────                 ─────
   engine ──DG_DrawFrame──▶ platform      engine ──sound_module_t──▶ SDL / Allegro
   (the doomkit contract)                 (internal to the engine, not doomkit)
```

doomkit is the documentation and clean extraction of the *left* seam. The right
seam is engine-internal upstream code; doomkit neither owns it nor exposes it.

## Why the two seams are not symmetric

This is the part worth internalising, because it explains why "just add a sound
callback like `DG_DrawFrame`" is not actually simple:

| | What the engine hands the platform | What the platform must do |
|---|---|---|
| **Video** | A **finished** 8-bit paletted frame buffer | Convert + blit it. That's all. |
| **Sound** | *Events* ("play lump #37 on channel 3, volume V, stereo separation S") and for music a **raw blob of MUS bytes** | **Mix** several PCM channels into one stream (resample 11 kHz mono, apply volume + L/R pan) **and synthesize** the MUS/MIDI music into audio |

The engine renders video *for* you; it only *orchestrates* sound. Producing the
actual samples — a software mixer plus a MIDI/OPL synth — is real DSP work, and
it is exactly the work the SDL and Allegro backends exist to do. That is why
audio needs a backend at all, while video needs only a memcpy.

## Why this is the right call for doomkit's audience

doomkit exists to make porting DOOM to a *new* or *constrained* platform legible —
the "I ran DOOM on my toaster / oscilloscope / microcontroller" class of port.
For those targets:

- The demo is the **picture**. Sound is rarely the point.
- The hardware often has **no audio output**, or no spare cycles to mix PCM and
  emulate an OPL2 chip in real time — which is the heaviest part of DOOM audio.
- Adding a generic mixer + synth would be the most code, for the least-wanted
  feature, on the platforms least able to run it — while diluting the one thing
  doomkit is for: a small, fully-testable, six-function seam.

So the absence is a feature of the scope, not a gap to be filled.

---

## How to get sound anyway (when your platform can)

If your platform has **SDL** or another OS-level audio API, you do **not** need
doomkit to add anything — the upstream engine already ships a complete backend.
You enable it at the engine layer, not the doomkit layer:

1. **Compile the engine's audio files** that doomkit's library build deliberately
   omits: `i_sdlsound.c`, `i_sdlmusic.c`, `mus2mid.c` (and their support files
   such as `gusconf.c`). doomkit's `Makefile` lists the portable engine set
   explicitly in `DG_ENGINE_NAMES` and leaves these out on purpose — add them
   back for an SDL build.
2. **Define `FEATURE_SOUND`** when compiling the engine. Without it, `i_sound.c`
   wires up no modules and every audio call is a no-op (this is doomkit's current
   default).
3. **Link `SDL2` and `SDL2_mixer`.**
4. **Initialise the audio subsystem** on your side — e.g. `SDL_InitSubSystem(
   SDL_INIT_AUDIO)` in your `DG_Init`.

That is the whole job for an SDL desktop or wasm port: it is build-and-link
plumbing, not new logic, because the mixing and music synthesis already exist in
the engine. The DOOM sound code is pull-based — once the backend is initialised
the engine drives it directly from the WAD; no extra `DG_*` callback is involved.

## If you have neither SDL nor an OS audio API

Then you are in the genuinely hard case, and it is out of scope for doomkit. To
produce sound you would have to supply, yourself, the two pieces the SDL backend
otherwise provides:

- a **software mixer** that combines DOOM's PCM channels (with volume and stereo
  separation) into one output buffer, and
- a **music synth** — `mus2mid.c` converts MUS → MIDI portably, but turning MIDI
  into audio needs an OPL2 emulator, a SoundFont/Timidity synth, or a hardware
  MIDI device.

Both have clean, dependency-free reference implementations in
[Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom) (`i_sound.c`
mixing and the `opl/` library). If you ever attempt this, the lowest-cost path is
**SFX only, no music** — the iconic sounds for a fraction of the effort, since the
OPL/MIDI synth is the bulk of the work — exposed through a single "here are
finished samples, play them" hook rather than a full audio contract. But for
doomkit's purpose, documenting the boundary (this file) is the deliverable;
building that subsystem is not.
