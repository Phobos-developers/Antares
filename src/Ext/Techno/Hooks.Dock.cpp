#include "Body.h"

#include "../BuildingType/Body.h"
#include "../../Utilities/TemplateDef.h"

#include <BuildingClass.h>
#include <UnitClass.h>

// the refinery this unit docked to, if it sits on the refinery's unload cell
static BuildingClass* GetDockUnloadBuilding(TechnoClass* pThis)
{
	auto const pBuilding = abstract_cast<BuildingClass*>(pThis->GetNthLink());

	if(!pBuilding) {
		return nullptr;
	}

	auto const pExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);
	auto const& offset = pExt->DockUnloadCell.Get();

	auto const dock = pBuilding->GetMapCoords();
	auto const cell = pThis->GetMapCoords();

	if(cell.X != dock.X + offset.X || cell.Y != dock.Y + offset.Y) {
		return nullptr;
	}

	return pBuilding;
}

// the facing this unit turns to before unloading at its refinery
static DirStruct GetDockUnloadFacing(TechnoClass* pThis)
{
	if(pThis->HasAnyLink()) {
		if(auto const pBuilding = abstract_cast<BuildingClass*>(pThis->GetNthLink())) {
			auto const pExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);
			return DirStruct(pExt->DockUnloadFacing * 2048);
		}
	}

	return DirStruct(0x4000);
}

DEFINE_HOOK(0x43CA80, BuildingClass_ReceivedRadioCommand_DockUnloadCell, 0x7)
{
	GET(BuildingClass* const, pThis, ESI);
	GET(CellStruct* const, pCell, EAX);

	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	auto const& offset = pExt->DockUnloadCell.Get();

	auto const x = pCell->X + offset.X;
	auto const y = pCell->Y + offset.Y;

	R->DX(static_cast<WORD>(x));
	R->AX(static_cast<WORD>(y));

	return 0x43CA8D;
}

DEFINE_HOOK(0x7376D9, UnitClass_ReceivedRadioCommand_DockUnload_Facing, 0x5)
{
	GET(UnitClass* const, pThis, ESI);
	GET(DirStruct* const, pFacing, EAX);

	auto const facing = GetDockUnloadFacing(pThis);

	if(*pFacing == facing) {
		return 0x73771B;
	}

	pThis->Locomotor->Do_Turn(facing);

	return 0x73770C;
}

DEFINE_HOOK(0x73DF66, UnitClass_Mi_Unload_DockUnload_Facing, 0x5)
{
	GET(UnitClass* const, pThis, ESI);
	GET(DirStruct* const, pFacing, EAX);

	auto const facing = GetDockUnloadFacing(pThis);

	if(*pFacing == facing || pThis->unknown_bool_6AF) {
		return 0x73DFBD;
	}

	pThis->Locomotor->Do_Turn(facing);

	return 0x73DFB0;
}

DEFINE_HOOK(0x73E013, UnitClass_Mi_Unload_DockUnloadCell1, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	R->EAX(GetDockUnloadBuilding(pThis));
	return 0x73E05F;
}

DEFINE_HOOK(0x73E17F, UnitClass_Mi_Unload_DockUnloadCell2, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	R->EAX(GetDockUnloadBuilding(pThis));
	return 0x73E1CB;
}

DEFINE_HOOK(0x73E2BF, UnitClass_Mi_Unload_DockUnloadCell3, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	R->EAX(GetDockUnloadBuilding(pThis));
	return 0x73E30B;
}

DEFINE_HOOK(0x741BDB, UnitClass_SetDestination_DockUnloadCell, 0x7)
{
	GET(UnitClass* const, pThis, EBP);

	R->EAX(GetDockUnloadBuilding(pThis));
	return 0x741C28;
}

static_assert(offsetof(BuildingClass, Type) == 0x520, "BuildingClass layout slipped");
static_assert(offsetof(RadioClass, RadioLinks) == 0xE0, "RadioClass layout slipped");
static_assert(offsetof(FootClass, Locomotor) == 0x674, "FootClass layout slipped");
static_assert(offsetof(FootClass, unknown_bool_6AF) == 0x6AF, "FootClass layout slipped");
