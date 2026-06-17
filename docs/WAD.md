# Getting the game data — the WAD file

Every part of doomkit that runs *real* DOOM needs two separate things: the
**engine** (the code, from upstream doomgeneric) and a **WAD** (the data — maps,
textures, sprites, sounds, music). The engine is useless without a WAD, and this
package ships neither. This page is the beginner's guide to the WAD half: what it
is, where to get one **legally**, and how to point the engine at it.

> The headless `make run-null` demo needs **no WAD** — it uses a fake engine and
> a generated test image. You only need a WAD once you link the real engine — for
> example the headless `make run-null-engine WAD=...` demo, or any real port
> (see [PORTING.md](PORTING.md) and [Running real DOOM](../README.md#running-real-doom)).

---

## What is a WAD?

**WAD** = "Where's All the Data". It's a single archive file (`.wad`) holding
every asset the game uses, stored as named chunks called **lumps**. There are
two kinds:

| Kind | Means | Examples | You need |
|------|-------|----------|----------|
| **IWAD** | *Internal* WAD — a complete, standalone game | `doom1.wad`, `DOOM.WAD`, `freedoom1.wad` | **exactly one** to play |
| **PWAD** | *Patch* WAD — an add-on layered on top of an IWAD (custom maps/mods) | community level packs | optional, never on its own |

For a first port you only care about getting **one IWAD**. You pass it to the
engine with `-iwad` (see [below](#pointing-the-engine-at-your-wad)).

---

## Where to get a WAD (legally)

You have three legitimate options. Pick the first that fits.

### 1. Freedoom — free, legal to redistribute (recommended for ports & demos)

[Freedoom](https://freedoom.github.io/) is a completely free, open-content IWAD
that runs on the DOOM engine. It is **not** id Software's data — it's an original
replacement — so you may ship it with your port, put it in CI, and hand it to
anyone. For doomkit's audience (new platforms, demos, automated builds) this is
usually the right choice.

```sh
# Download the latest release (file names may differ by version):
#   freedoom1.wad  (Phase 1 — Doom-1-style, 1 episode usable in shareware-equivalent scope)
#   freedoom2.wad  (Phase 2 — Doom-2-style)
# from https://github.com/freedoom/freedoom/releases
./doom -iwad freedoom1.wad
```

### 2. The official DOOM shareware WAD — free to download, *not* to redistribute

The classic `doom1.wad` (≈4 MB, episode 1 only) was released as shareware and is
free to **download and use**, but its license does **not** let you redistribute
it (so don't commit it to a public repo). It's the file most DOOM-port tutorials
assume. It ships inside the original shareware installer; many DOOM source-port
projects also mirror it. A commonly-used direct download (mirrored for exactly
this purpose, and referenced by many "DOOM on X" guides):

```sh
curl -L -o doom1.wad https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad
./doom -iwad doom1.wad
```

Mirrors come and go — if that link is dead, search for "doom1.wad shareware" or
grab it from any DOOM source port's downloads. **Whatever you download, verify it
with the checksum [below](#verify-what-you-downloaded)** before trusting it; the
shareware `doom1.wad` is MD5 `f0cefca49926d00903cf57551d901abe`.

### 3. A retail WAD you already own

If you own DOOM, DOOM II, Final DOOM, or a store re-release (Steam, GOG, the
2019/2020 re-releases), the IWAD (`DOOM.WAD`, `DOOM2.WAD`, etc.) is in the game's
install folder. You may use your own copy; you may not give it to others.

> **Rule of thumb:** want to *share* your port with the WAD included → Freedoom.
> Just testing on your own machine → any of the three.

---

## Pointing the engine at your WAD

The engine reads a command-line flag:

```sh
./doom -iwad /path/to/freedoom1.wad
```

- The path can be absolute or relative to where you run the program.
- If you omit `-iwad`, the engine searches its built-in default locations and a
  few environment variables (e.g. `DOOMWADDIR`) and will **exit with an error**
  if it finds nothing — that error means "no WAD", not "broken port".
- Drag-and-drop / file-picker platforms (the **browser/wasm** example) load the
  WAD's bytes a different way — you select the file in the page and it's written
  into the in-memory filesystem before the engine boots. See
  [`examples/platforms/wasm/`](../examples/platforms/wasm/).

---

## Verify what you downloaded

WADs from the open internet vary (different versions, repacks, corruption). If
something looks wrong, check the file's MD5 against a known value:

```sh
md5sum freedoom1.wad     # Linux
md5 freedoom1.wad        # macOS
```

Then compare against the checksum on the project's release page (Freedoom
publishes them; retail/shareware MD5s are catalogued by the source-port
community). A mismatch explains "the engine refuses to start" or "weird
textures" before you go hunting in your own code.

---

## Common WAD problems

| Symptom | Likely cause |
|---------|--------------|
| `Error: IWAD not found` / engine exits at startup | No `-iwad` given and none in the default paths. Pass `-iwad /path/to/file.wad`. |
| `PWAD ... is not an IWAD` or similar | You passed a patch/add-on WAD as the main game. Use an **IWAD** (table above). |
| Wrong/garbled maps or missing levels | Version mismatch (e.g. a Doom-1 engine path with a Doom-2 WAD), or a corrupt download — [verify the checksum](#verify-what-you-downloaded). |
| Works on your machine, fails for others | You used a **non-redistributable** WAD. Ship **Freedoom** instead. |

---

See also: [PORTING.md](PORTING.md) for the full build-and-run walkthrough and
[GLOSSARY.md](GLOSSARY.md) for any unfamiliar terms.
