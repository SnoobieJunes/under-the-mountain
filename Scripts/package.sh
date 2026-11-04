#!/bin/sh
# [2026-11-04 T-020] One-command packaged macOS build (the T-020 vehicle).
# RunUAT BuildCookRun: cook the MapsToCook list from Config/DefaultGame.ini,
# build the game target, stage (ICU data + dylibs land in the staged layout —
# the T-057 root cause was launching the UNSTAGED binary), pak, and archive
# a runnable .app. Output defaults to Saved/Packaged (gitignored); override
# with PACKAGE_ARCHIVE_DIR. Run from the repo root, like every Scripts/*.sh.
# Exit 0 + the OK line iff the .app exists under the archive dir.
set -eu
. "$(dirname "$0")/ue-env.sh"
ARCHIVE_DIR="${PACKAGE_ARCHIVE_DIR:-$PROJECT_ROOT/Saved/Packaged}"
mkdir -p "$ARCHIVE_DIR"
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$UPROJECT" -noP4 \
  -platform=Mac -clientconfig=Development \
  -cook -build -stage -pak -package \
  -archive "-archivedirectory=$ARCHIVE_DIR" \
  -prereqs -nodebuginfo -compressed
APP="$(find "$ARCHIVE_DIR" -maxdepth 4 -name 'UnderTheMountain.app' -type d | head -n 1)"
[ -n "$APP" ] || { echo "FAIL: no UnderTheMountain.app under $ARCHIVE_DIR" >&2; exit 1; }
echo "OK: packaged macOS build at $APP"
