# [2026-10-28 T-015] Author M_FireBreath: translucent orange emissive
# material for the pawn's fire cone mesh.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def require(value, action):
    if not value:
        raise RuntimeError("create_combat_assets failed: %s" % action)
    return value


DIR = "/Game/Dragon"
path = DIR + "/M_FireBreath"
if eal.does_asset_exist(path):
    require(eal.delete_asset(path), "delete existing %s" % path)
mat = require(
    asset_tools.create_asset("M_FireBreath", DIR, unreal.Material, unreal.MaterialFactoryNew()),
    "create M_FireBreath",
)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -100)
c.set_editor_property("constant", unreal.LinearColor(1.0, 0.42, 0.08, 1.0))
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
o = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 100)
o.set_editor_property("r", 0.35)
mel.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(mat)
require(eal.save_asset(path), "save M_FireBreath")
print("FIRE_MATERIAL_DONE", path)
