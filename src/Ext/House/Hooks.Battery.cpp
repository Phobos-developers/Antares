#include "Body.h"

#include <BuildingClass.h>

#include <algorithm>

static_assert(offsetof(BuildingClass, Type) == 0x520, "BuildingClass layout slipped");
static_assert(offsetof(BuildingClass, IsOverpowered) == 0x661, "BuildingClass layout slipped");
static_assert(offsetof(HouseClass, PowerOutput) == 0x53A4, "HouseClass layout slipped");
static_assert(offsetof(HouseClass, PowerDrain) == 0x53A8, "HouseClass layout slipped");

DEFINE_HOOK(0x44019D, BuildingClass_Update_Battery, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	auto const pExt = HouseExt::ExtMap.Find(pThis->Owner);
	auto const& types = pExt->Battery_Overpower;

	if(std::find(types.begin(), types.end(), pThis->Type) != types.end()) {
		pThis->IsOverpowered = true;
	}

	return 0;
}

DEFINE_HOOK(0x4555D5, BuildingClass_IsPowerOnline_KeepOnline, 0x5)
{
	GET(BuildingClass*, pThis, ESI);

	auto const pExt = HouseExt::ExtMap.Find(pThis->Owner);
	auto const& types = pExt->Battery_KeepOnline;

	auto const kept = std::find(types.begin(), types.end(), pThis->Type) != types.end();

	R->EDI(kept ? 0 : 2);

	return 0x4555DA;
}

DEFINE_HOOK(0x508C7F, HouseClass_UpdatePower_Auxiliary, 0x6)
{
	GET(HouseClass*, pThis, ESI);

	auto const pExt = HouseExt::ExtMap.Find(pThis);
	auto const power = pExt->AuxPower;

	pThis->PowerOutput = (power > 0) ? power : 0;
	pThis->PowerDrain = (power < 0) ? -power : 0;

	return 0x508C8B;
}

DEFINE_HOOK(0x508E66, HouseClass_UpdateRadar_Battery, 0x8)
{
	GET(HouseClass*, pThis, ECX);

	auto const pExt = HouseExt::ExtMap.Find(pThis);

	return pExt->BatteriesActive > 0 ? 0x508E87 : 0x508F2F;
}
