# Examples

There are two kinds of example, split by *how they connect to the engine*:

| Folder | Mechanism | Use when… |
|--------|-----------|-----------|
| [`platforms/`](platforms/) | implement the six `DG_*` callbacks, compiled **with** the engine | you're putting DOOM on a new screen/device (desktop, browser, phone, headless) |
| [`languages/`](languages/) | drive a **prebuilt** `libdoomgeneric` over an FFI (`make lib` first) | you want to run DOOM from another language (Go, C#, Java, Python, Rust, Node, …) |

Plus [`minimal_main.c`](minimal_main.c) — the canonical `doomgeneric_Create()` +
`doomgeneric_Tick()` loop on its own.

New here? Start with [`platforms/null/platform_null.c`](platforms/null/platform_null.c):
it's heavily commented and actually runs (`make run-null`). For the same callbacks
driving the **real engine** headless, see
[`platforms/null/platform_null_engine.c`](platforms/null/platform_null_engine.c)
(`make run-null-engine ENGINE=... WAD=...`). Then copy
[`platforms/template/platform_template.c`](platforms/template/platform_template.c)
and fill in the six TODOs for your target.

See the root [README](../README.md#pick-your-platform) for the full
"pick your platform" matrix.
