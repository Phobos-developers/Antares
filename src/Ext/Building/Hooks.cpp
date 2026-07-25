#include "Body.h"
#include "../BuildingType/Body.h"
#include "../HouseType/Body.h"
#include "../Rules/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"
#include "../../Misc/Network.h"

#include <SpecificStructures.h>
#include <ScenarioClass.h>
#include <InfantryClass.h>
#include <CellClass.h>
#include <HouseClass.h>
#include <VoxClass.h>
#include <MessageListClass.h>

#include <cmath>

/* #754 - evict Hospital/Armory contents */

// one more hook at 448277

// one more hook at 447113

// running out of money pauses repairing for human players instead of
// switching repair mode off outright
DEFINE_HOOK(0x4509B4, BuildingClass_UpdateRepair_Funds, 0x7)
{
	GET(BuildingClass* const, pThis, ESI);

	return (pThis->Owner->IsControlledByHuman()
			&& !RulesExt::Global()->RepairStopOnInsufficientFunds)
		? 0x4509BBu
		: 0u
	;
}

DEFINE_HOOK(0x44D8A1, BuildingClass_UnloadPassengers_Unload, 0x6)
{
	GET(BuildingClass *, B, EBP);

	BuildingExt::KickOutHospitalArmory(B);
	return 0;
}

// for yet unestablished reasons a unit might not be present.
// maybe something triggered the KickOutHospitalArmory
DEFINE_HOOK(0x44BB1B, BuildingClass_Mi_Repair_Promote, 0x6)
{
	//GET(BuildingClass*, pThis, EBP);
	GET(TechnoClass*, pTrainee, EAX);

	return pTrainee ? 0 : 0x44BB3C;
}

/* 	#218 - specific occupiers -- see Hooks.Trenches.cpp */

// EMP'd power plants don't produce power
DEFINE_HOOK(0x44E855, BuildingClass_PowerProduced_EMP, 0x6) {
	GET(BuildingClass*, pBld, ESI);
	return ((pBld->EMPLockRemaining > 0) ? 0x44E873 : 0);
}

// restore pip count for tiberium storage (building and house)
DEFINE_HOOK(0x44D755, BuildingClass_GetPipFillLevel_Tiberium, 0x6)
{
	GET(BuildingClass*, pThis, ECX);
	GET(BuildingTypeClass*, pType, ESI);

	double amount = 0.0;
	if(pType->Storage > 0) {
		amount = pThis->Tiberium.GetTotalAmount() / pType->Storage;
	} else {
		amount = pThis->Owner->GetStoragePercentage();
	}

	int ret = Game::F2I(pType->GetPipMax() * amount);
	R->EAX(ret);
	return 0x44D750;
}

// the game specifically hides tiberium building pips. allow them, but
// take care they don't show up for the original game
DEFINE_HOOK(0x709B4E, TechnoClass_DrawPipscale_SkipSkipTiberium, 0x6)
{
	GET(TechnoClass*, pThis, EBP);

	bool showTiberium = true;
	if(auto pType = specific_cast<BuildingTypeClass*>(pThis->GetTechnoType())) {
		if((pType->Refinery || pType->ResourceDestination) && pType->Storage > 0) {
			// show only if this refinery uses storage. otherwise, the original
			// refineries would show an unused tiberium pip scale
			auto pExt = TechnoTypeExt::ExtMap.Find(pType);
			showTiberium = pExt->Refinery_UseStorage;
		}
	}

	return showTiberium ? 0x709B6E : 0x70A980;
}

// also consider NeedsEngineer when activating animations
// if the status changes, animations might start to play that aren't
// supposed to play because the building requires an Engineer which
// didn't capture the building yet.
DEFINE_HOOK(0x4467D6, BuildingClass_Place_NeedsEngineer, 0x6)
{
	GET(BuildingClass*, pThis, EBP);
	R->AL(pThis->Type->Powered || (pThis->Type->NeedsEngineer && !pThis->HasEngineer));
	return 0x4467DC;
}

DEFINE_HOOK(0x454BF7, BuildingClass_UpdatePowered_NeedsEngineer, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	R->CL(pThis->Type->Powered || (pThis->Type->NeedsEngineer && !pThis->HasEngineer));
	return 0x454BFD;
}

DEFINE_HOOK(0x451A54, BuildingClass_PlayAnim_NeedsEngineer, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	R->CL(pThis->Type->Powered || (pThis->Type->NeedsEngineer && !pThis->HasEngineer));
	return 0x451A5A;
}

// infantry exiting hospital get their focus reset, but not for armory
DEFINE_HOOK(0x444D26, BuildingClass_KickOutUnit_ArmoryExitBug, 0x6)
{
	GET(BuildingTypeClass*, pType, EDX);
	R->AL(pType->Hospital || pType->Armory);
	return 0x444D2C;
}

// do not crash if the EMP cannon primary has no Report sound
DEFINE_HOOK(0x44D4CA, BuildingClass_Mi_Missile_NoReport, 0x9)
{
	GET(TechnoTypeClass*, pType, EAX);
	GET(WeaponTypeClass*, pWeapon, EBP);

	bool play = !pType->IsGattling && pWeapon->Report.Count;
	return play ? 0x44D4D4 : 0x44D51F;
}

DEFINE_HOOK(0x44840B, BuildingClass_ChangeOwnership_Tech, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(HouseClass*, pNewOwner, EBX);

	if(pThis->Owner != pNewOwner) {
		const auto pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

		auto PrintMessage = [](const CSFText& text) {
			if(!text.empty()) {
				auto color = HouseClass::CurrentPlayer->ColorSchemeIndex;
				MessageListClass::Instance.PrintMessage(text, RulesClass::Instance->MessageDelay, color);
			}
		};

		if(pThis->Owner->IsControlledByCurrentPlayer()) {
			VoxClass::PlayIndex(pExt->LostEvaEvent);
			PrintMessage(pExt->MessageLost);
		}
		if(pNewOwner->IsControlledByCurrentPlayer()) {
			VoxClass::PlayIndex(pThis->Type->CaptureEvaEvent);
			PrintMessage(pExt->MessageCapture);
		}
	}

	return 0x44848F;
}

// support oil derrick logic on building upgrades
DEFINE_HOOK(0x4409F4, BuildingClass_Put_ProduceCash, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(BuildingClass*, pToUpgrade, EDI);

	auto pExt = BuildingExt::ExtMap.Find(pToUpgrade);

	if(auto delay = pThis->Type->ProduceCashDelay) {
		pExt->CashUpgradeTimers[pToUpgrade->UpgradeLevel - 1].Start(delay);
	}

	// an upgrade with FactoryOwners.Permanent hands over its plans too
	BuildingExt::UpdateFactoryPlans(pThis);

	return 0;
}

DEFINE_HOOK(0x43FD2C, BuildingClass_Update_ProduceCash, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	auto pExt = BuildingExt::ExtMap.Find(pThis);

	// The building and its upgrades are ticked into one sum, then paid out once.
	// Restarting the timer is decoupled from paying: a timer that has already run
	// out (left == 0) is restarted silently, only the frame it reaches 1 pays.
	int total = 0;

	auto Process = [&total](BuildingTypeClass* pType, CDTimerClass& timer) {
		auto const delay = pType->ProduceCashDelay;
		if(delay > 0) {
			auto const left = timer.GetTimeLeft();
			if(left <= 1) {
				timer.Start(delay + 1);
				if(left == 1) {
					total += pType->ProduceCashAmount;
				}
			}
		}
	};

	Process(pThis->Type, pThis->CashProductionTimer);

	for(size_t i = 0; i < 3; ++i) {
		if(const auto& pUpgrade = pThis->Upgrades[i]) {
			Process(pUpgrade, pExt->CashUpgradeTimers[i]);
		}
	}

	if(total && !pThis->Owner->Type->MultiplayPassive && pThis->IsPowerOnline()) {
		pExt->ProduceCashDisplay(total);
		pThis->Owner->TransactMoney(total);
	}

	return 0x43FDD6;
}

DEFINE_HOOK(0x4482BD, BuildingClass_ChangeOwnership_ProduceCash, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(HouseClass*, pNewOwner, EBX);
	auto pExt = BuildingExt::ExtMap.Find(pThis);

	int total = 0;

	auto Process = [&total](BuildingTypeClass* pType, CDTimerClass& timer) {
		// a type that only produces cash over time still gets its timer primed
		auto const startup = pType->ProduceCashStartup;
		if(startup || pType->ProduceCashAmount) {
			total += startup;
			if(auto const delay = pType->ProduceCashDelay) {
				timer.Start(delay + 1);
			}
		}
	};

	Process(pThis->Type, pThis->CashProductionTimer);

	for(size_t i = 0; i < 3; ++i) {
		if(const auto& pUpgrade = pThis->Upgrades[i]) {
			Process(pUpgrade, pExt->CashUpgradeTimers[i]);
		}
	}

	if(total && !pNewOwner->Type->MultiplayPassive) {
		pNewOwner->TransactMoney(total);
		pExt->ProduceCashDisplay(total);
	}

	return 0x4482FC;
}

// make temporal weapons play nice with power toggle.
// previously, power state was set to true unconditionally.
DEFINE_HOOK(0x452287, BuildingClass_GoOnline_TogglePower, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);
	auto const pExt = BuildingExt::ExtMap.Find(pThis);
	pExt->TogglePower_HasPower = true;
	return 0;
}

DEFINE_HOOK(0x452393, BuildingClass_GoOffline_TogglePower, 0x7)
{
	GET(BuildingClass* const, pThis, ESI);
	auto const pExt = BuildingExt::ExtMap.Find(pThis);
	pExt->TogglePower_HasPower = false;
	return 0;
}

DEFINE_HOOK(0x452210, BuildingClass_Enable_TogglePower, 0x7)
{
	GET(BuildingClass* const, pThis, ECX);
	auto const pExt = BuildingExt::ExtMap.Find(pThis);
	pThis->HasPower = pExt->TogglePower_HasPower;
	return 0x452217;
}

// replaces the UnitReload handling and makes each docker independent of all
// others. this means planes don't have to wait one more ReloadDelay because
// the first docker triggered repair mission while the other dockers arrive
// too late and need to be put to sleep first.
DEFINE_HOOK(0x44C844, BuildingClass_MissionRepair_Reload, 0x6)
{
	GET(BuildingClass* const, pThis, EBP);
	auto const pExt = BuildingExt::ExtMap.Find(pThis);

	// ensure there are enough slots
	pExt->DockReloadTimers.Reserve(pThis->RadioLinks.Capacity);

	// update all dockers, check if there's
	// at least one needing more attention
	bool keep_reloading = false;
	for(auto i = 0; i < pThis->RadioLinks.Capacity; ++i) {
		if(auto const pLink = pThis->GetNthLink(i)) {

			auto const SendCommand = [=](RadioCommand command) {
				auto const response = pThis->SendCommand(command, pLink);
				return response == RadioCommand::AnswerPositive;
			};

			// check if reloaded and repaired already
			auto const pLinkType = pLink->GetTechnoType();
			auto done = SendCommand(RadioCommand::QueryReadiness)
				&& pLink->Health == pLinkType->Strength;

			if(!done) {
				// check if docked
				auto const miss = pLink->GetCurrentMission();
				if(miss == Mission::Enter 
					|| !SendCommand(RadioCommand::QueryMoving))
				{
					continue;
				}

				keep_reloading = true;

				// make the unit sleep first
				if(miss != Mission::Sleep) {
					pLink->QueueMission(Mission::Sleep, false);
					continue;
				}

				// check whether the timer completed
				auto const last_timer = pExt->DockReloadTimers[i];
				if(last_timer > Unsorted::CurrentFrame) {
					continue;
				}

				// set the next frame
				auto const pLinkExt = TechnoTypeExt::ExtMap.Find(pLinkType);
				auto const defaultRate = RulesClass::Instance->ReloadRate;
				auto const rate = pLinkExt->ReloadRate.Get(defaultRate);
				auto const frames = static_cast<int>(rate * 900);
				pExt->DockReloadTimers[i] = Unsorted::CurrentFrame + frames;

				// only reload if the timer was not outdated
				if(last_timer != Unsorted::CurrentFrame) {
					continue;
				}

				// reload and repair, return true if both failed
				done = !SendCommand(RadioCommand::RequestReload)
					&& !SendCommand(RadioCommand::RequestRepair);
			}

			if(done) {
				pLink->EnterIdleMode(0, 1);
				pLink->ForceMission(Mission::Guard);
				pLink->ProceedToNextPlanningWaypoint();

				pExt->DockReloadTimers[i] = -1;
			}
		}
	}

	if(keep_reloading) {
		// update each frame
		R->EAX(1);
	} else {
		pThis->QueueMission(Mission::Guard, false);
		R->EAX(3);
	}

	return 0x44C968;
}

DEFINE_HOOK(0x44D760, BuildingClass_Destroyed_UnitLost, 0x7)
{
	GET(BuildingClass* const, pThis, ECX);

	auto const pType = pThis->Type;

	// exclude unimportant buildings, and only play for current player
	if(!pType->DontScore && !pType->Insignificant && pThis->Owner->IsControlledByCurrentPlayer()) {
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
		auto const idx = pTypeExt->EVA_UnitLost;

		if(idx >= 0 && !TechnoExt::ExtMap.Find(pThis)->SuppressLossMessage) {
			pThis->GetMapCoords();
			VoxClass::PlayIndex(idx, -1, -1);
		}
	}

	return 0;
}

DEFINE_HOOK(0x440C08, BuildingClass_Put_AIBaseNormal, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	R->EAX(!BuildingExt::ExtMap.Find(pThis)->IsBaseNormal());
	return 0x440C2C;
}

DEFINE_HOOK(0x445A72, BuildingClass_Remove_AIBaseNormal, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	R->EAX(!BuildingExt::ExtMap.Find(pThis)->IsBaseNormal());
	return 0x445A94;
}

DEFINE_HOOK(0x456370, BuildingClass_UnmarkBaseSpace_AIBaseNormal, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	R->EAX(!BuildingExt::ExtMap.Find(pThis)->IsBaseNormal());
	return 0x456394;
}

DEFINE_HOOK(0x4A8FF5, MapClass_CanBuildingTypeBePlacedHere_Ignore, 0x5)
{
	GET(BuildingClass* const, pThis, ESI);

	return BuildingExt::ExtMap.Find(pThis)->SkipBaseNormal ? 0x4A8FFA : 0;
}
