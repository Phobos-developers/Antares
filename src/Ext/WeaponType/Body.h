#pragma once

#include <CCINIClass.h>
#include <GeneralStructures.h>
#include <WeaponTypeClass.h>

#include "../../Misc/Debug.h"

#include "../_Container.hpp"

#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"

class BombClass;
class BulletClass;
class EBolt;
class ParticleSystemTypeClass;
class RadBeam;
class RadType;
class RadSiteClass;
struct SHPStruct;
class TechnoClass;
class VocClass;
class WarheadTypeClass;
class WaveClass;

class WeaponTypeExt
{
public:
	using base_type = WeaponTypeClass;

	class ExtData final : public Extension<WeaponTypeClass, ExtData>
	{
		// wave reverse indexes
		static auto const idxVehicle = 0;
		static auto const idxAircraft = 1;
		static auto const idxBuilding = 2;
		static auto const idxInfantry = 3;
		static auto const idxOther = 4;

	public:
		static constexpr DWORD Canary = 0x33333333;

		// Generic
		Valueable<bool> IsDetachedRailgun;

		// Coloured Rad Beams
		Nullable<ColorStruct> Beam_Color;
		Valueable<int> Beam_Duration;
		Valueable<double> Beam_Amplitude;
		Valueable<bool> Beam_IsHouseColor;

		// Coloured EBolts
		Nullable<ColorStruct> Bolt_Color1;
		Nullable<ColorStruct> Bolt_Color2;
		Nullable<ColorStruct> Bolt_Color3;
		Nullable<ParticleSystemTypeClass *> Bolt_ParticleSystem;

		// TS Lasers
		Valueable<bool> Wave_IsHouseColor;
		Valueable<bool> Wave_IsLaser;
		Valueable<bool> Wave_IsBigLaser;
		Nullable<Vector3D<int>> Wave_Intensity;
		Nullable<Vector3D<int>> Wave_Color;
		Valueable<bool> Wave_Reverse[5];

		Valueable<int> LaserThickness;

		// custom Ivan Bombs
		Valueable<bool> Ivan_DeathBomb;
		Valueable<bool> Ivan_DeathBombOnAllies;
		Valueable<bool> Ivan_KillsBridges;
		Valueable<bool> Ivan_Detachable;
		Nullable<int> Ivan_Damage;
		Nullable<int> Ivan_Delay;
		NullableIdx<VocClass> Ivan_TickingSound;
		NullableIdx<VocClass> Ivan_AttachSound;
		Nullable<WarheadTypeClass *> Ivan_WH;
		Nullable<SHPStruct *> Ivan_Image;
		Nullable<int> Ivan_FlickerRate;
		Nullable<bool> Ivan_CanDetonateTimeBomb;
		Nullable<bool> Ivan_CanDetonateDeathBomb;
		Valueable<bool> Ivan_DetonateOnSell;

		RadType * Rad_Type;

		// #680 Chrono Prison
		Valueable<bool> Abductor; //!< Will this weapon force eligible targets into the passenger hold of the shooter?
		Valueable<bool> Abductor_ChangeOwner;
		Valueable<bool> Abductor_Temporal;
		Valueable<double> Abductor_AbductBelowPercent;
		Valueable<int> Abductor_MaxHealth;
		Valueable<AnimTypeClass *> Abductor_AnimType;

		// brought back from TS
		Valueable<Leptons> ProjectileRange;

		Nullable<bool> ApplyDamage; // whether Damage should be applied even if IsSonic=yes or UseFireParticles=yes

		Valueable<int> Ammo;

		Valueable<MouseCursorType> Cursor_Attack;
		Valueable<MouseCursorType> Cursor_AttackOutOfRange;

		ExtData(WeaponTypeClass* OwnerObject) : Extension<WeaponTypeClass, ExtData>(OwnerObject),
			IsDetachedRailgun(false),
			Beam_Color(),
			Beam_Duration(15),
			Beam_Amplitude(40.0),
			Beam_IsHouseColor(false),
			Bolt_Color1(),
			Bolt_Color2(),
			Bolt_Color3(),
			Bolt_ParticleSystem(),
			Wave_IsHouseColor(false),
			Wave_IsLaser(false),
			Wave_IsBigLaser(false),
			Wave_Intensity(),
			Wave_Color(),
			LaserThickness(-1),
			Ivan_DeathBomb(false),
			Ivan_DeathBombOnAllies(false),
			Ivan_KillsBridges(true),
			Ivan_Detachable(true),
			Ivan_Damage(),
			Ivan_Delay(),
			Ivan_TickingSound(),
			Ivan_AttachSound(),
			Ivan_WH(),
			Ivan_Image(),
			Ivan_FlickerRate(),
			Ivan_CanDetonateTimeBomb(),
			Ivan_CanDetonateDeathBomb(),
			Ivan_DetonateOnSell(true),
			Rad_Type(nullptr),
			Abductor(false),
			Abductor_ChangeOwner(false),
			Abductor_Temporal(false),
			Abductor_AbductBelowPercent(1.0),
			Abductor_MaxHealth(0),
			Abductor_AnimType(nullptr),
			ProjectileRange(Leptons(100000)),
			ApplyDamage(),
			Ammo(1),
			Cursor_Attack(MouseCursorType::Attack),
			Cursor_AttackOutOfRange(MouseCursorType::AttackOutOfRange)
		{
			for(int i = 0; i < 5; ++i) {
				this->Wave_Reverse[i] = false;
			}
		}

		void LoadFromINIFile(CCINIClass* pINI);
		void Initialize(CCINIClass* pINI);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		bool IsWave() const {
			auto const pThis = this->OwnerObject();
			return this->Wave_IsLaser || this->Wave_IsBigLaser || pThis->IsSonic || pThis->IsMagBeam;
		}

		bool IsWaveReversedAgainst(AbstractClass const* pTarget) const;

		ColorStruct GetBeamColor() const;

		bool conductAbduction(BulletClass *);

		bool Abduct(TechnoClass* pAttacker, TechnoClass* pTarget) const;

		void PlantBomb(TechnoClass* pSource, ObjectClass* pTarget) const;

		int GetProjectileRange() const {
			return this->ProjectileRange.Get();
		}

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<WeaponTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

/*
	struct WeaponWeight
	{
		short index;
		bool InRange;
		float DPF;
		bool operator < (const WeaponWeight &RHS) const {
			return (this->InRange < RHS.InRange && this->DPF < RHS.DPF);
		}
	};

	EXT_P_DECLARE(WeaponTypeClass);
	EXT_FUNCS(WeaponTypeClass);
	EXT_INI_FUNCS(WeaponTypeClass);

*/

	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static void DetonateBombOnSell(BombClass* pBomb);

	static AresMap<BombClass*, const ExtData*> BombExt;
	static AresMap<WaveClass*, const ExtData*> WaveExt;
	static AresMap<EBolt*, const ExtData*> BoltExt;
	static AresMap<RadSiteClass*, const ExtData*> RadSiteExt;

	// the colors the wave currently being drawn is tinted with
	struct WaveColorData
	{
		Vector3D<int> Intensity;
		Vector3D<int> Color;
		bool Modified;
	};

	static WaveColorData WaveColors;

	static WaveColorData GetWaveColorData(WaveClass* pWave);
	static WORD ModifyWaveColor(WORD source, int intensity, const WaveColorData& colors);

	static WaveClass* CreateWave(const CoordStruct& crdSrc, const CoordStruct& crdTgt,
		TechnoClass* pOwner, WaveType type, AbstractClass* pTarget, BYTE idxWeapon,
		const ExtData* pData);

	static EBolt* CreateBolt(WeaponTypeClass* pWeapon);
	static EBolt* CreateBolt(WeaponTypeExt::ExtData* pWeapon = nullptr);
};
