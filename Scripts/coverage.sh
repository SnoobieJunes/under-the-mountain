#!/bin/sh
# [2026-10-27 T-002] Coverage report. Honest baseline: UE C++ line coverage is
# NOT instrumented in this build (no -Coverage UBT pass configured); this script
# measures test presence instead of inventing a percentage. Floor in AGENTS.md
# is 0% until T-022 instruments llvm-cov. It fails loudly if tests vanish.
set -eu
COUNT="$(grep -rl 'IMPLEMENT_.*AUTOMATION_TEST' Source/UnderTheMountain/Tests 2>/dev/null | wc -l | tr -d ' ')"
echo "automation test files: $COUNT"
[ "$COUNT" -ge 1 ] || { echo "FAIL: no automation tests found"; exit 1; }
echo "line coverage: not instrumented (floor 0%, see T-022)"
