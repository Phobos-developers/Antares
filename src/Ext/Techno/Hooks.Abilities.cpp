#include "Body.h"
#include "../TechnoType/Body.h"
#include "../WarheadType/Body.h"

#include <HouseClass.h>
#include <TechnoClass.h>
#include <WarheadTypeClass.h>

DEFINE_HOOK(0x4DA584, FootClass_Update_RadImmune, 0x7)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(TechnoTypeClass* const, pType, EAX);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	if(pExt->HasAbility(AresAbility::RadImmune, pThis->Veterancy)) {
		return 0x4DA63B;
	}

	return 0;
}

DEFINE_HOOK(0x6FC417, TechnoClass_CanFire_PsionicsImmune, 0x6)
{
	GET(TechnoTypeClass* const, pType, EAX);
	GET(TechnoClass* const, pTarget, EBP);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	auto immune = pType->ImmuneToPsionics
		|| pExt->HasAbility(AresAbility::PsionicsImmune, pTarget->Veterancy);

	if(pExt->ImmuneToBerserk.isset()) {
		immune = pExt->ImmuneToBerserk.Get();
	}

	R->ECX(immune);

	return 0x6FC41D;
}

DEFINE_HOOK(0x701BFE, TechnoClass_ReceiveDamage_Abilities, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(WarheadTypeClass* const, pWH, EBP);
	GET_STACK(TechnoClass* const, pSource, 0xD4);
	GET_STACK(HouseClass* const, pHouse, 0xE0);

	auto const pType = pThis->GetTechnoType();
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	auto const& veterancy = pThis->Veterancy;

	if(pWH->Radiation) {
		if(pType->ImmuneToRadiation
			|| pExt->HasAbility(AresAbility::RadImmune, veterancy))
		{
			return 0x701C1C;
		}
	}

	if(pWH->PsychicDamage) {
		if(pType->ImmuneToPsionicWeapons
			|| pExt->HasAbility(AresAbility::PsionicWeaponImmune, veterancy))
		{
			return 0x701C1C;
		}
	}

	if(pWH->Poison) {
		if(pType->ImmuneToPoison
			|| pExt->HasAbility(AresAbility::PoisonImmune, veterancy))
		{
			return 0x701C1C;
		}
	}

	// this replaces the game's plain AffectsAllies check
	if(auto const pSourceHouse = pSource ? pSource->Owner : pHouse) {
		if(!WarheadTypeExt::CanAffectTarget(pThis, pSourceHouse, pWH)) {
			return 0x701CC2;
		}
	}

	if(!pWH->Psychedelic) {
		return 0x701DCC;
	}

	if(pThis->Owner->IsAlliedWith(pHouse)) {
		return 0x701CFC;
	}

	auto immune = pType->ImmuneToPsionics
		|| pExt->HasAbility(AresAbility::PsionicsImmune, veterancy);

	if(pExt->ImmuneToBerserk.isset()) {
		immune = pExt->ImmuneToBerserk.Get();
	}

	return immune ? 0x701CFC : 0x701D2E;
}
