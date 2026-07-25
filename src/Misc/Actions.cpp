#include "Actions.h"
#include "../Misc/Debug.h"
#include "../Enum/CursorTypes.h"
#include "../Ext/Techno/Body.h"
#include <HouseClass.h>
#include <TechnoClass.h>

#include <Helpers\Macro.h>

// over shrouded cells most actions do not survive. this maps the ones that do
// onto the action whose cursor is shown instead.
static Action RemapShroudedAction(Action action)
{
	switch(action) {
	case Action::NoMove:
		if(ObjectClass::CurrentObjects.Count) {
			auto const pType = ObjectClass::CurrentObjects.GetItem(0)->GetTechnoType();
			if(pType->MoveToShroud) {
				return Action::Move;
			}
		}
		return Action::NoMove;

	case Action::Attack:
		return Action::Move;

	case Action::Enter:
	case Action::Self_Deploy:
	case Action::Harvest:
	case Action::Select:
	case Action::ToggleSelect:
	case Action::Capture:
	case Action::Repair:
	case Action::Sabotage:
	case Action::DontUse2:
	case Action::DontUse3:
	case Action::DontUse4:
	case Action::DontUse5:
	case Action::DontUse6:
	case Action::DontUse7:
	case Action::DontUse8:
	case Action::Damage:
	case Action::GRepair:
	case Action::EnterTunnel:
	case Action::DragWaypoint:
	case Action::AreaAttack:
	case Action::IvanBomb:
	case Action::NoIvanBomb:
	case Action::Detonate:
	case Action::DetonateAll:
	case Action::SelectNode:
	case Action::AttackSupport:
	case Action::Demolish:
	case Action::Airstrike:
		return Action::None;

	case Action::Eaten:
	case Action::NoGRepair:
		return Action::NoRepair;

	case Action::Sell:
	case Action::SellUnit:
		return Action::NoSell;

	case Action::TogglePower:
		return Action::NoTogglePower;

	case Action::NoEnterTunnel:
		return Action::NoEnter;

	case Action::ChronoWarp:
		return Action::ChronoSphere;

	case Action::SelectBeacon:
		return Action::Select;

	case Action::AmerParaDrop:
		return Action::ParaDrop;

	default:
		return action;
	}
}

// the cursor an action falls back to when the override table has no entry
static MouseCursorType GetActionCursor(Action action)
{
	switch(action) {
	case Action::Move:
		return MouseCursorType::Move;

	case Action::NoMove:
	case Action::NoIvanBomb:
		return MouseCursorType::NoMove;

	case Action::Enter:
	case Action::Capture:
	case Action::Repair:
	case Action::EnterTunnel:
		return MouseCursorType::Enter;

	case Action::Self_Deploy:
	case Action::AreaAttack:
		return MouseCursorType::Deploy;

	case Action::Attack:
		return MouseCursorType::Attack;

	case Action::Harvest:
		return MouseCursorType::AttackOutOfRange;

	case Action::Select:
	case Action::ToggleSelect:
	case Action::SelectBeacon:
		return MouseCursorType::Select;

	case Action::Eaten:
		return MouseCursorType::EngineerRepair;

	case Action::Sell:
		return MouseCursorType::Sell;

	case Action::SellUnit:
		return MouseCursorType::SellUnit;

	case Action::NoSell:
		return MouseCursorType::NoSell;

	case Action::NoRepair:
	case Action::NoGRepair:
		return MouseCursorType::NoRepair;

	case Action::Sabotage:
	case Action::Demolish:
		return MouseCursorType::Demolish;

	case Action::Tote:
		return static_cast<MouseCursorType>(86);

	case Action::Nuke:
		return MouseCursorType::Nuke;

	case Action::GuardArea:
		return MouseCursorType::Protect;

	case Action::Heal:
	case Action::PlaceWaypoint:
	case Action::TibSunBug:
	case Action::EnterWaypointMode:
	case Action::FollowWaypoint:
	case Action::SelectWaypoint:
	case Action::LoopWaypointPath:
	case Action::AttackWaypoint:
	case Action::EnterWaypoint:
	case Action::PatrolWaypoint:
		return MouseCursorType::Disallowed;

	case Action::GRepair:
		return MouseCursorType::Repair;

	case Action::NoDeploy:
		return MouseCursorType::NoDeploy;

	case Action::NoEnter:
		return MouseCursorType::NoEnter;

	case Action::TogglePower:
		return static_cast<MouseCursorType>(88);

	case Action::NoTogglePower:
		return static_cast<MouseCursorType>(89);

	case Action::IronCurtain:
		return MouseCursorType::IronCurtain;

	case Action::LightningStorm:
		return MouseCursorType::LightningStorm;

	case Action::ChronoSphere:
	case Action::ChronoWarp:
		return MouseCursorType::Chronosphere;

	case Action::ParaDrop:
	case Action::AmerParaDrop:
		return MouseCursorType::ParaDrop;

	case Action::IvanBomb:
		return MouseCursorType::IvanBomb;

	case Action::Detonate:
	case Action::DetonateAll:
		return MouseCursorType::Detonate;

	case Action::DisarmBomb:
		return MouseCursorType::Disarm;

	case Action::PlaceBeacon:
		return MouseCursorType::Beacon;

	case Action::AttackMoveNav:
	case Action::AttackMoveTar:
		return MouseCursorType::AttackOutOfRange2;

	case Action::PsychicDominator:
		return MouseCursorType::PsychicDominator;

	case Action::SpyPlane:
		return MouseCursorType::SpyPlane;

	case Action::GeneticConverter:
		return MouseCursorType::GeneticMutator;

	case Action::ForceShield:
		return MouseCursorType::ForceShield;

	case Action::NoForceShield:
		return MouseCursorType::NoForceShield;

	case Action::Airstrike:
		return MouseCursorType::AirStrike;

	case Action::PsychicReveal:
		return MouseCursorType::PsychicReveal;

	default:
		return MouseCursorType::Default;
	}
}

DEFINE_HOOK(0x4AB35A, DisplayClass_SetAction_CustomCursor, 0x6)
{
	GET(MouseClass* const, pThis, ESI);
	GET(Action, action, EAX);

	GET_STACK(ObjectClass* const, pObject, 0x18);
	GET_STACK(bool const, shrouded, 0x28);
	GET_STACK(bool const, miniMap, 0x34);

	if(shrouded) {
		auto const pOverride = CursorType::FindAction(action);

		if(pOverride && pOverride->Mode == 2) {
			action = Action::None;
		} else if(!pOverride || pOverride->Mode != 1) {
			action = RemapShroudedAction(action);
		}
	}

	if(action == Action::Attack) {
		auto const pCurrent = (ObjectClass::CurrentObjects.Count == 1)
			? ObjectClass::CurrentObjects.GetItem(0) : nullptr;

		if(pObject && pCurrent && (pCurrent->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None) {
			auto const pTechno = static_cast<TechnoClass*>(pCurrent);
			auto const idxWeapon = pTechno->SelectWeapon(pObject);

			if(!pTechno->IsCloseEnough(pObject, idxWeapon)) {
				CursorType::SetAction(
					TechnoExt::GetCursor(pTechno, idxWeapon, true), Action::Attack, 0);
			}
		} else {
			action = Action::Harvest;
		}
	}

	auto const pOverride = CursorType::FindAction(action);
	auto const index = pOverride
		? static_cast<MouseCursorType>(pOverride->Index)
		: GetActionCursor(action);

	pThis->SetCursor(index, miniMap);

	return 0x4AB78F;
}

DEFINE_HOOK(0x5BDDC0, MouseClass_Update_Reset, 0x5)
{
	CursorType::ClearActions();
	return 0;
}

DEFINE_HOOK(0x4D7524, FootClass_ActionOnObject_Allow, 0x9)
{
	//overwrote the ja, need to replicate it
	GET(Action, CursorIndex, EBP);
	if(CursorIndex == Action::None || CursorIndex > Action::Airstrike) {
		if(CursorIndex == Actions::SuperWeaponAllowed || CursorIndex == Actions::SuperWeaponDisallowed) {
			return 0x4D769F;
		} else {
			return 0x4D7CC0;
		}
	} else {
		return 0x4D752D;
	}
}

DEFINE_HOOK(0x653CA6, RadarClass_GetMouseAction_AllowMinimap, 0x5)
{
	GET(Action const, action, ESI);

	if(auto const pOverride = CursorType::FindAction(action)) {
		return (CursorType::GetCursor(static_cast<MouseCursorType>(pOverride->Index))->MiniFrame >= 0)
			? 0x653CC0
			: 0x653CBA
		;
	}

	//overwrote the ja, need to replicate it
	return (static_cast<unsigned int>(action) > 0x48u)
		? 0x653CBA
		: 0x653CAB
	;
}

DEFINE_HOOK(0x6929FC, DisplayClass_ChooseAction_CanSell, 0x7)
{
	GET(TechnoClass *, Target, ESI);
	switch(Target->WhatAmI()) {
		case AbstractType::Aircraft:
		case AbstractType::Unit:
			R->Stack(0x10, Action::SellUnit);
			return 0x692B06;
		case AbstractType::Building:
			R->Stack(0x10, Target->IsStrange() ? Action::NoSell : Action::Sell);
			return 0x692B06;
		default:
			return 0x692AFE;
	}
}

DEFINE_HOOK(0x4ABFBE, DisplayClass_LeftMouseButtonUp_ExecPowerToggle, 0x7)
{
	GET(TechnoClass *, Target, ESI);
	return (Target && Target->Owner->IsControlledByHuman() && Target->WhatAmI() == AbstractType::Building)
	 ? 0x4ABFCE
	 : 0x4AC294
	;
}
