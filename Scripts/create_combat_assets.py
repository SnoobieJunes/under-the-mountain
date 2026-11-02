# [2026-10-28 T-015] Originally authored M_FireBreath, the material
# for the pawn's fire cone mesh.
# [2026-11-01 T-051] The cone is gone; fire is NS_FireBreath, a
# Niagara system. Niagara stack authoring is not possible from
# Unreal Python (no NiagaraEditorLibrary, no emitter methods on
# UNiagaraSystem), so NS_FireBreath was authored through the
# editor's Niagara MCP toolset (NiagaraToolset_System) in a live
# editor, and this file is now the provenance record + verifier.
# To recreate or retune it, open the editor (Scripts/run-editor.sh)
# and apply, via NiagaraToolset_System:
#   CreateNiagaraSystem  NS_FireBreath in /Game/Dragon from
#     template /Niagara/DefaultAssets/Templates/Systems/DirectionalBurst
#   RemoveEmitter        LocationBasedRibbon (ribbon not wanted)
#   Emitter DirectionalBurst:
#     RemoveModule SpawnBurst_Instantaneous; AddModule SpawnRate
#     SpawnRate = 240/s; Life Cycle Mode stays System (the emitter
#       lives exactly while the pawn's Niagara component is active)
#     InitializeParticle: Lifetime 0.35-0.75 s, Color (1, 0.42, 0.08),
#       Sprite Size (45,45)-(95,95)
#     AddVelocityInCone: Cone Angle 16, Cone Axis (1,0,0) local,
#       Velocity Strength random range 2200-3000 cm/s (reaches the
#       1400 cm damage range inside the 0.35-0.75 s lifetime)
#     GravityForce: Gravity (0,0,0) - the stream flies straight;
#       Drag and ScaleColor stay at template values (fade-out)
#     Renderer: the template's NiagaraSpriteRendererProperties with
#       DefaultSpriteMaterial (color carries the fire read)
# The pawn (ADragonCharacter) activates this system while
# UDragonCombatComponent::IsFiring and adds a flickering point
# light; utm.fire.forcevisuals=1 holds the visuals for captures.
# Running this script verifies the asset exists and loads.
import unreal

eal = unreal.EditorAssetLibrary


def require(value, action):
    if not value:
        raise RuntimeError("create_combat_assets failed: %s" % action)
    return value


# Cold-registry warm-up for -run=pythonscript, as in the other
# content scripts (T-050).
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Dragon"], True, True)

path = "/Game/Dragon/NS_FireBreath"
require(eal.does_asset_exist(path), "NS_FireBreath missing at %s - author it per the header" % path)
system = require(eal.load_asset(path), "load %s" % path)
print("FIRE_BREATH_VERIFIED", path, system.get_class().get_name())
