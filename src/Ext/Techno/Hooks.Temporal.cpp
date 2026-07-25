#include "Body.h"
#include "../Building/Body.h"
#include "../BuildingType/Body.h"
#include "../TechnoType/Body.h"
#include "../WarheadType/Body.h"
#include "../WeaponType/Body.h"

#include <BuildingClass.h>
#include <CellClass.h>
#include <HouseClass.h>
#include <TemporalClass.h>
#include <UnitClass.h>

// the weapons factory this object is still standing in, if any
BuildingClass* TechnoExt::IsInWarfactory(TechnoClass* const pThis, bool const checkNaval)
{
	if(!pThis || pThis->WhatAmI() != AbstractType::Unit) {
		return nullptr;
	}

	auto const pBuilding = pThis->GetCell()->GetBuilding();
	if(!pBuilding || pBuilding != pThis->GetNthLink()) {
		return nullptr;
	}

	auto const pType = pBuilding->Type;
	if(!pType->WeaponsFactory || (checkNaval && pType->Naval)) {
		return nullptr;
	}

	return pBuilding;
}

bool TechnoExt::IsWarpable(TechnoClass* const pThis)
{
	if(!pThis || pThis->IsIronCurtained()) {
		return false;
	}

	auto const pType = pThis->GetTechnoType();
	if(!pType->Warpable) {
		return false;
	}

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	if(pThis->Veterancy.IsElite()) {
		if(pTypeExt->EliteAbilities[AresAbility::Unwarpable]
			|| pTypeExt->VeteranAbilities[AresAbility::Unwarpable])
		{
			return false;
		}
	} else if(pThis->Veterancy.IsVeteran()
		&& pTypeExt->VeteranAbilities[AresAbility::Unwarpable])
	{
		return false;
	}

	// a building that is about to be chronoshifted would be removed by the
	// chronosphere and never come back, so its owner could not be defeated.
	if(auto const pBld = abstract_cast<BuildingClass*>(pThis)) {
		if(BuildingExt::ExtMap.Find(pBld)->AboutToChronoshift) {
			return false;
		}
	}

	return TechnoExt::IsInWarfactory(pThis, true) == nullptr;
}

int TechnoExt::GetWarpPerStep(TemporalClass* pThis, int helpers)
{
	auto damage = 0;

	while(pThis) {
		if(helpers > 50) {
			break;
		}
		++helpers;

		auto const pOwner = pThis->Owner;
		auto const pExt = TechnoExt::ExtMap.Find(pOwner);
		auto const pWeapon = pOwner->GetWeapon(pExt->idxSlot_Warp)->WeaponType;

		damage += pWeapon->Damage;
		pThis->WarpPerStep = pWeapon->Damage;
		pThis = pThis->PrevTemporal;
	}

	return damage;
}

// the warhead of the weapon this temporal was created from
static WarheadTypeExt::ExtData* GetTemporalWarheadExt(TemporalClass* const pThis)
{
	auto const pOwner = pThis->Owner;
	auto const pExt = TechnoExt::ExtMap.Find(pOwner);
	auto const pWeapon = pOwner->GetWeapon(pExt->idxSlot_Warp)->WeaponType;
	return WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
}

DEFINE_HOOK(0x71AB10, TemporalClass_GetWarpPerStep, 0x6)
{
	GET(TemporalClass* const, pThis, ECX);
	GET_STACK(int const, helpers, 0x4);

	R->EAX(TechnoExt::GetWarpPerStep(pThis, helpers));
	return 0x71AB57;
}

DEFINE_HOOK(0x71AE50, TemporalClass_CanWarpTarget, 0x8)
{
	GET_STACK(TechnoClass* const, pTarget, 0x4);

	R->EAX(TechnoExt::IsWarpable(pTarget));
	return 0x71AF19;
}

// bugfix #379: Temporal friendly kills give veterancy
// bugfix #1266: Temporal kills gain double experience
DEFINE_HOOK(0x71A917, TemporalClass_Update_Erase, 0x5)
{
	GET(TemporalClass* const, pThis, ESI);

	if(GetTemporalWarheadExt(pThis)->UnitLost_Suppress) {
		TechnoExt::ExtMap.Find(pThis->Target)->SuppressLossMessage = true;
	}

	return 0x71A97D;
}

DEFINE_HOOK(0x71AAAC, TemporalClass_Update_Abductor, 0x6)
{
	GET(TemporalClass* const, pThis, ESI);

	auto const pOwner = pThis->Owner;
	auto const pExt = TechnoExt::ExtMap.Find(pOwner);
	auto const pWeapon = pOwner->GetWeapon(pExt->idxSlot_Warp)->WeaponType;
	auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

	if(pWeaponExt->Abductor_Temporal && pWeaponExt->Abduct(pOwner, pThis->Target)) {
		return 0x71AAD5;
	}

	return 0;
}

DEFINE_HOOK(0x71AFB2, TemporalClass_Fire_HealthFactor, 0x5)
{
	GET(TemporalClass* const, pThis, ESI);
	GET(TechnoClass* const, pTarget, ECX);
	GET(int const, strength, EAX);

	auto const health = static_cast<double>(pTarget->Health);
	auto const maximum = static_cast<double>(pTarget->GetType()->Strength);
	auto const factor = (1.0 - health / maximum)
		* GetTemporalWarheadExt(pThis)->Temporal_HealthFactor;

	auto const duration = (1.0 - factor) * (10 * strength);

	R->EAX(duration > 1.0 ? static_cast<int>(duration) : 1);
	return 0x71AFB7;
}

DEFINE_HOOK(0x4521C8, BuildingClass_Disable_Temporal_Factories, 0x6)
{
	GET(BuildingClass* const, pThis, ECX);

	BuildingExt::UpdateFactoryQueues(pThis);

	return 0;
}

DEFINE_HOOK(0x452218, BuildingClass_Enable_Temporal_Factories, 0x6)
{
	GET(BuildingClass* const, pThis, ECX);

	BuildingExt::UpdateFactoryQueues(pThis);

	return 0;
}
