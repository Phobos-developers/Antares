#include "Body.h"
#include "../../Utilities/AresEnums.h"

#include <HouseClass.h>
#include <TagClass.h>
#include <UnitClass.h>

DEFINE_HOOK(0x71E949, TEventClass_HasOccured, 0x7)
{
	GET(TEventClass*, pEvent, EBP);

	GET_STACK(TriggerEvent const, eventType, 0x30);
	GET_STACK(HouseClass* const, pOwner, 0x34);
	GET_STACK(ObjectClass* const, pAttachedTo, 0x38);
	GET_STACK(void* const, pSource, 0x44);

	// check for events handled in Ares.
	bool ret = false;
	if(TEventExt::HasOccured(pEvent, eventType, pOwner, pAttachedTo, pSource, &ret)) {
		// returns true or false
		return ret ? 0x71F1B1 : 0x71F163;
	}

	// not handled in Ares.
	return 0;
}

// the general events requiring a house
DEFINE_HOOK(0x71F06C, TEventClass_HasOccured_PlayerAtX1, 0x5)
{
	GET(int const, param, ECX);

	auto const pHouse = TEventExt::ResolveHouseParam(param);
	R->EAX(pHouse);

	// continue normally if a house was found or this isn't Player@X logic,
	// otherwise return false directly so events don't fire for non-existing
	// players.
	return (pHouse || !HouseClass::Index_IsMP(param)) ? 0x71F071u : 0x71F0D5u;
}

// validation for Spy as House, the Entered/Overflown Bys and the Crossed V/H Lines
DEFINE_HOOK_AGAIN(0x71ED33, TEventClass_HasOccured_PlayerAtX2, 0x5)
DEFINE_HOOK_AGAIN(0x71F1C9, TEventClass_HasOccured_PlayerAtX2, 0x5)
DEFINE_HOOK_AGAIN(0x71F1ED, TEventClass_HasOccured_PlayerAtX2, 0x5)
DEFINE_HOOK(0x71ED01, TEventClass_HasOccured_PlayerAtX2, 0x5)
{
	GET(int const, param, ECX);
	R->EAX(TEventExt::ResolveHouseParam(param));
	return R->Origin() + 5;
}

// param for Attacked by House is the array index
DEFINE_HOOK(0x71EE79, TEventClass_HasOccured_PlayerAtX3, 0x9)
{
	GET(int, param, EAX);
	GET(HouseClass* const, pHouse, EDX);

	// convert Player @ X to real index
	if(HouseClass::Index_IsMP(param)) {
		auto const pPlayer = TEventExt::ResolveHouseParam(param);
		param = pPlayer ? pPlayer->ArrayIndex : -1;
	}

	return (pHouse->ArrayIndex == param) ? 0x71EE82u : 0x71F163u;
}

// what the new events attach to
DEFINE_HOOK(0x71F683, TEventClass_GetFlags, 0x5)
{
	GET(TriggerEvent const, eventKind, ECX);

	auto flags = 0;
	if(TEventExt::GetAttachFlags(eventKind, &flags)) {
		R->EAX(flags);
		return 0x71F6F6;
	}

	return (static_cast<unsigned int>(eventKind) > 59u) ? 0x71F69Cu : 0x71F688u;
}

// how the new events write their parameters back to INI
DEFINE_HOOK(0x71F39B, TEventClass_SaveToINI, 0x5)
{
	GET(TriggerEvent const, eventKind, EDX);

	auto mode = 0;
	if(TEventExt::GetSaveMode(eventKind, &mode)) {
		R->EAX(mode);
		return 0x71F3FE;
	}

	return (static_cast<unsigned int>(eventKind) > 61u) ? 0x71F3FCu : 0x71F3A0u;
}

// whether the new events are remembered once they occured
DEFINE_HOOK(0x71F9C0, TEventClass_Persistable, 0x6)
{
	GET(TEventClass* const, pEvent, ECX);

	auto persistable = false;
	if(TEventExt::GetPersistable(pEvent->EventKind, &persistable)) {
		R->EAX(persistable ? 1 : 0);
		return 0x71F9DF;
	}

	return 0;
}

// the spotlight events, replacing the game's Enemy In Spotlight
DEFINE_HOOK(0x4368C9, BuildingLightClass_Update_Trigger, 0x5)
{
	GET(ObjectClass* const, pTechno, EAX);

	if(auto const pTag = pTechno->AttachedTag) {
		pTag->RaiseEvent(TriggerEvent::EnemyInSpotlight, pTechno, CellStruct::Empty);
	}

	if(pTechno->IsAlive) {
		if(auto const pTag = pTechno->AttachedTag) {
			pTag->RaiseEvent(AresTriggerEvent::EnemyInSpotlightNow, pTechno, CellStruct::Empty);
		}
	}

	return 0x4368D9;
}

// destroyed by the house owning the killer
DEFINE_HOOK(0x702DD6, TechnoClass_RegisterDestruction_Trigger, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(TechnoClass* const, pKiller, EDI);

	if(pThis->IsAlive && pKiller) {
		if(auto const pTag = pThis->AttachedTag) {
			// the _ByHouse events reuse the source slot for a house, so this and
			// the two below are the only RaiseEvent calls that do not pass a techno
			pTag->RaiseEvent(
				AresTriggerEvent::DestroyedByHouse, pThis, CellStruct::Empty, false,
				reinterpret_cast<TechnoClass*>(pKiller->Owner));
		}
	}

	return 0;
}

DEFINE_HOOK(0x7032B0, TechnoClass_RegisterLoss_Trigger, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(HouseClass* const, pKiller, EDI);

	if(pThis->IsAlive && pKiller) {
		if(auto const pTag = pThis->AttachedTag) {
			pTag->RaiseEvent(
				AresTriggerEvent::DestroyedByHouse, pThis, CellStruct::Empty, false,
				reinterpret_cast<TechnoClass*>(pKiller));
		}
	}

	return 0;
}

DEFINE_HOOK(0x744745, UnitClass_RegisterDestruction_Trigger, 0x5)
{
	GET(UnitClass* const, pThis, ESI);
	GET(TechnoClass* const, pKiller, EDI);

	if(auto const pTag = pThis->AttachedTag) {
		pTag->RaiseEvent(
			AresTriggerEvent::DestroyedByHouse, pThis, CellStruct::Empty, false,
			reinterpret_cast<TechnoClass*>(pKiller->Owner));
	}

	return 0;
}
