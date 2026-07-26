#include "Body.h"

#include "../HouseType/Body.h"
#include "../Rules/Body.h"
#include "../Script/Body.h"

#include <AircraftClass.h>
#include <HouseClass.h>
//#include <Helpers/Enumerators.h>

// #895225: make the AI smarter. this code was missing from YR.
// it clears the targets and assigns the attacker the team's current focus.
DEFINE_HOOK(0x6EB432, TeamClass_AttackedBy_Retaliate, 0x9)
{
	GET(TeamClass*, pThis, ESI);
	GET(AbstractClass*, pAttacker, EBP);

	// get ot if global option is off
	if(!RulesExt::Global()->TeamRetaliate) {
		return 0x6EB47A;
	}

	auto pFocus = abstract_cast<TechnoClass*>(pThis->Focus);
	auto pSpawn = pThis->SpawnCell;

	// a team with a focus it can still shoot only switches when that focus has
	// been left behind; with no spawn cell to measure against, it keeps the one
	// it has
	if(!pFocus || !pFocus->GetWeapon(0)->WeaponType
		|| (pSpawn && pFocus->IsCloseEnoughToAttackCoords(pSpawn->GetCoords())))
	{
		// disallow aircraft, or units considered as aircraft, or stuff not on map like parasites
		if(pAttacker->WhatAmI() != AircraftClass::AbsID) {
			// friendly fire is not a reason to change targets
			if(auto const pObject = abstract_cast<ObjectClass*>(pAttacker)) {
				if(pThis->Owner->IsAlliedWith(pObject)) {
					return 0x6EB47A;
				}
			}

			if(auto pAttackerFoot = abstract_cast<FootClass*>(pAttacker)) {
				if(pAttackerFoot->InLimbo || pAttackerFoot->GetTechnoType()->ConsideredAircraft) {
					return 0x6EB47A;
				}
			}

			pThis->Focus = pAttacker;

			// this is the original code, but commented out because it's responsible for switching
			// targets when the team is attacked by two or more opponents. Now, the team should pick
			// the first target, and keep it. -AlexB
			//for(NextTeamMember i(pThis->FirstUnit); i; ++i) {
			//	if(i->IsAlive && i->Health && (Unsorted::ScenarioInit || !i->InLimbo)) {
			//		if(i->IsTeamLeader || i->WhatAmI() == AircraftClass::AbsID) {
			//			i->SetTarget(nullptr);
			//			i->SetDestination(nullptr, true);
			//		}
			//	}
			//}
		}
	}

	return 0x6EB47A;
}

// the team script actions handled in Ares
DEFINE_HOOK(0x6E9443, TeamClass_Update, 0x8)
{
	GET(TeamClass* const, pTeam, ESI);
	GET(ScriptActionNode* const, pAction, EAX);

	GET_STACK(bool const, flag, 0x10);

	return ScriptExt::Handled(pTeam, pAction, flag) ? 0x6E95ABu : 0u;
}

// the script action's parameter is added to the distance
DEFINE_HOOK(0x6EF8A1, TeamClass_GatherAtEnemyBase_Distance, 0x6)
{
	GET_BASE(ScriptActionNode* const, pAction, 0x8);

	R->EDX(RulesClass::Instance->AISafeDistance + pAction->Argument);
	return 0x6EF8A7;
}

// gathering at the own base has its own distance
DEFINE_HOOK(0x6EFB69, TeamClass_GatherAtFriendlyBase_Distance, 0x6)
{
	GET_BASE(ScriptActionNode* const, pAction, 0x8);

	auto const distance = RulesExt::Global()->AIFriendlyDistance.Get(
		RulesClass::Instance->AISafeDistance);

	R->EDX(distance + pAction->Argument);
	return 0x6EFB6F;
}

DEFINE_HOOK(0x6EFC70, TeamClass_IronCurtain, 0x5)
{
	GET(TeamClass* const, pTeam, ECX);

	GET_STACK(ScriptActionNode* const, pAction, 0x4);
	GET_STACK(bool const, flag, 0x8);

	TeamExt::FireIronCurtain(pTeam, pAction, flag);
	return 0x6EFE4F;
}

// the plane paradropping reinforcements is the owner's
DEFINE_HOOK(0x65DBB3, TeamTypeClass_CreateInstance_Plane, 0x5)
{
	GET(TechnoClass* const, pTechno, EBP);

	auto const pExt = HouseTypeExt::ExtMap.Find(pTechno->Owner->Type);
	R->ECX(pExt->GetParadropPlane());

	++Unsorted::ScenarioInit;
	return 0x65DBD0;
}
