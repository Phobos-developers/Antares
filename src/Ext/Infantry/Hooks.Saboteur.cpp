#include "Body.h"

#include "../BuildingType/Body.h"
#include "../TechnoType/Body.h"
#include "../../Enum/CursorTypes.h"

#include <BuildingClass.h>
#include <HouseClass.h>

// what an infantry does when it enters a building, in the order the game asks
enum class InfiltrateAction {
	None = 0,
	Spy = 1,
	Sabotage = 2,
	Capture = 3,
	Bomb = 4
};

static InfiltrateAction GetInfiltrateAction(
	InfantryClass const* const pThis, BuildingClass const* const pBuilding)
{
	auto const pThisType = pThis->Type;
	auto const pBuildingType = pBuilding->Type;

	if(pThisType->C4 || pThis->HasAbility(Ability::C4)) {
		if(pBuildingType->CanC4) {
			return InfiltrateAction::Bomb;
		}
	}

	auto const agent = pThisType->Agent;

	if(agent && pBuildingType->Spyable) {
		auto const pOwner = pBuilding->GetOwningHouse();

		if(!pOwner || !pThis->Owner->IsAlliedWith(pOwner)) {
			return InfiltrateAction::Spy;
		}
	}

	auto const pThisTypeExt = TechnoTypeExt::ExtMap.Find(pThisType);
	auto const saboteur = pThisTypeExt->Saboteur;

	if(saboteur && BuildingTypeExt::IsSabotagable(pBuildingType)) {
		return InfiltrateAction::Sabotage;
	}

	if(agent || saboteur || !pBuildingType->Capturable) {
		return InfiltrateAction::None;
	}

	return InfiltrateAction::Capture;
}

DEFINE_HOOK(0x7004AD, TechnoClass_GetActionOnObject_Saboteur, 0x6)
{
	// this is known to be InfantryClass, and Infiltrate is yes
	GET(InfantryClass const* const, pThis, ESI);
	GET(ObjectClass const* const, pObject, EDI);

	auto const pBldObject = abstract_cast<BuildingClass const*>(pObject);

	if(!pBldObject) {
		return 0x700536;
	}

	return (GetInfiltrateAction(pThis, pBldObject) != InfiltrateAction::None)
		? 0x700531u
		: 0x700536u
	;
}

DEFINE_HOOK(0x51EE6B, InfantryClass_GetActionOnObject_Saboteur, 0x6)
{
	GET(InfantryClass const* const, pThis, EDI);
	GET(ObjectClass const* const, pObject, ESI);

	auto const pBldObject = abstract_cast<BuildingClass const*>(pObject);

	if(!pBldObject) {
		return 0x51F04E;
	}

	if(auto const pOwner = pBldObject->GetOwningHouse()) {
		if(pThis->Owner->IsAlliedWith(pOwner)) {
			return 0x51F04E;
		}
	}

	switch(GetInfiltrateAction(pThis, pBldObject)) {
	case InfiltrateAction::Spy:
	{
		auto const pExt = BuildingTypeExt::ExtMap.Find(pBldObject->Type);
		CursorType::SetAction(pExt->Cursor_Spy, Action::Capture, 0);
		return 0x51EEED;
	}
	case InfiltrateAction::Sabotage:
		CursorType::SetAction(static_cast<MouseCursorType>(93), Action::Capture, 0);
		return 0x51EEED;

	case InfiltrateAction::None:
		return 0x51F04E;

	default:
		return 0x51EEED;
	}
}

DEFINE_HOOK(0x51B2CB, InfantryClass_SetTarget_Saboteur, 0x6)
{
	GET(InfantryClass* const, pThis, ESI);
	GET(ObjectClass* const, pTarget, EDI);

	if(auto const pBldObject = abstract_cast<BuildingClass const*>(pTarget)) {
		auto const pThisType = pThis->Type;
		auto const pTargetType = pBldObject->Type;

		auto const pThisTypeExt = TechnoTypeExt::ExtMap.Find(pThisType);

		auto allowed = false;

		if(pThisType->Agent) {
			allowed = pTargetType->Spyable;
		} else if(pThisTypeExt->Saboteur) {
			allowed = BuildingTypeExt::IsSabotagable(pTargetType);
		} else {
			allowed = pTargetType->Capturable;
		}

		if(allowed) {
			pThis->SetDestination(pTarget, true);
		}
	}

	return 0x51B33F;
}

DEFINE_HOOK(0x519FF8, InfantryClass_UpdatePosition_Saboteur, 0x6)
{
	GET(InfantryClass* const, pThis, ESI);
	GET(BuildingClass* const, pBuilding, EDI);

	switch(GetInfiltrateAction(pThis, pBuilding)) {
	case InfiltrateAction::Spy:
		return 0x51A002;

	case InfiltrateAction::Sabotage:
		break;

	default:
		return 0x51A03E;
	}

	if(pBuilding->IsIronCurtained() || pBuilding->IsBeingWarpedOut()
		|| pBuilding->GetCurrentMission() == Mission::Selling)
	{
		// building not sabotagable atm
		pThis->AbortMotion();
		pThis->Uncloak(false);

	} else if(!pBuilding->C4Applied) {
		// sabotage
		pBuilding->C4Applied = true;
		pBuilding->C4AppliedBy = pThis;

		auto const delay = RulesClass::Instance->C4Delay;
		auto const duration = static_cast<int>(delay * 900);
		pBuilding->Flash(duration / 2);
		pBuilding->C4Timer.Start(duration);

		if(auto const pTag = pBuilding->AttachedTag) {
			pTag->RaiseEvent(
				TriggerEvent::EnteredBy, pThis, CellStruct::Empty, false,
				nullptr);
		}

		return 0x51A010;
	}

	// scatter out
	pThis->ReloadTimer.Start(pThis->GetROF(1));

	return 0x51A03E;
}
