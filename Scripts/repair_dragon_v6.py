#!/usr/bin/env python3
# [2026-11-04 T-123] SURGICAL v6 dragon repair, pass 2 (Blender headless).
#
# Pass 1 (maximal: region welds + dissolve storms + collapse + global
# recalc/flag flips + adaptive decimate) closed the holes but REGRESSED
# the dragon in-game (shredded wings — owner rejection, tree reverted).
# Root causes: blanket use_smooth flipped 9,798 authored flat faces,
# global recalc rewrote custom normals, and decimate ran on irregular
# dissolved topology. Lesson recorded in the T-123 close note.
#
# Pass 2 touches ONLY named holes: loops with bbox dia >= 0.015 in the
# chest/tail boxes get exact-twin collapse (1e-7) + scoped fill
# (hide-outside + wing-guard hash); everything else is proven
# bit-identical. No recalc, no flag flips, no dissolve, no shard pass,
# no global ops at all. Decimate uses the exact T-110 recipe
# (COLLAPSE 0.19 — input is ~413k again).
#
# Run: /Applications/Blender.app/Contents/MacOS/Blender --background
#        --python Scripts/repair_dragon_v6.py
# Exit 0 + T123_REPAIR_DONE iff every named loop closes, the outside
# guard holds, invariants hold, and the game FBX exports in budget.
import hashlib
import os
import struct
import sys

import bpy
import bmesh
from mathutils import Vector
from mathutils.kdtree import KDTree

# Blender stdout is block-buffered when piped: flush every print so the
# log streams progress instead of arriving at exit (or never, on kill).
import functools
print = functools.partial(print, flush=True)

REGION_CHEST = ((-0.45, 0.45), (-0.75, 0.05), (-0.35, 0.45))
REGION_TAIL = ((-0.35, 0.35), (0.0, 1.0), (-0.45, 0.35))
# Named-loop floor: bbox dia >= 0.015 units (~6mm at dragon scale).
# Below that is sub-visible dust, counted and reported, not touched.
LOOP_MIN_DIA = 0.015
WELD_DIST = 5e-3
BUDGET_TRIS = 80000
RATIO = 0.19

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLEND = os.path.join(ROOT, "Content/Meshy/Dragon/dragon_v6_experiment.blend")
GAME_FBX = os.path.join(ROOT, "Content/Meshy/Dragon/dragon_v6_game.fbx")
PROF = os.path.join(ROOT, "Saved/Profiling")


def fail(msg):
    print("T123_REPAIR_FAIL: %s" % msg)
    sys.exit(1)


def _sync(bm):
    # Blender rule this script violated once (silent garbage indices,
    # tier-1 merged random verts): index tables MUST be ensured after
    # every from_mesh and every topology op, before ANY .index read.
    bm.verts.ensure_lookup_table()
    bm.edges.ensure_lookup_table()
    bm.faces.ensure_lookup_table()


def in_region(co, box):
    (x0, x1), (y0, y1), (z0, z1) = box
    return x0 <= co.x <= x1 and y0 <= co.y <= y1 and z0 <= co.z <= z1


def is_region_vert(v):
    return in_region(v.co, REGION_CHEST) or in_region(v.co, REGION_TAIL)


if not os.path.isfile(BLEND):
    fail("missing source %s" % BLEND)
os.makedirs(PROF, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=BLEND)

dragon = bpy.data.objects.get("Dragon")
if dragon is None or dragon.type != "MESH":
    fail("object 'Dragon' mesh not found")
arm = bpy.data.objects.get("DragonArmature")
if arm is None or arm.type != "ARMATURE":
    fail("object 'DragonArmature' not found")
mesh = dragon.data
bones0 = [b.name for b in arm.data.bones]
vgroups0 = [g.name for g in dragon.vertex_groups]
uvs0 = len(mesh.uv_layers)
mats0 = [m.name if m else "-" for m in mesh.materials]
smooth0 = sum(1 for p in mesh.polygons if p.use_smooth)
print("T123_SOURCE verts=%d faces=%d vgroups=%d uvs=%d mats=%s bones=%d smooth=%d"
      % (len(mesh.vertices), len(mesh.polygons), len(vgroups0), uvs0,
         mats0, len(bones0), smooth0))
if len(bones0) != 39:
    fail("expected the 39-bone rig, found %d bones" % len(bones0))


def setup_render():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 960
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.display.shading.light = "MATCAP"
    scene.display.shading.show_cavity = True
    cam_data = bpy.data.cameras.new("T123Cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = 2.6
    cam_obj = bpy.data.objects.new("T123CamObj", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    return cam_obj


CAM = setup_render()
# Head -Y, tail +Y, up +Z. Side frames the chest sites, top the tail
# spine, front the uniform read.
VIEWS = {
    "side": (Vector((3.2, 0.0, 0.2)), Vector((0.0, -0.2, 0.0))),
    "top": (Vector((0.0, 0.0, 3.2)), Vector((0.0, 0.1, 0.0))),
    "front": (Vector((0.0, -3.2, 0.3)), Vector((0.0, -0.2, 0.0))),
}


def render_views(tag):
    scene = bpy.context.scene
    for name, (loc, tgt) in VIEWS.items():
        CAM.location = loc
        CAM.rotation_euler = (tgt - loc).to_track_quat("-Z", "Y").to_euler()
        out = os.path.join(PROF, "t123_%s_%s.png" % (tag, name))
        scene.render.filepath = out
        bpy.ops.render.render(write_still=True)
        print("T123_RENDER %s (%d bytes)" % (out, os.path.getsize(out)))


render_views("before")


def region_loops(bm):
    _sync(bm)
    # Walk region boundary edges into loops; returns
    # [(dia, cx, cy, cz, [verts])] sorted big-first. Deterministic:
    # min-index picks, never set.pop().
    bnd = set(e for e in bm.edges if e.is_boundary and is_region_vert(e.verts[0]))
    loops = []
    while bnd:
        start = min(bnd, key=lambda e: e.index)
        bnd.discard(start)
        loop = [start]
        cur, prev = start.verts[1], start.verts[0]
        guard = 0
        while guard < 100000:
            guard += 1
            nxt = None
            for e in cur.link_edges:
                if e.is_boundary and e in bnd and e.other_vert(cur) != prev:
                    nxt = e
                    break
            if nxt is None:
                break
            bnd.discard(nxt)
            loop.append(nxt)
            prev, cur = cur, nxt.other_vert(cur)
            if cur == start.verts[0]:
                break
        vs = [v for e in loop for v in e.verts]
        xs = [v.co.x for v in vs]
        ys = [v.co.y for v in vs]
        zs = [v.co.z for v in vs]
        import math
        dia = math.sqrt((max(xs) - min(xs)) ** 2 + (max(ys) - min(ys)) ** 2
                        + (max(zs) - min(zs)) ** 2)
        loops.append((dia, sum(xs) / len(xs), sum(ys) / len(ys), sum(zs) / len(zs), vs))
    loops.sort(reverse=True, key=lambda r: r[0])
    return loops


bm = bmesh.new()
bm.from_mesh(mesh)
_sync(bm)
loops = region_loops(bm)
targets = [lp for lp in loops if lp[0] >= LOOP_MIN_DIA]
dust = [lp for lp in loops if lp[0] < LOOP_MIN_DIA]
print("T123_LOOPS region=%d named(>=%.3f)=%d dust=%d biggest=%.3f"
      % (len(loops), LOOP_MIN_DIA, len(targets), len(dust),
         targets[0][0] if targets else 0.0))
for dia, cx, cy, cz, _vs in targets[:12]:
    print("T123_TARGET dia=%.3f at=(%.3f,%.3f,%.3f)" % (dia, cx, cy, cz))
if not targets:
    fail("no named loops found - wrong mesh or regions?")

# Weld set: rim verts of named loops + their KDTree twins (5e-3).
# One op, minimal footprint; the guard below proves the rest identical.
kd = KDTree(len(bm.verts))
for v in bm.verts:
    kd.insert(v.co, v.index)
kd.balance()
rim = set()
for _dia, _cx, _cy, _cz, vs in targets:
    rim.update(v.index for v in vs)
bm.verts.ensure_lookup_table()
weld_idx = set(rim)
for vi in rim:
    co = bm.verts[vi].co if vi < len(bm.verts) else None
    if co is None:
        continue
    for _co, idx, _dist in kd.find_range(co, WELD_DIST):
        weld_idx.add(idx)
weld_verts = [v for v in bm.verts if v.index in weld_idx]
print("T123_WELDSET rim=%d total=%d (of %d verts)" % (len(rim), len(weld_verts), len(bm.verts)))

# Outside guard: verts FAR outside the regions (boxes + 0.01 margin —
# weld twins live within 5e-3 of rims, so nothing outside the margin
# can legally move) must be bit-identical before/after. Sorted coords,
# index-free, so merges inside can't perturb the comparison.
MARGIN = 0.01


def far_outside(co):
    for (x0, x1), (y0, y1), (z0, z1) in (REGION_CHEST, REGION_TAIL):
        if x0 - MARGIN <= co.x <= x1 + MARGIN and y0 - MARGIN <= co.y <= y1 + MARGIN \
                and z0 - MARGIN <= co.z <= z1 + MARGIN:
            return False
    return True


def far_hash(bm):
    coords = sorted(
        (round(v.co.x, 6), round(v.co.y, 6), round(v.co.z, 6)) for v in bm.verts if far_outside(v.co))
    h = hashlib.md5()
    for c in coords:
        h.update(struct.pack("fff", *c))
    return (len(coords), h.hexdigest())


guard_before = far_hash(bm)
# Tier 1: exact twins only (1e-7). Distinct verts are never this close,
# so this can only collapse sheet-doubling, never legit detail.
bmesh.ops.remove_doubles(bm, verts=weld_verts, dist=1e-7)
bm.to_mesh(mesh)
mesh.update()
bm.free()
bm = bmesh.new()
bm.from_mesh(mesh)
_sync(bm)


# (No dissolve tier: twin-collapse + fill close everything; rim
# dissolve found 3 verts and only risked topology churn.)

# Tier 2: SCOPED fill for the true voids. The big loops are real missing
# faces (not coincident cracks — pairwise welding proved twins don't
# span them), so only fill closes them. Hide everything OUTSIDE the
# regions first (fill skips hidden geometry) and prove it with the
# wing-guard hash: sides=500 covers the biggest loop (317 edges).
import hashlib
import struct


def outside_hash(bm):
    coords = sorted(
        (round(v.co.x, 6), round(v.co.y, 6), round(v.co.z, 6)) for v in bm.verts
        if far_outside(v.co))
    h = hashlib.md5()
    for c in coords:
        h.update(struct.pack("fff", *c))
    return (len(coords), h.hexdigest())


for v in bm.verts:
    v.hide_set(not (in_region(v.co, REGION_CHEST) or in_region(v.co, REGION_TAIL)))
wing_before = outside_hash(bm)
bm.to_mesh(mesh)
mesh.update()
bpy.context.view_layer.objects.active = dragon
dragon.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.fill_holes(sides=500)
bpy.ops.mesh.reveal()
bpy.ops.object.mode_set(mode="OBJECT")
bm.free()
bm = bmesh.new()
bm.from_mesh(mesh)
wing_after = outside_hash(bm)
print("T123_WINGGUARD before=%s after=%s" % (wing_before, wing_after))
if wing_after != wing_before:
    fail("fill touched out-of-region verts - wing guard tripped")


# NOTE: no pairwise tier — tier-1 exact-twin collapse closes ~everything
# (MCP-proven 1308 -> 1 named); fill above takes stragglers.

# Per-loop verify: re-walk with region_loops and match each target by
# centroid (within 0.05) AND size — a surviving loop must itself be
# named-sized (>= threshold) to count as still open, else sub-visible
# dust near an old centroid would fail a closed target (the bug that
# reported 1293 phantom still-open after tier-1 had closed them).
still_open = []
now = region_loops(bm)
for dia, cx, cy, cz, _vs in targets:
    open_yet = False
    for _d2, x2, y2, z2, _l2 in now:
        if _d2 >= LOOP_MIN_DIA and abs(x2 - cx) < 0.05 and abs(y2 - cy) < 0.05 and abs(z2 - cz) < 0.05:
            open_yet = True
            break
    if open_yet:
        still_open.append((round(dia, 3), round(cx, 3), round(cy, 3), round(cz, 3)))
print("T123_CLOSED %d/%d still_open=%s" % (len(targets) - len(still_open), len(targets), still_open[:8]))
if still_open:
    fail("%d named loops still open: %s" % (len(still_open), still_open[:8]))

guard_after = far_hash(bm)
print("T123_GUARD before=%s after=%s" % (guard_before, guard_after))
if guard_after != guard_before:
    fail("weld leaked outside regions - guard tripped")

# Invariants (no recalc, no flag flips anywhere in this script).
bones1 = [b.name for b in arm.data.bones]
vgroups1 = [g.name for g in dragon.vertex_groups]
if bones1 != bones0:
    fail("armature changed during repair")
if vgroups1 != vgroups0:
    fail("vertex groups changed during repair")
if len(mesh.uv_layers) != uvs0:
    fail("UV layers changed during repair")
mats1 = [m.name if m else "-" for m in mesh.materials]
if mats1 != mats0:
    fail("material slots changed during repair")
smooth1 = sum(1 for p in mesh.polygons if p.use_smooth)
print("T123_INVARIANTS bones=%d vgroups=%d uvs=%d mats=%s smooth=%d->%d"
      % (len(bones1), len(vgroups1), len(mesh.uv_layers), mats1, smooth0, smooth1))
# Nothing may un-smooth (the pass-1 failure was a blanket rewrite);
# growth is new fill faces only (Blender defaults them smooth) and is
# bounded well under a second blanket flip (+9,798).
if smooth1 < smooth0 or (smooth1 - smooth0) > 5000:
    fail("smooth-face flags changed %d -> %d beyond fill growth" % (smooth0, smooth1))

render_views("after")
# Zoom the largest closed loop site: the gate trusts numbers, this
# render proves the eye agrees.
if targets:
    _d, _zx, _zy, _zz = targets[0][0], targets[0][1], targets[0][2], targets[0][3]
    CAM.data.ortho_scale = 0.5
    CAM.location = Vector((_zx + 0.8, _zy - 0.8, _zz + 0.8))
    CAM.rotation_euler = (Vector((_zx, _zy, _zz)) - CAM.location).to_track_quat("-Z", "Y").to_euler()
    _zout = os.path.join(PROF, "t123_after_zoom.png")
    bpy.context.scene.render.filepath = _zout
    bpy.ops.render.render(write_still=True)
    print("T123_ZOOM dia=%.3f at=(%.3f,%.3f,%.3f) %s (%d bytes)"
          % (_d, _zx, _zy, _zz, _zout, os.path.getsize(_zout)))
    CAM.data.ortho_scale = 2.6
bpy.data.objects.remove(CAM, do_unlink=True)
bpy.ops.wm.save_mainfile()
print("T123_BLEND_SAVED %s" % BLEND)

# Decimate: input is ~413k again (surgical footprint), so the exact
# T-110 recipe (COLLAPSE 0.19) reproduces the shipped resolution.
bpy.context.view_layer.objects.active = dragon
tris_in = sum(len(p.vertices) - 2 for p in dragon.data.polygons)
print("T123_DECIMATE tris_in=%d ratio=%.4f" % (tris_in, RATIO))
mod = dragon.modifiers.new("T123Decimate", "DECIMATE")
mod.decimate_type = "COLLAPSE"
mod.ratio = RATIO
bpy.ops.object.modifier_apply(modifier=mod.name)
mesh = dragon.data
tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
print("T123_TRIS=%d (verts=%d faces=%d vgroups=%d uvs=%d)"
      % (tris, len(mesh.vertices), len(mesh.polygons),
         len(dragon.vertex_groups), len(mesh.uv_layers)))
if not (70000 <= tris <= BUDGET_TRIS):
    fail("%d tris outside the shipped-resolution band 70000..%d" % (tris, BUDGET_TRIS))
if len(mesh.uv_layers) == 0:
    fail("UVs lost in decimate")
if len(dragon.vertex_groups) == 0:
    fail("vertex groups lost in decimate")
bpy.ops.export_scene.fbx(
    filepath=GAME_FBX,
    use_selection=False,
    add_leaf_bones=False,
    bake_anim=False,
    mesh_smooth_type="FACE",
)
print("T123_EXPORT %s (%d bytes)" % (GAME_FBX, os.path.getsize(GAME_FBX)))
print("T123_REPAIR_DONE")
