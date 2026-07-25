#pragma once

#include <algorithm>

#include "../../Misc/AttachEffect.h"
#include "../../Misc/JammerClass.h"
#include "../../Misc/PoweredUnitClass.h"

#include "../../Utilities/Constructs.h"
#include "../../Utilities/Enums.h"

#include "../_Container.hpp"

class AircraftClass;
class AlphaShapeClass;
class BuildingLightClass;
class EBolt;
class HouseClass;
class HouseTypeClass;
class InfantryClass;
struct SHPStruct;
class TemporalClass;
class WarheadTypeClass;

class TechnoExt
{
public:
	using base_type = TechnoClass;

	class ExtData final : public Extension<TechnoClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x55555555;

		// weapon slots fsblargh
		BYTE idxSlot_Wave;
		BYTE idxSlot_Beam;
		BYTE idxSlot_Warp;
		BYTE idxSlot_Parasite;

		BuildingClass *GarrisonedIn; // when infantry garrisons a building, we need a fast way to find said building when damage forwarding kills it

		AnimClass *EMPSparkleAnim;
		Mission EMPLastMission;

		// 305 Radar Jammers
		std::unique_ptr<JammerClass> RadarJam;

		// issue #617 powered units
		std::unique_ptr<PoweredUnitClass> PoweredUnit;

		//#1573, #1623, #255 Stat-modifiers/ongoing animations
		std::vector<AttachEffectClass> AttachedEffects;

		//stuff for #1623
		int AttachedTechnoEffect_Delay;
		// RecreateAnims is +0x30 and isset is +0x31, not the other way round:
		// AttachEffectClass::Update writes the value 2 to +0x31 (0x10059C8A),
		// which only isset ever takes, while +0x30 is written 0 and 1 only.
		bool AttachEffects_RecreateAnims;
		BYTE AttachedTechnoEffect_isset; // 0 = not applied, 1 = applied, 2 = the type has no effect to apply

		TemporalClass * MyOriginalTemporal;

		BuildingLightClass* Spotlight;

		EBolt * MyBolt;

		HouseTypeClass* OriginalHouseType;

		CDTimerClass CloakSkipTimer;
		CDTimerClass DisableWeaponTimer;
		CDTimerClass SelfHealCombatTimer;

		int HijackerHealth;
		HouseClass* HijackerHouse;
		float HijackerVeterancy;

		double AttachEffects_ROFMultiplier;

		//crate fields
		double Crate_FirepowerMultiplier;
		double Crate_ArmorMultiplier;
		double Crate_SpeedMultiplier;
		bool Crate_Cloakable;

		OptionalStruct<bool, true> AltOccupation; // if the unit marks cell occupation flags, this is set to whether it uses the "high" occupation members

		bool Survivors_Done;

		bool DriverKilled;

		bool AlwaysOperated; // this type can never lose its operator, so stop asking

		bool PayloadCreated;

		bool SuppressLossMessage;

		SuperClass* SuperWeapon; // the super weapon somehow attached to this (not provided by this)
		AbstractClass* SuperTarget; // the attached super weapon's target (if any)

		int TechnoValue; // credits waiting to be shown as a flying string
		int TechnoValue_NextDisplayFrame;

		bool TakeVehicleMode; // the team script wants this one to hijack rather than garrison

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass, ExtData>(OwnerObject),
			idxSlot_Wave(0),
			idxSlot_Beam(0),
			idxSlot_Warp(0),
			idxSlot_Parasite(0),
			GarrisonedIn(nullptr),
			EMPSparkleAnim(nullptr),
			EMPLastMission(Mission::None),
			RadarJam(nullptr),
			PoweredUnit(nullptr),
			AttachedTechnoEffect_Delay(0x7FFFFFFF),
			AttachEffects_RecreateAnims(false),
			AttachedTechnoEffect_isset(0),
			MyOriginalTemporal(nullptr),
			Spotlight(nullptr),
			MyBolt(nullptr),
			OriginalHouseType(nullptr),
			HijackerHealth(-1),
			HijackerHouse(nullptr),
			HijackerVeterancy(0.0f),
			AttachEffects_ROFMultiplier(1.0),
			Crate_FirepowerMultiplier(1.0),
			Crate_ArmorMultiplier(1.0),
			Crate_SpeedMultiplier(1.0),
			Crate_Cloakable(false),
			AltOccupation(),
			Survivors_Done(false),
			DriverKilled(false),
			AlwaysOperated(false),
			PayloadCreated(false),
			SuppressLossMessage(false),
			SuperWeapon(nullptr),
			SuperTarget(nullptr),
			TechnoValue(0),
			TechnoValue_NextDisplayFrame(0),
			TakeVehicleMode(false)
		{ }

		~ExtData() {
			this->SetSpotlight(nullptr);
		}

		// when any pointer in the game expires, this is called - be sure to tell everyone we own to invalidate it
		void InvalidatePointer(void *ptr, bool bRemoved) {
			AnnounceInvalidPointer(this->GarrisonedIn, ptr);
			this->InvalidateAttachEffectPointer(ptr);
			AnnounceInvalidPointer(this->MyOriginalTemporal, ptr);
			AnnounceInvalidPointer(this->Spotlight, ptr);
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		bool IsOperated();
		bool IsPowered() const;

		AresAction GetActionHijack(TechnoClass* pTarget);
		bool PerformActionHijack(TechnoClass* pTarget);
		bool PerformHijackOnArea();

		bool IsDriverKillable(double belowPercent) const;
		bool ApplyKillDriver(HouseClass* pNewOwner, TechnoClass* pKiller, bool removeVeterancy);

		unsigned int AlphaFrame(const SHPStruct* Image) const;

		bool DrawVisualFX() const;

		UnitTypeClass* GetUnitType() const;

		bool IsDeactivated() const;

		void InvalidateAttachEffectPointer(void *ptr);

		void RefineTiberium(float amount, int idxType);
		void DepositTiberium(float amount, float bonus, int idxType);

		bool IsCloakable(bool allowPassive) const;
		bool CloakAllowed() const;
		bool CloakDisallowed(bool allowPassive) const;
		bool CanSelfCloakNow() const;

		void SetSpotlight(BuildingLightClass* pSpotlight);

		void CalculateBounty(TechnoClass* pAttacker);
		void DisplayValue(bool force);

		bool AcquireHunterSeekerTarget() const;

		void RecalculateStats();

		int GetSelfHealAmount() const;

		void CreateInitialPayload();

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();

		bool InvalidateExtDataIgnorable(void* const ptr) const {
			auto const abs = static_cast<AbstractClass*>(ptr)->WhatAmI();
			switch(abs) {
			case AbstractType::Building:
			case AbstractType::BuildingLight:
			case AbstractType::Temporal:
			case AbstractType::Anim:
			case AbstractType::Aircraft:
			case AbstractType::Infantry:
			case AbstractType::Unit:
				return false;
			default:
				return true;
			}
		}

		void InvalidatePointer(void *ptr, bool bRemoved);
	};

	static ExtContainer ExtMap;
	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static AresMap<ObjectClass*, AlphaShapeClass*> Alpha;

	static BuildingLightClass * ActiveBuildingLight;

	// 4 bytes wide in the stream, not a bool
	static int NeedsRegap;

	// set when the unit's shadow was drawn ahead of the unit itself, so the
	// engine's own shadow pass can be skipped for it
	static bool DrawnShadowManually;

	static void SpawnSurvivors(FootClass *pThis, TechnoClass *pKiller, bool Select, bool IgnoreDefenses);
	static bool EjectSurvivor(FootClass *Survivor, CoordStruct loc, bool Select);
	static void EjectPassengers(FootClass *, int);
	static CoordStruct GetPutLocation(CoordStruct, int);
	static bool EjectRandomly(FootClass*, CoordStruct const &, int, bool);
	// If available, removes the hijacker from its victim and creates an InfantryClass instance.
	static InfantryClass* RecoverHijacker(FootClass *pThis);

	static void StopDraining(TechnoClass *Drainer, TechnoClass *Drainee);

	static bool CreateWithDroppod(FootClass *Object, const CoordStruct& XYZ);

	static bool UpdateType(TechnoClass* pThis, TechnoTypeClass* pToType);

	static void TransferIvanBomb(TechnoClass *From, TechnoClass *To);
	static void TransferAttachedEffects(TechnoClass *From, TechnoClass *To);
	static void TransferOriginalOwner(TechnoClass* pFrom, TechnoClass* pTo);

	static void FreeSpecificSlave(TechnoClass *Slave, HouseClass *Affector);
	static void DetachSpecificSpawnee(TechnoClass *Spawnee, HouseClass *NewSpawneeOwner);
	static bool CanICloakByDefault(TechnoClass *pTechno);

	static bool IsCloaked(TechnoClass* pTechno);

	static void Destroy(TechnoClass* pTechno, TechnoClass* pKiller = nullptr, HouseClass* pKillerHouse = nullptr, WarheadTypeClass* pWarhead = nullptr);

	static bool SpawnVisceroid(CoordStruct &crd, ObjectTypeClass* pType, int chance, bool ignoreTibDeathToVisc);

	static int DecreaseAmmo(
		TechnoClass* pThis, WeaponTypeClass const* pWeapon = nullptr);

	static MouseCursorType GetCursor(TechnoClass* pThis, int idxWeapon, bool outOfRange);

	static Action GetBombOverObject(TechnoClass* pThis, ObjectClass* pTarget);

	static BuildingClass* IsInWarfactory(TechnoClass* pThis, bool checkNaval);

	static bool IsWarpable(TechnoClass* pThis);

	static int GetWarpPerStep(TemporalClass* pThis, int helpers);
/*
	static int SelectWeaponAgainst(TechnoClass *pThis, TechnoClass *pTarget);
	static bool EvalWeaponAgainst(TechnoClass *pThis, TechnoClass *pTarget, WeaponTypeClass* W);
	static float EvalVersesAgainst(TechnoClass *pThis, TechnoClass *pTarget, WeaponTypeClass* W);
*/
};

// A MassAction participant in its own right, not part of TechnoExt's: it clears
// separately, and it owns the FIRST block of the savegame globals stream. Its
// position in the MassAction list is therefore savegame-visible.
class AlphaExt
{
public:
	static void Clear();
	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);
};
