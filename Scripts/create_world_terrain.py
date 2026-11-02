#!/usr/bin/env python3
# [2026-11-01 T-049] Entry point for the huge-world terrain.
#
# Why this is a launcher and not an unreal-python script like its
# siblings: creating a Landscape from Unreal Python is impossible -
# spawning ALandscape inside a -run=pythonscript commandlet asserts,
# and GUI-editor Python receives a LandscapePlaceholder with zero
# components (both proven in T-049 probes). The actual generator is
# an editor commandlet in C++ (Source/UnderTheMountain/Private/
# Editor/TerrainCommandlet.cpp, UTerrainCommandlet), which calls
# ALandscapeProxy::Import - the import wizard's own entry point.
# This file just runs it against the right project/engine paths and
# forwards the exit code. Re-runnable: the commandlet destroys and
# rebuilds Terrain_World identically (closed-form height function).
#
# Usage: python3 Scripts/create_world_terrain.py
import os
import subprocess
import sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UE_ROOT = os.environ.get(
    "UE_ROOT", "/Users/auston/Development Projects/UnrealEngine")
EDITOR_CMD = os.path.join(
    UE_ROOT, "Engine", "Binaries", "Mac", "UnrealEditor-Cmd")
UPROJECT = os.path.join(PROJECT_ROOT, "UnderTheMountain.uproject")


def main():
    if not os.path.isfile(EDITOR_CMD):
        print("create_world_terrain: editor binary not found at %s "
              "(set UE_ROOT)" % EDITOR_CMD, file=sys.stderr)
        return 1
    cmd = [EDITOR_CMD, UPROJECT, "-run=Terrain",
           "-Unattended", "-NoSplash", "-NoSound"]
    print("create_world_terrain: %s" % " ".join(cmd))
    result = subprocess.run(cmd)
    if result.returncode == 0:
        print("create_world_terrain: TERRAIN_DONE expected in the log above")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
