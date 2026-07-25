#include "Body.h"

#include "../TechnoType/Body.h"
#include "../WeaponType/Body.h"
#include "../WarheadType/Body.h"
#include "../../Enum/CursorTypes.h"

#include <AircraftClass.h>
#include <BuildingClass.h>
#include <InfantryClass.h>
#include <InputManagerClass.h>
#include <UnitClass.h>

// the cursor a weapon shows over its target, defaulting to the vanilla pair
MouseCursorType TechnoExt::GetCursor(TechnoClass* pThis, int idxWeapon, bool outOfRange)
{
	auto const pWeapon = pThis->GetWeapon(idxWeapon);

	if(!pWeapon || !pWeapon->WeaponType) {
		return static_cast<MouseCursorType>(
			static_cast<int>(MouseCursorType::Attack) + outOfRange);
	}

	auto const pExt = WeaponTypeExt::ExtMap.Find(pWeapon->WeaponType);
	return outOfRange ? pExt->Cursor_AttackOutOfRange : pExt->Cursor_Attack;
}

// whether a heal weapon would restore this target, and which cursor says so
static bool GetHealCursor(TechnoClass* pThis, ObjectClass* pTarget, MouseCursorType& index)
{
	auto const pTargetType = pTarget->GetTechnoType();
	auto const idxWeapon = pThis->SelectWeapon(pTarget);
	auto const pWeapon = pThis->GetWeapon(idxWeapon)->WeaponType;
	auto const pExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);

	if(pExt->GetVerses(pTargetType->Armor).Verses <= 0.0) {
		return false;
	}

	index = (pTargetType->Organic || pTarget->WhatAmI() == AbstractType::Infantry)
		? static_cast<MouseCursorType>(90)
		: static_cast<MouseCursorType>(91)
	;

	return true;
}

DEFINE_HOOK(0x6FFEC0, TechnoClass_GetActionOnObject_Cursors, 0x5)
{
	GET(TechnoClass* const, pThis, ECX);
	GET_STACK(ObjectClass* const, pTarget, 0x4);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	CursorType::SetAction(pExt->Cursor_Move, Action::Move, 0);
	CursorType::SetAction(pExt->Cursor_NoMove, Action::NoMove, 0);

	if(auto const pTargetType = pTarget->GetTechnoType()) {
		auto const pTargetExt = TechnoTypeExt::ExtMap.Find(pTargetType);
		CursorType::SetAction(pTargetExt->Cursor_Enter, Action::Repair, 0);
		CursorType::SetAction(pTargetExt->Cursor_Enter, Action::Enter, 0);
		CursorType::SetAction(pTargetExt->Cursor_NoEnter, Action::NoEnter, 0);
	}

	return 0;
}

DEFINE_HOOK(0x700600, TechnoClass_GetActionOnCell_Cursors, 0x5)
{
	GET(TechnoClass* const, pThis, ECX);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	CursorType::SetAction(pExt->Cursor_Move, Action::Move, 0);
	CursorType::SetAction(pExt->Cursor_NoMove, Action::NoMove, 0);

	return 0;
}

DEFINE_HOOK(0x7000CD, TechnoClass_GetActionOnObject_SelfDeployCursor, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	CursorType::SetAction(pExt->Cursor_Deploy, Action::AreaAttack, 0);
	CursorType::SetAction(pExt->Cursor_Deploy, Action::Self_Deploy, 0);
	CursorType::SetAction(pExt->Cursor_NoDeploy, Action::NoDeploy, 0);

	return 0;
}

DEFINE_HOOK(0x7400F0, UnitClass_GetActionOnObject_SelfDeployCursor_Bunker, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	if(auto const pBunker = abstract_cast<BuildingClass*>(pThis->BunkerLinkedItem)) {
		auto const pExt = TechnoTypeExt::ExtMap.Find(pBunker->Type);
		CursorType::SetAction(pExt->Cursor_Deploy, Action::Self_Deploy, 0);
		return 0x73FFE6;
	}

	GET(TechnoTypeClass const* const, pType, EAX);

	return pType->DeployFire
		? 0x7400FA
		: 0x740115
	;
}

DEFINE_HOOK(0x70055D, TechnoClass_GetActionOnObject_AttackCursor, 0x8)
{
	GET(TechnoClass* const, pThis, ESI);
	GET_STACK(int const, idxWeapon, 0x14);

	CursorType::SetAction(
		TechnoExt::GetCursor(pThis, idxWeapon, false), Action::Attack, 0);

	return 0;
}

DEFINE_HOOK(0x700AA8, TechnoClass_GetActionOnCell_AttackCursor, 0x8)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(int const, idxWeapon, EBP);

	CursorType::SetAction(
		TechnoExt::GetCursor(pThis, idxWeapon, false), Action::Attack, 0);

	return 0;
}

DEFINE_HOOK(0x51E710, InfantryClass_GetActionOnObject_Heal, 0x7)
{
	GET(InfantryClass* const, pThis, EDI);
	GET(ObjectClass* const, pTarget, ESI);

	if(pThis == pTarget) {
		return 0x51E748;
	}

	if(pTarget && (pTarget->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None
		&& pTarget->GetHealthPercentage() < RulesClass::Instance->ConditionGreen
		&& !InputManagerClass::Instance->IsForceMoveKeyPressed())
	{
		auto index = MouseCursorType::Default;

		if(GetHealCursor(pThis, pTarget, index)) {
			CursorType::SetAction(index, Action::Heal, 0);
			return 0x51E739;
		}
	}

	if(auto const pBuilding = abstract_cast<BuildingClass const*>(pTarget)) {
		if(pBuilding->Type->Grinding) {
			return 0x51E7A6;
		}
	}

	return 0x51E757;
}

DEFINE_HOOK(0x73FDBD, UnitClass_GetActionOnObject_Heal, 0x5)
{
	GET(Action const, action, EBX);

	// the player explicitly asked to guard this spot, so do not offer to heal
	if(action == Action::GuardArea) {
		return 0x73FE48;
	}

	GET(UnitClass* const, pThis, ESI);
	GET(ObjectClass* const, pTarget, EDI);

	if(pThis == pTarget || InputManagerClass::Instance->IsForceMoveKeyPressed()) {
		return 0x73FE3B;
	}

	if(!pTarget || (pTarget->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) {
		return 0x73FE3B;
	}

	if(pTarget->GetHealthPercentage() >= RulesClass::Instance->ConditionGreen
		|| !pTarget->IsSurfaced())
	{
		return 0x73FE3B;
	}

	if(pTarget->WhatAmI() == AbstractType::Aircraft && pTarget->GetCell()->GetBuilding()) {
		return 0x73FE3B;
	}

	auto index = MouseCursorType::Default;

	if(!GetHealCursor(pThis, pTarget, index)) {
		return 0x73FE3B;
	}

	CursorType::SetAction(index, Action::GRepair, 0);

	return 0x73FE08;
}

DEFINE_HOOK(0x417DD2, AircraftClass_GetActionOnObject_NoManualUnload, 0x6)
{
	GET(AircraftClass const* const, pThis, ESI);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	return pExt->NoManualUnload ? 0x417DF4u : 0u;
}

// skip the check for UnitRepair, as it does not play well with UnitReload and
// Factory=AircraftType at all. in fact, it's prohibited, and thus docking to
// other structures was never allowed.
DEFINE_HOOK(0x417E16, AircraftClass_GetActionOnObject_Dock, 0x6)
{
	// target is known to be a building
	GET(AircraftClass* const, pThis, ESI);
	GET(BuildingClass* const, pBuilding, EDI);

	// enter and no-enter cursors only if aircraft can dock
	if(pThis->Type->Dock.FindItemIndex(pBuilding->Type) != -1) {
		return 0x417E4B;
	}

	// select cursor
	return 0x417E7D;
}

static_assert(offsetof(TechnoTypeClass, Organic) == 0xD97, "TechnoTypeClass layout slipped");
static_assert(offsetof(TechnoTypeClass, MoveToShroud) == 0xC8D, "TechnoTypeClass layout slipped");
static_assert(offsetof(TechnoClass, BunkerLinkedItem) == 0x2E4, "TechnoClass layout slipped");
