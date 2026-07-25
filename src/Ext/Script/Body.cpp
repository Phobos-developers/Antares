#include "Body.h"

#include "../House/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"

#include <FootClass.h>
#include <HouseClass.h>
#include <TeamClass.h>

#include <Helpers/Enumerators.h>

// Sends all team members off to enter something, be that a building or a
// vehicle, and releases everyone that found one.
/*!
	\date 2022-02-04
*/
void ScriptExt::TakeVehicles(
	TeamClass* const pTeam, ScriptActionNode* const pAction, bool const flag)
{
	auto const hijack = (pAction->Action
		!= static_cast<int>(ScriptAction::GarrisonStructure));

	for(NextTeamMember member(pTeam->FirstUnit); member; ++member) {
		TechnoExt::ExtMap.Find(*member)->TakeVehicleMode = hijack;

		if(member->GarrisonStructure()) {
			pTeam->LiberateMember(*member, -1, true);
		}
	}

	pTeam->StepCompleted = true;
}

// Handles the execution of team script actions.
/*!
	\returns True if this script action was handled by Ares, false otherwise.

	\date 2022-02-04
*/
bool ScriptExt::Handled(
	TeamClass* const pTeam, ScriptActionNode* const pAction, bool const flag)
{
	switch(static_cast<ScriptAction>(pAction->Action)) {
	case ScriptAction::GarrisonStructure:
	case ScriptAction::TakeVehicles:
		TakeVehicles(pTeam, pAction, flag);
		return true;

	case ScriptAction::AuxiliaryPower: {
		auto const pOwner = pTeam->Owner;

		HouseExt::ExtMap.Find(pOwner)->AuxPower += pAction->Argument;
		pOwner->RecheckPower = true;
		break;
	}

	case ScriptAction::KillDrivers: {
		auto const pTarget = HouseClass::FindSpecial();
		auto changed = false;

		// killing a driver can remove members from the team, so rescan until
		// nothing changes any more.
		do {
			changed = false;

			for(NextTeamMember member(pTeam->FirstUnit); member; ++member) {
				if(member->Health <= 0 || !member->IsAlive || !member->IsOnMap
					|| member->InLimbo)
				{
					continue;
				}

				auto const pExt = TechnoExt::ExtMap.Find(*member);

				if(pExt->DriverKilled || !pExt->IsDriverKillable(1.0)) {
					continue;
				}

				if(pExt->ApplyKillDriver(pTarget, nullptr, false)) {
					changed = true;
				}
			}
		} while(changed);
		break;
	}

	case ScriptAction::ConvertType:
		// Generalises the hardcoded TRUCKA/TRUCKB "Load Truck"/"Unload Truck"
		// pair. Members without Convert.Script are left alone. Unlike the
		// water/land conversion there is no "already this type" short-circuit.
		for(NextTeamMember member(pTeam->FirstUnit); member; ++member) {
			auto const pExt = TechnoTypeExt::ExtMap.Find(member->GetTechnoType());

			if(auto const pConvertTo = pExt->Convert_Script.Get()) {
				TechnoExt::UpdateType(*member, pConvertTo);
			}
		}
		break;

	case ScriptAction::SonarReveal:
	case ScriptAction::DisableWeapons: {
		auto const duration = pAction->Argument;
		auto const sonar = (pAction->Action
			== static_cast<int>(ScriptAction::SonarReveal));

		for(NextTeamMember member(pTeam->FirstUnit); member; ++member) {
			auto const pExt = TechnoExt::ExtMap.Find(*member);
			auto& timer = sonar ? pExt->CloakSkipTimer : pExt->DisableWeaponTimer;

			if(duration > 0) {
				// only ever extend the effect
				if(duration > timer.GetTimeLeft()) {
					timer.Start(duration);
				}
			} else if(duration == 0) {
				timer.Stop();
			}
		}
		break;
	}

	default:
		return false;
	}

	pTeam->StepCompleted = true;
	return true;
}
