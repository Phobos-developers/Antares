#include "Body.h"
#include "../TechnoType/Body.h"

#include <AircraftClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>

DEFINE_HOOK(0x51F716, InfantryClass_Mi_Unload_Undeploy, 0x5)
{
	GET(InfantryClass* const, pThis, ESI);
	GET(InfantryTypeClass* const, pType, ECX);

	if(pType->UndeployDelay < 0) {
		pThis->PlayAnim(Sequence::Undeploy, true, false);
	}

	R->EBX(1);
	return 0x51F7C9;
}

DEFINE_HOOK(0x739ADA, UnitClass_SimpleDeploy_Height, 0xA)
{
	GET(UnitClass* const, pThis, ESI);

	if(pThis->Deployed) {
		return 0x739CBF;
	}

	if(!pThis->InAir && pThis->Type->DeployToLand && pThis->GetHeight() > 0) {
		pThis->InAir = true;
	}

	R->EAX(1);
	return 0x739B14;
}

DEFINE_HOOK(0x73DE90, UnitClass_Mi_Unload_SimpleDeployer, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	if(pThis->Deployed) {
		auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->Type);
		if(auto const pToType = pExt->Convert_Deploy) {
			if(TechnoExt::UpdateType(pThis, pToType)) {
				pThis->Deployed = false;
			}
		}
	}

	// no early-out on Is_Moving_Now(). It pre-empts the game's own Deploying and
	// Undeploying checks below, so a turning unit never advances its deploy state,
	// Deployed stays set, and the conversion above runs again every frame.
	return 0;
}
