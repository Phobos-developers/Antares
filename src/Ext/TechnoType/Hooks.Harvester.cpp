#include "Body.h"

#include "../../Utilities/TemplateDef.h"

#include <InfantryClass.h>
#include <SlaveManagerClass.h>
#include <UnitClass.h>

static int GetLongScan(TechnoTypeClass* pType, int fallback)
{
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	return pExt->Harvester_LongScan.Get(Leptons(fallback));
}

static int GetShortScan(TechnoTypeClass* pType, int fallback)
{
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	return pExt->Harvester_ShortScan.Get(Leptons(fallback));
}

static int GetScanCorrection(TechnoTypeClass* pType, int fallback)
{
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	return pExt->Harvester_ScanCorrection.Get(Leptons(fallback));
}

static int GetTooFarDistance(TechnoTypeClass* pType, int fallback)
{
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	return pExt->Harvester_TooFarDistance.Get(fallback);
}

// a negative delay never wakes the slave miner up
static bool CanKick(TechnoTypeClass* pType, int lastFrame)
{
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	auto const delay = pExt->Harvester_KickDelay.Get(
		RulesClass::Instance->SlaveMinerKickFrameDelay);

	return delay >= 0 && delay + lastFrame < Unsorted::CurrentFrame;
}

DEFINE_HOOK(0x6AF748, SlaveManagerClass_UpdateSlaves_SlaveScan, 0x6)
{
	GET(InfantryClass* const, pSlave, ESI);

	R->EAX(GetShortScan(
		pSlave->Type, RulesClass::Instance->SlaveMinerSlaveScan));

	return 0x6AF74E;
}

DEFINE_HOOK_AGAIN(0x6AFDFC, SlaveManagerClass_UpdateMiner_LongScan, 0x6)
DEFINE_HOOK_AGAIN(0x6B00BD, SlaveManagerClass_UpdateMiner_LongScan, 0x6)
DEFINE_HOOK(0x6B02CC, SlaveManagerClass_UpdateMiner_LongScan, 0x6)
{
	GET(TechnoClass* const, pOwner, ECX);

	R->EAX(GetLongScan(
		pOwner->GetTechnoType(), RulesClass::Instance->SlaveMinerLongScan));

	return R->Origin() + 6;
}

DEFINE_HOOK_AGAIN(0x6B006D, SlaveManagerClass_UpdateMiner_ShortScan, 0x6)
DEFINE_HOOK(0x6B026C, SlaveManagerClass_UpdateMiner_ShortScan, 0x6)
{
	GET(TechnoClass* const, pOwner, ECX);

	R->EAX(GetShortScan(
		pOwner->GetTechnoType(), RulesClass::Instance->SlaveMinerShortScan));

	return R->Origin() + 6;
}

DEFINE_HOOK(0x6B01A3, SlaveManagerClass_UpdateMiner_ScanCorrection, 0x6)
{
	GET(SlaveManagerClass* const, pThis, ESI);

	R->EAX(GetScanCorrection(pThis->Owner->GetTechnoType(),
		RulesClass::Instance->SlaveMinerScanCorrection));

	return 0x6B01A9;
}

DEFINE_HOOK(0x6B1065, SlaveManagerClass_ShouldWakeUp_ShortScan, 0x5)
{
	GET(SlaveManagerClass* const, pThis, ESI);

	auto const pType = pThis->Owner->GetTechnoType();

	if(!CanKick(pType, pThis->LastScanFrame)) {
		return 0x6B10C6;
	}

	R->EAX(GetShortScan(pType, RulesClass::Instance->SlaveMinerShortScan));

	return 0x6B1085;
}

// harvesters go looking for ore even without a refinery to unload at
DEFINE_HOOK(0x73E66D, UnitClass_Mi_Harvest_SkipDock, 0x6)
{
	return 0x73E6CF;
}

DEFINE_HOOK_AGAIN(0x73E772, UnitClass_Mi_Harvest_LongScan, 0x6)
DEFINE_HOOK(0x73E851, UnitClass_Mi_Harvest_LongScan, 0x6)
{
	GET(UnitClass* const, pThis, EBP);

	R->EAX(GetLongScan(pThis->Type, RulesClass::Instance->TiberiumLongScan));

	return R->Origin() + 6;
}

DEFINE_HOOK_AGAIN(0x73E9F1, UnitClass_Mi_Harvest_ShortScan, 0x6)
DEFINE_HOOK_AGAIN(0x73EA17, UnitClass_Mi_Harvest_ShortScan, 0x6)
DEFINE_HOOK_AGAIN(0x73EAA6, UnitClass_Mi_Harvest_ShortScan, 0x6)
DEFINE_HOOK(0x73EAC6, UnitClass_Mi_Harvest_ShortScan, 0x6)
{
	GET(UnitClass* const, pThis, EBP);

	R->EAX(GetShortScan(pThis->Type, RulesClass::Instance->TiberiumShortScan));

	return R->Origin() + 6;
}

DEFINE_HOOK(0x73EC0E, UnitClass_Mi_Harvest_TooFarDistance1, 0x6)
{
	GET(UnitClass* const, pThis, EBP);

	R->EDX(GetTooFarDistance(
		pThis->Type, RulesClass::Instance->HarvesterTooFarDistance));

	return 0x73EC14;
}

DEFINE_HOOK(0x73EE40, UnitClass_Mi_Harvest_TooFarDistance2, 0x6)
{
	GET(UnitClass* const, pThis, EBP);

	R->EDX(GetTooFarDistance(
		pThis->Type, RulesClass::Instance->ChronoHarvTooFarDistance));

	return 0x73EE46;
}

DEFINE_HOOK(0x74081F, UnitClass_Mi_Guard_KickFrameDelay, 0x5)
{
	GET(UnitClass* const, pThis, ESI);

	return CanKick(pThis->Type, pThis->CurrentMissionStartTime)
		? 0x74083Bu : 0x740854u;
}

DEFINE_HOOK(0x74410D, UnitClass_Mi_AreaGuard_KickFrameDelay, 0x5)
{
	GET(UnitClass* const, pThis, ESI);

	return CanKick(pThis->Type, pThis->CurrentMissionStartTime)
		? 0x744129u : 0x74416Cu;
}

static_assert(offsetof(InfantryClass, Type) == 0x6C0, "InfantryClass layout slipped");
static_assert(offsetof(UnitClass, Type) == 0x6C4, "UnitClass layout slipped");
static_assert(offsetof(MissionClass, CurrentMissionStartTime) == 0xC0, "MissionClass layout slipped");
static_assert(offsetof(SlaveManagerClass, Owner) == 0x24, "SlaveManagerClass layout slipped");
static_assert(offsetof(SlaveManagerClass, LastScanFrame) == 0x60, "SlaveManagerClass layout slipped");
