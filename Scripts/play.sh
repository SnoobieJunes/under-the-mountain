#!/bin/sh
# [2026-11-01 T-056] One-command standalone playtest: build the game
# target, then launch a standalone -game window on the Lair with no
# editor and no MCP in the loop - so what you play is always the
# current code, never whatever a long-lived editor has loaded (the
# stale-editor trap behind T-047's "wings frozen" report and every
# dead-listener capture failure since).
#
# The launch goes through the engine's UnrealEditor binary in
# -game mode (a real game window, no editor UI). Launching the
# built Binaries/Mac/UnderTheMountain.app binary directly is the
# separate broken path T-057 owns (SIGSEGV/descriptor/ICU) - do not
# "fix" this script by pointing it there until T-057 closes.
set -eu
. "$(dirname "$0")/ue-env.sh"

"$UE_BUILD" UnderTheMountain Mac Development -Project="$UPROJECT" -WaitMutex -NoHotReload -NoUba
echo "OK: UnderTheMountain (game) built - launching standalone window"

exec "$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
	"$UPROJECT" /Game/Maps/Lair/Lair -game -windowed -ResX=1600 -ResY=900 "$@"
