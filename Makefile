# =============================================================================
#  doomkit -- Makefile
# -----------------------------------------------------------------------------
#  Common targets:
#    make test       Build and run every Unity test suite (the default).
#    make coverage   Run the suites under LLVM source-based coverage and print a
#                    per-file report. Target is 100% for everything in src/.
#    make run-null   Build and run the dependency-free "null" example, which
#                    drives the helper modules end-to-end and writes frame.ppm.
#    make run-null-engine
#                    Build and run the SAME headless port against the REAL engine
#                    (no display); ticks ~10s and writes frame_engine.ppm. Needs
#                    the engine sources and a WAD:
#                      make run-null-engine ENGINE=/path/to/doomgeneric/doomgeneric WAD=/path/doom1.wad
#    make lib        Build the shared libdoomgeneric the language bindings load.
#                    Needs the upstream engine sources:
#                      make lib ENGINE=/path/to/doomgeneric/doomgeneric
#    make wasm       Compile DOOM to WebAssembly for the browser example.
#                    Needs the Emscripten SDK (emcc) and the engine sources:
#                      make wasm ENGINE=/path/to/doomgeneric/doomgeneric
#    make clean      Remove all build output.
#
#  test/coverage/run-null need only a C compiler. `lib`, `wasm` and
#  `run-null-engine` additionally need the upstream DOOM engine sources (this
#  package does not vendor them); `run-null-engine` also needs a WAD.
# =============================================================================

CC      ?= cc
CSTD    ?= -std=c11
WARN     = -Wall -Wextra
INCLUDES = -Iinclude -Itests/vendor/unity
CFLAGS  ?= $(CSTD) $(WARN) $(INCLUDES)

BUILD    = build
UNITY    = tests/vendor/unity/unity.c

# Each suite is <name>: it links tests/test_<name>.c with src/dg_<name>.c.
SUITES   = keyqueue keymap palette framebuffer

# Convenience: the test binaries we will produce.
TEST_BINS = $(addprefix $(BUILD)/test_,$(SUITES))

# ---- shared-library build (make lib) ---------------------------------------
# Path to the upstream DOOM engine sources (the folder with d_main.c, i_video.c,
# ...). Override on the command line: make lib ENGINE=/path/to/doomgeneric/doomgeneric
ENGINE  ?= ../doomgeneric/doomgeneric
LIBDIR   = $(BUILD)/lib

# Pick the shared-object extension and link flag per OS.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LIBNAME = libdoomgeneric.dylib
  LIBFLAG = -dynamiclib
else
  LIBNAME = libdoomgeneric.so
  LIBFLAG = -shared
endif

# Flags for engine/shim objects: position-independent, hidden symbols (only the
# DG_API exports are visible), warnings off (1993 code), engine + shim headers.
LIB_CFLAGS = -fPIC -fvisibility=hidden -w -I$(ENGINE) -Ibindings

# The portable engine source set: the upstream Makefile's SRC_DOOM list MINUS
# doomgeneric_xlib (and all other doomgeneric_*.c platform files) -- our shim
# provides the DG_* symbols instead. A naive *.c glob would wrongly pull in
# SDL/Allegro/GUS-only files (i_sdlsound.c, gusconf.c, mus2mid.c, icon.c, ...),
# so we list the verified set explicitly.
DG_ENGINE_NAMES = dummy am_map doomdef doomstat dstrings d_event d_items d_iwad \
  d_loop d_main d_mode d_net f_finale f_wipe g_game hu_lib hu_stuff info i_cdmus \
  i_endoom i_joystick i_scale i_sound i_system i_timer memio m_argv m_bbox m_cheat \
  m_config m_controls m_fixed m_menu m_misc m_random p_ceilng p_doors p_enemy \
  p_floor p_inter p_lights p_map p_maputl p_mobj p_plats p_pspr p_saveg p_setup \
  p_sight p_spec p_switch p_telept p_tick p_user r_bsp r_data r_draw r_main \
  r_plane r_segs r_sky r_things sha1 sounds statdump st_lib st_stuff s_sound \
  tables v_video wi_stuff w_checksum w_file w_main w_wad z_zone w_file_stdc \
  i_input i_video doomgeneric

LIB_OBJS = $(addprefix $(LIBDIR)/,$(addsuffix .o,$(DG_ENGINE_NAMES))) $(LIBDIR)/capi.o

# ---- WebAssembly build (make wasm) -----------------------------------------
# Same engine set, compiled to WASM by Emscripten together with the browser
# platform port. emcc must be on PATH.
EMCC    ?= emcc
WASMDIR  = $(BUILD)/wasm
DG_ENGINE_SRCS = $(addprefix $(ENGINE)/,$(addsuffix .c,$(DG_ENGINE_NAMES)))

.PHONY: all test coverage run-null run-null-engine run-terminal lib wasm clean
all: test

# -----------------------------------------------------------------------------
#  make test  -- build each suite, then run it. Any failure stops the build.
# -----------------------------------------------------------------------------
test: $(TEST_BINS)
	@echo "=================== running test suites ==================="
	@set -e; for t in $(TEST_BINS); do echo ">>> $$t"; $$t; done
	@echo "=================== all suites passed  ==================="

# Pattern rule: build/test_<name> from tests/test_<name>.c + src/dg_<name>.c.
$(BUILD)/test_%: tests/test_%.c src/dg_%.c $(UNITY) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD):
	@mkdir -p $(BUILD)

# -----------------------------------------------------------------------------
#  make coverage  -- LLVM source-based coverage (clang/Apple toolchain).
#  Falls back with a clear message if llvm tools are unavailable.
# -----------------------------------------------------------------------------
COV_FLAGS = -fprofile-instr-generate -fcoverage-mapping
PROFDATA  = $(BUILD)/merged.profdata

coverage: | $(BUILD)
	@command -v xcrun >/dev/null 2>&1 || { echo "coverage needs the LLVM/Apple toolchain (xcrun)"; exit 1; }
	@echo "=================== building instrumented suites ==================="
	@set -e; for s in $(SUITES); do \
		$(CC) $(CFLAGS) $(COV_FLAGS) tests/test_$$s.c src/dg_$$s.c $(UNITY) -o $(BUILD)/cov_$$s; \
	done
	@echo "=================== running for coverage ==================="
	@set -e; rm -f $(BUILD)/*.profraw; for s in $(SUITES); do \
		LLVM_PROFILE_FILE=$(BUILD)/$$s.profraw $(BUILD)/cov_$$s >/dev/null; \
	done
	@xcrun llvm-profdata merge -sparse $(BUILD)/*.profraw -o $(PROFDATA)
	@echo "=================== coverage report (src/) ==================="
	@xcrun llvm-cov report \
		$(BUILD)/cov_keyqueue \
		-object $(BUILD)/cov_keymap \
		-object $(BUILD)/cov_palette \
		-object $(BUILD)/cov_framebuffer \
		-instr-profile=$(PROFDATA) \
		$(addprefix src/dg_,$(addsuffix .c,$(SUITES)))
	@echo ""
	@echo "Tip: 'make coverage-html' (if added) or inspect with: xcrun llvm-cov show ..."

# -----------------------------------------------------------------------------
#  make run-null  -- build the zero-dependency headless port and run it.
# -----------------------------------------------------------------------------
NULL_SRCS = examples/platforms/null/platform_null.c \
            src/dg_keyqueue.c src/dg_keymap.c src/dg_palette.c src/dg_framebuffer.c

run-null: $(BUILD)/null_demo
	@echo "=================== running null demo ==================="
	@cd $(BUILD) && ./null_demo
	@echo "wrote $(BUILD)/frame.ppm"

$(BUILD)/null_demo: $(NULL_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(NULL_SRCS) -o $@

# -----------------------------------------------------------------------------
#  make run-null-engine  -- the SAME headless port, but driving the REAL engine.
#
#  Like `run-null`, but instead of a fake engine it links the verified portable
#  engine set (DG_ENGINE_NAMES, from ENGINE=...) with platform_null_engine.c,
#  runs it for ~10 seconds with no display, and writes build/frame_engine.ppm.
#  Needs the upstream engine sources AND a WAD (game data):
#    make run-null-engine ENGINE=/path/to/doomgeneric/doomgeneric WAD=/path/doom1.wad
# -----------------------------------------------------------------------------
NULL_ENGINE_OBJDIR = $(BUILD)/null_engine_obj
NULL_ENGINE_OBJS = $(addprefix $(NULL_ENGINE_OBJDIR)/,$(addsuffix .o,$(DG_ENGINE_NAMES)))

run-null-engine: $(BUILD)/null_engine
	@test -n "$(WAD)" || { \
	  echo "ERROR: no WAD given. The real engine needs game data, e.g.:"; \
	  echo "  make run-null-engine ENGINE=$(ENGINE) WAD=/path/to/doom1.wad"; \
	  exit 1; }
	@test -f "$(WAD)" || { echo "ERROR: WAD not found: $(WAD)"; exit 1; }
	@echo "=================== running null engine demo ==================="
	@cd $(BUILD) && ./null_engine -iwad "$(abspath $(WAD))"
	@echo "wrote $(BUILD)/frame_engine.ppm"

$(BUILD)/null_engine: examples/platforms/null/platform_null_engine.c | $(BUILD)
	@test -f "$(ENGINE)/doomgeneric.h" || { \
	  echo "ERROR: DOOM engine sources not found at ENGINE=$(ENGINE)"; \
	  echo "  This package does not vendor the engine. Point ENGINE at upstream"; \
	  echo "  doomgeneric's 'doomgeneric/' folder (the one with d_main.c), e.g.:"; \
	  echo "    make run-null-engine ENGINE=/path/to/doomgeneric/doomgeneric WAD=..."; \
	  exit 1; }
	@echo "=================== building null_engine from $(ENGINE) ==================="
	@rm -rf $(NULL_ENGINE_OBJDIR) && mkdir -p $(NULL_ENGINE_OBJDIR)
	@for n in $(DG_ENGINE_NAMES); do \
	  $(CC) -w -I$(ENGINE) -Iinclude -c "$(ENGINE)/$$n.c" -o "$(NULL_ENGINE_OBJDIR)/$$n.o" || exit 1; \
	done
	$(CC) $(CSTD) $(WARN) -Iinclude -I$(ENGINE) \
	  examples/platforms/null/platform_null_engine.c $(NULL_ENGINE_OBJS) -lm -o $@

# -----------------------------------------------------------------------------
#  make run-terminal  -- the SAME real engine, drawn as colored ASCII in the TTY.
#
#  Like `run-null-engine`, but platform_terminal.c renders every frame to your
#  terminal and reads the keyboard live, so you can actually play. Interactive,
#  so it is NOT time-boxed and its output is NOT redirected. Same requirements:
#  the upstream engine sources AND a WAD.
#    make run-terminal ENGINE=/path/to/doomgeneric/doomgeneric WAD=/path/doom1.wad
#  Use a truecolor terminal ~80x50 or larger; quit with Ctrl-C or DOOM's menu.
# -----------------------------------------------------------------------------
TERMINAL_OBJDIR = $(BUILD)/terminal_engine_obj
TERMINAL_OBJS   = $(addprefix $(TERMINAL_OBJDIR)/,$(addsuffix .o,$(DG_ENGINE_NAMES)))

run-terminal: $(BUILD)/terminal_doom
	@test -n "$(WAD)" || { \
	  echo "ERROR: no WAD given. The real engine needs game data, e.g.:"; \
	  echo "  make run-terminal ENGINE=$(ENGINE) WAD=/path/to/doom1.wad"; \
	  exit 1; }
	@test -f "$(WAD)" || { echo "ERROR: WAD not found: $(WAD)"; exit 1; }
	@cd $(BUILD) && ./terminal_doom -iwad "$(abspath $(WAD))"

$(BUILD)/terminal_doom: examples/platforms/terminal/platform_terminal.c | $(BUILD)
	@test -f "$(ENGINE)/doomgeneric.h" || { \
	  echo "ERROR: DOOM engine sources not found at ENGINE=$(ENGINE)"; \
	  echo "  This package does not vendor the engine. Point ENGINE at upstream"; \
	  echo "  doomgeneric's 'doomgeneric/' folder (the one with d_main.c), e.g.:"; \
	  echo "    make run-terminal ENGINE=/path/to/doomgeneric/doomgeneric WAD=..."; \
	  exit 1; }
	@echo "=================== building terminal_doom from $(ENGINE) ==================="
	@rm -rf $(TERMINAL_OBJDIR) && mkdir -p $(TERMINAL_OBJDIR)
	@for n in $(DG_ENGINE_NAMES); do \
	  $(CC) -w -I$(ENGINE) -Iinclude -c "$(ENGINE)/$$n.c" -o "$(TERMINAL_OBJDIR)/$$n.o" || exit 1; \
	done
	$(CC) $(CSTD) $(WARN) -Iinclude -I$(ENGINE) \
	  examples/platforms/terminal/platform_terminal.c $(TERMINAL_OBJS) -lm -o $@

# -----------------------------------------------------------------------------
#  make lib  -- build libdoomgeneric (engine + C-ABI shim) for the bindings.
#
#  Compiles the verified portable engine set + bindings/doomgeneric_capi.c into
#  a shared library the language examples load. The shim provides the DG_*
#  symbols, so no platform file (doomgeneric_*.c) is included.
# -----------------------------------------------------------------------------
$(LIBDIR):
	@mkdir -p $(LIBDIR)

lib: | $(LIBDIR)
	@test -f "$(ENGINE)/doomgeneric.h" || { \
	  echo "ERROR: DOOM engine sources not found at ENGINE=$(ENGINE)"; \
	  echo "  This package does not vendor the engine. Point ENGINE at upstream"; \
	  echo "  doomgeneric's 'doomgeneric/' folder (the one with d_main.c), e.g.:"; \
	  echo "    make lib ENGINE=/path/to/doomgeneric/doomgeneric"; \
	  exit 1; }
	@echo "=================== building $(LIBNAME) from $(ENGINE) ==================="
	@rm -f $(LIBDIR)/*.o
	@for n in $(DG_ENGINE_NAMES); do \
	  $(CC) $(LIB_CFLAGS) -c "$(ENGINE)/$$n.c" -o "$(LIBDIR)/$$n.o" || exit 1; \
	done
	@$(CC) $(LIB_CFLAGS) -c bindings/doomgeneric_capi.c -o "$(LIBDIR)/capi.o"
	@$(CC) $(LIBFLAG) -o "$(LIBDIR)/$(LIBNAME)" $(LIB_OBJS)
	@echo "built $(LIBDIR)/$(LIBNAME)"
	@echo "next: copy it where an example expects it, e.g."
	@echo "      cp $(LIBDIR)/$(LIBNAME) examples/languages/rust/lib/    (rust/go)"
	@echo "  or set DYLD_LIBRARY_PATH/LD_LIBRARY_PATH=$(LIBDIR)          (python/c#/java)"

# -----------------------------------------------------------------------------
#  make wasm  -- compile DOOM to WebAssembly for the browser example.
#
#  Compiles the portable engine set + examples/platforms/wasm/platform_wasm.c + the
#  key-queue helper into one .wasm with Emscripten. Output: build/wasm/.
#  Needs emcc (the Emscripten SDK) on PATH and the upstream engine sources.
# -----------------------------------------------------------------------------
$(WASMDIR):
	@mkdir -p $(WASMDIR)

wasm: | $(WASMDIR)
	@command -v $(EMCC) >/dev/null 2>&1 || { \
	  echo "ERROR: '$(EMCC)' not found. Install the Emscripten SDK and run:"; \
	  echo "    source /path/to/emsdk/emsdk_env.sh"; \
	  exit 1; }
	@test -f "$(ENGINE)/doomgeneric.h" || { \
	  echo "ERROR: DOOM engine sources not found at ENGINE=$(ENGINE)"; \
	  echo "    make wasm ENGINE=/path/to/doomgeneric/doomgeneric"; \
	  exit 1; }
	@echo "=================== building WebAssembly from $(ENGINE) ==================="
	$(EMCC) -O2 -w -I$(ENGINE) -Iinclude \
	  examples/platforms/wasm/platform_wasm.c src/dg_keyqueue.c $(DG_ENGINE_SRCS) \
	  -sINVOKE_RUN=0 -sALLOW_MEMORY_GROWTH=1 \
	  -sEXPORTED_FUNCTIONS=_main,_wasm_push_key \
	  -sEXPORTED_RUNTIME_METHODS=callMain,FS,HEAPU32 \
	  -o $(WASMDIR)/doom.js
	@cp examples/platforms/wasm/index.html $(WASMDIR)/
	@echo "built $(WASMDIR)/{doom.js,doom.wasm,index.html}"
	@echo "serve it:  (cd $(WASMDIR) && python3 -m http.server 8000)  then open http://localhost:8000/"

clean:
	rm -rf $(BUILD)
