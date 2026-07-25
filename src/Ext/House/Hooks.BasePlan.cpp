#include "Body.h"
#include <Utilities/Macro.h>   // STACK_OFFS
#include "../BuildingType/Body.h"
#include "../HouseType/Body.h"
#include "../Rules/Body.h"
#include "../Side/Body.h"

#include <BuildingTypeClass.h>
#include <GameModeOptionsClass.h>
#include <InfantryTypeClass.h>
#include <MPGameModeClass.h>
#include <ScenarioClass.h>
#include <UnitTypeClass.h>
#include <Unsorted.h>

#include <set>

static int const& BuildLevel = *reinterpret_cast<int const*>(0x822CF4);

static void UpdateAIBaseNodes(HouseClass* pHouse) {
	reinterpret_cast<void(__thiscall*)(HouseClass*)>(0x505180)(pHouse);
}

// #917 - validate build list before it needs to be generated
DEFINE_HOOK(0x5054B0, HouseClass_GenerateAIBuildList_EnsureSanity, 0x6)
{
	GET(HouseClass* const, pThis, ECX);

	auto const pExt = HouseExt::ExtMap.Find(pThis);
	pExt->CheckBasePlanSanity();

	// allow the list to be generated even if it will crash the game - sanity
	// check will log potential problems and thou shalt RTFLog
	return 0;
}

// fixes SWs not being available in campaigns if they have been turned off in a
// multiplayer mode
DEFINE_HOOK(0x5055D8, HouseClass_GenerateAIBuildList_SWAllowed, 0x5)
{
	auto const allowed = SessionClass::Instance.GameMode == GameMode::Campaign
		|| GameModeOptionsClass::Instance.SWAllowed;

	R->EAX(allowed);
	return 0x5055DD;
}

// #917 - stupid copying logic
/**
 * v2[0] = v1[0];
 * v2[1] = v1[1];
 * v2[2] = v1[2];
 * for(int i = 3; i < v1.Count; ++i) {
 *  v2[i] = v1[i];
 * }
 * care to guess what happens when v1.Count is < 3?
 *
 * fixed old fix, which was quite broken itself...
 */

DEFINE_HOOK(0x505B58, HouseClass_GenerateAIBuildList_SkipManualCopy, 0x6)
{
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, PlannedBase1, STACK_OFFS(0xA4, 0x90));
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, PlannedBase2, STACK_OFFS(0xA4, 0x78));
	PlannedBase2.SetCapacity(PlannedBase1.Capacity, nullptr);
	return 0x505C2C;
}

DEFINE_HOOK(0x505C34, HouseClass_GenerateAIBuildList_FullAutoCopy, 0x5)
{
	R->EDI(0);
	return 0x505C39;
}

DEFINE_HOOK(0x505C95, HouseClass_GenerateAIBuildList_CountExtra, 0x7)
{
	GET(HouseClass* const, pThis, EBX);
	GET_STACK(int const, idxSide, 0x80);
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, BuildList, STACK_OFFS(0xA4, 0x78));

	auto const idxDifficulty = pThis->GetAIDifficultyIndex();
	auto& Random = ScenarioClass::Instance->Random;

	// optionally add the same buildings more than once, but ignore the
	// construction yard at index 0
	for(auto i = 1; i < BuildList.Count; ++i) {
		auto const pItem = BuildList[i];

		// only handle if occurs for the first time, otherwise we have an
		// escalating probability of desaster.
		auto const handled = make_iterator(BuildList.begin(), i);

		if(!handled.contains(pItem)) {
			auto const pExt = BuildingTypeExt::ExtMap.Find(pItem);
			if(idxDifficulty < pExt->AIBuildCounts.size()) {
				// fixed number of buildings, one minimum (exists already)
				auto count = Math::max(pExt->AIBuildCounts[idxDifficulty], 1);

				// random optional building counts
				if(idxDifficulty < pExt->AIExtraCounts.size()) {
					auto const& max = pExt->AIExtraCounts[idxDifficulty];
					count += Random.RandomRanged(0, Math::max(max, 0));
				}

				// account for the one that already exists
				for(auto j = 1; j < count; ++j) {
					auto const idx = Random.RandomRanged(
						i + 1, BuildList.Count);
					BuildList.AddItem(pItem);
					std::rotate(BuildList.begin() + idx, BuildList.end() - 1,
						BuildList.end());
				}
			}
		}
	}

	if(idxSide >= 0) {
		auto const pExt = SideExt::ExtMap.Find(SideClass::Array.GetItem(idxSide));

		auto const it = pExt->GetBaseDefenseCounts();
		if(idxDifficulty < it.size()) {
			R->EAX(it.at(idxDifficulty));
			return 0x505CE9;
		} else {
			Debug::Log("WTF! vector has %u items, requested item #%u\n",
				it.size(), idxDifficulty);
		}
	}

	return 0;
}

// I am crying all inside
DEFINE_HOOK(0x505CF1, HouseClass_GenerateAIBuildList_PadWithN1, 0x5)
{
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, PlannedBase2, STACK_OFFS(0xA4, 0x78));
	GET(int, DefenseCount, EAX);
	while(PlannedBase2.Count <= 3) {
		PlannedBase2.AddItem(reinterpret_cast<BuildingTypeClass*>(-1));
		--DefenseCount;
	}
	R->EDI(DefenseCount);
	R->EBX(-1);
	return (DefenseCount > 0) ? 0x505CF6u : 0x505D8Du;
}

// replaced the entire function, to have one centralized implementation
DEFINE_HOOK(0x5051E0, HouseClass_FirstBuildableFromArray, 0x5)
{
	GET(HouseClass const* const, pThis, ECX);
	GET_STACK(const DynamicVectorClass<BuildingTypeClass*>* const, pList, 0x4);

	auto const idxParentCountry = pThis->Type->FindParentCountryIndex();
	auto const pItem = HouseExt::FindBuildable(
		pThis, idxParentCountry, make_iterator(*pList));

	R->EAX(pItem);
	return 0x505300;
}

// #917 - handle the case of no shipyard gracefully
DEFINE_HOOK(0x50610E, HouseClass_FindPositionForBuilding_FixShipyard, 0x7)
{
	GET(BuildingTypeClass *, pShipyard, EAX);
	if(pShipyard) {
		R->ESI<int>(pShipyard->GetFoundationWidth() + 2);
		R->EAX<int>(pShipyard->GetFoundationHeight(false));
		return 0x506134;
	} else {
		return 0x5060CE;
	}
}

// prefer the inner base for cloak generators and whatever else asks for it
DEFINE_HOOK(0x506306, HouseClass_FindPlaceToBuild_Evaluate, 0x6)
{
	GET(BuildingTypeClass* const, pType, EDX);

	auto const pExt = BuildingTypeExt::ExtMap.Find(pType);
	R->CL(pExt->AIInnerBase.Get(pType->CloakGenerator));
	return 0x50630C;
}

// don't crash if you can't find a base unit
// I imagine we'll have a pile of hooks like this sooner or later
DEFINE_HOOK(0x4F65BF, HouseClass_CanAffordBase, 0x6)
{
	GET(UnitTypeClass*, pBaseUnit, ECX);
	if(pBaseUnit) {
		return 0;
	}
//	GET(HouseClass *, pHouse, ESI);
//	Debug::Log(Debug::Error, "AI House of country [%s] cannot build anything from [General]BaseUnit=.\n", pHouse->Type->ID);
	return 0x4F65DA;
}

DEFINE_HOOK(0x5D705E, MPGameMode_SpawnBaseUnit_BaseUnit, 0x6)
{
	enum { hasBaseUnit = 0x5D7084, hasNoBaseUnit = 0x5D70DB };

	GET(HouseClass *, pHouse, EDI);
	GET(UnitTypeClass *, pBaseUnit, EAX);
	if(!pBaseUnit) {
		Debug::Log(Debug::Severity::Fatal, "House of country [%s] cannot build anything from [General]BaseUnit=.\n", pHouse->Type->ID);
		return hasNoBaseUnit;
	}

	auto Unit = static_cast<UnitClass *>(pBaseUnit->CreateObject(pHouse));
	R->ESI<UnitClass *>(Unit);
	return hasBaseUnit;
}

DEFINE_HOOK(0x688B37, MPGameModeClass_CreateStartingUnits_B, 0x5)
{
	enum { hasBaseUnit = 0x688B75, hasNoBaseUnit = 0x688C09 };

	GET_STACK(HouseClass *, pHouse, 0x10);

	auto pArray = &RulesClass::Instance->BaseUnit;
	bool canBuild = false;
	UnitTypeClass* Item = nullptr;
	auto const idxParent = pHouse->Type->FindParentCountryIndex();
	for(int i = 0; i < pArray->Count; ++i) {
		Item = pArray->GetItem(i);
		if(pHouse->CanExpectToBuild(Item, idxParent)) {
			canBuild = true;
			break;
		}
	}
	if(!canBuild) {
		Debug::Log(Debug::Severity::Fatal, "House of country [%s] cannot build anything from [General]BaseUnit=.\n", pHouse->Type->ID);

		return hasNoBaseUnit;
	}

	auto Unit = static_cast<UnitClass *>(Item->CreateObject(pHouse));
	R->ESI<UnitClass *>(Unit);
	R->EBP(0);
	R->EDI<HouseClass *>(pHouse);
	return hasBaseUnit;
}

// the average unit cost the unit count setting is multiplied with. only the
// types the participating countries could actually start with are considered.
DEFINE_HOOK(0x5D6D9A, MPGameModeClass_CreateStartingUnits_UnitCost, 0x6)
{
	auto const pRules = RulesExt::Global();

	auto cost = 0;

	if(pRules->StartInMultiplayerUnitCost.isset()) {
		cost = pRules->StartInMultiplayerUnitCost;
	} else {
		std::set<TechnoTypeClass*> types;
		unsigned int owners = 0;

		// countries with an explicit list contribute their own types, the
		// others are looked up using their owner bit
		for(auto const pHouse : HouseClass::Array) {
			auto const pType = pHouse->Type;

			if(pType->MultiplayPassive) {
				continue;
			}

			auto const pExt = HouseTypeExt::ExtMap.Find(pType);

			if(pExt->StartInMultiplayer_Types.HasValue()) {
				for(auto const pItem : pExt->StartInMultiplayer_Types) {
					auto const abs = pItem->WhatAmI();
					if(abs == AbstractType::InfantryType
						|| abs == AbstractType::UnitType)
					{
						types.insert(pItem);
					}
				}
			} else {
				owners |= 1u << pType->ArrayIndex;
			}
		}

		auto const AddOwnable = [&types, owners](auto const& items) {
			for(auto const pItem : items) {
				if(pItem->AllowedToStartInMultiplayer
					&& (pItem->GetOwners() & owners)
					&& pItem->TechLevel <= BuildLevel)
				{
					types.insert(pItem);
				}
			}
		};

		AddOwnable(make_iterator(UnitTypeClass::Array));
		AddOwnable(make_iterator(InfantryTypeClass::Array));

		// base units are placed separately and do not count
		for(auto const pBaseUnit : RulesClass::Instance->BaseUnit) {
			types.erase(pBaseUnit);
		}

		auto total = 0;
		for(auto const pItem : types) {
			total += pItem->GetCost();
		}

		auto const count = types.size();
		auto const divisor = count ? count : 1u;
		cost = static_cast<int>((total + divisor / 2) / divisor);

		Debug::Log("Unit cost of %d derived from %u units totalling %d credits.\n",
			cost, count, total);
	}

	R->EBP(cost * GameModeOptionsClass::Instance.UnitCount);
	return 0x5D6ED6;
}

// let pre-placed construction yards finish their buildup, and keep the spawn
// cell the base unit or construction yard was put on
DEFINE_HOOK(0x5D6F61, MPGameModeClass_CreateStartingUnits_BaseCenter, 0x8)
{
	enum { Spawned = 0x5D6F77, Failed = 0x5D701B };

	GET(MPGameModeClass* const, pThis, ECX);
	GET(HouseClass* const, pHouse, ESI);
	GET(int* const, pMoney, EAX);
	GET(int const, money, EBP);

	*pMoney = money;

	auto const cell = pHouse->BaseSpawnCell;

	if(!pThis->SpawnBaseUnits(pHouse, reinterpret_cast<DWORD>(pMoney))) {
		return Failed;
	}

	for(auto const pConYard : pHouse->ConYards) {
		pConYard->QueueMission(Mission::Construction, true);
		++Unsorted::ScenarioInit;
		pConYard->EnterIdleMode(0, 1);
		--Unsorted::ScenarioInit;
	}

	if(pHouse->BaseSpawnCell == CellStruct::Empty) {
		pHouse->BaseSpawnCell = cell;
	}

	return Spawned;
}

// start with the first buildable item from BuildConst instead of a base unit
DEFINE_HOOK(0x5D7048, MPGameMode_SpawnBaseUnit_BuildConst, 0x5)
{
	enum { Created = 0x5D707E, Failed = 0x5D70DB };

	GET_STACK(HouseClass* const, pHouse, 0x18);

	auto const pTypeExt = HouseTypeExt::ExtMap.Find(pHouse->Type);
	if(!pTypeExt->StartInMultiplayer_WithConst) {
		return 0;
	}

	auto const idxParent = pHouse->Type->FindParentCountryIndex();
	auto const pType = HouseExt::FindBuildable(pHouse, idxParent,
		make_iterator(RulesClass::Instance->BuildConst));

	if(!pType) {
		Debug::Log(Debug::Severity::Fatal, "House of country [%s] cannot build anything from [General]BuildConst=.\n", pHouse->Type->ID);
		return Failed;
	}

	auto const pBuilding = static_cast<BuildingClass*>(pType->CreateObject(pHouse));
	if(!pBuilding) {
		return Failed;
	}

	pBuilding->ForceMission(Mission::Guard);

	// larger buildings are centered on the spawn cell
	if(pType->GetFoundationWidth() > 2 || pType->GetFoundationHeight(false) > 2) {
		--pHouse->BaseSpawnCell.X;
		--pHouse->BaseSpawnCell.Y;
	}

	if(!pHouse->IsControlledByHuman()) {
		UpdateAIBaseNodes(pHouse);

		auto const cell = pHouse->GetBaseCenter();
		pHouse->Base.Center = cell;
		pHouse->Base.BaseNodes.Items->MapCoords = cell;
		pHouse->Production = true;
		pHouse->AITriggersActive = true;
		pHouse->AutoBaseBuilding = true;
	}

	R->EAX(pBuilding);
	R->EDI(pHouse);
	return Created;
}

// use the explicit list of starting units, if there is one
DEFINE_HOOK(0x5D7163, MPGameMode_SpawnStartingUnits_Types, 0x8)
{
	enum { FindUnits = 0x5D716B, FindInfantry = 0x5D721A, Spawn = 0x5D72AB, Done = 0x5D743E };

	REF_STACK(DynamicVectorClass<UnitTypeClass*>, Units, STACK_OFFS(0x48, 0x18));
	REF_STACK(DynamicVectorClass<InfantryTypeClass*>, Infantry, STACK_OFFS(0x48, 0x30));
	GET_STACK(HouseClass* const, pHouse, 0x4C);

	auto const pExt = HouseTypeExt::ExtMap.Find(pHouse->Type);
	auto const& items = pExt->StartInMultiplayer_Types;

	if(!items.HasValue()) {
		return UnitTypeClass::Array.Count ? FindUnits : FindInfantry;
	}

	for(auto const pType : items) {
		if(auto const pUnit = abstract_cast<UnitTypeClass*>(pType)) {
			Units.AddItem(pUnit);
		} else if(auto const pInfantry = abstract_cast<InfantryTypeClass*>(pType)) {
			Infantry.AddItem(pInfantry);
		}
	}

	return items.empty() ? Done : Spawn;
}

// support countries that have no starting infantry at all
DEFINE_HOOK(0x5D7337, MPGameMode_SpawnStartingUnits_NoInfantry, 0x5)
{
	enum { PickUnit = 0x5D734F };

	GET_STACK(int const, InfantryCount, STACK_OFFS(0x48, 0x20));

	return InfantryCount ? 0 : PickUnit;
}
