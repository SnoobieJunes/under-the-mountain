# ASSET-PIPELINE.md — Meshy → Unreal (and Blender fallback)

Historical/procedural — PLAN.md is normative for what ships; this file is the
how for assets. Written 2026-10-27 (T-001); updated 2026-10-31 with the proven
dragon verdict, credit history, and remaining-spend forecast.
[2026-11-03: dragon Meshy-app flow suspended — owner supplies the newest
dragon directly (T-113); this doc's dragon sections get rewritten from the
T-113 procedure at its close. Non-dragon flows below stand.]

## The pipeline (every game asset)

1. **Design image first.** Generate the concept with Muse Image (direct Meta
   API / Muse image tooling; **0 Meshy credits**) from the task's prompt.
   Owner approves the image before any 3D. Do not route concept iteration
   through paid Meshy image generation unless the owner explicitly approves it
   for a named asset.
2. **3D generation.** `meshy_image_to_3d` from the approved image (textured),
   or `meshy_multi_image_to_3d` when multi-view exists. Direct
   `meshy_text_to_3d` is the fallback for props where silhouette is enough.
   Default to Smart Topology at the asset's triangle budget so a separate paid
   remesh is normally unnecessary.
3. **Remesh/optimise locally first.** Blender/UE optimisation is the default.
   Paid `meshy_remesh` only when local reduction would damage the silhouette or
   UVs: characters ≤ 30k tris, props ≤ 5k, treasure items ≤ 2k. Hero dragon is
   the exception (≤ 80k, tuned in M1).
4. **Rig + animation (characters only).** Humanoids use `meshy_rig` (its free
   walk/run outputs are saved with the asset); extra `meshy_animate` actions
   only when a preset demonstrably beats local authoring. Quadrupeds and the
   winged dragon use Blender for rig, skin, and animation — proved below.
5. **Download — always save locally, same session (owner rule, 2026-10-28).**
   Every Meshy output is saved to this machine before the session ends —
   never leave an asset only behind a signed Meshy link (links expire ~3 days
   after the task finishes; saved files are permanent). `meshy_download_model`
   → `Content/Meshy/Raw/<asset>/` (FBX for rigged characters, GLB for static
   props), plus everything else the task produced that a link could lose:
   the preview/thumbnail rendering (`<asset>_preview.png`), base-color
   textures, and for rig tasks the free walk/run animation files (withSkin
   GLB + FBX and armature GLB, fetched from the rigging task's URLs — rig
   tasks are not saved by `meshy_download_model` itself). Verify every saved
   file exists with non-zero size, and name the saved paths in the report.
   Raw files are untouchable once committed (AGENTS.md → LOOP untouchables)
   — re-download instead of editing.
6. **Import to Unreal** via unreal-mcp / editor import: FBX → Skeletal Mesh
   (characters) or Static Mesh (props), GLB via Interchange. Imported assets
   live beside Raw as `.uasset`; materials rebuilt as project materials,
   never shipped as raw Meshy material instances.
7. **Verify in PIE.** Screenshot + a 10-second PIE run; poly/texture check
   against the budget above. The importing task's Accept names this step.

## The dragon gate — verdict (T-010, proven 2026-10-28)

Repeat the proven hero pipeline exactly:

**Muse multi-image concepts → Meshy multi-image-to-3D mesh + PBR textures →
Blender quadruped rig/skin/animation → Unreal.**

Meshy produced the amazing dragon surface: clean textured mesh (82,933 tris at
the gate) from the approved multi-view concepts. Meshy did **not** produce the
shipped rig or dragon animations. Its 5-credit auto-rig returned a 24-joint
humanoid biped with no wing or tail bones and only two leg chains for four
legs, so it was rejected. Blender then did the work that makes the asset a
dragon in motion: the 33-bone quadruped/wing/tail rig, skin weights, the
24-frame walk, and the programmatic Fly clip. This division is both the better
product and the cheaper one: Meshy rig/animate/motion cannot currently express
this body plan at any credit price, while Blender animation costs 0 Meshy
credits and is already proved on the shipped Walk/Fly. "Zero Meshy animation
budget for the dragon" therefore means **zero for Meshy rig/animate/motion
only** — never zero for Meshy hero meshes/textures. A replacement dragon mesh
generation remains allowed as an owner-approved contingency if rendered rig
evidence ever proves the current mesh cannot hold the required poses.

## How Muse works the editor

Hermes hosts `unreal-mcp` (HTTP, `127.0.0.1:8000`, enable in `hermes mcp list`).
Flow per unreal-mcp skill: `list_toolsets` → `describe_toolset` → `call_tool`.
Used for: level/actor placement, Blueprint/Niagara/Material creation,
Live Coding recompiles after C++ edits, PIE control. Rules: save before and
after bulk edits, never mutate during PIE, treat any non-success return as a
stop. C++ itself is written as files in `Source/` (reviewable diffs); MCP is
for editor state, not a code-delivery channel.

## Incoming friend music (owner update 2026-10-31)

Music is **not generated**. A friend of the owner is composing the tracks;
they will arrive before hackathon completion and are in scope for T-071. When
they land:

- Put the delivered masters, untouched, in `Content/Audio/Raw/Music/` (WAV/FLAC
  preferred). Never edit a master in place.
- Working/imported loops live in `Content/Audio/Music/`; SFX in
  `Content/Audio/SFX/`; ambience in `Content/Audio/Ambience/`; shared
  SoundClasses/MetaSounds beside them under `Content/Audio/`.
- Record composer credit, permission/licence, intended placement, and loop
  points in the T-071 evidence (and the in-game credits when they exist).
- Make any loudness/loop trim on a working copy only, and name the source
  master it came from. The audio seam must accept late-arriving tracks by
  data/config swap, not code rewrite.

## Credits discipline

Quality is the purpose of the spend; waste is not. The rules are:

1. Check `meshy_check_balance` before and after every batch and record task IDs
   + balance movement in the asset report. Log batches >100 credits in
   goose-deuce.md as `[decision]`.
2. Batch by approved concept sheet. One generation per approved asset; fix in
   Blender/UE first. A paid retry needs a named failure (broken silhouette,
   fused limbs/props, unusable UVs) and counts against contingency.
3. Smart Topology (15 credits in the proven batches) is the default for props,
   buildings, NPC bodies, and animals. Reserve premium multi-image/latest
   generation (~30 credits) for hero close-ups such as the dragon and Seeing
   Stone.
4. Generate **unique bases**, then derive variants locally: scale, materials,
   tint, wear, attachments, and layout are Blender/UE work at 0 Meshy credits.
5. Never spend the last 100 credits before the packaged build except on an
   owner-approved hero-asset rescue. That reserve is for one failed hero asset,
   not routine variety.
6. No Meshy spend for code-shaped work: terrain, sky/weather, Niagara, UI,
   dragon/quadruped rigs and animation, goat rig, collision, LOD policy, or
   music/SFX mixing.

### Proven spend to date (receipts in repo docs)

| Batch | Proven cost | Evidence |
|---|---:|---|
| Dragon gate: multi-image-to-3D + rejected auto-rig | 35 | `goose-deuce.md` 2026-10-28 T-010; balance ~1150 → ~1115 |
| Later dragon iterations/other work before thieves | ~105 (balance-reconciled, unitemised) | Thief report opens at balance 1010 |
| Five thief bodies: 15 generation + 5 rig each | 100 | `docs/THIEF-ASSETS.md`; balance 1010 → 910 |
| World pack: 27 Smart Topology models ×15 + goat rig 5 | 410 | `docs/WORLD-ASSETS.md` §5; balance 910 → **500** |
| **Total reconciled from 1150** | **650** | Last proven balance: **500 credits (2026-10-28)** |

The current session's Meshy MCP has no API key configured, so today's balance
could not be re-read on 2026-10-31. Treat 500 as the last documented balance,
not a live guarantee; verify in Meshy before upgrading.

### Remaining forecast to finish the scoped hackathon game

Assumes Muse concept images (0 Meshy credits), Smart Topology at 15/model,
humanoid rig at 5/model, Blender for quadruped rigs/animation, and heavy reuse
of the 27-model world pack already on disk. Prices are the proven batch rates;
recheck the live tool quote before spending.

| Remaining work | Efficient estimate | Quality-safe allowance |
|---|---:|---:|
| Artifacts (T-081): Seeing Stone hero (30) + 7 unique wearable bases ×15; Flame Ring + Thief-Glow derived locally | 135 | 165 (all unique) |
| Bulk treasure (T-082): 6 unique bases ×15, variants by material/scale | 90 | 120 (8 bases) |
| Villagers (T-053): 4 bodies ×(15+5 rig), walk/run included with rig | 80 | 120 (6 bodies) |
| Intro gnome (T-018): 1 body ×(15+5 rig) | 20 | 20 |
| Wildlife (T-090): 3 species meshes ×15; Blender rigs | 45 | 60 (4 species) |
| Extra nature depth for the sprawling world: 3 foliage/rock variants ×15 | 0 (reuse only) | 45 |
| Optional shared humanoid preset actions if Blender quality fails | 0 | 6 |
| **Subtotal before retries** | **370** | **536** |
| Retry/failure contingency | 80 | 90 |
| **Forecast total** | **≈450 credits** | **≈625 credits** |

Not in this forecast: post-hackathon M3/M4-only generations (dragonslayers,
army, rival dragons, elemental-ring variants), any owner-approved replacement
hero dragon mesh (+30 per premium generation), or paid Meshy concept images
(policy above forbids them by default).

**Upgrade guidance:** if the live balance is still 500, the efficient path
(~450) technically fits but spends the 100-credit reserve — too tight for the
quality bar. Hold at least **550 available credits** for the efficient path
with the reserve intact (top up ~+50), or **725 available credits** for the
quality-safe path (~625 spend + ~100 reserve; top up ~+225). Do not buy a
larger tier until the live balance is confirmed; if it already shows ≥725, no
upgrade is needed for this forecast.

## Sprint landings — 2026-11-03 (T-100)

- Dragon v6 (parallel flight-overhaul work): Content/Meshy/Dragon/dragon_v6_experiment.blend (39-bone rig: 7 wing bones/side with finger fan) + dragon-wings-v6/exports/Dragon_v6.fbx + 6 4K textures (Body/Wing x BaseColor/Normal/ORM) + 13 review renders; Scripts/create_dragon_anims.py retargeted (v6 default, v5 fallback). UE import + material hookup NOT yet run — owned by T-068 remainder. Render review noted two membrane defects (glide wingtip edge, hover down-beat spike) to clear in the UE pass.
- Foliage: nat-broadleaf, nat-bush, nat-grass (FBX+GLB+basecolor+previews each) in Content/Meshy/World/; placement owned by the T-049/T-050 world chain.
- Goat: no updated files landed this sprint (all goat sources predate 2026-11-02) — re-supply or confirm before the livestock pass.
