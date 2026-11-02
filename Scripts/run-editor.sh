#!/bin/sh
# [2026-10-27 T-002] Open the project in Unreal Editor (GUI smoke path).
# [2026-11-01 T-048] One-instance guard + MCP readiness: only the first
# editor instance can bind the unreal-mcp port (:8000), so a second
# launch used to open an editor whose MCP was silently dead. The guard
# refuses a second launch while one is running; after launching, the
# script waits for the MCP port and says so (or says it never came up)
# instead of leaving the user to discover it mid-capture.
set -eu
. "$(dirname "$0")/ue-env.sh"

if pgrep -f "MacOS/UnrealEditor .*UnderTheMountain\.uproject" >/dev/null 2>&1; then
	echo "run-editor: an Unreal Editor for this project is already running." >&2
	echo "run-editor: MCP (:8000) lives in that instance only - use it, or quit it first." >&2
	exit 1
fi

"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$UPROJECT" "$@" &
EDITOR_PID=$!

# Wait up to 120 s for the MCP listener (editor cold start on this Mac
# is well under that; the Accept bar for T-048 is 2 minutes).
ATTEMPT=0
while [ "$ATTEMPT" -lt 60 ]; do
	if ! kill -0 "$EDITOR_PID" 2>/dev/null; then
		wait "$EDITOR_PID"
	fi
	if nc -z 127.0.0.1 8000 >/dev/null 2>&1; then
		echo "run-editor: unreal-mcp listening on http://127.0.0.1:8000/mcp (pid $EDITOR_PID)"
		break
	fi
	ATTEMPT=$((ATTEMPT + 1))
	sleep 2
done
if [ "$ATTEMPT" -ge 60 ]; then
	echo "run-editor: WARNING - editor is up but unreal-mcp is not listening on :8000 after 120 s." >&2
	echo "run-editor: check the editor log for LogModelContextProtocol, and see README (MCP section)." >&2
fi

wait "$EDITOR_PID"
