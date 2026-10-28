# [2026-10-28 T-027] Import the Meshy V4 textures and build M_DragonScales:
# base/normal/rough/metal from the V4 maps, final colour = base x ScaleTint
# except where the base is crimson-dominant (wing membranes/spines), which
# the picker must not recolour (T-027 accept). Assigns to SK_Dragon_v4 slot 0
# (previously WorldGridMaterial — the mesh shipped untextured).
import os

import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    # [2026-10-28 T-037] Surface Unreal's False/None helper failures
    # as a named exception before destructive work or success text.
    if not value:
        raise RuntimeError("create_dragon_material failed: %s" % action)
    return value


SRC = unreal.Paths.project_dir() + "Content/Meshy/Raw/Dragon/dragon_v4_textures/"
DST = "/Game/Meshy/Dragon/Textures"

def import_tex(png, name, srgb):
    dest = DST + "/" + name
    if eal.does_asset_exist(dest):
        return require(eal.load_asset(dest), "load existing texture %s" % dest)
    source = SRC + png
    # Validate the source before any material is deleted/rebuilt.
    require(os.path.isfile(source), "find source texture %s" % source)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", DST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    tex = require(eal.load_asset(dest), "load imported texture %s" % dest)
    tex.set_editor_property("srgb", srgb)
    if not srgb and "Normal" in name:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    require(eal.save_asset(dest), "save texture %s" % dest)
    return tex

T_BASE = import_tex("base_color.png", "T_Dragon_BaseColor", True)
T_NORM = import_tex("normal.png", "T_Dragon_Normal", False)
T_ROUGH = import_tex("roughness.png", "T_Dragon_Roughness", False)
T_METAL = import_tex("metallic.png", "T_Dragon_Metallic", False)
print("TEXTURES", T_BASE, T_NORM, T_ROUGH, T_METAL)

MAT_PATH = "/Game/Meshy/Dragon/M_DragonScales"
if eal.does_asset_exist(MAT_PATH):
    require(eal.delete_asset(MAT_PATH), "delete existing material %s" % MAT_PATH)
mat = require(
    asset_tools.create_asset("M_DragonScales", "/Game/Meshy/Dragon", unreal.Material, unreal.MaterialFactoryNew()),
    "create material %s" % MAT_PATH,
)
mat.set_editor_property("used_with_skeletal_mesh", True)

def sample(tex, x, y, sampler):
    s = require(
        mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y),
        "create texture sample expression",
    )
    s.set_editor_property("texture", tex)
    s.set_editor_property("sampler_type", sampler)
    return s

COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
LINEAR = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR

base = sample(T_BASE, -900, -300, COLOR)
norm = sample(T_NORM, -900, 100, NORMAL)
rough = sample(T_ROUGH, -900, 350, LINEAR)
metal = sample(T_METAL, -900, 550, LINEAR)

tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, -450)
tint.set_editor_property("parameter_name", "ScaleTint")
tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))

tinted = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -550, -300)
mel.connect_material_expressions(base, "RGB", tinted, "A")
mel.connect_material_expressions(tint, "", tinted, "B")

# crimson mask: saturate((R - max(G,B)) * 4) — membranes/spines stay base.
def mask_chan(tex_sample, chan, x, y):
    m = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x, y)
    m.set_editor_property("r", chan == "R")
    m.set_editor_property("g", chan == "G")
    m.set_editor_property("b", chan == "B")
    m.set_editor_property("a", False)
    mel.connect_material_expressions(tex_sample, "RGB", m, "")
    return m

r_ch = mask_chan(base, "R", -700, -100)
g_ch = mask_chan(base, "G", -700, 0)
b_ch = mask_chan(base, "B", -700, 100)
maxgb = mel.create_material_expression(mat, unreal.MaterialExpressionMax, -550, 0)
mel.connect_material_expressions(g_ch, "", maxgb, "A")
mel.connect_material_expressions(b_ch, "", maxgb, "B")
dom = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -420, -80)
mel.connect_material_expressions(r_ch, "", dom, "A")
mel.connect_material_expressions(maxgb, "", dom, "B")
four = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -420, 40)
four.set_editor_property("r", 4.0)
domx = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -40)
mel.connect_material_expressions(dom, "", domx, "A")
mel.connect_material_expressions(four, "", domx, "B")
sat = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -200, -40)
mel.connect_material_expressions(domx, "", sat, "")

final = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -80, -250)
mel.connect_material_expressions(tinted, "", final, "A")
mel.connect_material_expressions(base, "RGB", final, "B")
mel.connect_material_expressions(sat, "", final, "Alpha")

mel.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(norm, "RGB", unreal.MaterialProperty.MP_NORMAL)
mel.connect_material_property(rough, "R", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(metal, "R", unreal.MaterialProperty.MP_METALLIC)
mel.recompile_material(mat)
require(eal.save_asset(MAT_PATH), "save material %s" % MAT_PATH)
print("MATERIAL", MAT_PATH)

sk = require(eal.load_asset("/Game/Meshy/Dragon/SK_Dragon_v4"), "load SK_Dragon_v4")
# [2026-10-28 T-027] Whole-list replacement: mutating the fetched
# SkeletalMaterial struct in place does not write back (proven —
# slot stayed WorldGridMaterial on the first attempt).
sm = unreal.SkeletalMaterial()
sm.set_editor_property("material_interface", mat)
sk.set_editor_property("materials", [sm])
require(eal.save_asset("/Game/Meshy/Dragon/SK_Dragon_v4"), "save SK_Dragon_v4")
assigned = sk.get_editor_property("materials")[0].get_editor_property("material_interface").get_name()
require(assigned == "M_DragonScales", "verify SK_Dragon_v4 material assignment (got %s)" % assigned)
print("ASSIGNED slot0 ->", assigned)
print("DRAGON_MATERIAL_DONE")
