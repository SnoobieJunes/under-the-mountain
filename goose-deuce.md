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

## 2026-11-01 — T-068 shipped as flight-core slice; ABP graph + hero clips deferred  [scope]
- Task: T-068 (docs/ANIMATION-SPIKE.md v1 set; T-058 closed by the same work)
- What: Owner picked up "T-059" for a Meshy/Blender animation test; T-059 was already closed on stretch-scope, so under owner direction the work became T-068 in an isolated worktree (muse/t068-flight-anim, stretch docs folded in). Shipped slice: UDragonAnimInstance C++ parent (all decisions as tested statics), 8 Blender-authored/imported clips (Idle, Fly v2, Glide, Hover, Takeoff, Land, Roar, HitLight), per-state pawn driver with flap curves + mesh pitch/bank + one-shot stand-in for montages, wing buffet + tail slam as full verbs. NOT shipped from T-068's Accept: the ABP_Dragon AnimBP graph itself, Hit heavy / Defeat / Sleep/Wake / fire-aim / claw / bite clips, and in-engine PIE captures (Blender contact sheets only). Zero Meshy credits spent — Meshy rig/animation remains biped-only per the spike, so Blender was both the capable and the free route.
- Why: One session cannot honestly close a 7-10-session task; the flight core is the owner-visible win (T-058's complaint) and every deferred item now has its seam built (AnimInstance variables, one-shot call sites, combat verbs). Closing T-068 whole would have been a false attestation.
- Where: Source/UnderTheMountain/{Public,Private}/Dragon/DragonAnimInstance.*, DragonCharacter.*, DragonCombatComponent.*, Thief/ThiefBrain.* (Dazed), Tests/DragonAnimTest.cpp + DragonFlightTest.cpp, Scripts/create_dragon_anims.py + import_dragon_anims.py, Content/Meshy/Dragon/ (8 FBX + 8 uassets + t068_*.png), TASKS.md (T-068 narrowed, T-058/T-059/T-067 closed).
- Plan: T-068's Accept named the full v1 set + ABP graph → the task stays open with the remainder as its Accept (owed work on the board, per the rules); docs/PLAN.md is not falsified (it does not sequence clips inside T-068) and folds at the next sweep. Import foot-gun found en route: a CDO-referenced AnimSequence cannot be delete/rename-replaced from a commandlet — import_dragon_anims.py's header records the workaround (free the path before launch).
- Commit(s): this commit

## 2026-11-01 — Intro wake-up ships without the T-068 Sleep/Wake clips  [decision]
- Task: T-018 (docs/GAMEPLAY.md intro branches; T-068 animation set)
- What: The intro sequence shipped with a restrained camera move (boom 240→700 over the wake) and no skeletal Sleep/Wake animation, because T-068 (the reusable Sleep/Wake clip set the decision names) is owner-gated and not yet done - no sleep or wake clip exists in the project. The sequence states are named and timed so T-068's clips drop into the WakeUp state without restructuring (the pawn comment at the camera ease says so).
- Why: Blocking the whole intro (state machine, gnome, choice UI, branch accounting - all proven this session) on an owner-gated animation task would leave the first-60-seconds experience unbuilt for the sake of one clip; the decision's intent (a real hero impression, not an idle-pose cheat) is carried by the camera move until the clips land.
- Where: Source/UnderTheMountain/Private/Dragon/DragonCharacter.cpp (TickIntro WakeUp camera ease), Source/UnderTheMountain/Public/Intro + Private/Intro, TASKS.md T-018.
- Plan: T-018's Decision said wake-up uses T-068's set → the sequence honours the state design and defers only the clip; T-068 remains the owner-gated home for the animation, and wiring it in is a named follow-up in T-018's closing note. docs/PLAN.md is not falsified (it does not schedule the clip inside T-018); folded at the next sweep.
- Commit(s): this commit

## 2026-11-01 — Terrain built by C++ editor commandlet, not an Unreal Python script  [decision]
- Task: T-049 (docs/PLAN.md exterior/world scope)
- What: The huge-world terrain generator shipped as a C++ editor commandlet (UTerrainCommandlet in the new Source/UnderTheMountainEditor editor module, driven by Scripts/create_world_terrain.py as a launcher) instead of an unreal-python script like every other content builder in Scripts/. The project also gained the UnderTheMountainEditor module that AGENTS.md's layout section already described but which did not exist.
- Why: Unreal Python cannot create a Landscape, proven three ways on 2026-11-01: spawning ALandscape inside a -run=pythonscript commandlet asserts (!IsRunningCommandlet); GUI-editor Python receives a LandscapePlaceholder with zero components; and there is no Python-callable component creation or heightmap-file import. ALandscapeProxy::Import (the import wizard's entry point) is C++-only. An editor-only UCLASS inside the runtime module was also tried and rejected: UHT emits its reflection code for the game target too, breaking that build - hence the separate editor module.
- Where: Source/UnderTheMountainEditor/ (new module: Build.cs, module entry, TerrainCommandlet.h/.cpp), Source/UnderTheMountainEditor.Target.cs, UnderTheMountain.uproject (module registered; EngineAssociation untouched), Scripts/create_world_terrain.py (launcher), Content/Maps/Lair/Lair.umap (Terrain_World landscape).
- Plan: T-049's Touches named Scripts/create_world_terrain.py as the vehicle → the file exists and is the documented entry point, but it launches the C++ commandlet rather than running unreal-python. TASKS.md T-049 corrected in this commit. docs/PLAN.md makes no statement about the terrain vehicle, so it is not falsified and is folded at the next sweep.
- Commit(s): this commit

## 2026-10-31 — Owner review: sandbox guarantee, friend music, Meshy budget guardrails  [decision]
- Task: T-016, T-018, T-021, T-059, T-067–T-071, T-075, T-081/T-082, T-088, T-090 (docs/PLAN.md pillars; docs/GAMEPLAY.md §1; docs/ASSET-PIPELINE.md)
- What: Four owner corrections were applied to the documents. (1) The game is explicitly a sandbox: PT-LOOP and waves are benchmark/pressure systems, never rails; the player may ignore objectives and record an unscripted "go rogue" demo while theft, witnesses, Notoriety, food pressure, and win/lose keep responding. (2) Music is friend-composed and arriving before the deadline; it must not be generated from scratch. T-071 now owns ingestion (masters untouched in Content/Audio/Raw/Music/, working loops in Content/Audio/Music/, credit/permission recorded) plus SFX/ambience. (3) The dragon pipeline wording was corrected everywhere it could be misread: Meshy supplied the hero mesh/textures, Blender supplied the shipped quadruped rig/skin/Walk/Fly; only Meshy dragon rig/animation spend is zero. (4) A Meshy credit forecast and hard batch rules now live in docs/ASSET-PIPELINE.md: proven spend reconciles 1150 → 500, remaining forecast is ~450 efficient / ~625 quality-safe plus a 100-credit reserve. The owner's large README rewrite was reviewed and kept (not dropped): reverting would resurrect the false HEAD status ("thief AI parked, combat to come"); only its stale status/task-range/dragon-source/audio lines were patched.
- Why: Owner review in session, 2026-10-31: finite budget ("I don't have a large budget and I don't want this to land unfinished"), explicit sandbox/demo requirement, friend-music delivery, and a challenge to the "Meshy budget zero" phrasing. Evidence: docs/THIEF-ASSETS.md opens at balance 1010 and closes at 910 (−100); docs/WORLD-ASSETS.md §5 closes at 500 (−410); the dragon gate entry below records the rejected 24-joint Meshy biped at 5 credits.
- Where: docs/GAMEPLAY.md §1, docs/PLAN.md (sandbox principle + Meshy budget risk), docs/ASSET-PIPELINE.md (dragon verdict, friend-music ingestion, credit forecast/rules), docs/ANIMATION-SPIKE.md §C, docs/FLIGHT-SPIKE.md (unchanged on credits beyond the decision record), README.md (kept + minimal stale-line patches), TASKS.md (T-016/T-018/T-021/T-059/T-071 wording), CHANGELOG.md, this entry. No product code, no assets purchased, no music files present yet.
- Plan: GAMEPLAY.md's minute table previously read as a scripted session and the audio task implied fully sourced/generated audio → the sandbox guarantee and friend-music ingestion rule now stand in GAMEPLAY.md/PLAN.md/TASKS.md in this commit. No scope cut; the Meshy forecast is a spend ceiling with a 100-credit reserve, not permission to generate beyond the listed line items.
- Commit(s): this commit

## 2026-10-31 — Flight/animation spike decisions adopted under owner delegation  [decision]
- Task: T-075 + T-067 (docs/PLAN.md pillars 3–4; docs/GAMEPLAY.md §2)
- What: The owner annotated docs/FLIGHT-SPIKE.md and docs/ANIMATION-SPIKE.md (including several explicit answers and several "idk" responses), then instructed the agent to use best judgement on the rest. Adopted: hybrid momentum/precision flight with dive-as-posture, Brake, Landing Assist, one continuous stamina pool in 20-point impulse reserves, no stamina regeneration from speed/skimming, adopted KB+M/gamepad starting maps, starting speed/band values, zero-stamina glide/land mercy, and Settings → Infinite Stamina testing mode. Animation adopts the lean ABP_Dragon architecture, the documented v1 clip set including reusable Sleep/Wake, full combat verbs, CC0-only external animation, conditional wing weight-paint, batched review gates, and family-specific death presentation (dragon authored, thieves authored→ragdoll, goats authored).
- Why: Owner feedback/questions on the spike documents plus the owner's explicit instruction to decide the rest by best judgement. The notable rejected import from WoW is stamina regeneration at speed/skim: Vigor is an arcade permission resource, while our stamina is bodily energy in a food-survival loop; only physical ground effect (less drag/sink) is adopted.
- Where: docs/FLIGHT-SPIKE.md, docs/ANIMATION-SPIKE.md, docs/GAMEPLAY.md §2, docs/PLAN.md (spike decisions section), TASKS.md (T-075/T-067 and cross-referenced T-058/T-059/T-061/T-062/T-063/T-064/T-065/T-066/T-068/T-074/T-076/T-079/T-084/T-088/T-054/T-018), CHANGELOG.md, this entry. No product code or assets changed; no scope cut.
- Plan: docs/PLAN.md pillar 3 already described dive-for-speed/glide-to-travel, but docs/GAMEPLAY.md priced glide at −1/s, charged flap per beat, and left the flight architecture open → GAMEPLAY.md §2 is amended in this commit to the adopted target model (all new values tagged `[tuning: PT-STAM]`), docs/PLAN.md is amended in this commit, and TASKS.md is corrected in this commit. These are prototype decisions, not proven feel: T-075's PIE course and the animation render gates remain the falsifiers.
- Commit(s): this commit

## 2026-10-31 — Flight and animation spikes researched in parallel  [process]
- Task: T-075 + T-067 (AGENTS.md task loop; docs/PLAN.md pillars 3–4)
- What: The two spike research/design passes were run in parallel at the owner's explicit instruction, instead of claiming, completing, and closing one task before starting the next. Neither task is marked DONE: docs/FLIGHT-SPIKE.md and docs/ANIMATION-SPIKE.md now carry the evidence and recommendations, while T-075 still owes playable PIE prototypes and T-067 still depends on T-059 plus the owner's v1 pick.
- Why: Owner direction in session: "pull forward flight controls and animations spikes. Do those in parallel". The subjects are coupled (movement states determine wingbeat/aim/bank animation), so parallel evidence gathering was materially useful; no product code or assets were changed.
- Where: docs/FLIGHT-SPIKE.md, docs/ANIMATION-SPIKE.md, TASKS.md (T-075/T-067 pulled to the top of Now with findings; T-058/T-062/T-068 cross-references), docs/PLAN.md (spike-findings section), CHANGELOG.md, this entry.
- Plan: AGENTS.md says one task is completed and closed before the next is picked → this owner-directed research pass departs from that sequence, but the one-task rule resumes for prototype/implementation work and neither spike was falsely closed. docs/PLAN.md amended in this commit with the findings and remaining gates. TASKS.md corrected in this commit (T-075, T-067, T-058, T-062, T-068).
- Commit(s): this commit

## 2026-10-31 — Audit gaps filed at full scope; alpha-cut proposal rejected by owner  [decision]
- Task: T-075–T-092 (docs/PLAN.md M1–M2; repo-wide audit 2026-10-31)
- What: An audit of code vs docs/PLAN.md + docs/GAMEPLAY.md proved 18 items neither built nor on any task (flight spike; claw/crush; fire-vs-goats; flight collision damage; magic pool; full artifact set; bulk treasure; XP; sleep; tribute; hoard mound; save; settings; navigation; wildlife; water; day/night; ground idle folded into T-067). They are filed as T-075–T-092 with two board amendments (T-062 now depends on the T-075 spike pick; T-067's clip list gains ground idle). On the record: the audit also proposed cutting eight scoped tasks (T-049, T-050, T-052, T-053, T-055, T-066, T-069/T-070, T-018) from an "alpha" — the owner rejected any scope reduction explicitly ("the game is not viable without an open world and all of the things scoped"). No task was cut, moved, or re-scoped; the proposal never touched the board.
- Why: Owner direction 2026-10-31, in session. Evidence for each gap is code-level (named in each task's Why: e.g. TickFire accepts only thief brains while AGoatActor's damage path has no production caller; thief steering bakes in chamber-centre X constants; Stats has no magic pool while §3 charges one).
- Where: TASKS.md (T-075–T-092, T-062/T-067 amendments), docs/PLAN.md (2026-10-31 addendum), CHANGELOG.md (filed line), this entry. No product code.
- Plan: PLAN.md + GAMEPLAY.md promised systems (sleep/tribute, full artifacts, XP display, §5.4 claw/crush) that had no TASKS.md home, and were silent on saves/settings/navigation/water/time → the board now carries all of it; GAMEPLAY.md's phase tags remain delivery order, per the owner's no-cut ruling. docs/PLAN.md amended in this commit (addendum). TASKS.md corrected in this commit (T-075–T-092).
- Commit(s): this commit

## 2026-10-29 — Owner playtest reopens flight/lair scope; four unplanned demo gaps approved  [decision]
- Task: T-057–T-074 (docs/PLAN.md M1; owner Path A playtest 2026-10-29)
- What: The owner's first hands-on playtest produced 11 findings, filed (not started) as T-057–T-070: the standalone binary launch crashes (SIGSEGV, descriptor, ICU — T-057), the shipped flight animation does not read as flight (wings never extend, legs never tuck — T-058, reopening T-043/T-047's "proven" claim at the feel level), flight controls/cameras/dive are reopened (T-062–T-065, incl. RMB free-look + double-click steer, clarified by the owner the same day), the lair's spatial design is replaced (chamber chain → mountain with side caves and an open crown — T-066), plus tint picker (T-060), scent hotkey (T-061), two report-only animation spikes (T-059, T-067 → T-068), and lock-on + treasure pickup (T-069, T-070). Separately, an agent repo review proposed and the owner approved four additions the plan never named: audio (T-071), game-flow shell (T-072), onboarding/help (T-073), enemies that can damage the dragon (T-074).
- Why: Owner direction, 2026-10-29, twice in one session (11 items, then approval of the 4 + F6 clarification). Evidence: the crash log pasted into T-057; repo review found zero audio assets/components, zero game-flow code, no in-game control teaching (F7 is the proof), and no enemy→dragon damage path (only landings/exhaustion cost HP).
- Where: TASKS.md (T-057–T-074, T-063 clarification, T-068 cross-ref), docs/PLAN.md (addendum), CHANGELOG.md (two filed lines), this entry. No product code.
- Plan: PLAN.md M1 presumed flight feel, the lair layout, and HP pressure were on track and said nothing about audio, shell, or onboarding → now amended by the 2026-10-29 addendum in this commit: flight feel and lair layout are reopened (T-012/T-043 stand as shipped-but-superseded designs; their closes stay as written — supersession lives in T-058/T-062–T-066), and audio/shell/onboarding/enemy-damage are M1 scope. M1's exit gate is unchanged. Deep dive one at a time, owner-gated, per the owner's instruction.
- Commit(s): this commit

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

