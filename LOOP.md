# LOOP.md — protocol for `/loop` and autonomous sessions

AGENTS.md rules all apply, unabridged. One iteration = one full pass of
AGENTS.md → The task loop (all ten steps, PICK→NEXT) over one TASKS.md item —
never breadth-first, never a shortened loop. Deviations land in goose-deuce.md
per its rules, in the same commit, exactly as in an interactive session.

What a loop adds on top:

1. **Autonomy guard** — PICK skips `Gated: owner` and `Gated: hardware` items
   unless the loop was pointed at one; a loop never answers its own escalations.
2. **Verification** — fast inner loop while iterating: Scripts/test-fast.sh.
   Before commit, the full gate exactly as AGENTS.md → Project snapshot writes
   it, foot-gun flags included. Red = fix or revert; never end an iteration red.
3. **Push** — loops never push product commits. Not with prior approval, not
   once, not ever: pushing is an interactive-session action under AGENTS.md →
   Version control, and blanket authorization given at launch does not survive
   into iterations. The one carve-out is AGENTS.md's own: in a multi-agent repo,
   the standing-approved claim/board bookkeeping pushes still apply — or the
   loop's claims guard nothing.
4. **Budget** — subagents per AGENTS.md → Sub-agents and budget. Don't leave an
   item half-open at context exhaustion: finish it, or park it per AGENTS.md →
   Blockers, questions, escalation before the window closes.
5. **Pace** — schedule the next wakeup only after the tree is green and
   committed.
6. **Stop** — if two consecutive iterations yield no committable work
   (everything eligible needs the owner or hardware), park the loop: report the
   board state and the smallest owner decision that unblocks it (AGENTS.md →
   The task loop → PICK), and end. A stuck loop parks itself instead of
   spinning.
7. **Untouchables** — Content/Meshy/Raw/ import sources, the pinned EngineAssociation in the .uproject, generated Binaries/Intermediate folders.
8. **Route by size, not by default (owner rule, 2026-11-01)** — delegate to
   `muse_turn` only when the deliverable is large and self-contained
   (>~200 new lines, multi-file draft, bulk tests, survey/doc, asset batch)
   AND the brief fits on one page with exact signatures and file lists.
   One call per deliverable, implementation + its tests together, output
   files first. Everything else — single file pair, bug fixes, anything
   needing compile-error iteration, editor/PIE work — the parent does
   directly; a compiler error is never bounced back to Muse, it is fixed
   locally. Rationale: Hermes cuts `muse_turn` at ~420s while the plugin's
   own deadline is longer, so an oversized chunk dies as an orphaned
   `muse serve` (T-060, 2026-11-01: ~10% of a 5-hour subscription bucket
   bought tests-only and an uncompilable tree; the parent finished it
   in-session). On any `muse_turn` timeout: `git status` + check for a
   still-running `muse serve` and the live log before anything else —
   files on disk are the result, the return text is not — and never
   re-run the same brief blind. Muse is the drafter, never the finisher:
   every delegated chunk gets the parent's diff read, build, and gate
   before the next chunk. Stay in one session across iterations (prompt
   cache is the cheap path); do not start a fresh session per task.
