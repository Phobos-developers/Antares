#include "Body.h"

#include <HouseClass.h>
#include <TechnoClass.h>

// the two stat counter entry points also maintain the number of objects that
// keep the house alive in a Short Game.

DEFINE_HOOK(0x4FF563, HouseClass_RegisterTechnoLoss_StatCounters_KeepAlive, 0x6)
{
	GET(HouseClass* const, pThis, EDI);
	GET(TechnoClass* const, pTechno, ESI);

	auto const pExt = HouseExt::ExtMap.Find(pThis);
	auto const abs = pTechno->WhatAmI();

	auto const counted = pExt->KeepThisAlive(pTechno, abs, false);

	R->EAX(static_cast<int>(abs));

	return counted ? 0x4FF596 : 0x4FF6CE;
}

DEFINE_HOOK(0x4FF71B, HouseClass_RegisterTechnoGain_StatCounters_KeepAlive, 0x6)
{
	GET(HouseClass* const, pThis, EDI);
	GET(TechnoClass* const, pTechno, ESI);

	auto const pExt = HouseExt::ExtMap.Find(pThis);
	auto const abs = pTechno->WhatAmI();

	auto const counted = pExt->KeepThisAlive(pTechno, abs, true);

	R->EAX(static_cast<int>(abs));

	return counted ? 0x4FF748 : 0x4FF8C6;
}
