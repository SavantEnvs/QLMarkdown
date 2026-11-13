#!/usr/bin/env bash
#
# mayhem/build.sh — builds the fuzz target (fuzz_markdown + standalone reproducer) and the test oracle
# (qlmd-render) for QLMarkdown's cmark-gfm pipeline: cmark-gfm itself, the cmark-extra extensions, and a
# small C driver (mayhem/qlmd_render.c). The macOS app (Swift/AppKit) is not built.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

SRC=${SRC:-/mayhem}
cd "$SRC"

DEPS=/opt/toolchains/deps
CMARK_SRC=$DEPS/cmark-gfm-27d942c/src
JPCRE2_SRC=$DEPS/jpcre2-867af2d/src
B=$SRC/build            # gitignored (mayhem/.gitignore does not cover it; upstream's `build/**` does)
mkdir -p "$B"

# Sanitized fuzz build: ASan+UBSan (halting), SanCov on the library, DWARF 3.
FUZZ_FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS"
# Oracle build: the project's normal flags.
TEST_FLAGS="-O2 $COVERAGE_FLAGS"

# 1) cmark-gfm (static), once per flavour.
build_cmark() { # <dir> <flags>
  cmake -S "$CMARK_SRC" -B "$1" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" \
        -DCMAKE_C_FLAGS="$2" -DCMAKE_CXX_FLAGS="$2" \
        -DCMARK_TESTS=OFF -DCMARK_SHARED=OFF -DCMARK_STATIC=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 >/dev/null
  cmake --build "$1" -j"$MAYHEM_JOBS" --target libcmark-gfm_static libcmark-gfm-extensions_static >/dev/null
}
build_cmark "$B/cmark-fuzz" "$FUZZ_FLAGS -O1"
build_cmark "$B/cmark-test" "$TEST_FLAGS"

EXTRA=$SRC/cmark-extra
C_SRCS=(
  extensions/admonition.c extensions/alert.c extensions/defl.c extensions/emoji.c extensions/hashtag.c
  extensions/heads.c extensions/highlight.c extensions/math_ext.c extensions/mention.c
  extensions/sub_ext.c extensions/sup_ext.c extensions/wikilink.c c_log.c
)
CXX_SRCS=(extensions/emoji_utils.cpp extensions/heads_utils.cpp)

# The extensions include "../../cmark-gfm/src/parser.h" (upstream's cmark-gfm submodule, empty here): an
# include path <inc>/x/y with <inc>/cmark-gfm -> the pinned source makes that resolve without copying anything.
mkdir -p "$B/inc/x/y"; ln -sfn "$CMARK_SRC" "$B/inc/cmark-gfm"

# 2) Extension + driver objects, once per flavour: <obj dir> <cmark build dir> <flags>.
compile_all() {
  local out=$1 cb=$2 flags=$3
  local inc="-I$SRC/mayhem -I$SRC/mayhem/shim -I$EXTRA -I$EXTRA/extensions -I$B/inc/x/y -I$CMARK_SRC/src -I$CMARK_SRC/extensions -I$cb/src -I$cb/extensions -I$JPCRE2_SRC/src"
  rm -rf "$out" "$out-main"; mkdir -p "$out" "$out-main"
  {
    for s in "${C_SRCS[@]}"; do echo "$CC $flags $inc -c $EXTRA/$s -o $out/$(basename "$s").o"; done
    for s in "${CXX_SRCS[@]}"; do echo "$CXX -std=c++17 -include algorithm $flags $inc -c $EXTRA/$s -o $out/$(basename "$s").o"; done
    echo "$CC $flags $inc -c $SRC/mayhem/qlmd_render.c -o $out/qlmd_render.o"
    echo "$CC $flags $inc -c $SRC/mayhem/qlmd_render_cli.c -o $out-main/qlmd_render_cli.o"
    echo "$CC $flags $inc -c $SRC/mayhem/fuzz_markdown.c -o $out-main/fuzz_markdown.o"
  } | xargs -P "$MAYHEM_JOBS" -I{} bash -c '{}'
}
compile_all "$B/obj-fuzz" "$B/cmark-fuzz" "$FUZZ_FLAGS -O1"
compile_all "$B/obj-test" "$B/cmark-test" "$TEST_FLAGS"

LIBS_FUZZ=("$B/cmark-fuzz/extensions/libcmark-gfm-extensions.a" "$B/cmark-fuzz/src/libcmark-gfm.a")
LIBS_TEST=("$B/cmark-test/extensions/libcmark-gfm-extensions.a" "$B/cmark-test/src/libcmark-gfm.a")
PCRE=(-l:libpcre2-32.a)

# 3) Fuzz target + standalone reproducer. LeakSanitizer is off at build time via lsan_off.cc.
OBJ_F=("$B"/obj-fuzz/*.o); OBJ_T=("$B"/obj-test/*.o)
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$B/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"
$CXX $FUZZ_FLAGS $LIB_FUZZING_ENGINE "$B/obj-fuzz-main/fuzz_markdown.o" "${OBJ_F[@]}" "$B/lsan_off.o" \
     "${LIBS_FUZZ[@]}" "${PCRE[@]}" -o "$SRC/build/fuzz_markdown"
$CXX $FUZZ_FLAGS "$B/obj-fuzz-main/fuzz_markdown.o" "$B/standalone_main.o" "${OBJ_F[@]}" "$B/lsan_off.o" \
     "${LIBS_FUZZ[@]}" "${PCRE[@]}" -o "$SRC/build/fuzz_markdown-standalone"

# 4) Test oracle driver, normal flags, no sanitizers.
$CXX $TEST_FLAGS "$B/obj-test-main/qlmd_render_cli.o" "${OBJ_T[@]}" "${LIBS_TEST[@]}" "${PCRE[@]}" -o "$SRC/build/qlmd-render"
