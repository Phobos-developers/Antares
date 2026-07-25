#include "Body.h"
#include "../BuildingType/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"
#include "../House/Body.h"
#include "../Rules/Body.h"

#include "../../Misc/SavegameDef.h"

#include <AnimClass.h>
#include <BuildingClass.h>
#include <CellClass.h>
#include <MapClass.h>
#include <CellSpread.h>
#include <GeneralStructures.h> // for CellStruct
#include <VoxClass.h>
#include <RadarEventClass.h>
#include <SuperClass.h>
#include <ScenarioClass.h>
#include <MouseClass.h>
#include <Helpers\Enumerators.h>

#include <algorithm>

BuildingExt::ExtContainer BuildingExt::ExtMap;

std::vector<CellStruct> BuildingExt::TempFoundationData1;
std::vector<CellStruct> BuildingExt::TempFoundationData2;
std::vector<CellStruct> BuildingExt::TempCoveredCellsData;

// =============================
// member functions

DWORD BuildingExt::GetFirewallFlags(BuildingClass *pThis) {
	auto pCell = MapClass::Instance.GetCellAt(pThis->Location);
	DWORD flags = 0;
	for(size_t direction = 0; direction < 8; direction += 2) {
		auto pNeighbour = pCell->GetNeighbourCell(static_cast<FacingType>(direction));
		if(auto pBld = pNeighbour->GetBuilding()) {
			auto pTypeData = BuildingTypeExt::ExtMap.Find(pBld->Type);
			if(pTypeData->Firewall_Is && pBld->Owner == pThis->Owner && !pBld->InLimbo && pBld->IsAlive) {
				flags |= 1 << (direction >> 1);
			}
		}
	}
	return flags;
}

bool BuildingExt::IsActiveFirestormWall(BuildingClass* const pBuilding, HouseClass const* const pIgnore)
{
	if(HouseExt::IsAnyFirestormActive && pBuilding && pBuilding->Owner != pIgnore) {
		if(!pBuilding->InLimbo && pBuilding->IsAlive) {
			if(pBuilding->Owner->FirestormActive) {
				auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);
				return pTypeExt->Firewall_Is;
			}
		}
	}

	return false;
}

void BuildingExt::UpdateFactoryQueues(BuildingClass const* const pBuilding)
{
	auto const pType = pBuilding->Type;
	if(pType->Factory != AbstractType::None) {
		pBuilding->Owner->Update_FactoriesQueues(
			pType->Factory, pType->Naval, BuildCat::DontCare);
	}
}

void BuildingExt::UpdateDisplayTo(BuildingClass *pThis) {
	if(pThis->Type->Radar) {
		auto pHouse = pThis->Owner;
		pHouse->RadarVisibleTo.Clear();

		auto pExt = HouseExt::ExtMap.Find(pHouse);
		pHouse->RadarVisibleTo.data |= pExt->RadarPersist.data;

		for(auto pBld : pHouse->Buildings) {
			if(!pBld->InLimbo) {
				auto pData = BuildingTypeExt::ExtMap.Find(pBld->Type);
				if(pData->RevealRadar) {
					pHouse->RadarVisibleTo.data |= pBld->DisplayProductionTo.data;
				}
			}
		}
		MapClass::Instance.RedrawSidebar(2);
	}
}

// #664: Advanced Rubble
//! Takes a building off the map and puts its other Advanced Rubble state there.
/*!
	Both callers have already established that the state they are switching to
	is configured (either a type to become, or Remove set), so there is no
	"nothing to do" case left to diagnose here.

	Rubble is set to full health on placing, since, no matter what we do in the
	backend, to the player, each pile of rubble is a new one. The individual
	strength setting overrides this: a negative one in [-100,-1] is read as a
	percentage of the new type's Strength, a positive one as an absolute value
	capped at it.

	The placement runs under IKnowWhatImDoing, so the replacement lands without
	the usual deploy checks having a say; whether it landed is not consulted.

	\param pBuilding the building leaving the map
	\param remove    place nothing, just take the old state away
	\param pNewType  the type to place in its stead
	\param owner     which house the replacement belongs to
	\param strength  the replacement's health, see above
	\param pAnimType played over the old building's coordinates, may be null

	\returns the building that was placed, or nullptr if there was none.

	\author Renegade
	\date 16.12.09+

	Updated by
	\author Gluk-v48
	\date 07.04.14
*/
BuildingClass* BuildingExt::ExtData::PlaceRubble(BuildingClass* pBuilding,
	bool remove, BuildingTypeClass* pNewType, OwnerHouseKind owner, int strength,
	AnimTypeClass* pAnimType)
{
	pBuilding->Limbo(); // only takes it off the map
	pBuilding->DestroyNthAnim(BuildingAnimSlot::All);

	BuildingClass* pNew = nullptr;

	if(!remove) {
		auto pOwner = HouseExt::GetHouseKind(owner, true, pBuilding->Owner);
		pNew = static_cast<BuildingClass*>(pNewType->CreateObject(pOwner));

		if(strength <= -1 && strength >= -100) {
			// percentage of original health
			pNew->Health = std::max((-strength * pNew->Type->Strength) / 100, 1);
		} else if(strength > 0) {
			pNew->Health = std::min(strength, pNew->Type->Strength);
		} /* else Health = Strength*/

		++Unsorted::ScenarioInit;
		// GetFacing<256>, not <8>: Unlimbo wants a 256-direction facing. Shipped
		// (0x1000F6C1) inlines `((Raw >> 7) + 1) >> 1`, which is GetValue<8>.
		pNew->Unlimbo(pBuilding->Location, static_cast<DirType>(pBuilding->PrimaryFacing.Current().GetFacing<256>()));
		--Unsorted::ScenarioInit;
	}

	if(pAnimType) {
		GameCreate<AnimClass>(pAnimType, pBuilding->GetCoords());
	}

	return pNew;
}

//! Bumps all Technos that reside on a building's foundation out of the way.
/*!
	All units on the building's foundation are kicked out.

	\author AlexB
	\date 2013-04-24
*/
void BuildingExt::ExtData::KickOutOfRubble() {
	auto const pBld = this->OwnerObject();

	// get the number of non-end-marker cells and a pointer to the cell data
	auto const data = pBld->Type->FoundationData;

	using Item = std::pair<FootClass*, bool>;
	DynamicVectorClass<Item> list;

	// iterate over all cells and remove all infantry
	auto const location = MapClass::Instance.GetCellAt(pBld->Location)->MapCoords;
	for(auto i = data; *i != BuildingTypeExt::FoundationEndMarker; ++i) {
		// remove every techno that resides on this cell
		auto const pCell = MapClass::Instance.GetCellAt(location + *i);
		for(NextObject obj(pCell->GetContent()); obj; ++obj) {
			if(auto const pFoot = abstract_cast<FootClass*>(*obj)) {
				auto const selected = pFoot->IsSelected;
				if(pFoot->Limbo()) {
					list.AddItem(Item(pFoot, selected));
				}
			}
		}
	}

	// this part kicks out all units we found in the rubble
	for(auto const& item : list) {
		if(pBld->KickOutUnit(item.first, location) == KickOutResult::Succeeded) {
			if(item.second) {
				item.first->Select();
			}
		} else {
			item.first->UnInit();
		}
	}
}

// #666: IsTrench/Traversal
//! This function checks if the current and the target building are both of the same trench kind.
/*!
	\param targetBuilding a pointer to the target building.
	\return true if both buildings are trenches and are of the same trench kind, otherwise false.

	\author Renegade
	\date 25.12.09+
*/
bool BuildingExt::ExtData::sameTrench(BuildingClass* targetBuilding) {
	BuildingTypeExt::ExtData* currentTypeExtData = BuildingTypeExt::ExtMap.Find(this->OwnerObject()->Type);
	BuildingTypeExt::ExtData* targetTypeExtData = BuildingTypeExt::ExtMap.Find(targetBuilding->Type);

	return ((currentTypeExtData->IsTrench > -1) && (currentTypeExtData->IsTrench == targetTypeExtData->IsTrench));
}

// #666: IsTrench/Traversal
//! This function checks if occupants of the current building can, on principle, move on to the target building.
/*!
	Conditions checked are whether there are occupants in the current building, whether the target building is full,
	whether the current building even is a trench, and whether both buildings are of the same trench kind.

	The current system assumes 1x1 sized trench parts; it will probably work in all cases where the 0,0 (top) cells of
	buildings touch (e.g. a 3x3 building next to a 1x3 building), but not when the 0,0 cells are further apart.
	This may be changed at a later point in time, if there is demand for it.

	\param targetBuilding a pointer to the target building.
	\return true if traversal between both buildings is legal, otherwise false.
	\sa doTraverseTo()

	\author Renegade
	\date 16.12.09+
*/
bool BuildingExt::ExtData::canTraverseTo(BuildingClass* targetBuilding) {
	BuildingClass* currentBuilding = this->OwnerObject();
	//BuildingTypeClass* currentBuildingType = game_cast<BuildingTypeClass *>(currentBuilding->GetTechnoType());
	BuildingTypeClass* targetBuildingType = targetBuilding->Type;

	if((targetBuilding == currentBuilding)
		|| (targetBuilding->Occupants.Count >= targetBuildingType->MaxNumberOccupants)
		|| !currentBuilding->Occupants.Count
		|| !targetBuildingType->CanBeOccupied) { // I'm a little if-clause short and stdout... // beat you to the punchline -- D
		// Can't traverse if there's no one to move, or the target is full, we can't actually occupy the target,
		// or if it's actually the same building
		return false;
	}

	//BuildingTypeExt::ExtData* currentBuildingTypeExt = BuildingTypeExt::ExtMap.Find(currentBuilding->Type);
	//BuildingTypeExt::ExtData* targetBuildingTypeExt = BuildingTypeExt::ExtMap.Find(targetBuilding->Type);

	if(this->sameTrench(targetBuilding)) {
		// if we've come here, there's room, there are people to move, and the buildings are trenches and of the same kind
		// the only questioning remaining is whether they are next to each other.
		// if the target building is more than 256 leptons away, it's not on a straight neighboring cell.
		return targetBuilding->Location.DistanceFrom(currentBuilding->Location) <= 256.0;

	} else {
		return false; // not the same trench kind or not a trench
	}

}

// #666: IsTrench/Traversal
//! This function moves as many occupants as possible from the current to the target building.
/*!
	This function will move occupants from the current building to the target building until either
	a) the target building is full, or
	b) the current building is empty.

	\warning The function assumes the legality of the traversal was checked beforehand with #canTraverseTo(), and will therefore not do any safety checks.

	\param targetBuilding a pointer to the target building.
	\sa canTraverseTo()

	\author Renegade
	\date 16.12.09+
*/
void BuildingExt::ExtData::doTraverseTo(BuildingClass* targetBuilding) {
	BuildingClass* currentBuilding = this->OwnerObject();
	BuildingTypeClass* targetBuildingType = targetBuilding->Type;

	// depending on Westwood's handling, this could explode when Size > 1 units are involved...but don't tell the users that
	while(currentBuilding->Occupants.Count && (targetBuilding->Occupants.Count < targetBuildingType->MaxNumberOccupants)) {
		targetBuilding->Occupants.AddItem(currentBuilding->Occupants.GetItem(0));
		currentBuilding->Occupants.RemoveItem(0); // maybe switch Add/Remove if the game gets pissy about multiple of them walking around
	}

	// fix up firing index, as decrementing the source occupants can invalidate it
	if(currentBuilding->FiringOccupantIndex >= currentBuilding->GetOccupantCount()) {
		currentBuilding->FiringOccupantIndex = 0;
	}

	this->evalRaidStatus(); // if the traversal emptied the current building, it'll have to be returned to its owner
}

void BuildingExt::ExtData::evalRaidStatus() {
	// if the building is still marked as raided, but unoccupied, return it to its previous owner
	if(this->OwnerBeforeRaid && !this->OwnerObject()->Occupants.Count) {
		// Fix for #838: Only return the building to the previous owner if he hasn't been defeated
		if(!this->OwnerBeforeRaid->Defeated) {
			this->OwnerObject()->SetOwningHouse(this->OwnerBeforeRaid);
		}
		this->OwnerBeforeRaid = nullptr;
	}
}

// Firewalls
// #666: IsTrench/Linking
// Short check: Is the building of a linkable kind at all?
bool BuildingExt::ExtData::isLinkable() {
	BuildingTypeExt::ExtData* typeExtData = BuildingTypeExt::ExtMap.Find(this->OwnerObject()->Type);
	return typeExtData->IsLinkable();
}

// Full check: Can this building be linked to the target building?
bool BuildingExt::ExtData::canLinkTo(BuildingClass* targetBuilding) {
	BuildingClass* currentBuilding = this->OwnerObject();

	// Different owners // and owners not allied
	if((currentBuilding->Owner != targetBuilding->Owner) && !currentBuilding->Owner->IsAlliedWith(targetBuilding->Owner)) { //<-- see thread 1424
		return false;
	}

	BuildingTypeExt::ExtData* currentTypeExtData = BuildingTypeExt::ExtMap.Find(currentBuilding->Type);
	BuildingTypeExt::ExtData* targetTypeExtData = BuildingTypeExt::ExtMap.Find(targetBuilding->Type);

	// Firewalls
	if(currentTypeExtData->Firewall_Is && targetTypeExtData->Firewall_Is) {
		return true;
	}

	// Trenches
	if(this->sameTrench(targetBuilding)) {
		return true;
	}

	return false;
}
/*!

	\param theBuilding the building which we're trying to link to existing buildings.
	\param selectedCell the cell at which we're trying to build.
	\param buildingOwner the owner of the building we're trying to link.
	\sa isLinkable()
	\sa canLinkTo()

	\author DCoder, Renegade
	\date 26.12.09+
*/
void BuildingExt::buildLines(BuildingClass* theBuilding, CellStruct selectedCell, HouseClass* buildingOwner) {
	BuildingExt::ExtData* buildingExtData = BuildingExt::ExtMap.Find(theBuilding);
	//BuildingTypeExt::ExtData* buildingTypeExtData = BuildingTypeExt::ExtMap.Find(theBuilding->Type);

	// check if this building is linkable at all and abort if it isn't
	if(!buildingExtData->isLinkable()) {
		return;
	}

	short maxLinkDistance = static_cast<short>(theBuilding->Type->GuardRange / 256); // GuardRange governs how far the link can go, is saved in leptons

	for(size_t direction = 0; direction <= 7; direction += 2) { // the 4 straight directions of the simple compass
		CellStruct directionOffset = CellSpread::GetNeighbourOffset(direction); // coordinates of the neighboring cell in the given direction relative to the current cell (e.g. 0,1)
		int linkLength = 0; // how many cells to build on from center in direction to link up with a found building

		CellStruct cellToCheck = selectedCell;
		for(short distanceFromCenter = 1; distanceFromCenter <= maxLinkDistance; ++distanceFromCenter) {
			cellToCheck += directionOffset; // adjust the cell to check based on current distance, relative to the selected cell

			CellClass *cell = MapClass::Instance.TryGetCellAt(cellToCheck);

			if(!cell) { // don't parse this cell if it doesn't exist (duh)
				break;
			}

			if(BuildingClass *OtherEnd = cell->GetBuilding()) { // if we find a building...
				if(buildingExtData->canLinkTo(OtherEnd)) { // ...and it is linkable, we found what we needed
					linkLength = distanceFromCenter - 1; // distanceFromCenter directly would be on top of the found building
					break;
				}

				break; // we found a building, but it's not linkable
			}

			if(!cell->CanThisExistHere(theBuilding->Type->SpeedType, theBuilding->Type, buildingOwner)) { // abort if that buildingtype is not allowed to be built there
				break;
			}

		}

		// build a line of this buildingtype from the found building (if any) to the newly built one
		CellStruct cellToBuildOn = selectedCell;
		for(int distanceFromCenter = 1; distanceFromCenter <= linkLength; ++distanceFromCenter) {
			cellToBuildOn += directionOffset;

			if(CellClass *cell = MapClass::Instance.GetCellAt(cellToBuildOn)) {
				if(BuildingClass *tempBuilding = specific_cast<BuildingClass *>(theBuilding->Type->CreateObject(buildingOwner))) {
					CoordStruct coordBuffer = CellClass::Cell2Coord(cellToBuildOn);

					++Unsorted::ScenarioInit; // put the building there even if normal rules would deny - e.g. under units
					bool Put = tempBuilding->Unlimbo(coordBuffer, DirType::North);
					--Unsorted::ScenarioInit;

					if(Put) {
						tempBuilding->QueueMission(Mission::Construction, false);
						tempBuilding->DiscoveredBy(buildingOwner);
						tempBuilding->IsReadyToCommence = 1;
					} else {
						GameDelete(tempBuilding);
					}
				}
			}
		}
	}
}

// return 0-based index of frame to draw, taken from the building's main SHP image.
// return -1 to let nature take its course
signed int BuildingExt::GetImageFrameIndex(BuildingClass *pThis) {
	BuildingTypeExt::ExtData *pData = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if(pData->Firewall_Is) {
		return static_cast<int>(pThis->FirestormWallFrame);

		/* this is the code the game uses to calculate the firewall's frame number when you place/remove sections... should be a good base for trench frames

			int frameIdx = 0;
			CellClass *Cell = this->GetCell();
			for(int direction = 0; direction <= 7; direction += 2) {
				if(BuildingClass *B = Cell->GetNeighbourCell(static_cast<FacingType>(direction))->GetBuilding()) {
					if(B->IsAlive && !B->InLimbo) {
						frameIdx |= (1 << (direction >> 1));
					}
				}
			}

		*/
	}

	if(pData->IsTrench > -1) {
		return 0;
	}

	return -1;
}

void BuildingExt::KickOutHospitalArmory(BuildingClass *pThis)
{
	if(pThis->Type->Hospital || pThis->Type->Armory) {
		if(FootClass * Passenger = pThis->Passengers.RemoveFirstPassenger()) {
			pThis->KickOutUnit(Passenger, CellStruct::Empty);
		}
	}
}

// Reproduces BuildingExt::ExtData::UpdateFactoryPlans (0x100454B0). A building
// whose type - or any of whose upgrades - has FactoryOwners.Permanent hands its
// original owner's plans to whoever owns it now, permanently. The upgrade slots
// are what makes the documented "supported on upgrades" true.
void BuildingExt::UpdateFactoryPlans(BuildingClass *pThis) {
	BuildingTypeClass* const types[] = {
		pThis->Type, pThis->Upgrades[0], pThis->Upgrades[1], pThis->Upgrades[2]
	};

	for(auto const pType : types) {
		if(pType && TechnoTypeExt::ExtMap.Find(pType)->FactoryOwners_HaveAllPlans) {
			auto& plans = HouseExt::ExtMap.Find(pThis->Owner)->FactoryOwners_GatheredPlansOf;
			auto const pOriginal = TechnoExt::ExtMap.Find(pThis)->OriginalHouseType;

			if(!plans.Contains(pOriginal)) {
				plans.push_back(pOriginal);
			}
			return;
		}
	}
}

// Reproduces BuildingExt::ProduceCashDisplay (0x10011D50): the oil derrick payout
// is queued on the *building's* flying-string ticker, gated by the building type's
// own ProduceCashDisplay - upgrades contribute their money but never their flag.
void BuildingExt::ExtData::ProduceCashDisplay(int amount) {
	auto const pThis = this->OwnerObject();
	if(BuildingTypeExt::ExtMap.Find(pThis->Type)->ProduceCashDisplay) {
		TechnoExt::ExtMap.Find(pThis)->TechnoValue += amount;
	}
}

// =============================
// infiltration

bool BuildingExt::ExtData::InfiltratedBy(HouseClass *Enterer) {
	BuildingClass *EnteredBuilding = this->OwnerObject();
	BuildingTypeClass *EnteredType = EnteredBuilding->Type;
	HouseClass *Owner = EnteredBuilding->Owner;
	BuildingTypeExt::ExtData* pTypeExt = BuildingTypeExt::ExtMap.Find(EnteredBuilding->Type);
	HouseExt::ExtData* pEntererExt = HouseExt::ExtMap.Find(Enterer);

	if(!pTypeExt->InfiltrateCustom) {
		return false;
	}

	if(Owner == Enterer || Enterer->IsAlliedWith(Owner)) {
		return true;
	}

	bool raiseEva = false;

	if(Enterer->IsControlledByCurrentPlayer() || Owner->IsControlledByCurrentPlayer()) {
		CellStruct xy = EnteredBuilding->GetMapCoords();
		if(RadarEventClass::Create(RadarEventType::BuildingInfiltrated, xy)) {
			raiseEva = true;
		}
	}

	bool evaForOwner = Owner->IsControlledByCurrentPlayer() && raiseEva;
	bool evaForEnterer = Enterer->IsControlledByCurrentPlayer() && raiseEva;
	bool effectApplied = false;

	if(pTypeExt->ResetRadar) {
		Owner->ReshroudMap();
		if(!Owner->SpySatActive && evaForOwner) {
			VoxClass::Play("EVA_RadarSabotaged");
		}
		if(!Owner->SpySatActive && evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfRadarSabotaged");
		}
		effectApplied = true;
	}


	if(pTypeExt->PowerOutageDuration > 0) {
		Owner->CreatePowerOutage(pTypeExt->PowerOutageDuration);
		if(evaForOwner) {
			VoxClass::Play("EVA_PowerSabotaged");
		}
		if(evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfiltrated");
			VoxClass::Play("EVA_EnemyBasePoweredDown");
		}
		effectApplied = true;
	}


	if(pTypeExt->StolenTechIndex) {
		pEntererExt->StolenTech |= std::bitset<32>(pTypeExt->StolenTechIndex);

		Enterer->RecheckTechTree = true;
		if(evaForOwner) {
			VoxClass::Play("EVA_TechnologyStolen");
		}
		if(evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfiltrated");
			VoxClass::Play("EVA_NewTechnologyAcquired");
		}
		effectApplied = true;
	}


	if(pTypeExt->UnReverseEngineer) {
		Debug::Log("Undoing all Reverse Engineering achieved by house %ls\n", Owner->UIName);

		HouseExt::ExtMap.Find(Owner)->ReverseEngineered.clear();
		Owner->RecheckTechTree = true;

		if(evaForOwner) {
			VoxClass::Play("EVA_BuildingInfiltrated");
		}
		if(evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfiltrated");
		}
		effectApplied = true;
	}


	if(pTypeExt->ResetSW) {
		bool somethingReset = false;
		auto EnteredBuildingExt = BuildingExt::ExtMap.Find(EnteredBuilding);
		auto buildingSWCount = EnteredBuildingExt->GetSuperWeaponCount();
		for(auto i = 0u; i < buildingSWCount; ++i) {
			if(auto pSuper = EnteredBuildingExt->GetSuperWeapon(i)) {
				pSuper->Reset();
				somethingReset = true;
			}
		}
		for(int i = 0; i < EnteredType->Upgrades; ++i) {
			if(auto Upgrade = EnteredBuilding->Upgrades[i]) {
				auto UpgradeExt = BuildingTypeExt::ExtMap.Find(Upgrade);
				auto upgradeSWCount = UpgradeExt->GetSuperWeaponCount();
				for(auto j = 0u; j < upgradeSWCount; ++j) {
					int swIdx = UpgradeExt->GetSuperWeaponIndex(j, Owner);
					if(swIdx != -1) {
						Owner->Supers.Items[swIdx]->Reset();
						somethingReset = true;
					}
				}
			}
		}

		if(somethingReset) {
			if(evaForOwner || evaForEnterer) {
				VoxClass::Play("EVA_BuildingInfiltrated");
			}
			effectApplied = true;
		}
	}


	// SpyEffect.SuperWeapon: grant the enterer a super weapon. It is a one-shot
	// unless SpyEffect.SuperWeaponPermanent, in which case it also loses the
	// ability to be put on hold. A powered SW arrives on hold if the enterer is
	// in a power deficit.
	if(auto const pSWType = pTypeExt->SpySuperWeapon) {
		bool const permanent = pTypeExt->SuperWeaponPermanent;
		auto const idxSW = pSWType->ArrayIndex;
		auto const pSuper = Enterer->Supers[idxSW];
		bool const onHold = pSWType->IsPowered && Enterer->HasLowPower();

		if(pSuper->Grant(!permanent, Enterer == HouseClass::CurrentPlayer, onHold)) {
			if(permanent) {
				pSuper->CanHold = false;
			}

			if(Enterer == HouseClass::CurrentPlayer) {
				auto const pSidebar = &SidebarClass::Instance;
				pSidebar->AddCameo(AbstractType::Special, idxSW);
				pSidebar->RepaintSidebar(
					SidebarClass::GetObjectTabIdx(AbstractType::Super, idxSW, 0));
			}

			if(evaForOwner || evaForEnterer) {
				VoxClass::Play("EVA_BuildingInfiltrated");
			}
			effectApplied = true;
		}
	}


	// SpyEffect.SabotageDelay: arm the building like a planted C4 charge.
	// A negative value falls back to [General]C4Delay, in minutes.
	if(int sabotage = pTypeExt->SabotageDelay) {
		if(sabotage < 0) {
			sabotage = static_cast<int>(RulesClass::Instance->C4Delay * 900.0);
		}
		if(sabotage >= 0) {
			EnteredBuilding->C4Applied = true;
			EnteredBuilding->C4Timer.Start(sabotage);
		}
	}


	int bounty = 0;
	int available = Owner->Available_Money();
	if(pTypeExt->StolenMoneyAmount > 0) {
		bounty = pTypeExt->StolenMoneyAmount;
	} else if(pTypeExt->StolenMoneyPercentage > 0.0f) {
		bounty = static_cast<int>(available * pTypeExt->StolenMoneyPercentage);
	}
	if(bounty > 0) {
		bounty = std::min(bounty, available);
		Owner->TakeMoney(bounty);
		Enterer->GiveMoney(bounty);
		if(evaForOwner) {
			VoxClass::Play("EVA_CashStolen");
		}
		if(evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfCashStolen");
		}
		effectApplied = true;
	}


	// SpyEffect.UnitVeterancy still derives the branch from Factory=, but the
	// five explicit SpyEffect.<branch>Veterancy flags stack on top of it and are
	// not restricted to what this building actually produces. Naval, aircraft
	// and building promotion have no stock HouseClass flag, so they live on the
	// house extension.
	bool promotionStolen = false;

	if(pTypeExt->GainVeterancy) {
		switch(EnteredType->Factory) {
			case InfantryTypeClass::AbsID:
				Enterer->BarracksInfiltrated = true;
				promotionStolen = true;
				break;
			case UnitTypeClass::AbsID:
				Enterer->WarFactoryInfiltrated = true;
				promotionStolen = true;
				break;
			default:
				break;
		}
	}

	if(pTypeExt->InfantryVeterancy) {
		Enterer->BarracksInfiltrated = true;
		promotionStolen = true;
	}

	if(pTypeExt->VehicleVeterancy) {
		Enterer->WarFactoryInfiltrated = true;
		promotionStolen = true;
	}

	if(pTypeExt->NavalVeterancy) {
		pEntererExt->NavalYardInfiltrated = true;
		promotionStolen = true;
	}

	if(pTypeExt->AircraftVeterancy) {
		pEntererExt->AircraftFactoryInfiltrated = true;
		promotionStolen = true;
	}

	if(pTypeExt->BuildingVeterancy) {
		pEntererExt->BuildingInfiltrated = true;
		promotionStolen = true;
	}

	if(promotionStolen) {
		Enterer->RecheckTechTree = true;
		if(Enterer->IsControlledByCurrentPlayer()) {
			MouseClass::Instance.SidebarNeedsRepaint();
		}
		if(evaForOwner) {
			VoxClass::Play("EVA_TechnologyStolen");
		}
		if(evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfiltrated");
			VoxClass::Play("EVA_NewTechnologyAcquired");
		}
		effectApplied = true;
	}


	/*	RA1-Style Spying, as requested in issue #633
		This sets the respective bit to inform the game that a particular house has spied this building.
		Knowing that, the game will reveal the current production in this building to the players who have spied it.
		In practice, this means: If a player who has spied a factory clicks on that factory,
		he will see the cameo of whatever is being built in the factory.

		Addition 04.03.10: People complained about it not being optional. Now it is.
	*/
	if(pTypeExt->RevealProduction) {
		EnteredBuilding->DisplayProductionTo.Add(Enterer);
		if(evaForOwner || evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfiltrated");
		}
		effectApplied = true;
	}

	if(pTypeExt->RevealRadar) {
		/*	Remember the new persisting radar spy effect on the victim house itself, because
			destroying the building would destroy the spy reveal info in the ExtData, too.
			2013-08-12 AlexB
		*/
		if(pTypeExt->RevealRadarPersist) {
			auto pOwnerExt = HouseExt::ExtMap.Find(Owner);
			pOwnerExt->RadarPersist.Add(Enterer);
		}

		EnteredBuilding->DisplayProductionTo.Add(Enterer);
		BuildingExt::UpdateDisplayTo(EnteredBuilding);
		if(evaForOwner || evaForEnterer) {
			VoxClass::Play("EVA_BuildingInfiltrated");
		}
		MapClass::Instance.sub_657CE0();
		MapClass::Instance.RedrawSidebar(2);
		effectApplied = true;
	}

	if(effectApplied) {
		EnteredBuilding->Mark(MarkType::Change);
	}
	return true;
}

void BuildingExt::ExtData::UpdateFirewall(bool const changedState) {
	auto const pThis = this->OwnerObject();
	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if(!pTypeExt->Firewall_Is) {
		return;
	}

	auto const active = pThis->Owner->FirestormActive;

	if(!changedState) {
		// update only the idle anim
		auto& Anim = pThis->GetAnim(BuildingAnimSlot::SpecialTwo);

		// (0b0101 || 0b1010) == part of a straight line
		auto const connections = pThis->FirestormWallFrame & 0xF;
		if(active && Unsorted::CurrentFrame & 7 && !Anim
			&& connections != 0b0101 && connections != 0b1010
			&& (ScenarioClass::Instance->Random.Random() & 0xF) == 0)
		{
			if(AnimTypeClass* pType = RulesExt::Global()->FirestormIdleAnim) {
				auto const crd = pThis->GetCoords() - CoordStruct{ 740, 740, 0 };
				Anim = GameCreate<AnimClass>(pType, crd, 0, 1, 0x604, -10);
				Anim->IsBuildingAnim = true;
			}
		}
	} else {
		// update the frame, cell passability and active anim
		auto const idxFrame = BuildingExt::GetFirewallFlags(pThis)
			+ (active ? 32u : 0u);

		if(pThis->FirestormWallFrame != idxFrame) {
			pThis->FirestormWallFrame = idxFrame;
			pThis->GetCell()->RecalcAttributes(-1);
			pThis->Mark(MarkType::Change);
		}

		auto& Anim = pThis->GetAnim(BuildingAnimSlot::Special);

		auto const connections = idxFrame & 0xF;
		if(active && connections != 0b0101 && connections != 0b1010 && !Anim) {
			if(auto const& pType = RulesExt::Global()->FirestormActiveAnim) {
				auto const crd = pThis->GetCoords() - CoordStruct{ 128, 128, 0 };
				Anim = GameCreate<AnimClass>(pType, crd, 1, 0, 0x600, -10);
				Anim->IsFogged = pThis->IsFogged;
				Anim->IsBuildingAnim = true;
			}
		} else if(Anim) {
			GameDelete(Anim);
			Anim = nullptr;
		}
	}

	if(active) {
		this->ImmolateVictims();
	}
}

void BuildingExt::ExtData::UpdateFirewallLinks() {
	auto const pThis = this->OwnerObject();
	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if(pTypeExt->Firewall_Is) {
		// update this
		if(!pThis->InLimbo && pThis->IsAlive) {
			this->UpdateFirewall();
		}

		// and all surrounding buildings
		auto const pCell = MapClass::Instance.GetCellAt(pThis->Location);
		for(auto i = 0u; i < 8; i += 2) {
			auto const pNeighbour = pCell->GetNeighbourCell(static_cast<FacingType>(i));
			if(auto const pBld = pNeighbour->GetBuilding()) {
				auto const pExt = BuildingExt::ExtMap.Find(pBld);
				pExt->UpdateFirewall();
			}
		}
	}
}

void BuildingExt::ExtData::ImmolateVictims() {
	auto const pCell = this->OwnerObject()->GetCell();
	for(NextObject object(pCell->FirstObject); object; ++object) {
		if(auto pFoot = abstract_cast<FootClass*>(*object)) {
			if(!pFoot->GetType()->IgnoresFirestorm) {
				this->ImmolateVictim(pFoot);
			}
		}
	}
}

bool BuildingExt::ExtData::ImmolateVictim(ObjectClass* const pVictim, bool const destroy) {
	if(pVictim && pVictim->Health > 0) {
		auto const pRulesExt = RulesExt::Global();

		if(destroy) {
			auto const pThis = this->OwnerObject();

			auto const pWarhead = pRulesExt->FirestormWarhead.Get(
				RulesClass::Instance->C4Warhead);

			auto damage = pVictim->Health;
			pVictim->ReceiveDamage(&damage, 0, pWarhead, nullptr, true, true,
				pThis->Owner);
		}

		auto const& pType = (pVictim->GetHeight() < 100)
			? pRulesExt->FirestormGroundAnim
			: pRulesExt->FirestormAirAnim;

		if(pType) {
			auto const crd = pVictim->GetCoords();
			GameCreate<AnimClass>(pType, crd, 0, 1, 0x600, -10, false);
		}

		return true;
	}

	return false;
}

// Updates the activation of the sensor ability, if neccessary.
/*!
	The update is only performed if this is a sensor array and its state changed.

	\author AlexB
	\date 2012-10-08
*/
void BuildingExt::ExtData::UpdateSensorArray() {
	BuildingClass* pBld = this->OwnerObject();

	if(pBld->Type->SensorArray) {
		bool isActive = pBld->IsPowerOnline() && !pBld->Deactivated;
		bool wasActive = (this->SensorArrayActiveCounter > 0);

		if(isActive != wasActive) {
			if(isActive) {
				pBld->SensorArrayActivate();
			} else {
				pBld->SensorArrayDeactivate();
			}
		}
	}
}

// Assigns a secret production option to the building.
void BuildingExt::ExtData::UpdateSecretLab() {
	auto pThis = this->OwnerObject();
	auto pOwner = pThis->Owner;

	if(!pOwner || pOwner->Type->MultiplayPassive) {
		return;
	}

	auto pType = pThis->Type;

	// fixed item, no need to randomize
	if(pType->SecretInfantry || pType->SecretUnit || pType->SecretBuilding) {
		Debug::Log("[Secret Lab] %s has a fixed boon.\n", pType->ID);
		return;
	}

	auto pData = BuildingTypeExt::ExtMap.Find(pType); 

	// go on if not placed or always recalculate on capture
	if(this->SecretLab_Placed && !pData->Secret_RecalcOnCapture) {
		return;
	}

	DynamicVectorClass<TechnoTypeClass*> Options;

	auto AddToOptions = [pOwner, &Options](const Iterator<TechnoTypeClass*> &items) {
		auto OwnerBits = 1u << pOwner->Type->ArrayIndex;

		for(const auto& Option : items) {
			auto pExt = TechnoTypeExt::ExtMap.Find(Option);

			if((pExt->Secret_RequiredHouses & OwnerBits) && !(pExt->Secret_ForbiddenHouses & OwnerBits)) {
				switch(HouseExt::RequirementsMet(pOwner, Option)) {
				case HouseExt::RequirementStatus::Forbidden:
				case HouseExt::RequirementStatus::Incomplete:
					Options.AddItem(Option);
				}
			}
		}
	};

	// generate a list of items
	if(pData->Secret_Boons.HasValue()) {
		AddToOptions(pData->Secret_Boons);
	} else {
		AddToOptions(make_iterator(RulesClass::Instance->SecretInfantry));
		AddToOptions(make_iterator(RulesClass::Instance->SecretUnits));
		AddToOptions(make_iterator(RulesClass::Instance->SecretBuildings));
	}

	// pick one of all eligible items
	if(Options.Count > 0) {
		auto Result = Options[ScenarioClass::Instance->Random.RandomRanged(0, Options.Count - 1)];
		Debug::Log("[Secret Lab] rolled %s for %s\n", Result->ID, pType->ID);
		pThis->SecretProduction = Result;
		this->SecretLab_Placed = true;
	} else {
		Debug::Log("[Secret Lab] %s has no boons applicable to country [%s]!\n",
			pType->ID, pOwner->Type->ID);
	}
}

bool BuildingExt::ExtData::IsBaseNormal() const {
	if(this->SkipBaseNormal) {
		return false;
	}

	auto const pThis = this->OwnerObject();
	auto const pType = pThis->Type;

	auto const pExt = BuildingTypeExt::ExtMap.Find(pType);
	if(pExt->AIBaseNormal.isset()) {
		return pExt->AIBaseNormal;
	}

	if(pType->UndeploysInto && pType->ResourceGatherer) {
		return false;
	}

	return !pThis->IsStrange();
}

size_t BuildingExt::ExtData::GetSuperWeaponCount() const {
	auto pExt = BuildingTypeExt::ExtMap.Find(this->OwnerObject()->Type);
	return pExt->GetSuperWeaponCount();
}

bool BuildingExt::ExtData::HasSuperWeapon() const {
	return this->GetFirstSuperWeaponIndex() != -1;
}

bool BuildingExt::ExtData::HasSuperWeapon(const int index, const bool withUpgrades) const {
	const auto pThis = this->OwnerObject();
	const auto pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	const auto count = pExt->GetSuperWeaponCount();
	for(auto i = 0u; i < count; ++i) {
		const auto idxSW = pExt->GetSuperWeaponIndex(i, pThis->Owner);
		if(idxSW == index) {
			return true;
		}
	}

	if(withUpgrades) {
		for(auto const& pUpgrade : pThis->Upgrades) {
			if(const auto pUpgradeExt = BuildingTypeExt::ExtMap.Find(pUpgrade)) {
				const auto countUpgrade = pUpgradeExt->GetSuperWeaponCount();
				for(auto i = 0u; i < countUpgrade; ++i) {
					const auto idxSW = pUpgradeExt->GetSuperWeaponIndex(i, pThis->Owner);
					if(idxSW == index) {
						return true;
					}
				}
			}
		}
	}

	return false;
}

int BuildingExt::ExtData::GetSuperWeaponIndex(const size_t index) const {
	const auto pThis = this->OwnerObject();
	const auto pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	return pExt->GetSuperWeaponIndex(index, pThis->Owner);
}

SuperClass* BuildingExt::ExtData::GetSuperWeapon(const size_t index) const {
	const auto idxSW = this->GetSuperWeaponIndex(index);
	return this->OwnerObject()->Owner->Supers.GetItemOrDefault(idxSW);
}

int BuildingExt::ExtData::GetFirstSuperWeaponIndex() const {
	const auto pThis = this->OwnerObject();
	const auto pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	const auto count = pExt->GetSuperWeaponCount();
	for(auto i = 0u; i < count; ++i) {
		const auto idxSW = pExt->GetSuperWeaponIndex(i, pThis->Owner);
		if(idxSW != -1) {
			return idxSW;
		}
	}
	return -1;
}

SuperClass* BuildingExt::ExtData::GetFirstSuperWeapon() const {
	const auto idxSW = this->GetFirstSuperWeaponIndex();
	return this->OwnerObject()->Owner->Supers.GetItemOrDefault(idxSW);
}

DWORD BuildingExt::FoundationLength(CellStruct const* const pFoundation) {
	auto pFCell = pFoundation;
	while(*pFCell != BuildingTypeExt::FoundationEndMarker) {
		++pFCell;
	}

	// include the end marker
	return static_cast<DWORD>(std::distance(pFoundation, pFCell + 1));
}

const std::vector<CellStruct>& BuildingExt::GetCoveredCells(
	BuildingClass* const pThis, CellStruct const mainCoords,
	int const shadowHeight)
{
	auto const pFoundation = pThis->GetFoundationData(false);
	auto const len = BuildingExt::FoundationLength(pFoundation);

	TempCoveredCellsData.clear();
	TempCoveredCellsData.reserve(len * shadowHeight);

	auto pFCell = pFoundation;

	while(*pFCell != BuildingTypeExt::FoundationEndMarker) {
		auto actualCell = mainCoords + *pFCell;
		for(auto i = shadowHeight; i > 0; --i) {
			TempCoveredCellsData.push_back(actualCell);
			--actualCell.X;
			--actualCell.Y;
		}
		++pFCell;
	}

	std::sort(TempCoveredCellsData.begin(), TempCoveredCellsData.end(),
		[](const CellStruct &lhs, const CellStruct &rhs) -> bool
	{
		return lhs.X > rhs.X || lhs.X == rhs.X && lhs.Y > rhs.Y;
	});

	auto const it = std::unique(
		TempCoveredCellsData.begin(), TempCoveredCellsData.end());
	TempCoveredCellsData.erase(it, TempCoveredCellsData.end());

	return TempCoveredCellsData;
}

void BuildingExt::Clear() {
	BuildingExt::ExtMap.Clear();

	BuildingExt::TempFoundationData1.clear();
	BuildingExt::TempFoundationData2.clear();
}

bool BuildingExt::ExtData::ReverseEngineer(TechnoClass *Victim) {
	BuildingTypeExt::ExtData *pReverseData = BuildingTypeExt::ExtMap.Find(this->OwnerObject()->Type);
	if(!pReverseData->ReverseEngineersVictims) {
		return false;
	}

	TechnoTypeClass *VictimType = Victim->GetTechnoType();
	TechnoTypeExt::ExtData *pVictimData = TechnoTypeExt::ExtMap.Find(VictimType);

	if(!pVictimData->CanBeReversed) {
		return false;
	}

	// ReversedAs substitutes the type that is unlocked. Only CanBeReversed is read
	// off the victim itself; everything below works on the substitute. There is no
	// category guard here - the docs explicitly allow reversing into a BuildingType.
	if(auto const pReversedAs = pVictimData->ReversedAs.Get()) {
		VictimType = pReversedAs;
	}

	HouseClass *Owner = this->OwnerObject()->Owner;
	HouseExt::ExtData *pOwnerData = HouseExt::ExtMap.Find(Owner);

	if(!pOwnerData->ReverseEngineered.contains(VictimType)) {
		bool WasBuildable = HouseExt::PrereqValidate(Owner, VictimType, false, true) == 1;
		pOwnerData->ReverseEngineered.insert(VictimType, true);
		if(!WasBuildable) {
			bool IsBuildable = HouseExt::RequirementsMet(Owner, VictimType) != HouseExt::RequirementStatus::Forbidden;
			if(IsBuildable) {
				Owner->RecheckTechTree = true;
				return true;
			}
		}
	}
	return false;
}

void BuildingExt::ExtData::KickOutClones(TechnoClass* const Production) {
	auto const Factory = this->OwnerObject();
	auto const FactoryType = Factory->Type;

	if(FactoryType->Cloning || (FactoryType->Factory != InfantryTypeClass::AbsID && FactoryType->Factory != UnitTypeClass::AbsID)) {
		return;
	}

	auto const ProductionType = Production->GetTechnoType();
	auto const ProductionTypeData = TechnoTypeExt::ExtMap.Find(ProductionType);
	if(!ProductionTypeData->Cloneable) {
		return;
	}

	// ClonedAs overrides what comes out of the cloning facility. A mismatch in
	// category cancels cloning outright - infantry can only be cloned as infantry
	// and units only as units - and it cancels the CloningVats path too.
	auto CloneType = ProductionType;
	if(auto const pClonedAs = ProductionTypeData->ClonedAs.Get()) {
		CloneType = pClonedAs;
	}
	if(CloneType->WhatAmI() != ProductionType->WhatAmI()) {
		return;
	}

	auto const FactoryOwner = Factory->Owner;

	auto const& CloningSources = ProductionTypeData->ClonedAt;

	auto KickOutClone = [CloneType, FactoryOwner](BuildingClass *B) -> void {
		auto Clone = static_cast<TechnoClass *>(CloneType->CreateObject(FactoryOwner));
		if(B->KickOutUnit(Clone, CellStruct::Empty) != KickOutResult::Succeeded) {
			Clone->UnInit();
		}
	};

	auto const IsUnit = (FactoryType->Factory != InfantryTypeClass::AbsID);

	// keep cloning vats for backward compat, unless explicit sources are defined
	if(!IsUnit && CloningSources.empty()) {
		for(auto const CloningVat : FactoryOwner->CloningVats) {
			KickOutClone(CloningVat);
		}
	}

	// and clone from new sources
	if(!CloningSources.empty() || IsUnit) {
		for(auto const B : FactoryOwner->Buildings) {
			if(B->InLimbo) {
				continue;
			}
			auto const BType = B->Type;

			auto ShouldClone = false;
			if(!CloningSources.empty()) {
				ShouldClone = CloningSources.Contains(BType);
			} else if(IsUnit) {
				auto const BData = BuildingTypeExt::ExtMap.Find(BType);
				ShouldClone = BData->CloningFacility && (BType->Naval == FactoryType->Naval);
			}

			if(ShouldClone) {
				KickOutClone(B);
			}
		}
	}
}

CoordStruct BuildingExt::GetCenterCoords(BuildingClass* pBuilding, bool includeBib)
{
	CoordStruct ret = pBuilding->GetCoords();
	ret.X += pBuilding->Type->GetFoundationWidth() / 2;
	ret.Y += pBuilding->Type->GetFoundationHeight(includeBib) / 2;
	return ret;
}

// =============================
// load / save

template <typename T>
void BuildingExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->OwnerBeforeRaid)
		.Process(this->FreeUnits_Done)
		.Process(this->AboutToChronoshift)
		.Process(this->SkipBaseNormal)
		.Process(this->PrismForwarding)
		.Process(this->RegisteredJammers)
		.Process(this->SensorArrayActiveCounter)
		.Process(this->SecretLab_Placed)
		.Process(this->TogglePower_HasPower)
		.Process(this->CashUpgradeTimers)
		.Process(this->DockReloadTimers);
}

void BuildingExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<BuildingClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void BuildingExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<BuildingClass, ExtData>::SaveToStream(Stm);
	Serialize(Stm);
}

bool BuildingExt::LoadGlobals(AresStreamReader& Stm) {
	auto ret = Stm
		.Process(TempFoundationData1)
		.Process(TempFoundationData2)
		.Success();

	MouseClass::Instance.CurrentFoundation_Data = TempFoundationData1.data();
	MouseClass::Instance.CurrentFoundationCopy_Data = TempFoundationData2.data();

	return ret;
}

bool BuildingExt::SaveGlobals(AresStreamWriter& Stm) {
	return Stm
		.Process(TempFoundationData1)
		.Process(TempFoundationData2)
		.Success();
}

// =============================
// container

BuildingExt::ExtContainer::ExtContainer() : Container("BuildingClass") {
}

BuildingExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x43BCBD, BuildingClass_CTOR, 0x6)
{
	GET(BuildingClass*, pItem, ESI);

	BuildingExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x43C022, BuildingClass_DTOR, 0x6)
{
	GET(BuildingClass*, pItem, ESI);

	BuildingExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x454190, BuildingClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x453E20, BuildingClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(BuildingClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BuildingExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x45417E, BuildingClass_Load_Suffix, 0x5)
{
	BuildingExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x454244, BuildingClass_Save_Suffix, 0x7)
{
	BuildingExt::ExtMap.SaveStatic();
	return 0;
}

static_assert(sizeof(BuildingExt::cPrismForwarding) == 0x30, "cPrismForwarding must match the 3.0p1 layout");

// the reserves are swapped against 0.A, which is what shrinks the struct to 0x30
static_assert(offsetof(BuildingExt::cPrismForwarding, SupportTarget) == 0x1C, "cPrismForwarding layout slipped");
static_assert(offsetof(BuildingExt::cPrismForwarding, DamageReserve) == 0x24, "cPrismForwarding layout slipped");
static_assert(offsetof(BuildingExt::cPrismForwarding, ModifierReserve) == 0x28, "cPrismForwarding layout slipped");

static_assert(sizeof(BuildingExt::ExtData) == 0xC0, "BuildingExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the bool cluster, the prism block, the jammer
// map, the two bools that moved ahead of the timers, and the last member.
static_assert(offsetof(BuildingExt::ExtData, OwnerBeforeRaid) == 0x08, "BuildingExt::ExtData layout slipped");
static_assert(offsetof(BuildingExt::ExtData, SkipBaseNormal) == 0x0E, "BuildingExt::ExtData layout slipped");
static_assert(offsetof(BuildingExt::ExtData, PrismForwarding) == 0x10, "BuildingExt::ExtData layout slipped");
static_assert(offsetof(BuildingExt::ExtData, RegisteredJammers) == 0x40, "BuildingExt::ExtData layout slipped");
static_assert(offsetof(BuildingExt::ExtData, SecretLab_Placed) == 0x50, "BuildingExt::ExtData layout slipped");
static_assert(offsetof(BuildingExt::ExtData, CashUpgradeTimers) == 0x54, "BuildingExt::ExtData layout slipped");
static_assert(offsetof(BuildingExt::ExtData, DockReloadTimers) == 0x78, "BuildingExt::ExtData layout slipped");
