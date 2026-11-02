# [2026-11-01 T-052] Sky for the exterior: SkyAtmosphere, volumetric
# clouds and height fog in Lair.umap, and the existing PeakSun /
# PeakSkyLight configured to work with them (shadows, atmosphere
# link, real-time capture). Weather STATES (clear/rain light, fog
# and rain-particle changes) are runtime glue on the dragon pawn -
# see ADragonCharacter::ApplyWeather; this script only places and
# configures the actors it reads.
# Re-runnable: Sky_* actors are destroyed and rebuilt; PeakSun and
# PeakSkyLight (create_lair.py) are reconfigured in place.
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
eal = unreal.EditorAssetLibrary


def require(value, action):
    if not value:
        raise RuntimeError("create_sky failed: %s" % action)
    return value


# Cold-registry warm-up for -run=pythonscript (see T-050).
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Maps"], True, True)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."),
        "verify editor world is %s" % MAP_PATH)

for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("Sky_"):
        require(eas.destroy_actor(a), "destroy prior sky actor %s" % a.get_actor_label())


def find_label(label):
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


def try_set(obj, prop, value):
    # Optional atmosphere/cloud properties vary by class; a missing
    # one is a printed skip, never a silent swallow or a hard fail.
    try:
        obj.set_editor_property(prop, value)
        print("SKY_SET", obj.get_name(), prop, value)
    except Exception as exc:  # noqa: BLE001 - reported, not hidden
        print("SKY_SKIP", obj.get_name(), prop, "(%s)" % exc)


# --- atmosphere + clouds + fog, centred over the lair/exterior ---
atmo = require(
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(19000, 0, 0)),
    "spawn SkyAtmosphere")
atmo.set_actor_label("Sky_Atmosphere")

clouds = require(
    eas.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(19000, 0, 0)),
    "spawn VolumetricCloud")
clouds.set_actor_label("Sky_Clouds")
cloud_comp = clouds.get_component_by_class(unreal.VolumetricCloudComponent)
if cloud_comp:
    # Layer heights on this component are kilometres: 0.10-0.35 km
    # puts a cloud deck 100-350 m up - above the terrain peaks
    # (~50 m) and low enough to fly through from cruising altitude.
    try_set(cloud_comp, "layer_bottom_altitude", 0.12)
    try_set(cloud_comp, "layer_height", 0.25)

fog = require(
    eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(19000, 0, 0)),
    "spawn ExponentialHeightFog")
fog.set_actor_label("Sky_Fog")
fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
if fog_comp:
    # Clear-state defaults; ADragonCharacter::ApplyWeather owns the
    # rain values at runtime. Density is per-cm exponential fog.
    try_set(fog_comp, "fog_density", 0.0006)
    try_set(fog_comp, "fog_height_falloff", 0.0004)

# --- configure the existing sun + sky light (create_lair.py) ---
sun = find_label("PeakSun")
require(sun, "PeakSun exists (create_lair.py must run first)")
# Keyword Rotator args: positional assigns as (roll, pitch, yaw) in
# this Python build (the T-050 finding).
sun.set_actor_rotation(unreal.Rotator(pitch=-52.0, yaw=-35.0, roll=0.0), False)
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
try_set(sun_comp, "intensity", 3.0)
try_set(sun_comp, "cast_shadows", True)
try_set(sun_comp, "atmosphere_sun_light", True)

skylight = find_label("PeakSkyLight")
require(skylight, "PeakSkyLight exists (create_lair.py must run first)")
sky_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
try_set(sky_comp, "intensity", 1.0)
try_set(sky_comp, "real_time_capture", True)

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("SKY_DONE")
