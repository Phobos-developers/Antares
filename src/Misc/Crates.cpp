#include "../Ext/Rules/Body.h"
#include "../Ext/Techno/Body.h"
#include "../Ext/TechnoType/Body.h"

#include <HouseClass.h>
#include <Powerups.h>
#include <ScenarioClass.h>

/*
	generic crate-handler file
	currently used only to shim crates into TechnoExt
	since Techno fields are used by AttachEffect

	Graion Dilach, 2013-05-31
*/

//overrides for crate checks
//481D52 - pass
//481C86 - override with Money

// the money bonus is varied by a configurable amount instead of a fixed 900
DEFINE_HOOK(0x48248D, CellClass_CrateBeingCollected_MoneyRandom, 0x6)
{
	GET(int const, money, EAX);

	auto const bonus = ScenarioClass::Instance->Random.RandomRanged(
		0, RulesExt::Global()->RandomCrateMoney);

	R->EDI(money + bonus);
	return 0x4824A7;
}

DEFINE_HOOK(0x565215, MapClass_CTOR_NoInit_Crates, 0x6)
{
	return 0x56522D;
}

DEFINE_HOOK(0x481C6C, CellClass_CrateBeingCollected_Armor1, 0x6)
{
	GET(TechnoClass *, Unit, EDI);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (UnitExt->Crate_ArmorMultiplier == 1.0){
		return 0x481D52;
	}
	return 0x481C86;
}

DEFINE_HOOK(0x481CE1, CellClass_CrateBeingCollected_Speed1, 0x6)
{
	GET(FootClass *, Unit, EDI);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (UnitExt->Crate_SpeedMultiplier == 1.0){
		return 0x481D52;
	}
	return 0x481C86;
}

DEFINE_HOOK(0x481D0E, CellClass_CrateBeingCollected_Firepower1, 0x6)
{
	GET(TechnoClass *, Unit, EDI);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (UnitExt->Crate_FirepowerMultiplier == 1.0){
		return 0x481D52;
	}
	return 0x481C86;
}

DEFINE_HOOK(0x481D3D, CellClass_CrateBeingCollected_Cloak1, 0x6)
{
	GET(TechnoClass *, Unit, EDI);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (TechnoExt::CanICloakByDefault(Unit) || UnitExt->Crate_Cloakable){
		return 0x481C86;
	}

	auto pType = Unit->GetTechnoType();
	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	// cloaking forbidden for type
	if(!pTypeExt->CloakAllowed) {
		return 0x481C86;
	}

	return 0x481D52;
}

//overrides on actual crate effect applications

DEFINE_HOOK(0x48294F, CellClass_CrateBeingCollected_Cloak2, 0x7)
{
	GET(TechnoClass *, Unit, EDX);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	UnitExt->Crate_Cloakable = 1;
	UnitExt->RecalculateStats();
	return 0x482956;
}

DEFINE_HOOK(0x482E57, CellClass_CrateBeingCollected_Armor2, 0x6)
{
	GET(TechnoClass *, Unit, ECX);
	GET_STACK(double, Pow_ArmorMultiplier, 0x20);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (UnitExt->Crate_ArmorMultiplier == 1.0){
		UnitExt->Crate_ArmorMultiplier = Pow_ArmorMultiplier;
		UnitExt->RecalculateStats();
		R->AL(Unit->GetOwningHouse()->IsInPlayerControl);
		return 0x482E89;
	}
	return 0x482E92;
}


DEFINE_HOOK(0x48303A, CellClass_CrateBeingCollected_Speed2, 0x6)
{
	GET(FootClass *, Unit, EDI);
	GET_STACK(double, Pow_SpeedMultiplier, 0x20);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (UnitExt->Crate_SpeedMultiplier == 1.0 && Unit->WhatAmI() != AbstractType::AircraftType){
		UnitExt->Crate_SpeedMultiplier = Pow_SpeedMultiplier;
		UnitExt->RecalculateStats();
		R->CL(Unit->GetOwningHouse()->IsInPlayerControl);
		return 0x483078;
	}
	return 0x483081;
}

DEFINE_HOOK(0x483226, CellClass_CrateBeingCollected_Firepower2, 0x6)
{
	GET(TechnoClass *, Unit, ECX);
	GET_STACK(double, Pow_FirepowerMultiplier, 0x20);
	TechnoExt::ExtData *UnitExt = TechnoExt::ExtMap.Find(Unit);
	if (UnitExt->Crate_FirepowerMultiplier == 1.0){
		UnitExt->Crate_FirepowerMultiplier = Pow_FirepowerMultiplier;
		UnitExt->RecalculateStats();
		R->AL(Unit->GetOwningHouse()->IsInPlayerControl);
		return 0x483258;
	}
	return 0x483261;
}

