#include <CellClass.h>
#include <FootClass.h>
#include <HouseClass.h>
#include <MapClass.h>
#include <RulesClass.h>
#include <UnitClass.h>

#include <AnimClass.h>
#include <DropPodLocomotionClass.h>
#include <HoverLocomotionClass.h>
#include <JumpjetLocomotionClass.h>
#include <LocomotionClass.h>
#include <TeleportLocomotionClass.h>
#include <TunnelLocomotionClass.h>
#include <VocClass.h>

#include "../Ext/Rules/Body.h"
#include "../Ext/TechnoType/Body.h"

namespace {
	DirStruct GetDeployDir(TechnoTypeClass* pType) {
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		DirStruct ret;
		ret.Raw = static_cast<unsigned short>(pTypeExt->DeployDir.isset()
			? pTypeExt->DeployDir * 0x2000
			: RulesClass::Instance->DeployDir * 0x100);
		return ret;
	}

	void PlayDigEffects(FootClass* pThis, bool digIn, bool createAnim) {
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

		auto const& sound = digIn ? pTypeExt->DigInSound : pTypeExt->DigOutSound;
		VocClass::PlayAt(sound.Get(RulesClass::Instance->DigSound), pThis->Location);

		if(createAnim) {
			auto const& anim = digIn ? pTypeExt->DigInAnim : pTypeExt->DigOutAnim;

			if(auto const pAnimType = anim.Get(RulesClass::Instance->Dig)) {
				GameCreate<AnimClass>(pAnimType, pThis->Location, 0, 1, 0x600u, 0, false);
			}
		}
	}
}

DEFINE_HOOK(0x4B619F, DropPodLocomotionClass_ILocomotion_MoveTo_AtmosphereEntry, 0x5)
{
	return RulesClass::Instance->AtmosphereEntry ? 0 : 0x4B61D6;
}

DEFINE_HOOK(0x513EAA, HoverLocomotionClass_UpdateHover_DeployToLand, 0x5)
{
	GET(HoverLocomotionClass* const, pThis, ESI);

	return pThis->LinkedTo->InAir ? 0x513ECD : 0;
}

DEFINE_HOOK(0x514A21, HoverLocomotionClass_ILocomotion_Process_DeployToLand, 0x9)
{
	GET(ILocomotion* const, pLoco, ESI);

	bool const isMoving = pLoco->Is_Moving_Now();
	R->AL(isMoving);

	auto const pLinkedTo = static_cast<LocomotionClass*>(pLoco)->LinkedTo;

	if(pLinkedTo->InAir) {
		auto const pType = pLinkedTo->GetTechnoType();

		if(pType->DeployToLand) {
			auto const land = pLinkedTo->GetCell()->LandType;

			if(land == LandType::Water || land == LandType::Beach) {
				pLinkedTo->InAir = false;
				pLinkedTo->QueueMission(Mission::Guard, true);
			}

			if(isMoving) {
				pLoco->Stop_Moving();
				pLinkedTo->SetDestination(nullptr, true);
			}

			if(pType->DeployingAnim) {
				auto const dir = GetDeployDir(pType);

				if(pLinkedTo->PrimaryFacing.Current() != dir) {
					pLinkedTo->PrimaryFacing.SetDesired(dir);
				}
			}

			if(pLinkedTo->GetHeight() <= 0) {
				pLinkedTo->InAir = false;
				pLoco->Mark_All_Occupation_Bits(MarkType::Up);
			}
		}
	}

	return 0x514A2A;
}

DEFINE_HOOK(0x514DFE, HoverLocomotionClass_ILocomotion_MoveTo_DeployToLand, 0x7)
{
	GET(ILocomotion* const, pLoco, ESI);

	auto const pLinkedTo = static_cast<LocomotionClass*>(pLoco)->LinkedTo;

	if(pLinkedTo->GetTechnoType()->DeployToLand) {
		pLinkedTo->InAir = false;
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x514F60, HoverLocomotionClass_ILocomotion_MoveTo, 0x7)
DEFINE_HOOK(0x514E97, HoverLocomotionClass_ILocomotion_MoveTo, 0x7)
{
	GET(ILocomotion* const, pLoco, ESI);

	auto const pLinkedTo = static_cast<LocomotionClass*>(pLoco)->LinkedTo;

	if(!pLinkedTo->Destination) {
		pLinkedTo->SetSpeedPercentage(0.0);
	}

	return 0;
}

DEFINE_HOOK(0x516305, HoverLocomotionClass_sub_515ED0, 0x9)
{
	GET(HoverLocomotionClass* const, pThis, ESI);

	pThis->sub_514F70(true);

	auto const pLinkedTo = pThis->LinkedTo;

	if(!pLinkedTo->Destination) {
		pLinkedTo->SetSpeedPercentage(0.0);
	}

	return 0x51630E;
}

DEFINE_HOOK(0x54C767, JumpjetLocomotionClass_State4_54C550_DeployDir, 0x6)
{
	GET(JumpjetLocomotionClass* const, pThis, ESI);

	auto const pType = pThis->LinkedTo->GetTechnoType();

	if(pType->DeployingAnim) {
		auto const dir = GetDeployDir(pType);

		if(pThis->LocomotionFacing.Current() != dir) {
			pThis->LocomotionFacing.SetDesired(dir);
		}
	}

	return 0x54C7A3;
}

DEFINE_HOOK(0x5F3FB2, ObjectClass_Update_MaxFallRate, 0x6)
{
	GET(ObjectClass* const, pThis, ESI);

	auto const pType = pThis->GetTechnoType();
	bool const parachuted = pType ? (pThis->Parachute != nullptr) : pThis->HasParachute;

	int rate = 1;
	int maxRate = parachuted
		? RulesClass::Instance->ParachuteMaxFallRate
		: RulesClass::Instance->NoParachuteMaxFallRate;

	if(pType) {
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		rate = parachuted
			? pTypeExt->FallRate_Parachute
			: pTypeExt->FallRate_NoParachute;

		auto const& maxOverride = parachuted
			? pTypeExt->FallRate_ParachuteMax
			: pTypeExt->FallRate_NoParachuteMax;

		if(maxOverride.isset()) {
			maxRate = maxOverride;
		}
	}

	int const fallRate = pThis->FallRate - rate;
	pThis->FallRate = (fallRate >= maxRate) ? fallRate : maxRate;

	return 0x5F3FFD;
}

DEFINE_HOOK(0x718275, TeleportLocomotionClass_MakeRoom, 0x9)
{
	GET(TeleportLocomotionClass* const, pThis, EBP);

	bool const crushInfantry = RulesExt::Global()->ChronoInfantryCrush;
	auto const pLinkedTo = pThis->LinkedTo;
	auto const pCoords = R->lea_Stack<CoordStruct*>(0x3C);
	auto const pCell = MapClass::Instance.GetCellAt(*pCoords);
	bool const linkedIsInfantry = (pLinkedTo->WhatAmI() == AbstractType::Infantry);

	R->Stack(0x48, false);
	R->EBX(pCell->OverlayTypeIndex);
	R->EDI(0);

	auto pObject = pCell->GetContent();

	while(pObject) {
		auto const pNext = pObject->NextObject;

		bool const isInfantry = (pObject->WhatAmI() == AbstractType::Infantry);

		bool blocking = pObject->IsIronCurtained();

		if(auto const pType = pObject->GetTechnoType()) {
			if(!TechnoTypeExt::ExtMap.Find(pType)->Chronoshift_Crushable) {
				blocking = true;
			}
		}

		ObjectClass* pVictim = nullptr;

		if(!crushInfantry && linkedIsInfantry && !isInfantry) {
			pVictim = pLinkedTo;
		} else if(!blocking && isInfantry && linkedIsInfantry) {
			if(pObject->GetCoords() == *pCoords) {
				pVictim = pObject;
			}
		} else if(blocking) {
			pVictim = pLinkedTo;
		} else if(generic_cast<FootClass*>(pObject)) {
			pVictim = pObject;
		} else {
			R->Stack(0x48, true);
		}

		if(pVictim) {
			int damage = pVictim->GetTechnoType()->Strength;
			pVictim->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead,
				nullptr, true, false, nullptr);
		}

		pObject = pNext;
	}

	if((pCell->Flags & (CellFlags::BridgeHead | CellFlags::Unknown_200)) == CellFlags::BridgeHead) {
		R->Stack(0x48, true);
	}

	R->Stack(0x20, pLinkedTo->GetCell());
	R->EAX(1);

	return 0x7184CE;
}

DEFINE_HOOK(0x739B8A, UnitClass_SimpleDeploy_Facing, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	if(!pThis->Type->DeployingAnim) {
		return 0;
	}

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	int const facing = pTypeExt->DeployDir.isset()
		? pTypeExt->DeployDir
		: ((static_cast<unsigned short>(RulesClass::Instance->DeployDir) >> 4) + 1) >> 1 & 7;

	int const current = ((pThis->PrimaryFacing.Current().Raw >> 12) + 1) >> 1 & 7;

	if(current != facing) {
		if(!pThis->Locomotor) {
			Game::RaiseError(E_POINTER);
		}

		if(!pThis->Locomotor->Is_Moving_Now()) {
			if(!pThis->Locomotor) {
				Game::RaiseError(E_POINTER);
			}

			DirStruct dir;
			dir.Raw = static_cast<unsigned short>(facing * 0x2000);
			pThis->Locomotor->Do_Turn(dir);
		}
	}

	return 0x739C70;
}

DEFINE_HOOK(0x728EF0, TunnelLocomotionClass_ILocomotion_Process_Dig, 0x5)
{
	GET(FootClass* const, pLinkedTo, EAX);

	PlayDigEffects(pLinkedTo, true, true);

	return 0x728F74;
}

DEFINE_HOOK(0x72920C, TunnelLocomotionClass_Turning, 0x9)
{
	GET(TunnelLocomotionClass* const, pThis, ESI);

	if(pThis->Coords != CoordStruct::Empty) {
		return 0;
	}

	pThis->State = TunnelLocomotionClass::DugOut;

	return 0x729369;
}

DEFINE_HOOK(0x7292CF, TunnelLocomotionClass_sub_7291F0_Dig, 0x8)
{
	GET(TunnelLocomotionClass* const, pThis, ESI);
	GET(int const, duration, EAX);

	pThis->DigTimer.StartTime = Unsorted::CurrentFrame;
	pThis->DigTimer.TimeLeft = duration;
	pThis->DigTimer.Rate = duration;

	PlayDigEffects(pThis->LinkedTo, true, true);

	return 0x729365;
}

DEFINE_HOOK(0x7293DA, TunnelLocomotionClass_sub_729370_Dig, 0x6)
{
	GET(FootClass* const, pLinkedTo, ECX);

	PlayDigEffects(pLinkedTo, true, true);

	return 0x72945E;
}

DEFINE_HOOK(0x7297C4, TunnelLocomotionClass_sub_729580_Dig, 0x6)
{
	GET(FootClass* const, pLinkedTo, EAX);

	PlayDigEffects(pLinkedTo, false, false);

	return 0x7297F3;
}

DEFINE_HOOK(0x7299A9, TunnelLocomotionClass_sub_7298F0_Dig, 0x5)
{
	GET(TunnelLocomotionClass* const, pThis, ESI);

	PlayDigEffects(pThis->LinkedTo, false, true);

	return 0x729A34;
}
