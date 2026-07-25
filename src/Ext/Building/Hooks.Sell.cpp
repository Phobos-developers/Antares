#include "Body.h"

#include "../BuildingType/Body.h"
#include "../Rules/Body.h"
#include "../TechnoType/Body.h"

#include <FootClass.h>

DEFINE_HOOK(0x447A63, BuildingClass_QueueImageAnim_Sell, 0x3)
{
	GET(BuildingClass* const, pThis, ESI);
	GET_BASE(int, delay, 0x8);

	auto const pType = pThis->Type;

	if(pThis->CurrentMission == Mission::Selling) {
		delay = BuildingTypeExt::ExtMap.Find(pType)->SellFrames;
	}

	R->EDX(pType);
	R->EAX(delay);

	return 0x447A6C;
}

DEFINE_HOOK(0x449518, BuildingClass_IsSellable_FirestormWall, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	enum { Firewall = 0x449522, Unsellable = 0x449536 };

	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return pExt->Firewall_Is ? Firewall : Unsellable;
}

DEFINE_HOOK(0x4D9EBD, FootClass_CanBeSold_SellUnit, 0x6)
{
	GET(FootClass* const, pThis, ESI);
	GET(BuildingClass* const, pDock, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const pDockType = pDock->Type;
	auto const pDockExt = BuildingTypeExt::ExtMap.Find(pDockType);

	auto const canSell = pDockExt->UnitSell.Get(pDockType->UnitRepair)
		&& !pTypeExt->Unsellable.Get(RulesExt::Global()->UnitsUnsellable);

	R->CL(canSell);

	return 0x4D9EC9;
}

DEFINE_HOOK(0x4D9EE1, FootClass_CanBeSold_Dock, 0x6)
{
	GET(FootClass* const, pThis, ESI);
	GET(BuildingClass* const, pDock, EAX);
	GET(CoordStruct* const, pBuffer, ECX);

	R->EAX(pDock->GetDockCoords(pBuffer, pThis));

	return 0x4D9EE7;
}
