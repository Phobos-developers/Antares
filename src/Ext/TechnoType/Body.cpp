#include "Body.h"
#include "../BuildingType/Body.h"
#include "../House/Body.h"
#include "../HouseType/Body.h"
#include "../Side/Body.h"
#include "../../Enum/Prerequisites.h"
#include "../../Misc/Debug.h"
#include "../../Utilities/TemplateDef.h"

#include <AbstractClass.h>
#include <HouseClass.h>
#include <PCX.h>
#include <Theater.h>
#include <VocClass.h>
#include <WarheadTypeClass.h>

TechnoTypeExt::ExtContainer TechnoTypeExt::ExtMap;

// =============================
// member funcs

void AbilityFlags::Read(INI_EX &parser, const char* const pSection, const char* const pKey)
{
	if(!parser.ReadString(pSection, pKey)) {
		return;
	}

	*this = AbilityFlags();

	static const char* const Names[] = {
		"EMPIMMUNE", "RADIMMUNE", "PROTECTED_DRIVER", "UNWARPABLE",
		"POISONIMMUNE", "PSIONICSIMMUNE", "PSIONICWEAPONIMMUNE" };

	// An unrecognised name is NOT an error and must not be reported as one.
	// VeteranAbilities=/EliteAbilities= are the game's own keys, and the game
	// parses its own ability set (STRONGER, FIREPOWER, SELF_HEAL, FASTER, ...)
	// out of the very same value; Ares only adds seven names of its own on top.
	// Shipped ParseAbilities (0x10041260) walks its seven-entry table, lets the
	// index run past the end for anything it does not know, and silently skips
	// it -- there is no INIParseFailed call anywhere in it.
	char* context = nullptr;
	for(auto cur = strtok_s(parser.value(), Ares::readDelims, &context); cur; cur = strtok_s(nullptr, Ares::readDelims, &context)) {
		for(auto i = 0u; i < std::size(Names); ++i) {
			if(!_strcmpi(cur, Names[i])) {
				this->Flags[i] = true;
				break;
			}
		}
	}
}

void TechnoTypeExt::ExtData::Initialize(CCINIClass* pINI) {
	auto pThis = this->OwnerObject();

	this->PrerequisiteLists.resize(1);

	this->Is_Deso = this->Is_Deso_Radiation = !strcmp(pThis->ID, "DESO");
	this->Is_Cow = !strcmp(pThis->ID, "COW");

	if(pThis->WhatAmI() == AircraftTypeClass::AbsID) {
		this->CustomMissileTrailerAnim = AnimTypeClass::Find("V3TRAIL");
		this->CustomMissileTakeoffAnim = AnimTypeClass::Find("V3TAKOFF");

		this->SmokeAnim = AnimTypeClass::Find("SGRYSMK1");
	}

	if(pThis->WhatAmI() != BuildingTypeClass::AbsID) {
		this->EVA_UnitLost = VoxClass::FindIndex("EVA_UnitLost");
	}

	auto const promoted = VoxClass::FindIndex("EVA_UnitPromoted");
	this->EVA_VeteranPromoted = promoted;
	this->EVA_ElitePromoted = promoted;
}

/*
EXT_LOAD(TechnoTypeClass)
{
	if(CONTAINS(Ext_p, pThis))
	{
		Create(pThis);

		ULONG out;
		pStm->Read(&Ext_p[pThis], sizeof(ExtData), &out);

		Ext_p[pThis]->Survivors_Pilots.Load(pStm);
		Ext_p[pThis]->Weapons.Load(pStm);
		Ext_p[pThis]->EliteWeapons.Load(pStm);
		for ( int ii = 0; ii < Ext_p[pThis]->Weapons.get_Count(); ++ii )
			SWIZZLE(Ext_p[pThis]->Weapons[ii].WeaponType);
		for ( int ii = 0; ii < Ext_p[pThis]->EliteWeapons.get_Count(); ++ii )
			SWIZZLE(Ext_p[pThis]->EliteWeapons[ii].WeaponType);
		SWIZZLE(Ext_p[pThis]->Insignia_R);
		SWIZZLE(Ext_p[pThis]->Insignia_V);
		SWIZZLE(Ext_p[pThis]->Insignia_E);
	}
}

EXT_SAVE(TechnoTypeClass)
{
	if(CONTAINS(Ext_p, pThis))
	{
		ULONG out;
		pStm->Write(&Ext_p[pThis], sizeof(ExtData), &out);

		Ext_p[pThis]->Survivors_Pilots.Save(pStm);
		Ext_p[pThis]->Weapons.Save(pStm);
		Ext_p[pThis]->EliteWeapons.Save(pStm);
	}
}
*/

void TechnoTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	auto pThis = this->OwnerObject();
	const char * section = pThis->ID;

	if(!pINI->GetSection(section)) {
		return;
	}

	INI_EX exINI(pINI);

	// survivors
	this->Survivors_Pilots.resize(SideClass::Array.Count, nullptr);

	this->Survivors_PilotCount.Read(exINI, section, "Survivor.Pilots");

	this->Survivors_PilotChance.Read(exINI, section, "Survivor.%sPilotChance");
	this->Survivors_PassengerChance.Read(exINI, section, "Survivor.%sPassengerChance");

	char flag[256];
	for(int i = 0; i < SideClass::Array.Count; ++i) {
		_snprintf_s(flag, 255, "Survivor.Side%u", i);
		if(pINI->ReadString(section, flag, "", Ares::readBuffer)) {
			if((this->Survivors_Pilots[i] = InfantryTypeClass::Find(Ares::readBuffer)) == nullptr) {
				if(!INIClass::IsBlank(Ares::readBuffer)) {
					Debug::INIParseFailed(section, flag, Ares::readBuffer);
				}
			}
		}
	}

	// prereqs

	// subtract the default list, get tag (not less than 0), add one back
	auto const prerequisiteLists = static_cast<size_t>(
		Math::max(pINI->ReadInteger(section, "Prerequisite.Lists",
		static_cast<int>(this->PrerequisiteLists.size()) - 1), 0) + 1);

	this->PrerequisiteLists.resize(prerequisiteLists);

	Prereqs::Parse(pINI, section, "Prerequisite", this->PrerequisiteLists[0]);

	Prereqs::Parse(pINI, section, "PrerequisiteOverride", pThis->PrerequisiteOverride);

	for(auto i = 0u; i < this->PrerequisiteLists.size(); ++i) {
		_snprintf_s(flag, 255, "Prerequisite.List%u", i);
		Prereqs::Parse(pINI, section, flag, this->PrerequisiteLists[i]);
	}

	Prereqs::Parse(pINI, section, "Prerequisite.Negative", this->PrerequisiteNegatives);

	if(pINI->ReadString(section, "Prerequisite.RequiredTheaters", "", Ares::readBuffer)) {
		this->PrerequisiteTheaters = 0;

		char* context = nullptr;
		for(char *cur = strtok_s(Ares::readBuffer, ",", &context); cur; cur = strtok_s(nullptr, ",", &context)) {
			signed int idx = Theater::FindIndex(cur);
			if(idx != -1) {
				this->PrerequisiteTheaters |= (1 << idx);
			} else {
				Debug::INIParseFailed(section, "Prerequisite.RequiredTheaters", cur);
			}
		}
	}

	// new secret lab
	this->Secret_RequiredHouses
		= pINI->ReadHouseTypesList(section, "SecretLab.RequiredHouses", this->Secret_RequiredHouses);

	this->Secret_ForbiddenHouses
		= pINI->ReadHouseTypesList(section, "SecretLab.ForbiddenHouses", this->Secret_ForbiddenHouses);

	this->Is_Deso = pINI->ReadBool(section, "IsDesolator", this->Is_Deso);
	this->Is_Deso_Radiation = pINI->ReadBool(section, "IsDesolator.RadDependant", this->Is_Deso_Radiation);
	this->Is_Cow  = pINI->ReadBool(section, "IsCow", this->Is_Cow);

	this->Is_Spotlighted = pINI->ReadBool(section, "HasSpotlight", this->Is_Spotlighted);
	this->Spot_Height = pINI->ReadInteger(section, "Spotlight.StartHeight", this->Spot_Height);
	this->Spot_Distance = pINI->ReadInteger(section, "Spotlight.Distance", this->Spot_Distance);
	if(pINI->ReadString(section, "Spotlight.AttachedTo", "", Ares::readBuffer)) {
		if(!_strcmpi(Ares::readBuffer, "body")) {
			this->Spot_AttachedTo = SpotlightAttachment::Body;
		} else if(!_strcmpi(Ares::readBuffer, "turret")) {
			this->Spot_AttachedTo = SpotlightAttachment::Turret;
		} else if(!_strcmpi(Ares::readBuffer, "barrel")) {
			this->Spot_AttachedTo = SpotlightAttachment::Barrel;
		} else {
			Debug::INIParseFailed(section, "Spotlight.AttachedTo", Ares::readBuffer);
		}
	}
	this->Spot_DisableR = pINI->ReadBool(section, "Spotlight.DisableRed", this->Spot_DisableR);
	this->Spot_DisableG = pINI->ReadBool(section, "Spotlight.DisableGreen", this->Spot_DisableG);
	this->Spot_DisableB = pINI->ReadBool(section, "Spotlight.DisableBlue", this->Spot_DisableB);
	this->Spot_DisableColor = pINI->ReadBool(section, "Spotlight.DisableColor", this->Spot_DisableColor);

	this->Is_Bomb = pINI->ReadBool(section, "IsBomb", this->Is_Bomb);

/*
	this is too late - Art files are loaded before this hook fires... brilliant
	if(pINI->ReadString(section, "WaterVoxel", "", buffer, 256)) {
		this->WaterAlt = 1;
	}
*/

	this->Insignia.Read(exINI, section, "Insignia.%s");
	this->InsigniaFrame.Read(exINI, section, "InsigniaFrame.%s");
	this->Parachute_Anim.Read(exINI, section, "Parachute.Anim");

	// new on 08.11.09 for #342 (Operator=)
	if(pINI->ReadString(section, "Operator", "", Ares::readBuffer)) { // try to read the flag
		this->IsAPromiscuousWhoreAndLetsAnyoneRideIt = (strcmp(Ares::readBuffer, "_ANY_") == 0); // set whether this type accepts all operators
		if(this->IsAPromiscuousWhoreAndLetsAnyoneRideIt) {
			this->Operator.clear();
		} else { // if not, find the specific operators it allows
			this->Operator.Read(exINI, section, "Operator");
		}
	}

	this->InitialPayload_Types.Read(exINI, section, "InitialPayload.Types");
	this->InitialPayload_Nums.Read(exINI, section, "InitialPayload.Nums");

	this->CameoPal.LoadFromINI(&CCINIClass::INI_Art, pThis->ImageFile, "CameoPalette");

	if(pINI->ReadString(section, "Prerequisite.StolenTechs", "", Ares::readBuffer)) {
		this->RequiredStolenTech.reset();

		char* context = nullptr;
		for(char *cur = strtok_s(Ares::readBuffer, ",", &context); cur; cur = strtok_s(nullptr, ",", &context)) {
			signed int idx = atoi(cur);
			if(idx > -1 && idx < 32) {
				this->RequiredStolenTech.set(idx);
			} else if(idx != -1) {
				Debug::INIParseFailed(section, "Prerequisite.StolenTechs", cur, "Expected a number between 0 and 31 inclusive");
			}
		}
	}

	this->VeteranAbilities.Read(exINI, section, "VeteranAbilities");
	this->EliteAbilities.Read(exINI, section, "EliteAbilities");

	this->ImmuneToEMP.Read(exINI, section, "ImmuneToEMP");
	this->EMP_Modifier.Read(exINI, section, "EMP.Modifier");
	this->EMP_Sparkles.Read(exINI, section, "EMP.Sparkles");

	if(pINI->ReadString(section, "EMP.Threshold", "inair", Ares::readBuffer)) {
		if(_strcmpi(Ares::readBuffer, "inair") == 0) {
			this->EMP_Threshold = -1;
		} else if((_strcmpi(Ares::readBuffer, "yes") == 0) || (_strcmpi(Ares::readBuffer, "true") == 0)) {
			this->EMP_Threshold = 1;
		} else if((_strcmpi(Ares::readBuffer, "no") == 0) || (_strcmpi(Ares::readBuffer, "false") == 0)) {
			this->EMP_Threshold = 0;
		} else {
			this->EMP_Threshold = pINI->ReadInteger(section, "EMP.Threshold", this->EMP_Threshold);
		}
	}

	// #733
	this->ProtectedDriver.Read(exINI, section, "ProtectedDriver");
	this->ProtectedDriver_MinHealth.Read(exINI, section, "ProtectedDriver.MinHealth");
	this->CanDrive.Read(exINI, section, "CanDrive");
	this->CanBeDriven.Read(exINI, section, "CanBeDriven");

	// #346, #464, #970, #1014
	this->PassengersGainExperience.Read(exINI, section, "Experience.PromotePassengers");
	this->ExperienceFromPassengers.Read(exINI, section, "Experience.FromPassengers");
	this->PassengerExperienceModifier.Read(exINI, section, "Experience.PassengerModifier");
	this->MindControlExperienceSelfModifier.Read(exINI, section, "Experience.MindControlSelfModifier");
	this->MindControlExperienceVictimModifier.Read(exINI, section, "Experience.MindControlVictimModifier");
	this->SpawnExperienceOwnerModifier.Read(exINI, section, "Experience.SpawnOwnerModifier");
	this->SpawnExperienceSpawnModifier.Read(exINI, section, "Experience.SpawnModifier");
	this->ExperienceFromAirstrike.Read(exINI, section, "Experience.FromAirstrike");
	this->AirstrikeExperienceModifier.Read(exINI, section, "Experience.AirstrikeModifier");
	this->Insignia_ShowEnemy.Read(exINI, section, "Insignia.ShowEnemy");

	this->VoiceRepair.Read(exINI, section, "VoiceIFVRepair");

	this->VoiceAirstrikeAttack.Read(exINI, section, "VoiceAirstrikeAttack");
	this->VoiceAirstrikeAbort.Read(exINI, section, "VoiceAirstrikeAbort");

	this->HijackerEnterSound.Read(exINI, section, "VehicleThief.EnterSound");
	this->HijackerLeaveSound.Read(exINI, section, "VehicleThief.LeaveSound");
	this->HijackerKillPilots.Read(exINI, section, "VehicleThief.KillPilots");
	this->HijackerBreakMindControl.Read(exINI, section, "VehicleThief.BreakMindControl");
	this->HijackerAllowed.Read(exINI, section, "VehicleThief.Allowed");
	this->HijackerOneTime.Read(exINI, section, "VehicleThief.OneTime");

	this->IronCurtain_Modifier.Read(exINI, section, "IronCurtain.Modifier");

	this->ForceShield_Modifier.Read(exINI, section, "ForceShield.Modifier");

	this->Chronoshift_Allow.Read(exINI, section, "Chronoshift.Allow");
	this->Chronoshift_IsVehicle.Read(exINI, section, "Chronoshift.IsVehicle");
	this->Chronoshift_Crushable.Read(exINI, section, "Chronoshift.Crushable");

	this->CameoPCX.Read(&CCINIClass::INI_Art, pThis->ImageFile, "CameoPCX");
	this->AltCameoPCX.Read(&CCINIClass::INI_Art, pThis->ImageFile, "AltCameoPCX");

	this->CanBeReversed.Read(exINI, section, "CanBeReversed");
	this->ReversedAs.Read(exINI, section, "ReversedAs");

	// #305
	this->RadarJamRadius.Read(exINI, section, "RadarJamRadius");

	// #1208
	this->PassengerTurret.Read(exINI, section, "PassengerTurret");
	
	// #617 powered units
	this->PoweredBy.Read(exINI, section, "PoweredBy");

	//#1623 - AttachEffect on unit-creation
	this->AttachedTechnoEffect.Read(exINI);

	this->BuiltAt.Read(exINI, section, "BuiltAt");

	this->Cloneable.Read(exINI, section, "Cloneable");

	this->ClonedAt.Read(exINI, section, "ClonedAt");
	this->ClonedAs.Read(exINI, section, "ClonedAs");

	this->CarryallAllowed.Read(exINI, section, "Carryall.Allowed");
	this->CarryallSizeLimit.Read(exINI, section, "Carryall.SizeLimit");

	// #680, 1362
	this->ImmuneToAbduction.Read(exINI, section, "ImmuneToAbduction");

	this->FactoryOwners.Read(exINI, section, "FactoryOwners");
	this->ForbiddenFactoryOwners.Read(exINI, section, "FactoryOwners.Forbidden");
	this->FactoryOwners_HaveAllPlans.Read(exINI, section, "FactoryOwners.HaveAllPlans");
	this->FactoryOwners_HaveAllPlans.Read(exINI, section, "FactoryOwners.Permanent");
	this->FactoryOwners_HasAllPlans.Read(exINI, section, "FactoryOwners.HasAllPlans");

	// issue #896235: cyclic gattling
	this->GattlingCyclic.Read(exINI, section, "Gattling.Cycle");

	// #245 custom missiles
	if(auto pAircraftType = specific_cast<AircraftTypeClass*>(pThis)) {
		this->IsCustomMissile.Read(exINI, section, "Missile.Custom");
		this->CustomMissileData.Read(exINI, section, "Missile");
		this->CustomMissileData.GetEx()->Type = pAircraftType;
		this->CustomMissileWarhead.Read(exINI, section, "Missile.Warhead");
		this->CustomMissileEliteWarhead.Read(exINI, section, "Missile.EliteWarhead");
		this->CustomMissileTakeoffAnim.Read(exINI, section, "Missile.TakeOffAnim");
		this->CustomMissileTrailerAnim.Read(exINI, section, "Missile.TrailerAnim");
		this->CustomMissileTrailerSeparation.Read(exINI, section, "Missile.TrailerSeparation");
		this->CustomMissileWeapon.Read(exINI, section, "Missile.Weapon");
		this->CustomMissileEliteWeapon.Read(exINI, section, "Missile.EliteWeapon");
	}

	// non-crashable aircraft
	this->Crashable.Read(exINI, section, "Crashable");

	this->CrashSpin.Read(exINI, section, "CrashSpin");

	this->AirRate.Read(exINI, section, "AirRate");

	// tiberium
	this->TiberiumProof.Read(exINI, section, "TiberiumProof");
	this->TiberiumRemains.Read(exINI, section, "TiberiumRemains");
	this->TiberiumSpill.Read(exINI, section, "TiberiumSpill");
	this->TiberiumTransmogrify.Read(exINI, section, "TiberiumTransmogrify");

	// refinery and storage
	this->Refinery_UseStorage.Read(exINI, section, "Refinery.UseStorage");

	// cloak
	this->CloakSound.Read(exINI, section, "CloakSound");
	this->DecloakSound.Read(exINI, section, "DecloakSound");
	this->CloakPowered.Read(exINI, section, "Cloakable.Powered");
	this->CloakDeployed.Read(exINI, section, "Cloakable.Deployed");
	this->CloakAllowed.Read(exINI, section, "Cloakable.Allowed");
	this->CloakStages.Read(exINI, section, "Cloakable.Stages");

	// sensors
	this->SensorArray_Warn.Read(exINI, section, "SensorArray.Warn");

	this->EVA_UnitLost.Read(exINI, section, "EVA.Lost");

	// linking units for type selection
	this->GroupAs.Read(pINI, section, "GroupAs");

	// crew settings
	this->Crew_TechnicianChance.Read(exINI, section, "Crew.TechnicianChance");
	this->Crew_EngineerChance.Read(exINI, section, "Crew.EngineerChance");

	// drain settings
	this->Drain_Local.Read(exINI, section, "Drain.Local");
	this->Drain_Amount.Read(exINI, section, "Drain.Amount");

	// smoke when damaged
	this->SmokeAnim.Read(exINI, section, "Smoke.Anim");
	this->SmokeChanceRed.Read(exINI, section, "Smoke.ChanceRed");
	this->SmokeChanceDead.Read(exINI, section, "Smoke.ChanceDead");

	// hunter seeker
	this->HunterSeekerDetonateProximity.Read(exINI, section, "HunterSeeker.DetonateProximity");
	this->HunterSeekerDescendProximity.Read(exINI, section, "HunterSeeker.DescendProximity");
	this->HunterSeekerAscentSpeed.Read(exINI, section, "HunterSeeker.AscentSpeed");
	this->HunterSeekerDescentSpeed.Read(exINI, section, "HunterSeeker.DescentSpeed");
	this->HunterSeekerEmergeSpeed.Read(exINI, section, "HunterSeeker.EmergeSpeed");
	this->HunterSeekerIgnore.Read(exINI, section, "HunterSeeker.Ignore");

	this->CivilianEnemy.Read(exINI, section, "CivilianEnemy");

	// particles
	this->DamageSparks.Read(exINI, section, "DamageSparks");

	this->ParticleSystems_DamageSmoke.Read(exINI, section, "DamageSmokeParticleSystems");
	this->ParticleSystems_DamageSparks.Read(exINI, section, "DamageSparksParticleSystems");

	// berserking options
	this->BerserkROFMultiplier.Read(exINI, section, "Berserk.ROFMultiplier");
	this->ImmuneToBerserk.Read(exINI, section, "ImmuneToBerserk");

	// super weapon
	this->DesignatorRange.Read(exINI, section, "DesignatorRange");
	this->InhibitorRange.Read(exINI, section, "InhibitorRange");

	// assault options
	this->AssaulterLevel.Read(exINI, section, "Assaulter.Level");

	// crush
	this->OmniCrusher_Aggressive.Read(exINI, section, "OmniCrusher.Aggressive");
	this->CrushDamage.Read(exINI, section, "CrushDamage.%s");
	this->CrushDamageWarhead.Read(exINI, section, "CrushDamage.Warhead");

	// reloading
	this->ReloadRate.Read(exINI, section, "ReloadRate");
	this->ReloadAmount.Read(exINI, section, "ReloadAmount");
	this->EmptyReloadAmount.Read(exINI, section, "EmptyReloadAmount");
	this->NoAmmoAmount.Read(exINI, section, "NoAmmoAmount");
	this->NoAmmoWeapon.Read(exINI, section, "NoAmmoWeapon");

	this->Saboteur.Read(exINI, section, "Saboteur");

	// note the wrong spelling of the tag for consistency
	this->CanPassiveAcquire_Guard.Read(exINI, section, "CanPassiveAquire.Guard");
	this->CanPassiveAcquire_Cloak.Read(exINI, section, "CanPassiveAquire.Cloak");

	// self healing
	this->SelfHealing_Rate.Read(exINI, section, "SelfHealing.Rate");
	this->SelfHealing_Amount.Read(exINI, section, "SelfHealing.%sAmount");
	this->SelfHealing_Max.Read(exINI, section, "SelfHealing.%sMax");
	this->SelfHealing_CombatDelay.Read(exINI, section, "SelfHealing.CombatDelay");

	this->PassengersWhitelist.Read(exINI, section, "Passengers.Allowed");
	this->PassengersBlacklist.Read(exINI, section, "Passengers.Disallowed");
	this->Passengers_BySize.Read(exINI, section, "Passengers.BySize");

	this->NoManualUnload.Read(exINI, section, "NoManualUnload");
	this->NoManualFire.Read(exINI, section, "NoManualFire");
	this->NoManualEnter.Read(exINI, section, "NoManualEnter");
	this->NoSelfGuardArea.Read(exINI, section, "NoSelfGuardArea");

	this->EnemyUIName.Read(exINI, section, "EnemyUIName");

	// bounty
	this->Bounty_Value.Read(exINI, section, "Bounty.%sValue");
	this->Bounty.Read(exINI, section, "Bounty");
	this->Bounty_Display.Read(exINI, section, "Bounty.Display");

	// promotion
	this->Promote_IncludePassengers.Read(exINI, section, "Promote.IncludePassengers");
	this->Promote_VeteranSound.Read(exINI, section, "Promote.VeteranSound");
	this->Promote_EliteSound.Read(exINI, section, "Promote.EliteSound");
	this->Promote_VeteranFlash.Read(exINI, section, "Promote.VeteranFlash");
	this->Promote_EliteFlash.Read(exINI, section, "Promote.EliteFlash");
	this->EVA_VeteranPromoted.Read(exINI, section, "EVA.VeteranPromoted");
	this->EVA_ElitePromoted.Read(exINI, section, "EVA.ElitePromoted");
	this->Promote_VeteranType.Read(exINI, section, "Promote.VeteranType");
	this->Promote_EliteType.Read(exINI, section, "Promote.EliteType");
	this->Promote_VeteranExperience.Read(exINI, section, "Promote.VeteranExperience");
	this->Promote_EliteExperience.Read(exINI, section, "Promote.EliteExperience");

	this->FactoryPlant_Multiplier.Read(exINI, section, "FactoryPlant.Multiplier");

	// digging in and out
	this->DigInSound.Read(exINI, section, "DigInSound");
	this->DigOutSound.Read(exINI, section, "DigOutSound");
	this->DigInAnim.Read(exINI, section, "DigIn");
	this->DigOutAnim.Read(exINI, section, "DigOut");

	// falling
	this->FallRate_Parachute.Read(exINI, section, "FallRate.Parachute");
	this->FallRate_NoParachute.Read(exINI, section, "FallRate.NoParachute");
	this->FallRate_ParachuteMax.Read(exINI, section, "FallRate.ParachuteMax");
	this->FallRate_NoParachuteMax.Read(exINI, section, "FallRate.NoParachuteMax");

	this->TurretROT.Read(exINI, section, "TurretROT");

	// cursors
	this->Cursor_Deploy.Read(exINI, section, "Cursor.Deploy");
	this->Cursor_NoDeploy.Read(exINI, section, "Cursor.NoDeploy");
	this->Cursor_Enter.Read(exINI, section, "Cursor.Enter");
	this->Cursor_NoEnter.Read(exINI, section, "Cursor.NoEnter");
	this->Cursor_Move.Read(exINI, section, "Cursor.Move");
	this->Cursor_NoMove.Read(exINI, section, "Cursor.NoMove");

	// build time
	this->BuildTime_Speed.Read(exINI, section, "BuildTime.Speed");
	this->BuildTime_Cost.Read(exINI, section, "BuildTime.Cost");
	this->BuildTime_LowPowerPenalty.Read(exINI, section, "BuildTime.LowPowerPenalty");
	this->BuildTime_MinLowPower.Read(exINI, section, "BuildTime.MinLowPower");
	this->BuildTime_MaxLowPower.Read(exINI, section, "BuildTime.MaxLowPower");
	this->BuildTime_MultipleFactory.Read(exINI, section, "BuildTime.MultipleFactory");

	this->FakeOf.Read(exINI, section, "FakeOf");

	this->DeployDir.Read(exINI, section, "DeployDir");

	// type conversion
	this->Convert_Deploy.Read(exINI, section, "Convert.Deploy");
	this->Convert_Water.Read(exINI, section, "Convert.Water");
	this->Convert_Land.Read(exINI, section, "Convert.Land");
	this->Convert_Script.Read(exINI, section, "Convert.Script");

	// harvesting
	this->Harvester_LongScan.Read(exINI, section, "Harvester.LongScan");
	this->Harvester_ShortScan.Read(exINI, section, "Harvester.ShortScan");
	this->Harvester_ScanCorrection.Read(exINI, section, "Harvester.ScanCorrection");
	this->Harvester_TooFarDistance.Read(exINI, section, "Harvester.TooFarDistance");
	this->Harvester_KickDelay.Read(exINI, section, "Harvester.KickDelay");

	this->Unsellable.Read(exINI, section, "Unsellable");
	this->KeepAlive.Read(exINI, section, "KeepAlive");

	this->RadialIndicatorRadius.Read(exINI, section, "RadialIndicatorRadius");

	this->GapRadiusInCells.Read(exINI, section, "GapRadiusInCells");
	this->SuperGapRadiusInCells.Read(exINI, section, "SuperGapRadiusInCells");
}

/*
	// weapons
	int WeaponCount = pINI->ReadInteger(section, "WeaponCount", pData->Weapons.get_Count());

	if(WeaponCount < 2)
	{
		WeaponCount = 2;
	}

	while(WeaponCount < pData->Weapons.get_Count())
	{
		pData->Weapons.RemoveItem(pData->Weapons.get_Count() - 1);
	}
	if(WeaponCount > pData->Weapons.get_Count())
	{
		pData->Weapons.SetCapacity(WeaponCount, nullptr);
		pData->Weapons.set_Count(WeaponCount);
	}

	while(WeaponCount < pData->EliteWeapons.get_Count())
	{
		pData->EliteWeapons.RemoveItem(pData->EliteWeapons.get_Count() - 1);
	}
	if(WeaponCount > pData->EliteWeapons.get_Count())
	{
		pData->EliteWeapons.SetCapacity(WeaponCount, nullptr);
		pData->EliteWeapons.set_Count(WeaponCount);
	}

	WeaponStruct *W = &pData->Weapons[0];
	ReadWeapon(W, "Primary", section, pINI);

	W = &pData->EliteWeapons[0];
	ReadWeapon(W, "ElitePrimary", section, pINI);

	W = &pData->Weapons[1];
	ReadWeapon(W, "Secondary", section, pINI);

	W = &pData->EliteWeapons[1];
	ReadWeapon(W, "EliteSecondary", section, pINI);

	for(int i = 0; i < WeaponCount; ++i)
	{
		W = &pData->Weapons[i];
		_snprintf(flag, 256, "Weapon%d", i);
		ReadWeapon(W, flag, section, pINI);

		W = &pData->EliteWeapons[i];
		_snprintf(flag, 256, "EliteWeapon%d", i);
		ReadWeapon(W, flag, section, pINI);
	}

void TechnoTypeClassExt::ReadWeapon(WeaponStruct *pWeapon, const char *prefix, const char *section, CCINIClass *pINI)
{
	char buffer[256];
	char flag[64];

	pINI->ReadString(section, prefix, "", buffer);

	if(strlen(buffer))
	{
		pWeapon->WeaponType = WeaponTypeClass::FindOrAllocate(buffer);
	}

	CCINIClass *pArtINI = &CCINIClass::INI_Art;

	CoordStruct FLH;
	// (Elite?)(Primary|Secondary)FireFLH - FIRE suffix
	// (Elite?)(Weapon%d)FLH - no suffix
	if(prefix[0] == 'W' || prefix[5] == 'W') // W EliteW
	{
		_snprintf(flag, 64, "%sFLH", prefix);
	}
	else
	{
		_snprintf(flag, 64, "%sFireFLH", prefix);
	}
	pArtINI->Read3Integers((int *)&FLH, section, flag, (int *)&pWeapon->FLH);
	pWeapon->FLH = FLH;

	_snprintf(flag, 64, "%sBarrelLength", prefix);
	pWeapon->BarrelLength = pArtINI->ReadInteger(section, flag, pWeapon->BarrelLength);
	_snprintf(flag, 64, "%sBarrelThickness", prefix);
	pWeapon->BarrelThickness = pArtINI->ReadInteger(section, flag, pWeapon->BarrelThickness);
	_snprintf(flag, 64, "%sTurretLocked", prefix);
	pWeapon->TurretLocked = pArtINI->ReadBool(section, flag, pWeapon->TurretLocked);
}
*/

const char* TechnoTypeExt::ExtData::GetSelectionGroupID() const
{
	return this->GroupAs ? this->GroupAs : this->OwnerObject()->ID;
}

const char* TechnoTypeExt::GetSelectionGroupID(ObjectTypeClass* pType)
{
	if(auto pExt = TechnoTypeExt::ExtMap.Find(static_cast<TechnoTypeClass*>(pType))) {
		return pExt->GetSelectionGroupID();
	}

	return pType->ID;
}

bool TechnoTypeExt::HasSelectionGroupID(ObjectTypeClass* pType, const char* pID)
{
	auto id = TechnoTypeExt::GetSelectionGroupID(pType);
	return (_strcmpi(id, pID) == 0);
}

bool TechnoTypeExt::ExtData::CameoIsElite(HouseClass const* const pHouse) const
{
	auto const pCountry = pHouse->Type;

	auto const pType = this->OwnerObject();
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	if(!pType->AltCameo && !pExt->AltCameoPCX.Exists()) {
		return false;
	}

	switch(pType->WhatAmI()) {
	case AbstractType::InfantryType:
		if(pHouse->BarracksInfiltrated && !pType->Naval && pType->Trainable) {
			return true;
		} else {
			return pCountry->VeteranInfantry.FindItemIndex(static_cast<InfantryTypeClass*>(pType)) != -1;
		}
	// The cameo has to agree with what Init actually grants, otherwise the veteran
	// chevron shows on units that will not start veteran. Naval vehicles are a
	// strict partition: WarFactoryInfiltrated is Naval=no only, NavalYardInfiltrated
	// is Naval=yes only. Aircraft and buildings have no Naval test at all.
	case AbstractType::UnitType:
		if(pType->Trainable) {
			auto const pHouseExt = HouseExt::ExtMap.Find(pHouse);
			if(pType->Naval ? pHouseExt->NavalYardInfiltrated : pHouse->WarFactoryInfiltrated) {
				return true;
			}
		}
		return pCountry->VeteranUnits.FindItemIndex(static_cast<UnitTypeClass*>(pType)) != -1;
	case AbstractType::AircraftType:
		if(pType->Trainable && HouseExt::ExtMap.Find(pHouse)->AircraftFactoryInfiltrated) {
			return true;
		}
		return pCountry->VeteranAircraft.FindItemIndex(static_cast<AircraftTypeClass*>(pType)) != -1;
	case AbstractType::BuildingType:
		if(auto const pItem = pType->UndeploysInto) {
			return pCountry->VeteranUnits.FindItemIndex(static_cast<UnitTypeClass*>(pItem)) != -1;
		} else {
			if(pType->Trainable && HouseExt::ExtMap.Find(pHouse)->BuildingInfiltrated) {
				return true;
			}
			auto const pData = HouseTypeExt::ExtMap.Find(pCountry);
			return pData->VeteranBuildings.Contains(static_cast<BuildingTypeClass*>(pType));
		}
	}

	return false;
}

bool TechnoTypeExt::ExtData::CanBeBuiltAt(
	BuildingTypeClass const* const pFactoryType) const
{
	auto const pBExt = BuildingTypeExt::ExtMap.Find(pFactoryType);
	return (this->BuiltAt.empty() && !pBExt->Factory_ExplicitOnly)
		|| this->BuiltAt.Contains(pFactoryType);
}

bool TechnoTypeExt::ExtData::CarryallCanLift(UnitClass * Target) {
	if(Target->ParasiteEatingMe) {
		return false;
	}
	auto TargetData = TechnoTypeExt::ExtMap.Find(Target->Type);
	UnitTypeClass *TargetType = Target->Type;
	bool canCarry = !TargetType->Organic && !TargetType->NonVehicle;
	if(TargetData->CarryallAllowed.isset()) {
		canCarry = !!TargetData->CarryallAllowed;
	}
	if(!canCarry) {
		return false;
	}
	if(this->CarryallSizeLimit.isset()) {
		int maxSize = this->CarryallSizeLimit;
		if(maxSize != -1) {
			return maxSize >= static_cast<TechnoTypeClass *>(Target->Type)->Size;
		}
	}
	return true;

}

bool TechnoTypeExt::ExtData::IsGenericPrerequisite() const
{
	if(this->GenericPrerequisite.empty()) {
		bool isGeneric = false;
		for(auto const& Prereq : GenericPrerequisite::Array) {
			if(Prereq->Alternates.FindItemIndex(this->OwnerObject()) != -1) {
				isGeneric = true;
				break;
			}
		}
		this->GenericPrerequisite = isGeneric;
	}

	return this->GenericPrerequisite;
}

// veteran abilities are kept once elite, so an elite unit has both sets
bool TechnoTypeExt::ExtData::HasAbility(AresAbility const ability, VeterancyStruct const& veterancy) const
{
	if(veterancy.IsElite()) {
		return this->VeteranAbilities[ability] || this->EliteAbilities[ability];
	}

	return veterancy.IsVeteran() && this->VeteranAbilities[ability];
}

// the weapon and turret slots the game has no room for live in the ext vectors,
// so every accessor has to pick the array based on the index
WeaponStruct* TechnoTypeExt::ExtData::GetWeapon(int const index, bool const elite)
{
	auto const pThis = this->OwnerObject();

	if(index < TechnoTypeClass::MaxWeapons) {
		return elite ? &pThis->EliteWeapon[index] : &pThis->Weapon[index];
	}

	auto& weapons = elite ? this->EliteWeapons : this->Weapons;
	return &weapons.data()[index - TechnoTypeClass::MaxWeapons];
}

int* TechnoTypeExt::ExtData::GetWeaponTurretIndex(int const index)
{
	if(index < TechnoTypeClass::MaxWeapons) {
		return &this->OwnerObject()->TurretWeapon[index];
	}

	return &this->WeaponTurretIndex.data()[index - TechnoTypeClass::MaxWeapons];
}

VoxelStruct* TechnoTypeExt::ExtData::GetTurretVoxel(int const index)
{
	if(index < TechnoTypeClass::MaxWeapons) {
		return &this->OwnerObject()->ChargerTurrets[index];
	}

	return &this->Turrets.data()[index - TechnoTypeClass::MaxWeapons];
}

VoxelStruct* TechnoTypeExt::ExtData::GetBarrelVoxel(int const index)
{
	if(index < TechnoTypeClass::MaxWeapons) {
		return &this->OwnerObject()->ChargerBarrels[index];
	}

	return &this->Barrels.data()[index - TechnoTypeClass::MaxWeapons];
}

void TechnoTypeExt::ExtData::ReadWeapons(CCINIClass* pINI)
{
	auto const pThis = this->OwnerObject();

	auto const overflow = static_cast<size_t>(
		std::max(0, pThis->WeaponCount - TechnoTypeClass::MaxWeapons));

	this->Weapons.resize(overflow);
	this->EliteWeapons.resize(overflow);
	this->WeaponTurretIndex.resize(overflow, -1);
	this->WeaponUINames.resize(overflow);

	INI_EX exINI(pINI);
	INI_EX exArt(&CCINIClass::INI_Art);

	auto const section = pThis->ID;
	auto const image = pThis->ImageFile;

	char flag[0x40];

	for(auto i = 0; i < pThis->WeaponCount; ++i) {
		auto const pWeapon = this->GetWeapon(i, false);
		auto const pElite = this->GetWeapon(i, true);

		// the elite key contains the ordinary one after the "Elite" prefix
		auto const length = _snprintf_s(flag, _TRUNCATE, "EliteWeapon%u", i + 1);
		auto const pSuffix = &flag[length];
		auto const cchSuffix = sizeof(flag) - length;
		auto const pPlain = &flag[5];

		detail::read(pWeapon->WeaponType, exINI, section, pPlain, true);
		detail::read(pElite->WeaponType, exINI, section, flag, true);

		_snprintf_s(pSuffix, cchSuffix, _TRUNCATE, "FLH");
		detail::read(pWeapon->FLH, exArt, image, pPlain);
		pElite->FLH = pWeapon->FLH;
		detail::read(pElite->FLH, exArt, image, flag);

		_snprintf_s(pSuffix, cchSuffix, _TRUNCATE, "BarrelLength");
		detail::read(pWeapon->BarrelLength, exArt, image, pPlain);
		pElite->BarrelLength = pWeapon->BarrelLength;
		detail::read(pElite->BarrelLength, exArt, image, flag);

		_snprintf_s(pSuffix, cchSuffix, _TRUNCATE, "BarrelThickness");
		detail::read(pWeapon->BarrelThickness, exArt, image, pPlain);
		pElite->BarrelThickness = pWeapon->BarrelThickness;
		detail::read(pElite->BarrelThickness, exArt, image, flag);

		_snprintf_s(pSuffix, cchSuffix, _TRUNCATE, "TurretLocked");
		detail::read(pWeapon->TurretLocked, exArt, image, pPlain);
		pElite->TurretLocked = pWeapon->TurretLocked;
		detail::read(pElite->TurretLocked, exArt, image, flag);
	}
}

void TechnoTypeExt::ExtData::LoadTurrets(CCINIClass* pINI)
{
	static const char* const TurretNames[] = {
		"Normal", "Repair", "MachineGun", "Flak", "Pistol", "Sniper", "Shock",
		"Explode", "BrainBlast", "RadCannon", "Chrono", "TerroristExplode",
		"Cow", "Initiate", "Virus", "YuriPrime", "Guardian" };

	auto const pThis = this->OwnerObject();

	auto const count = std::max(0, pThis->WeaponCount);
	auto const overflow = static_cast<size_t>(
		std::max(0, pThis->WeaponCount - TechnoTypeClass::MaxWeapons));

	this->WeaponTurretIndex.resize(overflow, -1);
	this->WeaponUINames.resize(static_cast<size_t>(count));

	INI_EX exINI(pINI);
	auto const section = pThis->ID;

	char flag[0x20];

	for(auto i = 0u; i < std::size(TurretNames); ++i) {
		auto const pName = TurretNames[i];

		_snprintf_s(flag, _TRUNCATE, "%sTurretWeapon", pName);
		auto weapon = -1;
		detail::read(weapon, exINI, section, flag);

		if(weapon >= 0) {
			_snprintf_s(flag, _TRUNCATE, "%sTurretIndex", pName);
			auto turret = (i < 4) ? static_cast<int>(i) : 0;
			detail::read(turret, exINI, section, flag);

			if(turret >= 0) {
				*this->GetWeaponTurretIndex(weapon) = turret;
			}
		}
	}

	for(auto i = 0; i < count; ++i) {
		_snprintf_s(flag, _TRUNCATE, "WeaponTurretIndex%u", i + 1);

		Nullable<int> index;
		index.Read(exINI, section, flag);

		auto const pTurret = this->GetWeaponTurretIndex(i);
		auto value = *pTurret;

		if(index.isset() && index.Get() >= 0) {
			value = index.Get();
			*pTurret = value;
		}

		if(value < 0 || value >= pThis->TurretCount) {
			Debug::Log(Debug::Severity::Warning,
				"Weapon %d on [%s] has an invalid turret index of %d.\n",
				i + 1, section, value);
		}
	}

	for(auto i = 0; i < count; ++i) {
		_snprintf_s(flag, _TRUNCATE, "WeaponUIName%u", i + 1);
		detail::read(this->WeaponUINames[i], exINI, section, flag);
	}
}

// =============================
// load / save

template <typename T>
void TechnoTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->Survivors_Pilots)
		.Process(this->Survivors_PilotChance)
		.Process(this->Survivors_PassengerChance)
		.Process(this->Survivors_PilotCount)
		.Process(this->Crew_TechnicianChance)
		.Process(this->Crew_EngineerChance)
		.Process(this->PrerequisiteLists)
		.Process(this->PrerequisiteNegatives)
		.Process(this->PrerequisiteTheaters)
		.Process(this->GenericPrerequisite)
		.Process(this->Secret_RequiredHouses)
		.Process(this->Secret_ForbiddenHouses)
		.Process(this->Is_Deso)
		.Process(this->Is_Deso_Radiation)
		.Process(this->Is_Cow)
		.Process(this->Is_Spotlighted)
		.Process(this->Spot_Height)
		.Process(this->Spot_Distance)
		.Process(this->Spot_AttachedTo)
		.Process(this->Spot_DisableR)
		.Process(this->Spot_DisableG)
		.Process(this->Spot_DisableB)
		.Process(this->Spot_DisableColor)
		.Process(this->Is_Bomb)
		.Process(this->Weapons)
		.Process(this->EliteWeapons)
		.Process(this->WeaponTurretIndex)
		.Process(this->WeaponUINames)
		.Process(this->Insignia)
		.Process(this->InsigniaFrame)
		.Process(this->Insignia_ShowEnemy)
		.Process(this->Parachute_Anim)
		.Process(this->Operator)
		.Process(this->IsAPromiscuousWhoreAndLetsAnyoneRideIt)
		.Process(this->InitialPayload_Types)
		.Process(this->InitialPayload_Nums)
		.Process(this->CameoPal)
		.Process(this->RequiredStolenTech)
		.Process(this->VeteranAbilities)
		.Process(this->EliteAbilities)
		.Process(this->ImmuneToEMP)
		.Process(this->EMP_Threshold)
		.Process(this->EMP_Modifier)
		.Process(this->EMP_Sparkles)
		.Process(this->IronCurtain_Modifier)
		.Process(this->ForceShield_Modifier)
		.Process(this->Chronoshift_Allow)
		.Process(this->Chronoshift_IsVehicle)
		.Process(this->Chronoshift_Crushable)
		.Process(this->ProtectedDriver)
		.Process(this->ProtectedDriver_MinHealth)
		.Process(this->CanDrive)
		.Process(this->CanBeDriven)
		.Process(this->AlternateTheaterArt)
		.Process(this->PassengersGainExperience)
		.Process(this->ExperienceFromPassengers)
		.Process(this->PassengerExperienceModifier)
		.Process(this->MindControlExperienceSelfModifier)
		.Process(this->MindControlExperienceVictimModifier)
		.Process(this->SpawnExperienceOwnerModifier)
		.Process(this->SpawnExperienceSpawnModifier)
		.Process(this->ExperienceFromAirstrike)
		.Process(this->AirstrikeExperienceModifier)
		.Process(this->VoiceRepair)
		.Process(this->VoiceAirstrikeAttack)
		.Process(this->VoiceAirstrikeAbort)
		.Process(this->HijackerEnterSound)
		.Process(this->HijackerLeaveSound)
		.Process(this->HijackerKillPilots)
		.Process(this->HijackerBreakMindControl)
		.Process(this->HijackerAllowed)
		.Process(this->HijackerOneTime)
		.Process(this->WaterImage)
		.Process(this->CloakSound)
		.Process(this->DecloakSound)
		.Process(this->CloakPowered)
		.Process(this->CloakDeployed)
		.Process(this->CloakAllowed)
		.Process(this->CloakStages)
		.Process(this->SensorArray_Warn)
		.Process(this->CameoPCX)
		.Process(this->AltCameoPCX)
		.Process(this->GroupAs)
		.Process(this->CanBeReversed)
		.Process(this->ReversedAs)
		.Process(this->RadarJamRadius)
		.Process(this->PassengerTurret)
		.Process(this->PoweredBy)
		.Process(this->AttachedTechnoEffect)
		.Process(this->BuiltAt)
		.Process(this->Cloneable)
		.Process(this->ClonedAt)
		.Process(this->ClonedAs)
		.Process(this->CarryallAllowed)
		.Process(this->CarryallSizeLimit)
		.Process(this->ImmuneToAbduction)
		.Process(this->FactoryOwners)
		.Process(this->ForbiddenFactoryOwners)
		.Process(this->FactoryOwners_HaveAllPlans)
		.Process(this->FactoryOwners_HasAllPlans)
		.Process(this->GattlingCyclic)
		.Process(this->Crashable)
		.Process(this->CrashSpin)
		.Process(this->AirRate)
		.Process(this->CivilianEnemy)
		.Process(this->IsCustomMissile)
		.Process(this->CustomMissileData)
		.Process(this->CustomMissileWarhead)
		.Process(this->CustomMissileEliteWarhead)
		.Process(this->CustomMissileTakeoffAnim)
		.Process(this->CustomMissileTrailerAnim)
		.Process(this->CustomMissileTrailerSeparation)
		.Process(this->CustomMissileWeapon)
		.Process(this->CustomMissileEliteWeapon)
		.Process(this->TiberiumProof)
		.Process(this->TiberiumRemains)
		.Process(this->TiberiumSpill)
		.Process(this->TiberiumTransmogrify)
		.Process(this->Refinery_UseStorage)
		.Process(this->EVA_UnitLost)
		.Process(this->Drain_Local)
		.Process(this->Drain_Amount)
		.Process(this->SmokeChanceRed)
		.Process(this->SmokeChanceDead)
		.Process(this->SmokeAnim)
		.Process(this->HunterSeekerDetonateProximity)
		.Process(this->HunterSeekerDescendProximity)
		.Process(this->HunterSeekerAscentSpeed)
		.Process(this->HunterSeekerDescentSpeed)
		.Process(this->HunterSeekerEmergeSpeed)
		.Process(this->HunterSeekerIgnore)
		.Process(this->DesignatorRange)
		.Process(this->InhibitorRange)
		.Process(this->DamageSparks)
		.Process(this->ParticleSystems_DamageSmoke)
		.Process(this->ParticleSystems_DamageSparks)
		.Process(this->BerserkROFMultiplier)
		.Process(this->ImmuneToBerserk)
		.Process(this->AssaulterLevel)
		.Process(this->OmniCrusher_Aggressive)
		.Process(this->CrushDamage)
		.Process(this->CrushDamageWarhead)
		.Process(this->ReloadRate)
		.Process(this->ReloadAmount)
		.Process(this->EmptyReloadAmount)
		.Process(this->NoAmmoAmount)
		.Process(this->NoAmmoWeapon)
		.Process(this->Saboteur)
		.Process(this->CanPassiveAcquire_Guard)
		.Process(this->CanPassiveAcquire_Cloak)
		.Process(this->SelfHealing_Rate)
		.Process(this->SelfHealing_Amount)
		.Process(this->SelfHealing_Max)
		.Process(this->SelfHealing_CombatDelay)
		.Process(this->PassengersWhitelist)
		.Process(this->PassengersBlacklist)
		.Process(this->Passengers_BySize)
		.Process(this->NoManualUnload)
		.Process(this->NoManualFire)
		.Process(this->NoManualEnter)
		.Process(this->NoSelfGuardArea)
		.Process(this->EnemyUIName)
		.Process(this->Bounty_Value)
		.Process(this->Bounty)
		.Process(this->Bounty_Display)
		.Process(this->Promote_IncludePassengers)
		.Process(this->Promote_VeteranSound)
		.Process(this->Promote_EliteSound)
		.Process(this->Promote_VeteranFlash)
		.Process(this->Promote_EliteFlash)
		.Process(this->EVA_VeteranPromoted)
		.Process(this->EVA_ElitePromoted)
		.Process(this->Promote_VeteranType)
		.Process(this->Promote_EliteType)
		.Process(this->Promote_VeteranExperience)
		.Process(this->Promote_EliteExperience)
		.Process(this->FactoryPlant_Multiplier)
		.Process(this->DigInSound)
		.Process(this->DigOutSound)
		.Process(this->DigInAnim)
		.Process(this->DigOutAnim)
		.Process(this->FallRate_Parachute)
		.Process(this->FallRate_NoParachute)
		.Process(this->FallRate_ParachuteMax)
		.Process(this->FallRate_NoParachuteMax)
		.Process(this->TurretROT)
		.Process(this->Cursor_Deploy)
		.Process(this->Cursor_NoDeploy)
		.Process(this->Cursor_Enter)
		.Process(this->Cursor_NoEnter)
		.Process(this->Cursor_Move)
		.Process(this->Cursor_NoMove)
		.Process(this->BuildTime_Speed)
		.Process(this->BuildTime_Cost)
		.Process(this->BuildTime_LowPowerPenalty)
		.Process(this->BuildTime_MinLowPower)
		.Process(this->BuildTime_MaxLowPower)
		.Process(this->BuildTime_MultipleFactory)
		.Process(this->FakeOf)
		.Process(this->DeployDir)
		.Process(this->Convert_Deploy)
		.Process(this->Convert_Water)
		.Process(this->Convert_Land)
		.Process(this->Convert_Script)
		.Process(this->Harvester_LongScan)
		.Process(this->Harvester_ShortScan)
		.Process(this->Harvester_ScanCorrection)
		.Process(this->Harvester_TooFarDistance)
		.Process(this->Harvester_KickDelay)
		.Process(this->Unsellable)
		.Process(this->KeepAlive)
		.Process(this->RadialIndicatorRadius)
		.Process(this->GapRadiusInCells)
		.Process(this->SuperGapRadiusInCells);
}

void TechnoTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<TechnoTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<TechnoTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

TechnoTypeExt::ExtContainer::ExtContainer() : Container("TechnoTypeClass") {
}

TechnoTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x711835, TechnoTypeClass_CTOR, 0x5)
{
	GET(TechnoTypeClass*, pItem, ESI);

	TechnoTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x711AE0, TechnoTypeClass_DTOR, 0x5)
{
	GET(TechnoTypeClass*, pItem, ECX);

	TechnoTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x716DC0, TechnoTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x7162F0, TechnoTypeClass_SaveLoad_Prefix, 0x6)
{
	GET_STACK(TechnoTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x716DAC, TechnoTypeClass_Load_Suffix, 0xA)
{
	TechnoTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x717094, TechnoTypeClass_Save_Suffix, 0x5)
{
	TechnoTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x716132, TechnoTypeClass_LoadFromINI, 0x5)
DEFINE_HOOK(0x716123, TechnoTypeClass_LoadFromINI, 0x5)
{
	GET(TechnoTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x380);

	TechnoTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

DEFINE_HOOK(0x679CAF, RulesClass_LoadAfterTypeData_CompleteInitialization, 0x5) {
	//GET(CCINIClass*, pINI, ESI);

	for(auto const& pType : BuildingTypeClass::Array) {
		auto const pExt = BuildingTypeExt::ExtMap.Find(pType);
		pExt->CompleteInitialization();
	}

	return 0;
}

static_assert(sizeof(TechnoTypeExt::ExtData) == 0x680, "TechnoTypeExt::ExtData must match the 3.0p1 layout");

// The sizeof above cannot discriminate on its own: ExtData is alignas(64), so
// sizeof is roundup(extent, 64) and a range of extents rounds to the same 0x680.
// These anchor the layout on real members. A field that changes width shifts
// everything after it while leaving the serialize order intact, which is the one
// kind of drift nothing else here would catch.
static_assert(offsetof(TechnoTypeExt::ExtData, Survivors_Pilots) == 0x008, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, Is_Spotlighted) == 0x077, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, Insignia) == 0x0D4, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, EMP_Sparkles) == 0x148, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, MindControlExperienceVictimModifier) == 0x190, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, CloakSound) == 0x1E8, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, AttachedTechnoEffect) == 0x290, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, AirRate) == 0x328, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, Refinery_UseStorage) == 0x390, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, DamageSparks) == 0x3F0, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, CanPassiveAcquire_Guard) == 0x46D, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, Bounty) == 0x500, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, DigOutSound) == 0x558, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, BuildTime_Cost) == 0x5B8, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, Harvester_KickDelay) == 0x63C, "TechnoTypeExt::ExtData layout slipped");
static_assert(offsetof(TechnoTypeExt::ExtData, SuperGapRadiusInCells) == 0x654, "TechnoTypeExt::ExtData layout slipped");

