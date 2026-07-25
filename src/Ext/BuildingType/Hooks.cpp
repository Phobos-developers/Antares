#include "Body.h"

#include "../Building/Body.h"
#include "../House/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"

#include <ScenarioClass.h>
#include <AnimClass.h>
#include <VocClass.h>

// =============================
// other hooks

DEFINE_HOOK(0x45E416, BuildingTypeClass_CTOR_Initialize, 0x6)
{
	GET(BuildingTypeClass*, pThis, ESI);

	pThis->BuildingAnimFrame[3].dwUnknown = 0;
	pThis->BuildingAnimFrame[3].FrameCount = 1;
	pThis->BuildingAnimFrame[3].FrameDuration = 0;

	pThis->VoxelBarrelScale = 1.0;

	pThis->VoxelBarrelOffsetToPitchPivotPoint = CoordStruct::Empty;
	pThis->VoxelBarrelOffsetToRotatePivotPoint = CoordStruct::Empty;
	pThis->VoxelBarrelOffsetToBuildingPivotPoint = CoordStruct::Empty;
	pThis->VoxelBarrelOffsetToBarrelEnd = CoordStruct::Empty;

	return 0;
}

// a cloning facility is a factory for the purpose of picking the building the
// new unit walks out of, but never a candidate for the alternate kickout
DEFINE_HOOK(0x4444B3, BuildingClass_KickOutUnit_NoAlternateKickout, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	auto const pType = pThis->Type;
	auto const pExt = BuildingTypeExt::ExtMap.Find(pType);

	return (pType->Factory == AbstractType::None || pExt->CloningFacility)
		? 0x4452C5u
		: 0u
	;
}

DEFINE_HOOK(0x455DA0, BuildingClass_IsFactory_CloningFacility, 0x6)
{
	GET(BuildingClass* const, pThis, ECX);

	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return pExt->CloningFacility ? 0x455DCDu : 0u;
}

DEFINE_HOOK(0x445F80, BuildingClass_Place, 0x5)
{
	GET(BuildingClass *, pThis, ECX);
	if(pThis->Type->SecretLab) {
		auto pExt = BuildingExt::ExtMap.Find(pThis);
		pExt->UpdateSecretLab();
	}

	BuildingExt::UpdateFactoryPlans(pThis);

	return 0;
}

DEFINE_HOOK(0x43FB6D, BuildingClass_Update_LaserFencePost, 0x6)
{
	GET(BuildingClass*, B, ESI);
	if(B->Type->LaserFencePost) {
		B->CreateEndPost(1);
	}
	return 0;
}

DEFINE_HOOK(0x465D4A, BuildingTypeClass_IsUndeployable, 0x6)
{
	GET(BuildingTypeClass *, pThis, ECX);
	if(pThis->Foundation == BuildingTypeExt::CustomFoundation) {
		BuildingTypeExt::ExtData* pData = BuildingTypeExt::ExtMap.Find(pThis);

		R->EAX(pData->CustomHeight == 1 && pData->CustomWidth == 1);
		return 0x465D6D;
	}
	return 0;
}

DEFINE_HOOK(0x465550, BuildingTypeClass_GetFoundationOutline, 0x6)
{
	GET(BuildingTypeClass *, pThis, ECX);
	if(pThis->Foundation == BuildingTypeExt::CustomFoundation) {
		BuildingTypeExt::ExtData* pData = BuildingTypeExt::ExtMap.Find(pThis);

		R->EAX(pData->OutlineData.data());
		return 0x46556D;
	}
	return 0;
}

DEFINE_HOOK(0x464AF0, BuildingTypeClass_GetSizeInLeptons, 0x6)
{
	GET(BuildingTypeClass *, pThis, ECX);
	if(pThis->Foundation == BuildingTypeExt::CustomFoundation) {
		GET_STACK(CoordStruct *, Coords, 0x4);
		BuildingTypeExt::ExtData* pData = BuildingTypeExt::ExtMap.Find(pThis);

		Coords->X = pData->CustomWidth * 256;
		Coords->Y = pData->CustomHeight * 256;
		Coords->Z = Unsorted::LevelHeight * pThis->Height;
		R->EAX(Coords);
		return 0x464B2C;
	}
	return 0;
}

DEFINE_HOOK(0x45ECE0, BuildingTypeClass_GetMaxPips, 0x6)
{
	GET(BuildingTypeClass *, pThis, ECX);
	if(pThis->Foundation == BuildingTypeExt::CustomFoundation) {
		BuildingTypeExt::ExtData* pData = BuildingTypeExt::ExtMap.Find(pThis);

		R->EAX(pData->CustomWidth);
		return 0x45ECED;
	}
	return 0;
}

DEFINE_HOOK(0x45F2B4, BuildingTypeClass_Load2DArt_BuildupTime, 0x5)
{
	GET(BuildingTypeClass* const, pThis, EBP);
	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis);
	pExt->UpdateBuildupFrames();
	return 0x45F310;
}

DEFINE_HOOK(0x465A48, BuildingTypeClass_GetBuildup_BuildupTime, 0x5)
{
	GET(BuildingTypeClass* const, pThis, ESI);
	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis);
	pExt->UpdateBuildupFrames();
	return 0x465AAE;
}

DEFINE_HOOK(0x45EAA5, BuildingTypeClass_LoadArt_BuildupTime, 0x6)
{
	GET(BuildingTypeClass* const, pThis, ESI);
	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis);
	pExt->UpdateBuildupFrames();
	return 0x45EB3A;
}

DEFINE_HOOK(0x459C03, BuildingClass_CanBeSelectedNow_MassSelectable, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	auto const pType = pThis->Type;
	auto const pExt = BuildingTypeExt::ExtMap.Find(pType);

	if(!pExt->MassSelectable.Get(pType->IsVehicle())) {
		R->EAX(0);
		return 0x459C12;
	}

	return 0x459C14;
}

DEFINE_HOOK(0x4FB2FD, HouseClass_UnitFromFactory_BuildingSlam, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	auto const idxSound = pExt->SlamSound.Get(RulesClass::Instance->BuildingSlam);

	VocClass::PlayGlobal(idxSound, 0x2000, 1.0f);

	return 0x4FB319;
}
