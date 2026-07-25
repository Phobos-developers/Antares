#include <InfantryClass.h>
#include <BuildingClass.h>
#include <SpecificStructures.h>
#include "../Building/Body.h"
#include "../BuildingType/Body.h"
#include "../Techno/Body.h"
#include "Body.h"
#include "../Rules/Body.h"
#include "../TechnoType/Body.h"
#include "../../Enum/CursorTypes.h"
#include "../../Misc/Actions.h"
#include <HouseClass.h>
#include <InputManagerClass.h>
#include <VoxClass.h>

// #664: Advanced Rubble - reconstruction part: Check
DEFINE_HOOK(0x51E635, InfantryClass_GetActionOnObject_EngineerOverFriendlyBuilding, 0x5) {
	GET(BuildingClass *, pTarget, ESI);
	GET(InfantryClass *, pThis, EDI);

	auto const pData = BuildingTypeExt::ExtMap.Find(pTarget->Type);

	if((pData->RubbleIntact || pData->RubbleIntactRemove)
		&& pThis && pTarget->Owner->IsAlliedWith(pThis))
	{
		CursorType::SetAction(static_cast<MouseCursorType>(94), Action::GRepair, 0);
		R->EAX(Action::GRepair);
		return 0x51E458;
	}

	// overwrote the fnstsw test, need to replicate it
	return (R->EAX() & 0x4000)
		? 0x51E63A
		: 0x51E659
	;
}

DEFINE_HOOK(0x51E4ED, InfantryClass_GetActionOnObject_EngineerRepairable, 0x6)
{
	GET(BuildingClass const* const, pTarget, ESI);

	auto const pType = pTarget->Type;
	auto const pExt = BuildingTypeExt::ExtMap.Find(pType);

	R->ECX(pExt->EngineerRepairable.Get(pType->Repairable));

	return 0x51E4F3;
}

DEFINE_HOOK(0x51FA82, InfantryClass_GetActionOnCell_EngineerRepairable, 0x6)
{
	GET(BuildingTypeClass const* const, pType, EBP);

	auto const pExt = BuildingTypeExt::ExtMap.Find(pType);

	R->EAX(pExt->EngineerRepairable.Get(pType->Repairable));

	return 0x51FA88;
}

DEFINE_HOOK(0x51E748, InfantryClass_GetActionOnObject_NoSelfGuardArea, 0x8)
{
	GET(InfantryClass const* const, pThis, EDI);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	return pExt->NoSelfGuardArea ? 0x51E7A6u : 0u;
}

// #664: Advanced Rubble - reconstruction part: Reconstruction
DEFINE_HOOK(0x519FAF, InfantryClass_UpdatePosition_EngineerRepairsFriendly, 0x6)
{
	GET(InfantryClass *, pThis, ESI);
	GET(BuildingClass *, Target, EDI);

	auto const pTargetType = Target->Type;
	BuildingTypeExt::ExtData* TargetTypeExtData = BuildingTypeExt::ExtMap.Find(pTargetType);

	if(TargetTypeExtData->RubbleIntact || TargetTypeExtData->RubbleIntactRemove) {
		auto const pRubble = BuildingExt::ExtData::PlaceRubble(Target,
			TargetTypeExtData->RubbleIntactRemove, TargetTypeExtData->RubbleIntact,
			TargetTypeExtData->RubbleIntactOwner, TargetTypeExtData->RubbleIntactStrength,
			TargetTypeExtData->RubbleIntactAnim);

		// the pile of rubble has served its purpose and is gone for good
		Target->UnInit();

		if(!pRubble) {
			// Rubble.Intact.Remove: nothing was put back, so there is nobody to
			// hand the engineer to. Take the "already at full health" exit.
			return 0x519FB9;
		}

		// hand the engineer to the building that is on the map now, not to the
		// one that just left it
		bool wasSelected = pThis->IsSelected;
		pThis->Limbo();
		CellStruct Cell = pThis->GetMapCoords();
		pRubble->KickOutUnit(pThis, Cell);
		if(wasSelected) {
			pThis->Select();
		}

		return 0x51A65D; //0x51A010 eats the Engineer, 0x51A65D hopefully does not
	}

	// EngineerRepairable was declared, parsed and streamed but never consulted:
	// a building an engineer may not repair takes the "nothing to repair" exit
	// at 0x519FB9 the same way a building already at full health does.
	auto const repairable = TargetTypeExtData->EngineerRepairable.Get(pTargetType->Repairable);

	return repairable ? 0u : 0x519FB9u;
}

DEFINE_HOOK(0x51DF38, InfantryClass_Remove, 0xA)
{
	GET(InfantryClass *, pThis, ESI);
	TechnoExt::ExtData* pData = TechnoExt::ExtMap.Find(pThis);

	if(auto pGarrison = pData->GarrisonedIn) {
		if(!pGarrison->Occupants.Remove(pThis)) {
			Debug::Log("Infantry %s was garrisoned in building %s, but building didn't find it. WTF?", pThis->Type->ID, pGarrison->Type->ID);
		}
	}

	pData->GarrisonedIn = nullptr;

	return 0;
}

DEFINE_HOOK(0x51DFFD, InfantryClass_Put, 0x5)
{
	GET(InfantryClass *, pThis, EDI);
	TechnoExt::ExtData* pData = TechnoExt::ExtMap.Find(pThis);
	pData->GarrisonedIn = nullptr;

	return 0;
}

DEFINE_HOOK(0x518434, InfantryClass_ReceiveDamage_SkipDeathAnim, 0x7)
{
	GET(InfantryClass *, pThis, ESI);
	//GET_STACK(ObjectClass *, pAttacker, 0xE0);
//	InfantryExt::ExtData* trooperAres = InfantryExt::ExtMap.Find(pThis);
//	bool skipInfDeathAnim = false; // leaving this default in case this is expanded in the future

	// there is not InfantryExt ExtMap yet!
	// too much space would get wasted since there is only four bytes worth of data we need to store per object
	// so those four bytes get stashed in Techno Map instead. they will get their own map if there's ever enough data to warrant it
	TechnoExt::ExtData* pData = TechnoExt::ExtMap.Find(pThis);

	return pData->GarrisonedIn ? 0x5185F1 : 0;
}

// should correct issue #743
DEFINE_HOOK(0x51D799, InfantryClass_PlayAnim_WaterSound, 0x7)
{
	GET(InfantryClass *, I, ESI);
	return (I->Transporter || I->Type->MovementZone != MovementZone::AmphibiousDestroyer)
		? 0x51D8BF
		: 0x51D7A6
	;
}

DEFINE_HOOK(0x51E5BB, InfantryClass_GetActionOnObject_MultiEngineerA, 0x7) {
	// skip old logic's way to determine the cursor
	return 0x51E5D9;
}

DEFINE_HOOK(0x51E5E1, InfantryClass_GetActionOnObject_MultiEngineerB, 0x7) {
	GET(BuildingClass *, pBld, ECX);
	Action ret = InfantryExt::GetEngineerEnterEnemyBuildingAction(pBld);

	// use a dedicated cursor
	if(ret == Action::Damage) {
		CursorType::SetAction(static_cast<MouseCursorType>(87), Action::Damage, 0);
	}

	// return our action
	R->EAX(ret);
	return 0;
}

DEFINE_HOOK(0x519D9C, InfantryClass_UpdatePosition_MultiEngineer, 0x5) {
	GET(InfantryClass *, pEngi, ESI);
	GET(BuildingClass *, pBld, EDI);

	// damage or capture
	Action action = InfantryExt::GetEngineerEnterEnemyBuildingAction(pBld);
	if(action == Action::Damage) {
		int Damage = static_cast<int>(ceil(pBld->Type->Strength * RulesExt::Global()->EngineerDamage));
		pBld->ReceiveDamage(&Damage, 0, RulesClass::Instance->C4Warhead, pEngi, true, false, nullptr);
		return 0x51A010;
	} else {
		return 0x519EAA;
	}
}

// #1008047: the C4 did not work correctly in YR, because some ability checks were missing
DEFINE_HOOK(0x51C325, InfantryClass_IsCellOccupied_C4Ability, 0x6)
{
	GET(InfantryClass*, pThis, EBP);

	return (pThis->Type->C4 || pThis->HasAbility(Ability::C4)) ? 0x51C37D : 0x51C335;
}

DEFINE_HOOK(0x51A4D2, InfantryClass_UpdatePosition_C4Ability, 0x6)
{
	GET(InfantryClass*, pThis, ESI);

	return (!pThis->Type->C4 && !pThis->HasAbility(Ability::C4)) ? 0x51A7F4 : 0x51A4E6;
}

// do not prone in water
DEFINE_HOOK(0x5201CC, InfantryClass_UpdatePanic_ProneWater, 0x6)
{
	GET(InfantryClass*, pThis, ESI);
	auto landType = pThis->GetCell()->LandType;
	return (landType != LandType::Beach && landType != LandType::Water) ? 0 : 0x5201DC;
}

// #1283638: ivans cannot enter grinders; they get an attack cursor. if the
// grinder is rigged with a bomb, ivans can enter. this fix lets ivans enter
// allied grinders. pressing the force fire key brings back the old behavior.
DEFINE_HOOK(0x51EB48, InfantryClass_GetActionOnObject_IvanGrinder, 0xA)
{
	GET(InfantryClass*, pThis, EDI);
	GET(ObjectClass*, pTarget, ESI);

	if(auto pTargetBld = abstract_cast<BuildingClass*>(pTarget)) {
		if(pTargetBld->Type->Grinding && pThis->Owner->IsAlliedWith(pTargetBld)) {
			if(!InputManagerClass::Instance->IsForceFireKeyPressed()) {
				static const byte return_grind[] = {
					0x5F, 0x5E, 0x5D, // pop edi, esi and ebp
					0xB8, 0x0B, 0x00, 0x00, 0x00, // eax = Action::Repair (not Action::Eaten)
					0x5B, 0x83, 0xC4, 0x28, // esp += 0x28
					0xC2, 0x08, 0x00 // retn 8
				};

				return reinterpret_cast<DWORD>(return_grind);
			}
		}
	}

	return 0;
}
