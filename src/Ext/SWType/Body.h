#pragma once

#include <CCINIClass.h>
#include <SuperWeaponTypeClass.h>
#include <VoxClass.h>

#include "../../Ares.CRT.h"
#include "../../Misc/Actions.h"
//#include "../../Misc/SWTypes.h"
#include "../../Utilities/Enums.h"
#include "../../Utilities/Template.h"
#include "../../Utilities/Constructs.h"

#ifdef DEBUGBUILD
#include "../../Misc/Debug.h"
#endif

#include "../_Container.hpp"

#include <array>

class AircraftTypeClass;
class AnimClass;
class HouseClass;
class MissionClass;
class NewSWType;
class ParadropPlane;
class RadarEventClass;
class SuperClass;
class VocClass;

struct AITargetingModeInfo {
	SuperWeaponAITargetingMode Mode;
	SuperWeaponAITargetingConstraints Constrain;
	SuperWeaponAITargetingPreference Preference;
	SuperWeaponTarget Target;
	SuperWeaponAffectedHouse House;
};

struct SWRange {
	SWRange(float widthOrRange = -1.0f, int height = -1) : WidthOrRange(widthOrRange), Height(height) {}
	SWRange(int widthOrRange, int height = -1) : WidthOrRange(static_cast<float>(widthOrRange)), Height(height) {}

	float range() const {
		return this->WidthOrRange;
	}

	int width() const {
		return static_cast<int>(this->WidthOrRange);
	}

	int height() const {
		return this->Height;
	}

	bool empty() const {
		return this->WidthOrRange < 0.0
			&& this->Height < 0;
	}

	float WidthOrRange;
	int Height;
};

struct LightingColor {
	int Red, Green, Blue, Ambient;
	bool HasValue;
};

class SWTypeExt
{
public:
	using base_type = SuperWeaponTypeClass;

	static const std::array<const AITargetingModeInfo, 21> AITargetingModes;

	// the index of the first custom sw type
	static const int FirstCustomType = 12;

	class ExtData final : public Extension<SuperWeaponTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x66666666;

		// SpyPlane
		ValueableIdx<AircraftTypeClass> SpyPlane_TypeIndex;
		Valueable<int> SpyPlane_Count;
		Valueable<Mission> SpyPlane_Mission;

		// Lightning Storm
		Nullable<int> Weather_Duration;
		Nullable<int> Weather_HitDelay;
		Nullable<int> Weather_ScatterDelay;
		Valueable<int> Weather_ScatterCount;
		Nullable<int> Weather_Separation;
		Valueable<int> Weather_CloudHeight;
		Nullable<int> Weather_RadarOutage;
		Valueable<int> Weather_DebrisMin;
		Valueable<int> Weather_DebrisMax;
		Nullable<bool> Weather_PrintText;
		Valueable<bool> Weather_IgnoreLightningRod;
		Nullable<AnimTypeClass*> Weather_BoltExplosion;
		NullableVector<AnimTypeClass*> Weather_Clouds;
		NullableVector<AnimTypeClass*> Weather_Bolts;
		NullableVector<AnimTypeClass*> Weather_Debris;
		NullableIdxVector<VocClass> Weather_Sounds;
		Valueable<SuperWeaponAffectedHouse> Weather_RadarOutageAffects;

		// Nuke
		Valueable<WeaponTypeClass*> Nuke_Payload;
		Valueable<AnimTypeClass*> Nuke_PsiWarning;
		Nullable<AnimTypeClass*> Nuke_TakeOff;
		Valueable<bool> Nuke_SiloLaunch;

		// Generic Paradrop
		AresMap<AbstractTypeClass*, std::vector<ParadropPlane>> ParaDrop;

		// EMPulse / Fire
		Valueable<bool> EMPulse_Linked;
		Valueable<bool> EMPulse_TargetSelf;
		Valueable<int> EMPulse_PulseDelay;
		Nullable<AnimTypeClass*> EMPulse_PulseBall;
		ValueableVector<BuildingTypeClass*> EMPulse_Cannons;

		// Generic Protection
		Nullable<int> Protect_Duration;
		Nullable<int> Protect_PlayFadeSoundTime;
		Nullable<int> Protect_PowerOutageDuration;
		Valueable<bool> Protect_IsForceShield;

		// Chronosphere
		Nullable<AnimTypeClass *> Chronosphere_BlastSrc;
		Nullable<AnimTypeClass *> Chronosphere_BlastDest;
		Valueable<bool> Chronosphere_KillOrganic;
		Valueable<bool> Chronosphere_KillTeleporters;
		Valueable<bool> Chronosphere_KillCargo;
		Valueable<bool> Chronosphere_AffectUndeployable;
		Valueable<bool> Chronosphere_AffectBuildings;
		Valueable<bool> Chronosphere_AffectUnwarpable;
		Valueable<bool> Chronosphere_AffectIronCurtain;
		Valueable<bool> Chronosphere_BlowUnplaceable;
		Valueable<bool> Chronosphere_ReconsiderBuildings;

		// Genetic Mutator
		Nullable<bool> Mutate_Explosion;
		Valueable<bool> Mutate_IgnoreCyborg;
		Valueable<bool> Mutate_IgnoreNotHuman;
		Valueable<bool> Mutate_KillNatural;

		// Psychic Dominator
		Valueable<bool> Dominator_Capture;
		Nullable<int> Dominator_FireAtPercentage;
		Valueable<int> Dominator_FirstAnimHeight;
		Valueable<int> Dominator_SecondAnimHeight;
		Nullable<AnimTypeClass*> Dominator_FirstAnim;
		Nullable<AnimTypeClass*> Dominator_SecondAnim;
		Nullable<AnimTypeClass*> Dominator_ControlAnim;
		Valueable<bool> Dominator_Ripple;
		Valueable<bool> Dominator_CaptureMindControlled;
		Valueable<bool> Dominator_CapturePermaMindControlled;
		Valueable<bool> Dominator_CaptureImmuneToPsionics;
		Valueable<bool> Dominator_PermanentCapture;

		// Sonar
		Valueable<int> Sonar_Delay;

		// Hunter Seeker
		Valueable<UnitTypeClass*> HunterSeeker_Type;
		Valueable<bool> HunterSeeker_RandomOnly;
		ValueableVector<BuildingTypeClass*> HunterSeeker_Buildings;

		// Drop Pod
		Nullable<int> DropPod_Minimum;
		Nullable<int> DropPod_Maximum;
		Valueable<double> DropPod_Veterancy;
		ValueableVector<TechnoTypeClass*> DropPod_Types;

		// Battery
		Valueable<int> Battery_Power;
		ValueableVector<BuildingTypeClass*> Battery_KeepOnline;
		ValueableVector<BuildingTypeClass*> Battery_Overpower;

		// Money
		Valueable<int> Money_Amount;
		Valueable<int> Money_DrainAmount;
		Valueable<int> Money_DrainDelay;

		// Generic
		ValueableIdx<VoxClass> EVA_Ready;
		ValueableIdx<VoxClass> EVA_Activated;
		ValueableIdx<VoxClass> EVA_Detected;
		ValueableIdx<VoxClass> EVA_Impatient;
		ValueableIdx<VoxClass> EVA_InsufficientFunds;
		ValueableIdx<VoxClass> EVA_SelectTarget;

		// anim/sound
		NullableIdx<VocClass> SW_Sound;
		NullableIdx<VocClass> SW_ActivationSound;
		Nullable<AnimTypeClass *> SW_Anim;
		Valueable<int> SW_AnimHeight;
		Valueable<SuperWeaponAffectedHouse> SW_AnimVisibility;

		Valueable<bool> SW_AutoFire;
		Valueable<bool> SW_ManualFire;
		Valueable<bool> SW_FireToShroud;
		Valueable<bool> SW_RadarEvent;
		Valueable<bool> SW_ShowCameo;
		Valueable<bool> SW_Unstoppable;
		Valueable<bool> SW_AllowPlayer;
		Valueable<bool> SW_AllowAI;
		Valueable<SuperWeaponAffectedHouse> SW_TimerVisibility;
		Valueable<MouseCursorType> SW_Cursor;
		Valueable<MouseCursorType> SW_NoCursor;
		AresFixedString<0x19> SW_PostDependent;
		Valueable<bool> SW_AlwaysGranted;
		Valueable<bool> SW_UseAITargeting;
		Valueable<SuperWeaponAITargetingMode> SW_AITargetingType;
		Nullable<SuperWeaponAITargetingConstraints> SW_AITargetingConstraints;
		Nullable<SuperWeaponAITargetingPreference> SW_AITargetingPreference;
		Nullable<double> SW_ChargeToDrainRatio;

		SWRange SW_Range;
		Valueable<int> SW_MaxCount;
		Valueable<SuperWeaponAffectedHouse> SW_AffectsHouse;
		Valueable<SuperWeaponAffectedHouse> SW_RequiresHouse;
		Nullable<SuperWeaponAffectedHouse> SW_AIRequiresHouse;
		Valueable<SuperWeaponTarget> SW_AffectsTarget;
		Valueable<SuperWeaponTarget> SW_RequiresTarget;
		Nullable<SuperWeaponTarget> SW_AIRequiresTarget;
		Nullable<WarheadTypeClass *> SW_Warhead;
		Nullable<int> SW_Damage;
		Nullable<int> SW_Deferment;
		DWORD SW_RequiredHouses;
		DWORD SW_ForbiddenHouses;
		ValueableVector<BuildingTypeClass*> SW_AuxBuildings;
		ValueableVector<BuildingTypeClass*> SW_NegBuildings;

		Valueable<int> SW_Group;
		Valueable<int> SW_Shots;
		Valueable<bool> SW_InitialReady;
		Valueable<bool> SW_VirtualCharge;

		// Lighting
		Valueable<bool> Lighting_Enabled;
		Nullable<int> Lighting_Ambient;
		Nullable<int> Lighting_Green;
		Nullable<int> Lighting_Blue;
		Nullable<int> Lighting_Red;

		// Messages
		Valueable<CSFText> Message_Detected;
		Valueable<CSFText> Message_Ready;
		Valueable<CSFText> Message_Launch;
		Valueable<CSFText> Message_Activate;
		Valueable<CSFText> Message_Abort;
		Valueable<CSFText> Message_InsufficientFunds;
		Valueable<CSFText> Message_CannotFire;
		Valueable<int> Message_ColorScheme;
		Valueable<bool> Message_FirerColor;

		// Texts
		Valueable<CSFText> Text_Preparing;
		Valueable<CSFText> Text_Hold;
		Valueable<CSFText> Text_Ready;
		Valueable<CSFText> Text_Charging;
		Valueable<CSFText> Text_Active;

		// Range
		Valueable<double> SW_RangeMinimum;
		Valueable<double> SW_RangeMaximum;
		ValueableVector<TechnoTypeClass*> SW_Designators;
		Valueable<bool> SW_AnyDesignator;
		ValueableVector<TechnoTypeClass*> SW_Inhibitors;
		Valueable<bool> SW_AnyInhibitor;

		CustomPalette CameoPal;

		// Unit Delivery
		ValueableVector<TechnoTypeClass *> SW_Deliverables;
		Valueable<bool> SW_DeliverBuildups;
		Valueable<bool> SW_DeliverBaseNormal;
		Valueable<OwnerHouseKind> SW_OwnerHouse;

		AresPCXFile SidebarPCX;

		SuperWeaponType HandledByNewSWType;
		Action LastAction;

		ExtData(SuperWeaponTypeClass* OwnerObject) : Extension<SuperWeaponTypeClass, ExtData>(OwnerObject),
			SpyPlane_TypeIndex(0),
			SpyPlane_Count(1),
			SpyPlane_Mission(Mission::SpyplaneApproach),
			Weather_CloudHeight(-1),
			Weather_ScatterCount(1),
			Nuke_PsiWarning(nullptr),
			Sonar_Delay(0),
			HunterSeeker_Type(nullptr),
			HunterSeeker_RandomOnly(false),
			HunterSeeker_Buildings(),
			DropPod_Minimum(),
			DropPod_Maximum(),
			DropPod_Veterancy(2.0),
			DropPod_Types(),
			EMPulse_Linked(false),
			EMPulse_TargetSelf(false),
			EMPulse_PulseDelay(32),
			EMPulse_PulseBall(),
			SW_MaxCount(-1),
			Money_Amount(0),
			Money_DrainAmount(0),
			Money_DrainDelay(0),
			EVA_Ready(-1),
			EVA_Activated(-1),
			EVA_Detected(-1),
			EVA_Impatient(-1),
			EVA_InsufficientFunds(VoxClass::FindIndex("EVA_InsufficientFunds")),
			EVA_SelectTarget(VoxClass::FindIndex("EVA_SelectTarget")),
			Message_Detected(),
			Message_Ready(),
			Message_Launch(),
			Message_Activate(),
			Message_Abort(),
			Message_InsufficientFunds(),
			Message_CannotFire(CSFText("MSG:CannotFire")),
			Message_ColorScheme(-1),
			Message_FirerColor(false),
			Text_Preparing(),
			Text_Ready(CSFText("TXT_READY")),
			Text_Hold(CSFText("TXT_HOLD")),
			Text_Charging(CSFText("TXT_CHARGING")),
			Text_Active(CSFText("TXT_FIRESTORM_ON")),
			Lighting_Enabled(true),
			SW_AnimHeight(0),
			SW_AnimVisibility(SuperWeaponAffectedHouse::All),
			SW_AutoFire(false),
			SW_ManualFire(true),
			SW_ShowCameo(true),
			SW_Unstoppable(false),
			SW_AllowPlayer(true),
			SW_AllowAI(true),
			SW_TimerVisibility(SuperWeaponAffectedHouse::All),
			SW_Cursor(MouseCursorType::Attack),
			SW_NoCursor(MouseCursorType::Disallowed),
			SW_AlwaysGranted(false),
			SW_UseAITargeting(false),
			SW_AffectsHouse(SuperWeaponAffectedHouse::All),
			SW_RequiresHouse(SuperWeaponAffectedHouse::None),
			SW_AffectsTarget(SuperWeaponTarget::All),
			SW_RequiresTarget(SuperWeaponTarget::None),
			SW_AITargetingType(SuperWeaponAITargetingMode::None),
			SW_FireToShroud(true),
			SW_RadarEvent(true),
			SW_Range(),
			SW_Group(0),
			SW_Shots(-1),
			SW_InitialReady(false),
			SW_VirtualCharge(false),
			SW_RangeMinimum(-1.0),
			SW_RangeMaximum(-1.0),
			SW_Designators(),
			SW_AnyDesignator(false),
			SW_Inhibitors(),
			SW_AnyInhibitor(false),
			SW_RequiredHouses(0xFFFFFFFFu),
			SW_ForbiddenHouses(0u),
			HandledByNewSWType(SuperWeaponType::Invalid),
			LastAction(Action::None),
			CameoPal(),
			SW_DeliverBuildups(true),
			SW_DeliverBaseNormal(true)
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);
		void Initialize(CCINIClass* pINI);

		bool UpdateLightingColor(LightingColor& Lighting) const;

		bool IsAnimVisible(HouseClass* pFirer);
		bool IsTimerVisible(HouseClass* pOwner) const;
		Action GetAction(CellStruct cell);
		bool CanFireAt(HouseClass* pOwner, CellStruct coords, bool manual);
		bool IsHouseAffected(HouseClass* pFirer, HouseClass* pHouse);
		bool IsHouseAffected(HouseClass* pFirer, HouseClass* pHouse, SuperWeaponAffectedHouse value);
		bool IsTechnoAffected(TechnoClass* pTechno);
		void PrintMessage(const CSFText& message, HouseClass* pFirer);

		SuperWeaponTarget GetAIRequiredTarget() const;
		SuperWeaponAffectedHouse GetAIRequiredHouse() const;
		SuperWeaponAITargetingConstraints GetAITargetingConstraints() const;
		SuperWeaponAITargetingPreference GetAITargetingPreference() const;
		bool MeetsAITargetingConstraints(HouseClass* pOwner, bool manual) const;
		Iterator<TechnoClass*> GetPotentialAITargets(HouseClass* pTarget = nullptr) const;

		bool IsAvailable(HouseClass* pHouse) const;
		bool CanShoot(HouseClass* pHouse) const;

		NewSWType* GetNewSWType() const;
		bool IsOriginalType() const;
		bool IsTypeRedirected() const;
		SuperWeaponType GetTypeIndexWithRedirect() const;
		SuperWeaponType GetNewTypeIndex() const;

		WarheadTypeClass* GetWarhead() const;
		AnimTypeClass* GetAnim() const;
		int GetSound() const;
		int GetDamage() const;
		SWRange GetRange() const;
		double GetChargeToDrainRatio() const;

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

	private:
		static SuperWeaponAffectedHouse GetRelation(HouseClass* pFirer, HouseClass* pHouse);
		bool IsCellEligible(CellClass* pCell, SuperWeaponTarget allowed);
		bool IsTechnoEligible(TechnoClass* pTechno, SuperWeaponTarget allowed);

		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<SWTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();

		void InvalidatePointer(void* ptr, bool bRemove);
	};

	static ExtContainer ExtMap;
	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static SuperWeaponTypeClass *CurrentSWType;

	static bool Activate(SuperClass* pSuper, CellStruct cell, bool isPlayer);
	static bool Deactivate(SuperClass* pSuper, CellStruct cell, bool isPlayer);
	static bool TryFire(SuperClass* pSuper, bool manual);

	bool static Launch(SuperClass* pThis, NewSWType* pData, const CellStruct &Coords, bool IsPlayer);
	void static ClearChronoAnim(SuperClass* pThis);
	void static CreateChronoAnim(SuperClass* pThis, const CoordStruct &Coords, AnimTypeClass* pAnimType);
	static bool ChangeLighting(SuperWeaponTypeClass* pCustom = nullptr);
	static LightingColor GetLightingColor(SuperWeaponTypeClass* pCustom = nullptr);
};

class ParadropPlane {
public:
	Valueable<AircraftTypeClass*> Aircraft;
	ValueableVector<TechnoTypeClass*> Types;
	ValueableVector<int> Num;
};

template <>
struct Savegame::AresStreamObject<ParadropPlane> {

	bool ReadFromStream(AresStreamReader &Stm, ParadropPlane &Value, bool RegisterForChange) const {
		return Stm
			.Process(Value.Aircraft, RegisterForChange)
			.Process(Value.Types, RegisterForChange)
			.Process(Value.Num, RegisterForChange)
			.Success();
	};

	bool WriteToStream(AresStreamWriter &Stm, const ParadropPlane &Value) const {
		return Stm
			.Process(Value.Aircraft)
			.Process(Value.Types)
			.Process(Value.Num)
			.Success();
	};
};
