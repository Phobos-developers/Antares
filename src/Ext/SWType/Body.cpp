#include <PCX.h>

#include "Body.h"
#include "../../Misc/SWTypes.h"
#include "../../Misc/SWTypes/Nuke.h"
#include "../../Misc/SWTypes/Dominator.h"
#include "../../Misc/SWTypes/LightningStorm.h"
#include "../House/Body.h"
#include "../HouseType/Body.h"
#include "../Side/Body.h"
#include "../../Enum/CursorTypes.h"
#include "../../Ares.h"
#include "../../Ares.CRT.h"
#include "../../Utilities/Enums.h"
#include "../../Utilities/AresEnums.h"
#include "../TEvent/Body.h"

#include <TagClass.h>
#include "../../Utilities/TemplateDef.h"

#include <RadarEventClass.h>
#include <SuperClass.h>
#include <AnimClass.h>
#include <InfantryClass.h>
#include <VocClass.h>
#include <WarheadTypeClass.h>
#include <MessageListClass.h>
#include <Notifications.h>
#include <ScenarioClass.h>

SWTypeExt::ExtContainer SWTypeExt::ExtMap;

SuperWeaponTypeClass *SWTypeExt::CurrentSWType = nullptr;

// note: this array is indexed using the targeting mode. see that the first
// parameter is always equal to the index of the item.
const std::array<const AITargetingModeInfo, 21> SWTypeExt::AITargetingModes = {{
	{SuperWeaponAITargetingMode::None, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::Nuke, SuperWeaponAITargetingConstraints::Enemy, SuperWeaponAITargetingPreference::Offensive, SuperWeaponTarget::AllTechnos, SuperWeaponAffectedHouse::Enemies},
	{SuperWeaponAITargetingMode::LightningStorm, SuperWeaponAITargetingConstraints::Enemy | SuperWeaponAITargetingConstraints::LightningStormInactive, SuperWeaponAITargetingPreference::Offensive, SuperWeaponTarget::AllTechnos, SuperWeaponAffectedHouse::Enemies},
	{SuperWeaponAITargetingMode::PsychicDominator, SuperWeaponAITargetingConstraints::OffensiveCellClear | SuperWeaponAITargetingConstraints::Enemy | SuperWeaponAITargetingConstraints::DominatorInactive, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::Infantry | SuperWeaponTarget::Unit, SuperWeaponAffectedHouse::All},
	{SuperWeaponAITargetingMode::ParaDrop, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::Offensive, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::GeneticMutator, SuperWeaponAITargetingConstraints::OffensiveCellClear, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::Infantry, SuperWeaponAffectedHouse::All},
	{SuperWeaponAITargetingMode::ForceShield, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::Defensive, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::NoTarget, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::Offensive, SuperWeaponAITargetingConstraints::Enemy, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::AllTechnos, SuperWeaponAffectedHouse::Enemies},
	{SuperWeaponAITargetingMode::Stealth, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::AllTechnos, SuperWeaponAffectedHouse::Enemies},
	{SuperWeaponAITargetingMode::Self, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::Base, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::MultiMissile, SuperWeaponAITargetingConstraints::Enemy, SuperWeaponAITargetingPreference::Offensive, SuperWeaponTarget::Building, SuperWeaponAffectedHouse::Enemies},
	{SuperWeaponAITargetingMode::HunterSeeker, SuperWeaponAITargetingConstraints::Enemy, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::EnemyBase, SuperWeaponAITargetingConstraints::Enemy, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::IronCurtain, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::Attack, SuperWeaponAITargetingConstraints::Attacked, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::LowPower, SuperWeaponAITargetingConstraints::LowPower, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::LowPowerAttack, SuperWeaponAITargetingConstraints::Attacked | SuperWeaponAITargetingConstraints::LowPower, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::DropPod, SuperWeaponAITargetingConstraints::Enemy, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
	{SuperWeaponAITargetingMode::LightningRandom, SuperWeaponAITargetingConstraints::None, SuperWeaponAITargetingPreference::None, SuperWeaponTarget::None, SuperWeaponAffectedHouse::None},
}};

void SWTypeExt::ExtData::Initialize(CCINIClass* pINI)
{
	NewSWType::Init();

	auto pThis = this->OwnerObject();
	const char * section = pThis->get_ID();

	if(!pINI->GetSection(section)) {
		return;
	}

	INI_EX exINI(pINI);

	if(exINI.ReadString(section, "Action") && !_strcmpi(exINI.value(), "Custom")) {
		pThis->Action = Actions::SuperWeaponAllowed;
	}

	if(exINI.ReadString(section, "Type")) {
		auto customType = NewSWType::FindIndex(exINI.value());
		if(customType > SuperWeaponType::Invalid) {
			pThis->Type = customType;
		}
	}

	// find a NewSWType that handles this original one.
	if(this->IsOriginalType()) {
		this->HandledByNewSWType = NewSWType::FindHandler(pThis->Type);
	}

	// if this is handled by a NewSWType, initialize it.
	if(auto pNewSWType = this->GetNewSWType()) {
		pThis->Action = Actions::SuperWeaponAllowed;
		pNewSWType->Initialize(this);
	}
	this->LastAction = pThis->Action;
}

void SWTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	const char * section = this->OwnerObject()->get_ID();

	if(!pINI->GetSection(section)) {
		return;
	}

	INI_EX exINI(pINI);

	// read general properties
	this->EVA_Ready.Read(exINI, section, "EVA.Ready");
	this->EVA_Activated.Read(exINI, section, "EVA.Activated");
	this->EVA_Detected.Read(exINI, section, "EVA.Detected");
	this->EVA_Impatient.Read(exINI, section, "EVA.Impatient");
	this->EVA_InsufficientFunds.Read(exINI, section, "EVA.InsufficientFunds");
	this->EVA_SelectTarget.Read(exINI, section, "EVA.SelectTarget");

	this->SW_FireToShroud.Read(exINI, section, "SW.FireIntoShroud");
	this->SW_AutoFire.Read(exINI, section, "SW.AutoFire");
	this->SW_ManualFire.Read(exINI, section, "SW.ManualFire");
	this->SW_RadarEvent.Read(exINI, section, "SW.CreateRadarEvent");
	this->SW_ShowCameo.Read(exINI, section, "SW.ShowCameo");
	this->SW_Unstoppable.Read(exINI, section, "SW.Unstoppable");
	this->SW_AllowPlayer.Read(exINI, section, "SW.AllowPlayer");
	this->SW_AllowAI.Read(exINI, section, "SW.AllowAI");
	this->SW_TimerVisibility.Read(exINI, section, "SW.TimerVisibility");

	this->Money_Amount.Read(exINI, section, "Money.Amount");
	this->Money_DrainAmount.Read(exINI, section, "Money.DrainAmount");
	this->Money_DrainDelay.Read(exINI, section, "Money.DrainDelay");

	this->SW_Sound.Read(exINI, section, "SW.Sound");
	this->SW_ActivationSound.Read(exINI, section, "SW.ActivationSound");

	this->SW_Anim.Read(exINI, section, "SW.Animation");
	this->SW_AnimHeight.Read(exINI, section, "SW.AnimationHeight");

	this->SW_AnimVisibility.Read(exINI, section, "SW.AnimationVisibility");
	this->SW_AffectsHouse.Read(exINI, section, "SW.AffectsHouse");
	this->SW_AlwaysGranted.Read(exINI, section, "SW.AlwaysGranted");
	this->SW_UseAITargeting.Read(exINI, section, "SW.UseAITargeting");
	this->SW_AITargetingType.Read(exINI, section, "SW.AITargeting");
	this->SW_AITargetingConstraints.Read(exINI, section, "SW.AITargeting.Constraints");
	this->SW_AITargetingPreference.Read(exINI, section, "SW.AITargeting.Preference");
	this->SW_AffectsTarget.Read(exINI, section, "SW.AffectsTarget");
	this->SW_RequiresTarget.Read(exINI, section, "SW.RequiresTarget");
	this->SW_RequiresHouse.Read(exINI, section, "SW.RequiresHouse");
	this->SW_AIRequiresTarget.Read(exINI, section, "SW.AIRequiresTarget");
	this->SW_AIRequiresHouse.Read(exINI, section, "SW.AIRequiresHouse");

	this->SW_MaxCount.Read(exINI, section, "SW.MaxCount");

	this->SW_Deferment.Read(exINI, section, "SW.Deferment");
	this->SW_ChargeToDrainRatio.Read(exINI, section, "SW.ChargeToDrainRatio");

	this->SW_Cursor.Read(exINI, section, "Cursor");
	this->SW_NoCursor.Read(exINI, section, "NoCursor");

	this->SW_Warhead.Read(exINI, section, "SW.Warhead");
	this->SW_Damage.Read(exINI, section, "SW.Damage");

	if(pINI->ReadString(section, "SW.Range", Ares::readDefval, Ares::readBuffer)) {
		char* context = nullptr;
		char* p = strtok_s(Ares::readBuffer, Ares::readDelims, &context);
		if(p && *p) {
			this->SW_Range.WidthOrRange = static_cast<float>(atof(p));
			this->SW_Range.Height = 0;

			p = strtok_s(nullptr, Ares::readDelims, &context);
			if(p && *p) {
				this->SW_Range.Height = atoi(p);
			}
		}
	}

	// lighting
	this->Lighting_Enabled.Read(exINI, section, "Light.Enabled");
	this->Lighting_Ambient.Read(exINI, section, "Light.Ambient");
	this->Lighting_Red.Read(exINI, section, "Light.Red");
	this->Lighting_Green.Read(exINI, section, "Light.Green");
	this->Lighting_Blue.Read(exINI, section, "Light.Blue");

	// messages and their properties
	this->Message_FirerColor.Read(exINI, section, "Message.FirerColor");
	if(pINI->ReadString(section, "Message.Color", Ares::readDefval, Ares::readBuffer)) {
		this->Message_ColorScheme = ColorScheme::FindIndex(Ares::readBuffer);
		if(this->Message_ColorScheme < 0) {
			Debug::INIParseFailed(section, "Message.Color", Ares::readBuffer, "Expected a valid color scheme name.");
		}
	}

	this->Message_Detected.Read(exINI, section, "Message.Detected");
	this->Message_Ready.Read(exINI, section, "Message.Ready");
	this->Message_Launch.Read(exINI, section, "Message.Launch");
	this->Message_Activate.Read(exINI, section, "Message.Activate");
	this->Message_Abort.Read(exINI, section, "Message.Abort");
	this->Message_InsufficientFunds.Read(exINI, section, "Message.InsufficientFunds");
	this->Message_CannotFire.Read(exINI, section, "Message.CannotFire");

	this->Text_Preparing.Read(exINI, section, "Text.Preparing");
	this->Text_Ready.Read(exINI, section, "Text.Ready");
	this->Text_Hold.Read(exINI, section, "Text.Hold");
	this->Text_Charging.Read(exINI, section, "Text.Charging");
	this->Text_Active.Read(exINI, section, "Text.Active");

	// range related
	this->SW_RangeMinimum.Read(exINI, section, "SW.RangeMinimum");
	this->SW_RangeMaximum.Read(exINI, section, "SW.RangeMaximum");

	// designator related
	this->SW_Designators.Read(exINI, section, "SW.Designators");
	this->SW_AnyDesignator.Read(exINI, section, "SW.AnyDesignator");

	// inhibitor related
	this->SW_Inhibitors.Read(exINI, section, "SW.Inhibitors");
	this->SW_AnyInhibitor.Read(exINI, section, "SW.AnyInhibitor");

	// the fallback is handled in the PreDependent SW's code
	this->SW_PostDependent.Read(pINI, section, "SW.PostDependent");

	// restrictions for this super weapon
	this->SW_RequiredHouses = pINI->ReadHouseTypesList(section, "SW.RequiredHouses", this->SW_RequiredHouses);
	this->SW_ForbiddenHouses = pINI->ReadHouseTypesList(section, "SW.ForbiddenHouses", this->SW_ForbiddenHouses);
	this->SW_AuxBuildings.Read(exINI, section, "SW.AuxBuildings");
	this->SW_NegBuildings.Read(exINI, section, "SW.NegBuildings");

	this->SW_Group.Read(exINI, section, "SW.Group");
	this->SW_Shots.Read(exINI, section, "SW.Shots");
	this->SW_InitialReady.Read(exINI, section, "SW.InitialReady");
	this->SW_VirtualCharge.Read(exINI, section, "SW.VirtualCharge");

	// initialize the NewSWType that handles this SWType.
	if(auto pNewSWType = this->GetNewSWType()) {
		auto pThis = this->OwnerObject();
		pThis->Action = this->LastAction;
		pNewSWType->LoadFromINI(this, pINI);
		this->LastAction = pThis->Action;

		// whatever the user does, we take care of the stupid tags.
		// there is no need to have them not hardcoded.
		auto const flags = pNewSWType->Flags();
		pThis->PreClick = static_cast<bool>(flags & SuperWeaponFlags::PreClick);
		pThis->PostClick = static_cast<bool>(flags & SuperWeaponFlags::PostClick);
		pThis->PreDependent = -1;
	}

	this->CameoPal.LoadFromINI(pINI, section, "SidebarPalette");

	this->SidebarPCX.Read(pINI, section, "SidebarPCX");
}

bool SWTypeExt::ExtData::IsAvailable(HouseClass* pHouse) const {
	const auto pThis = this->OwnerObject();

	// this super weapon has been fired as often as allowed
	if(this->SW_Shots >= 0) {
		const auto pExt = HouseExt::ExtMap.Find(pHouse);
		if(pExt->ShotsAmount(pThis->ArrayIndex).ShootAmount >= this->SW_Shots) {
			return false;
		}
	}

	// some super weapons are only for humans or only for the AI
	if(pHouse->IsControlledByHuman() ? !this->SW_AllowPlayer : !this->SW_AllowAI) {
		return false;
	}

	// check whether the optional aux building exists
	if(pThis->AuxBuilding && pHouse->CountOwnedAndPresent(pThis->AuxBuilding) <= 0) {
		return false;
	}

	// allow only certain houses, disallow forbidden houses
	const auto OwnerBits = 1u << pHouse->Type->ArrayIndex;
	if(!(this->SW_RequiredHouses & OwnerBits) || (this->SW_ForbiddenHouses & OwnerBits)) {
		return false;
	}

	// check that any aux building exist and no neg building
	auto IsBuildingPresent = [pHouse](BuildingTypeClass* pType) {
		return pType && pHouse->CountOwnedAndPresent(pType) > 0;
	};

	const auto& Aux = this->SW_AuxBuildings;
	if(!Aux.empty() && std::none_of(Aux.begin(), Aux.end(), IsBuildingPresent)) {
		return false;
	}

	const auto& Neg = this->SW_NegBuildings;
	if(std::any_of(Neg.begin(), Neg.end(), IsBuildingPresent)) {
		return false;
	}

	return true;
}

SuperWeaponTarget SWTypeExt::ExtData::GetAIRequiredTarget() const {
	if(this->SW_AIRequiresTarget.isset()) {
		return this->SW_AIRequiresTarget;
	}

	auto index = static_cast<unsigned int>(this->SW_AITargetingType.Get());

	if(index < SWTypeExt::AITargetingModes.size()) {
		return SWTypeExt::AITargetingModes[index].Target;
	}

	return SuperWeaponTarget::None;
}

SuperWeaponAffectedHouse SWTypeExt::ExtData::GetAIRequiredHouse() const {
	if(this->SW_AIRequiresHouse.isset()) {
		return this->SW_AIRequiresHouse;
	}

	auto index = static_cast<unsigned int>(this->SW_AITargetingType.Get());

	if(index < SWTypeExt::AITargetingModes.size()) {
		return SWTypeExt::AITargetingModes[index].House;
	}

	return SuperWeaponAffectedHouse::None;
}

bool SWTypeExt::ExtData::CanShoot(HouseClass* pHouse) const {
	if(this->SW_Shots < 0) {
		return true;
	}

	const auto pExt = HouseExt::ExtMap.Find(pHouse);
	return pExt->ShotsAmount(this->OwnerObject()->ArrayIndex).ShootAmount < this->SW_Shots;
}

SuperWeaponAITargetingConstraints SWTypeExt::ExtData::GetAITargetingConstraints() const {
	if(this->SW_AITargetingConstraints.isset()) {
		return this->SW_AITargetingConstraints;
	}

	auto index = static_cast<unsigned int>(this->SW_AITargetingType.Get());

	if(index < SWTypeExt::AITargetingModes.size()) {
		return SWTypeExt::AITargetingModes[index].Constrain;
	}

	return SuperWeaponAITargetingConstraints::None;
}

SuperWeaponAITargetingPreference SWTypeExt::ExtData::GetAITargetingPreference() const {
	if(this->SW_AITargetingPreference.isset()) {
		return this->SW_AITargetingPreference;
	}

	auto index = static_cast<unsigned int>(this->SW_AITargetingType.Get());

	if(index < SWTypeExt::AITargetingModes.size()) {
		return SWTypeExt::AITargetingModes[index].Preference;
	}

	return SuperWeaponAITargetingPreference::None;
}

bool SWTypeExt::ExtData::MeetsAITargetingConstraints(HouseClass* pOwner, bool manual) const {
	auto const flags = this->GetAITargetingConstraints();

	auto const offensive = pOwner->PreferredTargetCell != CellStruct::Empty;
	if((flags & SuperWeaponAITargetingConstraints::OffensiveCellClear) && offensive) {
		return false;
	}

	if((flags & SuperWeaponAITargetingConstraints::OffensiveCellSet) && !offensive) {
		return false;
	}

	auto const defensive = pOwner->PreferredDefensiveCell2 != CellStruct::Empty;
	if((flags & SuperWeaponAITargetingConstraints::DefensiveCellClear) && defensive) {
		return false;
	}

	if((flags & SuperWeaponAITargetingConstraints::DefensiveCellSet) && !defensive) {
		return false;
	}

	if((flags & SuperWeaponAITargetingConstraints::Enemy) && pOwner->EnemyHouseIndex < 0) {
		return false;
	}

	if(!manual) {
		if((flags & SuperWeaponAITargetingConstraints::LightningStormInactive) && LightningStorm::Active) {
			return false;
		}

		if((flags & SuperWeaponAITargetingConstraints::DominatorInactive) && PsyDom::Active()) {
			return false;
		}

		if((flags & SuperWeaponAITargetingConstraints::Attacked)) {
			auto const frame = pOwner->LATime;   // unknown_54D8 in the pinned YRpp
			if(frame && frame + 75 < Unsorted::CurrentFrame) {
				return false;
			}
		}

		if((flags & SuperWeaponAITargetingConstraints::LowPower) && !pOwner->HasLowPower()) {
			return false;
		}
	}

	return true;
}

Iterator<TechnoClass*> SWTypeExt::ExtData::GetPotentialAITargets(HouseClass* pTarget) const {
	const auto require = this->GetAIRequiredTarget() & SuperWeaponTarget::AllTechnos;

	if(require == SuperWeaponTarget::None || require & SuperWeaponTarget::Building) {
		// either buildings only or it includes buildings
		if(require == SuperWeaponTarget::Building) {
			// only buildings from here, either all or of a particular house
			if(pTarget) {
				return make_iterator(pTarget->Buildings);
			}
			return make_iterator(BuildingClass::Array);
		}
		return make_iterator(TechnoClass::Array);
	}

	if(require == SuperWeaponTarget::Infantry) {
		return make_iterator(InfantryClass::Array);
	}

	// it's techno stuff, but not buildings
	return make_iterator(FootClass::Array);
}

// can i see the animation of pFirer's SW?
bool SWTypeExt::ExtData::IsAnimVisible(HouseClass* pFirer) {
	auto relation = GetRelation(pFirer, HouseClass::CurrentPlayer);
	return (this->SW_AnimVisibility & relation) == relation;
}

// can i see the countdown of pOwner's SW?
bool SWTypeExt::ExtData::IsTimerVisible(HouseClass* pOwner) const {
	// observers see whatever the allies do
	if(HouseClass::IsCurrentPlayerObserver()) {
		return (this->SW_TimerVisibility & SuperWeaponAffectedHouse::Allies)
			!= SuperWeaponAffectedHouse::None;
	}

	auto relation = GetRelation(pOwner, HouseClass::CurrentPlayer);
	return (this->SW_TimerVisibility & relation) != SuperWeaponAffectedHouse::None;
}

// the action to show for firing this SW at cell, and the cursor that goes
// with it. Action::None means this SW does not handle the cell at all.
Action SWTypeExt::ExtData::GetAction(CellStruct cell) {
	auto const pThis = this->OwnerObject();
	auto const pType = this->GetNewSWType();

	if(!pType && pThis->Action != Actions::SuperWeaponAllowed) {
		return Action::None;
	}

	auto canFire = true;

	// prevent firing into shroud
	if(!this->SW_FireToShroud) {
		auto const pCell = MapClass::Instance.GetCellAt(cell);
		canFire = !MapClass::Instance.IsLocationShrouded(pCell->GetCoords());
	}

	// new SW types have to check whether the coordinates are valid.
	if(canFire && pType) {
		canFire = pType->CanFireAt(this, HouseClass::CurrentPlayer, cell, true);
	}

	auto index = this->SW_NoCursor;

	if(canFire) {
		SWTypeExt::CurrentSWType = pThis;
		index = this->SW_Cursor;
	} else {
		SWTypeExt::CurrentSWType = nullptr;
	}

	CursorType::AddMappedAction(
		index, this->SW_FireToShroud, static_cast<Action>(-1));

	return canFire ? Actions::SuperWeaponAllowed : Actions::SuperWeaponDisallowed;
}

// does pFirer's SW affects object belonging to pHouse?
bool SWTypeExt::ExtData::IsHouseAffected(HouseClass* pFirer, HouseClass* pHouse) {
	return IsHouseAffected(pFirer, pHouse, this->SW_AffectsHouse);
}

bool SWTypeExt::ExtData::IsHouseAffected(HouseClass* pFirer, HouseClass* pHouse, SuperWeaponAffectedHouse value) {
	auto relation = GetRelation(pFirer, pHouse);
	return (value & relation) == relation;
}

SuperWeaponAffectedHouse SWTypeExt::ExtData::GetRelation(HouseClass* pFirer, HouseClass* pHouse) {
	// that's me!
	if(pFirer == pHouse) {
		return SuperWeaponAffectedHouse::Owner;
	}

	if(pFirer->IsAlliedWith(pHouse)) {
		// only friendly houses
		return SuperWeaponAffectedHouse::Allies;
	}

	// the bad guys
	return SuperWeaponAffectedHouse::Enemies;
}

bool SWTypeExt::ExtData::IsCellEligible(CellClass* pCell, SuperWeaponTarget allowed) {
	if(allowed & SuperWeaponTarget::AllCells) {
		if(pCell->LandType == LandType::Water) {
			// check whether it supports water
			return (allowed & SuperWeaponTarget::Water) != SuperWeaponTarget::None;
		} else {
			// check whether it supports non-water
			return (allowed & SuperWeaponTarget::Land) != SuperWeaponTarget::None;
		}
	}
	return true;
}

bool SWTypeExt::ExtData::IsTechnoEligible(TechnoClass* pTechno, SuperWeaponTarget allowed) {
	if(allowed & SuperWeaponTarget::AllContents) {
		if(pTechno) {
			switch(pTechno->WhatAmI()) {
			case AbstractType::Infantry:
				return (allowed & SuperWeaponTarget::Infantry) != SuperWeaponTarget::None;
			case AbstractType::Unit:
			case AbstractType::Aircraft:
				return (allowed & SuperWeaponTarget::Unit) != SuperWeaponTarget::None;
			case AbstractType::Building:
				return (allowed & SuperWeaponTarget::Building) != SuperWeaponTarget::None;
			}
		} else {
			// is the target cell allowed to be empty?
			return (allowed & SuperWeaponTarget::NoContent) != SuperWeaponTarget::None;
		}
	}
	return true;
}

bool SWTypeExt::ExtData::IsTechnoAffected(TechnoClass* pTechno) {
	// check land and water cells
	if(!IsCellEligible(pTechno->GetCell(), this->SW_AffectsTarget)) {
		return false;
	}

	// check for specific techno type
	if(!IsTechnoEligible(pTechno, this->SW_AffectsTarget)) {
		return false;
	}

	// no restriction
	return true;
}

bool SWTypeExt::ExtData::CanFireAt(HouseClass* pOwner, CellStruct coords, bool manual) {
	auto pCell = MapClass::Instance.GetCellAt(coords);

	// check cell type
	auto const AllowedTarget = !manual ? this->GetAIRequiredTarget() : SW_RequiresTarget;
	if(!IsCellEligible(pCell, AllowedTarget)) {
		return false;
	}

	// check for techno type match
	auto const pTechno = abstract_cast<TechnoClass*>(pCell->GetContent());
	auto const AllowedHouse = !manual ? this->GetAIRequiredHouse() : SW_RequiresHouse;
	if(pTechno && AllowedHouse != SuperWeaponAffectedHouse::None) {
		if(!IsHouseAffected(pOwner, pTechno->Owner, AllowedHouse)) {
			return false;
		}
	}

	if(!IsTechnoEligible(pTechno, AllowedTarget)) {
		return false;
	}

	// no restriction
	return true;
}

namespace {
	//! Springs an event on every tag the house has related to it.
	//! Indexed rather than ranged, and the count is reread each step, because
	//! springing can remove a tag from the very list being walked.
	void SpringHouseTags(HouseClass* const pOwner, TriggerEvent const event,
		void* const pPayback)
	{
		for(auto i = 0; i < pOwner->RelatedTags.Count; ++i) {
			if(auto const pTag = pOwner->RelatedTags.Items[i]) {
				pTag->RaiseEvent(event, nullptr, CellStruct::Empty, false,
					static_cast<TechnoClass*>(pPayback));
			}
		}
	}
}

bool SWTypeExt::Activate(
	SuperClass* const pSuper, CellStruct const cell, bool const isPlayer)
{
	auto const pExt = SWTypeExt::ExtMap.Find(pSuper->Type);

	if(auto const pNewType = pExt->GetNewSWType()) {
		if(!SWTypeExt::Launch(pSuper, pNewType, cell, isPlayer)) {
			return false;
		}

		// SuperNearWaypoint needs both the super and where it went, so its payback
		// is a small struct rather than an object. The owner is reread between the
		// two sweeps, matching the shipped order.
		TEventExt::SuperTarget target{ pSuper, cell };
		SpringHouseTags(pSuper->Owner, AresTriggerEvent::SuperNearWaypoint, &target);
		SpringHouseTags(pSuper->Owner, AresTriggerEvent::SuperActivated, pSuper);

		return true;
	}

	return false;
}

bool SWTypeExt::Deactivate(
	SuperClass* const pSuper, CellStruct const cell, bool const isPlayer)
{
	auto const pExt = SWTypeExt::ExtMap.Find(pSuper->Type);

	if(auto const pNewType = pExt->GetNewSWType()) {
		pNewType->Deactivate(pSuper, cell, isPlayer);

		SpringHouseTags(pSuper->Owner, AresTriggerEvent::SuperDeactivated, pSuper);

		return true;
	}

	return false;
}

bool SWTypeExt::Launch(SuperClass* pThis, NewSWType* pSW, const CellStruct &Coords, bool IsPlayer) {
	if(SWTypeExt::ExtData *pData = SWTypeExt::ExtMap.Find(pThis->Type)) {

		// launch the SW, then play sounds and animations. if the SW isn't launched
		// nothing will be played.
		if(pSW->Activate(pThis, Coords, IsPlayer)) {
			auto const pOwnerExt = HouseExt::ExtMap.Find(pThis->Owner);
			pOwnerExt->UpdateShootCount(pThis->Type->ArrayIndex);

			auto const flags = pSW->Flags();

			if(flags & SuperWeaponFlags::PostClick) {
				// use the properties of the originally fired SW
				if(HouseExt::ExtData *pExt = HouseExt::ExtMap.Find(pThis->Owner)) {
					if(auto pLast = pThis->Owner->Supers.GetItemOrDefault(pExt->SWLastIndex)) {
						pThis = pLast;
						pData = SWTypeExt::ExtMap.Find(pThis->Type);
					}
				}
			}

			// the last shot may have made this super weapon unavailable
			if(pThis->IsOneTime || !pData->CanShoot(pThis->Owner)) {
				pThis->Owner->RecheckTechTree = true;
			}

			if(!Unsorted::MuteSWLaunches && (pData->EVA_Activated != -1) && !(flags & SuperWeaponFlags::NoEVA)) {
				VoxClass::PlayIndex(pData->EVA_Activated);
			}
			
			if(!(flags & SuperWeaponFlags::NoMoney)) {
				pThis->Owner->TransactMoney(pData->Money_Amount);
			}

			CellClass *pTarget = MapClass::Instance.GetCellAt(Coords);

			CoordStruct coords = pTarget->GetCoordsWithBridge();

			auto pAnim = pData->GetAnim();
			if(pAnim && !(flags & SuperWeaponFlags::NoAnim)) {
				coords.Z += pData->SW_AnimHeight;
				AnimClass *placeholder = GameCreate<AnimClass>(pAnim, coords);
				placeholder->Invisible = !pData->IsAnimVisible(pThis->Owner);
			}

			int sound = pData->GetSound();
			if(sound && !(flags & SuperWeaponFlags::NoSound)) {
				VocClass::PlayAt(sound, coords, nullptr);
			}

			if(pData->SW_RadarEvent && !(flags & SuperWeaponFlags::NoEvent)) {
				RadarEventClass::Create(RadarEventType::SuperweaponActivated, Coords);
			}

			if(!(flags & SuperWeaponFlags::NoMessage)) {
				pData->PrintMessage(pData->Message_Launch, pThis->Owner);
			}

			// this sw has been fired. clean up.
			int idxThis = pThis->Owner->Supers.FindItemIndex(pThis);
			if(IsPlayer && !(flags & SuperWeaponFlags::NoCleanup)) {
				// what's this? we reset the selected SW only for the player on this
				// computer, so others don't deselect it when firing simultaneously.
				// and we only do this, if this type's index is the current one, because
				// auto-firing might happen while the player still selects a target.
				// PostClick SWs do have a different type index, so they need to be
				// special cased, but they can't auto-fire anyhow.
				if(pThis->Owner == HouseClass::CurrentPlayer) {
					if(idxThis == Unsorted::CurrentSWType || (flags & SuperWeaponFlags::PostClick)) {
						Unsorted::CurrentSWType = -1;
					}
				}

				// do not play ready sound. this thing just got off.
				if(pData->EVA_Ready != -1) {
					VoxClass::SilenceIndex(pData->EVA_Ready);
				}
			}

			if(HouseExt::ExtData *pExt = HouseExt::ExtMap.Find(pThis->Owner)) {
				// post-click actions. AutoFire SWs cannot support this. Consider
				// two Chronospheres auto-firing (the second may be launched manually).
				// the ChronoWarp would chose the source selected last, because it
				// would overwrite the previous (unfired) SW's index.
				if(!(flags & SuperWeaponFlags::PostClick) && !pData->SW_AutoFire) {
					pExt->SWLastIndex = idxThis;
				}
			}
	
			return true;
		}
	}

	return false;
}

bool SWTypeExt::ExtData::IsOriginalType() const {
	auto const first = SWTypeExt::FirstCustomType;
	return this->OwnerObject()->Type < static_cast<SuperWeaponType>(first);
}

// is this an original type handled by a NewSWType?
bool SWTypeExt::ExtData::IsTypeRedirected() const {
	return this->HandledByNewSWType > SuperWeaponType::Invalid;
}

SuperWeaponType SWTypeExt::ExtData::GetTypeIndexWithRedirect() const {
	return this->IsTypeRedirected() ? this->HandledByNewSWType : this->OwnerObject()->Type;
}

SuperWeaponType SWTypeExt::ExtData::GetNewTypeIndex() const {
	// if new type, return new type, if original type return only if handled (else it's -1).
	return this->IsOriginalType() ? this->HandledByNewSWType : this->OwnerObject()->Type;
}

NewSWType* SWTypeExt::ExtData::GetNewSWType() const {
	auto TypeIdx = this->GetNewTypeIndex();

	if(TypeIdx >= static_cast<SuperWeaponType>(SWTypeExt::FirstCustomType)) {
		return NewSWType::GetNthItem(TypeIdx);
	}

	return nullptr;
}

void SWTypeExt::ExtData::PrintMessage(const CSFText& message, HouseClass* pFirer) {
	if(message.empty()) {
		return;
	}

	int color = ColorScheme::FindIndex("Gold");
	if(this->Message_FirerColor) {
		// firer color
		if(pFirer) {
			color = pFirer->ColorSchemeIndex;
		}
	} else {
		if(this->Message_ColorScheme > -1) {
			// user defined color
			color = this->Message_ColorScheme;
		} else if(HouseClass::CurrentPlayer) {
			// default way: the current player's color
			color = HouseClass::CurrentPlayer->ColorSchemeIndex;
		}
	}

	// print the message
	MessageListClass::Instance.PrintMessage(message, RulesClass::Instance->MessageDelay, color);
}

void SWTypeExt::ClearChronoAnim(SuperClass* pThis)
{
	if(pThis->Animation) {
		pThis->Animation->RemainingIterations = 0;
		pThis->Animation = nullptr;
		PointerExpiredNotification::NotifyInvalidAnim.Remove(pThis);
	}

	if(pThis->AnimationGotInvalid) {
		PointerExpiredNotification::NotifyInvalidAnim.Remove(pThis);
		pThis->AnimationGotInvalid = false;
	}
}

void SWTypeExt::CreateChronoAnim(SuperClass* const pThis, const CoordStruct &Coords, AnimTypeClass* const pAnimType)
{
	ClearChronoAnim(pThis);
	
	if(pAnimType) {
		if(auto pAnim = GameCreate<AnimClass>(pAnimType, Coords)) {
			auto const pData = SWTypeExt::ExtMap.Find(pThis->Type);
			pAnim->Invisible = !pData->IsAnimVisible(pThis->Owner);
			pThis->Animation = pAnim;
			PointerExpiredNotification::NotifyInvalidAnim.Add(pThis);
		}
	}
}

bool SWTypeExt::ChangeLighting(SuperWeaponTypeClass* pCustom) {
	auto lighting = GetLightingColor(pCustom);

	if(lighting.HasValue) {
		auto scen = ScenarioClass::Instance;
		scen->AmbientTarget = lighting.Ambient;
		scen->RecalcLighting(lighting.Red, lighting.Green, lighting.Blue, 1);
	}

	return lighting.HasValue;
}

LightingColor SWTypeExt::GetLightingColor(SuperWeaponTypeClass* pCustom) {
	auto scen = ScenarioClass::Instance;
	SuperWeaponTypeClass* pType = nullptr;

	LightingColor ret;
	if(NukeFlash::IsFadingIn() || ChronoScreenEffect::Status) {
		// nuke flash
		ret.Ambient = scen->NukeAmbient;
		ret.Red = scen->NukeLighting.Tint.Red;
		ret.Green = scen->NukeLighting.Tint.Green;
		ret.Blue = scen->NukeLighting.Tint.Blue;

		pType = SW_NuclearMissile::CurrentNukeType;
	} else if(LightningStorm::Active) {
		// lightning storm
		ret.Ambient = scen->IonAmbient;
		ret.Red = scen->IonLighting.Tint.Red;
		ret.Green = scen->IonLighting.Tint.Green;
		ret.Blue = scen->IonLighting.Tint.Blue;

		if(SuperClass *pSuper = SW_LightningStorm::CurrentLightningStorm) {
			pType = pSuper->Type;
		}
	} else if(PsyDom::Status != PsychicDominatorStatus::Inactive && PsyDom::Status != PsychicDominatorStatus::Over) {
		// psychic dominator
		ret.Ambient = scen->DominatorAmbient;
		ret.Red = scen->DominatorLighting.Tint.Red;
		ret.Green = scen->DominatorLighting.Tint.Green;
		ret.Blue = scen->DominatorLighting.Tint.Blue;

		if(SuperClass *pSuper = SW_PsychicDominator::CurrentPsyDom) {
			pType = pSuper->Type;
		}
	} else {
		// no special lightning
		ret.Ambient = scen->AmbientOriginal;
		ret.Red = scen->NormalLighting.Tint.Red;
		ret.Green = scen->NormalLighting.Tint.Green;
		ret.Blue = scen->NormalLighting.Tint.Blue;

		ret.HasValue = false;
	}

	ret.Red *= 10;
	ret.Green *= 10;
	ret.Blue *= 10;

	// active SW or custom one?
	if(auto pData = SWTypeExt::ExtMap.Find(pCustom ? pCustom : pType)) {
		pData->UpdateLightingColor(ret);
	}

	return ret;
}

bool SWTypeExt::ExtData::UpdateLightingColor(LightingColor& Lighting) const {
	if(this->Lighting_Enabled) {
		auto UpdateValue = [](const Nullable<int> &from, int& into, int factor) {
			int value = from.Get(-1);
			if(value >= 0) {
				into = factor * value;
			}
		};

		UpdateValue(this->Lighting_Ambient, Lighting.Ambient, 1);
		UpdateValue(this->Lighting_Red, Lighting.Red, 10);
		UpdateValue(this->Lighting_Green, Lighting.Green, 10);
		UpdateValue(this->Lighting_Blue, Lighting.Blue, 10);

		Lighting.HasValue = true;
		return true;
	} else {
		Lighting.HasValue = false;
		return false;
	}
}

WarheadTypeClass* SWTypeExt::ExtData::GetWarhead() const {
	if(auto pType = this->GetNewSWType()) {
		return pType->GetWarhead(this);
	}

	return this->SW_Warhead.Get(nullptr);
}

AnimTypeClass* SWTypeExt::ExtData::GetAnim() const {
	if(auto pType = this->GetNewSWType()) {
		return pType->GetAnim(this);
	}

	return this->SW_Anim.Get(nullptr);
}

int SWTypeExt::ExtData::GetSound() const {
	if(auto pType = this->GetNewSWType()) {
		return pType->GetSound(this);
	}

	return this->SW_Sound.Get(-1);
}

int SWTypeExt::ExtData::GetDamage() const {
	if(auto pType = this->GetNewSWType()) {
		return pType->GetDamage(this);
	}

	return this->SW_Damage.Get(0);
}

SWRange SWTypeExt::ExtData::GetRange() const {
	if(auto pType = this->GetNewSWType()) {
		return pType->GetRange(this);
	}

	return this->SW_Range;
}

double SWTypeExt::ExtData::GetChargeToDrainRatio() const {
	return this->SW_ChargeToDrainRatio.Get(RulesClass::Instance->ChargeToDrainRatio);
}

// =============================
// load / save

template <typename T>
void SWTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->SpyPlane_TypeIndex)
		.Process(this->SpyPlane_Count)
		.Process(this->SpyPlane_Mission)
		.Process(this->Weather_Duration)
		.Process(this->Weather_HitDelay)
		.Process(this->Weather_ScatterDelay)
		.Process(this->Weather_ScatterCount)
		.Process(this->Weather_Separation)
		.Process(this->Weather_CloudHeight)
		.Process(this->Weather_RadarOutage)
		.Process(this->Weather_DebrisMin)
		.Process(this->Weather_DebrisMax)
		.Process(this->Weather_PrintText)
		.Process(this->Weather_IgnoreLightningRod)
		.Process(this->Weather_BoltExplosion)
		.Process(this->Weather_Clouds)
		.Process(this->Weather_Bolts)
		.Process(this->Weather_Debris)
		.Process(this->Weather_Sounds)
		.Process(this->Weather_RadarOutageAffects)
		.Process(this->Nuke_Payload)
		.Process(this->Nuke_PsiWarning)
		.Process(this->Nuke_TakeOff)
		.Process(this->Nuke_SiloLaunch)
		.Process(this->ParaDrop)
		.Process(this->EMPulse_Linked)
		.Process(this->EMPulse_TargetSelf)
		.Process(this->EMPulse_PulseDelay)
		.Process(this->EMPulse_PulseBall)
		.Process(this->EMPulse_Cannons)
		.Process(this->Protect_Duration)
		.Process(this->Protect_PlayFadeSoundTime)
		.Process(this->Protect_PowerOutageDuration)
		.Process(this->Protect_IsForceShield)
		.Process(this->Chronosphere_BlastSrc)
		.Process(this->Chronosphere_BlastDest)
		.Process(this->Chronosphere_KillOrganic)
		.Process(this->Chronosphere_KillTeleporters)
		.Process(this->Chronosphere_KillCargo)
		.Process(this->Chronosphere_AffectUndeployable)
		.Process(this->Chronosphere_AffectBuildings)
		.Process(this->Chronosphere_AffectUnwarpable)
		.Process(this->Chronosphere_AffectIronCurtain)
		.Process(this->Chronosphere_BlowUnplaceable)
		.Process(this->Chronosphere_ReconsiderBuildings)
		.Process(this->Mutate_Explosion)
		.Process(this->Mutate_IgnoreCyborg)
		.Process(this->Mutate_IgnoreNotHuman)
		.Process(this->Mutate_KillNatural)
		.Process(this->Dominator_Capture)
		.Process(this->Dominator_FireAtPercentage)
		.Process(this->Dominator_FirstAnimHeight)
		.Process(this->Dominator_SecondAnimHeight)
		.Process(this->Dominator_FirstAnim)
		.Process(this->Dominator_SecondAnim)
		.Process(this->Dominator_ControlAnim)
		.Process(this->Dominator_Ripple)
		.Process(this->Dominator_CaptureMindControlled)
		.Process(this->Dominator_CapturePermaMindControlled)
		.Process(this->Dominator_CaptureImmuneToPsionics)
		.Process(this->Dominator_PermanentCapture)
		.Process(this->Sonar_Delay)
		.Process(this->HunterSeeker_Type)
		.Process(this->HunterSeeker_RandomOnly)
		.Process(this->HunterSeeker_Buildings)
		.Process(this->DropPod_Minimum)
		.Process(this->DropPod_Maximum)
		.Process(this->DropPod_Veterancy)
		.Process(this->DropPod_Types)
		.Process(this->Battery_Power)
		.Process(this->Battery_KeepOnline)
		.Process(this->Battery_Overpower)
		.Process(this->Money_Amount)
		.Process(this->Money_DrainAmount)
		.Process(this->Money_DrainDelay)
		.Process(this->EVA_Ready)
		.Process(this->EVA_Activated)
		.Process(this->EVA_Detected)
		.Process(this->EVA_Impatient)
		.Process(this->EVA_InsufficientFunds)
		.Process(this->EVA_SelectTarget)
		.Process(this->SW_Sound)
		.Process(this->SW_ActivationSound)
		.Process(this->SW_Anim)
		.Process(this->SW_AnimHeight)
		.Process(this->SW_AnimVisibility)
		.Process(this->SW_AutoFire)
		.Process(this->SW_ManualFire)
		.Process(this->SW_FireToShroud)
		.Process(this->SW_RadarEvent)
		.Process(this->SW_ShowCameo)
		.Process(this->SW_Unstoppable)
		.Process(this->SW_AllowPlayer)
		.Process(this->SW_AllowAI)
		.Process(this->SW_TimerVisibility)
		.Process(this->SW_Cursor)
		.Process(this->SW_NoCursor)
		.Process(this->SW_PostDependent)
		.Process(this->SW_AlwaysGranted)
		.Process(this->SW_UseAITargeting)
		.Process(this->SW_AITargetingType)
		.Process(this->SW_AITargetingConstraints)
		.Process(this->SW_AITargetingPreference)
		.Process(this->SW_ChargeToDrainRatio)
		.Process(this->SW_Range)
		.Process(this->SW_MaxCount)
		.Process(this->SW_AffectsHouse)
		.Process(this->SW_RequiresHouse)
		.Process(this->SW_AIRequiresHouse)
		.Process(this->SW_AffectsTarget)
		.Process(this->SW_RequiresTarget)
		.Process(this->SW_AIRequiresTarget)
		.Process(this->SW_Warhead)
		.Process(this->SW_Damage)
		.Process(this->SW_Deferment)
		.Process(this->SW_RequiredHouses)
		.Process(this->SW_ForbiddenHouses)
		.Process(this->SW_AuxBuildings)
		.Process(this->SW_NegBuildings)
		.Process(this->SW_Group)
		.Process(this->SW_Shots)
		.Process(this->SW_InitialReady)
		.Process(this->SW_VirtualCharge)
		.Process(this->Lighting_Enabled)
		.Process(this->Lighting_Ambient)
		.Process(this->Lighting_Green)
		.Process(this->Lighting_Blue)
		.Process(this->Lighting_Red)
		.Process(this->Message_Detected)
		.Process(this->Message_Ready)
		.Process(this->Message_Launch)
		.Process(this->Message_Activate)
		.Process(this->Message_Abort)
		.Process(this->Message_InsufficientFunds)
		.Process(this->Message_CannotFire)
		.Process(this->Message_ColorScheme)
		.Process(this->Message_FirerColor)
		.Process(this->Text_Preparing)
		.Process(this->Text_Hold)
		.Process(this->Text_Ready)
		.Process(this->Text_Charging)
		.Process(this->Text_Active)
		.Process(this->SW_RangeMinimum)
		.Process(this->SW_RangeMaximum)
		.Process(this->SW_Designators)
		.Process(this->SW_AnyDesignator)
		.Process(this->SW_Inhibitors)
		.Process(this->SW_AnyInhibitor)
		.Process(this->CameoPal)
		.Process(this->SW_Deliverables)
		.Process(this->SW_DeliverBuildups)
		.Process(this->SW_DeliverBaseNormal)
		.Process(this->SW_OwnerHouse)
		.Process(this->SidebarPCX)
		.Process(this->HandledByNewSWType)
		.Process(this->LastAction);
}

void SWTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<SuperWeaponTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void SWTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<SuperWeaponTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool SWTypeExt::LoadGlobals(AresStreamReader& Stm) {
	return Stm
		.Process(CurrentSWType)
		.Success();
}

bool SWTypeExt::SaveGlobals(AresStreamWriter& Stm) {
	return Stm
		.Process(CurrentSWType)
		.Success();
}

// =============================
// container

SWTypeExt::ExtContainer::ExtContainer() : Container("SuperWeaponTypeClass") {
}

SWTypeExt::ExtContainer::~ExtContainer() = default;

void SWTypeExt::ExtContainer::InvalidatePointer(void* ptr, bool bRemoved) {
	AnnounceInvalidPointer(SWTypeExt::CurrentSWType, ptr);
}

// =============================
// container hooks

DEFINE_HOOK(0x6CE6F6, SuperWeaponTypeClass_CTOR, 0x5)
{
	GET(SuperWeaponTypeClass*, pItem, EAX);

	SWTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x6CEFE0, SuperWeaponTypeClass_SDDTOR, 0x8)
{
	GET(SuperWeaponTypeClass*, pItem, ECX);

	SWTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x6CE8D0, SuperWeaponTypeClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x6CE800, SuperWeaponTypeClass_SaveLoad_Prefix, 0xA)
{
	GET_STACK(SuperWeaponTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	SWTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x6CE8BE, SuperWeaponTypeClass_Load_Suffix, 0x7)
{
	SWTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x6CE8EA, SuperWeaponTypeClass_Save_Suffix, 0x3)
{
	SWTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x6CEE50, SuperWeaponTypeClass_LoadFromINI, 0xA)
DEFINE_HOOK(0x6CEE43, SuperWeaponTypeClass_LoadFromINI, 0xA)
{
	GET(SuperWeaponTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x3FC);

	SWTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

static_assert(sizeof(SWTypeExt::ExtData) == 0x500, "SWTypeExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the middle and the end of the block.
static_assert(offsetof(SWTypeExt::ExtData, SpyPlane_TypeIndex) == 0x008, "SWTypeExt::ExtData layout slipped");
static_assert(offsetof(SWTypeExt::ExtData, EVA_Ready) == 0x1AC, "SWTypeExt::ExtData layout slipped");
static_assert(offsetof(SWTypeExt::ExtData, SW_Cursor) == 0x1EC, "SWTypeExt::ExtData layout slipped");
static_assert(offsetof(SWTypeExt::ExtData, Message_CannotFire) == 0x380, "SWTypeExt::ExtData layout slipped");
static_assert(offsetof(SWTypeExt::ExtData, SidebarPCX) == 0x4B0, "SWTypeExt::ExtData layout slipped");
static_assert(offsetof(SWTypeExt::ExtData, LastAction) == 0x4D8, "SWTypeExt::ExtData layout slipped");

// ParaDrop streams as a map of vectors of 28-byte planes: Aircraft(4, swizzled)
// + Types(12) + Num(12). That accounts for the whole plane with nothing padded or
// elided, and the assert below catches a member changing width underneath it.
static_assert(sizeof(ParadropPlane) == 28, "ParadropPlane must stay 28 bytes: Aircraft(4) + Types(12) + Num(12)");
static_assert(offsetof(ParadropPlane, Types) == 4, "ParadropPlane layout slipped");
static_assert(offsetof(ParadropPlane, Num) == 16, "ParadropPlane layout slipped");
