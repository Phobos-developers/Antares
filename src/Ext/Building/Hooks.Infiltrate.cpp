#include "Body.h"
#include "../BuildingType/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"

#include <FactoryClass.h>
#include <HouseClass.h>
#include <PCX.h>

/* #633 - spy building infiltration */
// wrapper around the entire function
DEFINE_HOOK(0x4571E0, BuildingClass_Infiltrate, 0x5)
{
	GET(BuildingClass *, EnteredBuilding, ECX);
	GET_STACK(HouseClass *, Enterer, 0x4);

	BuildingExt::ExtData *pBuilding = BuildingExt::ExtMap.Find(EnteredBuilding);

	return (pBuilding->InfiltratedBy(Enterer))
		? 0x4575A2
		: 0
	;
}

// #814: force sidebar repaint for standard spy effects
DEFINE_HOOK_AGAIN(0x4574D2, BuildingClass_Infiltrate_Standard, 0x6)
DEFINE_HOOK(0x457533, BuildingClass_Infiltrate_Standard, 0x6)
{
	MouseClass::Instance.SidebarNeedsRepaint();
	return R->Origin() + 6;
}

DEFINE_HOOK(0x43E7B0, BuildingClass_DrawVisible, 0x5)
{
	GET(BuildingClass*, pThis, ECX);
	GET_STACK(Point2D*, pLocation, 0x4);
	GET_STACK(RectangleStruct*, pBounds, 0x8);

	auto pType = pThis->Type;
	auto pExt = BuildingTypeExt::ExtMap.Find(pType);

	// helpers (with support for the new spy effect)
	bool bAllied = pThis->Owner->IsAlliedWith(HouseClass::CurrentPlayer);
	bool bReveal = pExt->RevealProduction && pThis->DisplayProductionTo.Contains(HouseClass::CurrentPlayer);

	// an observer is shown everything an ally would be: both the production
	// cameo and the extra info. `cmp edx, ds:0AC1198h` at 0x100161C0.
	bool bObserver = HouseClass::IsCurrentPlayerObserver();

	if(!pThis->IsSelected) {
		return 0x43E8F2;
	}

	// display production cameo
	if(bReveal || bObserver) {
		// which factory to read: for a house *any* human plays, the house's
		// primary factory for this building's category; otherwise whatever
		// this building itself is producing. Note this is ControlledByHuman
		// (`CurrentPlayer`, plus `PlayerControl` in campaign) and not
		// ControlledByPlayer, which in a multiplayer game asks whether the
		// house is the one at *this* machine -- the wrong question now that
		// an ally's or an observer's view reaches here.
		auto pFactory = pThis->Owner->IsControlledByHuman()
			? pThis->Owner->GetPrimaryFactory(pType->Factory, pType->Naval, BuildCat::DontCare)
			: pThis->Factory;

		if(pFactory && pFactory->Object) {
			if(auto pProdType = pFactory->Object->GetTechnoType()) {
				auto pProdExt = TechnoTypeExt::ExtMap.Find(pProdType);

				// support for pcx cameos
				if(auto pPCX = pProdExt->CameoPCX.GetSurface()) {
					const int cameoWidth = 60;
					const int cameoHeight = 48;

					RectangleStruct cameoBounds = {0, 0, cameoWidth, cameoHeight};
					RectangleStruct destRect = {pLocation->X - cameoWidth / 2, pLocation->Y - cameoHeight / 2, cameoWidth, cameoHeight};
					RectangleStruct destClip = Drawing::Intersect(destRect, *pBounds, nullptr, nullptr);

					DSurface::Temp->CopyFrom(pBounds, &destClip, pPCX, &cameoBounds, &cameoBounds, true, true);
				} else {
					// old shp cameos, fixed palette
					auto pCameo = pProdType->GetCameo();
					auto pConvert = pProdExt->CameoPal.Convert ? pProdExt->CameoPal.GetConvert() : FileSystem::CAMEO_PAL;
					DSurface::Temp->DrawSHP(pConvert, pCameo, 0, pLocation, pBounds, BlitterFlags(0xE00), 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
				}

				// the cameo replaces the extra info, it is not drawn over it:
				// both cameo paths return straight to 0x43E8F2 (0x100163C7,
				// 0x1001637E) without reaching the DrawExtraInfo block.
				return 0x43E8F2;
			}
		}
	}

	// show building or house state
	if(bAllied || bReveal || bObserver) {
		Point2D loc = {pLocation->X - 10, pLocation->Y + 10};
		pThis->DrawExtraInfo(loc, *pLocation, *pBounds);
	}

	return 0x43E8F2;
}

// if this is a radar, change the owner's house bitfields responsible for radar reveals
DEFINE_HOOK(0x44161C, BuildingClass_Destroy_OldSpy1, 0x6)
{
	GET(BuildingClass *, B, ESI);
	B->DisplayProductionTo.Clear();
	BuildingExt::UpdateDisplayTo(B);
	return 0x4416A2;
}

// if this is a radar, change the owner's house bitfields responsible for radar reveals
DEFINE_HOOK(0x448312, BuildingClass_ChangeOwnership_OldSpy1, 0xa)
{
	GET(HouseClass *, newOwner, EBX);
	GET(BuildingClass *, B, ESI);

	if(B->DisplayProductionTo.Contains(newOwner)) {
		B->DisplayProductionTo.Remove(newOwner);
		BuildingExt::UpdateDisplayTo(B);
	}
	return 0x4483A0;
}

// if this is a radar, drop the new owner from the bitfield
DEFINE_HOOK(0x448D95, BuildingClass_ChangeOwnership_OldSpy2, 0x8)
{
	GET(HouseClass *, newOwner, EDI);
	GET(BuildingClass *, B, ESI);

	if(B->DisplayProductionTo.Contains(newOwner)) {
		B->DisplayProductionTo.Remove(newOwner);
	}

	return 0x448DB9;
}

DEFINE_HOOK(0x44F7A0, BuildingClass_UpdateDisplayTo, 0x0)
{
	GET(BuildingClass *, B, ECX);
	BuildingExt::UpdateDisplayTo(B);
	return 0x44F813;
}

DEFINE_HOOK(0x509303, HouseClass_AllyWith_unused, 0x0)
{
	GET(HouseClass *, pThis, ESI);
	GET(HouseClass *, pThat, EAX);

	pThis->RadarVisibleTo.Add(pThat);
	return 0x509319;
}
