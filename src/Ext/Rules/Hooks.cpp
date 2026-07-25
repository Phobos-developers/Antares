#include "Body.h"
#include "../../Ares.h"

#include <OverlayClass.h>

DEFINE_HOOK(0x668BF0, RulesClass_Addition, 0x5)
{
	GET(RulesClass*, pItem, ECX);
	GET_STACK(CCINIClass*, pINI, 0x4);

//	RulesClass::Initialized = false;
	RulesExt::LoadFromINIFile(pItem, pINI);
	return 0;
}

DEFINE_HOOK(0x679A15, RulesData_LoadBeforeTypeData, 0x6)
{
	GET(RulesClass*, pItem, ECX);
	GET_STACK(CCINIClass*, pINI, 0x4);

//	RulesClass::Initialized = true;
	RulesExt::LoadBeforeTypeData(pItem, pINI);
	return 0;
}

DEFINE_HOOK(0x679CAF, RulesData_LoadAfterTypeData, 0x5)
{
	RulesClass* pItem = RulesClass::Instance;
	GET(CCINIClass*, pINI, ESI);

	RulesExt::LoadAfterTypeData(pItem, pINI);
	return 0;
}

DEFINE_HOOK(0x518744, InfantryClass_ReceiveDamage_ElectricDeath, 0x6)
{
	AnimTypeClass *El = RulesExt::Global()->ElectricDeath;
	if(!El) {
		El = AnimTypeClass::Find("ELECTRO");
	}
	if(!El) {
		El = AnimTypeClass::Array.GetItem(1);
	}

	R->EDX(El);
	return 0x51874D;
}

DEFINE_HOOK(0x48A2D9, DamageArea_ExplodesThreshold, 0x6)
{
	GET(OverlayTypeClass*, pOverlay, EAX);
	GET_STACK(int, damage, 0x24);

	bool explodes = pOverlay->Explodes && damage >= RulesExt::Global()->OverlayExplodeThreshold;

	return explodes ? 0x48A2E7 : 0x48A433;
}

// TiberiumTransmogrify is never initialized explitly, thus do that here
DEFINE_HOOK(0x66748A, RulesClass_CTOR_TiberiumTransmogrify, 0x6)
{
	GET(RulesClass*, pThis, ESI);
	pThis->TiberiumTransmogrify = 0;
	return 0;
}
