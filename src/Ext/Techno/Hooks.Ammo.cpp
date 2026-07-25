#include "Body.h"
#include "../TechnoType/Body.h"
#include "../WeaponType/Body.h"
#include "../../Misc/Debug.h"
#include "../../Ares.h"

#include <BuildingClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>

// bugfix #471: InfantryTypes and BuildingTypes don't reload their ammo properly

DEFINE_HOOK(0x43FE8E, BuildingClass_Update_Reload, 0x6)
{
	GET(BuildingClass *, B, ESI);
	BuildingTypeClass *BType = B->Type;
	if(!BType->Hospital && !BType->Armory) { // TODO: rethink this
		B->Reload();
	}
	return 0x43FEBE;
}

DEFINE_HOOK(0x6FCFA4, TechnoClass_GetROF_BuildingHack, 0x5)
{
	//GET(TechnoClass *, T, ESI);
	// actual game code: if(auto B = specific_cast<BuildingClass *>(T)) { if(T->currentAmmo > 1) { return 1; } }
	// if the object being queried doesn't have a weapon (Armory/Hospital), it'll return 1 anyway

	return 0x6FCFC1;
}

// attached effects scale the rate of fire
DEFINE_HOOK(0x6FD0BF, TechnoClass_GetROF_AttachEffect, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	double factor = TechnoExt::ExtMap.Find(pThis)->AttachEffects_ROFMultiplier;
	__asm { fmul factor };

	return 0;
}

DEFINE_HOOK(0x5200D7, InfantryClass_UpdatePanic_DontReload, 0x6)
{
	return 0x52010B;
}

DEFINE_HOOK(0x51BCB2, InfantryClass_Update_Reload, 0x6)
{
	GET(InfantryClass *, I, ESI);
	if(I->InLimbo) {
		return 0x51BDCF;
	}
	I->Reload();
	return 0x51BCC0;
}

// only rearm if this weapon actually spent a round
DEFINE_HOOK(0x51DF8C, InfantryClass_Fire_Ammo, 0x6)
{
	GET(InfantryClass* const, pThis, ESI);
	GET_STACK(int const, idxWeapon, 0x10);

	auto const pWeapon = pThis->GetWeapon(idxWeapon)->WeaponType;

	if(WeaponTypeExt::ExtMap.Find(pWeapon)->Ammo > 0) {
		auto const ammo = pThis->Type->Ammo;
		if(ammo > 0 && pThis->Ammo < ammo) {
			pThis->StartReloading();
		}
	}

	return 0;
}

DEFINE_HOOK(0x7413FF, UnitClass_Fire_Ammo, 0x7)
{
	GET(UnitClass* const, pThis, ESI);
	GET_STACK(WeaponTypeClass* const, pWeapon, 0x28);

	if(WeaponTypeExt::ExtMap.Find(pWeapon)->Ammo > 0) {
		pThis->StartReloading();
	}

	return 0x741406;
}

// weapons can take more than one round of ammo
DEFINE_HOOK(0x6FCA0D, TechnoClass_CanFire_Ammo, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeapon, EBX);

	auto pExt = WeaponTypeExt::ExtMap.Find(pWeapon);

	return (pThis->Ammo < 0 || pThis->Ammo >= pExt->Ammo)
		? 0x6FCA26u : 0x6FCA17u;
}

// remove ammo rounds depending on weapon
DEFINE_HOOK(0x6FF656, TechnoClass_Fire_Ammo, 0xA)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(WeaponTypeClass const* const, pWeapon, EBX);

	if(TechnoExt::DecreaseAmmo(pThis, pWeapon) > 0) {
		if(auto const pBld = specific_cast<BuildingClass*>(pThis)) {
			auto const ammo = pBld->Type->Ammo;
			if(ammo > 0 && pBld->Ammo < ammo) {
				pBld->StartReloading();
			}
		}
	}

	return 0x6FF660;
}

// a type that runs out of ammo can fall back to a different weapon
DEFINE_HOOK(0x6F3410, TechnoClass_SelectWeapon_NoAmmoWeapon, 0x5)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pType = pThis->GetTechnoType();
	if(pType->Ammo < 0) {
		return 0;
	}

	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	if(pExt->NoAmmoWeapon < 0 || pThis->Ammo > pExt->NoAmmoAmount) {
		return 0;
	}

	R->EAX(pExt->NoAmmoWeapon);
	return 0x6F3406;
}

// variable amounts of rounds to reload
DEFINE_HOOK(0x6FB05B, TechnoClass_Reload_ReloadAmount, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	auto const pType = pThis->GetTechnoType();
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	int amount = pExt->ReloadAmount;
	if(!pThis->Ammo) {
		amount = pExt->EmptyReloadAmount.Get(amount);
	}

	// clamping to support negative values
	auto const ammo = pThis->Ammo + amount;
	pThis->Ammo = Math::clamp(ammo, 0, pType->Ammo);

	return 0x6FB061;
}
