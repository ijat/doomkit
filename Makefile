# =============================================================================
#  genericdoom-cleancode -- Makefile
# -----------------------------------------------------------------------------
#  Common targets:
#    make test       Build and run every Unity test suite (the default).
#    make coverage   Run the suites under LLVM source-based coverage and print a
#                    per-file report. Target is 100% for everything in src/.
#    make run-null   Build and run the dependency-free "null" example, which
#                    drives the helper modules end-to-end and writes frame.ppm.
#    make clean      Remove all build output.
#
#  Everything here needs only a C compiler and make -- no external libraries.
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

.PHONY: all test coverage run-null clean
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
NULL_SRCS = examples/null/platform_null.c \
            src/dg_keyqueue.c src/dg_keymap.c src/dg_palette.c src/dg_framebuffer.c

run-null: $(BUILD)/null_demo
	@echo "=================== running null demo ==================="
	@cd $(BUILD) && ./null_demo
	@echo "wrote $(BUILD)/frame.ppm"

$(BUILD)/null_demo: $(NULL_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(NULL_SRCS) -o $@

clean:
	rm -rf $(BUILD)
