# Content/Audio — [2026-11-04 T-071]

Audio seam: `UAudioDirectorComponent` (`Source/UnderTheMountain/Public/Audio/`).
Verbs call the seam; no verb calls PlaySound directly. Assign assets to the
director's slots editor-side; unassigned slots log once and stay silent.

## Slots (data-driven, editor-assigned)
- Music: `TitleTrack`, `LairTrack`, `ExteriorTrack`, `CombatTrack`
- SFX cues (`SfxCues` map): SFX_DragonRoar, SFX_DragonFire, SFX_DragonEat,
  SFX_DragonLand, SFX_DragonFlap, SFX_WingBuffet, SFX_TailSlam,
  SFX_DragonTakeoff, SFX_DragonClaw, SFX_DragonCrush
- Ambience: lair vs exterior, follows the music state.
- Master volume: `SetMasterVolume` (T-088 Settings hook), clamped 0–1.

Expected layout once assets land (do not commit binaries without licence):
`Raw/Music/` untouched masters · `Music/` working loops · `SFX/` · `Ambience/`

## Music credit (owner-supplied 2026-11-03, verbatim)
Original compositions by **Keith Vernon-Hutchinson, Hutchinson Music,
PRS/MCPS/PPL-UK** — *Desert Winds, The Ancient One, Ryze of the Darkness,
Drooon & Drooon Dark Wizard, Vikings Sounds & Samples* — **for use only in
Under the Mountain** (Created & Designed by Auston Leroy).

Status 2026-11-04: masters not yet in this tree (untracked files in another
checkout are not copied here). Ingestion, loop points and per-track slot
assignment remain editor/asset-side under T-071.
