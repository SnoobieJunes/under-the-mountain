#!/bin/sh
# [2026-10-27 T-002] Full pre-commit gate: setup, editor+game builds,
# clang-format check on Source/, automation tests. Red anywhere = red gate.
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
echo "GATE GREEN"
