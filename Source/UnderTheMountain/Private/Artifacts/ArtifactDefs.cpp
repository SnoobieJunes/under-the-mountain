// [2026-11-04 T-081] See ArtifactDefs.h. Values verbatim from GAMEPLAY S6.
#include "Artifacts/ArtifactDefs.h"

const TArray<FArtifactDefinition>& FArtifactCatalog::All()
{
	static const TArray<FArtifactDefinition> Defs = {
		{TEXT("SeeingStone"), TEXT("Seeing Stone"), TEXT("StoneSocket"), 1200, TEXT("M1"), EArtifactEffectKind::Implemented, TEXT("True-sight through rock; off while carried (ScentComponent gate)")},
		{TEXT("HardenedScales"), TEXT("Hardened Scales"), TEXT("Body"), 600, TEXT("M1"), EArtifactEffectKind::Implemented, TEXT("-30% damage; pit/landing halved, cap 50%")},
		{TEXT("MagicPendant"), TEXT("Magic Pendant"), TEXT("Neck"), 500, TEXT("M1"), EArtifactEffectKind::DataOnlyNamed, TEXT("Magic->HP heal; needs T-080 magic pool")},
		{TEXT("EnhancedClaws"), TEXT("Enhanced Claws"), TEXT("Claws"), 450, TEXT("M1"), EArtifactEffectKind::Implemented, TEXT("Claw damage 25->40")},
		{TEXT("MindControlCrown"), TEXT("Mind-Control Crown"), TEXT("Head"), 1500, TEXT("M4 effect, M1 treasure"), EArtifactEffectKind::DataOnlyNamed, TEXT("M4 domination; M1 value only")},
		{TEXT("RingOfPower"), TEXT("Ring of Power"), TEXT("Ring"), 400, TEXT("M1"), EArtifactEffectKind::Implemented, TEXT("+25 max magic; fire +25%")},
		{TEXT("FlameRing"), TEXT("Flame Ring"), TEXT("Ring"), 450, TEXT("M1 basic / M4 deepened"), EArtifactEffectKind::Implemented, TEXT("Fire ignites: 6 HP/s burn 3 s")},
		{TEXT("ElementalRings"), TEXT("Elemental Rings"), TEXT("Ring"), 450, TEXT("M4"), EArtifactEffectKind::DataOnlyNamed, TEXT("M4 breath element swap; one DA per element at M4")},
		{TEXT("TreasureSightHelm"), TEXT("Treasure-Sight Helm"), TEXT("Head"), 550, TEXT("M1"), EArtifactEffectKind::DataOnlyNamed, TEXT("LoS Prize+ outline; no code yet")},
		{TEXT("StrengthArmor"), TEXT("Strength Armor"), TEXT("Body"), 650, TEXT("M1"), EArtifactEffectKind::DataOnlyNamed, TEXT("Melee +30%; no code yet")},
		{TEXT("ThiefGlow"), TEXT("Thief-Glow"), TEXT("Claws"), 400, TEXT("M1"), EArtifactEffectKind::DataOnlyNamed, TEXT("Nearby-thief glow; no code yet")},
	};
	return Defs;
}
const FArtifactDefinition* FArtifactCatalog::Find(FName Id)
{
	for (const FArtifactDefinition& D : All())
		if (D.Id == Id)
			return &D;
	return nullptr;
}
int32 FArtifactCatalog::TotalStartingGold()
{
	// [2026-11-04 T-081] S6 start set excludes ElementalRings (never in M1 start hoard).
	int32 Sum = 0;
	for (const FArtifactDefinition& D : All())
		if (D.Id != TEXT("ElementalRings"))
			Sum += D.GoldValue;
	return Sum;
}
bool FArtifactRules::IsActiveFor(const UTreasureRegistry* R, FName Id)
{
	return R != nullptr && IsActive(R->Find(Id));
}
float FArtifactRules::DamageAfterHardenedScales(float Damage, bool bActive, bool bPitOrLanding)
{
	if (!bActive || Damage <= 0.0f)
		return FMath::Max(0.0f, Damage);
	float Result = Damage * 0.7f;
	if (bPitOrLanding)
		Result = FMath::Max(Result * 0.5f, Damage * 0.5f); // cap total reduction 50%
	return Result;
}
bool FArtifactRules::CanWearTogether(const TArray<FName>& Worn, FName Candidate)
{
	const FArtifactDefinition* C = FArtifactCatalog::Find(Candidate);
	if (C == nullptr || Worn.Contains(Candidate))
		return false;
	int32 Rings = 0;
	for (FName Id : Worn)
	{
		const FArtifactDefinition* W = FArtifactCatalog::Find(Id);
		if (W == nullptr)
			continue;
		if (W->Slot == TEXT("Ring"))
			++Rings;
		// Body stacks (Scales+Armor); other same-slot pairs conflict.
		if (W->Slot == C->Slot && C->Slot != TEXT("Body") && C->Slot != TEXT("Ring"))
			return false;
	}
	if (C->Slot == TEXT("Ring") && Rings >= 2)
		return false;
	return true;
}
