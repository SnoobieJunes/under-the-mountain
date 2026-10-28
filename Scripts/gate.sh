#!/bin/sh
# [2026-10-27 T-002] Full pre-commit gate: setup, editor+game builds,
# clang-format check on Source/, automation tests. Red anywhere = red gate.
# [2026-10-27 T-022] Plus a coverage-floor check on the Saved/Coverage
# artifact after the tests (not a coverage.sh run; see that block).
set -eu
. "$(dirname "$0")/ue-env.sh"
Scripts/setup.sh
Scripts/build.sh
"$UE_BUILD" UnderTheMountain Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload -NoUba
echo "OK: UnderTheMountain (game) built"
CLANG_FORMAT="$(xcrun --find clang-format)"
find Source -name '*.h' -o -name '*.cpp' | while read -r f; do
  "$CLANG_FORMAT" --dry-run --Werror -style=file:"$PROJECT_ROOT/.clang-format" "$f" >/dev/null
done
echo "OK: clang-format check (Source/)"
Scripts/test-fast.sh
# [2026-10-27 T-022] Coverage floor check. gate.sh deliberately does NOT run
# Scripts/coverage.sh: even the shipped module-only rsp-relink path (see
# coverage.sh header; the whole-target UBT flag channel was a measured
# 2750-action rebuild and is abandoned) recompiles the module and re-runs
# the headless test pass this gate just ran, so it stays out of the
# pre-commit loop. The honest cheaper
# enforcement is to verify the artifact coverage.sh produces: it must exist,
# be a number at or above the AGENTS.md snapshot floor, and be newer than
# every file under Source/UnderTheMountain. Missing/stale/below-floor all
# fail loudly here instead of passing silently. Consequence, on purpose: the
# gate is red on a fresh clone and after any Source/UnderTheMountain edit
# until Scripts/coverage.sh is re-run.
FLOOR="$(sed -n 's/^- \*\*Coverage floor:\*\* \([0-9][0-9.]*\)%.*/\1/p' AGENTS.md | head -n 1)"
[ -n "$FLOOR" ] || { echo "FAIL: could not parse coverage floor from AGENTS.md snapshot line"; exit 1; }
COV_TXT="Saved/Coverage/line-coverage.txt"
[ -s "$COV_TXT" ] || { echo "FAIL: $COV_TXT missing - run Scripts/coverage.sh"; exit 1; }
STALE="$(find Source/UnderTheMountain -type f -newer "$COV_TXT" | head -n 1)"
[ -z "$STALE" ] || { echo "FAIL: $COV_TXT is stale ($STALE is newer) - re-run Scripts/coverage.sh"; exit 1; }
VALUE="$(tr -d '[:space:]' < "$COV_TXT")"
case "$VALUE" in
  ''|*[!0-9.]*) echo "FAIL: $COV_TXT does not contain a bare number (got '$VALUE') - re-run Scripts/coverage.sh"; exit 1 ;;
esac
awk -v v="$VALUE" -v f="$FLOOR" 'BEGIN { exit !(v + 0 >= f + 0) }' \
  || { echo "FAIL: line coverage $VALUE% is below the AGENTS.md floor $FLOOR%"; exit 1; }
echo "OK: coverage $VALUE% >= floor $FLOOR% ($COV_TXT)"
echo "GATE GREEN"
