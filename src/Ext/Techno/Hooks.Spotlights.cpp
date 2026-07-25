#include "Body.h"
#include <FPSCounter.h>
#include "../../Utilities/DirMath.h"
#include "../TechnoType/Body.h"

#include <YRMath.h>
#include <HouseClass.h>
#include <SpotlightClass.h>
#include <TacticalClass.h>

BuildingLightClass * TechnoExt::ActiveBuildingLight = nullptr;

// just in case
DEFINE_HOOK(0x420F40, AlphaShapeClass_DrawAll_Details, 0x6)
{
	// upstream splits the pin's single FPSCounter class: the counters stay on
	// FPSCounter, the thresholds move to Detail (MinFrameRate 0x829FF4,
	// GetMinFrameRate 0x55AF60). Detail::ReduceEffects() is exactly
	// `CurrentFrameRate < GetMinFrameRate()`, which is what this tested.
	return !Detail::ReduceEffects() ? 0u : 0x421346u;
}

DEFINE_HOOK(0x6F6D0E, TechnoClass_Put_1, 0x7)
{
	GET(TechnoClass *, T, ESI);
	TechnoTypeExt::ExtData *pTypeData = TechnoTypeExt::ExtMap.Find(T->GetTechnoType());

	if(pTypeData->Is_Spotlighted) {
		auto pExt = TechnoExt::ExtMap.Find(T);
		auto pSpotlight = GameCreate<BuildingLightClass>(T);
		pExt->SetSpotlight(pSpotlight);
	}

	return 0;
}

DEFINE_HOOK(0x6F6F20, TechnoClass_Put_2, 0x6)
{
	GET(TechnoClass *, T, ESI);
	TechnoTypeExt::ExtData *pTypeData = TechnoTypeExt::ExtMap.Find(T->GetTechnoType());

	if(pTypeData->Is_Spotlighted) {
		auto pExt = TechnoExt::ExtMap.Find(T);
		auto pSpotlight = GameCreate<BuildingLightClass>(T);
		pExt->SetSpotlight(pSpotlight);
	}

	return 0;
}

DEFINE_HOOK(0x441163, BuildingClass_Put_DontSpawnSpotlight, 0x0)
{
	return 0x441196;
}

DEFINE_HOOK(0x435820, BuildingLightClass_CTOR, 0x6)
{
	GET_STACK(TechnoClass *, T, 0x4);
	GET(BuildingLightClass *, BL, ECX);

	if(auto pExt = TechnoExt::ExtMap.Find(T)) {
		pExt->Spotlight = BL;
	}
	return 0;
}

DEFINE_HOOK(0x4370C0, BuildingLightClass_SDDTOR, 0xA)
{
	GET(BuildingLightClass*, pThis, ECX);

	if(!Ares::bShuttingDown) {
		auto pTechno = pThis->OwnerObject;
		if(auto pExt = TechnoExt::ExtMap.Find(pTechno)) {
			pExt->Spotlight = nullptr;
		}
	}
	return 0;
}

DEFINE_HOOK(0x436A2D, BuildingLightClass_PointerGotInvalid_OwnerCloak, 0x6)
{
	GET_STACK(bool const, removed, 0x10);

	// a cloaking owner only drops its soft references. keep the spotlight
	// attached so it reappears once the owner uncloaks.
	return removed ? 0u : 0x436A33u;
}

DEFINE_HOOK(0x436459, BuildingLightClass_Update, 0x6)
{
	GET(BuildingLightClass *, BL, EDI);
	TechnoClass *Owner = BL->OwnerObject;
	if(Owner && Owner->WhatAmI() != AbstractType::Building) {
		TechnoTypeExt::ExtData *pTypeData = TechnoTypeExt::ExtMap.Find(Owner->GetTechnoType());
		CoordStruct Loc = Owner->Location;

		// Turret and Barrel both take the turret's facing: the switch at
		// 0x1004F8C1 sends AttachedTo 1 and 2 alike to TechnoClass+0x3A0 and
		// never reads BarrelFacing (+0x370). Only Body uses Facing (+0x388).
		auto const& facing =
			(pTypeData->Spot_AttachedTo == TechnoTypeExt::SpotlightAttachment::Body)
			? Owner->PrimaryFacing : Owner->SecondaryFacing;

		auto const radians = AresDir::ToRadians(facing.Current());
		Loc.X += static_cast<int>(Math::cos(radians) * pTypeData->Spot_Distance);
		Loc.Y -= static_cast<int>(Math::sin(radians) * pTypeData->Spot_Distance);

		BL->field_B8 = Loc;
		BL->field_C4 = Loc;
//		double zer0 = 0.0;
		__asm { fldz }
	} else {
		double Angle = RulesClass::Instance->SpotlightAngle;
		__asm { fld Angle }
	}

	return R->AL() ? 0x436461u : 0x4364C8u;
}

DEFINE_HOOK(0x435BFA, BuildingLightClass_Draw_Start, 0x6)
{
	GET(BuildingLightClass* const, pThis, ESI);

	TechnoExt::ActiveBuildingLight = pThis;

	auto const pOwner = pThis->OwnerObject;

	if(!pOwner
		|| pOwner->CloakState >= CloakState::Cloaked
		|| pOwner->Deactivated
		|| pOwner->IsBeingWarpedOut()
		|| pOwner->GetHeight() < -10)
	{
		return 0x4361BC;
	}

	if(auto const pBld = abstract_cast<BuildingClass*>(pOwner)) {
		if(!pBld->IsPowerOnline() || pBld->IsFogged) {
			return 0x4361BC;
		}
	}

	return 0x435C52;
}

// Spotlight.StartHeight is the height of the topmost of the three points the
// game builds the light cone from; the other two sit 30 and 180 below it, and
// none may fall below the vanilla constant it replaces. All three sites add the
// base register the stolen `lea` added -- the light is drawn relative to the
// object, not at an absolute z.
static int SpotlightHeight() {
	auto const pOwner = TechnoExt::ActiveBuildingLight->OwnerObject;
	return TechnoTypeExt::ExtMap.Find(pOwner->GetTechnoType())->Spot_Height;
}

// stolen: lea eax, [ebx+1AEh]
DEFINE_HOOK(0x436072, BuildingLightClass_Draw_430, 0x6)
{
	GET(int const, base, EBX);
	R->EAX(base + Math::max(SpotlightHeight(), 0));
	return 0x436078;
}

// stolen: lea ecx, [ebx+190h]
DEFINE_HOOK(0x4360D8, BuildingLightClass_Draw_400, 0x6)
{
	GET(int const, base, EBX);
	R->ECX(base + Math::max(SpotlightHeight() - 30, 400));
	return 0x4360DE;
}

// stolen: lea ecx, [edi+0FAh]
DEFINE_HOOK(0x4360FF, BuildingLightClass_Draw_250, 0x6)
{
	GET(int const, base, EDI);
	R->ECX(base + Math::max(SpotlightHeight() - 180, 250));
	TechnoExt::ActiveBuildingLight = nullptr;
	return 0x436105;
}

DEFINE_HOOK(0x435CD3, BuildingLightClass_Draw_Spotlight, 0x6)
{
	GET_STACK(SpotlightClass *, Spot, 0x14);
	GET(BuildingLightClass *, BL, ESI);

	TechnoClass *Owner = BL->OwnerObject;
	TechnoTypeExt::ExtData *pTypeData = TechnoTypeExt::ExtMap.Find(Owner->GetTechnoType());

	SpotlightFlags Flags = SpotlightFlags::None;
	if(pTypeData->Spot_DisableColor) {
		Flags |= SpotlightFlags::NoColor;
	}
	if(pTypeData->Spot_DisableR) {
		Flags |= SpotlightFlags::NoRed;
	}
	if(pTypeData->Spot_DisableG) {
		Flags |= SpotlightFlags::NoGreen;
	}
	if(pTypeData->Spot_DisableB) {
		Flags |= SpotlightFlags::NoBlue;
	}

	Spot->DisableFlags = Flags;

	return 0;
}
