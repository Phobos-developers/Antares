#include "Body.h"
#include "../../Utilities/AresEnums.h"

#include "../Rules/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"

#include <BuildingClass.h>
#include <HouseClass.h>
#include <ObjectClass.h>
#include <ParasiteClass.h>
#include <ScenarioClass.h>
#include <TagClass.h>
#include <TechnoClass.h>
#include <VocClass.h>

#include <cstdlib>

DEFINE_HOOK(0x4892BE, DamageArea_NullDamage, 0x6)
{
	GET_BASE(WarheadTypeClass*, pWH, 0xC);
	GET(int, damage, ESI);

	if(!pWH || ScenarioClass::Instance->SpecialFlags.Inert) {
		return 0x48A4B7;
	}

	if(!damage && !WarheadTypeExt::ExtMap.Find(pWH)->AllowZeroDamage) {
		return 0x48A4B7;
	}

	R->ESI(pWH);

	return 0x4892DD;
}

DEFINE_HOOK(0x489E9F, DamageArea_BridgeAbsoluteDestroyer, 0x5)
{
	GET(WarheadTypeClass*, pWH, EBX);
	GET(WarheadTypeClass*, pIonCannon, EDI);

	auto const pExt = WarheadTypeExt::ExtMap.Find(pWH);
	R->Stack(0x13, pExt->BridgeAbsoluteDestroyer.Get(pWH == pIonCannon));

	return 0x489EA4;
}

DEFINE_HOOK(0x489FD8, DamageArea_BridgeAbsoluteDestroyer2, 0x6)
{
	return R->Stack<bool>(0xF) ? 0x48A004 : 0x489FE0;
}

DEFINE_HOOK(0x48A15D, DamageArea_BridgeAbsoluteDestroyer3, 0x6)
{
	return R->Stack<bool>(0xF) ? 0x48A188 : 0x48A165;
}

DEFINE_HOOK(0x48A229, DamageArea_BridgeAbsoluteDestroyer4, 0x6)
{
	return R->Stack<bool>(0xF) ? 0x48A250 : 0x48A231;
}

DEFINE_HOOK(0x48A283, DamageArea_BridgeAbsoluteDestroyer5, 0x6)
{
	return R->Stack<bool>(0xF) ? 0x48A2AA : 0x48A28B;
}

DEFINE_HOOK(0x48A4F9, SelectDamageAnimation_FixNegatives, 0x6)
{
	GET(int, damage, EDI);

	R->EDI(std::abs(damage));

	return damage ? 0x48A4FF : 0x48A618;
}

DEFINE_HOOK(0x44266B, BuildingClass_ReceiveDamage_Destroyed, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(ObjectClass*, pKiller, EBP);

	pThis->Destroyed(pKiller);

	return 0;
}

DEFINE_HOOK(0x442974, BuildingClass_ReceiveDamage_Malicious, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET_STACK(WarheadTypeClass*, pWH, 0xA8);

	WarheadTypeExt::ReceiveDamage_WH = pWH;
	pThis->Owner->BuildingUnderAttack(pThis);
	WarheadTypeExt::ReceiveDamage_WH = nullptr;

	return 0x442980;
}

DEFINE_HOOK(0x5F53E5, ObjectClass_ReceiveDamage_Relative, 0x6)
{
	GET(ObjectClass*, pThis, ESI);
	GET(int*, pDamage, EDI);
	GET_STACK(WarheadTypeClass*, pWH, 0x30);

	if(pWH) {
		auto const pExt = WarheadTypeExt::ExtMap.Find(pWH);

		if(pExt->RelativeDamage) {
			auto const damage = pExt->CalculateRelativeDamage(pThis);
			*pDamage = (*pDamage < 0) ? -damage : damage;
		}
	}

	return 0;
}

DEFINE_HOOK(0x5F5456, ObjectClass_ReceiveDamage_Culling, 0x6)
{
	GET(ObjectClass*, pThis, ESI);
	GET(int*, pDamage, EDI);
	GET(bool, ignoreDefenses, EBX);
	GET_STACK(WarheadTypeClass*, pWH, 0x30);
	GET_STACK(TechnoClass*, pAttacker, 0x34);

	if(!ignoreDefenses && pWH && pAttacker && *pDamage > 0) {
		if(WarheadTypeExt::ExtMap.Find(pWH)->ApplyCulling(pAttacker, pThis)) {
			*pDamage = pThis->Health;
		}
	}

	auto const damage = *pDamage;
	R->ECX(damage);

	if(!damage) {
		return 0x5F545C;
	}

	return (damage > 0) ? 0x5F5498 : 0x5F546A;
}

DEFINE_HOOK(0x5F57B5, ObjectClass_ReceiveDamage_Trigger, 0x6)
{
	GET(ObjectClass*, pThis, ESI);
	GET(TechnoClass*, pAttacker, EDI);
	GET(DamageState, state, EBP);

	auto const raise = [pThis, pAttacker](TriggerEvent const event) {
		if(pThis->IsAlive) {
			if(auto const pTag = pThis->AttachedTag) {
				pTag->RaiseEvent(event, pThis, CellStruct::Empty, false, pAttacker);
				return true;
			}
		}
		return false;
	};

	if(state != DamageState::NowDead) {
		if(raise(TriggerEvent::AttackedByAnybody)) {
			raise(TriggerEvent::AttackedByHouse);
		}
	}

	if(raise(AresTriggerEvent::AttackedOrDestroyedByHouse)) {
		raise(AresTriggerEvent::AttackedOrDestroyedByAnybody);
	}

	return 0x5F580C;
}

DEFINE_HOOK(0x701914, TechnoClass_ReceiveDamage_Damaging, 0x7)
{
	GET(int, damage, EAX);

	R->Stack(0xE, damage > 0);

	return 0;
}

DEFINE_HOOK(0x701A5C, TechnoClass_ReceiveDamage_IronCurtainFlash, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(WarheadTypeClass*, pWH, 0xD0);

	if(pWH) {
		auto const pExt = WarheadTypeExt::ExtMap.Find(pWH);

		if(!pExt->IC_Flash.Get(RulesExt::Global()->IronCurtainFlash)) {
			return 0x701A98;
		}
	}

	return (pThis->ForceShielded == 1) ? 0x701A65 : 0x701A69;
}

DEFINE_HOOK(0x702050, TechnoClass_ReceiveDamage_SuppressUnitLost, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(WarheadTypeClass*, pWH, 0xD0);

	if(pWH && WarheadTypeExt::ExtMap.Find(pWH)->UnitLost_Suppress) {
		TechnoExt::ExtMap.Find(pThis)->SuppressLossMessage = true;
	}

	return 0;
}

DEFINE_HOOK(0x702185, TechnoClass_ReceiveDamage_OverrideVoiceDie, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(WarheadTypeClass*, pWH, 0xD0);

	if(pWH) {
		auto const pExt = WarheadTypeExt::ExtMap.Find(pWH);

		if(pExt->VoiceDie_Override.isset()) {
			auto const index = pExt->VoiceDie_Override.Get();
			if(index >= 0) {
				VocClass::PlayAt(index, pThis->Location, nullptr);
			}
			return 0x702190;
		}
	}

	return 0;
}

DEFINE_HOOK(0x7021F5, TechnoClass_ReceiveDamage_OverrideDieSound, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(WarheadTypeClass*, pWH, 0xD0);

	if(pWH) {
		auto const pExt = WarheadTypeExt::ExtMap.Find(pWH);

		if(pExt->DieSound_Override.isset()) {
			auto const index = pExt->DieSound_Override.Get();
			if(index >= 0) {
				VocClass::PlayAt(index, pThis->Location, nullptr);
			}
			return 0x702200;
		}
	}

	return 0;
}

DEFINE_HOOK(0x702819, TechnoClass_ReceiveDamage_Aftermath, 0xA)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(int*, pDamage, 0xC8);
	GET_STACK(WarheadTypeClass*, pWH, 0xD0);
	GET_STACK(TechnoClass*, pAttacker, 0xD4);
	GET_STACK(bool, ignoreDefenses, 0xD8);
	GET_STACK(bool, damaging, 0x12);
	GET_STACK(DamageState, state, 0x20);

	auto ret = 0u;
	auto shrugged = false;

	// the shimmer is for damage that actually landed
	if(state == DamageState::Unaffected) {
		ret = 0x702823u;
		shrugged = !ignoreDefenses && damaging && !*pDamage;
	}

	auto const pType = pThis->GetTechnoType();
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if(state != DamageState::Unaffected && damaging) {
		auto const delay = TechnoTypeExt::ExtMap.Find(pType)->SelfHealing_CombatDelay;
		if(delay > 0) {
			pExt->SelfHealCombatTimer.Start(delay);
		}
	}

	if(!pWH) {
		return ret;
	}

	auto const pWHExt = WarheadTypeExt::ExtMap.Find(pWH);

	if(shrugged && pWHExt->EffectsRequireDamage) {
		return ret;
	}

	if(pWHExt->EffectsRequireVerses
		&& pWHExt->GetVerses(pType->Armor).Verses < 0.0001)
	{
		return ret;
	}

	pWHExt->applyKillDriver(pAttacker, pThis);

	auto const sonar = pWHExt->Sonar_Duration;
	if(sonar > 0 && sonar > pExt->CloakSkipTimer.GetTimeLeft()) {
		pExt->CloakSkipTimer.Start(sonar);
	}

	auto const disable = pWHExt->DisableWeapons_Duration;
	if(disable > 0 && disable > pExt->DisableWeaponTimer.GetTimeLeft()) {
		pExt->DisableWeaponTimer.Start(disable);
	}

	auto const flash = pWHExt->Flash_Duration;
	if(flash > 0 && flash > pThis->Flashing.DurationRemaining) {
		pThis->Flash(flash);
	}

	return ret;
}

// weapons stay silent while a DisableWeapons warhead's effect lasts
DEFINE_HOOK(0x6FC0D3, TechnoClass_CanFire_DisableWeapons, 0x8)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	return pExt->DisableWeaponTimer.InProgress() ? 0x6FC0DF : 0;
}

DEFINE_HOOK(0x629BC0, ParasiteClass_UpdateSquiddy_Culling, 0x8)
{
	GET(ParasiteClass*, pThis, ESI);
	GET(WarheadTypeClass*, pWH, EDI);

	auto const pAttacker = pThis->Owner;
	auto const pVictim = pThis->Victim;

	if(!WarheadTypeExt::ExtMap.Find(pWH)->ApplyCulling(pAttacker, pVictim)) {
		return 0x629D19;
	}

	auto const pHouse = pAttacker->Owner;
	auto const allied = pHouse && pVictim->Owner->IsAlliedWith(pHouse);

	return allied ? 0x629C5D : 0x629BF3;
}
