#include "Body.h"
#include "../TechnoType/Body.h"

#include <BuildingClass.h>
#include <CellClass.h>
#include <HouseClass.h>
#include <TechnoTypeClass.h>

static void __fastcall DrawRadialIndicator(
	bool drawLine, bool adjustColor, CoordStruct coords, ColorStruct color,
	float radius, bool unknown1, bool unknown2)
	{ JMP_STD(0x456980); }

DEFINE_HOOK(0x41BE80, ObjectClass_DrawRadialIndicator, 0x3)
{
	GET(ObjectClass* const, pObject, ECX);

	auto const pType = pObject->GetTechnoType();

	if((pObject->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None
		|| !pType->HasRadialIndicator)
	{
		return 0;
	}

	auto const pThis = static_cast<TechnoClass*>(pObject);

	if(pThis->Deactivated) {
		return 0;
	}

	auto const pOwner = pThis->Owner;

	if(!pOwner->IsControlledByCurrentPlayer()) {
		return 0;
	}

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	int radius;

	if(pTypeExt->RadialIndicatorRadius.isset()) {
		radius = pTypeExt->RadialIndicatorRadius.Get();
	} else if(pType->GapGenerator) {
		radius = pTypeExt->GapRadiusInCells;
	} else {
		auto const pWeapon = pThis->GetTurretWeapon();

		if(!pWeapon || !pWeapon->WeaponType) {
			return 0;
		}

		auto const range = pWeapon->WeaponType->Range;

		if(range <= 0) {
			return 0;
		}

		radius = range / 256;
	}

	if(radius > 0) {
		DrawRadialIndicator(false, true, pThis->GetCoords(), pOwner->Color,
			static_cast<float>(radius), false, true);
	}

	return 0;
}

DEFINE_HOOK(0x44E2B0, BuildingClass_Mi_Unload_LargeGap, 0x6)
{
	GET(BuildingClass* const, pThis, EBP);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if(pTypeExt->SuperGapRadiusInCells) {
		auto const charged = !pThis->GapSuperCharged && pThis->IsPowerOnline();

		if(pThis->GapSuperCharged != charged) {
			pThis->DestroyGap();

			pThis->HasExtraPowerDrain = charged;
			pThis->GapSuperCharged = charged;
			pThis->GapRadius = charged
				? pTypeExt->SuperGapRadiusInCells : pTypeExt->GapRadiusInCells;
			pThis->Owner->RecheckPower = true;

			pThis->CreateGap();
		}
	}

	return 0x44E371;
}

DEFINE_HOOK(0x454BDC, BuildingClass_UpdatePowered_LargeGap, 0x7)
{
	GET(BuildingTypeClass* const, pType, ECX);

	R->EDX(TechnoTypeExt::ExtMap.Find(pType)->GapRadiusInCells.Get());

	return 0x454BE3;
}

DEFINE_HOOK(0x4566B0, BuildingClass_GetRangeOfRadial_Radius, 0x6)
{
	GET(BuildingClass* const, pThis, ECX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if(!pTypeExt->RadialIndicatorRadius.isset()) {
		return 0;
	}

	R->EAX(pTypeExt->RadialIndicatorRadius.Get());

	return 0x45674E;
}

DEFINE_HOOK(0x4566D5, BuildingClass_GetRangeOfRadial_LargeGap, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	R->EAX(pThis->GapSuperCharged
		? pTypeExt->SuperGapRadiusInCells.Get() : pTypeExt->GapRadiusInCells.Get());

	return 0x456745;
}

DEFINE_HOOK_AGAIN(0x6FB4A3, TechnoClass_CreateGap_LargeGap, 0x7)
DEFINE_HOOK(0x6FB1B5, TechnoClass_CreateGap_LargeGap, 0x7)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(TechnoTypeClass* const, pType, EAX);

	pThis->GapRadius = TechnoTypeExt::ExtMap.Find(pType)->GapRadiusInCells;

	return R->Origin() + 0xD;
}

DEFINE_HOOK(0x6FB306, TechnoClass_CreateGap_Optimize, 0x6)
{
	GET(CellClass* const, pCell, EAX);

	auto counter = pCell->ShroudCounter;

	if(counter >= 0 && counter != 1) {
		++counter;
		pCell->ShroudCounter = counter;
	}

	++pCell->GapsCoveringThisCell;

	if(counter >= 1) {
		pCell->AltFlags &= ~AltCellFlags::Clear;
	}

	return 0x6FB3BD;
}

DEFINE_HOOK(0x6FB5F0, TechnoClass_DeleteGap_Optimize, 0x6)
{
	GET(CellClass* const, pCell, EAX);

	--pCell->GapsCoveringThisCell;

	if(HouseClass::CurrentPlayer->SpySatActive
		&& static_cast<int>(pCell->GapsCoveringThisCell) <= 0)
	{
		--pCell->ShroudCounter;

		if(pCell->ShroudCounter <= 0) {
			pCell->AltFlags |= AltCellFlags::Clear;
		}
	}

	return 0x6FB69E;
}
