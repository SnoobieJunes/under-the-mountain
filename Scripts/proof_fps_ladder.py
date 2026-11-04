#!/usr/bin/env python3
# [2026-11-04 T-115] proof_fps_ladder.py — fps ladder tooling per quality
# [2026-11-04 T-115] preset (T-115 Accept: lair + village at every T-128
# [2026-11-04 T-115] preset, PROOF fps lines, buckets, worst frame; presets
# [2026-11-04 T-115] at/above 60 fps named). Editor-free: `plan` prints the
# [2026-11-04 T-115] run matrix, `parse` reads PROOF lines out of captured
# [2026-11-04 T-115] -game stdout/log files. It NEVER invents a number: a
# [2026-11-04 T-115] preset/scene with no PROOF line is reported MISSING.
# Preset -> sg.* levels below are the UE scalability ladder (0=Low..3=Epic);
# [2026-11-04 T-115] when T-128 names its presets, alias them here.
import argparse
import re
import sys

PRESETS = {
    "Low": 0, "Medium": 1, "High": 2, "Ultra": 3,
}
SCENES = ("lair", "village")
SG_GROUPS = (
    "sg.ResolutionQuality", "sg.ViewDistanceQuality", "sg.AntiAliasingQuality",
    "sg.ShadowQuality", "sg.GlobalIlluminationQuality", "sg.ReflectionQuality",
    "sg.PostProcessQuality", "sg.TextureQuality", "sg.EffectsQuality",
    "sg.FoliageQuality", "sg.ShadingQuality",
)
FPS_RE = re.compile(r"PROOF fps avg=([\d.]+) frames=(\d+) avgms=([\d.]+) worstms=([\d.]+)")
BUCKET_RE = re.compile(r"PROOF fpsbuckets at60=(\d+) at30=(\d+) slow=(\d+)")


def sg_fragment(level: int) -> str:
    return ", ".join(f"{g} {level}" for g in SG_GROUPS)


def cmd_plan(a) -> int:
    for preset, level in PRESETS.items():
        if a.preset and preset != a.preset:
            continue
        for scene in SCENES:
            spawn = a.lair_spawn if scene == "lair" else a.village_spawn
            print(f"# {preset} / {scene}: sg level {level}; shotdelay must exceed the 10 s warmup")
            print(f"FPS_LADDER_RUN preset={preset} scene={scene} "
                  f"Scripts/capture_dragon.sh fps_{preset.lower()}_{scene} \"{spawn}\" {a.yaw} {a.shotdelay} "
                  f"# plus ExecCmds: {sg_fragment(level)}  (capture stdout to Saved/Profiling/fps_{preset.lower()}_{scene}.log)")
    print("FPS_LADDER_PLAN note: capture_dragon.sh does not forward sg.* today; "
          "append them to its -ExecCmds in the run lane or launch -game manually with the same ExecCmds.")
    return 0


def parse_file(path):
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return None
    fps = FPS_RE.findall(text)
    if "PROOF fps insufficient-window" in text or not fps:
        return {"insufficient": True}
    avg, frames, avgms, worst = fps[-1]
    buckets = BUCKET_RE.findall(text)
    at60 = at30 = slow = None
    if buckets:
        at60, at30, slow = (int(x) for x in buckets[-1])
    return {"avg": float(avg), "frames": int(frames), "avgms": float(avgms),
            "worstms": float(worst), "at60": at60, "at30": at30, "slow": slow}


def cmd_parse(a) -> int:
    runs = {}
    for spec in a.run or []:
        parts = spec.split(":", 2)
        if len(parts) != 3:
            print(f"FPS_LADDER FAIL bad --run {spec!r} (want PRESET:SCENE:LOGFILE)")
            return 2
        preset, scene, path = parts
        runs[(preset, scene)] = parse_file(path)
    missing = []
    print("FPS_LADDER preset scene avg_fps frames avg_ms worst_ms at60% verdict")
    for preset in PRESETS:
        if a.preset and preset != a.preset:
            continue
        for scene in SCENES:
            r = runs.get((preset, scene))
            if r is None:
                missing.append(f"{preset}/{scene}")
                print(f"FPS_LADDER {preset} {scene} MISSING (no log supplied)")
            elif r.get("insufficient"):
                missing.append(f"{preset}/{scene}")
                print(f"FPS_LADDER {preset} {scene} INSUFFICIENT-WINDOW (shot fired before warmup end)")
            else:
                total = (r["at60"] or 0) + (r["at30"] or 0) + (r["slow"] or 0)
                pct = f"{100.0 * r['at60'] / total:.1f}" if total and r["at60"] is not None else "-"
                verdict = "AT-OR-ABOVE-60" if r["avg"] >= 60.0 else "below-60"
                print(f"FPS_LADDER {preset} {scene} avg={r['avg']:.1f} frames={r['frames']} "
                      f"avgms={r['avgms']:.2f} worstms={r['worstms']:.2f} at60%={pct} {verdict}")
    if missing:
        print("FPS_LADDER RESULT incomplete missing=" + ",".join(missing))
        return 1
    print("FPS_LADDER RESULT complete (numbers are parsed from supplied logs, not generated)")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("plan")
    p.add_argument("--preset", default="")
    p.add_argument("--lair-spawn", default="X=0 Y=0 Z=200")
    p.add_argument("--village-spawn", default="X=50000 Y=0 Z=200")
    p.add_argument("--yaw", default="0")
    p.add_argument("--shotdelay", default="25")
    p.set_defaults(fn=cmd_plan)
    q = sub.add_parser("parse")
    q.add_argument("--preset", default="")
    q.add_argument("--run", action="append", help="PRESET:SCENE:LOGFILE (repeatable)")
    q.set_defaults(fn=cmd_parse)
    a = ap.parse_args()
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
