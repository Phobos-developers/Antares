#include "Body.h"

#include "../SWType/Body.h"

#include <CellClass.h>
#include <FootClass.h>
#include <HouseClass.h>
#include <SuperClass.h>
#include <SuperWeaponTypeClass.h>
#include <TeamClass.h>

// Fires an Iron Curtain super weapon of the group the script action names.
/*!
	Fires the first fully charged super weapon that uses the Iron Curtain AI
	targeting mode and belongs to the group. If none is ready, but one is close
	enough to being ready, the team waits for it. Otherwise the script step is
	completed without doing anything.

	\date 2022-02-04
*/
void TeamExt::FireIronCurtain(
	TeamClass* const pTeam, ScriptActionNode* const pAction, bool const flag)
{
	auto const pLeader = pTeam->FetchALeader();

	if(!pLeader) {
		pTeam->StepCompleted = true;
		return;
	}

	auto const pOwner = pLeader->Owner;
	auto const havePower = !pOwner->PowerDrain
		|| (pOwner->PowerOutput >= pOwner->PowerDrain);

	SuperWeaponTypeClass* pBest = nullptr;
	auto found = false;

	for(auto const pSWType : SuperWeaponTypeClass::Array) {
		auto const pTypeExt = SWTypeExt::ExtMap.Find(pSWType);

		if(pTypeExt->SW_AITargetingType != SuperWeaponAITargetingMode::IronCurtain
			|| pTypeExt->SW_Group != pAction->Argument)
		{
			continue;
		}

		auto const pSuper = pOwner->Supers.GetItem(pSWType->ArrayIndex);

		if(pSuper->IsReady && (havePower || !pSWType->IsPowered)) {
			pBest = pSWType;
			found = true;
			break;
		}

		if(pBest || !pSuper->IsPresent) {
			continue;
		}

		auto const total = pSuper->GetRechargeTime();
		auto const charged = static_cast<double>(
			total - pSuper->RechargeTimer.GetTimeLeft());

		if(charged >= RulesClass::Instance->AIMinorSuperReadyPercent
			* static_cast<double>(total))
		{
			pBest = pSWType;
		}
	}

	if(found) {
		auto const coords = pTeam->SpawnCell->GetCoords();
		CellStruct const cell = {
			static_cast<short>(coords.X / 256), static_cast<short>(coords.Y / 256) };

		pOwner->Fire_SW(pBest->ArrayIndex, cell);
		pTeam->StepCompleted = true;

	} else if(!pBest) {
		pTeam->StepCompleted = true;
	}
}
