#!/bin/sh
# [2026-11-03 T-075] Scripted course runner: builds the game target, then flies
# the T-075 legs in the real Lair (real ticks, real geometry) and exits.
# Evidence lands in the game log (FlightCourse/CourseLeg/CourseLab lines) plus
# the per-tick CSV dump. Usage: Scripts/fly-course.sh [scheme]
# (scheme default 2 = C hybrid; 1 = A, 3 = B). Shipped game untouched: the
# course only arms on utm.flight.course, the scheme gate defaults to shipped.
set -eu
. "$(dirname "$0")/ue-env.sh"
SCHEME="${1:-2}"

"$UE_BUILD" UnderTheMountain Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload -NoUba "$@"
echo "OK: UnderTheMountain (game) built - flying course on scheme $SCHEME"

exec "$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
	"$UPROJECT" /Game/Maps/Lair/Lair -game -windowed -ResX=1280 -ResY=720 \
	-ExecCmds="utm.intro.enabled 0, utm.flight.scheme $SCHEME, utm.flight.course 1" \
	-Unattended -NoSound
