# goose-deuce.md — the deviations log

Why this file exists: the failure mode is the **silent deviation** — the plan says
one thing, the code does another, and nobody finds out until the gap bites. Work
that follows the plan needs no entry here. Work that departs from it is fine
*exactly when it is recorded*, with its reasoning, at the moment it ships.

## The trigger

> Any departure from what the plan, the task, or a project standard said would
> happen — scope cut, approach change, deferral, unplanned addition, standard
> knowingly broken, step skipped, tool or dependency substituted, or a known
> finding knowingly left unfixed — gets an entry in goose-deuce.md, in the same
> commit that ships it.

Honest accounting includes the negative case: findings you are consciously **not**
fixing this pass are listed, with why, so they read as decisions instead of
oversights. When in doubt, write the entry — thirty seconds of logging beats an
hour of archaeology.

**Not a deviation:** choices the plan never spoke to at that granularity — names,
helper extraction, test-case count. But plan *silence* is not an exemption when
the choice is hard to reverse (an on-disk format, a schema, a dependency, a public
interface): log it `[decision]`.

## When to write

In the same commit as the deviation, tagged to it — `Commit(s): this commit` when
the entry is written before the hash exists. Never batched at the end of a
session: an entry reconstructed later records what you wish you had thought.

A deviation discovered **after** its commit is still logged — never silently
dropped. Date it the day you found it, flag the reconstruction in `Why:`, and cite
the shipping commit: `Commit(s): <hash> (logged late, reconstructed YYYY-MM-DD)`.
Late is worse than timely; unlogged is worse than late.

A deviation that ships no code still gets an entry — a skipped gate, a skipped
ritual step, a skipped sweep, a reverted attempt, an approval boundary crossed.
The entry is then its own bookkeeping commit (`goose-deuce: <title>`), written
when the call is made, `Commit(s): this commit`.

## Entry format

New entries go at the **top** of `## Log`, newest first — "the most recent
entries" always means the top of `## Log`. The fenced block is the format
specimen (`<…>` are field descriptions — never filled in the spec, always filled
in an entry):

```markdown
## YYYY-MM-DD — <short title>  [<tag>]
- Task: <T-### / phase / docs/PLAN.md section that governs this work>
- What: <what was actually done — the deviation itself, stated plainly>
- Why: <the rationale, with its evidence named: the failing test, the missing
  credential, the measured number, the foot-gun found>
- Where: <files / modules / functions touched>
- Plan: <what docs/PLAN.md §… or the TASKS.md item said — name it> → <how it now
  stands>. <TASKS.md corrected in this commit — name the item. docs/PLAN.md
  amended in this commit if the deviation falsifies it, otherwise folded at the
  next sweep — say which.>
- Commit(s): <hash(es), or "this commit">
```

**All six fields are required, in this order — an entry missing any field is not
an entry.** The four questions — what was done, why, where, and what the plan said
→ how it now stands — must be answerable from the entry alone, without opening the
diff; `Task:` and `Commit(s):` are what make it findable later. Titles are unique
within a date (they are the citation key).

## Tags

One per entry — the dominant character of the call:

- `[scope]` — something the plan promised is trimmed or not shipped.
- `[approach]` — the plan's goal met by different means than it prescribed.
- `[deferred]` — work moved to later; the owing TASKS.md item is linked in the entry.
- `[extra]` — unplanned work shipped: requested mid-flight, or forced by a finding.
- `[tech-debt]` — an accepted shortcut with a follow-up owed; TASKS.md item linked.
- `[process]` — a project standard or AGENTS.md rule knowingly broken or changed,
  or a lapse being put on the record (late log, stale claim, skipped step).
- `[decision]` — the plan was followed (or was silent) but the call is worth a
  record: a sanctioned fallback taken, an option evaluated and rejected, a
  hard-to-reverse choice made where the plan said nothing.

Trigger → tag: scope cut `[scope]` · approach change / tool or dependency
substituted `[approach]` · deferral `[deferred]` · unplanned addition `[extra]` ·
known finding left unfixed / shortcut accepted `[tech-debt]` · standard broken or
step skipped `[process]` · sanctioned fallback taken, or a hard-to-reverse call
where the plan was silent `[decision]`. `[deferred]` and `[tech-debt]` are the
tags that owe a TASKS.md item — when choosing between one of those and another
tag, choose the owing one.

## Rules

- **Immutable entries.** Corrections are appended inside the entry as a final
  bullet — `- [Correction, YYYY-MM-DD: …]` — never edited into the field they
  correct. Stale paths and superseded claims stay as written: they are a record of
  what the tree looked like at the time.
- **`this commit` is final.** Never backfill a hash into a committed entry — that
  is a rewrite. Recover the hash with `git log --oneline -- goose-deuce.md`. Real
  hashes appear in `Commit(s):` only for entries about *earlier* commits
  (late-logged, or corrections citing where a fix landed).
- **Owed work lives in TASKS.md, not here.** Every `[deferred]` and `[tech-debt]`
  entry names its TASKS.md item; an entry with no item is a silent drop wearing a
  log entry as a disguise.
- **Re-deferral gets a new entry.** When a deferred item comes due and is pushed
  again, that is a fresh call — log it fresh. It keeps being deferred honestly
  until it ships.
- **Multi-agent collisions** on the top of `## Log` resolve by keeping both
  entries in date order — never by dropping one.
- **The sweep** — at the cadence AGENTS.md → Deviations sets. It is a TASKS.md
  item like any other work. Walk the entries above the previous sweep's marker
  (sweep #1 has no marker: its range starts at the repo's first commit, which is
  also where the weekly clock starts): (1) fold every pending `Plan:` adjustment
  into docs/PLAN.md; (2) verify every `[deferred]` / `[tech-debt]` entry still
  has a live TASKS.md item — restore any orphan and append a `[Correction, …]`
  noting the lapse; (3) close by writing the sweep's own `[process]` entry
  naming the date range covered — that entry is the next sweep's starting
  marker. Full audit checklist: step 5 of
  /Users/auston/Development Projects/Templates/NEW-PROJECT-GUIDE.md.

## Log

<!-- Real entries accumulate below, NEWEST AT THE TOP. The top entries are
     session-start required reading (count: AGENTS.md → Session ritual). -->

## 2026-10-28 — T-017 resequenced ahead of combat/notoriety; eat deferred  [decision]
- Task: T-017 (docs/PLAN.md M1/M2 phasing)
- What: The exterior flight zone shipped before T-015 (combat) and T-016 (notoriety), at the owner's explicit direction ("i want to fly around"). It is greybox (plateau/terraces/valley/village primitives + goat actors) in the same Lair map. Goats are present and wandering but NOT edible, and no Notoriety cost exists yet.
- Why: Owner decision 2026-10-28, mid-session. The flight zone is the owner's current priority; the Meshy world meshes are separately gated on the WORLD-ASSETS approval, which has not been given, so greybox carries the layout.
- Where: Content/Maps/Lair/Lair.umap, Source/UnderTheMountain/Livestock/, Scripts/create_exterior.py, TASKS.md (T-017 accept rewritten).
- Plan: PLAN.md phasing put combat/notoriety (M1) before the exterior (M2) → order swapped by owner instruction; the edible-goat + Notoriety-cost parts of the original T-017 accept now ride on T-015/T-016 respectively. docs/PLAN.md unaffected (phases are delivery order, and this entry records the swap).
- Commit(s): this commit

## 2026-10-28 — Sweep #1 (T-004): range repo start (T-000) through T-033  [process]
- Task: T-004 (AGENTS.md → Deviations cadence; NEW-PROJECT-GUIDE step 5)
- What: First sweep. Docs amended (PLAN architecture names, README status/coverage/controls); adversarial review + stranger test run by independent sub-agents; findings filed as T-035–T-042; successor sweep T-034 filed. This entry is the starting marker for sweep #2's range.
- Why: 19 tasks had closed with no sweep (cadence is 10). Orphan audit: no [deferred]/[tech-debt] orphans — the sole tech-debt entry was resolved by T-022, T-012's owed PIE walk lives on T-030, T-013's Niagara debt was corrected/closed by T-031. Stranger test passed overall; its three ambiguities (claimed task's Gated field left as 'cadence', successor filed at promotion rather than at close, AGENTS.md version line stale at 2026-10-27) are recorded here and in T-042 — the first two are board bookkeeping habits now on record, the third needs an owner-approved AGENTS.md edit (writes are approval-blocked from this harness).
- Where: docs/PLAN.md, README.md, TASKS.md (T-035–T-042, T-034), goose-deuce.md (this entry).
- Plan: AGENTS.md → Deviations said sweep every 10 closed tasks → sweep #1 done at 19 (late — the cadence promotion step was missed at closes #10–#19; this entry is the record). docs/PLAN.md amended in this commit (architecture table). TASKS.md corrected in this commit (findings + successor).
- Commit(s): this commit

## 2026-10-28 — T-013 scent ribbon is cube geometry, not Niagara  [approach]
- Task: T-013 (docs/GAMEPLAY.md section 3 scent rendering)
- What: AScentRibbonActor renders each scent ribbon as a stretched engine cube with the M_ScentGold material (width = value-bracket width), spawned/pooled by UScentComponent. No Niagara system is authored or spawned. The T-key/gamepad true-sight toggle is bound like every other dragon action but was not exercised by a live key press (Slate PressKey did not reach the PIE pawn; two attempts, no log line).
- Why: The registry/math contract (value-only encoding, brackets, carried pulse, 30% dim, cap) is the design pillar and is fully test-proven; a Niagara ribbon asset is visual polish that would have consumed the iteration in asset authoring. Input-driven checks of all bindings are already owed by T-030.
- Where: Source/UnderTheMountain/Scent/, Content/Gameplay/Treasures/ (M_ScentGold), Scripts/create_treasures.py.
- Plan: GAMEPLAY.md section 3 said "One Niagara ribbon per registered treasure" → now: ribbon visuals are cube geometry in M1; Niagara ribbon upgrade is filed as T-031 in this commit. docs/PLAN.md unaffected (it does not prescribe the ribbon renderer).
- Commit(s): this commit
- [Correction, 2026-10-28: resolved by T-031 — NS_ScentRibbon (location-based ribbon emitter, SpawnRate 240/s, 0.6s lifetime, RibbonWidth linked to FScentMath via User.RibbonWidth, ScaleColor alpha linked to User.RibbonOpacity) replaces the cube; AScentRibbonActor sweeps the emitter nose<->treasure. PIE evidence Content/Gameplay/Treasures/niagara_ribbon_pie.png; 7/7 tests.]

## 2026-10-28 — T-012 traversal verified geometrically; PIE walk deferred to T-030  [approach]
- Task: T-012 (lair level; TASKS.md Accept)
- What: Scripted traversal is verified by segment-vs-AABB tests in Scripts/verify_lair.py (14/14, controls live) instead of physics traces, and the PIE check proved level load + dragon pawn spawn at DragonStart rather than an input-driven chamber walk. The 60 fps evidence is a standalone -game CSV capture (last 120 frames avg 16.81 ms at the 60 Hz vsync cap), not a PIE stat capture.
- Why: Probed live in the headless editor: every SystemLibrary trace variant returns None even into a BlockAll cube floor (no physics scene in a commandlet; `unreal.KismetSystemLibrary` does not exist in UE 5.8 Python). In the GUI editor, MCP ActorTools address only the editor world, not PIE actors, so a scripted PIE walk/teleport of the pawn is not available; input-driven feel is already T-030's human/MCP check.
- Where: Scripts/verify_lair.py, Content/Maps/Lair/ (Lair.umap + M_RockDark/M_GoldWarm/M_WoodDark), Saved/Profiling/lair_fps.csv (evidence, gitignored).
- Plan: T-012 Accept said "scripted traversal" + PIE + 60 fps → met by substitution above; the input-driven in-PIE chamber traversal remains owed and is recorded on T-030 in this commit. docs/PLAN.md unaffected.
- Commit(s): this commit

## 2026-10-28 — T-010 dragon rig verdict: Meshy auto-rig unusable, Blender fixup  [decision]
- Task: T-010 (docs/PLAN.md → Risks, Meshy dragon rig gate)
- What: Meshy multi-image-to-3D (tasks 01a181bc…, 30 cr; rig 01a181c9…, 5 cr; balance 1150→1115 approx) produced a clean textured dragon (82,933 tris, GLB/FBX in Content/Meshy/Raw/Dragon/, imported as SK_Dragon_Rigged in Content/Meshy/Dragon/). The auto-rig SUCCEEDED but is a 24-joint HUMANOID biped skeleton (Hips/Spine/Spine01/Spine02, neck/Head, Left/Right Shoulder-Arm-ForeArm-Hand, 2× UpLeg-Leg-Foot-Toe) mapped onto a quadruped: no wing bones, no tail bones, only 2 leg chains for 4 legs. Verdict: rig NOT usable for dragon flight/walk.
- Why: Meshy rigging is humanoid-only (tool schema: 'textured humanoid model'), exactly the foot-gun AGENTS.md records. Mesh itself is good (preview verified gold/crimson, wings/tail complete) but is in a perched display pose on a rock base, which the fixup must also address (remove base, neutral pose).
- Where: Content/Meshy/Raw/Dragon/ (raw, untouchable), Content/Meshy/Dragon/ (SK_Dragon_Rigged + preview evidence), docs/ASSET-PIPELINE.md dragon gate.
- Plan: PLAN.md dragon gate said fail → Blender fixup path with code-driven wing flap → stands unchanged; that is now T-028 with the bone list + preview as failing evidence. docs/PLAN.md unaffected.
- Commit(s): this commit

## 2026-10-27 — T-023 proof path substituted: CLI + protocol, not in-session tools  [process]
- Task: T-023 (unreal-mcp live proof)
- What: The live proof ran via `hermes mcp test unreal-mcp` (✓ Connected, 3 meta-tools) plus a direct MCP protocol call to `list_toolsets` (returned EditorApp, Niagara, PCG, AgentSkill, Logs toolsets), instead of through this session's unreal-mcp tools after `/reload-mcp`.
- Why: `/reload-mcp` did not re-register tools in the running desktop session (owner-reported; the session catalog was fixed at session start and the 18:36 reconcile had dropped portable/native registrations). The server side was never broken: it started on OnPostEngineInit once first-launch shader compilation finished, listening on 127.0.0.1:8000.
- Where: TASKS.md (T-023 close), Hermes desktop session (client-side registration still pending a new session).
- Plan: T-023's Accept named the in-session path → Accept met in substance (Hermes client connects, read-only toolset listing returned), path substituted. In-session tools arrive with the next desktop session; direct protocol calls bridge until then. docs/PLAN.md unaffected.
- Commit(s): this commit

## 2026-10-27 — Art direction: Style B (realism, texture-carried dragon)  [decision]
- Task: T-025 (docs/PLAN.md → Art direction; hackathon submission)
- What: Style B (grounded realism) chosen over Style A (stylised painterly)
  and C (dark gothic). World realism via UE lighting/materials; dragon
  realism carried by texture/normal maps over clean geometry.
- Why: Owner decision 2026-10-27 after mockups (dragon, countryside, flight,
  lair in A and B). B has the wow factor; the perf risk on the M1 Max is
  lighting/resolution (managed: dynamic res, Lumen software RT, 2K cap,
  per-level 60 fps gates), not triangle count. Selection mockups were
  generated on the session image backend (fal), NOT Muse Image — no Meta API
  key exists in the profile; official concepts regenerate via the muse-harness
  plugin (Muse Image + SAM 3.1) in T-025 after restart.
- Where: docs/PLAN.md, docs/ART-DIRECTION.md (T-025), TASKS.md (T-010 now
  depends on T-025), idea.md art line.
- Plan: PLAN.md had no art direction → section added in this commit; grade
  fallback to Style A lighting recorded there if dragon iterations stall.
- Commit(s): this commit

## 2026-10-27 — Coverage floor is an honest 0% until llvm-cov lands  [tech-debt]
- Task: T-002 (AGENTS.md → Project snapshot, coverage floor)
- What: The coverage floor ships as 0% with Scripts/coverage.sh reporting
  "not instrumented" instead of a percentage.
- Why: UE C++ line coverage needs a dedicated UBT/llvm-cov pass that does not
  exist in the T-002 toolchain; inventing a floor number would violate the
  snapshot's "set from the measured number" rule. The measured baseline today
  is: 1 automation test file, 0% instrumented lines.
- Where: Scripts/coverage.sh, Scripts/gate.sh, AGENTS.md snapshot.
- Plan: AGENTS.md said floor is measured, never wished → floor recorded as the
  measured 0% and ratchets at first instrumentation. TASKS.md corrected in
  this commit: T-022 owes the instrumentation. docs/PLAN.md unaffected.
- Commit(s): this commit
- [Correction, 2026-10-28: resolved by T-022 — Scripts/coverage.sh now measures real llvm-cov line coverage (module-only rsp-relink; measured 31.1% product lines), AGENTS.md floor ratcheted 0% → 29%, gate.sh enforces it.]

## 2026-10-27 — Engine and scope: Unreal 5.8.4, full open-world retained  [decision]
- Task: T-001 (docs/PLAN.md phases; hackathon submission 2026-11-04)
- What: Unreal Engine 5.8.4 chosen for the submission over Kaigen3D; the full
  open-world design from idea.md retained as the vision, delivered in phases
  M1–M5 (docs/PLAN.md) instead of cut to a standalone slice.
- Why: Owner decision 2026-10-27 after evaluation. Unreal is installed and
  verified on this Mac (5.8.4 source, editor present) with a proven
  Meshy FBX/GLB import path; Kaigen3D is closed-beta, licence-gated, not
  installed locally, and its asset/animation pipeline is undocumented
  (docs mostly "soon") — an unbounded risk inside 8 remaining days.
  Kaigen remains a post-submission spike candidate. "Metal 4" corrected:
  UE5.8 Mac renders via Metal already; the showcase claim is a native
  Apple Silicon build at 60 fps, not custom renderer work.
- Where: docs/PLAN.md, idea.md, docs/ASSET-PIPELINE.md, AGENTS.md snapshot.
- Plan: no prior plan existed → docs/PLAN.md created in this commit carries
  the phasing; the M1 phase is the hackathon-critical path and M2 absorbs
  slips first. TASKS.md corrected in this commit (M1 backlog T-010+).
- Commit(s): this commit

