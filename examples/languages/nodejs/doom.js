// =============================================================================
//  examples/languages/nodejs/doom.js
// -----------------------------------------------------------------------------
//  Drive the doomkit shared library from Node.js using koffi, the modern Node
//  FFI (think "ctypes for Node"). No native compilation needed. GPLv2. See LICENSE.
// =============================================================================
//
//  The one rule (same as every managed language): keep the registered callbacks
//  reachable so the GC can't collect the function pointers the engine holds. We
//  keep them in the module-level `callbacks` object for the program's lifetime.
//
//  RUN (after `npm install` and building libdoomgeneric -- see bindings/README.md):
//      # put the library where this script looks (./lib) or set DOOMKIT_LIB:
//      cp /path/to/libdoomgeneric.* lib/
//      node doom.js -iwad /path/to/doom1.wad        # writes frame.ppm at frame 100
// =============================================================================

'use strict';

const koffi = require('koffi');
const fs = require('fs');
const path = require('path');

// ---- locate and load the shared library -----------------------------------
const candidates = [
  process.env.DOOMKIT_LIB,
  path.join(__dirname, 'lib', 'libdoomgeneric.dylib'),
  path.join(__dirname, 'lib', 'libdoomgeneric.so'),
  'libdoomgeneric.dylib',
  'libdoomgeneric.so',
].filter(Boolean);

let lib;
for (const c of candidates) {
  try { lib = koffi.load(c); break; } catch (_) { /* try next */ }
}
if (!lib) {
  console.error('could not load libdoomgeneric; build it (make lib) and put it in ./lib or set DOOMKIT_LIB');
  process.exit(1);
}

// ---- describe the C types --------------------------------------------------
// The callback struct: six function pointers (opaque void* to koffi).
const DgCallbacks = koffi.struct('dg_callbacks', {
  init:             'void *',
  draw_frame:       'void *',
  sleep_ms:         'void *',
  get_ticks_ms:     'void *',
  get_key:          'void *',
  set_window_title: 'void *',
});

// Callback prototypes (the signatures C will call back with).
const InitProto   = koffi.proto('void   InitProto()');
const DrawProto   = koffi.proto('void   DrawProto()');
const SleepProto  = koffi.proto('void   SleepProto(uint32_t ms)');
const TicksProto  = koffi.proto('uint32_t TicksProto()');
const GetKeyProto = koffi.proto('int    GetKeyProto(int *pressed, unsigned char *key)');
const TitleProto  = koffi.proto('void   TitleProto(const char *title)');

// The exported library functions.
const dg_set_callbacks = lib.func('void dg_set_callbacks(dg_callbacks *cb)');
const dg_create        = lib.func('void dg_create(int argc, const char **argv)');
const dg_tick          = lib.func('void dg_tick()');
const dg_screen_buffer = lib.func('uint32_t *dg_screen_buffer()');
const dg_resx          = lib.func('int dg_resx()');
const dg_resy          = lib.func('int dg_resy()');

// ---- the six callbacks -----------------------------------------------------
const start = Date.now();
let frame = 0;

function onInit() {
  console.log(`[node] init: framebuffer ${dg_resx()}x${dg_resy()}`);
}

function onDrawFrame() {
  if (++frame !== 100) return;
  const w = dg_resx(), h = dg_resy();
  const ptr = dg_screen_buffer();
  // Copy the framebuffer out of native memory as an array of 0x00RRGGBB words.
  const pixels = koffi.decode(ptr, koffi.array('uint32_t', w * h));

  const rgb = Buffer.allocUnsafe(w * h * 3);
  for (let i = 0; i < w * h; i++) {
    const p = pixels[i];
    rgb[i * 3]     = (p >>> 16) & 0xff;
    rgb[i * 3 + 1] = (p >>> 8) & 0xff;
    rgb[i * 3 + 2] = p & 0xff;
  }
  const out = fs.createWriteStream('frame.ppm');
  out.write(`P6\n${w} ${h}\n255\n`);
  out.end(rgb);
  console.log(`[node] wrote frame.ppm at frame ${frame}`);
}

function onSleepMs(ms) {
  // A tiny busy-wait keeps this single-file demo synchronous. A real app would
  // run the tick loop on a timer instead of blocking.
  const until = Date.now() + ms;
  while (Date.now() < until) { /* spin */ }
}

function onGetTicksMs() { return (Date.now() - start) & 0xffffffff; }
function onGetKey(_pressed, _key) { return 0; }            // no input
function onSetTitle(title) { console.log(`[node] title: ${title}`); }

// Register the JS functions as C callbacks. koffi.register returns a pointer;
// we stash them so they stay alive for the whole run.
const callbacks = {
  init:             koffi.register(onInit,      koffi.pointer(InitProto)),
  draw_frame:       koffi.register(onDrawFrame, koffi.pointer(DrawProto)),
  sleep_ms:         koffi.register(onSleepMs,   koffi.pointer(SleepProto)),
  get_ticks_ms:     koffi.register(onGetTicksMs,koffi.pointer(TicksProto)),
  get_key:          koffi.register(onGetKey,    koffi.pointer(GetKeyProto)),
  set_window_title: koffi.register(onSetTitle,  koffi.pointer(TitleProto)),
};

// ---- run -------------------------------------------------------------------
dg_set_callbacks(callbacks);

const argv = ['doomdemo', ...process.argv.slice(2)]; // prepend a fake argv[0]
dg_create(argv.length, argv);

for (let i = 0; i < 200; i++) dg_tick();
console.log('[node] done.');
