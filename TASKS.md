# TASKS.md — Under the Mountain work queue
<!-- One queue. New DONE entries at the top of Done. Tasks without observable
     acceptance criteria are not pickable — sharpen them first. -->

## Now
- [ ] T-001 — Plan, backlog, and living docs
      Why: docs/PLAN.md is the normative plan every later task measures against.
      Accept: docs/PLAN.md written (full open-world vision, phased M1–M5, non-goals); README expanded; CHANGELOG entries for T-000/T-001; phase-1 backlog below complete with Why/Accept/Touches/Depends/Gated on every task.
      Touches: docs/PLAN.md, README.md, CHANGELOG.md, TASKS.md, idea.md   Depends: T-000   Gated: none
- [ ] T-002 — Toolchain and the test gate
      Why: one pinned UE/Xcode toolchain + a runnable gate before product code.
      Accept: Scripts/setup.sh, run-editor.sh, test-fast.sh, gate.sh, coverage.sh, build.sh all run as written in AGENTS.md → Project snapshot; no `TBD step 3` markers remain; cold-clone gate green.
      Touches: Scripts/, UnderTheMountain.uproject, Source/, Config/, AGENTS.md   Depends: T-001   Gated: none
- [ ] T-003 — Walking skeleton: dragon in an empty lair, playable in PIE
      Why: smallest end-to-end slice proving the architecture (input → dragon pawn → camera → flight) on real hardware.
      Accept: PIE on this Mac: dragon pawn spawns in a greybox lair, walks, flaps/dives with stamina drain, gamepad + keyboard; automation smoke test passes via Scripts/gate.sh.
      Touches: Source/UnderTheMountain/, Content/Maps/, Config/   Depends: T-002   Gated: none

## Later
- [ ] T-004 — goose-deuce sweep
      Why: standing audit cadence (AGENTS.md → Deviations).
      Accept: NEW-PROJECT-GUIDE step-5 checklist run; [process] marker entry filed; successor sweep task filed.
      Touches: goose-deuce.md, docs/PLAN.md, TASKS.md   Depends: none   Gated: cadence

## Blocked

## Dropped

## Done
- [x] T-000 — Scaffold Under the Mountain from templates (2026-10-27, this commit)
