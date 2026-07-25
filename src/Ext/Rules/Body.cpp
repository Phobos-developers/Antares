#include "Body.h"
#include "../Side/Body.h"
#include "../HouseType/Body.h"
#include "../../Enum/Prerequisites.h"
#include "../../Enum/ArmorTypes.h"
#include "../../Enum/CursorTypes.h"
#include "../../Enum/RadTypes.h"
#include "../../Enum/TunnelTypes.h"
#include "../../Utilities/TemplateDef.h"
#include <FPSCounter.h>
#include <GameOptionsClass.h>

std::unique_ptr<RulesExt::ExtData> RulesExt::Data = nullptr;

void RulesExt::Allocate(RulesClass *pThis) {
	Data = std::make_unique<RulesExt::ExtData>(pThis);
}

void RulesExt::Remove(RulesClass *pThis) {
	Data = nullptr;
}

void RulesExt::LoadFromINIFile(RulesClass *pThis, CCINIClass *pINI) {
	Data->LoadFromINI(pINI);
}

void RulesExt::LoadBeforeTypeData(RulesClass *pThis, CCINIClass *pINI) {
	GenericPrerequisite::LoadFromINIList(pINI);
	ArmorType::LoadFromINIList(pINI);
	CursorType::LoadFromINIList(pINI);
	TunnelTypeClass::LoadFromINIList(pINI);

	SideExt::ExtMap.LoadAllFromINI(pINI);
	HouseTypeExt::ExtMap.LoadAllFromINI(pINI);

	Data->LoadBeforeTypeData(pThis, pINI);
}

void RulesExt::LoadAfterTypeData(RulesClass *pThis, CCINIClass *pINI) {
	if(pINI == CCINIClass::INI_Rules) {
		Data->InitializeAfterTypeData(pThis);
	}
	Data->LoadAfterTypeData(pThis, pINI);
}

void RulesExt::ExtData::Initialize(CCINIClass* pINI) {
	GenericPrerequisite::AddDefaults();
	ArmorType::AddDefaults();
	CursorType::LoadDefault();
}

void RulesExt::ExtData::LoadFromINIFile(CCINIClass* pINI) {
	// earliest loader - can't really do much because nothing else is initialized yet, so lookups won't work
	const char* sectionGlobalControls = "GlobalControls";

	INI_EX exINI(pINI);

	this->AllowParallelAIQueues.Read(exINI, sectionGlobalControls, "AllowParallelAIQueues");

	if(pINI->ReadString(sectionGlobalControls, "AllowBypassBuildLimit", "", Ares::readBuffer)) {
		bool temp[3] = {};
		int read = Parser<bool, 3>::Parse(Ares::readBuffer, temp);

		for(int i = 0; i < read; ++i) {
			int diffIdx = 2 - i; // remapping so that HouseClass::AIDifficulty can be used as an index
			this->AllowBypassBuildLimit[diffIdx] = temp[i];
		}
	}

	this->DebugKeysEnabled.Read(exINI, sectionGlobalControls, "DebugKeysEnabled");
}

void RulesExt::ExtData::LoadBeforeTypeData(RulesClass *pThis, CCINIClass *pINI) {
	const char* section = "WeaponTypes";
	const char* sectionGeneral = "General";
	const char* sectionCombatDamage = "CombatDamage";
	const char* sectionAV = "AudioVisual";
	const char* sectionCrateRules = "CrateRules";

	int len = pINI->GetKeyCount(section);
	for (int i = 0; i < len; ++i) {
		const char *key = pINI->GetKeyName(section, i);
		if (pINI->ReadString(section, key, "", Ares::readBuffer)) {
			WeaponTypeClass::FindOrAllocate(Ares::readBuffer);
		}
	}

	RulesExt::ExtData *pData = RulesExt::Global();

	if (!pData) {
		return;
	}

	INI_EX exINI(pINI);

	pData->CanMakeStuffUp.Read(exINI, sectionGeneral, "CanMakeStuffUp");

	pData->Tiberium_DamageEnabled.Read(exINI, sectionGeneral, "TiberiumDamageEnabled");
	pData->Tiberium_HealEnabled.Read(exINI, sectionGeneral, "TiberiumHealEnabled");
	pData->Tiberium_ExplosiveWarhead.Read(exINI, sectionCombatDamage, "TiberiumExplosiveWarhead");
	pData->Tiberium_ExplosiveAnim.Read(exINI, sectionAV, "TiberiumExplosiveAnim");

	pData->OverlayExplodeThreshold.Read(exINI, sectionGeneral, "OverlayExplodeThreshold");

	pData->EnemyInsignia.Read(exINI, sectionGeneral, "EnemyInsignia");

	pData->ReturnStructures.Read(exINI, sectionGeneral, "ReturnStructures");

	pData->TypeSelectUseDeploy.Read(exINI, sectionGeneral, "TypeSelectUseDeploy");

	pData->TeamRetaliate.Read(exINI, sectionGeneral, "TeamRetaliate");

	pData->DeactivateDim_Powered.Read(exINI, sectionAV, "DeactivateDimPowered");
	pData->DeactivateDim_EMP.Read(exINI, sectionAV, "DeactivateDimEMP");
	pData->DeactivateDim_Operator.Read(exINI, sectionAV, "DeactivateDimOperator");

	pData->BerserkROFMultiplier.Read(exINI, sectionCombatDamage, "BerserkROFMultiplier");

	pData->AutoRepelAI.Read(exINI, sectionCombatDamage, "AutoRepel");
	pData->AutoRepelPlayer.Read(exINI, sectionCombatDamage, "PlayerAutoRepel");

	pData->MessageSilosNeeded.Read(exINI, sectionGeneral, "Message.SilosNeeded");

	pData->DegradeEnabled.Read(exINI, sectionGeneral, "Degrade.Enabled");
	pData->DegradePercentage.Read(exINI, sectionGeneral, "Degrade.Percentage");
	pData->DegradeAmountNormal.Read(exINI, sectionGeneral, "Degrade.AmountNormal");
	pData->DegradeAmountConsumer.Read(exINI, sectionGeneral, "Degrade.AmountConsumer");

	pData->AlliedSolidTransparency.Read(exINI, sectionCombatDamage, "AlliedSolidTransparency");

	pData->DamageAirConsiderBridges.Read(exINI, sectionGeneral, "DamageAirConsiderBridges");

	pData->DiskLaserAnimEnabled.Read(exINI, sectionAV, "DiskLaserAnimEnabled");

	pData->DisplayCreditsDelay.Read(exINI, sectionAV, "DisplayCreditsDelay");
	pData->StealthSpeakDelay.Read(exINI, sectionAV, "StealthSpeakDelay");
	pData->SubterraneanSpeakDelay.Read(exINI, sectionAV, "SubterraneanSpeakDelay");

	pData->UnitsUnsellable.Read(exINI, sectionGeneral, "UnitsUnsellable");

	pData->VeteranFlashTimer.Read(exINI, sectionAV, "VeteranFlashTimer");

	pData->RandomCrateMoney.Read(exINI, sectionCrateRules, "RandomCrateMoney");

	pData->IronCurtainFlash.Read(exINI, sectionAV, "IronCurtainFlash");

	pData->RepairStopOnInsufficientFunds.Read(exINI, sectionGeneral, "RepairStopOnInsufficientFunds");
	pData->ChronoInfantryCrush.Read(exINI, sectionGeneral, "ChronoInfantryCrush");

	pData->StartInMultiplayerUnitCost.Read(exINI, sectionGeneral, "StartInMultiplayerUnitCost");
	pData->AIFriendlyDistance.Read(exINI, sectionGeneral, "AIFriendlyDistance");

	pData->EMPAIRecoverMission.Read(exINI, sectionCombatDamage, "EMPAIRecoverMission");
}

// this runs between the before and after type data loading methods for rules ini
void RulesExt::ExtData::InitializeAfterTypeData(RulesClass* const pThis) {
	this->FirestormActiveAnim = AnimTypeClass::Find("GAFSDF_A");
	this->FirestormIdleAnim = AnimTypeClass::Find("FSIDLE");
	this->FirestormGroundAnim = AnimTypeClass::Find("FSGRND");
	this->FirestormAirAnim = AnimTypeClass::Find("FSAIR");
}

// this should load everything that TypeData is not dependant on
// i.e. InfantryElectrocuted= can go here since nothing refers to it
// but [GenericPrerequisites] have to go earlier because they're used in parsing TypeData
void RulesExt::ExtData::LoadAfterTypeData(RulesClass *pThis, CCINIClass *pINI) {
	RulesExt::ExtData *pData = RulesExt::Global();

	if (!pData) {
		return;
	}

	INI_EX exINI(pINI);

	pData->ElectricDeath.Read(exINI, "AudioVisual", "InfantryElectrocuted");

	pData->DecloakSound.Read(exINI, "AudioVisual", "DecloakSound");
	pData->CloakHeight.Read(exINI, "General", "CloakHeight");

	for (int i = 0; i < WeaponTypeClass::Array.Count; ++i) {
		WeaponTypeClass::Array.GetItem(i)->LoadFromINI(pINI);
	}

	pData->EngineerDamage.Read(exINI, "General", "EngineerDamage");
	pData->EngineerAlwaysCaptureTech.Read(exINI, "General", "EngineerAlwaysCaptureTech");

	pData->EnemyWrench.Read(exINI, "General", "EnemyWrench");

	pData->HunterSeekerBuildings.Read(exINI, "SpecialWeapons", "HSBuilding");
	pData->HunterSeekerDetonateProximity.Read(exINI, "General", "HunterSeekerDetonateProximity");
	pData->HunterSeekerDescendProximity.Read(exINI, "General", "HunterSeekerDescendProximity");
	pData->HunterSeekerAscentSpeed.Read(exINI, "General", "HunterSeekerAscentSpeed");
	pData->HunterSeekerDescentSpeed.Read(exINI, "General", "HunterSeekerDescentSpeed");
	pData->HunterSeekerEmergeSpeed.Read(exINI, "General", "HunterSeekerEmergeSpeed");

	pData->DropPodTrailer.Read(exINI, "General", "DropPodTrailer");
	pData->DropPodTypes.Read(exINI, "General", "DropPodTypes");
	pData->DropPodMinimum.Read(exINI, "General", "DropPodMinimum");
	pData->DropPodMaximum.Read(exINI, "General", "DropPodMaximum");

	pData->TogglePowerAllowed.Read(exINI, "General", "TogglePowerAllowed");
	pData->TogglePowerDelay.Read(exINI, "General", "TogglePowerDelay");
	pData->TogglePowerIQ.Read(exINI, "IQ", "TogglePower");

	pData->FirestormActiveAnim.Read(exINI, "AudioVisual", "FirestormActiveAnim");
	pData->FirestormIdleAnim.Read(exINI, "AudioVisual", "FirestormIdleAnim");
	pData->FirestormGroundAnim.Read(exINI, "AudioVisual", "FirestormGroundAnim");
	pData->FirestormAirAnim.Read(exINI, "AudioVisual", "FirestormAirAnim");
	pData->FirestormWarhead.Read(exINI, "CombatDamage", "FirestormWarhead");
	pData->DamageToFirestormDamageCoefficient.Read(exINI, "General", "DamageToFirestormDamageCoefficient");

	pData->BountyEnablers.Read(exINI, "General", "BountyEnablers");
	pData->BountyDisplay.Read(exINI, "AudioVisual", "BountyDisplay");
}

bool RulesExt::DetailsCurrentlyEnabled()
{
	// not only checks for the min frame rate from the rules, but also whether
	// the low frame rate is actually desired. in that case, don't reduce.
	auto const current = FPSCounter::CurrentFrameRate;
	auto const wanted = static_cast<unsigned int>(
		60 / Math::clamp(GameOptionsClass::Instance.GameSpeed, 1, 6));
	return current >= wanted || current >= Detail::GetMinFrameRate();
}

bool RulesExt::DetailsCurrentlyEnabled(int const minDetailLevel)
{
	return GameOptionsClass::Instance.DetailLevel >= minDetailLevel
		&& DetailsCurrentlyEnabled();
}

// =============================
// load / save

template <typename T>
void RulesExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->ElectricDeath)
		.Process(this->EngineerDamage)
		.Process(this->EngineerAlwaysCaptureTech)
		.Process(this->MultiEngineer)
		.Process(this->TogglePowerAllowed)
		.Process(this->TogglePowerDelay)
		.Process(this->TogglePowerIQ)
		.Process(this->CanMakeStuffUp)
		.Process(this->Tiberium_DamageEnabled)
		.Process(this->Tiberium_HealEnabled)
		.Process(this->Tiberium_ExplosiveWarhead)
		.Process(this->Tiberium_ExplosiveAnim)
		.Process(this->OverlayExplodeThreshold)
		.Process(this->DecloakSound)
		.Process(this->CloakHeight)
		.Process(this->EnemyInsignia)
		.Process(this->EnemyWrench)
		.Process(this->ReturnStructures)
		.Process(this->TypeSelectUseDeploy)
		.Process(this->TeamRetaliate)
		.Process(this->DeactivateDim_Powered)
		.Process(this->DeactivateDim_EMP)
		.Process(this->DeactivateDim_Operator)
		.Process(this->BerserkROFMultiplier)
		.Process(this->HunterSeekerBuildings)
		.Process(this->HunterSeekerDetonateProximity)
		.Process(this->HunterSeekerDescendProximity)
		.Process(this->HunterSeekerAscentSpeed)
		.Process(this->HunterSeekerDescentSpeed)
		.Process(this->HunterSeekerEmergeSpeed)
		.Process(this->DropPodMinimum)
		.Process(this->DropPodMaximum)
		.Process(this->DropPodTypes)
		.Process(this->DropPodTrailer)
		.Process(this->AutoRepelAI)
		.Process(this->AutoRepelPlayer)
		.Process(this->MessageSilosNeeded)
		.Process(this->DegradeEnabled)
		.Process(this->DegradePercentage)
		.Process(this->DegradeAmountNormal)
		.Process(this->DegradeAmountConsumer)
		.Process(this->FirestormActiveAnim)
		.Process(this->FirestormIdleAnim)
		.Process(this->FirestormGroundAnim)
		.Process(this->FirestormAirAnim)
		.Process(this->FirestormWarhead)
		.Process(this->DamageToFirestormDamageCoefficient)
		.Process(this->AlliedSolidTransparency)
		.Process(this->DamageAirConsiderBridges)
		.Process(this->DiskLaserAnimEnabled)
		.Process(this->DisplayCreditsDelay)
		.Process(this->StealthSpeakDelay)
		.Process(this->SubterraneanSpeakDelay)
		.Process(this->BountyEnablers)
		.Process(this->BountyDisplay)
		.Process(this->UnitsUnsellable)
		.Process(this->VeteranFlashTimer)
		.Process(this->RandomCrateMoney)
		.Process(this->DebugKeysEnabled)
		.Process(this->AllowParallelAIQueues)
		.Process(this->AllowBypassBuildLimit)
		.Process(this->IronCurtainFlash)
		.Process(this->RepairStopOnInsufficientFunds)
		.Process(this->ChronoInfantryCrush)
		.Process(this->StartInMultiplayerUnitCost)
		.Process(this->AIFriendlyDistance)
		.Process(this->EMPAIRecoverMission);
}

void RulesExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<RulesClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void RulesExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<RulesClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool RulesExt::LoadGlobals(AresStreamReader& Stm) {
	for(auto& tab : TabCameos) {
		Savegame::ReadAresStream(Stm, tab);
	}

	return Stm.Success();
}

bool RulesExt::SaveGlobals(AresStreamWriter& Stm) {
	for(const auto& tab : TabCameos) {
		Savegame::WriteAresStream(Stm, tab);
	}

	return Stm.Success();
}

// =============================
// container hooks

DEFINE_HOOK(0x667A1D, RulesClass_CTOR, 0x5) {
	GET(RulesClass*, pItem, ESI);

	RulesExt::Allocate(pItem);
	return 0;
}

DEFINE_HOOK(0x667A30, RulesClass_DTOR, 0x5) {
	GET(RulesClass*, pItem, ECX);

	RulesExt::Remove(pItem);
	return 0;
}

IStream* g_pStm = nullptr;

DEFINE_HOOK_AGAIN(0x674730, RulesClass_SaveLoad_Prefix, 0x6)
DEFINE_HOOK(0x675210, RulesClass_SaveLoad_Prefix, 0x5)
{
	//GET(RulesClass*, pItem, ECX);
	GET_STACK(IStream*, pStm, 0x4);

	g_pStm = pStm;

	return 0;
}

DEFINE_HOOK(0x678841, RulesClass_Load_Suffix, 0x7)
{
	auto buffer = RulesExt::Global();

	AresByteStream Stm(0);
	if(Stm.ReadBlockFromStream(g_pStm)) {
		AresStreamReader Reader(Stm);

		if(Reader.Expect(RulesExt::ExtData::Canary) && Reader.RegisterChange(buffer)) {
			buffer->LoadFromStream(Reader);
		}
	}

	return 0;
}

DEFINE_HOOK(0x675205, RulesClass_Save_Suffix, 0x8)
{
	auto buffer = RulesExt::Global();
	AresByteStream saver(sizeof(*buffer));
	AresStreamWriter writer(saver);

	writer.Expect(RulesExt::ExtData::Canary);
	writer.RegisterChange(buffer);

	buffer->SaveToStream(writer);
	saver.WriteBlockToStream(g_pStm);

	return 0;
}

static_assert(sizeof(RulesExt::ExtData) == 0x180, "RulesExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the middle and the end of the block.
static_assert(offsetof(RulesExt::ExtData, ElectricDeath) == 0x008, "RulesExt::ExtData layout slipped");
static_assert(offsetof(RulesExt::ExtData, DecloakSound) == 0x038, "RulesExt::ExtData layout slipped");
static_assert(offsetof(RulesExt::ExtData, HunterSeekerBuildings) == 0x070, "RulesExt::ExtData layout slipped");
static_assert(offsetof(RulesExt::ExtData, MessageSilosNeeded) == 0x0B0, "RulesExt::ExtData layout slipped");
static_assert(offsetof(RulesExt::ExtData, DisplayCreditsDelay) == 0x118, "RulesExt::ExtData layout slipped");
static_assert(offsetof(RulesExt::ExtData, AllowBypassBuildLimit) == 0x14A, "RulesExt::ExtData layout slipped");
static_assert(offsetof(RulesExt::ExtData, EMPAIRecoverMission) == 0x160, "RulesExt::ExtData layout slipped");
