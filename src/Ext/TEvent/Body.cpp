#include "Body.h"
#include "../../Utilities/AresEnums.h"

#include "../House/Body.h"

#include "../../Misc/SavegameDef.h"

#include <AircraftClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <ScenarioClass.h>
#include <SuperClass.h>
#include <TechnoTypeClass.h>
#include <UnitClass.h>

#include <cmath>

//Static init
TEventExt::ExtContainer TEventExt::ExtMap;

namespace {
	// the metadata tables are indexed by the event kind, offset by the first
	// event Ares added.
	constexpr auto FirstAresEvent = AresTriggerEvent::UnderEMP;
	constexpr auto AresEventCount = 27u;

	bool GetAresEventIndex(TriggerEvent eventKind, unsigned int* index) {
		auto const value = static_cast<unsigned int>(eventKind)
			- static_cast<unsigned int>(FirstAresEvent);

		if(value >= AresEventCount) {
			return false;
		}

		*index = value;
		return true;
	}
}

// Gets the TechnoType pointed to by the event's TechnoName field.
/*!
	Resolves the TechnoName to a TechnoTypeClass and caches it. This function
	is an O(n) operation for the first call, every subsequent call is O(1).

	\returns The TechnoTypeClass TechnoName points to, nullptr if not set or invalid.

	\date 2012-05-09, 2013-02-09
*/
TechnoTypeClass* TEventExt::ExtData::GetTechnoType()
{
	if(this->TechnoType.empty()) {
		const char* eventTechno = this->OwnerObject()->String;
		TechnoTypeClass* pType = TechnoTypeClass::Find(eventTechno);

		if(!pType) {
			Debug::Log(Debug::Severity::Error, "Event references non-existing techno type \"%s\".", eventTechno);
			Debug::RegisterParserError();
		}

		this->TechnoType = pType;
	}

	return this->TechnoType;
}

// Gets whether the referenced TechnoType exists at least 'count' times.
/*!
	\param count The number of instances that have to exist.
	\param pOwner If set, only objects owned by this house are counted.

	\remark Returns false if the type cannot be resolved.

	\returns True if TechnoType exists at least count times, false otherwise.

	\date 2012-05-09, last updated 2022-02-04
*/
bool TEventExt::ExtData::TechTypeExists(int count, HouseClass* pOwner)
{
	auto const pType = this->GetTechnoType();

	if(!pType) {
		return false;
	}

	if(count <= 0) {
		return true;
	}

	if(!pType->Insignificant && !pType->DontScore) {
		// decreases count by the number of owned techno types. iff count is zero or less,
		// this techno type exists at least 'count' times.
		if(pOwner) {
			return (count - pOwner->CountOwnedNow(pType)) <= 0;
		}

		for(auto const pHouse : HouseClass::Array) {
			count -= pHouse->CountOwnedNow(pType);

			if(count <= 0) {
				return true;
			}
		}

		return false;
	}

	// the game doesn't keep track of this type, so scan the array it lives in.
	auto const countMatches = [count, pOwner, pType](auto const& array) {
		auto matches = 0;

		for(auto const pItem : array) {
			if((!pOwner || pOwner == pItem->Owner) && pItem->Type == pType) {
				if(++matches >= count) {
					return true;
				}
			}
		}

		return matches >= count;
	};

	switch(pType->WhatAmI()) {
	case AbstractType::AircraftType:
		return countMatches(AircraftClass::Array);

	case AbstractType::BuildingType:
		return countMatches(BuildingClass::Array);

	case AbstractType::InfantryType:
		return countMatches(InfantryClass::Array);

	default:
		return countMatches(UnitClass::Array);
	}
}

// Handles the occurence of events.
/*!
	Override any checks for whether an event occured or not. Set ret to true
	if the event occured, to false otherwise.

	\returns True if this event was handled by Ares, false otherwise.

	\date 2012-05-09, last updated 2022-02-04
*/
bool TEventExt::HasOccured(
	TEventClass* pEvent, TriggerEvent const eventType, HouseClass* const pOwner,
	ObjectClass* const pAttachedTo, void* const pSource, bool* ret)
{
	if(pEvent->EventKind < TriggerEvent::TechTypeExists) {
		return false;
	}

	auto const pExt = ExtMap.Find(pEvent);

	// the object the tag is attached to has to be a techno.
	auto const pAttachedTechno = (pAttachedTo
		&& (pAttachedTo->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None)
		? static_cast<TechnoClass*>(pAttachedTo) : nullptr;

	// this event only fires for the event kind that has been sprung.
	auto const isSprung = (pEvent->EventKind == eventType);

	// the house of the object that sprung the event has to match the argument.
	auto const isSourceHouse = [pEvent, pSource]() {
		auto const pTechno = static_cast<TechnoClass*>(pSource);
		return pTechno && pTechno->Owner->ArrayIndex == pEvent->Value;
	};

	*ret = false;

	switch(pEvent->EventKind) {
	case AresTriggerEvent::UnderEMP:
	case AresTriggerEvent::UnderEMP_ByHouse:
	case AresTriggerEvent::RemoveEMP:
	case AresTriggerEvent::RemoveEMP_ByHouse:
		if(pAttachedTechno && isSprung) {
			auto const locked = static_cast<int>(pAttachedTechno->EMPLockRemaining);

			switch(eventType) {
			case AresTriggerEvent::UnderEMP:
				*ret = (locked > 0);
				break;

			case AresTriggerEvent::UnderEMP_ByHouse:
				*ret = isSourceHouse() && (locked > 0);
				break;

			case AresTriggerEvent::RemoveEMP:
				*ret = (locked <= 0);
				break;

			case AresTriggerEvent::RemoveEMP_ByHouse:
				*ret = isSourceHouse() && (locked <= 0);
				break;

			default:
				break;
			}
		}
		break;

	case AresTriggerEvent::EnemyInSpotlightNow:
		*ret = true;
		break;

	case AresTriggerEvent::DriverKiller:
	case AresTriggerEvent::DriverKilled_ByHouse:
		*ret = pAttachedTechno && isSprung
			&& (eventType == AresTriggerEvent::DriverKiller || isSourceHouse());
		break;

	case AresTriggerEvent::VehicleTaken:
	case AresTriggerEvent::VehicleTaken_ByHouse:
		*ret = pAttachedTechno && isSprung
			&& (eventType == AresTriggerEvent::VehicleTaken || isSourceHouse());
		break;

	case AresTriggerEvent::Abducted:
	case AresTriggerEvent::Abducted_ByHouse:
	case AresTriggerEvent::AbductSomething:
	case AresTriggerEvent::AbductSomething_OfHouse:
		if(pAttachedTechno && isSprung) {
			switch(eventType) {
			case AresTriggerEvent::Abducted:
			case AresTriggerEvent::AbductSomething:
				*ret = true;
				break;

			case AresTriggerEvent::Abducted_ByHouse: {
				auto const pAbstract = static_cast<AbstractClass*>(pSource);
				*ret = pAbstract
					&& (pAbstract->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None
					&& isSourceHouse();
				break;
			}

			case AresTriggerEvent::AbductSomething_OfHouse: {
				auto const pHouse = static_cast<HouseClass*>(pSource);
				*ret = pHouse && pHouse->WhatAmI() == HouseClass::AbsID
					&& pHouse->ArrayIndex == pEvent->Value;
				break;
			}

			default:
				break;
			}
		}
		break;

	case AresTriggerEvent::SuperActivated:
	case AresTriggerEvent::SuperDeactivated: {
		auto const pSuper = static_cast<SuperClass*>(pSource);
		*ret = isSprung && pSuper && pSuper->WhatAmI() == SuperClass::AbsID
			&& pSuper->Type->ArrayIndex == pEvent->Value;
		break;
	}

	case AresTriggerEvent::SuperNearWaypoint: {
		auto const pTarget = static_cast<TEventExt::SuperTarget*>(pSource);

		if(isSprung
			&& !_strcmpi(pTarget->Super->Type->ID, pEvent->String))
		{
			auto const waypoint = ScenarioClass::Instance->GetWaypointCoords(pEvent->Value);
			auto const dX = pTarget->Cell.X - waypoint.X;
			auto const dY = pTarget->Cell.Y - waypoint.Y;

			*ret = (std::sqrt(static_cast<double>(dX * dX + dY * dY)) <= 5.0);
		}
		break;
	}

	case AresTriggerEvent::ReverseEngineered: {
		auto const pHouseExt = HouseExt::ExtMap.Find(pOwner);
		*ret = pHouseExt->ReverseEngineered.contains(pExt->GetTechnoType());
		break;
	}

	case AresTriggerEvent::ReverseEngineerAnything:
	case AresTriggerEvent::ReverseEngineerType:
		if(isSprung) {
			if(eventType == AresTriggerEvent::ReverseEngineerAnything) {
				*ret = true;
			} else {
				auto const pTechno = static_cast<TechnoClass*>(pSource);
				*ret = (pTechno->GetTechnoType() == pExt->GetTechnoType());
			}
		}
		break;

	case AresTriggerEvent::HouseOwnTechnoType:
		*ret = pExt->TechTypeExists(pEvent->Value, pOwner);
		break;

	case AresTriggerEvent::HouseDoesntOwnTechnoType:
		*ret = !pExt->TechTypeExists(pEvent->Value + 1, pOwner);
		break;

	case AresTriggerEvent::AttackedOrDestroyedByAnybody:
	case AresTriggerEvent::AttackedOrDestroyedByHouse:
		*ret = isSprung
			&& (eventType == AresTriggerEvent::AttackedOrDestroyedByAnybody || isSourceHouse());
		break;

	case AresTriggerEvent::DestroyedByHouse: {
		auto const pHouse = static_cast<HouseClass*>(pSource);
		*ret = isSprung && pHouse && pHouse->ArrayIndex == pEvent->Value;
		break;
	}

	case AresTriggerEvent::TechnoTypeDoesntExistMoreThan:
		*ret = !pExt->TechTypeExists(pEvent->Value + 1, nullptr);
		break;

	case AresTriggerEvent::AllKeepAlivesDestroyed:
	case AresTriggerEvent::AllKeepAlivesBuildingDestroyed: {
		auto const pHouseExt = HouseExt::ExtMap.Find(ResolveHouseParam(pEvent->Value));

		*ret = (pEvent->EventKind == AresTriggerEvent::AllKeepAlivesDestroyed)
			? (pHouseExt->KeepAliveCount <= 0)
			: (pHouseExt->KeepAliveBuildingsCount <= 0);
		break;
	}

	case TriggerEvent::TechTypeExists:
		*ret = pExt->TechTypeExists(pEvent->Value, nullptr);
		break;

	case TriggerEvent::TechTypeDoesntExist:
		*ret = !pExt->TechTypeExists(1, nullptr);
		break;

	default:
		return false;
	}

	return true;
}

// Gets what the new events attach to.
bool TEventExt::GetAttachFlags(TriggerEvent const eventKind, int* ret)
{
	static constexpr const int Flags[] = {
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 8, 8, 8, 2, 2, 8, 8, 2, 2,
		8, 16, 8, 8 };

	auto index = 0u;

	if(!GetAresEventIndex(eventKind, &index)) {
		return false;
	}

	*ret = Flags[index];
	return true;
}

// Gets whether the new events are remembered once they occured.
bool TEventExt::GetPersistable(TriggerEvent const eventKind, bool* ret)
{
	static constexpr const bool Persistable[] = {
		false, false, false, false, false, true, true, true, true, true, true,
		true, true, true, true, true, false, true, true, false, false, false,
		false, true, false, true, true };

	auto index = 0u;

	if(!GetAresEventIndex(eventKind, &index)) {
		return false;
	}

	*ret = Persistable[index];
	return true;
}

// Gets how the new events' parameters are written back to INI.
bool TEventExt::GetSaveMode(TriggerEvent const eventKind, int* ret)
{
	static constexpr const int Modes[] = {
		0, 13, 0, 13, 0, 0, 13, 0, 13, 0, 13, 0, 13, 0, 0, 43, 43, 0, 43, 43,
		43, 0, 13, 13, 43, 13, 13 };

	auto index = 0u;

	if(!GetAresEventIndex(eventKind, &index)) {
		return false;
	}

	*ret = Modes[index];
	return true;
}

// Resolves a param to a house.
HouseClass* TEventExt::ResolveHouseParam(int const param, HouseClass* const pOwnerHouse) {
	if(param == 8997) {
		return pOwnerHouse;
	}

	if(HouseClass::Index_IsMP(param)) {
		return HouseClass::FindByIndex(param);
	}

	return HouseClass::FindByCountryIndex(param);
}

// =============================
// load / save

void TEventExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<TEventClass, ExtData>::LoadFromStream(Stm);

	// the resolved type is a cache of TechnoName, so it is dropped rather than
	// carried in the stream. the next GetTechnoType() looks it up again.
	this->TechnoType.clear();
}

void TEventExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<TEventClass, ExtData>::SaveToStream(Stm);
}

// =============================
// container

TEventExt::ExtContainer::ExtContainer() : Container("TEventClass") {
}

TEventExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x71E7F8, TEventClass_CTOR, 0x5)
{
	GET(TEventClass*, pItem, ESI);

	TEventExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x71FAA6, TEventClass_SDDTOR, 0x6)
{
	GET(TEventClass*, pItem, ESI);

	TEventExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x71F930, TEventClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x71F8C0, TEventClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TEventClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TEventExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x71F92B, TEventClass_Load_Suffix, 0x5)
{
	TEventExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x71F94A, TEventClass_Save_Suffix, 0x5)
{
	TEventExt::ExtMap.SaveStatic();
	return 0;
}
