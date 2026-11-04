// Copyright Under the Mountain contributors.
// [2026-11-01 T-049] Terrain commandlet implementation. See the
// header for why this is C++ (Python cannot create Landscape
// components). Everything below is editor-only.

#include "TerrainCommandlet.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorLevelUtils.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// [2026-11-01 T-049] Terrain grid: 505x505 height samples over
// 504x504 quads (the well-formed landscape split: 63-quad
// subsections, 2 per component, 4x4 components - a 65-quad
// subsection makes the weightmap 130 wide and Import asserts it
// is a power of two). Quad size 1000 cm gives a 5.04 km square
// centred on the lair - at MaxFlySpeed (1800 cm/s) that is ~4.7
// minutes of flight edge to edge, the "minutes of flight" the
// task asks for. 10 m quads are coarse up close and fine at vista
// range; the lair and village keep their authored boxes.
constexpr int32 GridSize = 505; // samples per side (quads + 1)
constexpr float QuadSizeCm = 1000.0f;
constexpr float ZScale = 40.0f; // landscape native +-256 units * 40 = +-10240 cm
constexpr float ZRangeCm = 10240.0f;

// [2026-11-01 T-049] Smoothstep for masks: 0 at Edge0, 1 at Edge1.
double SmoothStep(double Edge0, double Edge1, double X)
{
	const double T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0, 1.0);
	return T * T * (3.0 - 2.0 * T);
}

// [2026-11-01 T-049] The world height function, in cm, closed-form
// in world (X, Y). Three ingredients, then two masks:
//  - rolling fields: low, low-frequency swells everywhere, so the
//    far ground is never a billiard table;
//  - distant ranges: ridged sine crests whose amplitude grows with
//    distance from the lair, so the horizon is mountain walls in
//    every direction (no visible void or map edge from altitude);
//  - the lair massif: one gaussian mountain south-east of the peak
//    shaft exit (19000, 0), so flying out of the shaft reads as
//    leaving a mountain, not a box.
// Masks: the lair corridor (the chamber chain, x < 24000 and
// |y| < 9000) is pressed to -250 cm, below every chamber floor
// (floor tops are z=0), so terrain can never poke into the
// interior; a soft shoulder blends it back out.
double TerrainHeightCm(double X, double Y)
{
	const double Fields = 350.0 * FMath::Sin(X * 2.1e-5 + 1.7) * FMath::Cos(Y * 1.7e-5) +
						  250.0 * FMath::Sin((X + Y) * 1.3e-5);

	const double DistFromLair = FMath::Sqrt(FMath::Square(X - 10000.0) + FMath::Square(Y));
	const double RangeMask = SmoothStep(70000.0, 150000.0, DistFromLair);
	const double Ridge =
		FMath::Pow(1.0 - FMath::Abs(FMath::Sin(X * 4.5e-5 + 2.0 * FMath::Sin(Y * 3.0e-5))), 3.0);
	const double Ranges = RangeMask * (900.0 + 3400.0 * Ridge);

	const double MassifDist2 =
		FMath::Square(X - 19000.0) + FMath::Square(Y + 10000.0);
	const double Massif = 4600.0 * FMath::Exp(-MassifDist2 / (2.0 * 20000.0 * 20000.0));

	double Height = Fields + Ranges + Massif;

	// Lair corridor mask: 1 deep inside the chamber chain, 0 outside
	// the shoulder. Pressed height is -250 (below the z=0 floors).
	const double CorridorX = 1.0 - SmoothStep(21000.0, 26000.0, X);
	const double CorridorY = 1.0 - SmoothStep(7000.0, 10000.0, FMath::Abs(Y));
	const double Corridor = CorridorX * CorridorY;
	// [2026-11-04 T-122] Valley press: the T-017 flight zone (terraces +
	// valley floor boxes at z<=0, x in [27k, 63k]) predates the T-049
	// landscape, and the massif skirt buries it under ~29 m of terrain
	// at the village (proven: TerrainHeightCm(38000, 0) = 2914 with the
	// old mask). Press the valley rect to the same -250 the boxes were
	// authored against. Bounds are load-bearing (proven: zero box-top
	// excesses on a 250 cm grid over the whole flight zone): west
	// blends 23000->27000 so Terrace1's south edge (the massif north
	// lobe hits 4324 raw at (25000, -8000)) and the valley floor's
	// west edge (x=27000) both sit on fully pressed ground; east
	// blends back by x=90000 so the valley exits through a pass
	// instead of a canyon to the map edge; |y| blends 21000->26000.
	// Combined with max() so either mask fully presses and shoulders
	// never stack past 1. Lair zone x<23000 is corridor-only,
	// bit-identical to the pre-T-122 function. Twin:
	// Scripts/terrain_profile.py terrain_height (T-050 mirror rule:
	// change one, change the other; diffed equal at the T-122 close).
	const double ValleyX =
		SmoothStep(23000.0, 27000.0, X) * (1.0 - SmoothStep(66000.0, 90000.0, X));
	const double ValleyY = 1.0 - SmoothStep(21000.0, 26000.0, FMath::Abs(Y));
	const double Press = FMath::Max(Corridor, ValleyX * ValleyY);
	Height = Height * (1.0 - Press) + (-250.0) * Press;

	return FMath::Clamp(Height, -800.0, 5200.0);
}
} // namespace

UTerrainCommandlet::UTerrainCommandlet()
{
	// [2026-11-01 T-049] Commandlets that load and save maps need the
	// editor world and asset loading; these flags are the standard
	// map-editing commandlet contract.
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UTerrainCommandlet::Main(const FString& Params)
{
	const FString MapPath = TEXT("/Game/Maps/Lair/Lair");
	UE_LOG(LogTemp, Display, TEXT("TerrainCommandlet: loading %s"), *MapPath);
	if (!FEditorFileUtils::LoadMap(MapPath, false, true))
	{
		UE_LOG(LogTemp, Error, TEXT("TerrainCommandlet: FAILED to load %s"), *MapPath);
		return 1;
	}
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("TerrainCommandlet: no editor world after load"));
		return 1;
	}

	// [2026-11-01 T-049] Re-runnable: a previous terrain actor is
	// destroyed and rebuilt from the same closed-form function, so
	// re-running after a height retune replaces, never duplicates.
	for (TActorIterator<ALandscape> It(World); It; ++It)
	{
		if (It->GetActorLabel() == TEXT("Terrain_World"))
		{
			World->DestroyActor(*It);
			UE_LOG(LogTemp, Display, TEXT("TerrainCommandlet: destroyed prior Terrain_World"));
			break;
		}
	}

	// [2026-11-01 T-049] The landscape's local origin is its corner:
	// with 504 quads of 1000 cm the actor sits at (-252000, -252000)
	// so the grid covers +-2.52 km around the lair at (0..21000, 0).
	const FVector Origin(-(GridSize - 1) * QuadSizeCm * 0.5, -(GridSize - 1) * QuadSizeCm * 0.5,
						 0.0f);
	ALandscape* Landscape =
		World->SpawnActor<ALandscape>(Origin, FRotator::ZeroRotator);
	if (!Landscape)
	{
		UE_LOG(LogTemp, Error, TEXT("TerrainCommandlet: FAILED to spawn ALandscape"));
		return 1;
	}
	Landscape->SetActorLabel(TEXT("Terrain_World"));
	Landscape->SetActorScale3D(FVector(QuadSizeCm, QuadSizeCm, ZScale));

	// [2026-11-01 T-049] Height samples encoded for the landscape's
	// uint16 range: world z = (sample/65535 - 0.5) * 512 * ZScale,
	// so +-ZRangeCm maps onto the full range. Row-major, Y rows of X
	// samples - the layout ALandscapeProxy::Import documents.
	TArray<uint16> HeightSamples;
	HeightSamples.SetNumUninitialized(GridSize * GridSize);
	for (int32 GridY = 0; GridY < GridSize; ++GridY)
	{
		for (int32 GridX = 0; GridX < GridSize; ++GridX)
		{
			const double WorldX = Origin.X + GridX * QuadSizeCm;
			const double WorldY = Origin.Y + GridY * QuadSizeCm;
			const double Height = TerrainHeightCm(WorldX, WorldY);
			const double Encoded = FMath::Clamp((Height + ZRangeCm) / (2.0 * ZRangeCm), 0.0, 1.0);
			HeightSamples[GridY * GridSize + GridX] =
				static_cast<uint16>(Encoded * 65535.0);
		}
	}
	TMap<FGuid, TArray<uint16>> HeightData;
	const FGuid LandscapeGuid = FGuid::NewGuid();
	// [2026-11-01 T-049] With no edit layers in play, Import reads
	// the height/layer maps under the ZERO Guid (FinalLayerGuid =
	// FGuid() in LandscapeEdit.cpp) - a fresh Guid here asserts in
	// FindChecked. The landscape's own Guid is the first argument.
	HeightData.Add(FGuid(), MoveTemp(HeightSamples));

	// [2026-11-01 T-049] 504 quads = 2 subsections of 63 quads per
	// component side, 4x4 components - the standard well-formed
	// split for a 505-sample heightmap. No weight layers: the
	// landscape wears the lair's rock material as a plain surface
	// (height variation, not paint layers, carries the vista).
	// [2026-11-01 T-049] Import asserts the height-data and
	// material-layer maps carry the same layer Guids; with no weight
	// layers the layer map still needs the (empty) entry.
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> LayerInfos;
	LayerInfos.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());
	Landscape->Import(LandscapeGuid, 0, 0, GridSize - 1, GridSize - 1, 2, 63, HeightData,
					  nullptr, LayerInfos,
					  ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());

	// [2026-11-03 T-066] Mountain material (renamed from the T-012
	// M_RockDark set: fresh names dodge stale headless-registry cache
	// collisions — see create_lair.py).
	UMaterialInterface* RockMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Maps/Lair/M_MountainRock.M_MountainRock"));
	if (RockMaterial)
	{
		// [2026-11-01 T-049] Property write plus an explicit
		// UpdateAllComponentMaterialInstances: the setter that does
		// both (EditorSetLandscapeMaterial) is not exported from the
		// Landscape module, and a plain assignment alone leaves the
		// components on the pale default material (seen in the
		// first run's viewport capture).
		Landscape->LandscapeMaterial = RockMaterial;
		Landscape->UpdateAllComponentMaterialInstances();
		UE_LOG(LogTemp, Display, TEXT("TerrainCommandlet: landscape material now %s, components=%d"),
			   Landscape->LandscapeMaterial ? *Landscape->LandscapeMaterial->GetName() : TEXT("null"),
			   Landscape->LandscapeComponents.Num());
		if (Landscape->LandscapeComponents.Num() > 0 &&
			Landscape->LandscapeComponents[0] != nullptr)
		{
			UMaterialInterface* CompMat = Landscape->LandscapeComponents[0]->GetLandscapeMaterial();
			UE_LOG(LogTemp, Display, TEXT("TerrainCommandlet: component 0 material %s"),
				   CompMat ? *CompMat->GetName() : TEXT("null"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("TerrainCommandlet: M_MountainRock not found; landscape keeps default material"));
	}
	Landscape->RegisterAllComponents();

	// [2026-11-04 T-122] Never save a component-less landscape: the map
	// lost its entire terrain in c7531d3 (2.05MB -> 454KB) because this
	// commandlet destroys-then-rebuilds and saves unconditionally, so a
	// silent Import failure ships a terrain-less map that nothing
	// detects (verify_lair.py only sees StaticMeshActors). 16 = the 4x4
	// components T-049's well-formed split guarantees. Returning 1 here
	// leaves the on-disk map (with its old terrain) untouched.
	if (Landscape->LandscapeComponents.Num() != 16)
	{
		UE_LOG(LogTemp, Error,
			   TEXT("TerrainCommandlet: REFUSING to save - Import produced %d components, expected 16"),
			   Landscape->LandscapeComponents.Num());
		return 1;
	}

	const FBox Bounds = Landscape->GetComponentsBoundingBox(true);
	UE_LOG(LogTemp, Display,
		   TEXT("TerrainCommandlet: bounds min=(%.0f, %.0f, %.0f) max=(%.0f, %.0f, %.0f)"),
		   Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z, Bounds.Max.X, Bounds.Max.Y, Bounds.Max.Z);

	// [2026-11-01 T-049] FEditorFileUtils::SaveMap treats its second
	// argument as a literal filename (it tried to move the temp file
	// onto "/Game/Maps/Lair/Lair" as a path); the LoadingAndSaving
	// utility takes the package path this commandlet means.
	if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPath))
	{
		UE_LOG(LogTemp, Error, TEXT("TerrainCommandlet: FAILED to save %s"), *MapPath);
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("TERRAIN_DONE extent_m=%.0f quad_cm=%.0f zrange=+/-%.0f"),
		   (GridSize - 1) * QuadSizeCm / 100.0, QuadSizeCm, ZRangeCm);
	return 0;
}
