#include "Body.h"
#include <WeaponTypeClass.h>
#include "../../Enum/ArmorTypes.h"
#include "../House/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"
#include "../../Misc/EMPulse.h"
#include "../../Utilities/TemplateDef.h"

#include <WarheadTypeClass.h>
#include <GeneralStructures.h>
#include <HouseClass.h>
#include <ObjectClass.h>
#include <BulletClass.h>
#include <IonBlastClass.h>
#include <CellClass.h>
#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <SlaveManagerClass.h>
#include <EMPulseClass.h>
#include <AnimClass.h>
#include "../Bullet/Body.h"
#include <FootClass.h>
#include <ScenarioClass.h>
#include <UnitClass.h>
#include "../../Utilities/Helpers.Alex.h"

#include <Helpers/Template.h>
#include <set>

WarheadTypeExt::ExtContainer WarheadTypeExt::ExtMap;

AresMap<IonBlastClass*, const WarheadTypeExt::ExtData*> WarheadTypeExt::IonExt;

WarheadTypeClass * WarheadTypeExt::Temporal_WH = nullptr;

WarheadTypeClass * WarheadTypeExt::EMP_WH = nullptr;

WarheadTypeClass * WarheadTypeExt::ReceiveDamage_WH = nullptr;

void WarheadTypeExt::ExtData::Initialize(CCINIClass* pINI) {
	if(!_strcmpi(this->OwnerObject()->ID, "NUKE")) {
		this->PreImpactAnim = AnimTypeClass::FindIndex("NUKEBALL");
		this->NukeFlashDuration = 30;
	}
}

void WarheadTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	auto pThis = this->OwnerObject();
	const char * section = pThis->ID;

	INI_EX exINI(pINI);

	if(!pINI->GetSection(section)) {
		// no section at all: the armor list still has to be covered
		ArmorType::GrowForWarhead(pThis);
		return;
	}

	// writing custom verses parser just because
	if(pINI->ReadString(section, "Verses", "", Ares::readBuffer)) {
		int idx = 0;
		char* context = nullptr;
		for(char *cur = strtok_s(Ares::readBuffer, ",", &context); cur; cur = strtok_s(nullptr, ",", &context)) {
			this->Verses[idx].Parse(cur);
			++idx;
			if(idx > 10) {
				break;
			}
		}
	}

	ArmorType::LoadForWarhead(pINI, pThis);

	this->MindControl_Permanent.Read(exINI, section, "MindControl.Permanent");

	this->EMP_Duration.Read(exINI, section, "EMP.Duration");
	this->EMP_Cap.Read(exINI, section, "EMP.Cap");
	this->EMP_Sparkles.Read(exINI, section, "EMP.Sparkles");

	this->IC_Duration.Read(exINI, section, "IronCurtain.Duration");
	this->IC_Cap.Read(exINI, section, "IronCurtain.Cap");
	this->IC_Flash.Read(exINI, section, "IronCurtain.Flash");

	this->Temporal_HealthFactor.Read(exINI, section, "Temporal.HealthFactor");
	this->Temporal_WarpAway.Read(exINI, section, "Temporal.WarpAway");

	this->DeployedDamage.Read(exINI, section, "Damage.Deployed");

	this->AffectsEnemies.Read(exINI, section, "AffectsEnemies");
	this->AffectsOwner.Read(exINI, section, "AffectsOwner");

	this->InfDeathAnim.Read(exINI, section, "InfDeathAnim");

	this->NukeFlashDuration.Read(exINI, section, "NukeFlash.Duration");
	this->PreImpactAnim.Read(exINI, section, "PreImpactAnim");
	this->PreImpactAnim_Moves.Read(exINI, section, "PreImpactAnim.Moves");

	this->KillDriver.Read(exINI, section, "KillDriver");
	this->KillDriver_KillBelowPercent.Read(exINI, section, "KillDriver.KillBelowPercent");
	this->KillDriver_Chance.Read(exINI, section, "KillDriver.Chance");
	this->KillDriver_Owner.Read(exINI, section, "KillDriver.Owner");
	this->KillDriver_RemoveVeterancy.Read(exINI, section, "KillDriver.RemoveVeterancy");

	this->Malicious.Read(exINI, section, "Malicious");

	this->PreventScatter.Read(exINI, section, "PreventScatter");

	this->BridgeAbsoluteDestroyer.Read(exINI, section, "BridgeAbsoluteDestroyer");

	this->CellSpread_MaxAffect.Read(exINI, section, "CellSpread.MaxAffect");

	this->AttachedEffect.Read(exINI);

	this->DamageAirThreshold.Read(exINI, section, "DamageAirThreshold");

	this->UnitLost_Suppress.Read(exINI, section, "UnitLost.Suppress");

	this->SuppressDeathWeapon_Vehicles.Read(exINI, section, "DeathWeapon.SuppressVehicles");
	this->SuppressDeathWeapon_Infantry.Read(exINI, section, "DeathWeapon.SuppressInfantry");
	this->SuppressDeathWeapon.Read(exINI, section, "DeathWeapon.Suppress");

	this->RelativeDamage.Read(exINI, section, "RelativeDamage");
	this->RelativeDamage_Buildings.Read(exINI, section, "RelativeDamage.Buildings");
	this->RelativeDamage_Aircraft.Read(exINI, section, "RelativeDamage.Aircraft");
	this->RelativeDamage_Infantry.Read(exINI, section, "RelativeDamage.Infantry");
	this->RelativeDamage_Vehicles.Read(exINI, section, "RelativeDamage.Vehicles");
	this->RelativeDamage_Terrain.Read(exINI, section, "RelativeDamage.Terrain");

	this->Sonar_Duration.Read(exINI, section, "Sonar.Duration");

	this->DisableWeapons_Duration.Read(exINI, section, "DisableWeapons.Duration");

	this->Flash_Duration.Read(exINI, section, "Flash.Duration");

	this->IonCannon.Read(exINI, section, "IonCannon");
	this->IonCannon_Rock.Read(exINI, section, "IonCannon.Rock");
	this->Ripple_Radius.Read(exINI, section, "Ripple.Radius");
	// the IonCannon spelling is read second, so it wins where both are given
	this->Ripple_Radius.Read(exINI, section, "IonCannon.Ripple");
	this->IonCannon_Blast.Read(exINI, section, "IonCannon.Blast");
	this->IonCannon_Beam.Read(exINI, section, "IonCannon.Beam");
	this->IonCannon_Warhead.Read(exINI, section, "IonCannon.Warhead");
	this->IonCannon_Damage.Read(exINI, section, "IonCannon.Damage");

	this->EffectsRequireDamage.Read(exINI, section, "EffectsRequireDamage");
	this->EffectsRequireVerses.Read(exINI, section, "EffectsRequireVerses");
	this->AllowZeroDamage.Read(exINI, section, "AllowZeroDamage");

	this->DieSound_Override.Read(exINI, section, "DieSound.Override");
	this->VoiceDie_Override.Read(exINI, section, "VoiceDie.Override");

	this->Culling_BelowHealth.Read(exINI, section, "Culling.%sBelowHealth");
	this->Culling_Chance.Read(exINI, section, "Culling.%sChance");
};

/*!
	This function checks if the passed warhead has IonCannon or Ripple.Radius set,
	and, if so, applies the effect. Without IonCannon only the impact is created,
	without the beam that precedes it.
	\note Moved here from hook BulletClass_Fire.
	\param coords The coordinates of the warhead impact, the center of the affected area.
*/
void WarheadTypeExt::ExtData::applyIonCannon(const CoordStruct &coords) {
	if(this->IonCannon || this->Ripple_Radius.Get(0) > 0) {
		auto const pBlast = GameCreate<IonBlastClass>(coords);
		pBlast->DisableIonBeam = !this->IonCannon;
		WarheadTypeExt::IonExt[pBlast] = this;
	}
}

// Applies this warhead's Iron Curtain effect.
/*!
	This function checks if the passed warhead has IronCurtain.Duration set,
	and, if so, applies the effect.

	Units will be damaged before the Iron Curtain gets effective. AffectAllies
	and AffectEnemies are respected. Verses support is limited: If it is 0%,
	the unit won't get affected, otherwise, it will be 100% affected.

	\note Moved here from hook BulletClass_Fire.

	\param coords The coordinates of the warhead impact, the center of the Iron Curtain area.
	\param Owner Owner of the Iron Curtain effect, i.e. the one triggering this.
	\param damage The damage the firing weapon deals before the Iron Curtain effect starts.

	\date 2010-06-28
*/
void WarheadTypeExt::ExtData::applyIronCurtain(const CoordStruct &coords, HouseClass* Owner, int damage) {
	CellStruct cellCoords = MapClass::Instance.GetCellAt(coords)->MapCoords;

	if(this->IC_Duration != 0) {
		// set of affected objects. every object can be here only once.
		auto items = Helpers::Alex::getCellSpreadItems(coords, this->OwnerObject()->CellSpread, true);

		// affect each object
		for(auto curTechno : items) {
			// don't protect the dead
			if(!curTechno || curTechno->InLimbo || !curTechno->IsAlive || !curTechno->Health) {
				continue;
			}

			// affects enemies or allies respectively? shipped skips the whole
			// ladder when there is no invoking house (`if(!pOwner.pointer)` at
			// 0x100535C4 jumps straight past it), rather than consulting it
			// with a null house
			if(Owner && !WarheadTypeExt::CanAffectTarget(curTechno, Owner, this->OwnerObject())) {
				continue;
			}

			// duration modifier
			int duration = this->IC_Duration;

			auto pType = curTechno->GetTechnoType();

			// modify good durations only
			if(duration > 0) {
				if(auto pData = TechnoTypeExt::ExtMap.Find(pType)) {
					duration = static_cast<int>(duration * pData->IronCurtain_Modifier);
				}
			}

			// respect verses the boolean way
			if(std::abs(this->GetVerses(pType->Armor).Verses) < 0.001) {
				continue;
			}

			// get the values
			int oldValue = (curTechno->IronCurtainTimer.Expired() ? 0 : curTechno->IronCurtainTimer.GetTimeLeft());
			int newValue = Helpers::Alex::getCappedDuration(oldValue, duration, this->IC_Cap);

			// update iron curtain
			if(oldValue <= 0) {
				// start iron curtain effect?
				if(newValue > 0) {
					// damage the victim before ICing it
					if(damage) {
						curTechno->ReceiveDamage(&damage, 0, this->OwnerObject(), nullptr, true, false, Owner);
					}

					// unit may be destroyed already.
					if(curTechno->IsAlive) {
						// start and prevent the multiplier from being applied twice
						curTechno->IronCurtain(newValue, Owner, false);
						curTechno->IronCurtainTimer.Start(newValue);
					}
				}
			} else {
				// iron curtain effect is already on.
				if(newValue > 0) {
					// set new length and reset tint stage
					curTechno->IronCurtainTimer.Start(newValue);
					curTechno->IronTintStage = 4;
				} else {
					// turn iron curtain off
					curTechno->IronCurtainTimer.Stop();
				}
			}
		}
	}
}

/*!
	This function checks if the passed warhead has EMP.Duration set, and, if so, applies the effect.
	\note Moved here from hook BulletClass_Fire.
	\param coords The coordinates of the warhead impact, the center of the EMP area.
	\param source The unit that launched the EMP.
*/
void WarheadTypeExt::ExtData::applyEMP(const CoordStruct &coords, TechnoClass *source) {
	if (this->EMP_Duration) {
		// launch our rewritten EMP.
		EMPulse::CreateEMPulse(this, coords, source);
	}
}

/*!
	This function checks if the passed warhead has MindControl.Permanent set, and, if so, applies the effect.
	\note Moved here from hook BulletClass_Fire.
	\param Owner Owner of the Mind Control effect, i.e. the one controlling the target afterwards.
	\param Target Target of the Mind Control effect, i.e. the one being controlled by the owner afterwards.
	\return false if effect wasn't applied, true if it was.
		This is important for the chain of damage effects, as, in case of true, the target is now a friendly unit.
*/
bool WarheadTypeExt::ExtData::applyPermaMC(HouseClass* const Owner, AbstractClass* const Target) const {
	if(this->MindControl_Permanent && Owner) {
		if(auto const pTarget = abstract_cast<TechnoClass*>(Target)) {
			auto const pType = pTarget->GetTechnoType();

			if(!pType->ImmuneToPsionics) {
				if(auto const pController = pTarget->MindControlledBy) {
					pController->CaptureManager->FreeUnit(pTarget);
				}

				pTarget->SetOwningHouse(Owner, true);
				pTarget->MindControlledByAUnit = true;
				pTarget->QueueMission(Mission::Guard, false);

				if(auto& pAnim = pTarget->MindControlRingAnim) {
					pAnim->UnInit();
					pAnim = nullptr;
				}

				auto const pBld = abstract_cast<BuildingClass*>(pTarget);

				CoordStruct location = pTarget->GetCoords();
				if(pBld) {
					location.Z += pBld->Type->Height * Unsorted::LevelHeight;
				} else {
					location.Z += pType->MindControlRingOffset;
				}

				if(auto const pAnimType = RulesClass::Instance->PermaControlledAnimationType) {
					if(auto const pAnim = GameCreate<AnimClass>(pAnimType, location)) {
						pTarget->MindControlRingAnim = pAnim;
						pAnim->SetOwnerObject(pTarget);
						if(pBld) {
							pAnim->ZAdjust = -1024;
						}
					}
				}

				return true;
			}
		}
	}

	return false;
}

/*!
	This function checks if the projectile transporting the warhead should pass through
		the building's walls and deliver the warhead to the occupants instead. If so, it performs that effect.
	\note Moved here from hook BulletClass_Fire.
	\note This cannot logically be triggered in situations where the warhead is not delivered by a projectile,
		such as the GenericWarhead super weapon.
	\param Bullet The projectile
	\todo This should probably be moved to /Ext/Bullet/ instead, I just maintained the previous structure to ease transition.
		Since UC.DaMO (#778) in 0.5 will require a reimplementation of this logic anyway,
		we can probably just leave it here until then.
*/
void WarheadTypeExt::applyOccupantDamage(BulletClass* const Bullet) {
	if(auto const pExt = BulletExt::ExtMap.Find(Bullet)) {
		if(pExt->DamageOccupants()) {
			// the occupants have been damaged, do not damage the building (the original target)
			Bullet->Health = 0;
			Bullet->DamageMultiplier = 0;
			Bullet->Limbo();
		}
	}
}

//! Gets whether a Techno can be affected by a warhead fired by a house.
/*!
	A warhead will not affect allies if AffectsAllies is not set and will not
	affect enemies if AffectsEnemies is not set.

	\param pTarget The Techno pWarhead is fired at.
	\param pSourceHouse The house that fired pWarhead.
	\param pWarhead The fired warhead.

	\returns True if pWarhead can affect pTarget, false otherwise.

	\author AlexB
	\date 2010-04-27
*/
bool WarheadTypeExt::CanAffectTarget(TechnoClass* const pTarget, HouseClass* const pSourceHouse, WarheadTypeClass* const pWarhead) {
	if(pSourceHouse && pTarget && pWarhead) {
		const auto pExt = WarheadTypeExt::ExtMap.Find(pWarhead);

		// the firer's own objects can be singled out, and fall back to AffectsAllies
		if(pSourceHouse == pTarget->Owner) {
			return pExt->AffectsOwner.Get(pWarhead->AffectsAllies);
		}

		// apply AffectsAllies if owner and target house are allied
		if(auto const pTargetHouse = pTarget->Owner) {
			if(pSourceHouse->IsAlliedWith(pTargetHouse)) {
				return pWarhead->AffectsAllies;
			}
		}

		// this warhead is designed to ignore enemy units
		return pExt->AffectsEnemies;
	}

	return true;
}

//! Gets the damage this warhead deals relative to the victim's health or maximum health.
/*!
	Positive values are a percentage of the victim type's Strength, negative ones a
	percentage of its current Health. Objects that are neither buildings, aircraft,
	infantry, vehicles nor terrain are never affected.

	\param pVictim The object the warhead is about to damage.

	\returns The absolute amount of damage to deal, or 0 if there is none.
*/
int WarheadTypeExt::ExtData::CalculateRelativeDamage(ObjectClass* const pVictim) const {
	int relative = 0;

	switch(pVictim->WhatAmI()) {
	case AbstractType::Unit:
	{
		auto const pType = static_cast<UnitClass*>(pVictim)->Type;
		relative = pType->ConsideredAircraft
			? this->RelativeDamage_Aircraft
			: (pType->Organic ? this->RelativeDamage_Infantry : this->RelativeDamage_Vehicles);
		break;
	}
	case AbstractType::Building:
		relative = this->RelativeDamage_Buildings;
		break;
	case AbstractType::Aircraft:
		relative = this->RelativeDamage_Aircraft;
		break;
	case AbstractType::Infantry:
		relative = this->RelativeDamage_Infantry;
		break;
	case AbstractType::Terrain:
		relative = this->RelativeDamage_Terrain;
		break;
	default:
		return 0;
	}

	if(!relative) {
		return 0;
	}

	if(relative < 0) {
		return relative * pVictim->Health / -100;
	}

	if(auto const pType = pVictim->GetType()) {
		return relative * pType->Strength / 100;
	}

	return 0;
}

//! Gets whether this warhead culls the victim outright.
/*!
	Culling is limited to the health band Culling.BelowHealth allows for the attacker's
	rank. A positive value is a health percentage, a non-positive one the negated damage
	state the victim may be in at most. Culling.Chance then rolls for the kill.

	\param pAttacker The unit firing this warhead.
	\param pVictim The object the warhead is about to damage.

	\returns True if the victim is to be killed, false otherwise.
*/
bool WarheadTypeExt::ExtData::ApplyCulling(TechnoClass* const pAttacker, ObjectClass* const pVictim) const {
	if(!this->OwnerObject()->Culling) {
		return false;
	}

	auto const rank = pAttacker->Veterancy.Veterancy;
	auto const elite = (rank >= 2.0f);
	auto const veteran = !elite && (rank >= 1.0f);

	auto const below = elite
		? this->Culling_BelowHealth.Elite
		: (veteran ? this->Culling_BelowHealth.Veteran : this->Culling_BelowHealth.Rookie);

	if(below > 0) {
		auto const health = static_cast<int>(pVictim->GetHealthPercentage() * 100.0);
		if(health > below) {
			return false;
		}
	} else if(static_cast<int>(pVictim->GetHealthStatus()) > -below) {
		return false;
	}

	auto const chance = elite
		? this->Culling_Chance.Elite
		: (veteran ? this->Culling_Chance.Veteran : this->Culling_Chance.Rookie);

	return chance < 0 || ScenarioClass::Instance->Random.RandomRanged(0, 99) < chance;
}

// Request #733: KillDriver/"Jarmen Kell"
/*! This function checks if the KillDriver effect should be applied, and, if so, applies it.
	\param pSource Pointer to the firing unit
	\param pVictim Pointer to the target unit
	\return true if the effect was applied, false if not
	\author Renegade & AlexB
	\date 05.04.10
	\todo This needs to be refactored to work with the generic warhead SW. I want to create a generic cellspread function first.
*/
bool WarheadTypeExt::ExtData::applyKillDriver(
	TechnoClass* const pSource, AbstractClass* const pVictim) const
{
	if(!pSource || !this->KillDriver) {
		return false;
	}

	auto const pTarget = abstract_cast<FootClass*>(pVictim);

	if(!pTarget) {
		return false;
	}

	if(!WarheadTypeExt::CanAffectTarget(pTarget, pSource->Owner, this->OwnerObject())) {
		return false;
	}

	auto const pTargetExt = TechnoExt::ExtMap.Find(pTarget);

	if(!pTargetExt->IsDriverKillable(this->KillDriver_KillBelowPercent)) {
		return false;
	}

	// the driver may survive this one
	if(ScenarioClass::Instance->Random.RandomRanged(1, 0x7FFFFFFF)
		* 4.656612873077393e-10 > this->KillDriver_Chance)
	{
		return false;
	}

	// get the new owner
	auto const pInvoker = pSource->Owner;
	auto pOwner = HouseExt::GetHouseKind(this->KillDriver_Owner, false,
		nullptr, pInvoker, pInvoker, pTarget->Owner);
	if(!pOwner) {
		pOwner = HouseClass::FindSpecial();
	}

	if(!pOwner) {
		return false;
	}

	return pTargetExt->ApplyKillDriver(
		pOwner, pSource, this->KillDriver_RemoveVeterancy);
}

//AttachedEffects, request #1573, #255
//copy-pasted from AlexB's applyIC
//since CellSpread effect is needed due to MO's proposed cloak SW (which is the reason why I was bugged with this), it has it.
//Graion Dilach, ~2011-10-14... I forgot the exact date :S

void WarheadTypeExt::ExtData::applyAttachedEffect(const CoordStruct &coords, HouseClass* const pInvoker) {
	if(this->AttachedEffect.Duration != 0) {
		// set of affected objects. every object can be here only once.
		const auto items = Helpers::Alex::getCellSpreadItems(coords, this->OwnerObject()->CellSpread, true);

		// affect each object
		for(const auto curTechno : items) {
			// don't attach to dead
			if(!curTechno || curTechno->InLimbo || !curTechno->IsAlive || !curTechno->Health) {
				continue;
			}

			if(pInvoker && !WarheadTypeExt::CanAffectTarget(curTechno, pInvoker, this->OwnerObject())) {
				continue;
			}

			if(std::abs(this->GetVerses(curTechno->GetTechnoType()->Armor).Verses) < 0.001) {
				continue;
			}

			this->AttachedEffect.Attach(curTechno, this->AttachedEffect.Duration, pInvoker);
		}
	}
}

// =============================
// load / save

template <typename T>
void WarheadTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->MindControl_Permanent)
		.Process(this->EMP_Duration)
		.Process(this->EMP_Cap)
		.Process(this->EMP_Sparkles)
		.Process(this->IC_Duration)
		.Process(this->IC_Cap)
		.Process(this->IC_Flash)
		.Process(this->Verses)
		.Process(this->DeployedDamage)
		.Process(this->Temporal_HealthFactor)
		.Process(this->Temporal_WarpAway)
		.Process(this->AffectsEnemies)
		.Process(this->AffectsOwner)
		.Process(this->InfDeathAnim)
		.Process(this->NukeFlashDuration)
		.Process(this->PreImpactAnim)
		.Process(this->PreImpactAnim_Moves)
		.Process(this->KillDriver)
		.Process(this->KillDriver_KillBelowPercent)
		.Process(this->KillDriver_Chance)
		.Process(this->KillDriver_Owner)
		.Process(this->KillDriver_RemoveVeterancy)
		.Process(this->Malicious)
		.Process(this->PreventScatter)
		.Process(this->BridgeAbsoluteDestroyer)
		.Process(this->CellSpread_MaxAffect)
		.Process(this->DamageAirThreshold)
		.Process(this->AttachedEffect)
		.Process(this->UnitLost_Suppress)
		.Process(this->SuppressDeathWeapon_Vehicles)
		.Process(this->SuppressDeathWeapon_Infantry)
		.Process(this->SuppressDeathWeapon)
		.Process(this->RelativeDamage)
		.Process(this->RelativeDamage_Buildings)
		.Process(this->RelativeDamage_Aircraft)
		.Process(this->RelativeDamage_Infantry)
		.Process(this->RelativeDamage_Vehicles)
		.Process(this->RelativeDamage_Terrain)
		.Process(this->Sonar_Duration)
		.Process(this->DisableWeapons_Duration)
		.Process(this->Flash_Duration)
		.Process(this->IonCannon)
		.Process(this->IonCannon_Rock)
		.Process(this->Ripple_Radius)
		.Process(this->IonCannon_Blast)
		.Process(this->IonCannon_Beam)
		.Process(this->IonCannon_Warhead)
		.Process(this->IonCannon_Damage)
		.Process(this->EffectsRequireDamage)
		.Process(this->EffectsRequireVerses)
		.Process(this->AllowZeroDamage)
		.Process(this->DieSound_Override)
		.Process(this->VoiceDie_Override);
}

void WarheadTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<WarheadTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void WarheadTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<WarheadTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool WarheadTypeExt::LoadGlobals(AresStreamReader& Stm) {
	return Stm
		.Process(Temporal_WH)
		.Process(EMP_WH)
		.Process(IonExt)
		.Success();
}

bool WarheadTypeExt::SaveGlobals(AresStreamWriter& Stm) {
	return Stm
		.Process(Temporal_WH)
		.Process(EMP_WH)
		.Process(IonExt)
		.Success();
}

// =============================
// container

WarheadTypeExt::ExtContainer::ExtContainer() : Container("WarheadTypeClass") {
}

WarheadTypeExt::ExtContainer::~ExtContainer() = default;

void WarheadTypeExt::ExtContainer::InvalidatePointer(void* ptr, bool bRemoved) {
	AnnounceInvalidPointer(WarheadTypeExt::Temporal_WH, ptr);
}

// =============================
// container hooks

DEFINE_HOOK(0x75D1A9, WarheadTypeClass_CTOR, 0x7)
{
	GET(WarheadTypeClass*, pItem, EBP);

	WarheadTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x75E5C8, WarheadTypeClass_SDDTOR, 0x6)
{
	GET(WarheadTypeClass*, pItem, ESI);

	WarheadTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x75E2C0, WarheadTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x75E0C0, WarheadTypeClass_SaveLoad_Prefix, 0x8)
{
	GET_STACK(WarheadTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	WarheadTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x75E2AE, WarheadTypeClass_Load_Suffix, 0x7)
{
	WarheadTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x75E39C, WarheadTypeClass_Save_Suffix, 0x5)
{
	WarheadTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x75DEAF, WarheadTypeClass_LoadFromINI, 0x5)
DEFINE_HOOK(0x75DEA0, WarheadTypeClass_LoadFromINI, 0x5)
{
	GET(WarheadTypeClass*, pItem, ESI);
	GET_STACK(CCINIClass*, pINI, 0x150);

	WarheadTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

static_assert(sizeof(WarheadTypeExt::ExtData) == 0x180, "WarheadTypeExt::ExtData must match the 3.0p1 layout");
static_assert(sizeof(WarheadTypeExt::VersesData) == 0x10, "WarheadTypeExt::VersesData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the middle and the end of the block.
static_assert(offsetof(WarheadTypeExt::ExtData, MindControl_Permanent) == 0x008, "WarheadTypeExt::ExtData layout slipped");
static_assert(offsetof(WarheadTypeExt::ExtData, Verses) == 0x024, "WarheadTypeExt::ExtData layout slipped");
static_assert(offsetof(WarheadTypeExt::ExtData, DamageAirThreshold) == 0x080, "WarheadTypeExt::ExtData layout slipped");
static_assert(offsetof(WarheadTypeExt::ExtData, AttachedEffect) == 0x088, "WarheadTypeExt::ExtData layout slipped");
static_assert(offsetof(WarheadTypeExt::ExtData, RelativeDamage) == 0x0E0, "WarheadTypeExt::ExtData layout slipped");
static_assert(offsetof(WarheadTypeExt::ExtData, Ripple_Radius) == 0x108, "WarheadTypeExt::ExtData layout slipped");
static_assert(offsetof(WarheadTypeExt::ExtData, Culling_Chance) == 0x150, "WarheadTypeExt::ExtData layout slipped");
