#include "Body.h"
#include "../TechnoType/Body.h"

// the killer's own type decides whether it loots at all
DEFINE_HOOK(0x702E64, TechnoClass_RegisterDestruction_Bounty, 0x6)
{
	GET(TechnoClass* const, pKiller, EDI);
	GET(TechnoClass* const, pVictim, ESI);

	if(pKiller) {
		auto const pExt = TechnoTypeExt::ExtMap.Find(pKiller->GetTechnoType());
		if(pExt->Bounty) {
			TechnoExt::ExtMap.Find(pVictim)->CalculateBounty(pKiller);
		}
	}

	return 0;
}
