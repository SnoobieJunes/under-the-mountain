# Art Direction — Style B (Grounded Realism)

**Date:** 2026-10-27 · **Task:** T-025 · **Status:** Concept pack APPROVED by owner 2026-10-27 (gold/crimson dragon revision); SAM 3.1 / Meshy steps are next (T-010)
**Concepts:** [`docs/concepts/`](concepts/) (10 images, Muse Image `muse-image-1.0`, direct Meta API)

---

## 1. Style B definition (owner-chosen)

- **Grounded realism.** A realistic world, lit cinematically. Nothing painterly, cartooned, or stylised.
- The dragon reads as a real animal: credibility comes from **surface** (texture, normal detail,
  weathering, wear) laid over a **clean, simple silhouette** — never from exaggerated proportions,
  spikes everywhere, or glowing effects.
- **Muted, earthy palette** throughout: stone, worn leather, tarnished gold. The dragon is the
  colour hero: **burnished gold scales with deep crimson wing membranes and dorsal crest**.
  Other saturated accents stay functional: amber eyes, hoard glow, seeing-stone's pale teal.
- **2K mindset:** author and ship textures at 2K max (2048 px on the long edge per mesh part).
  The concept images themselves are 1920×1280 (or 1120×2240 portrait) — already in that envelope.
  No 4K hero textures; spend the budget on normal/roughness detail, not resolution.
- No text, watermarks, logos, or UI in any concept or derived asset art.

**Canonical dragon descriptor** (used verbatim in every dragon prompt, and to be reused verbatim
in any future generation, texture brief, or Meshy prompt so the animal stays the *same* animal):

> a massive grounded-realism dragon, burnished gold textured scales, deep crimson wing membranes and crimson dorsal crest spikes, weathered horns, amber eyes, long tail, four legs

## 2. Palette (hex approximations sampled from the concept pack)

Dominant colours below were sampled from the generated PNGs (median-cut quantisation +
accent-pixel medians, 2026-10-27). Treat them as **targets ± a shade**, not exact albedo —
bake final albedo from lit renders, not from these concepts directly.

| Role | Hex (approx.) | Source concept |
|---|---|---|
| Dragon scale — gold shadow | `#4A3418` approx. | dragon-side, dragon-head |
| Dragon scale — burnished gold mid/highlight | `#AF8658` – `#C49A58` approx. | dragon-side, dragon-three-quarter |
| Dragon wing membrane / dorsal crest — crimson | `#6E1423` – `#8A1F2D` approx. | dragon-side, dragon-three-quarter |
| Dragon eye — amber accent | `#A8773A` (median), up to `#BA8635` in the iris | dragon-three-quarter, dragon-head |
| Neutral concept background (segmentation-friendly) | `#AEABA4` – `#B3B0AC` | dragon-side, thief sheets |
| Cavern stone — deep | `#070705` – `#1F1910` | lair-interior |
| Cavern stone — lit | `#322A1F` – `#524432` | lair-interior |
| Hoard gold — mid | `#AF8658` – `#B98B50` | hoard-artifacts, lair-interior |
| Village fields / golden-hour earth | `#6A5944`, `#9F876B`, gold fields `#AB8963` | countryside-village |
| Village shadow / roof slate | `#221F16` – `#383731` | countryside-village |
| Thief wardrobe — worn leather & cloth | `#1C150F` – `#41362C`, braids/straps `#584F45` | thief-burglar, thief-adventurer, thief-dwarf-party |
| Seeing-stone — the one cool accent | `#75ADA3` (pale teal, desaturated) | hoard-artifacts |

Rules that follow from the swatches:

- Saturation stays low everywhere except: amber eyes, lamplight/hoard glow, seeing-stone, and
  gemstones (emerald in the crown/pendant). If everything glows, nothing glows.
- Gold is **tarnished** gold (`#AF8658` family), never saturated yellow `#FFD700`.
- Greens are moss/field greens pulled toward brown; no bright fantasy green. The dragon's
  canonical scheme is gold/crimson only (a player-facing scale-colour picker is filed as
  T-027 — implementation must tint from this gold base, not repaint it).

## 3. Lighting rules

- **Cinematic, motivated light only.** Every bright area has a visible source: low sun,
  light-shafts through cavern fissures, lanterns, or the hoard's own reflected glow.
- **Exteriors:** golden hour default — low warm key, long shadows, mist in the valley
  (see `dragon-three-quarter`, `countryside-village`). Overcast/neutral is reserved for
  reference sheets, never for in-game hero shots.
- **Lair interiors:** deep shadow base (`#070705` family) with warm pools of light; the hoard
  mound is the brightest object in its chamber. Keep a 3-chamber depth read — foreground dark,
  mid-ground hoard glow, background chamber barely lit (see `lair-interior`).
- **Reference/model sheets:** flat, even, neutral-grey background (`#AEABA4` family) so the
  silhouette segments cleanly later. No rim-light tricks that eat the outline.
- Contrast carries mood, not colour grading: no teal-orange blockbuster LUT, no bloom-heavy
  fantasy glow. If it could be a film still, it's right; if it could be a book cover painting, it's wrong.

## 4. Texture & material rules

- **The dragon's realism is texture-carried.** Model the silhouette clean and comparatively
  smooth; put the detail in maps:
  - Albedo: burnished-gold scale variation, crimson membrane/crest, dirt/moss in creases, worn edges on horns and claws.
  - Normal (the workhorse): individual scale relief, cracked horn grain, wing-membrane veining
    and small tears. This map sells the animal at 2K — author it at 2K, don't upscale a 1K bake.
  - Roughness: matte scale tops, slightly glossier eye and wet mouth line, dusty wing membrane.
  - No emissive on the dragon body. The amber eye may take a *slight* emissive lift only if it
    dies in shadow in-engine; try without first.
- **Silhouette budget:** horns, jaw teeth, claws, dorsal spines, and wing fingers are geometry;
  everything smaller is normal map. If a detail can't be seen at gameplay distance, it is not geometry.
- **Humans/dwarves:** weathered, repaired, muddy. Leather is scuffed (`#41362C` family),
  chainmail is dull, cloth is patched. No pristine costume-shop armour, no heraldic bright colours.
  Faces are real faces (pores, scars, dirt), per `thief-burglar` / `thief-adventurer` / `thief-dwarf-party`.
- **Environments:** stone, slate, timber, and earth with photographed-looking wear — moss on the
  shady side, water-darkened rock at stream level, soot above hearths. Trim sheets and tileables
  at 2K; unique hero props (crown, seeing-stone, pendant) get their own 1K–2K sets.
- **Hoard:** coins/goblets are a *mass* material (instanced/scattered, tarnished gold, high
  metalness, varied roughness); only the named artifacts in `hoard-artifacts` are hero meshes.

## 5. Pipeline: concept → SAM 3.1 cutout → Meshy (after owner approval)

`dragon-side.png` is the designated source for image-to-3D: full-body side profile on a plain
neutral background (1920×1280, entire tail in frame — verified 2026-10-27). **Owner approved
the pack 2026-10-27**; the steps below are unblocked (they run under T-010).

1. **Segment with SAM 3.1 (segmentation only — it does not generate images).**
   Direct Meta API, same `MODEL_API_KEY` bearer auth as generation:
   - `POST https://api.meta.ai/v1/responses`
   - Model: `sam-3.1`
   - Input: an `input_image` part carrying `dragon-side.png`, with text prompt **`dragon`**
   - Output: segmentation mask for the prompted object.
2. **Cut out:** apply the mask to produce a clean dragon cutout on a transparent (or flat plain)
   background. Check the mask edge along the wing membrane, horns, and tail spines — those are
   the thin structures segmentation most often loses. Save as e.g. `docs/concepts/dragon-side-cutout.png`.
3. **Meshy image-to-3D:** feed the cutout (not the raw concept) to Meshy. The clean silhouette
   is what makes this viable — background, shadow, or extra props in frame will poison the mesh.
4. Texture/material pass then follows §4: keep Meshy's base silhouette, re-author/repair the
   normal and roughness detail at 2K; do not accept a blurry generated texture as final.

## 6. File inventory — `docs/concepts/` (generated 2026-10-27, T-025)

All files generated via Muse Image (`muse-image-1.0`, direct `api.meta.ai/v1/responses` API),
decoded from WebP and converted to PNG. All 10 succeeded on the first attempt; no retries needed.
Dragon images (1–4) all used the verbatim canonical descriptor in §1.

| # | File | Size | Dimensions | Purpose |
|---|---|---|---|---|
| 1 | `dragon-side.png` | 1,948,342 B (1.9 MB) | 1920×1280 | Full-body side view, plain bg — **SAM 3.1 / Meshy source** (see §5) |
| 2 | `dragon-front.png` | 3,135,875 B (3.1 MB) | 1280×1920 | Front view, plain bg — modelling reference (symmetry, chest plate, stance) |
| 3 | `dragon-three-quarter.png` | 3,619,188 B (3.6 MB) | 1920×1280 | Hero 3/4 over mountain valley — key art, scale/lighting target |
| 4 | `dragon-head.png` | 3,272,293 B (3.3 MB) | 1920×1280 | Head/neck close-up, plain bg — scale, horn, eye, teeth texture target |
| 5 | `lair-interior.png` | 3,597,443 B (3.4 MB) | 1920×1280 | Cavern lair, 3-chamber depth, hoard glow — environment/lighting target |
| 6 | `countryside-village.png` | 3,589,940 B (3.4 MB) | 1920×1280 | Village below the mountain, golden hour — exterior world target |
| 7 | `thief-burglar.png` | 1,576,632 B (1.5 MB) | 1120×2240 | Human burglar, full body, plain bg — character reference |
| 8 | `thief-dwarf-party.png` | 2,575,083 B (2.5 MB) | 1920×1280 | Dwarf raiding party ×3, full body, plain bg — character reference |
| 9 | `thief-adventurer.png` | 2,066,918 B (2.0 MB) | 1120×2240 | Armed human adventurer, full body, plain bg — character reference |
| 10 | `hoard-artifacts.png` | 3,227,239 B (3.1 MB) | 1920×1280 | Crown, seeing-stone orb, pendant, rings, coins — hero-prop target |

Total: 10 files, ~28 MB on disk. Generator script was run from `/tmp` (not committed to the repo).

**Next step:** owner approved 2026-10-27 → SAM 3.1 segmentation per §5 under T-010 → Meshy image-to-3D.
