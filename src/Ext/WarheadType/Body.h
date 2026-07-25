#pragma once

#include <CCINIClass.h>
#include <WarheadTypeClass.h>
#include <GeneralStructures.h>

#include <Conversions.h>

#include "../../Misc/AttachEffect.h"

#include "../_Container.hpp"

#include "../../Utilities/Constructs.h"
#include "../../Utilities/Enums.h"
#include "../../Utilities/Template.h"

#ifdef DEBUGBUILD
#include "../../Misc/Debug.h"
#endif

#include <vector>

class AnimTypeClass;
class BulletClass;
class HouseClass;
class IonBlastClass;
class TechnoClass;
class VocClass;

class WarheadTypeExt
{
public:
	using base_type = WarheadTypeClass;

	struct VersesData {
		double Verses{ 1.0 };
		bool ForceFire{ true };
		bool Retaliate{ true };
		bool PassiveAcquire{ true };

		bool operator ==(const VersesData &RHS) const {
			return (CLOSE_ENOUGH(this->Verses, RHS.Verses));
		}

		void Parse(const char *str) {
			WarheadFlags flags;
			this->Verses = Conversions::Str2Armor(str, &flags);
			this->ForceFire = flags.ForceFire;
			this->Retaliate = flags.Retaliate;
			this->PassiveAcquire = flags.PassiveAcquire;
		}
	};

	class ExtData final : public Extension<WarheadTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x22222222;

		Valueable<bool> MindControl_Permanent;

		Valueable<int> EMP_Duration;
		Valueable<int> EMP_Cap;
		Valueable<AnimTypeClass*> EMP_Sparkles;

		Valueable<int> IC_Duration;
		Valueable<int> IC_Cap;
		Nullable<bool> IC_Flash;

		std::vector<VersesData> Verses;
		Valueable<double> DeployedDamage;

		Valueable<double> Temporal_HealthFactor;
		Nullable<AnimTypeClass *> Temporal_WarpAway;

		Valueable<bool> AffectsEnemies; // request #397
		Nullable<bool> AffectsOwner;

		Valueable<AnimTypeClass*> InfDeathAnim;

		Nullable<int> NukeFlashDuration;
		ValueableIdx<AnimTypeClass> PreImpactAnim;
		Valueable<bool> PreImpactAnim_Moves;

		Valueable<bool> KillDriver; //!< Whether this warhead turns the target vehicle over to the special side ("kills the driver"). Request #733.

		Valueable<double> KillDriver_KillBelowPercent;
		Valueable<double> KillDriver_Chance;

		Valueable<OwnerHouseKind> KillDriver_Owner;
		Valueable<bool> KillDriver_RemoveVeterancy;

		Valueable<bool> Malicious;

		Valueable<bool> PreventScatter;

		Nullable<bool> BridgeAbsoluteDestroyer;

		Valueable<int> CellSpread_MaxAffect;

		Valueable<int> DamageAirThreshold;

		AttachEffectTypeClass AttachedEffect;

		Valueable<bool> UnitLost_Suppress;
		Valueable<bool> SuppressDeathWeapon_Vehicles;
		Valueable<bool> SuppressDeathWeapon_Infantry;
		ValueableVector<TechnoTypeClass*> SuppressDeathWeapon;

		Valueable<bool> RelativeDamage;
		Valueable<int> RelativeDamage_Buildings;
		Valueable<int> RelativeDamage_Aircraft;
		Valueable<int> RelativeDamage_Infantry;
		Valueable<int> RelativeDamage_Vehicles;
		Valueable<int> RelativeDamage_Terrain;

		Valueable<int> Sonar_Duration;

		Valueable<int> DisableWeapons_Duration;

		Valueable<int> Flash_Duration;

		// ion cannon
		Valueable<bool> IonCannon;
		Valueable<bool> IonCannon_Rock;
		Nullable<int> Ripple_Radius;
		Nullable<AnimTypeClass*> IonCannon_Blast;
		Nullable<AnimTypeClass*> IonCannon_Beam;
		Nullable<WarheadTypeClass*> IonCannon_Warhead;
		Nullable<int> IonCannon_Damage;

		Valueable<bool> EffectsRequireDamage;
		Valueable<bool> EffectsRequireVerses;
		Valueable<bool> AllowZeroDamage;

		NullableIdx<VocClass> DieSound_Override;
		NullableIdx<VocClass> VoiceDie_Override;

		Promotable<int> Culling_BelowHealth;
		Promotable<int> Culling_Chance;

		ExtData(WarheadTypeClass* OwnerObject) : Extension<WarheadTypeClass, ExtData>(OwnerObject),
			MindControl_Permanent(false),
			EMP_Duration(0),
			EMP_Cap(-1),
			EMP_Sparkles(nullptr),
			IC_Duration(0),
			IC_Cap(-1),
			IC_Flash(),
			Verses(11),
			DeployedDamage(1.00),
			Temporal_HealthFactor(0.0),
			Temporal_WarpAway(),
			AffectsEnemies(true),
			AffectsOwner(),
			InfDeathAnim(nullptr),
			NukeFlashDuration(),
			PreImpactAnim(-1),
			PreImpactAnim_Moves(false),
			KillDriver(false),
			KillDriver_KillBelowPercent(1.00),
			KillDriver_Chance(1.00),
			KillDriver_Owner(OwnerHouseKind::Special),
			KillDriver_RemoveVeterancy(false),
			Malicious(true),
			PreventScatter(false),
			BridgeAbsoluteDestroyer(),
			CellSpread_MaxAffect(-1),
			DamageAirThreshold(0),
			AttachedEffect(OwnerObject),
			UnitLost_Suppress(false),
			SuppressDeathWeapon_Vehicles(false),
			SuppressDeathWeapon_Infantry(false),
			RelativeDamage(false),
			RelativeDamage_Buildings(0),
			RelativeDamage_Aircraft(0),
			RelativeDamage_Infantry(0),
			RelativeDamage_Vehicles(0),
			RelativeDamage_Terrain(0),
			Sonar_Duration(0),
			DisableWeapons_Duration(0),
			Flash_Duration(0),
			IonCannon(false),
			IonCannon_Rock(true),
			Ripple_Radius(),
			IonCannon_Blast(),
			IonCannon_Beam(),
			IonCannon_Warhead(),
			IonCannon_Damage(),
			EffectsRequireDamage(false),
			EffectsRequireVerses(true),
			AllowZeroDamage(false),
			DieSound_Override(),
			VoiceDie_Override()
		{
			this->Culling_BelowHealth.Rookie = 0;
			this->Culling_BelowHealth.Veteran = 0;
			this->Culling_BelowHealth.Elite = -1;

			this->Culling_Chance.SetAll(-1);
		}

		~ExtData() = default;

		void Initialize(CCINIClass* pINI);

		void LoadFromINIFile(CCINIClass* pINI);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		void applyIronCurtain(const CoordStruct &coords, HouseClass* pOwner, int damage);
		void applyEMP(const CoordStruct &coords, TechnoClass* pSource);
		bool applyPermaMC(HouseClass* pOwner, AbstractClass* pTarget) const;

		// the invoker is a house, not a techno: shipped applyAE (0x100533B0)
		// compares its argument against the target's TechnoClass::Owner (+0x21C)
		// and hands the same value to AttachAe as the effect's Invoker
		void applyAttachedEffect(const CoordStruct &coords, HouseClass* pInvoker);

		bool applyKillDriver(TechnoClass* pSource, AbstractClass* pTarget) const; // #733

		void applyIonCannon(const CoordStruct &coords);

		int CalculateRelativeDamage(ObjectClass* pVictim) const;

		bool ApplyCulling(TechnoClass* pAttacker, ObjectClass* pVictim) const;

		VersesData& GetVerses(Armor armor) {
			return this->Verses[static_cast<int>(armor)];
		}

		const VersesData& GetVerses(Armor armor) const {
			return this->Verses[static_cast<int>(armor)];
		}

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<WarheadTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();

		void InvalidatePointer(void* ptr, bool bRemoved);
	};

	static ExtContainer ExtMap;

	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static WarheadTypeClass *Temporal_WH;

	static WarheadTypeClass *EMP_WH;

	static WarheadTypeClass *ReceiveDamage_WH;

	static AresMap<IonBlastClass*, const WarheadTypeExt::ExtData*> IonExt;

	static void applyIonCannon(WarheadTypeClass * pWH, const CoordStruct &coords) {
		if(auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH)) {
			pWHExt->applyIonCannon(coords);
		}
	}
	static void applyIronCurtain(WarheadTypeClass * pWH, const CoordStruct &coords, HouseClass * House, int damage) {
		if(auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH)) {
			pWHExt->applyIronCurtain(coords, House, damage);
		}
	}
	static void applyEMP(WarheadTypeClass * pWH, const CoordStruct &coords, TechnoClass *source) {
		if(auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH)) {
			pWHExt->applyEMP(coords, source);
		}
	}
	static bool applyPermaMC(WarheadTypeClass* pWH, HouseClass* House, AbstractClass* Target) {
		if(auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH)) {
			pWHExt->applyPermaMC(House, Target);
		}
	}
	static void applyOccupantDamage(BulletClass *);

	static bool CanAffectTarget(TechnoClass* pTarget, HouseClass* pSourceHouse, WarheadTypeClass* pWarhead);

	static void applyAttachedEffect(WarheadTypeClass * pWH, const CoordStruct &coords, HouseClass * pInvoker) {
		if(auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH)) {
			pWHExt->applyAttachedEffect(coords, pInvoker);
		}
	}
};
