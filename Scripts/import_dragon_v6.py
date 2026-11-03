# [2026-11-03 T-110] Import the decimated v6 dragon (dragon_v6_game.fbx,
# [2026-11-03 T-110] Scripts/decimate_dragon_v6.py output: <=80k tris, 39-bone
# [2026-11-03 T-110] rig, 7 wing bones/side + finger fan) as SK_Dragon_v6 +
# [2026-11-03 T-110] SK_Dragon_v6_Skeleton at NEW paths (the v4 assets stay on
# [2026-11-03 T-110] disk for rollback; the pawn re-points in DragonCharacter.cpp).
# [2026-11-03 T-110] Imports the 6 4K PBR PNGs and builds two materials:
# [2026-11-03 T-110] M_DragonScalesV6 (body: base x ScaleTint — no crimson mask,
# [2026-11-03 T-110] the wing membranes are a separate material now) and
# [2026-11-03 T-110] M_DragonWingV6 (wing: base, no tint). ORM packs R=AO /
# [2026-11-03 T-110] G=Roughness / B=Metallic (packed-ORM convention; the T-111
# [2026-11-03 T-110] PIE review is the visual proof the channels read right).
# [2026-11-03 T-110] Like its v5 predecessor this script is NOT re-runnable
# [2026-11-03 T-110] while the assets exist (loaded-CDO foot-gun: the pawn holds
# [2026-11-03 T-110] SK_Dragon_v6 once this lands) — it raises naming the asset
# [2026-11-03 T-110] so a re-import is a deliberate delete-from-disk + restart.
# Run headless: "$UE_EDITOR_CMD" "$UPROJECT" -run=pythonscript
#   -script="<repo>/Scripts/import_dragon_v6.py" -Unattended -NullRHI -NoSplash
# Exit 0 + DRAGON_V6_IMPORT_DONE iff mesh + skeleton + 6 textures +
# 2 materials exist with the right slot assignment.
import os

import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/Meshy/Dragon"
TEX_DST = DST + "/Textures"
STAGE = DST + "/MeshImport"


def require(value, action):
    # [2026-10-28 T-037] Surface Unreal's False/None helper failures
    # as a named exception before destructive work or success text.
    if not value:
        raise RuntimeError("import_dragon_v6 failed: %s" % action)
    return value


MESH = DST + "/SK_Dragon_v6"
SKELETON = DST + "/SK_Dragon_v6_Skeleton"
MAT_BODY = DST + "/M_DragonScalesV6"
MAT_WING = DST + "/M_DragonWingV6"
for existing in (MESH, SKELETON, MAT_BODY, MAT_WING):
    if eal.does_asset_exist(existing):
        raise RuntimeError(
            "import_dragon_v6: %s already exists - delete it from disk and restart "
            "the editor before re-running (loaded-CDO foot-gun, see header)" % existing)

# --- 1. skeletal mesh, staged then renamed (the import_dragon_mesh.py shape) ---
task = unreal.AssetImportTask()
task.set_editor_property(
    "filename", unreal.Paths.project_dir() + "Content/Meshy/Dragon/dragon_v6_game.fbx")
task.set_editor_property("destination_path", STAGE)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
task.set_editor_property("options", options)
asset_tools.import_asset_tasks([task])

staged = list(eal.list_assets(STAGE, recursive=False))
print("V6_STAGED", staged)
renames = []
for path in staged:
    if path.endswith("_Skeleton"):
        renames.append((path, SKELETON))
    elif path.endswith("_PhysicsAsset"):
        renames.append((path, None))
    else:
        asset = eal.load_asset(path)
        if asset and asset.get_class().get_name() == "SkeletalMesh":
            renames.append((path, MESH))
        else:
            renames.append((path, None))
require(any(dst == MESH for _, dst in renames), "find staged SkeletalMesh")
require(any(dst == SKELETON for _, dst in renames), "find staged Skeleton")

# skeleton first so the mesh rename never points at a vanished path
for src, dst in sorted(renames, key=lambda r: 0 if r[1] and r[1].endswith("Skeleton") else 1):
    if dst is None:
        require(eal.delete_asset(src), "delete staged %s" % src)
    else:
        require(eal.rename_asset(src, dst), "rename %s -> %s" % (src, dst))

# --- 2. the six 4K textures (T_DragonV6_* — the auto-imported T_DragonBody_*
# --- partials were backup-moved before this import, so no collision) ---
SRC_TEX = (unreal.Paths.project_dir()
           + "Content/Meshy/Dragon/dragon-wings-v6/exports/textures/")


def import_tex(png, name, srgb, normalmap=False):
    dest = TEX_DST + "/" + name
    require(not eal.does_asset_exist(dest), "texture slot %s already taken" % dest)
    source = SRC_TEX + png
    require(os.path.isfile(source), "find source texture %s" % source)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", TEX_DST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    tex = require(eal.load_asset(dest), "load imported texture %s" % dest)
    tex.set_editor_property("srgb", srgb)
    if normalmap:
        tex.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    require(eal.save_asset(dest), "save texture %s" % dest)
    return tex


T_BODY_BASE = import_tex("T_DragonBody_BaseColor.png", "T_DragonV6_Body_BaseColor", True)
T_BODY_NORM = import_tex("T_DragonBody_Normal.png", "T_DragonV6_Body_Normal", False, True)
T_BODY_ORM = import_tex("T_DragonBody_ORM.png", "T_DragonV6_Body_ORM", False)
T_WING_BASE = import_tex("T_DragonWing_BaseColor.png", "T_DragonV6_Wing_BaseColor", True)
T_WING_NORM = import_tex("T_DragonWing_Normal.png", "T_DragonV6_Wing_Normal", False, True)
T_WING_ORM = import_tex("T_DragonWing_ORM.png", "T_DragonV6_Wing_ORM", False)
print("V6_TEXTURES", T_BODY_BASE, T_BODY_NORM, T_BODY_ORM, T_WING_BASE, T_WING_NORM, T_WING_ORM)

# --- 3. materials (the create_dragon_material.py graph shape, ORM-fed) ---
COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
LINEAR = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR


def build_material(name, base_tex, norm_tex, orm_tex, with_tint):
    mat = require(
        asset_tools.create_asset(
            name, DST, unreal.Material, unreal.MaterialFactoryNew()),
        "create material %s" % name)
    mat.set_editor_property("used_with_skeletal_mesh", True)

    def sample(tex, x, y, sampler):
        s = require(
            mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y),
            "create texture sample expression")
        s.set_editor_property("texture", tex)
        s.set_editor_property("sampler_type", sampler)
        return s

    base = sample(base_tex, -900, -300, COLOR)
    norm = sample(norm_tex, -900, 100, NORMAL)
    orm = sample(orm_tex, -900, 350, LINEAR)

    colour = base
    if with_tint:
        tint = mel.create_material_expression(
            mat, unreal.MaterialExpressionVectorParameter, -600, -450)
        tint.set_editor_property("parameter_name", "ScaleTint")
        tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
        tinted = mel.create_material_expression(
            mat, unreal.MaterialExpressionMultiply, -550, -300)
        mel.connect_material_expressions(base, "RGB", tinted, "A")
        mel.connect_material_expressions(tint, "", tinted, "B")
        colour = tinted

    def orm_chan(chan, x, y):
        m = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x, y)
        m.set_editor_property("r", chan == "R")
        m.set_editor_property("g", chan == "G")
        m.set_editor_property("b", chan == "B")
        m.set_editor_property("a", False)
        mel.connect_material_expressions(orm, "RGB", m, "")
        return m

    mel.connect_material_property(colour, "" if with_tint else "RGB",
                                  unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(norm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(orm_chan("G", -550, 350), "",
                                  unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(orm_chan("B", -550, 450), "",
                                  unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(orm_chan("R", -550, 550), "",
                                  unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.recompile_material(mat)
    require(eal.save_asset(DST + "/" + name), "save material %s" % name)
    return mat


mat_body = build_material("M_DragonScalesV6", T_BODY_BASE, T_BODY_NORM, T_BODY_ORM, True)
mat_wing = build_material("M_DragonWingV6", T_WING_BASE, T_WING_NORM, T_WING_ORM, False)
print("V6_MATERIALS", mat_body.get_name(), mat_wing.get_name())

# --- 4. slot assignment by slot name (wing slot -> wing material) ---
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([DST], force_rescan=True)
mesh = require(eal.load_asset(MESH), "load SK_Dragon_v6")
slots = mesh.get_editor_property("materials")
print("V6_SLOTS", [s.get_editor_property("material_slot_name").to_string()
                   if hasattr(s.get_editor_property("material_slot_name"), "to_string")
                   else str(s.get_editor_property("material_slot_name")) for s in slots])
new_slots = []
for slot in slots:
    raw_name = slot.get_editor_property("material_slot_name")
    slot_name = str(raw_name)
    picked = mat_wing if "wing" in slot_name.lower() else mat_body
    fresh = unreal.SkeletalMaterial()
    fresh.set_editor_property("material_interface", picked)
    new_slots.append(fresh)
    print("V6_SLOTMAP %s -> %s" % (slot_name, picked.get_name()))
# Whole-list replacement: mutating the fetched structs in place does
# not write back (proven in T-027 — slot stayed WorldGridMaterial).
mesh.set_editor_property("materials", new_slots)
require(eal.save_loaded_asset(mesh), "save SK_Dragon_v6 with materials")

saved = [s.get_editor_property("material_interface").get_name()
         for s in mesh.get_editor_property("materials")]
require("M_DragonScalesV6" in saved and "M_DragonWingV6" in saved,
        "verify both v6 materials assigned (got %s)" % saved)
print("V6_ASSIGNED", saved)
print("DRAGON_V6_IMPORT_DONE")
