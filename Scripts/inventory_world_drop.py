#!/usr/bin/env python3
# [2026-11-04 T-114] inventory_world_drop.py — editor-free inventory of the
# [2026-11-04 T-114] owner drop in Content/Meshy/World (goat liv-* + foliage
# [2026-11-04 T-114] nat_*). Counts files/formats/bytes per asset stem, flags
# [2026-11-04 T-114] stems with no FBX (not importable) and no texture, and
# [2026-11-04 T-114] never reads Content/Meshy/Raw (never hand-edited).
# Usage: python3 Scripts/inventory_world_drop.py [--root DIR] [--json OUT]
import argparse
import collections
import json
import os
import sys

PREFIXES = ("liv-", "nat-")


def stem_of(name: str) -> str:
    base = os.path.splitext(name)[0]
    # [2026-11-04 T-114] Evidence frames/renders of an asset (walk-cycle
    # [2026-11-04 T-114] stills, turntables, contact sheets) inventory under
    # [2026-11-04 T-114] their asset, not as separate stems.
    for marker in ("_walk_f", "_walk_contact", "_walk_cycle", "_turntable"):
        if marker in base:
            base = base.split(marker)[0]
    for suffix in ("_base_color", "_basecolor", "_preview", "_fbm", "_Anim", "_Skeleton"):
        if suffix in base:
            return base.split(suffix)[0]
    return base


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap.add_argument("--json", default="")
    args = ap.parse_args()
    world = os.path.join(args.root, "Content", "Meshy", "World")
    if not os.path.isdir(world):
        print(f"WORLD_INVENTORY BLOCKED no dir {world}")
        return 1
    files = [f for f in sorted(os.listdir(world))
             if f.startswith(PREFIXES) and os.path.isfile(os.path.join(world, f))]
    by_ext = collections.Counter(os.path.splitext(f)[1].lower() or "<none>" for f in files)
    by_stem = collections.defaultdict(list)
    total_bytes = 0
    for f in files:
        by_stem[stem_of(f)].append(f)
        total_bytes += os.path.getsize(os.path.join(world, f))
    goat = {s: v for s, v in by_stem.items() if s.startswith("liv-")}
    nat = {s: v for s, v in by_stem.items() if s.startswith("nat-")}
    print(f"WORLD_INVENTORY dir={world}")
    print(f"WORLD_INVENTORY files={len(files)} bytes={total_bytes} stems={len(by_stem)} "
          f"goat_stems={len(goat)} nat_stems={len(nat)}")
    print("WORLD_INVENTORY formats " + " ".join(f"{k}={v}" for k, v in sorted(by_ext.items())))
    for label, group in (("GOAT", goat), ("NAT", nat)):
        for stem in sorted(group):
            exts = sorted({os.path.splitext(f)[1].lower() for f in group[stem]})
            flags = []
            if ".fbx" not in exts:
                flags.append("NO-FBX")
            if not any(e in exts for e in (".png", ".jpg")):
                flags.append("NO-TEXTURE-FILE")
            print(f"WORLD_INVENTORY {label} {stem} files={len(group[stem])} exts={','.join(exts)} "
                  f"{' '.join(flags)}".rstrip())
    if args.json:
        payload = {"world": world, "files": len(files), "bytes": total_bytes,
                   "formats": dict(by_ext),
                   "stems": {s: sorted(v) for s, v in sorted(by_stem.items())}}
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(payload, fh, indent=2, sort_keys=True)
        print(f"WORLD_INVENTORY json={args.json}")
    print("WORLD_INVENTORY RESULT pass (read-only; import/placement are editor lanes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
