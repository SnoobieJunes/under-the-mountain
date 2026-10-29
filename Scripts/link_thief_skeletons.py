# [2026-10-28 T-044] Link each thief's walk/run anim skeletons as
# CompatibleSkeletons of its rigged-mesh skeleton. Meshy's rig and
# animation downloads create separate Skeleton assets for the same
# 24-joint hierarchy; without this link UE refuses to play the clips
# on the rigged mesh.
import unreal

eal = unreal.EditorAssetLibrary


def require(value, action):
    if not value:
        raise RuntimeError("link_thief_skeletons failed: %s" % action)
    return value


for slug in ("burglar", "adventurer", "dwarf_carrier", "dwarf_guard", "dwarf_trapper"):
    base = "/Game/Meshy/Thieves/%s_rigged_Skeleton" % slug
    rigged = eal.load_asset(base)
    if not rigged:
        print("SKIP missing", base)
        continue
    compat = list(rigged.get_editor_property("compatible_skeletons"))
    for kind in ("walking", "running"):
        other = eal.load_asset("/Game/Meshy/Thieves/%s_%s_Skeleton" % (slug, kind))
        if other and other not in compat:
            compat.append(other)
    rigged.set_editor_property("compatible_skeletons", compat)
    require(eal.save_asset(base), "save skeleton %s" % base)
    print("LINKED", slug, [c.get_name() for c in compat])
print("THIEF_SKELETONS_DONE")
