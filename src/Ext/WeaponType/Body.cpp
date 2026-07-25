#include "Body.h"
#include <AbstractClass.h>
#include <BulletClass.h>
#include <EBolt.h>
#include <FootClass.h>
#include <TechnoTypeClass.h>
#include <LocomotionClass.h>
#include <WaveClass.h>

#include <new>
#include "../WarheadType/Body.h"
#include "../Techno/Body.h"
#include "../TechnoType/Body.h"
#include "../../Utilities/TemplateDef.h"

WeaponTypeExt::ExtContainer WeaponTypeExt::ExtMap;

WeaponTypeExt::WaveColorData WeaponTypeExt::WaveColors;

namespace
{
	struct WaveColorDefaults
	{
		Vector3D<int> Intensity;
		Vector3D<int> Color;
	};

	const WaveColorDefaults DefaultWaveColorsLaser = {{0, 0, 0}, {0x40, 0x00, 0x60}};
	const WaveColorDefaults DefaultWaveColorsSonic = {{0, 0x100, 0x100}, {0, 0, 0}};
	const WaveColorDefaults DefaultWaveColorsMagBeam = {{0x80, 0, 0x400}, {0, 0, 0}};
}

AresMap<BombClass*, const WeaponTypeExt::ExtData*> WeaponTypeExt::BombExt;
AresMap<WaveClass*, const WeaponTypeExt::ExtData*> WeaponTypeExt::WaveExt;
AresMap<EBolt*, const WeaponTypeExt::ExtData*> WeaponTypeExt::BoltExt;
AresMap<RadSiteClass*, const WeaponTypeExt::ExtData*> WeaponTypeExt::RadSiteExt;

void WeaponTypeExt::ExtData::Initialize(CCINIClass* pINI)
{
	this->Wave_Reverse[idxVehicle] = this->OwnerObject()->IsMagBeam;
};

void WeaponTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	auto pThis = this->OwnerObject();
	const char * section = pThis->get_ID();
	if(!pINI->GetSection(section)) {
		return;
	}

	INI_EX exINI(pINI);

	this->IsDetachedRailgun.Read(exINI, section, "IsDetachedRailgun");

	this->Beam_Duration.Read(exINI, section, "Beam.Duration");
	this->Beam_Amplitude.Read(exINI, section, "Beam.Amplitude");
	this->Beam_IsHouseColor.Read(exINI, section, "Beam.IsHouseColor");
	this->Beam_Color.Read(exINI, section, "Beam.Color");

	this->Wave_IsLaser.Read(exINI, section, "Wave.IsLaser");
	this->Wave_IsBigLaser.Read(exINI, section, "Wave.IsBigLaser");
	this->Wave_IsHouseColor.Read(exINI, section, "Wave.IsHouseColor");
	this->Wave_Intensity.Read(exINI, section, "Wave.Intensity");
	this->Wave_Color.Read(exINI, section, "Wave.Color");

	this->Wave_Reverse[idxVehicle].Read(exINI, section, "Wave.ReverseAgainstVehicles");
	this->Wave_Reverse[idxAircraft].Read(exINI, section, "Wave.ReverseAgainstAircraft");
	this->Wave_Reverse[idxBuilding].Read(exINI, section, "Wave.ReverseAgainstBuildings");
	this->Wave_Reverse[idxInfantry].Read(exINI, section, "Wave.ReverseAgainstInfantry");
	this->Wave_Reverse[idxOther].Read(exINI, section, "Wave.ReverseAgainstOthers");

	this->Bolt_Color1.Read(exINI, section, "Bolt.Color1");
	this->Bolt_Color2.Read(exINI, section, "Bolt.Color2");
	this->Bolt_Color3.Read(exINI, section, "Bolt.Color3");
	this->Bolt_ParticleSystem.Read(exINI, section, "Bolt.ParticleSystem");

	this->LaserThickness.Read(exINI, section, "LaserThickness");

	this->Ivan_DeathBomb.Read(exINI, section, "IvanBomb.DeathBomb");
	this->Ivan_DeathBombOnAllies.Read(exINI, section, "IvanBomb.DeathBombOnAllies");
	this->Ivan_KillsBridges.Read(exINI, section, "IvanBomb.DestroysBridges");
	this->Ivan_Detachable.Read(exINI, section, "IvanBomb.Detachable");
	this->Ivan_Damage.Read(exINI, section, "IvanBomb.Damage");
	this->Ivan_Delay.Read(exINI, section, "IvanBomb.Delay");
	this->Ivan_FlickerRate.Read(exINI, section, "IvanBomb.FlickerRate");
	this->Ivan_TickingSound.Read(exINI, section, "IvanBomb.TickingSound");
	this->Ivan_AttachSound.Read(exINI, section, "IvanBomb.AttachSound");
	this->Ivan_WH.Read(exINI, section, "IvanBomb.Warhead");
	this->Ivan_Image.Read(exINI, section, "IvanBomb.Image");
	this->Ivan_CanDetonateTimeBomb.Read(exINI, section, "IvanBomb.CanDetonateTimeBomb");
	this->Ivan_CanDetonateDeathBomb.Read(exINI, section, "IvanBomb.CanDetonateDeathBomb");
	this->Ivan_DetonateOnSell.Read(exINI, section, "IvanBomb.DetonateOnSell");

	// #680 Chrono Prison
	this->Abductor.Read(exINI, section, "Abductor");
	this->Abductor_ChangeOwner.Read(exINI, section, "Abductor.ChangeOwner");
	this->Abductor_Temporal.Read(exINI, section, "Abductor.Temporal");
	this->Abductor_AbductBelowPercent.Read(exINI, section, "Abductor.AbductBelowPercent");
	this->Abductor_MaxHealth.Read(exINI, section, "Abductor.MaxHealth");
	this->Abductor_AnimType.Read(exINI, section, "Abductor.Anim");

	// brought back from TS
	this->ProjectileRange.Read(exINI, section, "ProjectileRange");

	this->ApplyDamage.Read(exINI, section, "ApplyDamage");

	this->Ammo.Read(exINI, section, "Ammo");

	this->Cursor_Attack.Read(exINI, section, "Cursor.Attack");
	this->Cursor_AttackOutOfRange.Read(exINI, section, "Cursor.AttackOutOfRange");
}

// #680 Chrono Prison / Abductor
/**
	This function checks if an abduction should be performed,
	and, if so, performs it, provided that is possible.

	\author Renegade
	\date 24.08.2010
	\param[in] Bullet The projectile that hits the victim.
	\retval true An abduction was performed. It should be assumed that the target is gone from the map at this point.
	\retval false No abduction was performed. This can be because the weapon is not an abductor, or because an error occurred. The target should still be on the map.
	\todo see if TechnoClass::Transporter needs to be set in here
*/
bool WeaponTypeExt::ExtData::conductAbduction(BulletClass * Bullet) {
	if(!Bullet->Owner
		|| !this->Abduct(Bullet->Owner, abstract_cast<TechnoClass*>(Bullet->Target)))
	{
		return false;
	}

	// ..and neuter the bullet, since it's not supposed to hurt the prisoner after the abduction
	Bullet->Health = 0;
	Bullet->DamageMultiplier = 0;
	Bullet->Limbo();

	return true;
}

bool WeaponTypeExt::ExtData::Abduct(
	TechnoClass* const Attacker, TechnoClass* const pVictim) const
{
	// ensuring a few base parameters
	if(!Attacker || !pVictim || !this->Abductor) {
		return false;
	}

	auto Target = abstract_cast<FootClass*>(pVictim);

	if(!Target) {
		// the target was not a valid passenger type
		return false;
	}

	auto TargetType = Target->GetTechnoType();
	auto TargetTypeExt = TechnoTypeExt::ExtMap.Find(TargetType);
	auto AttackerType = Attacker->GetTechnoType();

	//issue 1362
	if(TargetTypeExt->ImmuneToAbduction) {
		return false;
	}

	if(!WarheadTypeExt::CanAffectTarget(Target, Attacker->Owner, this->OwnerObject()->Warhead)) {
		return false;
	}

	if(Target->IsIronCurtained()) {
		return false;
	}

	//Don't abduct the target if it has more life then the abducting percent
	if(this->Abductor_AbductBelowPercent < Target->GetHealthPercentage()) {
		return false;
	}

	// too tough to be taken away
	if(this->Abductor_MaxHealth > 0 && this->Abductor_MaxHealth < Target->Health) {
		return false;
	}

	// Don't abduct the target if it's too fat in general, or if there's not enough room left in the hold // alternatively, NumPassengers
	if((TargetType->Size > AttackerType->SizeLimit)
		|| (TargetType->Size > (AttackerType->Passengers - Attacker->Passengers.GetTotalSize()))) {
		return false;
	}

	// if we ended up here, the target is of the right type, and the attacker can take it
	// so we abduct the target...

	Target->StopMoving();
	Target->SetDestination(nullptr, true); // Target->UpdatePosition(int) ?
	Target->SetTarget(nullptr);
	Target->CurrentTargets.Clear(); // Target->ShouldLoseTargetNow ?
	Target->SetArchiveTarget(nullptr);
	Target->QueueMission(Mission::Sleep, true);
	Target->MissionAccumulateTime = 0; // don't ask
	Target->unknown_5A0 = 0;
	Target->CurrentGattlingStage = 0;
	Target->SetCurrentWeaponStage(0);

	// the team should not wait for me
	if(Target->BelongsToATeam()) {
		Target->Team->LiberateMember(Target);
	}

	// if this unit is being mind controlled, break the link
	if(auto MindController = Target->MindControlledBy) {
		if(auto MC = MindController->CaptureManager) {
			MC->FreeUnit(Target);
		}
	}

	// if this unit is a mind controller, break the link
	if(Target->CaptureManager) {
		Target->CaptureManager->FreeAll();
	}

	// if this unit is currently in a state of temporal flux, get it back to our time-frame
	if(Target->TemporalTargetingMe) {
		Target->TemporalTargetingMe->Detach();
	}

	//if the target is spawned, detach it from it's spawner
	if(Target->SpawnOwner) {
		TechnoExt::DetachSpecificSpawnee(Target, HouseClass::FindSpecial());
	}

	// if the unit is a spawner, kill the spawns
	if(Target->SpawnManager) {
		Target->SpawnManager->KillNodes();
		Target->SpawnManager->ResetTarget();
	}

	//if the unit is a slave, it should be freed
	if(Target->SlaveOwner) {
		TechnoExt::FreeSpecificSlave(Target, HouseClass::FindSpecial());
	}

	// If the unit is a SlaveManager, free the slaves
	if(auto pSlaveManager = Target->SlaveManager) {
		pSlaveManager->Killed(Attacker);
		pSlaveManager->ZeroOutSlaves();
		Target->SlaveManager->Owner = Target;
	}

	// if we have an abducting animation, play it
	if(this->Abductor_AnimType) {
		GameCreate<AnimClass>(this->Abductor_AnimType, Target->Location);
	}

	// Limbo() below takes the occupation bits down by itself. Doing it here as
	// well re-marks them at GetCoords(), which for a moving target is not the
	// cell the locomotor tracked -- so that cell stays occupied forever.
	Target->ClearPlanningTokens(nullptr);
	Target->Flashing.DurationRemaining = 0;

	//if it's owner meant to be changed, do it here
	if(this->Abductor_ChangeOwner && !TargetType->ImmuneToPsionics) {
		Target->SetOwningHouse(Attacker->Owner);
	}

	if(!Target->Limbo()) {
		Debug::Log(Debug::Severity::Warning, "Abduction: Target unit %p (%s) could not be removed.\n", Target, Target->get_ID());
	}
	Target->OnBridge = false;

	// because we are throwing away the locomotor in a split second, piggybacking
	// has to be stopped. otherwise the object might remain in a weird state.
	while(LocomotionClass::End_Piggyback(Target->Locomotor)) { };

	// throw away the current locomotor and instantiate
	// a new one of the default type for this unit.
	if(auto NewLoco = LocomotionClass::CreateInstance(TargetType->Locomotor)) {
		Target->Locomotor = std::move(NewLoco);
		Target->Locomotor->Link_To_Object(Target);
	}

	// handling for Locomotor weapons: since we took this unit from the Magnetron
	// in an unfriendly way, set these fields here to unblock the unit
	if(Target->IsAttackedByLocomotor || Target->IsLetGoByLocomotor) {
		Target->IsAttackedByLocomotor = false;
		Target->IsLetGoByLocomotor = false;
		Target->FrozenStill = false;
	}

	Target->Transporter = Attacker;
	if(AttackerType->OpenTopped && Target->Owner->IsAlliedWith(Attacker)) {
		Attacker->EnteredOpenTopped(Target);
	}

	if(Attacker->WhatAmI() == AbstractType::Building) {
		Target->Absorbed = true;
	}
	Attacker->AddPassenger(Target);
	Attacker->Undiscover();

	return true;
}

// Plants customizable IvanBombs on a target.
/*
	Plants a bomb and changes the customizable properties. Also, the weapon
	type that planted the bomb is remembered for use in hooks.

	The original Plant function has been changed to not play sounds any more,
	and it allows all kinds of TechnoClass sources, not just infantry.

	\param pSource The bomber techno who plants the bombs.
	\param pTarget The victim to be rigged.

	\author AlexB
	\date 2013-10-28
*/
void WeaponTypeExt::ExtData::PlantBomb(TechnoClass* pSource, ObjectClass* pTarget) const {
	// ensure target isn't rigged already
	if(pTarget && !pTarget->AttachedBomb) {
		BombListClass::Instance.Plant(pSource, pTarget);

		// if target has a bomb, planting was successful
		if(auto pBomb = pTarget->AttachedBomb) {
			WeaponTypeExt::BombExt[pBomb] = this;

			pBomb->DetonationFrame = Unsorted::CurrentFrame + this->Ivan_Delay.Get(RulesClass::Instance->IvanTimedDelay);
			pBomb->TickSound = this->Ivan_TickingSound.Get(RulesClass::Instance->BombTickingSound);

			auto const allied = pSource->Owner->IsAlliedWith(pTarget->GetOwningHouse());

			if(allied ? this->Ivan_DeathBombOnAllies : this->Ivan_DeathBomb) {
				pBomb->DeathBomb = 1;
			}

			int index = this->Ivan_AttachSound.Get(RulesClass::Instance->BombAttachSound);
			if(index != -1 && pSource->Owner->IsControlledByCurrentPlayer()) {
				VocClass::PlayAt(index, pBomb->Target->Location, nullptr);
			}
		}
	}
}

// blows up a bomb attached to an object that is being sold, unless the weapon
// that planted it prefers the bomb to be disarmed silently.
void WeaponTypeExt::DetonateBombOnSell(BombClass* const pBomb)
{
	auto const pData = WeaponTypeExt::BombExt.get_or_default(pBomb);

	if(pData->Ivan_DetonateOnSell) {
		pBomb->Detonate();
	}
}

bool WeaponTypeExt::ExtData::IsWaveReversedAgainst(
	AbstractClass const* const pTarget) const
{
	auto const abs = pTarget->WhatAmI();

	switch(abs)
	{
	case AbstractType::Unit:
		return this->Wave_Reverse[idxVehicle];
	case AbstractType::Aircraft:
		return this->Wave_Reverse[idxAircraft];
	case AbstractType::Building:
		return this->Wave_Reverse[idxBuilding];
	case AbstractType::Infantry:
		return this->Wave_Reverse[idxInfantry];
	default:
		return this->Wave_Reverse[idxOther];
	}
}

ColorStruct WeaponTypeExt::ExtData::GetBeamColor() const {
	auto pThis = this->OwnerObject();

	if(pThis->IsRadBeam || pThis->IsRadEruption) {
		if(pThis->Warhead && pThis->Warhead->Temporal) { //Marshall added the check for Warhead because PrismForwarding.SupportWeapon does not require a Warhead
			// Well, a RadEruption Temporal will look pretty funny, but this is what WW uses
			return this->Beam_Color.Get(RulesClass::Instance->ChronoBeamColor);
		}
	}

	return this->Beam_Color.Get(RulesClass::Instance->RadColor);
}

// the colors a wave is tinted with. the defaults depend on the wave type, thus
// a change of type still picks the appropriate value, unless the modder set the
// colors by hand, in which case those are used.
WeaponTypeExt::WaveColorData WeaponTypeExt::GetWaveColorData(WaveClass* const pWave)
{
	WaveColorData ret = {};

	auto const pData = WeaponTypeExt::WaveExt.get_or_default(pWave);

	if(!pData) {
		return ret;
	}

	auto const pThis = pData->OwnerObject();

	auto const& defaults = pThis->IsMagBeam
		? DefaultWaveColorsMagBeam
		: (pThis->IsSonic ? DefaultWaveColorsSonic : DefaultWaveColorsLaser);

	auto intensity = defaults.Intensity;
	auto color = defaults.Color;

	if(pData->Wave_Intensity.isset()) {
		intensity = pData->Wave_Intensity.Get();
	}

	if(pData->Wave_Color.isset()) {
		color = pData->Wave_Color.Get();
	}

	if(pData->Wave_IsHouseColor && pWave->Owner) {
		auto const& houseColor = pWave->Owner->Owner->Color;
		color = Vector3D<int>{houseColor.R, houseColor.G, houseColor.B};
	}

	if(!intensity.X && !intensity.Y && !intensity.Z
		&& !color.X && !color.Y && !color.Z)
	{
		return ret;
	}

	ret.Intensity = intensity;
	ret.Color = color;
	ret.Modified = true;

	return ret;
}

WORD WeaponTypeExt::ModifyWaveColor(
	WORD const source, int const intensity, const WaveColorData& colors)
{
	ColorStruct current;
	Drawing::Int_To_RGB(source, current);

	// ugly hack to fix byte wraparound problems
	auto const upcolor = [intensity](BYTE const component, int const scale, int const offset) {
		auto const value = component + ((component * scale * intensity) >> 16)
			+ ((offset * intensity) >> 8);
		return static_cast<BYTE>(Math::clamp(value, 0, 255));
	};

	return static_cast<WORD>(Drawing::RGB_To_Int(
		upcolor(current.R, colors.Intensity.X, colors.Color.X),
		upcolor(current.G, colors.Intensity.Y, colors.Color.Y),
		upcolor(current.B, colors.Intensity.Z, colors.Color.Z)));
}

WaveClass* WeaponTypeExt::CreateWave(
	const CoordStruct& crdSrc, const CoordStruct& crdTgt, TechnoClass* const pOwner,
	WaveType const type, AbstractClass* const pTarget, BYTE const idxWeapon,
	const ExtData* const pData)
{
	// the constructor already draws, and drawing needs the extension data
	auto const pWave = static_cast<WaveClass*>(
		YRMemory::AllocateChecked(sizeof(WaveClass)));

	if(pData) {
		WeaponTypeExt::WaveExt[pWave] = pData;
	}

	TechnoExt::ExtMap.Find(pOwner)->idxSlot_Wave = idxWeapon;

	return new(pWave) WaveClass(crdSrc, crdTgt, pOwner, type, pTarget);
}

EBolt* WeaponTypeExt::CreateBolt(WeaponTypeClass* pWeapon) {
	auto pExt = WeaponTypeExt::ExtMap.Find(pWeapon);
	return WeaponTypeExt::CreateBolt(pExt);
}

EBolt* WeaponTypeExt::CreateBolt(WeaponTypeExt::ExtData* pWeapon) {
	auto ret = GameCreate<EBolt>();

	if(ret && pWeapon) {
		WeaponTypeExt::BoltExt[ret] = pWeapon;
	}

	return ret;
}

// =============================
// load / save

template <typename T>
void WeaponTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->IsDetachedRailgun)
		.Process(this->Beam_Color)
		.Process(this->Beam_Duration)
		.Process(this->Beam_Amplitude)
		.Process(this->Beam_IsHouseColor)
		.Process(this->Bolt_Color1)
		.Process(this->Bolt_Color2)
		.Process(this->Bolt_Color3)
		.Process(this->Bolt_ParticleSystem)
		.Process(this->Wave_IsHouseColor)
		.Process(this->Wave_IsLaser)
		.Process(this->Wave_IsBigLaser)
		.Process(this->Wave_Intensity)
		.Process(this->Wave_Color)
		.Process(this->Wave_Reverse)
		.Process(this->LaserThickness)
		.Process(this->Ivan_DeathBomb)
		.Process(this->Ivan_DeathBombOnAllies)
		.Process(this->Ivan_KillsBridges)
		.Process(this->Ivan_Detachable)
		.Process(this->Ivan_Damage)
		.Process(this->Ivan_Delay)
		.Process(this->Ivan_TickingSound)
		.Process(this->Ivan_AttachSound)
		.Process(this->Ivan_WH)
		.Process(this->Ivan_Image)
		.Process(this->Ivan_FlickerRate)
		.Process(this->Ivan_CanDetonateTimeBomb)
		.Process(this->Ivan_CanDetonateDeathBomb)
		.Process(this->Ivan_DetonateOnSell)
		.Process(this->Rad_Type)
		.Process(this->Abductor)
		.Process(this->Abductor_ChangeOwner)
		.Process(this->Abductor_Temporal)
		.Process(this->Abductor_AbductBelowPercent)
		.Process(this->Abductor_MaxHealth)
		.Process(this->Abductor_AnimType)
		.Process(this->ProjectileRange)
		.Process(this->ApplyDamage)
		.Process(this->Ammo)
		.Process(this->Cursor_Attack)
		.Process(this->Cursor_AttackOutOfRange);
}

void WeaponTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<WeaponTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void WeaponTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<WeaponTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool WeaponTypeExt::LoadGlobals(AresStreamReader& Stm) {
	return Stm
		.Process(BombExt)
		.Process(WaveExt)
		.Process(BoltExt)
		.Process(RadSiteExt)
		.Success();
}

bool WeaponTypeExt::SaveGlobals(AresStreamWriter& Stm) {
	return Stm
		.Process(BombExt)
		.Process(WaveExt)
		.Process(BoltExt)
		.Process(RadSiteExt)
		.Success();
}

// =============================
// container

WeaponTypeExt::ExtContainer::ExtContainer() : Container("WeaponTypeClass") {
}

WeaponTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x771EE9, WeaponTypeClass_CTOR, 0x5)
{
	GET(WeaponTypeClass*, pItem, ESI);

	WeaponTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x77311D, WeaponTypeClass_SDDTOR, 0x6)
{
	GET(WeaponTypeClass*, pItem, ESI);

	WeaponTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x772EB0, WeaponTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x772CD0, WeaponTypeClass_SaveLoad_Prefix, 0x7)
{
	GET_STACK(WeaponTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	WeaponTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x772EA6, WeaponTypeClass_Load_Suffix, 0x6)
{
	WeaponTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x772F8C, WeaponTypeClass_Save, 0x5)
{
	WeaponTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x7729C7, WeaponTypeClass_LoadFromINI, 0x5)
DEFINE_HOOK_AGAIN(0x7729D6, WeaponTypeClass_LoadFromINI, 0x5)
DEFINE_HOOK(0x7729B0, WeaponTypeClass_LoadFromINI, 0x5)
{
	GET(WeaponTypeClass*, pItem, ESI);
	GET_STACK(CCINIClass*, pINI, 0xE4);

	WeaponTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

static_assert(sizeof(WeaponTypeExt::ExtData) == 0x100, "WeaponTypeExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the middle and the end of the block.
static_assert(offsetof(WeaponTypeExt::ExtData, IsDetachedRailgun) == 0x008, "WeaponTypeExt::ExtData layout slipped");
static_assert(offsetof(WeaponTypeExt::ExtData, Wave_Intensity) == 0x03C, "WeaponTypeExt::ExtData layout slipped");
static_assert(offsetof(WeaponTypeExt::ExtData, Wave_Reverse) == 0x05C, "WeaponTypeExt::ExtData layout slipped");
static_assert(offsetof(WeaponTypeExt::ExtData, Ivan_Image) == 0x094, "WeaponTypeExt::ExtData layout slipped");
static_assert(offsetof(WeaponTypeExt::ExtData, Rad_Type) == 0x0AC, "WeaponTypeExt::ExtData layout slipped");
static_assert(offsetof(WeaponTypeExt::ExtData, Abductor_AnimType) == 0x0C4, "WeaponTypeExt::ExtData layout slipped");
static_assert(offsetof(WeaponTypeExt::ExtData, Cursor_AttackOutOfRange) == 0x0D8, "WeaponTypeExt::ExtData layout slipped");
