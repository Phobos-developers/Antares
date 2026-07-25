#include <AnimClass.h>
#include <Utilities/Macro.h>   // STACK_OFFS
#include <InfantryClass.h>
#include <IonBlastClass.h>
#include <MapClass.h>
#include <ScenarioClass.h>
#include <TemporalClass.h>
#include <WeaponTypeClass.h>
#include <HouseTypeClass.h>
#include <HouseClass.h>
#include <SideClass.h>
#include "Body.h"
#include "../Techno/Body.h"
#include "../Bullet/Body.h"
#include "../WeaponType/Body.h"
#include "../../Enum/ArmorTypes.h"

// feature #384: Permanent MindControl Warheads + feature #200: EMP Warheads
// attach #407 here - set TechnoClass::Flashing.Duration // that doesn't exist, according to yrpp::TechnoClass.h::struct FlashData
// attach #561 here, reuse #407's additional hooks for colouring
DEFINE_HOOK(0x46920B, BulletClass_Detonate, 0x6) {
	GET(BulletClass* const, pThis, ESI);
	GET_BASE(const CoordStruct* const, pCoordsDetonation, 0x8);

	auto const pWarhead = pThis->WH;
	auto const pWHExt = WarheadTypeExt::ExtMap.Find(pWarhead);

	auto const pOwnerHouse = pThis->Owner ? pThis->Owner->Owner : nullptr;

	// this snapping stuff does not belong here. it should go into BulletClass::Fire
	auto coords = *pCoordsDetonation;
	auto snapped = false;

	static auto const SnapDistance = 64;
	if(pThis->Target && pThis->DistanceFrom(pThis->Target) < SnapDistance) {
		coords = pThis->Target->GetCoords();
		snapped = true;
	}

	// these effects should be applied no matter what happens to the target
	pWHExt->applyIonCannon(coords);

	bool targetStillOnMap = true;
	if(snapped) {
		if(auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pThis->WeaponType)) {
			targetStillOnMap = !pWeaponExt->conductAbduction(pThis);
		}
	}

	// if the target gets abducted, there's nothing there to apply IC, EMP, etc. to
	// mind that conductAbduction() neuters the bullet, so if you wish to change
	// this check, you have to fix that as well
	if(targetStillOnMap) {
		auto const damage = pThis->WeaponType ? pThis->WeaponType->Damage : 0;
		pWHExt->applyIronCurtain(coords, pOwnerHouse, damage);
		pWHExt->applyEMP(coords, pThis->Owner);
		pWHExt->applyAttachedEffect(coords, pOwnerHouse);

		// KillDriver is not applied here: 3.0p1 reaches it only from
		// TechnoClass_ReceiveDamage_Aftermath, so that it sees the damage
		// gates (EffectsRequireDamage / EffectsRequireVerses) first
		if(snapped) {
			WarheadTypeExt::applyOccupantDamage(pThis);
		}
	}

	return pWHExt->applyPermaMC(pOwnerHouse, pThis->Target) ? 0x469AA4u : 0u;
}

// issue 472: deglob WarpAway
DEFINE_HOOK(0x71A900, TemporalClass_Update_WarpAway, 0x6) {
	GET(TemporalClass* const, pThis, ESI);

	auto const pOwner = pThis->Owner;
	auto const pExt = TechnoExt::ExtMap.Find(pOwner);
	auto const pWeapon = pOwner->GetWeapon(pExt->idxSlot_Warp)->WeaponType;
	auto const pData = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);

	R->EDX<AnimTypeClass *>(pData->Temporal_WarpAway.Get(RulesClass::Instance->WarpAway));
	return 0x71A906;
}

DEFINE_HOOK(0x517FC1, InfantryClass_ReceiveDamage_DeployedDamage, 0x6) {
	GET(WarheadTypeClass *, WH, EBP);

	// the null test comes first, which is what 3.0p1 does: the old order
	// reached ExtMap.Find and the multiply with a null warhead and only then
	// asked whether the pointer was safe
	if(!WH) {
		return 0x518016;
	}

	GET(InfantryClass *, I, ESI);
	bool IgnoreDefenses = R->BL() != 0;

	if(IgnoreDefenses || !I->IsDeployed()) {
		return 0;
	}

	GET(int *, Damage, EDI);

	WarheadTypeExt::ExtData *pData = WarheadTypeExt::ExtMap.Find(WH);

	*Damage = static_cast<int>(*Damage * pData->DeployedDamage);

	return 0x517FF9u;
}
/*
 * Fixing issue #722
 */

DEFINE_HOOK(0x7384BD, UnitClass_ReceiveDamage_OreMinerUnderAttack, 0x6)
{
	GET_STACK(WarheadTypeClass *, WH, STACK_OFFS(0x44, -0xC));

	auto pData = WarheadTypeExt::ExtMap.Find(WH);
	return !pData->Malicious ? 0x738535u : 0u;
}

DEFINE_HOOK(0x4F94A5, HouseClass_BuildingUnderAttack_Malicious, 0x6)
{
	if(auto const pWH = WarheadTypeExt::ReceiveDamage_WH) {
		if(!WarheadTypeExt::ExtMap.Find(pWH)->Malicious) {
			return 0x4F95D4;
		}
	}
	return 0;
}

DEFINE_HOOK(0x702669, TechnoClass_ReceiveDamage_SuppressDeathWeapon, 0x9)
{
	GET(TechnoClass* const, pThis, ESI);
	GET_STACK(WarheadTypeClass* const, pWarhead, STACK_OFFS(0xC4, -0xC));

	auto const pExt = WarheadTypeExt::ExtMap.Find(pWarhead);
	auto const abs = pThis->WhatAmI();

	auto const suppressed =
		(abs == AbstractType::Unit && pExt->SuppressDeathWeapon_Vehicles)
		|| (abs == AbstractType::Infantry && pExt->SuppressDeathWeapon_Infantry)
		|| pExt->SuppressDeathWeapon.Contains(pThis->GetTechnoType());
	
	if(!suppressed) {
		pThis->FireDeathWeapon(0);
	}

	return 0x702672;
}

DEFINE_HOOK(0x46670F, BulletClass_Update_PreImpactAnim, 0x6)
{
	GET(BulletClass* const, pThis, EBP);

	if(!pThis->NextAnim) {
		return 0x46671D;
	}

	auto const pExt = WarheadTypeExt::ExtMap.Find(pThis->WH);

	if(pExt->PreImpactAnim_Moves) {
		auto const coords = pThis->NextAnim->GetCoords();

		pThis->Location = coords;
		pThis->Target = MapClass::Instance.GetCellAt(coords);
	}

	return 0x467FEE;
}
