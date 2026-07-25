#include "Body.h"

#include <TechnoClass.h>
#include <UnitTypeClass.h>

// the vanilla loop only knows about the 18 slots the game has room for, so it is
// replaced wholesale by the reader that also fills the ext overflow vectors
DEFINE_HOOK(0x7128C0, TechnoTypeClass_LoadFromINI_Weapons1, 0x6)
{
	GET(TechnoTypeClass* const, pThis, EBP);
	GET(CCINIClass* const, pINI, ESI);

	TechnoTypeExt::ExtMap.Find(pThis)->ReadWeapons(pINI);

	return 0x712A8F;
}

// ReadWeapons already got the art keys, so skip the vanilla art pass
DEFINE_HOOK(0x715B1F, TechnoTypeClass_LoadFromINI_Weapons2, 0x6)
{
	return 0x715F9E;
}

DEFINE_HOOK(0x747BCF, UnitTypeClass_LoadFromINI_Turrets, 0x5)
{
	GET(UnitTypeClass* const, pThis, ESI);
	GET(CCINIClass* const, pINI, EBX);

	if(pThis->Gunner) {
		TechnoTypeExt::ExtMap.Find(pThis)->LoadTurrets(pINI);
	}

	return 0x747E90;
}

DEFINE_HOOK(0x7177C0, TechnoTypeClass_GetWeapon, 0x0B)
{
	GET_STACK(int const, index, 0x4);

	if(index < TechnoTypeClass::MaxWeapons) {
		return 0;
	}

	GET(TechnoTypeClass* const, pThis, ECX);

	R->EAX(TechnoTypeExt::ExtMap.Find(pThis)->GetWeapon(index, false));
	return 0x7177D4;
}

DEFINE_HOOK(0x7177E0, TechnoTypeClass_GetEliteWeapon, 0x0B)
{
	GET_STACK(int const, index, 0x4);

	if(index < TechnoTypeClass::MaxWeapons) {
		return 0;
	}

	GET(TechnoTypeClass* const, pThis, ECX);

	R->EAX(TechnoTypeExt::ExtMap.Find(pThis)->GetWeapon(index, true));
	return 0x7177F4;
}

DEFINE_HOOK(0x717890, TechnoTypeClass_SetWeaponTurretIndex, 0x8)
{
	GET(TechnoTypeClass* const, pThis, ECX);
	GET_STACK(int const, index, 0x4);
	GET_STACK(int const, weapon, 0x8);

	*TechnoTypeExt::ExtMap.Find(pThis)->GetWeaponTurretIndex(weapon) = index;

	return 0x71789F;
}

DEFINE_HOOK(0x7178B0, TechnoTypeClass_GetWeaponTurretIndex, 0x0B)
{
	GET(TechnoTypeClass* const, pThis, ECX);
	GET_STACK(int const, weapon, 0x4);

	R->EAX(*TechnoTypeExt::ExtMap.Find(pThis)->GetWeaponTurretIndex(weapon));

	return 0x7178BB;
}

DEFINE_HOOK(0x70DC70, TechnoClass_SwitchGunner, 0x6)
{
	GET(TechnoClass* const, pThis, ECX);
	GET_STACK(int, index, 0x4);

	auto const pType = pThis->GetTechnoType();

	if(!pType->IsChargeTurret) {
		if(index < 0 || index >= pType->WeaponCount) {
			index = 0;
		}

		auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
		pThis->CurrentTurretNumber = *pExt->GetWeaponTurretIndex(index);
		pThis->CurrentWeaponNumber = index;
	}

	return 0x70DCDB;
}
