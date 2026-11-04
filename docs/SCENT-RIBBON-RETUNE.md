# Scent ribbon retune (T-127) — code values + Niagara authoring

> [2026-11-04 T-127] Code side landed editor-free: `FScentMath` now computes
> density/sheen from gold value (value-only, no-information rule intact),
> `AScentRibbonActor` pushes them, and the end-on white-bar (T-111) is faded
> code-side. The Niagara asset still has to define the user variables below
> or it will ignore the pushed values — that half is blocked-on-editor.

## Code values (proven by `UnderTheMountain.Scent.RibbonRetune`, not yet run — no UE build in this lane)

| Cue | Trinket (10g) | Crown (1200g) | Rule |
|---|---|---|---|
| Width | ~3.9 cm | ~33 cm | existing brackets, unchanged (value ranks by width first) |
| `User.RibbonDensity` | ~0.95→clamped 1.0 | ~2.4 | `clamp(0.75 + width/20, 1.0, 2.5)` |
| `User.RibbonSheen` | ~0.34 | ~1.0 | `clamp(ln(1+gold)/ln(1001), 0, 1)` |
| `User.RibbonSheenColor` | (0.70, 0.52, 0.17) approx | (1.00, 0.78, 0.28) | lerp tarnished→metallic gold by sheen |

## Niagara authoring owed in NS_ScentRibbon (editor lane)

1. Add user parameters: `RibbonDensity` (float, default 1.0), `RibbonSheen`
   (float, default 0.0), `RibbonSheenColor` (LinearColor, default gold).
2. Multiply emitter SpawnRate by `RibbonDensity` (density retune; record the
   base spawn rate it multiplies — do not bake density into width).
3. Particle colour = mix(existing tarnished gold, `RibbonSheenColor`,
   `RibbonSheen`) and add a metallic/specular lift scaled by `RibbonSheen`
   (exact material route is the editor author's call; record it here after).
4. Keep `User.RibbonWidth` / `User.RibbonOpacity` as-is; the code-side
   end-on fade already multiplies effective opacity in
   `AScentRibbonActor::Tick` (1.0 broadside → 0.25 end-on).

## Still owed to close T-127

- [ ] Niagara variables above authored + saved
- [ ] -game captures at gameplay distance in lair + exterior light reviewed
- [ ] fps cost of density ≤2.5× recorded (rides the T-115 ladder tooling)
