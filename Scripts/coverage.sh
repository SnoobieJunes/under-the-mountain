#!/bin/sh
# [2026-10-27 T-022] Real llvm-cov line coverage for Source/UnderTheMountain,
# via module-only rsp-relink. Replaces the T-002 test-presence baseline
# (kept below as a fail-fast check).
#
# Why not instrument through UBT: on Mac UE 5.8.4 there is no module-only
# flag channel. ModuleRules.bCodeCoverage is never turned into flags by
# MacToolChain/AppleToolChain/ClangToolChain (only LinuxToolChain.cs:322
# and the Windows-clang path in VCToolChain.cs:885 emit the coverage
# flags), UEBuildModuleCPP overwrites the module value with
# Target.bCodeCoverage, and AdditionalCompilerArguments is target-level
# only (TargetRules.cs:2914,2921). The previous version of this script
# used that target-level channel (-CompilerArguments=/-LinkerArguments=),
# which put the ENTIRE editor target in a unique build environment:
# measured 2750-action full rebuild, attempt killed at 95/2750. Whole-
# target instrumentation was measured impractical on this machine and is
# not coming back.
#
# Shipped path - module-only rsp-relink: run Scripts/build.sh first (this
# script never builds), then replay UBT's own response files for this
# module only, from Intermediate/Build/Mac/arm64/UnrealEditor/Development/
# UnderTheMountain/: each *.cpp.o.rsp is recompiled with
# -fprofile-instr-generate -fcoverage-mapping into Saved/Coverage/obj/,
# and libUnrealEditor-UnderTheMountain.dylib.rsp relinks those objects
# (plus -fprofile-instr-generate) into the real dylib in Binaries/Mac.
# Cost: 8 module compiles + 1 link + one headless editor test run. Still
# not run by gate.sh (it checks this script's artifact for freshness
# instead; see gate.sh [2026-10-27 T-022]).
#
# Replay details that are load-bearing:
# - clang parses the .rsp itself via @file. The shell never word-splits
#   an rsp: paths here contain spaces ("Development Projects") and are
#   double-quoted inside the rsp, which only clang's rsp parser handles.
#   Our own parsing is strict line-shape validation only (below); any
#   deviation from the expected shape is a loud failure, never a guess.
# - clang runs from $UE_ROOT/Engine/Source: UBT's compile and link
#   actions use EngineSourceDirectory as WorkingDirectory
#   (AppleToolChain.cs:783, MacToolChain.cs:537), and the rsp relative
#   paths (-I"Runtime/...", -include-pch "../Intermediate/...",
#   "../Binaries/Mac/libUnrealEditor-*.dylib") only resolve there.
# - The original dylib is backed up under Saved/Coverage/ and restored
#   by an EXIT trap on success and failure alike. The llvm-cov steps
#   below run before exit, while the instrumented dylib (which carries
#   the coverage map) is still in place.
# - An rsp left behind by the killed whole-target attempt may already
#   end with the coverage flags; appending them again is harmless, and a
#   normal Scripts/build.sh regenerates clean rsps anyway.
# - Denominator is Source/UnderTheMountain product lines only (Tests/
#   excluded since 2026-10-28 - see the report comment below). The bCodeCoverage
#   block in UnderTheMountain.Build.cs stays in place, harmless and
#   opt-in; this script does not use it or UTM_COVERAGE.
# - Only SIGKILL can skip the restore trap. The leftover-backup guard
#   below restores the original on the next run, or run Scripts/build.sh
#   to restore normal binaries outright.
#
# Outputs in Saved/Coverage/: raw profiles, merged.profdata, report.txt,
# summary.json and line-coverage.txt (bare X.X, read by gate.sh). Any
# missing profile/report is a loud non-zero failure, never an invented
# number.
set -eu
. "$(dirname "$0")/ue-env.sh"
cd "$PROJECT_ROOT"

# [2026-10-27 T-002, kept by T-022] Fail fast and cheap if the tests vanish;
# instrumenting a build with nothing to run would still "measure" 0 honestly
# but only after a full compile.
COUNT="$(grep -rl 'IMPLEMENT_.*AUTOMATION_TEST' Source/UnderTheMountain/Tests 2>/dev/null | wc -l | tr -d ' ')"
echo "automation test files: $COUNT"
[ "$COUNT" -ge 1 ] || { echo "FAIL: no automation tests found"; exit 1; }

COV_DIR="$PROJECT_ROOT/Saved/Coverage"
OBJ_DIR="$COV_DIR/obj"
PROFDATA="$COV_DIR/merged.profdata"
RSP_DIR="$PROJECT_ROOT/Intermediate/Build/Mac/arm64/UnrealEditor/Development/UnderTheMountain"
LINK_RSP="$RSP_DIR/libUnrealEditor-UnderTheMountain.dylib.rsp"
ENGINE_SRC="$UE_ROOT/Engine/Source"
mkdir -p "$COV_DIR"

# [2026-10-27 T-022] (1) Prerequisites from a normal build. This script
# does not build: no rsps or no module dylib means build first.
[ -d "$RSP_DIR" ] && [ -s "$LINK_RSP" ] \
  || { echo "FAIL: UBT response files missing in $RSP_DIR - run Scripts/build.sh first"; exit 1; }
set -- "$RSP_DIR"/*.cpp.o.rsp
[ -e "$1" ] || { echo "FAIL: no *.cpp.o.rsp files in $RSP_DIR - run Scripts/build.sh first"; exit 1; }
N_RSP=$#
[ -d "$ENGINE_SRC" ] || { echo "FAIL: engine source directory missing at $ENGINE_SRC (UE_ROOT=$UE_ROOT)"; exit 1; }
CLANGXX="$(xcrun --find clang++ 2>/dev/null || true)"
[ -n "$CLANGXX" ] && [ -x "$CLANGXX" ] || { echo "FAIL: clang++ not found via xcrun"; exit 1; }
[ -x "$UE_EDITOR_CMD" ] || { echo "FAIL: editor binary missing at $UE_EDITOR_CMD - run Scripts/build.sh first"; exit 1; }
DYLIB_LIST="$(find "$PROJECT_ROOT/Binaries" -name 'libUnrealEditor-UnderTheMountain.dylib' 2>/dev/null | sort)"
[ -n "$DYLIB_LIST" ] \
  || { echo "FAIL: libUnrealEditor-UnderTheMountain.dylib not found under Binaries - run Scripts/build.sh first"; exit 1; }
[ "$(printf '%s\n' "$DYLIB_LIST" | wc -l | tr -d ' ')" = 1 ] \
  || { echo "FAIL: multiple libUnrealEditor-UnderTheMountain.dylib under Binaries: $DYLIB_LIST"; exit 1; }
DYLIB="$DYLIB_LIST"
echo "module dylib: $DYLIB (replaying $N_RSP compile rsps + 1 link rsp)"

# [2026-10-27 T-022] (2) Back up the real dylib; restore it via trap on
# any exit. A backup still present at startup means a previous run was
# killed before its trap could run: restore that one first, so we never
# back up an already-instrumented dylib as if it were the original.
BACKUP="$COV_DIR/libUnrealEditor-UnderTheMountain.dylib.bak"
if [ -e "$BACKUP" ]; then
  echo "NOTE: leftover coverage backup found (interrupted previous run?) - restoring it first"
  cp -p "$BACKUP" "$DYLIB" && rm -f "$BACKUP"
fi
cp -p "$DYLIB" "$BACKUP"
restore_dylib() {
  if [ -s "$BACKUP" ]; then
    cp -p "$BACKUP" "$DYLIB" && rm -f "$BACKUP" && echo "restored original module dylib: $DYLIB"
  fi
}
trap restore_dylib EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# [2026-10-27 T-022] (3) Recompile each module cpp from its UBT rsp.
# Expected rsp shape (verified 2026-10-27, UE 5.8.4): first line '-c',
# exactly one quoted .cpp source line, exactly one '-MD -MF"..."' dep
# line, exactly one '-o "..."' line. The modified copy drops the dep
# line (UBT bookkeeping; it names the original .o) and points -o into
# Saved/Coverage/obj/. Everything else passes through byte-for-byte.
rm -rf "$OBJ_DIR"
mkdir -p "$OBJ_DIR"
for rsp in "$RSP_DIR"/*.cpp.o.rsp; do
  obj_name="$(basename "$rsp" .rsp)"
  obj_path="$OBJ_DIR/$obj_name"
  mod_rsp="$OBJ_DIR/$obj_name.rsp"
  [ "$(head -n 1 "$rsp")" = "-c" ] \
    || { echo "FAIL: unexpected rsp shape in $rsp (first line is not -c)"; exit 1; }
  [ "$(grep -c '^-o "' "$rsp" || true)" = 1 ] \
    || { echo "FAIL: unexpected rsp shape in $rsp (want exactly one '-o \"...\"' line)"; exit 1; }
  [ "$(grep -c '^-MD -MF"' "$rsp" || true)" = 1 ] \
    || { echo "FAIL: unexpected rsp shape in $rsp (want exactly one '-MD -MF\"...\"' line)"; exit 1; }
  [ "$(grep -c '^"/.*\.cpp"$' "$rsp" || true)" = 1 ] \
    || { echo "FAIL: unexpected rsp shape in $rsp (want exactly one quoted .cpp source line)"; exit 1; }
  awk -v obj="$obj_path" '
    /^-MD -MF"/ { next }
    /^-o "/ { print "-o \"" obj "\""; next }
    { print }
  ' "$rsp" > "$mod_rsp"
  echo "coverage compile: $obj_name"
  (cd "$ENGINE_SRC" && "$CLANGXX" "@$mod_rsp" -fprofile-instr-generate -fcoverage-mapping)
  [ -s "$obj_path" ] || { echo "FAIL: instrumented object missing at $obj_path"; exit 1; }
done

# [2026-10-27 T-022] (4) Relink the module dylib from UBT's link rsp,
# with every module .o swapped for its instrumented twin in OBJ_DIR.
# Expected link rsp shape: exactly one '-o "..."' line naming the real
# dylib found above, and one quoted "$RSP_DIR/<name>.cpp.o" line per
# compile rsp. An object from anywhere else, a count mismatch, or a
# different -o target means the rsps do not describe the built dylib:
# fail loudly instead of linking a guess.
[ "$(grep -c '^-o "' "$LINK_RSP" || true)" = 1 ] \
  || { echo "FAIL: unexpected rsp shape in $LINK_RSP (want exactly one '-o \"...\"' line)"; exit 1; }
LINK_O="$(sed -n 's/^-o "\(.*\)"$/\1/p' "$LINK_RSP")"
[ "$LINK_O" = "$DYLIB" ] \
  || { echo "FAIL: link rsp -o ($LINK_O) is not the built module dylib ($DYLIB) - run Scripts/build.sh first"; exit 1; }
N_LINK_OBJ="$(grep -c '^".*\.cpp\.o"$' "$LINK_RSP" || true)"
[ "$N_LINK_OBJ" = "$N_RSP" ] \
  || { echo "FAIL: link rsp lists $N_LINK_OBJ module objects but $N_RSP compile rsps were replayed - run Scripts/build.sh first"; exit 1; }
MOD_LINK_RSP="$OBJ_DIR/libUnrealEditor-UnderTheMountain.dylib.rsp"
awk -v objdir="$OBJ_DIR" -v rspdir="$RSP_DIR" '
  /^".*\.cpp\.o"$/ {
    p = $0; sub(/^"/, "", p); sub(/"$/, "", p)
    if (index(p, rspdir "/") != 1) {
      print "unexpected object in link rsp: " p > "/dev/stderr"; bad = 1; next
    }
    n = split(p, parts, "/")
    print "\"" objdir "/" parts[n] "\""; next
  }
  { print }
  END { exit bad }
' "$LINK_RSP" > "$MOD_LINK_RSP" || { echo "FAIL: unexpected rsp shape in $LINK_RSP"; exit 1; }
for rsp in "$RSP_DIR"/*.cpp.o.rsp; do
  obj_name="$(basename "$rsp" .rsp)"
  grep -Fq "\"$OBJ_DIR/$obj_name\"" "$MOD_LINK_RSP" \
    || { echo "FAIL: instrumented object $OBJ_DIR/$obj_name not present in modified link rsp"; exit 1; }
done
echo "coverage link: $DYLIB"
(cd "$ENGINE_SRC" && "$CLANGXX" "@$MOD_LINK_RSP" -fprofile-instr-generate)
[ -s "$DYLIB" ] || { echo "FAIL: relinked module dylib missing at $DYLIB"; exit 1; }

# [2026-10-27 T-022] Drop stale raw profiles first: a leftover .profraw must
# never let a broken instrumentation run look measured.
rm -f "$COV_DIR"/*.profraw "$PROFDATA"

# [2026-10-27 T-022] (5) Same headless automation run as Scripts/test-fast.sh,
# minus its build step (Scripts/build.sh would relink the normal dylib over
# the instrumented one before the tests run). %p only: %p-%m produced no
# file at all in the 2026-10-28 run (editor log proved the instrumented
# dylib loaded and tests passed), while a %p-only mini-run created the
# .profraw - the %m signature expansion is the difference. %c enables
# LLVM continuous mode: the 2026-10-28 %p-only run produced a 0-byte
# .profraw (the editor's shutdown bypasses the atexit flush), while %c
# memory-maps the counters and writes them live, no exit flush needed.
LLVM_PROFILE_FILE="$COV_DIR/coverage-%p%c.profraw" "$UE_EDITOR_CMD" "$UPROJECT" -ExecCmds="Automation RunTests UnderTheMountain; Quit" \
  -TestExit="Automation Test Queue Empty" -NullRHI -Unattended -NoSplash -NoSound
echo "OK: automation tests completed under coverage instrumentation"

set -- "$COV_DIR"/*.profraw
[ -e "$1" ] || { echo "FAIL: no .profraw produced in $COV_DIR (instrumentation flags did not take effect?)"; exit 1; }
# [2026-10-28 T-022] A 0-byte .profraw merges into an all-zeros profile and
# would print a fake 0.0% (seen in the pre-%c run) - refuse it loudly.
[ -s "$1" ] || { echo "FAIL: .profraw at $1 is empty (profile counters never flushed - is %c continuous mode in LLVM_PROFILE_FILE?)"; exit 1; }

xcrun llvm-profdata merge -sparse -o "$PROFDATA" "$COV_DIR"/*.profraw
[ -s "$PROFDATA" ] || { echo "FAIL: llvm-profdata produced no merged profile at $PROFDATA"; exit 1; }
[ -s "$DYLIB" ] || { echo "FAIL: instrumented module binary missing at $DYLIB"; exit 1; }

# [2026-10-27 T-022] Human-readable report restricted to this module's
# product sources. [2026-10-28 T-022] Denominator excludes Tests/: counting
# the tests' own lines (100% covered by construction) inflated the first
# measured 39.3%; product-only on the same profile was 31.1%.
SOURCES_LIST="$COV_DIR/sources.txt"
find Source/UnderTheMountain \( -name '*.cpp' -o -name '*.h' \) ! -path '*/Tests/*' | sort > "$SOURCES_LIST"
[ -s "$SOURCES_LIST" ] || { echo "FAIL: no sources found under Source/UnderTheMountain"; exit 1; }
set --
while IFS= read -r f; do
  set -- "$@" "$PROJECT_ROOT/$f"
done < "$SOURCES_LIST"
xcrun llvm-cov report "$DYLIB" -instr-profile="$PROFDATA" "$@" > "$COV_DIR/report.txt"
[ -s "$COV_DIR/report.txt" ] && grep -q '^TOTAL' "$COV_DIR/report.txt" \
  || { echo "FAIL: llvm-cov report missing or has no TOTAL row ($COV_DIR/report.txt)"; exit 1; }
cat "$COV_DIR/report.txt"

# [2026-10-27 T-022] The number itself comes from the JSON export, filtered
# per-file to Source/UnderTheMountain, instead of scraping report columns.
command -v python3 >/dev/null 2>&1 || { echo "FAIL: python3 not found (needed to read llvm-cov JSON export)"; exit 1; }
xcrun llvm-cov export -summary-only -instr-profile="$PROFDATA" "$DYLIB" > "$COV_DIR/summary.json"
[ -s "$COV_DIR/summary.json" ] || { echo "FAIL: llvm-cov export produced no summary at $COV_DIR/summary.json"; exit 1; }
PCT="$(python3 - "$COV_DIR/summary.json" <<'PYEOF'
import json
import sys

with open(sys.argv[1]) as handle:
    data = json.load(handle)
count = 0
covered = 0
for entry in data["data"][0]["files"]:
    name = entry["filename"].replace("\\", "/")
    if "/Source/UnderTheMountain/" not in name:
        continue
    if "/Tests/" in name:
        continue
    lines = entry["summary"]["lines"]
    count += lines["count"]
    covered += lines["covered"]
if count == 0:
    sys.exit("no Source/UnderTheMountain lines in coverage export")
print(f"{100.0 * covered / count:.1f}")
PYEOF
)" || { echo "FAIL: could not compute line coverage from $COV_DIR/summary.json"; exit 1; }

printf '%s\n' "$PCT" > "$COV_DIR/line-coverage.txt"
echo "line coverage: $PCT%"
