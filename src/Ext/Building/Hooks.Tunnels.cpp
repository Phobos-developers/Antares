#include "Body.h"
#include "../../Utilities/DirMath.h"
#include "../BuildingType/Body.h"
#include "../House/Body.h"
#include "../TechnoType/Body.h"
#include "../../Enum/TunnelTypes.h"

#include <BuildingClass.h>
#include <CaptureManagerClass.h>
#include <CellClass.h>
#include <CellSpread.h>
#include <FootClass.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <MapClass.h>
#include <TeamClass.h>
#include <TemporalClass.h>
#include <UnitClass.h>
#include <VocClass.h>

namespace {
	// not yet declared in YRpp
	using NearByLocationFunc = CellStruct* (__thiscall*)(
		MapClass*, CellStruct*, CellStruct const*, int, int, int, int, int, int,
		int, int, int, int, CellStruct*, int, int);

	HouseExt::TunnelData* GetTunnel(BuildingClass* const pBuilding) {
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);
		auto const index = static_cast<size_t>(pTypeExt->Tunnel);

		if(index >= TunnelTypeClass::Array.size()) {
			return nullptr;
		}

		return HouseExt::ExtMap.Find(pBuilding->Owner)->FindTunnel(index);
	}

	bool IsTunnel(BuildingTypeClass* const pType) {
		return BuildingTypeExt::ExtMap.Find(pType)->Tunnel >= 0;
	}

	// whether no other undamaged building of this house shares the tunnel network
	bool IsLastTunnel(BuildingClass* const pBuilding) {
		auto const index = BuildingTypeExt::ExtMap.Find(pBuilding->Type)->Tunnel;

		for(auto const pOther : pBuilding->Owner->Buildings) {
			if(pOther->Health > 0 && !pOther->InLimbo && pOther->IsOnMap
				&& pOther->CurrentMission != Mission::Construction
				&& pOther->CurrentMission != Mission::Selling
				&& pOther != pBuilding
				&& BuildingTypeExt::ExtMap.Find(pOther->Type)->Tunnel == index)
			{
				return false;
			}
		}

		return true;
	}

	void EnterTunnel(HouseExt::TunnelData* const pTunnel,
		BuildingClass* const pBuilding, FootClass* const pPassenger)
	{
		pPassenger->SetTarget(nullptr);
		pPassenger->OnBridge = false;
		pPassenger->MissionAccumulateTime = 0;
		pPassenger->GattlingValue = 0;
		pPassenger->SetCurrentWeaponStage(0);

		if(auto const pController = pPassenger->MindControlledBy) {
			if(auto const pManager = pController->CaptureManager) {
				pManager->FreeUnit(pPassenger);
			}
		}

		if(auto const sound = pBuilding->Type->EnterTransportSound; sound >= 0) {
			VocClass::PlayAt(sound, pBuilding->GetCoords());
		}

		pPassenger->Limbo();
		pPassenger->Undiscover();

		if(pPassenger->WhatAmI() == AbstractType::Infantry) {
			pPassenger->AbortMotion();
		}

		pTunnel->Passengers.push_back(pPassenger);
	}

	bool TryEnterTunnel(HouseExt::TunnelData* const pTunnel,
		BuildingClass* const pBuilding, FootClass* const pPassenger)
	{
		if(pPassenger->SendCommand(RadioCommand::QueryCanEnter, pBuilding)
			!= RadioCommand::AnswerPositive)
		{
			return false;
		}

		EnterTunnel(pTunnel, pBuilding, pPassenger);
		return true;
	}

	void KillPassengers(HouseExt::TunnelData* const pTunnel,
		BuildingClass* const pBuilding, TechnoClass* const pKiller)
	{
		if(pTunnel->Passengers.empty() || !IsLastTunnel(pBuilding)) {
			return;
		}

		while(!pTunnel->Passengers.empty()) {
			auto const pPassenger = pTunnel->Passengers.back();
			pTunnel->Passengers.pop_back();

			if(auto const pTeam = pPassenger->Team) {
				pTeam->LiberateMember(pPassenger);
			}

			pPassenger->RegisterDestruction(pKiller);
			pPassenger->UnInit();
		}
	}

	void UnlimboPassenger(HouseExt::TunnelData* const pTunnel,
		BuildingClass* const pBuilding)
	{
		auto const pPassenger = pTunnel->Passengers.back();
		pPassenger->OnBridge = pBuilding->OnBridge;

		auto const coords = pBuilding->GetCell()->GetCoords();
		auto const facing = pBuilding->PrimaryFacing.Current();

		++Unsorted::ScenarioInit;
		pPassenger->Unlimbo(coords, static_cast<DirType>(facing.GetFacing<256>()));
		--Unsorted::ScenarioInit;

		pPassenger->Scatter(CoordStruct::Empty, true, false);

		pTunnel->Passengers.pop_back();
	}

	bool UnloadPassenger(HouseExt::TunnelData* const pTunnel,
		BuildingClass* const pBuilding)
	{
		if(pTunnel->Passengers.empty()) {
			return false;
		}

		auto const pPassenger = pTunnel->Passengers.back();

		auto const away = AresDir::Add(pBuilding->PrimaryFacing.Current(), DirStruct(0x8000));
		auto const start = static_cast<unsigned int>(away.GetFacing<8>());

		auto const origin = pBuilding->GetMapCoords();
		auto const level = pBuilding->GetCellLevel();

		bool bothClear = true;
		bool found = false;
		unsigned int facing = 0;
		CellStruct target = CellStruct::Empty;
		CellClass* pNear = nullptr;
		CellClass* pFar = nullptr;

		for(int step = 0; step < 16; ++step) {
			facing = (start + static_cast<unsigned int>(step)) & 7u;

			auto const& offset = CellSpread::GetNeighbourOffset(facing);
			target = origin + offset;

			pNear = MapClass::Instance.GetCellAt(target);
			pFar = MapClass::Instance.GetCellAt(target + offset);

			auto const nearClear = pPassenger->IsCellOccupied(
				pNear, static_cast<FacingType>(facing), level, nullptr, true) == Move::OK;
			auto const farClear = pPassenger->IsCellOccupied(
				pFar, static_cast<FacingType>(facing), level, nullptr, true) == Move::OK;

			if(nearClear && (!bothClear || farClear) && !static_cast<bool>(pNear->Flags & CellFlags::BridgeHead)) {
				found = true;
				break;
			}

			if(step == 7) {
				bothClear = false;
			}
		}

		if(!found) {
			return false;
		}

		++Unsorted::ScenarioInit;

		CoordStruct coords {
			(target.X << 8) + 128,
			(target.Y << 8) + 128,
			0 };

		if(pPassenger->WhatAmI() == AbstractType::Infantry) {
			coords = MapClass::PickInfantrySublocation(coords, false);
		} else {
			auto const pType = pPassenger->GetTechnoType();

			auto const NearByLocation =
				reinterpret_cast<NearByLocationFunc>(0x56DC20);

			CellStruct spot;
			CellStruct nearby = CellStruct::Empty;
			NearByLocation(&MapClass::Instance, &spot, &target,
				static_cast<int>(pType->SpeedType), -1, 0, 0, 1, 1, 0, 0, 0, 1,
				&nearby, 0, 0);

			coords.X = (spot.X << 8) + 128;
			coords.Y = (spot.Y << 8) + 128;
		}

		pPassenger->Unlimbo(coords, static_cast<DirType>((facing & 0x3FFFFFFu) << 5));

		--Unsorted::ScenarioInit;

		if(auto const sound = pBuilding->Type->LeaveTransportSound; sound >= 0) {
			VocClass::PlayAt(sound, pBuilding->GetCoords());
		}

		pPassenger->QueueMission(Mission::Move, false);
		pPassenger->SetDestination(bothClear ? pFar : pNear, true);

		pTunnel->Passengers.pop_back();
		return true;
	}
}

DEFINE_HOOK(0x43C326, BuildingClass_ReceivedRadioCommand_QueryCanEnter_Tunnel, 0xA)
{
	GET(TechnoClass* const, pSender, EDI);
	GET(BuildingClass* const, pThis, ESI);

	enum {
		Deny = 0x43C3F0u,
		VanillaChecks = 0x43C4F8u,
		Accept = 0x43C535u
	};

	auto const fallBack = [R, pThis]() {
		R->EBX(pThis->Type);
		return static_cast<DWORD>(VanillaChecks);
	};

	auto const mission = pThis->GetCurrentMission();

	if(!pThis->BState
		|| mission == Mission::Construction
		|| mission == Mission::Selling)
	{
		return Deny;
	}

	auto const pType = pThis->Type;
	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);
	auto const pSenderType = pSender->GetTechnoType();

	// AmphibiousCrusher and AmphibiousDestroyer are amphibious too. Testing only
	// MovementZone::Amphibious sends them through the Naval mismatch below, which
	// is what keeps them out of water structures.
	auto const zone = pSenderType->MovementZone;
	bool const amphibious = zone == MovementZone::Amphibious
		|| zone == MovementZone::AmphibiousCrusher
		|| zone == MovementZone::AmphibiousDestroyer;

	if(!amphibious && pType->Naval != pSenderType->Naval) {
		return Deny;
	}

	if(pSenderType->BalloonHover || !pThis->HasPower) {
		return Deny;
	}

	auto const pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	auto const allowed = (pTechnoTypeExt->PassengersWhitelist.empty()
			|| pTechnoTypeExt->PassengersWhitelist.Contains(pSenderType))
		&& !pTechnoTypeExt->PassengersBlacklist.Contains(pSenderType);

	if(!allowed) {
		return Deny;
	}

	bool const absorbs = pType->UnitAbsorb || pType->InfantryAbsorb;
	bool const tunnel = pTypeExt->Tunnel >= 0;

	if(!absorbs && !tunnel) {
		if(!pThis->HasFreeLink(pSender) && !Unsorted::ScenarioInit) {
			return Deny;
		}

		return fallBack();
	}

	if(auto const pManager = pSender->CaptureManager) {
		if(pManager->IsControllingSomething()) {
			return Deny;
		}

		if(tunnel && pSender->IsMindControlled()) {
			return Deny;
		}
	}

	if(tunnel) {
		auto const pTunnel = GetTunnel(pThis);
		auto const count = static_cast<unsigned int>(pTunnel->Passengers.size()) + 1;

		// MaxCap is -1 for unlimited, hence the unsigned comparison
		if(count > static_cast<unsigned int>(pTunnel->MaxCap)) {
			return fallBack();
		}
	} else {
		auto const what = pSender->WhatAmI();

		if((what == AbstractType::Unit && !pType->UnitAbsorb)
			|| (what == AbstractType::Infantry && !pType->InfantryAbsorb))
		{
			return Deny;
		}

		if(pThis->Passengers.NumPassengers >= pType->Passengers) {
			return fallBack();
		}
	}

	if(pType->SizeLimit < pSenderType->Size) {
		return fallBack();
	}

	return Accept;
}

DEFINE_HOOK(0x43C716, BuildingClass_ReceivedRadioCommand_RequestCompleteEnter_Tunnel, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	return IsTunnel(pThis->Type) ? 0x43CCF2u : 0u;
}

DEFINE_HOOK(0x442DF2, BuildingClass_Demolish_Tunnel, 0x6)
{
	GET(BuildingClass* const, pThis, EDI);
	GET_STACK(TechnoClass*, pKiller, 0x90);

	if(auto const pTunnel = GetTunnel(pThis)) {
		if(!pKiller || (pKiller->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) {
			pKiller = nullptr;
		}

		KillPassengers(pTunnel, pThis, pKiller);
	}

	return 0;
}

DEFINE_HOOK(0x44351A, BuildingClass_ActionOnObject_Tunnel, 0x6)
{
	GET(BuildingClass* const, pThis, EBX);

	auto const pType = pThis->Type;
	bool const unload = pType->UnitAbsorb || pType->InfantryAbsorb || IsTunnel(pType);

	return unload ? 0x443534u : 0x443545u;
}

DEFINE_HOOK(0x44731C, BuildingClass_GetActionOnObject_Tunnel, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	auto const pTunnel = GetTunnel(pThis);

	return (pTunnel && !pTunnel->Passengers.empty()) ? 0x4472E7u : 0u;
}

DEFINE_HOOK(0x44A37F, BuildingClass_Mi_Selling_Tunnel, 0x6)
{
	GET(BuildingClass* const, pThis, EBP);

	if(auto const pTunnel = GetTunnel(pThis)) {
		if(IsLastTunnel(pThis)) {
			while(!pTunnel->Passengers.empty()) {
				UnlimboPassenger(pTunnel, pThis);
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x44D8A7, BuildingClass_Mi_Unload_Tunnel, 0x6)
{
	GET(BuildingClass* const, pThis, EBP);

	auto const pTunnel = GetTunnel(pThis);

	if(!pTunnel || pTunnel->Passengers.empty()) {
		return 0;
	}

	UnloadPassenger(pTunnel, pThis);

	return 0x44DC84;
}

DEFINE_HOOK(0x51A2AD, InfantryClass_UpdatePosition_Tunnel, 0x9)
{
	GET(InfantryClass* const, pThis, ESI);
	GET(BuildingClass* const, pBuilding, EDI);

	auto const pTunnel = GetTunnel(pBuilding);

	if(!pTunnel) {
		return 0;
	}

	return TryEnterTunnel(pTunnel, pBuilding, pThis) ? 0x51A396u : 0x51A488u;
}

DEFINE_HOOK(0x71A995, TemporalClass_Update_Tunnel, 0x5)
{
	GET(BuildingClass* const, pThis, EBP);
	GET(TemporalClass* const, pTemporal, ESI);

	if(auto const pTunnel = GetTunnel(pThis)) {
		KillPassengers(pTunnel, pThis, pTemporal->Owner);
	}

	return 0;
}

DEFINE_HOOK(0x73A23F, UnitClass_UpdatePosition_Tunnel, 0x6)
{
	GET(UnitClass* const, pThis, EBP);
	GET(BuildingClass* const, pBuilding, EBX);

	if(pThis->GetCurrentMission() != Mission::Enter
		|| pThis->Destination != pBuilding)
	{
		return 0;
	}

	auto const pTunnel = GetTunnel(pBuilding);

	if(!pTunnel) {
		return 0;
	}

	return TryEnterTunnel(pTunnel, pBuilding, pThis) ? 0x73A315u : 0x73A796u;
}

DEFINE_HOOK(0x73F606, UnitClass_IsCellOccupied_Tunnel, 0x6)
{
	GET(BuildingClass* const, pBuilding, ESI);

	auto const pType = pBuilding->Type;

	return (pType->UnitAbsorb || IsTunnel(pType)) ? 0x73F616u : 0x73F628u;
}

DEFINE_HOOK(0x741CE5, UnitClass_SetDestination_Tunnel, 0x6)
{
	GET(BuildingClass* const, pBuilding, ESI);

	auto const pType = pBuilding->Type;

	return (pType->UnitAbsorb || IsTunnel(pType)) ? 0x741CF5u : 0x741D12u;
}
