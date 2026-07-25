#include "Body.h"
#include "../../Utilities/AresEnums.h"

#include "../../Ext/House/Body.h"
#include "../../Ext/Techno/Body.h"

#include "../../Misc/EVAVoices.h"
#include "../../Misc/SWTypes.h"
#include "../../Misc/SWTypes/Firewall.h"

#include "../../Misc/SavegameDef.h"

#include <TagClass.h>
#include <TechnoClass.h>

//Static init
TActionExt::ExtContainer TActionExt::ExtMap;

// Enables the Firestorm super weapon for a house.
/*!
	\returns Always true.

	\date 2012-09-24
*/
bool TActionExt::ExtData::ActivateFirestorm(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	if(!pHouse->FirestormActive) {
		auto index = pHouse->FindSuperWeaponIndex(SW_Firewall::FirewallType);

		if(index >= 0) {
			pHouse->Fire_SW(index, CellStruct::Empty);
		}
	}
	return true;
}

// Disables the Firestorm super weapon for a house.
/*!
	\returns Always true.

	\date 2012-09-24
*/
bool TActionExt::ExtData::DeactivateFirestorm(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	if(pHouse->FirestormActive) {
		auto index = pHouse->FindSuperWeaponIndex(SW_Firewall::FirewallType);

		if(index >= 0) {
			pHouse->Fire_SW(index, CellStruct::Empty);
		}
	}
	return true;
}

// Changes a house's auxiliary power.
/*!
	\returns Always true.

	\date 2022-02-04
*/
bool TActionExt::ExtData::AuxiliaryPower(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto const pTarget = pAction->FindHouseByIndex(pTrigger, pAction->Value);
	auto const pTargetExt = HouseExt::ExtMap.Find(pTarget);

	pTargetExt->AuxPower += pAction->Value2;
	pTarget->RecheckPower = true;

	return true;
}

// Kills the drivers of all objects this trigger is attached to.
/*!
	\returns True if any object could have its driver killed, false otherwise.

	\date 2022-02-04
*/
bool TActionExt::ExtData::KillDriversOf(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto pTarget = pAction->FindHouseByIndex(pTrigger, pAction->Value);

	if(!pTarget) {
		pTarget = HouseClass::FindSpecial();
	}

	auto affected = false;
	auto changed = false;

	// killing a driver can remove objects from the array, so rescan until
	// nothing changes any more.
	do {
		changed = false;

		for(auto i = 0; i < TechnoClass::Array.Count; ++i) {
			auto const pTechno = TechnoClass::Array.GetItem(i);

			if(pTechno->Health <= 0 || !pTechno->IsAlive || !pTechno->IsOnMap
				|| pTechno->InLimbo)
			{
				continue;
			}

			auto const pTag = pTechno->AttachedTag;

			if(!pTag || !pTag->ContainsTrigger(pTrigger)) {
				continue;
			}

			auto const pTechnoExt = TechnoExt::ExtMap.Find(pTechno);

			if(pTechnoExt->DriverKilled || !pTechnoExt->IsDriverKillable(1.0)) {
				continue;
			}

			if(pTechnoExt->ApplyKillDriver(pTarget, nullptr, false)) {
				changed = true;
				--i;
			}

			affected = true;
		}
	} while(changed);

	return affected;
}

// Sets the EVA voice used from now on.
/*!
	\returns Always true.

	\date 2022-02-04
*/
bool TActionExt::ExtData::SetEVAVoice(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto const value = pAction->Value;

	if(value < static_cast<int>(EVAVoices::Types.size()) + 3) {
		VoxClass::EVAIndex = (value < -1) ? -1 : value;
	}

	return true;
}

// Sets the group of the object that triggered this action.
/*!
	\returns Always true.

	\date 2022-02-04
*/
bool TActionExt::ExtData::SetGroup(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	if(pObject) {
		static_cast<TechnoClass*>(pObject)->Group = pAction->Value;
	}

	return true;
}

// Handles the execution of actions.
/*!
	Override any execution of actions here. Set ret to the result of the action.

	\returns True if this action was executed by Ares, false otherwise.

	\date 2012-09-24
*/
bool TActionExt::Execute(
	TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location, bool* ret)
{
	auto pExt = ExtMap.Find(pAction);

	switch(pAction->ActionKind) {
	case TriggerAction::PlaySoundEffectRandom:
		// #1004906: function replaced to support more than 100 waypoints
		*ret = pAction->PlayAudioAtRandomWP(pHouse, pObject, pTrigger, location);
		break;

	case TriggerAction::ActivateFirestorm:
		*ret = pExt->ActivateFirestorm(pAction, pHouse, pObject, pTrigger, location);
		break;

	case TriggerAction::DeactivateFirestorm:
		*ret = pExt->DeactivateFirestorm(pAction, pHouse, pObject, pTrigger, location);
		break;

	case AresTriggerAction::AuxiliaryPower:
		*ret = pExt->AuxiliaryPower(pAction, pHouse, pObject, pTrigger, location);
		break;

	case AresTriggerAction::KillDriversOf:
		*ret = pExt->KillDriversOf(pAction, pHouse, pObject, pTrigger, location);
		break;

	case AresTriggerAction::SetEVAVoice:
		*ret = pExt->SetEVAVoice(pAction, pHouse, pObject, pTrigger, location);
		break;

	case AresTriggerAction::SetGroup:
		*ret = pExt->SetGroup(pAction, pHouse, pObject, pTrigger, location);
		break;

	default:
		UNREFERENCED_PARAMETER(pExt);
		return false;
	}

	return true;
}

// Gets what the new actions parse their arguments as.
bool TActionExt::GetMode(TriggerAction const actionKind, int* ret)
{
	switch(actionKind) {
	case AresTriggerAction::AuxiliaryPower:
		*ret = 46;
		break;

	case AresTriggerAction::KillDriversOf:
		*ret = 0;
		break;

	case AresTriggerAction::SetEVAVoice:
	case AresTriggerAction::SetGroup:
		*ret = 10;
		break;

	default:
		return false;
	}

	return true;
}

// Gets what the new actions attach to.
bool TActionExt::GetFlags(TriggerAction const actionKind, int* ret)
{
	switch(actionKind) {
	case AresTriggerAction::AuxiliaryPower:
	case AresTriggerAction::SetEVAVoice:
		*ret = 0;
		break;

	case AresTriggerAction::KillDriversOf:
	case AresTriggerAction::SetGroup:
		*ret = 2;
		break;

	default:
		return false;
	}

	return true;
}

// =============================
// load / save

template <typename T>
void TActionExt::ExtData::Serialize(T& Stm) {
	//Stm;
}

void TActionExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<TActionClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TActionExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<TActionClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") {
}

TActionExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

#ifdef MAKE_GAME_SLOWER_FOR_NO_REASON
DEFINE_HOOK(0x6DD176, TActionClass_CTOR, 0x5)
{
	GET(TActionClass*, pItem, ESI);

	TActionExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x6E4761, TActionClass_SDDTOR, 0x6)
{
	GET(TActionClass*, pItem, ESI);

	TActionExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x6E3E30, TActionClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x6E3DB0, TActionClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TActionClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TActionExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x6E3E29, TActionClass_Load_Suffix, 0x4)
{
	TActionExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x6E3E4A, TActionClass_Save_Suffix, 0x3)
{
	TActionExt::ExtMap.SaveStatic();
	return 0;
}
#endif
