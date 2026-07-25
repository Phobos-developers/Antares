#include "Body.h"
#include "../../Utilities/AresEnums.h"
#include "../BuildingType/Body.h"
#include "../HouseType/Body.h"
#include "../Rules/Body.h"
#include "../TechnoType/Body.h"
#include "../WeaponType/Body.h"
#include "../../Misc/FlyingStrings.h"
#include "../../Misc/SWTypes.h"
#include "../../Misc/PoweredUnitClass.h"
#include "../../Utilities/TemplateDef.h"

#include <AircraftClass.h>
#include <AnimClass.h>
#include <BombClass.h>
#include <BuildingLightClass.h>
#include <CaptureManagerClass.h>
#include <CellSpread.h>
#include <Conversions.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <BuildingClass.h>
#include <GeneralStructures.h>
#include <Helpers/Enumerators.h>
#include <Helpers/Template.h>
#include <LocomotionClass.h>
#include <SlaveManagerClass.h>
#include <SpawnManagerClass.h>
#include <TagClass.h>
#include <TeamClass.h>
#include <TemporalClass.h>
#include <TiberiumClass.h>
#include <UnitClass.h>

#include <algorithm>
#include <utility>

TechnoExt::ExtContainer TechnoExt::ExtMap;

int TechnoExt::NeedsRegap = 0;

bool TechnoExt::DrawnShadowManually = false;

void TechnoExt::SpawnSurvivors(FootClass* const pThis, TechnoClass* const pKiller, const bool Select, const bool IgnoreDefenses)
{
	auto const pType = pThis->GetTechnoType();
	auto const pOwner = pThis->Owner;
	auto const location = pThis->Location;

	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	// do not ever do this again for this unit
	auto const survivorsDone = pExt->Survivors_Done;
	pExt->Survivors_Done = true;

	// always eject passengers, but crew only if not already processed.
	if(!survivorsDone && !pExt->DriverKilled && !IgnoreDefenses) {
		// save this, because the hijacker can kill people
		auto pilotCount = pThis->GetCrewCount();

		// process the hijacker
		if(auto const pHijacker = RecoverHijacker(pThis)) {
			auto const pHijackerTypeExt = TechnoTypeExt::ExtMap.Find(pHijacker->Type);

			if(!EjectRandomly(pHijacker, location, 144, Select)) {
				pHijacker->RegisterDestruction(pKiller);
				GameDelete(pHijacker);
			} else {
				// the hijacker will now be controlled instead of the unit
				if(auto const pController = pThis->MindControlledBy) {
					++Unsorted::ScenarioInit; // disables sound effects
					pController->CaptureManager->FreeUnit(pThis);
					pController->CaptureManager->CaptureUnit(pHijacker); // does the immunetopsionics check for us
					--Unsorted::ScenarioInit;
					pHijacker->QueueMission(Mission::Guard, true); // override the fate the AI decided upon
				}

				VocClass::PlayAt(pHijackerTypeExt->HijackerLeaveSound, location, nullptr);

				// lower than 0: kill all, otherwise, kill n pilots
				pilotCount = ((pHijackerTypeExt->HijackerKillPilots < 0) ? 0 : (pilotCount - pHijackerTypeExt->HijackerKillPilots));
			}
		}

		int pilotChance = pTypeExt->Survivors_PilotChance.Get(pThis);
		if(pilotChance < 0) {
			pilotChance = static_cast<int>(RulesClass::Instance->CrewEscape * 100);
		}

		// possibly eject up to pilotCount crew members
		if(pType->Crewed && pilotChance > 0) {
			for(int i = 0; i < pilotCount; ++i) {
				if(auto pPilotType = pThis->GetCrew()) {
					if(ScenarioClass::Instance->Random.RandomRanged(1, 100) <= pilotChance) {
						auto const pPilot = static_cast<InfantryClass*>(pPilotType->CreateObject(pOwner));
						pPilot->Health /= 2;
						pPilot->Veterancy.Veterancy = pThis->Veterancy.Veterancy;

						if(!EjectRandomly(pPilot, location, 144, Select)) {
							pPilot->RegisterDestruction(pKiller);
							GameDelete(pPilot);
						} else if(auto const pTag = pThis->AttachedTag) {
							if(pTag->ShouldReplace()) {
								pPilot->ReplaceTag(pTag);
							}
						}
					}
				}
			}
		}
	}

	// passenger escape chances
	auto const passengerChance = pTypeExt->Survivors_PassengerChance.Get(pThis);

	// eject or kill all passengers
	while(pThis->Passengers.GetFirstPassenger()) {
		auto const pPassenger = pThis->RemoveFirstPassenger();
		if(!IgnoreDefenses) {
			bool trySpawn = false;
			if(passengerChance > 0) {
				trySpawn = ScenarioClass::Instance->Random.RandomRanged(1, 100) <= passengerChance;
			} else if(passengerChance == -1 && pThis->WhatAmI() == UnitClass::AbsID) {
				Move occupation = pPassenger->IsCellOccupied(pThis->GetCell(), FacingType::None, -1, nullptr, true);
				trySpawn = (occupation == Move::OK || occupation == Move::MovingBlock);
			}
			if(trySpawn && EjectRandomly(pPassenger, location, 128, Select)) {
				continue;
			}
		}

		// kill passenger, if not spawned
		pPassenger->RegisterDestruction(pKiller);
		pPassenger->UnInit();
	}
}

/**
	\param Survivor Passenger to eject
	\param loc Where to put the passenger
	\param Select Whether to select the Passenger afterwards
*/
bool TechnoExt::EjectSurvivor(FootClass *Survivor, CoordStruct loc, bool Select)
{
	CellClass* pCell = MapClass::Instance.TryGetCellAt(loc);

	if(!pCell) {
		return false;
	}

	Survivor->OnBridge = pCell->ContainsBridge();

	int floorZ = pCell->GetCoordsWithBridge().Z;
	bool chuted = (loc.Z - floorZ > 2 * Unsorted::LevelHeight);
	// both placements run inside the mutex: without it the survivor gets treated
	// as an ordinary arrival and can be relocated away from the wreck
	if(chuted) {
		// HouseClass::CreateParadrop does this when building passengers for a paradrop... it might be a wise thing to mimic!
		Survivor->Limbo();

		++Unsorted::ScenarioInit;
		auto const spawned = Survivor->SpawnParachuted(loc);
		--Unsorted::ScenarioInit;

		if(!spawned) {
			return false;
		}
	} else {
		loc.Z = floorZ;
		// Unlimbo takes a 256-direction facing, so the random eight-way pick has to
		// be scaled into that space. Shipped (0x100452A6) does it as `shl eax, 0Dh;
		// movzx eax, ax; shr eax, 8`, which is this shift.
		auto const facing = static_cast<DirType>(
			ScenarioClass::Instance->Random.RandomRanged(0, 7) << 5);

		++Unsorted::ScenarioInit;
		auto const placed = Survivor->Unlimbo(loc, facing);
		--Unsorted::ScenarioInit;

		if(!placed) {
			return false;
		}
	}

	Survivor->Transporter = nullptr;
	Survivor->LastMapCoords = pCell->MapCoords;

	// don't ask, don't tell
	if(chuted) {
		bool scat = Survivor->OnBridge;
		auto occupation = scat ? pCell->AltOccupationFlags : pCell->OccupationFlags;
		if((occupation & 0x1C) == 0x1C) {
			pCell->ScatterContent(CoordStruct::Empty, true, true, scat);
		}
	} else {
		Survivor->Scatter(CoordStruct::Empty, true, false);
		Survivor->QueueMission(Survivor->Owner->IsControlledByHuman() ? Mission::Guard : Mission::Hunt, 0);
	}

	Survivor->ShouldEnterOccupiable = false;
	Survivor->ShouldGarrisonStructure = false;

	if(Select) {
		Survivor->Select();
	}

	return true;
	//! \todo Tag
}

/**
	This function ejects a given number of passengers from the passed transporter.

	\param pThis Pointer to the transporter
	\param howMany How many passengers to eject - pass negative number for "all"
	\author Renegade
	\date 27.05.2010
*/
void TechnoExt::EjectPassengers(FootClass *pThis, int howMany) {
	if(howMany < 0) {
		howMany = pThis->Passengers.NumPassengers;
	}

	auto const openTopped = pThis->GetTechnoType()->OpenTopped;

	for(int i = 0; i < howMany && pThis->Passengers.FirstPassenger; ++i) {
		FootClass *passenger = pThis->RemoveFirstPassenger();
		if(!EjectRandomly(passenger, pThis->Location, 128, false)) {
			passenger->RegisterDestruction(nullptr);
			passenger->UnInit();
		} else if(openTopped) {
			pThis->ExitedOpenTopped(passenger);
		}
	}
}


/**
	This function drops the coordinates of an infantry subposition into the target parameter.
	Could probably work for vehicles as well, though they'd be off-center.

	\param current The current position of the transporter, the starting point to look from
	\param target A CoordStruct to save the finally computed position to
	\param distance The distance in leptons from the current position
	\author Renegade
	\date 27.05.2010
*/
CoordStruct TechnoExt::GetPutLocation(CoordStruct current, int distance) {
	// this whole thing does not at all account for cells which are completely occupied.
	auto tmpCoords = CellSpread::GetCell(ScenarioClass::Instance->Random.RandomRanged(0, 7));

	current.X += tmpCoords.X * distance;
	current.Y += tmpCoords.Y * distance;

	auto tmpCell = MapClass::Instance.GetCellAt(current);
	auto target = tmpCell->FindInfantrySubposition(current, false, false, false);

	target.Z = current.Z;
	return target;
}

//! Places a unit next to a given location on the battlefield.
/**
	
	\param pEjectee The FootClass to be ejected.
	\param location The current position of the transporter, the starting point to look from
	\param distance The distance in leptons from the current position
	\param select Whether the placed FootClass should be selected
	\author AlexB
	\date 12.04.2011
*/
bool TechnoExt::EjectRandomly(FootClass* pEjectee, CoordStruct const &location, int distance, bool select) {
	CoordStruct destLoc = GetPutLocation(location, distance);
	return TechnoExt::EjectSurvivor(pEjectee, destLoc, select);
}

//! Breaks the link between DrainTarget and DrainingMe.
/*!
	The links between the drainer and its victim are removed and the draining
	animation is no longer played.

	\param Drainer The Techno that drains power or credits.
	\param Drainee The Techno that power or credits get gets drained from.

	\author AlexB
	\date 2010-04-27
*/
void TechnoExt::StopDraining(TechnoClass *Drainer, TechnoClass *Drainee) {
	// fill the gaps
	if(Drainer && !Drainee)
		Drainee = Drainer->DrainTarget;

	if(!Drainer && Drainee)
		Drainer = Drainee->DrainingMe;

	// sanity check, then end draining.
	if(Drainer && Drainee) {
		// stop the animation.
		if (Drainer->DrainAnim) {
			Drainer->DrainAnim->UnInit();
			Drainer->DrainAnim = nullptr;
		}

		// remove links.
		Drainee->DrainingMe = nullptr;
		Drainer->DrainTarget = nullptr;

		// tell the game to recheck the drained
		// player's tech level.
		if (Drainee->Owner) {
			Drainee->Owner->RecheckTechTree = true;
		}
	}
}

bool TechnoExt::SpawnVisceroid(CoordStruct &crd, ObjectTypeClass* pType, int chance, bool ignoreTibDeathToVisc) {
	bool ret = false;
	// create a small visceroid if available and the cell is free
	if(ignoreTibDeathToVisc || ScenarioClass::Instance->TiberiumDeathToVisceroid) {
		auto pCell = MapClass::Instance.GetCellAt(crd);
		int rnd = ScenarioClass::Instance->Random.RandomRanged(0, 99);
		if(!(pCell->OccupationFlags & 0x20) && rnd < chance && pType) {
			if(auto pHouse = HouseClass::FindNeutral()) {
				if(auto pVisc = pType->CreateObject(pHouse)) {
					++Unsorted::ScenarioInit;
					ret = true;
					if(!pVisc->Unlimbo(crd, DirType::North)) {
						// opposed to TS, we clean up, though
						// the mutex should make it happen.
						pVisc->UnInit();
						ret = false;
					}
					--Unsorted::ScenarioInit;
				}
			}
		}
	}
	return ret;
}

int TechnoExt::DecreaseAmmo(
	TechnoClass* const pThis, WeaponTypeClass const* const pWeapon)
{
	int count = 1;
	if(auto const pExt = WeaponTypeExt::ExtMap.Find(pWeapon)) {
		count = pExt->Ammo;
	}

	for(auto i = count; i > 0; --i) {
		pThis->DecreaseAmmo();
	}

	return count;
}

unsigned int TechnoExt::ExtData::AlphaFrame(const SHPStruct* Image) const {
	int countFrames = Conversions::Int2Highest(Image->Frames);
	DirStruct Facing = this->OwnerObject()->PrimaryFacing.Current();
	return (Facing.Raw >> (16 - countFrames));
}

bool TechnoExt::ExtData::DrawVisualFX() const {
	auto Object = this->OwnerObject();
	if(Object->VisualCharacter(VARIANT_TRUE, Object->Owner) == VisualType::Normal) {
		if(!Object->Disguised) {
			return true;
		}
	}
	return false;
}

bool TechnoExt::ExtData::AcquireHunterSeekerTarget() const {
	auto const pThis = this->OwnerObject();

	if(!pThis->Target) {
		std::vector<TechnoClass*> preferredTargets;
		std::vector<TechnoClass*> randomTargets;

		// defaults if SW isn't set
		auto pOwner = pThis->GetOwningHouse();
		SWTypeExt::ExtData* pSWExt = nullptr;
		auto canPrefer = true;

		// check the hunter seeker SW
		if(auto const pSuper = this->SuperWeapon) {
			pOwner = pSuper->Owner;
			pSWExt = SWTypeExt::ExtMap.Find(pSuper->Type);
			canPrefer = !pSWExt->HunterSeeker_RandomOnly;
		}

		auto const isHumanControlled = pOwner->IsControlledByHuman();
		auto const mode = SessionClass::Instance.GameMode;

		// the AI in multiplayer games only attacks its favourite enemy
		auto const pFavouriteEnemy = HouseClass::Array.GetItemOrDefault(pOwner->EnemyHouseIndex);
		auto const favouriteEnemyOnly = (mode != GameMode::Campaign
			&& pFavouriteEnemy && !isHumanControlled);

		for(auto const& i : TechnoClass::Array) {

			// is the house ok?
			if(favouriteEnemyOnly) {
				if(i->Owner != pFavouriteEnemy) {
					continue;
				}
			} else if(!pSWExt && pOwner->IsAlliedWith(i->Owner)) {
				// default without SW
				continue;
			} else if(pSWExt && !pSWExt->IsHouseAffected(pOwner, i->Owner)) {
				// use SW
				continue;
			}

			// techno ineligible
			if(i->Health < 0 || i->InLimbo || !i->IsAlive) {
				continue;
			}

			// type prevents this being a target
			auto const pType = i->GetTechnoType();
			if(pType->Invisible || !pType->LegalTarget) {
				continue;
			}

			// is type to be ignored?
			auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
			if(pExt->HunterSeekerIgnore) {
				continue;
			}

			// harvester truce
			if(ScenarioClass::Instance->SpecialFlags.HarvesterImmune) {
				if(auto const pUnitType = abstract_cast<UnitTypeClass*>(pType)) {
					if(pUnitType->Harvester) {
						continue;
					}
				}
			}

			// allow to exclude certain techno types
			if(pSWExt && !pSWExt->IsTechnoAffected(i)) {
				continue;
			}

			// in multiplayer games, non-civilian targets are preferred
			// for human players
			auto const isPreferred = mode != GameMode::Campaign && isHumanControlled
				&& !i->Owner->Type->MultiplayPassive && canPrefer;

			// add to the right list
			if(isPreferred) {
				preferredTargets.push_back(i);
			} else {
				randomTargets.push_back(i);
			}
		}

		auto const& targets = !preferredTargets.empty() ? preferredTargets : randomTargets;

		if(auto const count = static_cast<int>(targets.size())) {
			auto const index = ScenarioClass::Instance->Random.RandomRanged(0, count - 1);
			auto const& pTarget = targets[index];

			// that's our target
			pThis->SetTarget(pTarget);
			return true;
		}
	}

	return false;
}

UnitTypeClass* TechnoExt::ExtData::GetUnitType() const {
	if(auto pUnit = abstract_cast<UnitClass*>(this->OwnerObject())) {
		auto pData = TechnoTypeExt::ExtMap.Find(pUnit->Type);
		if(pData->WaterImage && !pUnit->OnBridge && pUnit->GetCell()->LandType == LandType::Water) {
			return pData->WaterImage;
		}
	}
	return nullptr;
}

/*! This function checks if this object can currently be used, in terms of having or needing an operator.
	\return true if it needs an operator and has one, <b>or if it doesn't need an operator in the first place</b>. false if it needs an operator and doesn't have one.
	\author Renegade
	\date 27.04.10
*/
bool TechnoExt::ExtData::IsOperated() {
	auto pThis = this->OwnerObject();
	auto pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if(!pExt->Operator.empty()) {
		// loop & condition come from D
		for(NextObject object(pThis->Passengers.GetFirstPassenger()); object; ++object) {
			auto const pType = object->GetType();
			auto const isOperator = [pType](InfantryTypeClass* const pOperator) {
				return pOperator == pType;
			};

			if(std::any_of(pExt->Operator.begin(), pExt->Operator.end(), isOperator)) {
				// takes a specific operator and someone is present AND that someone is the operator, therefore it is operated
				return true;
			}
		}
		// takes a specific operator but either no one is present or it's not the operator, therefore it's not operated
		return false;
	} else if(pExt->IsAPromiscuousWhoreAndLetsAnyoneRideIt) {
		// takes anyone, therefore it's operated if anyone is there
		return pThis->Passengers.GetFirstPassenger() != nullptr;
	} else {
		/* Isn't even set as an Operator-using object, therefore we are returning TRUE,
		 since, logically, if it doesn't need operators, it can be/is operated, no matter if there are passengers or not.
		 (Also, if we didn't do this, Reactivate() would fail for for non-Operator-units, for example.) */

		// latch it: this answer can only change when the type does, and
		// TechnoExt::UpdateType clears the flag again.
		this->AlwaysOperated = true;
		return true;
	}
}

/*! This function checks if this object can currently be used, in terms of having or needing a powering structure and that structure's status.
	\return true if it needs a structure and has an active one, <b>or if it doesn't need a structure in the first place</b>. false if it needs a structure and doesn't have an active one.
	\author Renegade
	\date 27.04.10
*/
bool TechnoExt::ExtData::IsPowered() const {
	auto pThis = this->OwnerObject();
	auto pType = pThis->GetTechnoType();

	if(pType && pType->PoweredUnit) {
		for(const auto& pBuilding : pThis->Owner->Buildings) {
			if(pBuilding->Type->PowersUnit == pType
				&& pBuilding->RegisteredAsPoweredUnitSource
				&& !pBuilding->IsUnderEMP()) // alternatively, HasPower, IsPowerOnline()
			{
				return true; 
			}
		}
		// if we reach this, we found no building that currently powers this object
		return false;
	} else if(this->PoweredUnit) {
		// #617
		return this->PoweredUnit->IsPowered();
	} else {
		// object doesn't need a particular powering structure, therefore, for the purposes of the game, it IS powered
		return true;
	}
}

/*
 * Object should NOT be placed on the map (->Limbo() it or don't Put in the first place)
 * otherwise Bad Things (TM) will happen. Again.
 */
bool TechnoExt::CreateWithDroppod(FootClass *Object, const CoordStruct& XYZ) {
	auto MyCell = MapClass::Instance.GetCellAt(XYZ);
	if(Object->IsCellOccupied(MyCell, FacingType::None, -1, nullptr, false) != Move::OK) {
//		Debug::Log("Cell occupied... poof!\n");
		return false;
	} else {
//		Debug::Log("Destinating %s @ {%d, %d, %d}\n", Object->GetType()->ID, XYZ->X, XYZ->Y, XYZ->Z);
		LocomotionClass::ChangeLocomotorTo(Object, LocomotionClass::CLSIDs::Droppod);
		CoordStruct xyz = XYZ;
		xyz.Z = 0;
		Object->SetLocation(xyz);
		Object->SetDestination(MyCell, 1);
		Object->Locomotor->Move_To(XYZ);
		Object->PrimaryFacing.SetCurrent(DirStruct());
		if(!Object->InLimbo) {
			Object->See(0, 0);
			Object->QueueMission(Mission::Guard, 0);
			Object->NextMission();
			return true;
		}
		//Debug::Log("InLimbo... failed?\n");
		return false;
	}
}

// destroy a given techno by dealing absolute damage
void TechnoExt::Destroy(TechnoClass* pTechno, TechnoClass* pKiller, HouseClass* pKillerHouse, WarheadTypeClass* pWarhead) {
	if(!pKillerHouse && pKiller) {
		pKillerHouse = pKiller->Owner;
	}

	if(!pWarhead) {
		pWarhead = RulesClass::Instance->C4Warhead;
	}

	int health = pTechno->Health;
	pTechno->ReceiveDamage(&health, 0, pWarhead, pKiller, true, false, pKillerHouse);
}

// swaps the type of an infantry, vehicle or aircraft in place, keeping its
// health ratio and reapplying everything the type contributes.
bool TechnoExt::UpdateType(TechnoClass* const pThis, TechnoTypeClass* const pToType)
{
	TechnoTypeClass** ppType = nullptr;
	auto wanted = AbstractType::None;

	switch(pThis->WhatAmI()) {
	case AbstractType::Infantry:
		ppType = reinterpret_cast<TechnoTypeClass**>(&static_cast<InfantryClass*>(pThis)->Type);
		wanted = AbstractType::InfantryType;
		break;
	case AbstractType::Unit:
		ppType = reinterpret_cast<TechnoTypeClass**>(&static_cast<UnitClass*>(pThis)->Type);
		wanted = AbstractType::UnitType;
		break;
	case AbstractType::Aircraft:
		ppType = reinterpret_cast<TechnoTypeClass**>(&static_cast<AircraftClass*>(pThis)->Type);
		wanted = AbstractType::AircraftType;
		break;
	default:
		break;
	}

	if(pToType->WhatAmI() != wanted) {
		return false;
	}

	if(auto const pTemporal = pThis->TemporalImUsing) {
		if(pTemporal->Target) {
			// Detach drops the link without releasing the victim, which leaves it
			// frozen with nothing left to warp it back in
			pTemporal->LetGo();
		}
	}

	auto const pOwner = pThis->Owner;
	if(!pThis->InLimbo) {
		pOwner->RegisterLoss(pThis, false);
	}
	pOwner->RemoveTracking(pThis);

	auto const pFromType = *ppType;
	auto const ratio = pThis->Health / static_cast<double>(pThis->GetTechnoType()->Strength);

	*ppType = pToType;
	pThis->SetHealthPercentage(ratio);
	pThis->EstimatedHealth = pThis->Health;

	pOwner->AddTracking(pThis);
	if(!pThis->InLimbo) {
		pOwner->RegisterGain(pThis, true);
	}
	pOwner->RecheckTechTree = true;

	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	pExt->AlwaysOperated = false;

	// the effect the old type granted goes away with it
	pExt->AttachedTechnoEffect_isset = 0;
	auto& effects = pExt->AttachedEffects;
	auto const it = std::find_if(effects.begin(), effects.end(),
		[pFromType](AttachEffectClass const& item) {
			return item.Type->Owner == pFromType;
		});
	if(it != effects.end()) {
		effects.erase(it);
	}

	pExt->RecalculateStats();

	if(pToType->Ammo < pThis->Ammo) {
		pThis->Ammo = pToType->Ammo;
	}

	auto const pToTypeExt = TechnoTypeExt::ExtMap.Find(pToType);
	pExt->SetSpotlight(nullptr);
	if(pToTypeExt->Is_Spotlighted) {
		pExt->SetSpotlight(GameCreate<BuildingLightClass>(pThis));
	}

	pThis->PrimaryFacing.SetROT(static_cast<short>(pToType->ROT));
	pThis->SecondaryFacing.SetROT(
		static_cast<short>(pToTypeExt->TurretROT.Get(pToType->ROT)));

	auto const pFoot = static_cast<FootClass*>(pThis);

	if(pToType->Locomotor != pFromType->Locomotor) {
		auto pLoco = LocomotionClass::CreateInstance(pToType->Locomotor);
		pLoco->Link_To_Object(pThis);
		pFoot->Locomotor = std::move(pLoco);
	}

	// a jumpjet that has to land before it can deploy is told to go
	// nowhere, which makes it descend
	if(pToType->BalloonHover && pToType->DeployToLand
		&& pToType->Locomotor == LocomotionClass::CLSIDs::Jumpjet
		&& pFromType->Locomotor != LocomotionClass::CLSIDs::Jumpjet)
	{
		pFoot->Locomotor->Move_To(pThis->GetCoords());
	}

	return true;
}

void TechnoExt::TransferIvanBomb(TechnoClass *From, TechnoClass *To) {
	if(auto Bomb = From->AttachedBomb) {
		From->AttachedBomb = nullptr;
		Bomb->Target = To;
		To->AttachedBomb = Bomb;
		To->BombVisible = From->BombVisible;
		// if there already was a bomb attached to target unit, it's gone now...
		// it shouldn't happen though, this is used for (un)deploying objects only
	}
}

void TechnoExt::TransferOriginalOwner(TechnoClass* pFrom, TechnoClass* pTo) {
	auto pFromExt = TechnoExt::ExtMap.Find(pFrom);
	auto pToExt = TechnoExt::ExtMap.Find(pTo);

	pToExt->OriginalHouseType = pFromExt->OriginalHouseType;
}

void TechnoExt::TransferAttachedEffects(TechnoClass *From, TechnoClass *To) {
	auto FromExt = TechnoExt::ExtMap.Find(From);
	auto ToExt = TechnoExt::ExtMap.Find(To);

	ToExt->AttachedEffects.clear();

	// while recreation itself isn't the best idea, less hassle and more reliable
	// list gets intact in the end
	for(const auto& Item : FromExt->AttachedEffects) {
		Item.Type->Attach(To, Item.ActualDuration, Item.Invoker);
	}

	FromExt->AttachedEffects.clear();
	FromExt->AttachedTechnoEffect_isset = false;
	ToExt->RecalculateStats();
}

/*! This function recalculates the stats modifiable by crates and update them (aimed for request #255)
	\todo code that crate effects not get ignored
	\author Graion Dilach
	\date 2011-10-12
*/
void TechnoExt::ExtData::RecalculateStats() {
	auto const pThis = this->OwnerObject();

	auto Firepower = this->Crate_FirepowerMultiplier;
	auto Armor = this->Crate_ArmorMultiplier;
	auto Speed = this->Crate_SpeedMultiplier; //if there's hooks for crate-stuff, they could be the base for this
	auto ROF = 1.0;
	auto Cloak = TechnoExt::CanICloakByDefault(pThis) || this->Crate_Cloakable;

	//Debug::Log("[AttachEffect]Recalculating stats of %s...\n", pThis->get_ID());

	for(const auto& Item : this->AttachedEffects) {
		auto const pType = Item.Type;
		Firepower *= pType->FirepowerMultiplier;
		Speed *= pType->SpeedMultiplier;
		Armor *= pType->ArmorMultiplier;
		ROF *= pType->ROFMultiplier;
		Cloak |= pType->Cloakable;
	}

	pThis->FirepowerMultiplier = Firepower;
	pThis->ArmorMultiplier = Armor;
	this->AttachEffects_ROFMultiplier = ROF;

	pThis->Cloakable = Cloak;

	if(auto const pFoot = abstract_cast<FootClass*>(pThis)) {
		pFoot->SpeedMultiplier = Speed;
	}

	//Debug::Log("[AttachEffect]Calculation was successful.\n", pThis->get_ID());
}

int TechnoExt::ExtData::GetSelfHealAmount() const
{
	auto const pThis = this->OwnerObject();
	auto const pType = pThis->GetTechnoType();

	// no self-healing while the unit is still considered to be in combat
	if(this->SelfHealCombatTimer.InProgress()) {
		return 0;
	}

	if(pType->SelfHealing || pThis->HasAbility(Ability::SelfHeal)) {
		auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

		auto const rate = pExt->SelfHealing_Rate.Get(
			RulesClass::Instance->RepairRate);
		auto const frames = Math::max(static_cast<int>(rate * 900.0), 1);

		if(Unsorted::CurrentFrame % frames == 0) {
			auto const strength = pType->Strength;

			auto const percent = pExt->SelfHealing_Max.Get(pThis);
			auto const maxHealth = Math::clamp(
				static_cast<int>(percent * strength) + 1, 1, strength);

			auto const health = pThis->Health;
			if(health && health < maxHealth) {
				auto const amount = pExt->SelfHealing_Amount.Get(pThis);
				return Math::clamp(amount, 0, maxHealth - health);
			}
		}
	}

	return 0;
}

void TechnoExt::ExtData::CreateInitialPayload()
{
	if(this->PayloadCreated) {
		return;
	}
	this->PayloadCreated = true;

	auto const pThis = this->OwnerObject();
	auto const pType = pThis->GetTechnoType();
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	auto const pBld = abstract_cast<BuildingClass*>(pThis);
	auto const pBldType = pBld ? pBld->Type : nullptr;

	auto freeSlots = (pBld && pBldType->CanBeOccupied)
		? pBldType->MaxNumberOccupants - pBld->GetOccupantCount()
		: pType->Passengers - pThis->Passengers.NumPassengers;

	auto const sizePayloadNum = pTypeExt->InitialPayload_Nums.size();

	for(auto i = 0u; i < pTypeExt->InitialPayload_Types.size(); ++i) {
		auto const pPayloadType = pTypeExt->InitialPayload_Types[i];

		if(!pPayloadType) {
			continue;
		}

		// buildings and aircraft aren't valid payload, and building payload
		// can only be infantry
		auto const absPayload = pPayloadType->WhatAmI();
		if(absPayload == AbstractType::BuildingType
			|| absPayload == AbstractType::AircraftType
			|| (pBld && absPayload != AbstractType::InfantryType))
		{
			continue;
		}

		// if there are no nums, index gets huge and invalid, which means 1
		auto const idxPayloadNum = Math::min(i + 1, sizePayloadNum) - 1;
		auto const payloadNum = (idxPayloadNum < sizePayloadNum)
			? pTypeExt->InitialPayload_Nums[idxPayloadNum] : 1;

		// never fill in more than allowed
		auto const count = Math::min(payloadNum, freeSlots);
		freeSlots -= count;

		for(auto j = 0; j < count; ++j) {
			// the payload is built from inside the carrier's own init, where the
			// mutex is set. Leaving it set costs the payload its Academy veterancy,
			// and teams spawned by trigger actions come out empty entirely.
			auto const scenarioInit = std::exchange(Unsorted::ScenarioInit, 0);
			auto const pObject = pPayloadType->CreateObject(pThis->Owner);
			Unsorted::ScenarioInit = scenarioInit;

			if(pBld) {
				// buildings only allow infantry payload, so this in infantry
				auto const pPayload = static_cast<InfantryClass*>(pObject);

				if(pBldType->CanBeOccupied) {
					pBld->Occupants.AddItem(pPayload);

					auto const pCell = pThis->GetCell();
					pThis->UpdateThreatInCell(pCell);
				} else {
					pPayload->Limbo();

					if(pBldType->InfantryAbsorb) {
						pPayload->Absorbed = true;

						if(pPayload->CountedAsOwnedSpecial) {
							--pPayload->Owner->OwnedInfantry;
							pPayload->CountedAsOwnedSpecial = false;
						}

						if(pBldType->ExtraPowerBonus > 0) {
							pBld->Owner->RecheckPower = true;
						}
					} else {
						pPayload->SendCommand(RadioCommand::RequestLink, pBld);
					}

					pBld->Passengers.AddPassenger(pPayload);
					pPayload->AbortMotion();
				}

			} else {
				auto const pPayload = static_cast<FootClass*>(pObject);
				pPayload->SetLocation(pThis->Location);
				pPayload->Limbo();

				if(pType->OpenTopped) {
					pThis->EnteredOpenTopped(pPayload);
				}

				pPayload->Transporter = pThis;

				auto const old = std::exchange(VocClass::VoicesEnabled, false);
				pThis->AddPassenger(pPayload);
				VocClass::VoicesEnabled = old;
			}
		}
	}
}

/*! This function calculates whether the unit would be cloaked by default
	\author Graion Dilach
	\date 2011-10-16
*/
bool TechnoExt::CanICloakByDefault(TechnoClass *pTechno) {
	//Debug::Log("[AttachEffect]Can %s cloak by default?\n", pTechno->get_ID());
	auto tType = pTechno->GetTechnoType();
	return tType->Cloakable || pTechno->HasAbility(Ability::Cloak);
}

bool TechnoExt::IsCloaked(TechnoClass* pTechno) {
	if(pTechno->CloakState == CloakState::Cloaked) {
		return true;
	} else if(auto pBld = abstract_cast<BuildingClass*>(pTechno)) {
		return (pBld->Translucency == 15);
	}
	return false;
}

bool TechnoExt::ExtData::IsDeactivated() const {
	return this->OwnerObject()->Deactivated;
}

void TechnoExt::ExtData::InvalidateAttachEffectPointer(void *ptr) {
	for(auto& Item : this->AttachedEffects) {
		Item.InvalidatePointer(ptr);
	}
}

/*! This function detaches a specific spawned object from it's spawner.
	The check if it's a spawned object at all should be done before this function is called.
	Check for SpawnOwner, specifically.

	\param Spawnee The spawned object
	\param NewSpawneeOwner The house which should control the spawnee after the function
	\author Graion Dilach
	\date 2011-06-09
	\todo Get an assembly-reader to document Status in YR++ and update Status accordingly
*/
void TechnoExt::DetachSpecificSpawnee(TechnoClass *Spawnee, HouseClass *NewSpawneeOwner) {

	// setting up the nodes. Funnily, nothing else from the manager is needed
	const auto& SpawnNodes = Spawnee->SpawnOwner->SpawnManager->SpawnedNodes;

	//find the specific spawnee in the node
	for(auto SpawnControl : SpawnNodes) {

		if(Spawnee == SpawnControl->Unit) {

			SpawnControl->Unit = nullptr;
			Spawnee->SpawnOwner = nullptr;

			SpawnControl->Status = SpawnNodeStatus::Dead;

			Spawnee->SetOwningHouse(NewSpawneeOwner);
		}
	}
}

/*! This function frees a specific slave from it's manager.
	The check if it's a slave at all should be done before this function is called.
	Check for SlaveOwner, specifically.

	\param Slave The slave which should be freed
	\param Affector The house which causes this slave to be freed (where it should be freed to)
	\author Graion Dilach
	\date 2011-06-09
*/
void TechnoExt::FreeSpecificSlave(TechnoClass *Slave, HouseClass *Affector){

	//If you're a slave, you're an InfantryClass. But since most functions use TechnoClasses and the check can be done in that level as well
	//it's easier to set up the recasting in this function
	//Anybody who writes 357, take note that SlaveManager uses InfantryClasses everywhere, SpawnManager uses TechnoClasses derived from AircraftTypeClasses
	//as I wrote it in http://bugs.renegadeprojects.com/view.php?id=357#c10331
	//So, expand that one instead, kthx.

	if(InfantryClass * pSlave = specific_cast<InfantryClass *>(Slave)) {
		auto Manager= pSlave->SlaveOwner->SlaveManager;

		//LostSlave can free the unit from the miner, so we're awesome. 
		Manager->LostSlave(pSlave);
		pSlave->SlaveOwner = nullptr;

		//OK, delinked, Now relink it to the side which separated the slave from the miner
		pSlave->SetOwningHouse(Affector);
	}
}

// If available, creates an InfantryClass instance and removes the hijacker from the victim.
InfantryClass* TechnoExt::RecoverHijacker(FootClass* const pThis) {
	auto const pType = InfantryTypeClass::Array.GetItemOrDefault(
		pThis->HijackerInfantryType);

	if(pType) {
		auto const pExt = TechnoExt::ExtMap.Find(pThis);
		auto const pOwner = pExt->HijackerHouse ?
			pExt->HijackerHouse : pThis->Owner;

		pThis->HijackerInfantryType = -1;

		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
		if(!pTypeExt->HijackerOneTime && pOwner && !pOwner->Defeated) {
			auto const pHijacker = static_cast<InfantryClass*>(pType->CreateObject(pOwner));
			pHijacker->Health = std::max(pExt->HijackerHealth, 10) / 2;
			pHijacker->Veterancy.Veterancy = pExt->HijackerVeterancy;
			return pHijacker;
		}
	}

	return nullptr;
}

// this isn't called VehicleThief action, because it also includes other logic
// related to infantry getting into an vehicle like CanDrive.
AresAction TechnoExt::ExtData::GetActionHijack(TechnoClass* const pTarget) {
	const auto pThis = abstract_cast<const InfantryClass*>(this->OwnerObject());
	if(!pThis || !pTarget || !pThis->IsAlive || !pTarget->IsAlive) {
		return AresAction::None;
	}

	const auto pType = pThis->Type;
	const auto pTargetType = pTarget->GetTechnoType();
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	// this can't steal vehicles
	if(!pType->VehicleThief && !pTypeExt->CanDrive) {
		return AresAction::None;
	}

	// i'm in a state that forbids capturing
	if(pThis->IsDeployed() || !this->IsOperated()) {
		return AresAction::None;
	}

	// target type is not eligible (hijackers can also enter strange buildings)
	const auto absTarget = pTarget->WhatAmI();
	if(absTarget != AbstractType::Aircraft && absTarget != AbstractType::Unit
		&& (!pType->VehicleThief || absTarget != AbstractType::Building)) {
			return AresAction::None;
	}

	// target is bad
	if(pTarget->CurrentMission == Mission::Selling || pTarget->IsBeingWarpedOut()
		|| pTargetType->IsTrain || pTargetType->BalloonHover
		|| (absTarget != AbstractType::Unit && !pTarget->IsStrange())
		//|| (absTarget == abs_Unit && ((UnitTypeClass*)pTargetType)->NonVehicle) replaced by Hijacker.Allowed
		|| !pTarget->IsOnFloor()) {
			return AresAction::None;
	}

	// bunkered units can't be hijacked.
	if(pTarget->BunkerLinkedItem) {
		return AresAction::None;
	}

	// a thief that can't break mind control loses without trying further
	if(pType->VehicleThief) { 
		if(pTarget->IsMindControlled() && !pTypeExt->HijackerBreakMindControl) {
			return AresAction::None;
		}
	}

	 //drivers can drive, but only stuff owned by neutrals. if a driver is a vehicle thief
	 //also, it can reclaim units even if they are immune to hijacking (see below)
	const auto specialOwned = pTarget->Owner->Type->MultiplayPassive;
	if(specialOwned && pTypeExt->CanDrive) {
		return AresAction::Drive;
	}

	// hijacking only affects enemies
	if(pType->VehicleThief) {
		// can't steal allied unit (CanDrive and special already handled)
		if(pThis->Owner->IsAlliedWith(pTarget->Owner) || specialOwned) {
			return AresAction::None;
		}

		const auto pTargetTypeExt = TechnoTypeExt::ExtMap.Find(pTargetType);
		if(!pTargetTypeExt->HijackerAllowed) {
			return AresAction::None;
		}

		// allowed to steal from enemy
		return AresAction::Hijack;
	}

	// no hijacking ability
	return AresAction::None;
}

// perform the most appropriate hijack action
bool TechnoExt::ExtData::PerformActionHijack(TechnoClass* const pTarget) {
	// was the hijacker lost in the process?
	bool ret = false;

	if(const auto pThis = abstract_cast<InfantryClass*>(this->OwnerObject())) {
		const auto pType = pThis->Type;
		const auto pExt = TechnoExt::ExtMap.Find(pThis);
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		const auto action = pExt->GetActionHijack(pTarget);

		// abort capturing this thing, it looked
		// better from over there...
		if(action == AresAction::None) {
			pThis->SetDestination(nullptr, true);
			const auto& crd = pTarget->GetCoords();
			pThis->Scatter(crd, true, false);
			return false;
		}

		// prepare for a smooth transition. free the destination from
		// any mind control. #762
		if(pTarget->MindControlledBy) {
			pTarget->MindControlledBy->CaptureManager->FreeUnit(pTarget);
		}
		pTarget->MindControlledByAUnit = false;
		if(pTarget->MindControlRingAnim) {
			pTarget->MindControlRingAnim->UnInit();
			pTarget->MindControlRingAnim = nullptr;
		}

		bool asPassenger = false;
		if(action == AresAction::Drive) {
			const auto pDestTypeExt = TechnoTypeExt::ExtMap.Find(pTarget->GetTechnoType());
			if(!pDestTypeExt->Operator.empty() || pDestTypeExt->IsAPromiscuousWhoreAndLetsAnyoneRideIt) {
				asPassenger = true;
			}
		}

		if(!asPassenger) {
			// raise some events in case the hijacker/driver will be
			// swallowed by the vehicle.
			if(pTarget->AttachedTag) {
				pTarget->AttachedTag->RaiseEvent(TriggerEvent::DestroyedByAnything,
					pThis, CellStruct::Empty, false, nullptr);
			}
			pTarget->Owner->HasBeenThieved = true;
			if(auto const pTag = pThis->AttachedTag) {
				if(pTag->ShouldReplace()) {
					pTarget->ReplaceTag(pTag);
				}
			}
		} else {
			// raise some events in case the driver enters
			// a vehicle that needs an Operator
			if(pTarget->AttachedTag) {
				pTarget->AttachedTag->RaiseEvent(TriggerEvent::EnteredBy,
					pThis, CellStruct::Empty, false, nullptr);
			}
		}

		// if the hijacker is mind-controlled, free it,
		// too, and attach to the new target. #762
		const auto controller = pThis->MindControlledBy;
		if(controller) {
			++Unsorted::ScenarioInit;
			controller->CaptureManager->FreeUnit(pThis);
			--Unsorted::ScenarioInit;
		}

		// let's make a steal
		pTarget->SetOwningHouse(pThis->Owner, true);
		pTarget->GotHijacked();
		VocClass::PlayAt(pTypeExt->HijackerEnterSound, pTarget->Location, nullptr);

		// remove the driverless-marker
		const auto pDestExt = TechnoExt::ExtMap.Find(pTarget);
		pDestExt->DriverKilled = false;

		// save the hijacker's properties
		if(action == AresAction::Hijack) {
			pTarget->HijackerInfantryType = pType->ArrayIndex;
			pDestExt->HijackerHouse = pThis->Owner;
			pDestExt->HijackerHealth = pThis->Health;
			pDestExt->HijackerVeterancy = pThis->Veterancy.Veterancy;
		}

		// hook up the original mind-controller with the target #762
		if(controller) {
			++Unsorted::ScenarioInit;
			controller->CaptureManager->CaptureUnit(pTarget);
			--Unsorted::ScenarioInit;
		}

		// reboot the slave manager
		if(pTarget->SlaveManager) {
			pTarget->SlaveManager->ResumeWork();
		}

		// the hijacker enters and closes the door.
		ret = true;

		// only for the drive action: if the target requires an operator,
		// we add the driver to the passengers list instead of deleting it.
		// this does not check passenger count or size limits.
		if(asPassenger) {
			pTarget->AddPassenger(pThis);
			pThis->AbortMotion();
			ret = false;
		}

		pTarget->QueueMission(Mission::Guard, true);
	}

	return ret;
}

// looks for the nearest object this one could hijack or drive and heads for it
bool TechnoExt::ExtData::PerformHijackOnArea()
{
	auto const pThis = abstract_cast<InfantryClass*>(this->OwnerObject());
	if(!pThis) {
		return false;
	}

	auto const pType = pThis->Type;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	if(!pType->VehicleThief && !pTypeExt->CanDrive) {
		return false;
	}

	TechnoClass* pTarget = nullptr;
	auto best = std::numeric_limits<double>::max();
	auto bestID = -1;

	for(auto const pFoot : FootClass::Array) {
		auto const delta = pThis->Location - pFoot->Location;
		auto const distance = static_cast<double>(delta.X) * delta.X
			+ static_cast<double>(delta.Y) * delta.Y
			+ static_cast<double>(delta.Z) * delta.Z;

		auto const closer = distance < best
			|| (distance == best && pFoot->UniqueID < bestID);

		if(closer && this->GetActionHijack(pFoot) != AresAction::None) {
			pTarget = pFoot;
			best = distance;
			bestID = pFoot->UniqueID;
		}
	}

	auto const found = pTarget != nullptr;
	pThis->ShouldGarrisonStructure = found;
	this->TakeVehicleMode = found;

	if(found && (pThis->Target != pTarget || pThis->CurrentMission != Mission::Enter)) {
		pThis->SetDestination(pTarget, true);
		pThis->QueueMission(Mission::Enter, true);
	}

	return found;
}

// whether this object's driver can be shot out of it at all, and whether it
// is hurt badly enough for that to happen now
bool TechnoExt::ExtData::IsDriverKillable(double const belowPercent) const
{
	auto const pThis = this->OwnerObject();

	// target must be Vehicle or Aircraft
	auto const abs = pThis->WhatAmI();
	if(abs != AbstractType::Unit && abs != AbstractType::Aircraft) {
		return false;
	}

	// if this aircraft is expected to dock to anything, don't allow killing its pilot
	// (reason being: the game thinks you lost the aircraft that just turned, and assumes you have free aircraft space,
	// allowing you to build more aircraft, for the docking spot that is still occupied by the previous plane.)
	if(abs == AbstractType::Aircraft) {
		auto const pAircraftType = static_cast<AircraftClass*>(pThis)->Type;
		if(pAircraftType->AirportBound || pAircraftType->Dock.Count) {
			return false;
		}
	}

	// don't penetrate the Iron Curtain // typedef IronCurtain ChastityBelt
	if(pThis->BeingWarpedOut || pThis->IsIronCurtained()) {
		return false;
	}

	// a vehicle that has not left the factory that built it yet
	if(TechnoExt::IsInWarfactory(pThis, true)) {
		return false;
	}

	// not a living being
	auto const pType = pThis->GetTechnoType();
	if(pType->Natural || pType->Organic) {
		return false;
	}

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	// the promotion ability protects the driver outright
	if(pTypeExt->HasAbility(AresAbility::ProtectedDriver, pThis->Veterancy)) {
		return false;
	}

	// because these tags kinda have negative meaning, less means better.
	// if the driver is protected, he can by default only be killed if
	// health is below 0.0, while 1.0 means always killable.
	auto const maxKillHealth = Math::min(
		pTypeExt->ProtectedDriver_MinHealth.Get(
			pTypeExt->ProtectedDriver ? 0.0 : 1.0),
		belowPercent);

	return pThis->GetHealthPercentage() <= maxKillHealth;
}

// hands this object over to another house without its crew
bool TechnoExt::ExtData::ApplyKillDriver(
	HouseClass* const pNewOwner, TechnoClass* const pKiller,
	bool const removeVeterancy)
{
	auto const pTarget = abstract_cast<FootClass*>(this->OwnerObject());
	if(!pTarget) {
		return false;
	}

	auto const passive = pNewOwner->Type->MultiplayPassive;
	this->DriverKilled = passive;

	// exit if owner would not change
	if(pTarget->Owner == pNewOwner) {
		return false;
	}

	auto const pTargetTypeExt = TechnoTypeExt::ExtMap.Find(pTarget->GetTechnoType());

	if(pTarget->Passengers.GetFirstPassenger()) {
		// If this vehicle uses Operator=, we have to take care of actual "physical" drivers, rather than theoretical ones
		if(pTargetTypeExt->IsAPromiscuousWhoreAndLetsAnyoneRideIt) {
			// kill first passenger
			auto const pPassenger = pTarget->RemoveFirstPassenger();
			pPassenger->RegisterDestruction(pKiller);
			pPassenger->UnInit();

		} else if(!pTargetTypeExt->Operator.empty()) {
			// find the driver cowardly hiding among the passengers, then kill him
			for(NextObject passenger(pTarget->Passengers.GetFirstPassenger()); passenger; ++passenger) {
				auto const pPassenger = static_cast<FootClass*>(*passenger);
				auto const pType = pPassenger->GetTechnoType();
				auto const isOperator = [pType](InfantryTypeClass* const pOperatorType) {
					return pOperatorType == pType;
				};

				if(std::any_of(pTargetTypeExt->Operator.begin(), pTargetTypeExt->Operator.end(), isOperator)) {
					pTarget->RemovePassenger(pPassenger);
					pPassenger->RegisterDestruction(pKiller);
					pPassenger->UnInit();
					break;
				}
			}
		}

		// if passengers remain in the vehicle, operator-using or not, they should leave
		if(pTarget->Passengers.GetFirstPassenger()) {
			TechnoExt::EjectPassengers(pTarget, -1);
		}
	}

	// remove the hijacker
	pTarget->HijackerInfantryType = -1;

	if(removeVeterancy) {
		pTarget->Veterancy.Veterancy = 0.0f;
	}

	// If this unit is driving under influence, we have to free it first
	if(auto const pController = pTarget->MindControlledBy) {
		if(auto const pCaptureManager = pController->CaptureManager) {
			pCaptureManager->FreeUnit(pTarget);
		}
	}
	pTarget->MindControlledByAUnit = false;
	pTarget->MindControlledByHouse = nullptr;

	// remove the mind-control ring anim
	if(pTarget->MindControlRingAnim) {
		pTarget->MindControlRingAnim->UnInit();
		pTarget->MindControlRingAnim = nullptr;
	}

	// If this unit mind controls stuff, we should free the controllees, since they still belong to the previous owner
	if(pTarget->CaptureManager) {
		pTarget->CaptureManager->FreeAll();
	}

	// This unit will be freed of its duties
	if(pTarget->BelongsToATeam()) {
		pTarget->Team->LiberateMember(pTarget);
	}

	// If this unit spawns stuff, we should kill the spawns, since they still belong to the previous owner
	if(auto const pSpawnManager = pTarget->SpawnManager) {
		pSpawnManager->KillNodes();
		pSpawnManager->ResetTarget();
	}

	// If this unit enslaves stuff, we should free the slaves, since they still belong to the previous owner
	// <DCoder> SlaveManagerClass::Killed() sets the manager's Owner to NULL
	// <Renegade> okay, does Killed() also destroy the slave manager, or just unlink it from the unit?
	// <DCoder> unlink
	// <Renegade> so on principle, I could just re-link it?
	// <DCoder> yes you can
	if(auto const pSlaveManager = pTarget->SlaveManager) {
		pSlaveManager->Killed(pKiller);
		pSlaveManager->ZeroOutSlaves();
		pSlaveManager->Owner = pTarget;
		if(passive) {
			pSlaveManager->SuspendWork();
		} else {
			pSlaveManager->ResumeWork();
		}
	}

	// Hand over to a different house
	pTarget->SetOwningHouse(pNewOwner, true);

	if(passive) {
		pTarget->QueueMission(Mission::Harmless, true);
	}

	pTarget->SetTarget(nullptr);
	pTarget->SetDestination(nullptr, false);

	if(auto const pTag = pTarget->AttachedTag) {
		pTag->RaiseEvent(AresTriggerEvent::DriverKilled_ByHouse,
			pTarget, CellStruct::Empty, false, pKiller);
	}

	if(pTarget->IsAlive) {
		if(auto const pTag = pTarget->AttachedTag) {
			pTag->RaiseEvent(AresTriggerEvent::DriverKiller,
				pTarget, CellStruct::Empty, false, nullptr);
		}
	}

	return true;
}

// Processes an amount of tiberium of a specific type.
/*!
	This function calculates the value of tiberium and also
	considers ore purifiers.

	\param amount The amount of tiberium.
	\param idxType The type of tiberium.

	\author AlexB
	\date 2012-10-10
*/
void TechnoExt::ExtData::RefineTiberium(float const amount, int const idxType) {
	auto const pThis = this->OwnerObject();
	auto const pHouse = pThis->GetOwningHouse();

	// get the number of applicable purifiers
	auto purifiers = pHouse->NumOrePurifiers;
	if(!pHouse->IsHumanPlayer && SessionClass::Instance.GameMode != GameMode::Campaign) {
		purifiers += RulesClass::Instance->AIVirtualPurifiers.GetItem(pHouse->GetAIDifficultyIndex());
	}

	// bonus amount (in tiberium)
	auto const purified = purifiers * RulesClass::Instance->PurifierBonus * amount;

	// add the tiberium to the house's credits
	DepositTiberium(amount, purified, idxType);
}

// Adds the value of an amount of tiberium and its bonus amount to the house's credits.
/*!
	Stores the tiberium's value on the houses's accounts.

	\param amount The amount of raw tiberium.
	\param bonus The bonus tiberium amount.
	\param idxType The type of tiberium.

	\author AlexB
	\date 2012-10-10
*/
void TechnoExt::ExtData::DepositTiberium(float const amount, float const bonus, int const idxType) {
	auto const pThis = this->OwnerObject();
	auto const pHouse = pThis->GetOwningHouse();
	auto const pTiberium = TiberiumClass::Array.GetItem(idxType);
	auto value = 0;

	// always put the purified money on the bank account. otherwise ore purifiers
	// would fill up storage with tiberium that doesn't exist. this is consistent with
	// the original YR, because old GiveTiberium put it on the bank anyhow, despite its name.
	if(bonus > 0.0) {
		value += Game::F2I(bonus * pTiberium->Value * pHouse->Type->IncomeMult);
	}

	// also add the normal tiberium to the global account?
	if(amount > 0.0) {
		auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
		if(!pExt->Refinery_UseStorage) {
			value += Game::F2I(amount * pTiberium->Value * pHouse->Type->IncomeMult);
		} else {
			pHouse->GiveTiberium(amount, idxType);
		}
	}

	// deposit
	if(value > 0) {
		pHouse->GiveMoney(value);
	}
}

/*! Gets whether the techno has the ability to cloak itself or is cloaked by others.

	The techno may have the ability to cloak itself, gained the cloaking ability through
	veterancy or it may be under the influence of Cloak Generators.

	\param allowPassive Allow the techno to be cloaked by others.

	\return True, if the techno can cloak, false otherwise.

	\author AlexB
	\date 2012-09-28
*/
bool TechnoExt::ExtData::IsCloakable(bool allowPassive) const
{
	TechnoClass* pThis = this->OwnerObject();
	TechnoTypeClass* pType = pThis->GetTechnoType();
	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	// object disallowed from cloaking
	if(!pTypeExt->CloakAllowed) {
		return false;
	}

	// parachuted units cannot cloak. this makes paradropping
	// units uncloakable like they were in the vanilla game
	if(pThis->Parachute) {
		return false;
	}

	// check for active cloak
	if(pThis->IsCloakable() || pThis->HasAbility(Ability::Cloak)) {
		if(this->CanSelfCloakNow()) {
			return true;
		}
	}

	// if not actively cloakable
	if(allowPassive) {
		// cloak generators ignore everything above ground. this
		// fixes hover units not being affected by cloak.
		if(pThis->GetHeight() > RulesExt::Global()->CloakHeight.Get(RulesClass::Instance->HoverHeight)) {
			return false;
		}

		// search for cloak generators
		CoordStruct crd = pThis->GetCoords();
		CellClass* pCell = MapClass::Instance.GetCellAt(crd);
		return pCell->CloakGen_InclHouse(pThis->Owner->ArrayIndex);
	}

	return false;
}

/*! Gets whether the techno is allowed to cloak.

	Checks all circumstances that might conflict with the unit cloaking.

	\return True, if the techno is allowed to cloak, false otherwise.

	\author AlexB
	\date 2012-09-28
*/
bool TechnoExt::ExtData::CloakAllowed() const
{
	if(this->CloakDisallowed(true)) {
		return false;
	}

	TechnoClass* pThis = this->OwnerObject();

	if(pThis->CloakState == CloakState::Cloaked) {
		return false;
	}

	if(pThis->RearmTimer.InProgress()) {
		return false;
	}

	if(pThis->Target && pThis->IsCloseEnoughToAttack(pThis->Target)) {
		return false;
	}

	if(pThis->WhatAmI() != BuildingClass::AbsID && pThis->CloakProgress.Value) {
		return false;
	}

	if(pThis->CloakDelayTimer.InProgress()) {
		return false;
	}

	if(pThis->LocomotorSource) {
		if(auto pFoot = abstract_cast<FootClass*>(pThis)) {
			if(pFoot->IsAttackedByLocomotor) {
				return false;
			}
		}
	}

	return true;
}

/*! Gets whether the techno is disallowed to cloak.

	Certain features uncloak the techno. If a techno is cloaked and this returns true,
	it should be revealed, because something keeps it from maintaining the cloak.

	\param allowPassive Allow the techno to be cloaked by others.

	\return True, if the techno is disallowed to stay cloaked, false otherwise.

	\author AlexB
	\date 2012-09-28
*/
bool TechnoExt::ExtData::CloakDisallowed(bool allowPassive) const
{
	if(this->IsCloakable(allowPassive)) {
		TechnoClass* pThis = this->OwnerObject();
		return pThis->IsUnderEMP() || pThis->IsParalyzed()
			|| pThis->IsBeingWarpedOut() || pThis->IsWarpingIn()
			|| this->CloakSkipTimer.InProgress();
	}

	return true;
}

/*! Gets whether the techno is allowed to cloak, only accounting for features Ares adds.

	Edit this function to add new features that may prevent units from cloaking.

	\return True, if the techno is allowed to cloak, false otherwise.

	\author AlexB
	\date 2012-09-28
*/
bool TechnoExt::ExtData::CanSelfCloakNow() const
{
	auto pThis = this->OwnerObject();

	// cloaked and deactivated units are hard to find otherwise
	if(this->DriverKilled || pThis->Deactivated) {
		return false;
	}

	auto pType = pThis->GetTechnoType();
	auto pExt = TechnoTypeExt::ExtMap.Find(pType);

	if(specific_cast<BuildingTypeClass*>(pType)) {
		if(pExt->CloakPowered && !pThis->IsPowerOnline()) {
			return false;
		}
	}

	if(auto pInf = specific_cast<InfantryClass*>(pThis)) {
		if(pExt->CloakDeployed && !pInf->IsDeployed()) {
			return false;
		}
	}

	// allows cloak
	return true;
}

void TechnoExt::ExtData::SetSpotlight(BuildingLightClass* pSpotlight) {
	if(this->Spotlight != pSpotlight) {
		if(this->Spotlight) {
			GameDelete(this->Spotlight);
		}
		this->Spotlight = pSpotlight;
	}

	if(auto pBld = abstract_cast<BuildingClass*>(this->OwnerObject())) {
		if(pBld->Spotlight != pSpotlight) {
			if(pBld->Spotlight) {
				GameDelete(pBld->Spotlight);
			}
			pBld->Spotlight = pSpotlight;
		}
	}
}

// pays the killer for destroying this object, if its type collects bounties
void TechnoExt::ExtData::CalculateBounty(TechnoClass* const pAttacker)
{
	auto const pThis = this->OwnerObject();
	auto const pKillerHouse = pAttacker->Owner;
	auto const pVictimHouse = pThis->Owner;

	// no reward for killing your own and your friends'
	if(pKillerHouse) {
		if(pKillerHouse == pVictimHouse) {
			return;
		}

		auto const index = pKillerHouse->ArrayIndex;
		if(index == pVictimHouse->ArrayIndex) {
			return;
		}

		if(index >= 0 && pVictimHouse->Allies.Contains(index)) {
			return;
		}
	}

	if(!HouseTypeExt::ExtMap.Find(pVictimHouse->Type)->GivesBounty) {
		return;
	}

	// an empty list means no requirement at all
	auto const& enablers = RulesExt::Global()->BountyEnablers;
	if(!enablers.empty()) {
		auto const owns = [pKillerHouse](BuildingTypeClass* const pType) {
			return pKillerHouse->CountOwnedAndPresent(pType) > 0;
		};

		if(!std::any_of(enablers.begin(), enablers.end(), owns)) {
			return;
		}
	}

	auto const pVictimTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	auto const bounty = pVictimTypeExt->Bounty_Value.Get(pThis);
	if(!bounty) {
		return;
	}

	auto const pKillerType = pAttacker->GetTechnoType();
	auto const pKillerTypeExt = TechnoTypeExt::ExtMap.Find(pKillerType);

	pKillerHouse->TransactMoney(bounty);

	if(pKillerTypeExt->Bounty_Display.Get(RulesExt::Global()->BountyDisplay)) {
		// a missile is not around long enough to show anything, so its
		// carrier displays the amount instead
		auto pRecipient = pAttacker;
		if(pKillerType->MissileSpawn && pAttacker->SpawnOwner) {
			pRecipient = pAttacker->SpawnOwner;
		}

		TechnoExt::ExtMap.Find(pRecipient)->TechnoValue += bounty;
	}
}

// turns the accumulated credits into a flying string over this object
void TechnoExt::ExtData::DisplayValue(bool const force)
{
	auto const value = this->TechnoValue;
	if(!value) {
		return;
	}

	if(!force && Unsorted::CurrentFrame < this->TechnoValue_NextDisplayFrame) {
		return;
	}

	auto const delay = static_cast<int>(RulesExt::Global()->DisplayCreditsDelay * -900.0);
	this->TechnoValue = 0;
	this->TechnoValue_NextDisplayFrame = Unsorted::CurrentFrame - delay;

	wchar_t text[32];
	swprintf_s(text, L"%c$%d", value <= 0 ? L'-' : L'+', std::abs(value));

	ColorStruct const red { 0xFF, 0x00, 0x00 };
	ColorStruct const green { 0x00, 0xFF, 0x00 };
	auto const color = static_cast<WORD>(Drawing::RGB_To_Int(value > 0 ? green : red));

	auto const pThis = this->OwnerObject();
	auto coords = pThis->GetCoords();

	if(MapClass::Instance.IsLocationShrouded(coords)) {
		return;
	}

	if(pThis->VisualCharacter(VARIANT_FALSE, HouseClass::CurrentPlayer) == VisualType::Hidden) {
		return;
	}

	if(auto const pBuilding = specific_cast<BuildingClass*>(pThis)) {
		coords.Z += 104 * pBuilding->Type->Height;
	} else {
		coords.Z += 256;
	}

	FlyingStrings::Add(text, coords, color, 5);
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->idxSlot_Wave)
		.Process(this->idxSlot_Beam)
		.Process(this->idxSlot_Warp)
		.Process(this->idxSlot_Parasite)
		.Process(this->GarrisonedIn)
		.Process(this->EMPSparkleAnim)
		.Process(this->EMPLastMission)
		.Process(this->RadarJam)
		.Process(this->PoweredUnit)
		.Process(this->AttachedEffects)
		.Process(this->AttachedTechnoEffect_Delay)
		.Process(this->AttachEffects_RecreateAnims)
		.Process(this->AttachedTechnoEffect_isset)
		.Process(this->MyOriginalTemporal)
		.Process(this->Spotlight)
		.Process(this->MyBolt)
		.Process(this->OriginalHouseType)
		.Process(this->CloakSkipTimer)
		.Process(this->DisableWeaponTimer)
		.Process(this->SelfHealCombatTimer)
		.Process(this->HijackerHealth)
		.Process(this->HijackerHouse)
		.Process(this->HijackerVeterancy)
		.Process(this->AttachEffects_ROFMultiplier)
		.Process(this->Crate_FirepowerMultiplier)
		.Process(this->Crate_ArmorMultiplier)
		.Process(this->Crate_SpeedMultiplier)
		.Process(this->Crate_Cloakable)
		.Process(this->AltOccupation)
		.Process(this->Survivors_Done)
		.Process(this->DriverKilled)
		.Process(this->AlwaysOperated)
		.Process(this->PayloadCreated)
		.Process(this->SuppressLossMessage)
		.Process(this->SuperWeapon)
		.Process(this->SuperTarget)
		.Process(this->TechnoValue)
		.Process(this->TechnoValue_NextDisplayFrame)
		.Process(this->TakeVehicleMode);
}

void TechnoExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<TechnoClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<TechnoClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// TechnoExt::Alpha is deliberately not streamed: it is rebuilt from the objects it
// tracks, and is cleared wholesale rather than saved.
bool TechnoExt::LoadGlobals(AresStreamReader& Stm) {
	return Stm
		.Process(ActiveBuildingLight)
		.Process(NeedsRegap)
		.Process(DrawnShadowManually)
		.Success();
}

bool TechnoExt::SaveGlobals(AresStreamWriter& Stm) {
	return Stm
		.Process(ActiveBuildingLight)
		.Process(NeedsRegap)
		.Process(DrawnShadowManually)
		.Success();
}

// =============================
// container

TechnoExt::ExtContainer::ExtContainer() : Container("TechnoClass") {
}

TechnoExt::ExtContainer::~ExtContainer() = default;

void TechnoExt::ExtContainer::InvalidatePointer(void* ptr, bool bRemoved) {
	AnnounceInvalidPointer(TechnoExt::ActiveBuildingLight, ptr);
}

// =============================
// container hooks

DEFINE_HOOK(0x6F3260, TechnoClass_CTOR, 0x5)
{
	GET(TechnoClass*, pItem, ESI);

	TechnoExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
{
	GET(TechnoClass*, pItem, ECX);

	//TechnoExt::ExtData *pItemExt = TechnoExt::ExtMap.Find(pItem);
	TechnoExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK(0x710415, TechnoClass_AnimPointerExpired, 0x6)
{
	GET(TechnoClass*, pThis, ECX);
	GET(AnimClass*, pAnim, EAX);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	AnnounceInvalidPointer(pExt->GarrisonedIn, pAnim);
	AnnounceInvalidPointer(pExt->EMPSparkleAnim, pAnim);
	pExt->InvalidateAttachEffectPointer(pAnim);
	AnnounceInvalidPointer(pExt->Spotlight, pAnim);

	return 0;
}

DEFINE_HOOK_AGAIN(0x70C250, TechnoClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x70BF50, TechnoClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TechnoClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x70C249, TechnoClass_Load_Suffix, 0x5)
{
	TechnoExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x70C264, TechnoClass_Save_Suffix, 0x5)
{
	TechnoExt::ExtMap.SaveStatic();
	return 0;
}

static_assert(sizeof(TechnoExt::ExtData) == 0xC0, "TechnoExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the attached effect block, the timers, the
// doubles, the bool cluster and the last member.
static_assert(offsetof(TechnoExt::ExtData, idxSlot_Wave) == 0x008, "TechnoExt::ExtData layout slipped");
static_assert(offsetof(TechnoExt::ExtData, AttachedTechnoEffect_Delay) == 0x02C, "TechnoExt::ExtData layout slipped");
static_assert(offsetof(TechnoExt::ExtData, CloakSkipTimer) == 0x044, "TechnoExt::ExtData layout slipped");
static_assert(offsetof(TechnoExt::ExtData, AttachEffects_ROFMultiplier) == 0x078, "TechnoExt::ExtData layout slipped");
static_assert(offsetof(TechnoExt::ExtData, AltOccupation) == 0x099, "TechnoExt::ExtData layout slipped");
static_assert(offsetof(TechnoExt::ExtData, TakeVehicleMode) == 0x0B0, "TechnoExt::ExtData layout slipped");
