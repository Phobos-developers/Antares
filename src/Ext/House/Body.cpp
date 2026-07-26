#include "Body.h"
#include "../HouseType/Body.h"
#include "../Building/Body.h"
#include "../BuildingType/Body.h"
#include "../Rules/Body.h"
#include "../Side/Body.h"
#include "../SWType/Body.h"
#include "../TechnoType/Body.h"
#include "../../Enum/Prerequisites.h"
#include "../../Enum/TunnelTypes.h"
#include "../Techno/Body.h"
#include "../../Misc/SWTypes.h"
#include "../../Utilities/INIParser.h"
#include "../../Utilities/TemplateDef.h"

#include <FactoryClass.h>
#include <DiscreteSelectionClass.h>
#include <HouseClass.h>
#include <MouseClass.h>
#include <SuperClass.h>
#include <ScenarioClass.h>

#include "../../Misc/SavegameDef.h"

#include <functional>

HouseExt::ExtContainer HouseExt::ExtMap;

bool HouseExt::IsAnyFirestormActive = false;

CDTimerClass HouseExt::Timer_CloakedUnitDetected;
CDTimerClass HouseExt::Timer_SubterraneanUnitDetected;

std::vector<int> HouseExt::AIProduction_CreationFrames;
std::vector<int> HouseExt::AIProduction_Values;
std::vector<int> HouseExt::AIProduction_BestChoices;

// =============================
// member funcs

void HouseExt::ExtData::LoadFromINIFile(CCINIClass* const pINI) {
	auto const pThis = this->OwnerObject();
	auto const* const pSection = pThis->PlainName;

	INI_EX exINI(pINI);

	this->Degrades.Read(exINI, pSection, "Degrades");
}

HouseExt::TunnelData::TunnelData(TunnelTypeClass const* const pType)
	: MaxCap(Math::max(pType->Passengers, -1))
{ }

void HouseExt::TunnelData::RemovePassenger(void* const ptr) {
	auto const it = std::find(
		this->Passengers.begin(), this->Passengers.end(), ptr);

	if(it != this->Passengers.end()) {
		this->Passengers.erase(it);
	}
}

HouseExt::TunnelData* HouseExt::ExtData::FindTunnel(size_t const index) {
	auto const count = TunnelTypeClass::Array.size();

	for(auto i = this->Tunnels.size(); i < count; ++i) {
		this->Tunnels.emplace_back(TunnelTypeClass::Array[i].get());
	}

	return &this->Tunnels[index];
}

HouseExt::RequirementStatus HouseExt::RequirementsMet(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem)
{
	if(pItem->Unbuildable) {
		return RequirementStatus::Forbidden;
	}

	TechnoTypeExt::ExtData* pData = TechnoTypeExt::ExtMap.Find(pItem);
	if(!pItem) {
		return RequirementStatus::Forbidden;
	}
//	TechnoTypeClassExt::TechnoTypeClassData *pData = TechnoTypeClassExt::Ext_p[pItem];
	HouseExt::ExtData* pHouseExt = HouseExt::ExtMap.Find(pHouse);

	// this has to happen before the first possible "can build" response or NCO happens
	if(pItem->WhatAmI() != AbstractType::BuildingType
		&& HasFactory(pHouse, pItem, true, false, false, true).State
			<= FactoryState::NoFactory)
	{
		return RequirementStatus::Incomplete;
	}

	if(!(pData->PrerequisiteTheaters & (1 << static_cast<int>(ScenarioClass::Instance->Theater)))) { return RequirementStatus::Forbidden; }
	if(Prereqs::HouseOwnsAny(pHouse, pData->PrerequisiteNegatives)) { return RequirementStatus::Forbidden; }

	if(pHouseExt->ReverseEngineered.contains(pItem)) {
		return RequirementStatus::Overridden;
	}

	if(pData->RequiredStolenTech.any()) {
		if((pHouseExt->StolenTech & pData->RequiredStolenTech) != pData->RequiredStolenTech) { return RequirementStatus::Incomplete; }
	}

	// yes, the game checks it here
	// hack value - skip real prereq check
	if(Prereqs::HouseOwnsAny(pHouse, pItem->PrerequisiteOverride)) { return RequirementStatus::Overridden; }

	if(pHouse->HasFromSecretLab(pItem)) { return RequirementStatus::Overridden; }

	if(pHouse->IsControlledByHuman() && pItem->TechLevel == -1) { return RequirementStatus::Incomplete; }

	if(!pHouse->HasAllStolenTech(pItem)) { return RequirementStatus::Incomplete; }

	if(!pHouse->InRequiredHouses(pItem) || pHouse->InForbiddenHouses(pItem)) { return RequirementStatus::Forbidden; }

	if(!HouseExt::CheckFactoryOwners(pHouse, pItem)) { return RequirementStatus::Incomplete; }

	if(auto const pBldType = specific_cast<BuildingTypeClass const*>(pItem)) {
		if(HouseExt::IsDisabledFromShell(pHouse, pBldType)) {
			return RequirementStatus::Forbidden;
		}
	}

	return (pHouse->TechLevel >= pItem->TechLevel) ? RequirementStatus::Complete : RequirementStatus::Incomplete;
}

bool HouseExt::PrerequisitesMet(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem)
{
	if(!pItem) {
		return false;
	}

	auto const pData = TechnoTypeExt::ExtMap.Find(pItem);

	for(const auto& list : pData->PrerequisiteLists) {
		if(Prereqs::HouseOwnsAll(pHouse, list)) {
			return true;
		}
	}

	return false;
}

bool HouseExt::PrerequisitesListed(
	Prereqs::BTypeIter const& List, TechnoTypeClass const* const pItem)
{
	if(!pItem) {
		return false;
	}

	auto const pData = TechnoTypeExt::ExtMap.Find(pItem);

	for(const auto& list : pData->PrerequisiteLists) {
		if(Prereqs::ListContainsAll(List, list)) {
			return true;
		}
	}

	return false;
}

// HouseClass::Get_Factory walks FactoryClass::Array for a factory this house
// owns whose *currently produced object* is of this type. That is narrower than
// FactoryClass::FindByOwnerAndProduct, which asks CountTotal and so also counts
// items merely queued behind the one in production.

HouseExt::BuildLimitStatus HouseExt::CheckBuildLimit(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem,
	bool const includeQueued)
{
	int BuildLimit = pItem->BuildLimit;
	int Remaining = HouseExt::BuildLimitRemaining(pHouse, pItem);
	if(BuildLimit > 0) {
		if(Remaining <= 0) {
			return (includeQueued && pHouse->GetFactoryProducing(pItem))
				? BuildLimitStatus::NotReached
				: BuildLimitStatus::ReachedPermanently
			;
		}
	}
	return (Remaining > 0)
		? BuildLimitStatus::NotReached
		: BuildLimitStatus::ReachedTemporarily
	;
}

signed int HouseExt::BuildLimitRemaining(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem)
{
	auto const BuildLimit = pItem->BuildLimit;
	if(BuildLimit >= 0) {
		return BuildLimit - HouseExt::CountOwnedNowTotal(pHouse, pItem);
	} else {
		return -BuildLimit - pHouse->CountOwnedEver(pItem);
	}
}

int HouseExt::CountOwnedNowTotal(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem)
{
	int index = -1;
	int sum = 0;
	const BuildingTypeClass* pBType = nullptr;
	const UnitTypeClass* pUType = nullptr;
	const InfantryTypeClass* pIType = nullptr;
	const char* pPowersUp = nullptr;

	switch(pItem->WhatAmI()) {
	case AbstractType::BuildingType:
		pBType = static_cast<BuildingTypeClass const*>(pItem);
		pPowersUp = pBType->PowersUpBuilding;
		if(pPowersUp[0]) {
			if(auto const pTPowersUp = BuildingTypeClass::Find(pPowersUp)) {
				for(auto const& pBld : pHouse->Buildings) {
					if(pBld->Type == pTPowersUp) {
						for(auto const& pUpgrade : pBld->Upgrades) {
							if(pUpgrade == pBType) {
								++sum;
							}
						}
					}
				}
			}
		} else {
			sum = pHouse->CountOwnedNow(pBType);
			if(auto const pUndeploy = pBType->UndeploysInto) {
				sum += pHouse->CountOwnedNow(pUndeploy);
			}
		}
		break;

	case AbstractType::UnitType:
		pUType = static_cast<UnitTypeClass const*>(pItem);
		sum = pHouse->CountOwnedNow(pUType);
		if(auto const pDeploy = pUType->DeploysInto) {
			sum += pHouse->CountOwnedNow(pDeploy);
		}
		break;

	case AbstractType::InfantryType:
		pIType = static_cast<InfantryTypeClass const*>(pItem);
		sum = pHouse->CountOwnedNow(pIType);
		if(pIType->VehicleThief) {
			index = pIType->ArrayIndex;
			for(auto const& pUnit : UnitClass::Array) {
				if(pUnit->HijackerInfantryType == index
					&& pUnit->Owner == pHouse)
				{
					++sum;
				}
			}
		}
		break;

	case AbstractType::AircraftType:
		sum = pHouse->CountOwnedNow(
			static_cast<AircraftTypeClass const*>(pItem));
		break;

	default:
		__assume(0);
	}

	return sum;
}

signed int HouseExt::PrereqValidate(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem,
	bool const buildLimitOnly, bool const includeQueued)
{
	// `CurrentPlayer`, plus `PlayerControl` in campaign, cached once up front
	// at 0x10022592: the AI bypass at the tail re-reads the same flag.
	auto const human = pHouse->IsControlledByHuman();

	if(!buildLimitOnly) {
		RequirementStatus ReqsMet = HouseExt::RequirementsMet(pHouse, pItem);
		if(ReqsMet == RequirementStatus::Forbidden || ReqsMet == RequirementStatus::Incomplete) {
			return 0;
		}

		// the Prerequisite.* lists are only consulted for a house a human
		// plays: 0x100225CF falls through to the factory check when the house
		// is AI, and 0x100225D4 does the same when the status is Overridden.
		if(human && ReqsMet == RequirementStatus::Complete) {
			if(!HouseExt::PrerequisitesMet(pHouse, pItem)) {
				return 0;
			}
		}

		// the factory check is on the AI's path too -- shipped reaches it by
		// `goto`, not by returning early -- but it is skipped entirely when
		// only the build limit was asked for (`jz` at 0x100225B1).
		auto const state = HouseExt::HasFactory(
			pHouse, pItem, true, true, false, true).State;

		if(state <= FactoryState::NoFactory) {
			return 0;
		}

		if(state <= FactoryState::Unpowered) {
			return -1;
		}
	}

	if(!human && RulesExt::Global()->AllowBypassBuildLimit[pHouse->GetAIDifficultyIndex()]) {
		return 1;
	}

	return static_cast<signed int>(HouseExt::CheckBuildLimit(pHouse, pItem, includeQueued));
}

bool HouseExt::IsDisabledFromShell(
	HouseClass const* const pHouse, BuildingTypeClass const* const pItem)
{
	// SWAllowed does not apply to campaigns any more
	if(SessionClass::Instance.GameMode == GameMode::Campaign
		|| GameModeOptionsClass::Instance.SWAllowed)
	{
		return false;
	}

	if(pItem->SuperWeapon != -1) {
		// allow SWs only if not disableable from shell
		auto const pItem2 = const_cast<BuildingTypeClass*>(pItem);
		auto const& BuildTech = RulesClass::Instance->BuildTech;
		if(BuildTech.FindItemIndex(pItem2) == -1) {
			auto const pSuper = pHouse->Supers[pItem->SuperWeapon];
			if(pSuper->Type->DisableableFromShell) {
				return true;
			}
		}
	}

	return false;
}

size_t HouseExt::FindOwnedIndex(
	HouseClass const* const, int const idxParentCountry,
	Iterator<TechnoTypeClass const*> const items, size_t const start)
{
	auto const bitOwner = 1u << idxParentCountry;

	for(auto i = start; i < items.size(); ++i) {
		auto const pItem = items[i];

		if(pItem->InOwners(bitOwner)) {
			return i;
		}
	}

	return items.size();
}

size_t HouseExt::FindBuildableIndex(
	HouseClass const* const pHouse, int const idxParentCountry,
	Iterator<TechnoTypeClass const*> const items, size_t const start)
{
	for(auto i = start; i < items.size(); ++i) {
		auto const pItem = items[i];

		if(pHouse->CanExpectToBuild(pItem, idxParentCountry)) {
			auto const pBld = abstract_cast<const BuildingTypeClass*>(pItem);
			if(pBld && HouseExt::IsDisabledFromShell(pHouse, pBld)) {
				continue;
			}

			return i;
		}
	}

	return items.size();
}

HouseExt::FactoryCheckReturn HouseExt::HasFactory(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem,
	bool const allowOccupied, bool const requirePower,
	bool const requireCanBuild, bool const anyFactory)
{
	if(requireCanBuild && static_cast<int>(pHouse->CanBuild(pItem, true, true)) <= 0) {
		return { FactoryState::Unbuildable, nullptr };
	}

	auto const pExt = TechnoTypeExt::ExtMap.Find(pItem);
	auto const bitsOwners = pItem->GetOwners();
	auto const isNaval = pItem->Naval;
	auto const abs = pItem->WhatAmI();

	BuildingClass* pAvailable = nullptr;
	BuildingClass* pUnpowered = nullptr;

	for(auto const& pBld : pHouse->Buildings) {
		if(pBld->InLimbo
			|| pBld->GetCurrentMission() == Mission::Selling
			|| pBld->QueuedMission == Mission::Selling)
		{
			continue;
		}

		auto const pType = pBld->Type;

		if(pType->Factory != abs || !pType->InOwners(bitsOwners)) {
			continue;
		}

		// an occupied airbase is only good for another plane if it has a free
		// slot. anything else has to match the item's naval-ness.
		if(!allowOccupied && abs == AbstractType::AircraftType
			&& pBld->HasAnyLink())
		{
			if(!pBld->HasFreeLink()) {
				continue;
			}
		} else if(pType->Naval != (abs == AbstractType::UnitType && isNaval)) {
			continue;
		}

		if(!pExt->CanBeBuiltAt(pType)) {
			continue;
		}

		if(requirePower && (!pBld->HasPower || pBld->Deactivated)) {
			pUnpowered = pBld;
			continue;
		}

		pAvailable = pBld;

		if(pBld->IsPrimaryFactory) {
			return { FactoryState::Primary, pBld };
		}

		if(anyFactory) {
			break;
		}
	}

	if(pAvailable) {
		return { FactoryState::Available, pAvailable };
	}

	if(pUnpowered) {
		return { FactoryState::Unpowered, pUnpowered };
	}

	return { FactoryState::NoFactory, nullptr };
}

// FactoryOwners and FactoryOwners.Forbidden are one test, not two: a single
// candidate - a gathered plan, or an owned building - has to pass both filters
// at once. Splitting them lets building A satisfy the allowed list while
// building B satisfies the forbidden list, which the shipped code does not allow.
bool HouseExt::CheckFactoryOwners(
	HouseClass const* const pHouse, TechnoTypeClass const* const pItem)
{
	auto const pExt = TechnoTypeExt::ExtMap.Find(pItem);

	auto const& Owners = pExt->FactoryOwners;
	auto const& Forbidden = pExt->ForbiddenFactoryOwners;

	if(Owners.empty() && Forbidden.empty()) {
		return true;
	}

	auto Passes = [&Owners, &Forbidden](HouseTypeClass* const pCountry) -> bool {
		return (Owners.empty() || Owners.Contains(pCountry))
			&& (Forbidden.empty() || !Forbidden.Contains(pCountry));
	};

	auto const pHouseExt = HouseExt::ExtMap.Find(pHouse);

	for(auto const& pCountry : pHouseExt->FactoryOwners_GatheredPlansOf) {
		if(Passes(pCountry)) {
			return true;
		}
	}

	auto const abs = pItem->WhatAmI();

	for(auto const& pBld : pHouse->Buildings) {
		auto const pBldExt = TechnoExt::ExtMap.Find(pBld);

		if(!Passes(pBldExt->OriginalHouseType)) {
			continue;
		}

		// FactoryOwners.HasAllPlans makes a building stand in for every factory
		// kind of its original owner, so it satisfies any item's requirement.
		// Unlike FactoryOwners.Permanent this is evaluated live: lose the
		// building and the plans go with it.
		auto const pBldTypeExt = TechnoTypeExt::ExtMap.Find(pBld->Type);

		if(pBld->Type->Factory == abs || pBldTypeExt->FactoryOwners_HasAllPlans) {
			return true;
		}
	}

	return false;
}

bool HouseExt::UpdateAnyFirestormActive(bool const lastChange) {
	IsAnyFirestormActive = lastChange;

	// if last change activated one, there is at least one. else...
	if(!lastChange) {
		for(auto const& pHouse : HouseClass::Array) {
			if(pHouse->FirestormActive) {
				IsAnyFirestormActive = true;
				break;
			}
		}
	}

	return IsAnyFirestormActive;
}

HouseClass* HouseExt::GetHouseKind(
	OwnerHouseKind const kind, bool const allowRandom,
	HouseClass* const pDefault, HouseClass* const pInvoker,
	HouseClass* const pKiller, HouseClass* const pVictim)
{
	switch(kind) {
	case OwnerHouseKind::Invoker:
		return pInvoker ? pInvoker : pDefault;
	case OwnerHouseKind::Killer:
		return pKiller ? pKiller : pDefault;
	case OwnerHouseKind::Victim:
		return pVictim ? pVictim : pDefault;
	case OwnerHouseKind::Civilian:
		return HouseClass::FindCivilianSide();
	case OwnerHouseKind::Special:
		return HouseClass::FindSpecial();
	case OwnerHouseKind::Neutral:
		return HouseClass::FindNeutral();
	case OwnerHouseKind::Random:
		if(allowRandom) {
			auto& Random = ScenarioClass::Instance->Random;
			return HouseClass::Array.GetItem(
				Random.RandomRanged(0, HouseClass::Array.Count - 1));
		} else {
			return pDefault;
		}
	case OwnerHouseKind::Default:
	default:
		return pDefault;
	}
}

HouseExt::ExtData::~ExtData() = default;

void HouseExt::ExtData::SetFirestormState(bool const active) {
	auto const pHouse = this->OwnerObject();

	if(pHouse->FirestormActive == active) {
		return;
	}

	pHouse->FirestormActive = active;
	UpdateAnyFirestormActive(active);

	DynamicVectorClass<CellStruct> AffectedCoords;

	for(auto const& pBld : pHouse->Buildings) {
		auto const pTypeData = BuildingTypeExt::ExtMap.Find(pBld->Type);
		if(pTypeData->Firewall_Is) {
			auto const pExt = BuildingExt::ExtMap.Find(pBld);
			pExt->UpdateFirewall();
			auto const temp = pBld->GetMapCoords();
			AffectedCoords.AddItem(temp);
		}
	}

	MapClass::Instance.Update_Pathfinding_1();
	MapClass::Instance.Update_Pathfinding_2(AffectedCoords);
};

/**
 * moved the fix for #917 here - check a house's ability to handle base plan
 * before it actually tries to generate a base plan, not at game start (we have
 * no idea what houses at game start are supposed to be able to do base
 * planning, so mission maps fuck up)
 */
bool HouseExt::ExtData::CheckBasePlanSanity() {
	auto const pThis = this->OwnerObject();
	// this shouldn't happen, but you never know
	if(pThis->IsControlledByHuman() || pThis->IsNeutral()) {
		return true;
	}

	auto AllIsWell = true;

	auto const pRules = RulesClass::Instance;
	auto const pType = pThis->Type;

	auto const errorMsg = "AI House of country [%s] cannot build any object in "
		"%s. The AI ain't smart enough for that.\n";

	// if you don't have a base unit buildable, how did you get to base
	// planning? only through crates or map actions, so have to validate base
	// unit in other situations
	auto const idxParent = pType->FindParentCountryIndex();
	auto const canBuild = std::any_of(
		pRules->BaseUnit.begin(), pRules->BaseUnit.end(),
		[pThis, idxParent] (UnitTypeClass const* const pItem)
	{
		return pThis->CanExpectToBuild(pItem, idxParent);
	});

	if(!canBuild) {
		AllIsWell = false;
		Debug::Log(Debug::Severity::Error, errorMsg, pType->ID, "BaseUnit");
	}

	auto CheckList = [pThis, pType, idxParent, errorMsg, &AllIsWell] (
		Iterator<BuildingTypeClass const*> const list,
		const char* const ListName) -> void
	{
		if(!HouseExt::FindBuildable(pThis, idxParent, list)) {
			AllIsWell = false;
			Debug::Log(Debug::Severity::Error, errorMsg, pType->ID, ListName);
		}
	};

	// commented out lists that do not cause a crash, according to testers
	//CheckList(make_iterator(pRules->Shipyard), "Shipyard");
	CheckList(make_iterator(pRules->BuildPower), "BuildPower");
	CheckList(make_iterator(pRules->BuildRefinery), "BuildRefinery");
	CheckList(make_iterator(pRules->BuildWeapons), "BuildWeapons");
	//CheckList(make_iterator(pRules->BuildConst), "BuildConst");
	//CheckList(make_iterator(pRules->BuildBarracks), "BuildBarracks");
	//CheckList(make_iterator(pRules->BuildTech), "BuildTech");
	//CheckList(make_iterator(pRules->BuildRadar), "BuildRadar");
	//CheckList(make_iterator(pRules->ConcreteWalls), "ConcreteWalls");
	//CheckList(make_iterator(pRules->BuildDummy), "BuildDummy");
	//CheckList(make_iterator(pRules->BuildNavalYard), "BuildNavalYard");

	auto const pCountryData = HouseTypeExt::ExtMap.Find(pType);
	auto const Powerplants = pCountryData->GetPowerplants();
	CheckList(Powerplants, "Powerplants");

	//auto const pSide = SideClass::Array.GetItemOrDefault(pType->SideIndex);
	//if(auto const pSideExt = SideExt::ExtMap.Find(pSide)) {
	//	CheckList(make_iterator(pSideExt->BaseDefenses), "Base Defenses");
	//}

	return AllIsWell;
}

// how often a super weapon of this type has been fired, and when the
// house last had a look at it. index beyond the list means never.
HouseExt::ShotStuff HouseExt::ExtData::ShotsAmount(int idxSWType) const {
	if(static_cast<size_t>(idxSWType) < this->SWShotCounts.size()) {
		return this->SWShotCounts[idxSWType];
	}

	return ShotStuff{ 0, Unsorted::CurrentFrame };
}

void HouseExt::ExtData::UpdateShotsLastCheckedFrame(int idxSWType) {
	auto const count = static_cast<size_t>(SuperWeaponTypeClass::Array.Count);

	if(this->SWShotCounts.size() < count) {
		this->SWShotCounts.resize(count);
	}

	auto& item = this->SWShotCounts[idxSWType];
	if(item.LastCheckedFrame < 0) {
		item.LastCheckedFrame = Unsorted::CurrentFrame;
	}
}

void HouseExt::ExtData::UpdateShootCount(int idxSWType) {
	auto const count = static_cast<size_t>(SuperWeaponTypeClass::Array.Count);

	if(this->SWShotCounts.size() < count) {
		this->SWShotCounts.resize(count);
	}

	auto& item = this->SWShotCounts[idxSWType];
	++item.ShootAmount;
	item.LastCheckedFrame = Unsorted::CurrentFrame;
}

void HouseExt::ExtData::UpdateTogglePower() {
	const auto pThis = this->OwnerObject();

	auto pRulesExt = RulesExt::Global();

	if(!pRulesExt->TogglePowerAllowed
		|| pRulesExt->TogglePowerDelay <= 0
		|| pRulesExt->TogglePowerIQ < 0
		|| pRulesExt->TogglePowerIQ > pThis->IQLevel2
		|| pThis->Buildings.Count == 0
		|| pThis->IsBeingDrained 
		|| pThis->IsControlledByHuman()
		|| pThis->PowerBlackoutTimer.InProgress())
	{
		return;
	}

	if(Unsorted::CurrentFrame % pRulesExt->TogglePowerDelay == 0) {
		struct ExpendabilityStruct {
		private:
			std::tuple<const int&, BuildingClass&> Tie() const {
				// compare with tie breaker to prevent desyncs
				return std::tie(this->Value, *this->Building);
			}

		public:
			bool operator < (const ExpendabilityStruct& rhs) const {
				return this->Tie() < rhs.Tie();
			}

			bool operator > (const ExpendabilityStruct& rhs) const {
				return this->Tie() > rhs.Tie();
			}

			BuildingClass* Building;
			int Value;
		};

		// properties: the higher this value is, the more likely
		// this building is turned off (expendability)
		auto GetExpendability = [](BuildingClass* pBld) -> int {
			auto pType = pBld->Type;

			// disable super weapons, because a defenseless base is
			// worse than one without super weapons
			if(pType->HasSuperWeapon()) {
				return pType->PowerDrain * 20 / 10;
			}

			// non-base defenses should be disabled before going
			// to the base defenses. but power intensive defenses
			// might still evaluate worse
			if(!pType->IsBaseDefense) {
				return pType->PowerDrain * 15 / 10;
			}

			// default case, use power
			return pType->PowerDrain;
		};

		// create a list of all buildings that can be powered down
		// and give each building an expendability value
		std::vector<ExpendabilityStruct> Buildings;
		Buildings.reserve(pThis->Buildings.Count);

		const auto HasLowPower = pThis->HasLowPower();

		for(auto pBld : pThis->Buildings) {
			auto pType = pBld->Type;
			if(pType->CanTogglePower() && pType->PowerDrain > 0) {
				// if low power, we get buildings with StuffEnabled, if enough
				// power, we look for builidings that are disabled
				if(pBld->StuffEnabled == HasLowPower) {
					Buildings.emplace_back(ExpendabilityStruct{pBld, GetExpendability(pBld)});
				}
			}
		}

		int Surplus = pThis->PowerOutput - pThis->PowerDrain;

		if(HasLowPower) {
			// most expendable building first
			std::sort(Buildings.begin(), Buildings.end(), std::greater<>());

			// turn off the expendable buildings until power is restored
			for(const auto& item : Buildings) {
				auto Drain = item.Building->Type->PowerDrain;

				item.Building->GoOffline();
				Surplus += Drain;

				if(Surplus >= 0) {
					break;
				}
			}
		} else {
			// least expendable building first
			std::sort(Buildings.begin(), Buildings.end(), std::less<>());

			// turn on as many of them as possible
			for(const auto& item : Buildings) {
				auto Drain = item.Building->Type->PowerDrain;
				if(Surplus - Drain >= 0) {
					item.Building->GoOnline();
					Surplus -= Drain;
				}
			}
		}
	}
}

SideClass* HouseExt::GetSide(HouseClass* pHouse) {
	return SideClass::Array.GetItemOrDefault(pHouse->SideIndex);
}

int HouseExt::ExtData::GetSurvivorDivisor() const {
	if(auto pExt = SideExt::ExtMap.Find(HouseExt::GetSide(this->OwnerObject()))) {
		return pExt->GetSurvivorDivisor();
	}

	return 0;
}

InfantryTypeClass* HouseExt::ExtData::GetCrew() const {
	if(auto pExt = SideExt::ExtMap.Find(HouseExt::GetSide(this->OwnerObject()))) {
		return pExt->GetCrew();
	}

	return RulesClass::Instance->Technician;
}

InfantryTypeClass* HouseExt::ExtData::GetEngineer() const {
	if(auto pExt = SideExt::ExtMap.Find(HouseExt::GetSide(this->OwnerObject()))) {
		return pExt->GetEngineer();
	}

	return RulesClass::Instance->Engineer;
}

InfantryTypeClass* HouseExt::ExtData::GetTechnician() const {
	if(auto pExt = SideExt::ExtMap.Find(HouseExt::GetSide(this->OwnerObject()))) {
		return pExt->GetTechnician();
	}

	return RulesClass::Instance->Technician;
}

InfantryTypeClass* HouseExt::ExtData::GetDisguise() const {
	if(auto pExt = SideExt::ExtMap.Find(HouseExt::GetSide(this->OwnerObject()))) {
		return pExt->GetDisguise();
	}

	return RulesClass::Instance->ThirdDisguise;
}

void HouseExt::ExtData::UpdateAcademy(BuildingClass* pAcademy, bool added) {
	// check if added and there already, or removed and not there
	auto it = std::find(this->Academies.cbegin(), this->Academies.cend(), pAcademy);
	if(added == (it != this->Academies.cend())) {
		return;
	}

	// now this can be unconditional
	if(added) {
		this->Academies.push_back(pAcademy);
	} else {
		this->Academies.erase(it);
	}
}

void HouseExt::ExtData::ApplyAcademy(
	TechnoClass* const pTechno, AbstractType const considerAs) const
{
	// mutex in effect, ignore academies to fix preplaced order issues.
	// also triggered in game for certain "conversions" like deploy
	if(Unsorted::ScenarioInit) {
		return;
	}

	auto const pType = pTechno->GetTechnoType();

	// get the academy data for this type
	Valueable<double> BuildingTypeExt::ExtData::* pmBonus = nullptr;
	if(considerAs == AbstractType::Infantry) {
		pmBonus = &BuildingTypeExt::ExtData::AcademyInfantry;
	} else if(considerAs == AbstractType::Aircraft) {
		pmBonus = &BuildingTypeExt::ExtData::AcademyAircraft;
	} else if(considerAs == AbstractType::Unit) {
		pmBonus = &BuildingTypeExt::ExtData::AcademyVehicle;
	} else if(considerAs == AbstractType::Building) {
		pmBonus = &BuildingTypeExt::ExtData::AcademyBuilding;
	}

	auto veterancyBonus = 0.0;

	// aggregate the bonuses
	for(auto const& pBld : this->Academies) {
		auto const pExt = BuildingTypeExt::ExtMap.Find(pBld->Type);

		auto const isWhitelisted = pExt->AcademyWhitelist.empty()
			|| pExt->AcademyWhitelist.Contains(pType);

		if(isWhitelisted && !pExt->AcademyBlacklist.Contains(pType)) {
			const auto& data = pExt->*pmBonus;
			veterancyBonus = std::max(veterancyBonus, data.Get());
		}
	}

	// apply the bonus
	if(pType->Trainable) {
		auto& value = pTechno->Veterancy.Veterancy;
		if(veterancyBonus > value) {
			value = static_cast<float>(std::min(
				veterancyBonus, RulesClass::Instance->VeteranCap));
		}
	}
}

// tracks how many owned objects keep this house from losing a Short Game and
// answers whether the object counts towards the score statistics at all.
bool HouseExt::ExtData::KeepThisAlive(
	TechnoClass const* const pTechno, AbstractType const abs, bool const added)
{
	auto const pType = pTechno->GetTechnoType();
	auto const isBuilding = (abs == AbstractType::Building);

	auto counted = true;
	auto keepAlive = isBuilding;

	if(pType->Insignificant || pType->DontScore) {
		counted = false;
		keepAlive = false;
	}

	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	if(pExt->KeepAlive.Get(keepAlive)) {
		auto const delta = added ? 1 : -1;

		this->KeepAliveCount += delta;

		if(isBuilding) {
			this->KeepAliveBuildingsCount += delta;
		}
	}

	return counted;
}

// =============================
// load / save

bool HouseExt::ExtData::ReverseEngineer(TechnoTypeClass const* pVictimType)
{
	auto const pVictimData = TechnoTypeExt::ExtMap.Find(pVictimType);

	if(!pVictimData->CanBeReversed) {
		return false;
	}

	// ReversedAs substitutes the type that is unlocked. Only CanBeReversed is read
	// off the victim itself; everything below works on the substitute. There is no
	// category guard here - the docs explicitly allow reversing into a BuildingType.
	if(auto const pReversedAs = pVictimData->ReversedAs.Get()) {
		pVictimType = pReversedAs;
	}

	if(this->ReverseEngineered.contains(pVictimType)) {
		return false;
	}

	auto const pOwner = this->OwnerObject();
	auto const wasBuildable = HouseExt::PrereqValidate(pOwner, pVictimType, false, true) == 1;

	this->ReverseEngineered.insert(pVictimType, true);

	if(!wasBuildable
		&& HouseExt::RequirementsMet(pOwner, pVictimType) != RequirementStatus::Forbidden)
	{
		pOwner->RecheckTechTree = true;
	}

	// true means "newly recorded", not "the tech tree changed". Announcements and
	// the reverse-engineer triggers hang off this, and they fire for a type the
	// house happened to be able to build already.
	return true;
}

template <typename T>
void HouseExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->Degrades)
		.Process(this->IonSensitive)
		.Process(this->AuxPower)
		.Process(this->BatteriesActive)
		.Process(this->SWLastIndex)
		.Process(this->KeepAliveCount)
		.Process(this->KeepAliveBuildingsCount)
		.Process(this->Factory_BuildingType)
		.Process(this->Factory_InfantryType)
		.Process(this->Factory_VehicleType)
		.Process(this->Factory_NavyType)
		.Process(this->Factory_AircraftType)
		.Process(this->ReverseEngineered)
		.Process(this->StolenTech)
		.Process(this->RadarPersist)
		.Process(this->NavalYardInfiltrated)
		.Process(this->AircraftFactoryInfiltrated)
		.Process(this->BuildingInfiltrated)
		.Process(this->FactoryOwners_GatheredPlansOf)
		.Process(this->Academies)
		.Process(this->Tunnels)
		.Process(this->Battery_KeepOnline)
		.Process(this->Battery_Overpower)
		.Process(this->SWShotCounts);
}

bool HouseExt::TunnelData::Load(AresStreamReader &Stm, bool RegisterForChange) {
	return Stm
		.Process(this->Passengers, RegisterForChange)
		.Process(this->MaxCap)
		.Success();
}

bool HouseExt::TunnelData::Save(AresStreamWriter &Stm) const {
	return Stm
		.Process(this->Passengers)
		.Process(this->MaxCap)
		.Success();
}

void HouseExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<HouseClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void HouseExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<HouseClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool HouseExt::LoadGlobals(AresStreamReader& Stm) {
	return Stm
		.Process(IsAnyFirestormActive)
		.Process(Timer_CloakedUnitDetected)
		.Process(Timer_SubterraneanUnitDetected)
		.Success();
}

bool HouseExt::SaveGlobals(AresStreamWriter& Stm) {
	return Stm
		.Process(IsAnyFirestormActive)
		.Process(Timer_CloakedUnitDetected)
		.Process(Timer_SubterraneanUnitDetected)
		.Success();
}

// =============================
// container

HouseExt::ExtContainer::ExtContainer() : Container("HouseClass") {
}

HouseExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x4F6532, HouseClass_CTOR, 0x5)
{
	GET(HouseClass*, pItem, EAX);

	HouseExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x4F7371, HouseClass_DTOR, 0x6)
{
	GET(HouseClass*, pItem, ESI);

	HouseExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x504080, HouseClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x503040, HouseClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(HouseClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	HouseExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x504069, HouseClass_Load_Suffix, 0x7)
{
	HouseExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x5046DE, HouseClass_Save_Suffix, 0x7)
{
	HouseExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK(0x50114D, HouseClass_InitFromINI, 0x5)
{
	GET(HouseClass* const, pThis, EBX);
	GET(CCINIClass* const, pINI, ESI);

	HouseExt::ExtMap.LoadFromINI(pThis, pINI);

	return 0;
}

static_assert(sizeof(HouseExt::TunnelData) == 0x10, "HouseExt::TunnelData must match the 3.0p1 layout");
static_assert(sizeof(HouseExt::ShotStuff) == 0x08, "HouseExt::ShotStuff must match the 3.0p1 layout");
static_assert(sizeof(HouseExt::ExtData) == 0xC0, "HouseExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the head, the two new counters, the relocated reverse
// engineering map, the spy veterancy bools and the last member.
static_assert(offsetof(HouseExt::ExtData, Degrades) == 0x08, "HouseExt::ExtData layout slipped");
static_assert(offsetof(HouseExt::ExtData, AuxPower) == 0x0C, "HouseExt::ExtData layout slipped");
static_assert(offsetof(HouseExt::ExtData, KeepAliveCount) == 0x18, "HouseExt::ExtData layout slipped");
static_assert(offsetof(HouseExt::ExtData, ReverseEngineered) == 0x34, "HouseExt::ExtData layout slipped");
static_assert(offsetof(HouseExt::ExtData, NavalYardInfiltrated) == 0x48, "HouseExt::ExtData layout slipped");
static_assert(offsetof(HouseExt::ExtData, Tunnels) == 0x64, "HouseExt::ExtData layout slipped");
static_assert(offsetof(HouseExt::ExtData, SWShotCounts) == 0x88, "HouseExt::ExtData layout slipped");
