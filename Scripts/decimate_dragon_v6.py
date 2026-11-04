#!/usr/bin/env python3
# [2026-11-03 T-110] Decimate the v6 dragon to the hero budget.
#
# Dragon_v6.fbx ships 413,454 tris (289,485 verts) — 5.2x over the
# <=80k hero budget (docs/ASSET-PIPELINE.md step 3; the v4 precedent in
# T-028 decimated 309k -> 79k the same way). This script runs Blender
# headless: import, COLLAPSE-decimate to <=80k tris (UVs + skin weights
# preserved — decimate interpolates both), export dragon_v6_game.fbx
# beside the .blend (the dragon_v5_mesh.fbx precedent), print the counts.
#
# Run: /Applications/Blender.app/Contents/MacOS/Blender --background
#        --python Scripts/decimate_dragon_v6.py
# Exit 0 + V6_DECIMATE_DONE iff tris <= 80000 with UVs + weights intact.
import os
import sys

import bpy

BUDGET_TRIS = 80000
# Measured source load is 413,454 tris; aim just under budget so the
# assert below has margin against decimate-ratio rounding.
RATIO = 0.19

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# [2026-11-04 T-119] Source re-pointed: T-113 retired the old
# dragon-wings-v6/ drop; the live owner drop is
# Content/Meshy/dragon-wings-v6(new)/exports/Dragon_v6.fbx (proven
# identical to the T-110 source in T-113). DST is unchanged: it is the
# dragon_v6_game.fbx import_dragon_v6.py consumes.
SRC = os.path.join(ROOT, "Content/Meshy/dragon-wings-v6(new)/exports/Dragon_v6.fbx")
DST = os.path.join(ROOT, "Content/Meshy/Dragon/dragon_v6_game.fbx")


def fail(msg):
    print("V6_DECIMATE_FAIL: %s" % msg)
    sys.exit(1)


if not os.path.isfile(SRC):
    fail("missing source %s" % SRC)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)

meshes = [o for o in bpy.data.objects if o.type == "MESH"]
if len(meshes) != 1:
    fail("expected exactly 1 mesh, found %d" % len(meshes))
mesh_obj = meshes[0]
mesh = mesh_obj.data
if len(mesh.uv_layers) == 0:
    fail("source mesh has no UVs")
if len(mesh_obj.vertex_groups) == 0:
    fail("source mesh has no vertex groups (skin weights)")
print("V6_SOURCE verts=%d faces=%d vgroups=%d uvs=%d materials=%d"
      % (len(mesh.vertices), len(mesh.polygons), len(mesh_obj.vertex_groups),
         len(mesh.uv_layers), len(mesh.materials)))

mod = mesh_obj.modifiers.new("V6Decimate", "DECIMATE")
mod.decimate_type = "COLLAPSE"
mod.ratio = RATIO
bpy.context.view_layer.objects.active = mesh_obj
bpy.ops.object.modifier_apply(modifier=mod.name)

mesh = mesh_obj.data
tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
print("V6_TRIS=%d (verts=%d faces=%d vgroups=%d uvs=%d)"
      % (tris, len(mesh.vertices), len(mesh.polygons),
         len(mesh_obj.vertex_groups), len(mesh.uv_layers)))
if tris > BUDGET_TRIS:
    fail("%d tris still over the %d budget" % (tris, BUDGET_TRIS))
if len(mesh.uv_layers) == 0:
    fail("UVs lost in decimate")
if len(mesh_obj.vertex_groups) == 0:
    fail("vertex groups lost in decimate")

bpy.ops.export_scene.fbx(
    filepath=DST,
    use_selection=False,
    add_leaf_bones=False,
    bake_anim=False,
    mesh_smooth_type="FACE",
)
print("V6_EXPORT %s (%d bytes)" % (DST, os.path.getsize(DST)))
print("V6_DECIMATE_DONE")
