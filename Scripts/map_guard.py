# [2026-11-03 T-106] Shared re-run guard for the content scripts
# (create_lair / exterior / sky / npcs / treasures / thieves /
# burnables): every script destroys actors before spawning, so every
# script (1) copies the on-disk .umap to Saved/MapBackups/ before
# touching anything (ground truth via os.path/shutil — never the
# headless registry, which lies both ways), (2) logs a pre-destroy
# census per owned label prefix, and (3) asserts post-spawn label
# uniqueness before saving, so a re-run can neither lose work silently
# nor persist duplicates (a failed assert raises: no save happens, the
# in-memory map is discarded on exit). Import preamble, same in every
# script (project_dir is pure path logic, safe headless):
#   import os as _os
#   import sys as _sys
#   _sys.path.insert(0, _os.path.join(unreal.Paths.project_dir(), "Scripts"))
#   import map_guard
import os
import shutil

import unreal


def _repo_file(*parts):
    return os.path.join(unreal.Paths.project_dir(), *parts)


def backup_level(tag):
    """Copy the last-saved .umap aside BEFORE load/destroy.

    One file per tag, overwritten each run (bounded disk use); restore
    is a manual copy-back, and the log line says exactly how. Returns
    the backup path, or None on first creation (no prior map)."""
    src = _repo_file("Content", "Maps", "Lair", "Lair.umap")
    if not os.path.exists(src):
        print("MAP_GUARD backup skipped (no prior map):", tag)
        return None
    dst_dir = _repo_file("Saved", "MapBackups")
    os.makedirs(dst_dir, exist_ok=True)
    dst = os.path.join(dst_dir, "Lair_pre_%s.umap" % tag)
    shutil.copyfile(src, dst)
    print("MAP_GUARD backup: %s (restore: copy back over Content/Maps/Lair/Lair.umap)" % dst)
    return dst


def log_census(eas, prefixes):
    """Pre-destroy census: how many actors per owned prefix are about to go."""
    counts = {p: 0 for p in prefixes}
    for a in list(eas.get_all_level_actors()):
        label = a.get_actor_label()
        for p in prefixes:
            if label.startswith(p):
                counts[p] += 1
                break
    print("MAP_GUARD pre-destroy census:", " ".join("%s=%d" % (p, counts[p]) for p in prefixes))
    return counts


def assert_unique_labels(eas, prefixes):
    """Post-spawn, pre-save: no duplicate labels within our prefixes.

    Raises instead of saving, so a mis-ordered re-run can never persist
    duplicates (the failure is loud, the on-disk map untouched)."""
    seen = set()
    dupes = []
    for a in list(eas.get_all_level_actors()):
        label = a.get_actor_label()
        if any(label.startswith(p) for p in prefixes):
            if label in seen:
                dupes.append(label)
            seen.add(label)
    if dupes:
        raise RuntimeError("map_guard refusing save: duplicate labels %s" % sorted(set(dupes)))
    print("MAP_GUARD uniqueness ok (%d owned actors)" % len(seen))
