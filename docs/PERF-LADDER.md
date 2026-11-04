# Performance ladder (T-115) — how the fps-per-preset proof runs

> [2026-11-04 T-115] Tooling only in this lane: no fps numbers are claimed
> here. Numbers exist only when a real `-game` run on this Mac prints
> `PROOF fps` lines (GameMode, 10 s warmup skip) and
> `Scripts/proof_fps_ladder.py parse` reads them back.

## Run matrix

```bash
python3 Scripts/proof_fps_ladder.py plan            # prints 4 presets x lair/village
python3 Scripts/proof_fps_ladder.py plan --preset High
```

Presets map to UE scalability levels until T-128 names its own:
Low=0, Medium=1, High=2, Ultra=3, applied across all `sg.*` groups
(incl. GlobalIllumination/Reflection/Shadow). Each run needs
`utm.proof.shotdelay` > 10 s (default plan: 25 s) or the GameMode reports
`insufficient-window` instead of a number — the parser treats that as
missing, never as 0 fps.

## Parse + verdict

```bash
python3 Scripts/proof_fps_ladder.py parse \
  --run High:lair:Saved/Profiling/fps_high_lair.log \
  --run High:village:Saved/Profiling/fps_high_village.log # ... every preset
```

Output names each preset/scene `AT-OR-ABOVE-60` or `below-60`, with avg,
frame count, avg/worst ms and the at-60 bucket share. Exit 1 while any
preset/scene is missing or insufficient-window.

## Close conditions for T-115 (unchanged, not met by this doc)

- [ ] Lair + village measured at every T-128 preset on this Mac (parser RESULT complete)
- [ ] Preset(s) at/above 60 fps avg named from parsed logs
- [ ] No visual regression at the shipped default without owner sign-off (captures reviewed)
- [ ] LaunchServices launch watch (T-020's 2 raw-exec SIGSEGVs) confirmed absent or fixed
- [ ] docs/PLAN.md M1 exit-gate sentence amended to the measured ladder
