#include <BuildingClass.h>
#include <DisplayClass.h>
#include <Fundamentals.h>
#include <HouseClass.h>
#include <HouseTypeClass.h>
#include <OverlayTypeClass.h>
#include <RulesClass.h>
#include <SessionClass.h>
#include <UnitClass.h>

#include "../Ares.h"
#include "Debug.h"

#include <algorithm>

// a free unit that is not a harvester has nothing to harvest
DEFINE_HOOK(0x446E9F, BuildingClass_Place_FreeUnit_Mission, 0x6)
{
	GET(UnitClass* const, pUnit, EDI);

	pUnit->QueueMission(
		pUnit->Type->Harvester ? Mission::Harvest : Mission::Area_Guard, false);

	return 0x446EAD;
}

// nobody can get angry at a country that cannot take part in the fighting
DEFINE_HOOK(0x504796, HouseClass_AddAnger_MultiplayPassive, 0x6)
{
	GET(HouseClass* const, pThis, ECX);
	GET_STACK(HouseClass* const, pHouse, 0x10);

	if(SessionClass::Instance.GameMode != GameMode::Campaign
		&& pHouse->Type->MultiplayPassive)
	{
		R->ECX(0);
	} else {
		R->ECX(pThis->AngerNodes.Count);
	}

	return 0x50479C;
}

// a unit that is about to drop its target should not walk up to it, and one
// inside an open topped transport cannot move at all
DEFINE_HOOK(0x4D5782, FootClass_ApproachTarget_Passive, 0x6)
{
	GET(FootClass* const, pThis, EBX);

	R->ECX(pThis->ShouldLoseTargetNow || pThis->InOpenToppedTransport);

	return 0x4D5788;
}

// release the airspace even when the unit has already lost its cell
DEFINE_HOOK(0x4DB37C, FootClass_Remove_Airspace, 0x6)
{
	return 0x4DB3A4;
}

// a burst weapon keeps firing its remaining shots at the new target instead
// of starting over
DEFINE_HOOK(0x6FCF53, TechnoClass_SetTarget_Burst, 0x6)
{
	return 0x6FCF61;
}

DEFINE_HOOK(0x7387D1, UnitClass_Destroy_ShakeScreenZero, 0x6)
{
	return RulesClass::Instance->ShakeScreen ? 0 : 0x738801;
}

// only own and allied objects have to respect a bib cell
DEFINE_HOOK(0x73F7DD, UnitClass_IsCellOccupied_Bib, 0x8)
{
	GET(ObjectClass* const, pObject, EBX);
	GET(BuildingClass* const, pBuilding, ESI);

	return pBuilding->Owner->IsAlliedWith(pObject) ? 0 : 0x73F823;
}

// the single bubble sort pass the layer did per frame never converged on a
// big map. sort a fifteenth of it properly instead.
DEFINE_HOOK(0x551A30, LayerClass_YSortReorder, 0x5)
{
	GET(LayerClass* const, pThis, ECX);

	auto const step = pThis->Count / 15;
	auto const slice = Unsorted::CurrentFrame % 15;

	auto const begin = pThis->Items + step * slice;
	auto const end = (slice >= 14)
		? pThis->Items + pThis->Count
		: begin + step + step / 4;

	std::sort(begin, end, [](ObjectClass* const pA, ObjectClass* const pB) {
		return pA->GetYSort() < pB->GetYSort();
	});

	return 0x551A84;
}

DEFINE_HOOK(0x7BB445, XSurface_20, 0x6)
{
	return R->EAX() ? 0 : 0x7BB90C;
}

// the game forgot the trailing newline
DEFINE_HOOK(0x534F89, Game_ReloadNeutralMIX_NewLine, 0x5)
{
	Debug::Log("LOADED NEUTRAL.MIX\n");

	return 0x534F96;
}

// and passed the whole 49 byte name array by value here
DEFINE_HOOK(0x5FDDA4, IsOverlayIdxTiberium_Log, 0x6)
{
	GET(OverlayTypeClass* const, pType, EAX);

	Debug::Log("Overlay %s not really tiberium\n", pType->Name);

	return 0x5FDDC1;
}
