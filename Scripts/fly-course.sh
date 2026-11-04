#!/bin/sh
# [2026-11-03 T-075] Scripted course runner: builds the game target, then flies
# the T-075 legs in the real Lair (real ticks, real geometry) and exits.
# Evidence lands in the game log (FlightCourse/CourseLeg/CourseLab lines) plus
# the per-tick CSV dump. Usage: Scripts/fly-course.sh [scheme]
# (scheme default 2 = C hybrid; 1 = A, 3 = B). Shipped game untouched: the
# course only arms on utm.flight.course, the scheme gate defaults to shipped.
# [2026-11-04 T-124] Env knobs (positional args belong to the build line
# below, so these ride the environment): HOVER = hover-leg seconds
# (default 5; stamina proofs run 65), GOD = 1 enables
# utm.testing.infinitestamina for the run. The autostart cvar below is
# load-bearing, not convenience: since T-072's title screen (paused
# world) the course never arms without it — the pawn never ticks at
# Title, so two T-124 plain runs sat unarmed until killed, and the one
# god run that flew did so on T-112's unexplained auto-start flake.
set -eu
. "$(dirname "$0")/ue-env.sh"
SCHEME="${1:-2}"
HOVER="${HOVER:-5}"
EXECCMDS="utm.intro.enabled 0, utm.gameflow.autostart 1, utm.flight.scheme $SCHEME, utm.flight.course 1, utm.flight.coursehover $HOVER"
if [ "${GOD:-0}" = "1" ]; then
  EXECCMDS="$EXECCMDS, utm.testing.infinitestamina 1"
fi

"$UE_BUILD" UnderTheMountain Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload -NoUba "$@"
echo "OK: UnderTheMountain (game) built - flying course on scheme $SCHEME (hover ${HOVER}s, god ${GOD:-0})"

exec "$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
	"$UPROJECT" /Game/Maps/Lair/Lair -game -windowed -ResX=1280 -ResY=720 \
	-ExecCmds="$EXECCMDS" \
	-Unattended -NoSound
