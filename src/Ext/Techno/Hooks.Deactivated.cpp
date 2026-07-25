#include "Body.h"
#include "../Building/Body.h"
#include "../TechnoType/Body.h"
#include "../WeaponType/Body.h"
#include "../Rules/Body.h"
#include "../../Misc/Debug.h"

#include <AircraftClass.h>
#include <BombClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>
#include <VocClass.h>

static bool IsDeactivated(TechnoClass * pThis) {
	return TechnoExt::ExtMap.Find(pThis)->IsDeactivated();
};

// the action a deactivated object offers: detonating its own bomb, selecting an
// allied object, or nothing at all.
Action TechnoExt::GetBombOverObject(TechnoClass* pThis, ObjectClass* pTarget)
{
	if(pThis == pTarget && ObjectClass::CurrentObjects.Count == 1) {
		if(auto const pBomb = pThis->AttachedBomb) {
			if(pBomb->OwnerHouse->IsControlledByCurrentPlayer()) {
				auto const pExt = WeaponTypeExt::BombExt.get_or_default(pBomb);

				auto const canDetonate = (pBomb->IsDeathBomb() == FALSE)
					? pExt->Ivan_CanDetonateTimeBomb.Get(RulesClass::Instance->CanDetonateTimeBomb)
					: pExt->Ivan_CanDetonateDeathBomb.Get(RulesClass::Instance->CanDetonateDeathBomb);

				if(canDetonate) {
					return Action::Detonate;
				}
			}
		}
	}

	if(pTarget && (pTarget->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None) {
		if(auto const pOwner = pTarget->GetOwningHouse()) {
			if(pThis->Owner->IsAlliedWith(pOwner) && pTarget->IsSelectable()) {
				return Action::Select;
			}
		}
	}

	return Action::None;
}

DEFINE_HOOK(0x447548, BuildingClass_GetActionOnCell_Deactivated, 0x6)
{
	GET(BuildingClass *, pThis, ESI);
	if(IsDeactivated(pThis)) {
		R->EBX(Action::None);
		return 0x44776D;
	}
	return 0;
}

DEFINE_HOOK(0x447218, BuildingClass_GetActionOnObject_Deactivated, 0x6)
{
	GET(BuildingClass *, pThis, ESI);
	GET_STACK(ObjectClass *, pThat, 0x1C);
	if(IsDeactivated(pThis)) {
		R->EAX(TechnoExt::GetBombOverObject(pThis, pThat));
		return 0x447273;
	}
	return 0;
}

DEFINE_HOOK(0x7404B9, UnitClass_GetActionOnCell_Deactivated, 0x6)
{
	GET(UnitClass *, pThis, ESI);
	if(IsDeactivated(pThis)) {
		R->EAX(Action::None);
		return 0x740805;
	}
	return 0;
}

DEFINE_HOOK(0x73FD5A, UnitClass_GetActionOnObject_Deactivated, 0x5)
{
	GET(UnitClass *, pThis, ECX);
	GET_STACK(ObjectClass *, pThat, 0x20);
	if(IsDeactivated(pThis)) {
		R->EAX(TechnoExt::GetBombOverObject(pThis, pThat));
		return 0x73FD72;
	}
	return 0;
}

DEFINE_HOOK(0x51F808, InfantryClass_GetActionOnCell_Deactivated, 0x6)
{
	GET(InfantryClass *, pThis, EDI);
	if(IsDeactivated(pThis)) {
		R->EBX(Action::None);
		return 0x51FAE2;
	}
	return 0;
}

DEFINE_HOOK(0x51E440, InfantryClass_GetActionOnObject_Deactivated, 0x8)
{
	GET(InfantryClass *, pThis, EDI);
	GET_STACK(ObjectClass *, pThat, 0x3C);
	if(IsDeactivated(pThis)) {
		R->EAX(TechnoExt::GetBombOverObject(pThis, pThat));
		return 0x51E458;
	}
	return 0;
}

DEFINE_HOOK(0x417F83, AircraftClass_GetActionOnCell_Deactivated, 0x6)
{
	GET(AircraftClass *, pThis, ESI);
	if(IsDeactivated(pThis)) {
		R->EAX(Action::None);
		return 0x417F94;
	}
	return 0;
}

DEFINE_HOOK(0x417CCB, AircraftClass_GetActionOnObject_Deactivated, 0x5)
{
	GET(AircraftClass *, pThis, ECX);
	GET_STACK(ObjectClass *, pThat, 0x20);
	if(IsDeactivated(pThis)) {
		R->EAX(TechnoExt::GetBombOverObject(pThis, pThat));
		return 0x417CDF;
	}
	return 0;
}

DEFINE_HOOK(0x4D74EC, FootClass_ActionOnObject_Deactivated, 0x6)
{
	GET(FootClass *, pThis, ESI);
	GET_STACK(Action const, action, 0x10C);

	// Action::Detonate is the one order a deactivated object still accepts -
	// GetBombOverObject offers it over one, so the click must not be swallowed.
	// The explicit 0x4D74FA is required: returning 0 would run the overwritten
	// "mov al,[esi+298h]" and the game's own test would jump to 0x4D77EC anyway
	return (action != Action::Detonate && IsDeactivated(pThis))
		? 0x4D77EC
		: 0x4D74FA
	;
}

// another hook is at 443414 and shares the EIP with a trench enter handler

DEFINE_HOOK(0x4D7D58, FootClass_ActionOnCell_Deactivated, 0x6)
{
	GET(FootClass *, pThis, ESI);
	return (IsDeactivated(pThis))
		? 0x4D7D62
		: 0
	;
}

DEFINE_HOOK(0x4436F7, BuildingClass_ActionOnCell_Deactivated, 0x5)
{
	GET(BuildingClass *, pThis, ECX);
	return (IsDeactivated(pThis))
		? 0x443729
		: 0
	;
}

DEFINE_HOOK(0x5200B3, InfantryClass_UpdatePanic, 0x6)
{
	GET(InfantryClass *, pThis, ESI);
	if(IsDeactivated(pThis)) {
		if(pThis->PanicDurationLeft > 0) {
			--pThis->PanicDurationLeft;
		}
		return 0x52025A;
	}
	return 0;
}

DEFINE_HOOK(0x51D0DD, InfantryClass_Scatter, 0x6)
{
	GET(InfantryClass *, pThis, ESI);
	return (IsDeactivated(pThis))
		? 0x51D6E6
		: 0
	;
}

// do not order deactivated units to move
DEFINE_HOOK(0x73DBF9, UnitClass_Mi_Unload_Decactivated, 0x5)
{
	GET(FootClass*, pUnloadee, EDI);
	LEA_STACK(CellStruct**, ppCell, 0x0);
	LEA_STACK(CellStruct*, pPosition, 0x1C);

	if(pUnloadee->Deactivated) {
		pUnloadee->Locomotor->Power_Off();
	}

	if(!pUnloadee->Locomotor->Is_Powered()) {
		*ppCell = pPosition;
	}

	return 0;
}

DEFINE_HOOK(0x736135, UnitClass_Update_Deactivated, 0x6)
{
	GET(UnitClass*, pThis, ESI);
	auto pExt = TechnoExt::ExtMap.Find(pThis);

	// don't sparkle on EMP, Operator, ....
	return pExt->IsPowered() ? 0x7361A9 : 0;
}

DEFINE_HOOK(0x73C143, UnitClass_DrawVXL_Deactivated, 0x5)
{
	GET(UnitClass*, pThis, ECX);
	REF_STACK(int, Value, 0x1E0);

	auto pRules = RulesExt::Global();
	double factor = 1.0;

	if(pThis->IsUnderEMP()) {
		factor = pRules->DeactivateDim_EMP;
	} else if(pThis->IsDeactivated()) {
		auto pExt = TechnoExt::ExtMap.Find(pThis);

		// use the operator check because it is more
		// efficient than the powered check.
		if(!pExt->IsOperated()) {
			factor = pRules->DeactivateDim_Operator;
		} else {
			factor = pRules->DeactivateDim_Powered;
		}
	}

	Value = static_cast<int>(Value * factor);

	return 0x73C15F;
}

// complete replacement
DEFINE_HOOK(0x70FBE0, TechnoClass_Activate, 0x6)
{
	GET(TechnoClass* const, pThis, ECX);
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	/* Check abort conditions:
		- Is the object currently EMP'd?
		- Does the object need an operator, but doesn't have one?
		- Does the object need a powering structure that is offline?
		If any of the above conditions, bail out and don't activate the object.
	*/
	if(pThis->IsUnderEMP() || !pExt->IsPowered() || !pExt->IsOperated()) {
		return 0x70FC85;
	}

	pThis->Guard();

	if(auto const pFoot = abstract_cast<FootClass*>(pThis)) {
		pFoot->Locomotor->Power_On();
	}

	auto const wasDeactivated = std::exchange(pThis->Deactivated, false);

	if(wasDeactivated) {
		auto const pType = pThis->GetTechnoType();

		// change: don't play sound when mutex active
		if(!Unsorted::ScenarioInit && pType->ActivateSound != -1) {
			VocClass::PlayAt(pType->ActivateSound, pThis->Location, nullptr);
		}

		// change: add spotlight
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
		if(pTypeExt->Is_Spotlighted) {
			++Unsorted::ScenarioInit;
			auto const pSpotlight = GameCreate<BuildingLightClass>(pThis);
			pExt->SetSpotlight(pSpotlight);
			--Unsorted::ScenarioInit;
		}

		// change: update factories
		if(auto const pBld = abstract_cast<BuildingClass*>(pThis)) {
			BuildingExt::UpdateFactoryQueues(pBld);
		}
	}

	return 0x70FC85;
}

// complete replacement
DEFINE_HOOK(0x70FC90, TechnoClass_Deactivate, 0x6)
{
	GET(TechnoClass* const, pThis, ECX);

	// don't deactivate when inside/on the linked building
	if(pThis->IsTether) {
		auto const pLink = pThis->GetNthLink(0);

		if(pLink && pThis->GetCell()->GetBuilding() == pLink) {
			return 0x70FD6E;
		}
	}

	pThis->Guard();
	pThis->Deselect();

	if(auto const pFoot = abstract_cast<FootClass*>(pThis)) {
		pFoot->Locomotor->Power_Off();
	}

	auto const wasDeactivated = std::exchange(pThis->Deactivated, true);

	if(!wasDeactivated) {
		auto const pType = pThis->GetTechnoType();

		// change: don't play sound when mutex active
		if(!Unsorted::ScenarioInit && pType->DeactivateSound != -1) {
			VocClass::PlayAt(pType->DeactivateSound, pThis->Location, nullptr);
		}

		// change: remove spotlight
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
		if(pTypeExt->Is_Spotlighted) {
			auto const pExt = TechnoExt::ExtMap.Find(pThis);
			pExt->SetSpotlight(nullptr);
		}

		// change: update factories
		if(auto const pBld = abstract_cast<BuildingClass*>(pThis)) {
			BuildingExt::UpdateFactoryQueues(pBld);
		}
	}

	return 0x70FD6E;
}
