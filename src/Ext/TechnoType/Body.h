#pragma once

#include <TechnoTypeClass.h>
#include <ParticleSystemTypeClass.h>

#include "../../Ares.h"
#include "../_Container.hpp"
#include "../../Utilities/Template.h"
#include "../../Utilities/Constructs.h"
#include "../../Misc/AttachEffect.h"

#include <FileSystem.h>

#include <bitset>

class BuildingTypeClass;
class HouseTypeClass;
class VocClass;
class VoxClass;
class WarheadTypeClass;

// the promotion abilities a TechnoType can be given, one flag each
enum class AresAbility {
	EMPImmune = 0,
	RadImmune = 1,
	ProtectedDriver = 2,
	Unwarpable = 3,
	PoisonImmune = 4,
	PsionicsImmune = 5,
	PsionicWeaponImmune = 6,

	count = 7
};

struct AbilityFlags {
	bool operator [] (AresAbility ability) const {
		return this->Flags[static_cast<size_t>(ability)];
	}

	void Set(AresAbility ability, bool value) {
		this->Flags[static_cast<size_t>(ability)] = value;
	}

	void Read(INI_EX &parser, const char* pSection, const char* pKey);

	bool Flags[static_cast<size_t>(AresAbility::count)] {};
};

class TechnoTypeExt
{
public:
	using base_type = TechnoTypeClass;

	enum class SpotlightAttachment {
		Body, Turret, Barrel
	};

	class ExtData final : public Extension<TechnoTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x44444444;

		ValueableVector<InfantryTypeClass *> Survivors_Pilots;
		Promotable<int> Survivors_PilotChance;
		Promotable<int> Survivors_PassengerChance;
		// new on 28.09.09 for #631
		Valueable<int> Survivors_PilotCount; //!< Defines the number of pilots inside this vehicle if Crewed=yes; maximum number of pilots who can survive. Defaults to 0 if Crewed=no; defaults to 1 if Crewed=yes. // NOTE: Flag in INI is called Survivor.Pilots
		Nullable<int> Crew_TechnicianChance;
		Nullable<int> Crew_EngineerChance;

		// animated cameos
		//int Cameo_Interval;
		//int Cameo_CurrentFrame;
		//CDTimerClass Cameo_Timer;

		std::vector<DynamicVectorClass<int>> PrerequisiteLists;
		DynamicVectorClass<int> PrerequisiteNegatives;
		DWORD PrerequisiteTheaters;

		mutable OptionalStruct<bool, true> GenericPrerequisite;

		// new secret lab
		DWORD Secret_RequiredHouses;
		DWORD Secret_ForbiddenHouses;

		bool Is_Deso;
		bool Is_Deso_Radiation;
		bool Is_Cow;
		bool Is_Spotlighted;

		// spotlights
		int Spot_Height;
		int Spot_Distance;
		SpotlightAttachment Spot_AttachedTo;
		bool Spot_DisableR;
		bool Spot_DisableG;
		bool Spot_DisableB;
		bool Spot_DisableColor;

		bool Is_Bomb;

		// storage for the weapon slots the game has no room for
		std::vector<WeaponStruct> Weapons;
		std::vector<WeaponStruct> EliteWeapons;
		std::vector<VoxelStruct> Turrets;
		std::vector<VoxelStruct> Barrels;
		std::vector<int> WeaponTurretIndex;
		std::vector<CSFText> WeaponUINames;

		Promotable<SHPStruct *> Insignia;
		Promotable<int> InsigniaFrame;
		Nullable<bool> Insignia_ShowEnemy;

		Valueable<AnimTypeClass*> Parachute_Anim;

		// new on 08.11.09 for #342 (Operator=)
		ValueableVector<InfantryTypeClass *> Operator; //!< The InfantryTypes required to be a passenger of this unit in order for it to work. Defaults to empty. \sa TechnoClass_Update_CheckOperators, bool IsAPromiscuousWhoreAndLetsAnyoneRideIt
		bool IsAPromiscuousWhoreAndLetsAnyoneRideIt; //!< If this is true, Operator= is not checked, and the object will work with any passenger, provided there is one. \sa ValueableVector<InfantryTypeClass *> Operator

		ValueableVector<TechnoTypeClass*> InitialPayload_Types;
		ValueableVector<int> InitialPayload_Nums;

		CustomPalette CameoPal;

		std::bitset<32> RequiredStolenTech;

		AbilityFlags VeteranAbilities;
		AbilityFlags EliteAbilities;

		Nullable<bool> ImmuneToEMP;
		int EMP_Threshold;
		Valueable<double> EMP_Modifier;
		Nullable<AnimTypeClass*> EMP_Sparkles;

		Valueable<double> IronCurtain_Modifier;
		Valueable<double> ForceShield_Modifier;

		Valueable<bool> Chronoshift_Allow;
		Valueable<bool> Chronoshift_IsVehicle;
		Valueable<bool> Chronoshift_Crushable;

		// new on 05.04.10 for #733 (KillDriver/"Jarmen Kell")
		Valueable<bool> ProtectedDriver; //!< Whether the driver of this vehicle cannot be killed, i.e. whether this vehicle is immune to KillDriver. Request #733.
		Nullable<double> ProtectedDriver_MinHealth; //!< The health level the unit has to be below so the driver can be killed
		Valueable<bool> CanDrive; //!< Whether this TechnoType can act as the driver of vehicles whose driver has been killed. Request #733.
		Valueable<bool> CanBeDriven; //!< Whether this vehicle can be taken over after its driver has been killed.

		Valueable<bool> AlternateTheaterArt;

		Valueable<bool> PassengersGainExperience;
		Valueable<bool> ExperienceFromPassengers;
		Valueable<double> PassengerExperienceModifier;
		Valueable<double> MindControlExperienceSelfModifier;
		Valueable<double> MindControlExperienceVictimModifier;
		Valueable<double> SpawnExperienceOwnerModifier;
		Valueable<double> SpawnExperienceSpawnModifier;
		Valueable<bool> ExperienceFromAirstrike;
		Valueable<double> AirstrikeExperienceModifier;

		ValueableIdx<VocClass> VoiceRepair;
		NullableIdx<VocClass> VoiceAirstrikeAttack;
		NullableIdx<VocClass> VoiceAirstrikeAbort;

		ValueableIdx<VocClass> HijackerEnterSound;
		ValueableIdx<VocClass> HijackerLeaveSound;
		Valueable<int> HijackerKillPilots;
		Valueable<bool> HijackerBreakMindControl;
		Valueable<bool> HijackerAllowed;
		Valueable<bool> HijackerOneTime;

		Valueable<UnitTypeClass *> WaterImage;

		VoxelStruct NoSpawnAltImage; //!< The spawn-less body voxel, so it does not have to share storage with the turret.

		NullableIdx<VocClass> CloakSound;
		NullableIdx<VocClass> DecloakSound;
		Valueable<bool> CloakPowered;
		Valueable<bool> CloakDeployed;
		Valueable<bool> CloakAllowed;
		Nullable<int> CloakStages;

		Valueable<bool> SensorArray_Warn;

		AresPCXFile CameoPCX;
		AresPCXFile AltCameoPCX;

		AresFixedString<0x20> GroupAs;

		Valueable<bool> CanBeReversed;
		Valueable<TechnoTypeClass*> ReversedAs; //!< The type this unit is reverse engineered as, or NULL for the unit itself.

		// issue #305
		Valueable<int> RadarJamRadius; //!< Distance in cells to scan for & jam radars

		// issue #1208
		Valueable<bool> PassengerTurret; //!< Whether this unit's turret changes based on the number of people in its passenger hold.

		// issue #617
		ValueableVector<BuildingTypeClass*> PoweredBy;  //!< The buildingtype this unit is powered by or NULL.

		//issue #1623
		AttachEffectTypeClass AttachedTechnoEffect; //The AttachedEffect which should been on the Techno from the start.

		ValueableVector<BuildingTypeClass const*> BuiltAt;
		Valueable<bool> Cloneable;
		ValueableVector<BuildingTypeClass *> ClonedAt;
		Valueable<TechnoTypeClass*> ClonedAs; //!< The type this unit is cloned as, or NULL for the unit itself.

		Nullable<bool> CarryallAllowed;
		Nullable<int> CarryallSizeLimit;

		Valueable<bool> ImmuneToAbduction; //680, 1362

		ValueableVector<HouseTypeClass *> FactoryOwners;
		ValueableVector<HouseTypeClass *> ForbiddenFactoryOwners;
		Valueable<bool> FactoryOwners_HaveAllPlans;
		Valueable<bool> FactoryOwners_HasAllPlans;

		Valueable<bool> GattlingCyclic;

		Nullable<bool> Crashable;
		Valueable<bool> CrashSpin;
		Valueable<int> AirRate;

		Valueable<bool> CivilianEnemy;

		// custom missiles
		Valueable<bool> IsCustomMissile;
		Valueable<RocketStruct> CustomMissileData;
		Valueable<WarheadTypeClass*> CustomMissileWarhead;
		Valueable<WarheadTypeClass*> CustomMissileEliteWarhead;
		Valueable<AnimTypeClass*> CustomMissileTakeoffAnim;
		Valueable<AnimTypeClass*> CustomMissileTrailerAnim;
		Valueable<int> CustomMissileTrailerSeparation;
		Valueable<WeaponTypeClass*> CustomMissileWeapon;
		Valueable<WeaponTypeClass*> CustomMissileEliteWeapon;

		// tiberium related
		Nullable<bool> TiberiumProof;
		Nullable<bool> TiberiumRemains;
		Valueable<bool> TiberiumSpill;
		Nullable<int> TiberiumTransmogrify;

		// refinery and storage related
		Valueable<bool> Refinery_UseStorage;

		ValueableIdx<VoxClass> EVA_UnitLost;

		Valueable<bool> Drain_Local;
		Valueable<int> Drain_Amount;

		// smoke when damaged
		Nullable<int> SmokeChanceRed;
		Nullable<int> SmokeChanceDead;
		Valueable<AnimTypeClass*> SmokeAnim;

		// hunter seeker
		Nullable<int> HunterSeekerDetonateProximity;
		Nullable<int> HunterSeekerDescendProximity;
		Nullable<int> HunterSeekerAscentSpeed;
		Nullable<int> HunterSeekerDescentSpeed;
		Nullable<int> HunterSeekerEmergeSpeed;
		Valueable<bool> HunterSeekerIgnore;

		// super weapon
		Nullable<int> DesignatorRange;
		Nullable<int> InhibitorRange;

		// particles
		Nullable<bool> DamageSparks;

		NullableVector<ParticleSystemTypeClass*> ParticleSystems_DamageSmoke;
		NullableVector<ParticleSystemTypeClass*> ParticleSystems_DamageSparks;

		// berserk
		Nullable<double> BerserkROFMultiplier;
		Nullable<bool> ImmuneToBerserk;

		// assault options
		Valueable<int> AssaulterLevel;

		// crushing
		Valueable<bool> OmniCrusher_Aggressive;
		Promotable<int> CrushDamage;
		Nullable<WarheadTypeClass*> CrushDamageWarhead;

		Nullable<double> ReloadRate;

		Valueable<int> ReloadAmount;
		Nullable<int> EmptyReloadAmount;
		Valueable<int> NoAmmoAmount;
		Valueable<int> NoAmmoWeapon;

		Valueable<bool> Saboteur;

		Valueable<bool> CanPassiveAcquire_Guard;
		Valueable<bool> CanPassiveAcquire_Cloak;

		Nullable<double> SelfHealing_Rate;
		Promotable<int> SelfHealing_Amount;
		Promotable<double> SelfHealing_Max;
		Valueable<int> SelfHealing_CombatDelay;

		ValueableVector<TechnoTypeClass*> PassengersWhitelist;
		ValueableVector<TechnoTypeClass*> PassengersBlacklist;
		Valueable<bool> Passengers_BySize;

		Valueable<bool> NoManualUnload;
		Valueable<bool> NoManualFire;
		Valueable<bool> NoManualEnter;
		Valueable<bool> NoSelfGuardArea;

		Nullable<CSFText> EnemyUIName;

		// bounty
		Promotable<int> Bounty_Value;
		Valueable<bool> Bounty;
		Nullable<bool> Bounty_Display;

		// promotion
		Valueable<bool> Promote_IncludePassengers;
		NullableIdx<VocClass> Promote_VeteranSound;
		NullableIdx<VocClass> Promote_EliteSound;
		Nullable<int> Promote_VeteranFlash;
		Nullable<int> Promote_EliteFlash;
		ValueableIdx<VoxClass> EVA_VeteranPromoted;
		ValueableIdx<VoxClass> EVA_ElitePromoted;
		Valueable<TechnoTypeClass*> Promote_VeteranType;
		Valueable<TechnoTypeClass*> Promote_EliteType;
		Valueable<double> Promote_VeteranExperience;
		Valueable<double> Promote_EliteExperience;

		Valueable<double> FactoryPlant_Multiplier;

		// digging in and out
		NullableIdx<VocClass> DigInSound;
		NullableIdx<VocClass> DigOutSound;
		Nullable<AnimTypeClass*> DigInAnim;
		Nullable<AnimTypeClass*> DigOutAnim;

		// falling
		Valueable<int> FallRate_Parachute;
		Valueable<int> FallRate_NoParachute;
		Nullable<int> FallRate_ParachuteMax;
		Nullable<int> FallRate_NoParachuteMax;

		Nullable<int> TurretROT;

		// cursors
		Valueable<MouseCursorType> Cursor_Deploy;
		Valueable<MouseCursorType> Cursor_NoDeploy;
		Valueable<MouseCursorType> Cursor_Enter;
		Valueable<MouseCursorType> Cursor_NoEnter;
		Valueable<MouseCursorType> Cursor_Move;
		Valueable<MouseCursorType> Cursor_NoMove;

		// build time
		Nullable<double> BuildTime_Speed;
		Nullable<int> BuildTime_Cost;
		Nullable<double> BuildTime_LowPowerPenalty;
		Nullable<double> BuildTime_MinLowPower;
		Nullable<double> BuildTime_MaxLowPower;
		Nullable<double> BuildTime_MultipleFactory;

		Valueable<TechnoTypeClass*> FakeOf;

		Nullable<int> DeployDir;

		// type conversion
		Valueable<TechnoTypeClass*> Convert_Deploy;
		Valueable<TechnoTypeClass*> Convert_Water;
		Valueable<TechnoTypeClass*> Convert_Land;
		Valueable<TechnoTypeClass*> Convert_Script;

		// harvesting
		Nullable<Leptons> Harvester_LongScan;
		Nullable<Leptons> Harvester_ShortScan;
		Nullable<Leptons> Harvester_ScanCorrection;
		Nullable<int> Harvester_TooFarDistance;
		Nullable<int> Harvester_KickDelay;

		Nullable<bool> Unsellable;
		Nullable<bool> KeepAlive;

		Nullable<int> RadialIndicatorRadius;

		Valueable<int> GapRadiusInCells;
		Valueable<int> SuperGapRadiusInCells;

		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass, ExtData>(OwnerObject),
			Survivors_PilotChance(-1),
			Survivors_PassengerChance(-1),
			Survivors_PilotCount(-1),
			Crew_TechnicianChance(),
			Crew_EngineerChance(),
			PrerequisiteTheaters(0xFFFFFFFF),
			Secret_RequiredHouses(0xFFFFFFFF),
			Secret_ForbiddenHouses(0),
			Is_Deso(false),
			Is_Deso_Radiation(false),
			Is_Cow(false),
			Is_Spotlighted(false),
			Spot_Height(430),
			Spot_Distance(1024),
			Spot_AttachedTo(SpotlightAttachment::Body),
			Spot_DisableR(false),
			Spot_DisableG(false),
			Spot_DisableB(false),
			Spot_DisableColor(false),
			Is_Bomb(false),
			Insignia(nullptr),
			InsigniaFrame(-1),
			Insignia_ShowEnemy(),
			Parachute_Anim(nullptr),
			IsAPromiscuousWhoreAndLetsAnyoneRideIt(false),
			CameoPal(),
			RequiredStolenTech(0ull),
			VeteranAbilities(),
			EliteAbilities(),
			EMP_Threshold(-1),
			EMP_Modifier(1.0),
			IronCurtain_Modifier(1.0),
			ForceShield_Modifier(1.0),
			Chronoshift_Allow(true),
			Chronoshift_IsVehicle(false),
			Chronoshift_Crushable(true),
			ProtectedDriver(false),
			CanDrive(false),
			CanBeDriven(true),
			AlternateTheaterArt(false),
			PassengersGainExperience(false),
			ExperienceFromPassengers(true),
			PassengerExperienceModifier(1.0),
			MindControlExperienceSelfModifier(0.0),
			MindControlExperienceVictimModifier(1.0),
			SpawnExperienceOwnerModifier(0.0),
			SpawnExperienceSpawnModifier(1.0),
			ExperienceFromAirstrike(false),
			AirstrikeExperienceModifier(1.0),
			VoiceRepair(-1),
			HijackerEnterSound(-1),
			HijackerLeaveSound(-1),
			HijackerKillPilots(0),
			HijackerBreakMindControl(true),
			HijackerAllowed(true),
			HijackerOneTime(false),
			WaterImage(nullptr),
			NoSpawnAltImage(),
			CloakSound(),
			DecloakSound(),
			CloakPowered(false),
			CloakDeployed(false),
			CloakAllowed(true),
			CloakStages(),
			SensorArray_Warn(true),
			CanBeReversed(true),
			ReversedAs(nullptr),
			RadarJamRadius(0),
			PassengerTurret(false),
			AttachedTechnoEffect(OwnerObject),
			Cloneable(true),
			ClonedAs(nullptr),
			CarryallAllowed(),
			CarryallSizeLimit(),
			ImmuneToAbduction(false),
			FactoryOwners_HaveAllPlans(false),
			FactoryOwners_HasAllPlans(false),
			GattlingCyclic(false),
			CrashSpin(true),
			IsCustomMissile(false),
			CustomMissileData(),
			CustomMissileWarhead(nullptr),
			CustomMissileEliteWarhead(nullptr),
			CustomMissileTakeoffAnim(nullptr),
			CustomMissileTrailerAnim(nullptr),
			CustomMissileTrailerSeparation(3),
			TiberiumProof(),
			TiberiumRemains(),
			TiberiumSpill(false),
			TiberiumTransmogrify(),
			Refinery_UseStorage(false),
			EVA_UnitLost(-1),
			Drain_Local(false),
			Drain_Amount(0),
			SmokeChanceRed(),
			SmokeChanceDead(),
			SmokeAnim(nullptr),
			HunterSeekerDetonateProximity(),
			HunterSeekerDescendProximity(),
			HunterSeekerAscentSpeed(),
			HunterSeekerDescentSpeed(),
			HunterSeekerEmergeSpeed(),
			HunterSeekerIgnore(false),
			DesignatorRange(),
			InhibitorRange(),
			ImmuneToBerserk(),
			OmniCrusher_Aggressive(true),
			ReloadAmount(1),
			NoAmmoAmount(0),
			NoAmmoWeapon(-1),
			CanPassiveAcquire_Guard(true),
			CanPassiveAcquire_Cloak(true),
			SelfHealing_Amount(1),
			SelfHealing_Max(1.0),
			SelfHealing_CombatDelay(0),
			Passengers_BySize(true),
			NoSelfGuardArea(false),
			EnemyUIName(),
			Bounty_Value(0),
			Bounty(false),
			Bounty_Display(),
			Promote_IncludePassengers(false),
			EVA_VeteranPromoted(-1),
			EVA_ElitePromoted(-1),
			Promote_VeteranType(nullptr),
			Promote_EliteType(nullptr),
			Promote_VeteranExperience(0.0),
			Promote_EliteExperience(0.0),
			FactoryPlant_Multiplier(1.0),
			FallRate_Parachute(1),
			FallRate_NoParachute(1),
			Cursor_Deploy(MouseCursorType::Deploy),
			Cursor_NoDeploy(MouseCursorType::NoDeploy),
			Cursor_Enter(MouseCursorType::Enter),
			Cursor_NoEnter(MouseCursorType::NoEnter),
			Cursor_Move(MouseCursorType::Move),
			Cursor_NoMove(MouseCursorType::NoMove),
			FakeOf(nullptr),
			DeployDir(),
			Convert_Deploy(nullptr),
			Convert_Water(nullptr),
			Convert_Land(nullptr),
			Convert_Script(nullptr),
			Unsellable(),
			KeepAlive(),
			RadialIndicatorRadius(),
			GapRadiusInCells(0),
			SuperGapRadiusInCells(0)
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);
		void Initialize(CCINIClass* pINI);

		void ReadWeapons(CCINIClass* pINI);
		void LoadTurrets(CCINIClass* pINI);

		WeaponStruct* GetWeapon(int index, bool elite);
		int* GetWeaponTurretIndex(int index);
		VoxelStruct* GetTurretVoxel(int index);
		VoxelStruct* GetBarrelVoxel(int index);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		bool CameoIsElite(HouseClass const* pHouse) const;

		bool CanBeBuiltAt(BuildingTypeClass const* pFactoryType) const;

		bool CarryallCanLift(UnitClass * Target);

		const char* GetSelectionGroupID() const;

		bool IsGenericPrerequisite() const;

		bool HasAbility(AresAbility ability, VeterancyStruct const& veterancy) const;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static const char* GetSelectionGroupID(ObjectTypeClass* pType);
	static bool HasSelectionGroupID(ObjectTypeClass* pType, const char* pID);

	//static void ReadWeapon(WeaponStruct *pWeapon, const char *prefix, const char *section, CCINIClass *pINI);
};
