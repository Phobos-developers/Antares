#pragma once

#include <CCINIClass.h>
#include <RulesClass.h>

#include "../_Container.hpp"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"

//ifdef DEBUGBUILD
#include "../../Misc/Debug.h"
//endif

class AnimTypeClass;
class TechnoTypeClass;
class VocClass;
class WarheadTypeClass;

class RulesExt
{
public:
	using base_type = RulesClass;

	class ExtData final : public Extension<RulesClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x12341234;

		Valueable<AnimTypeClass* >ElectricDeath;
		Valueable<double> EngineerDamage;
		Valueable<bool> EngineerAlwaysCaptureTech;
		bool MultiEngineer[3];

		Valueable<bool> TogglePowerAllowed;
		Valueable<int> TogglePowerDelay;
		Valueable<int> TogglePowerIQ;

		Valueable<bool> CanMakeStuffUp;

		Valueable<bool> Tiberium_DamageEnabled;
		Valueable<bool> Tiberium_HealEnabled;
		Valueable<WarheadTypeClass*> Tiberium_ExplosiveWarhead;
		Valueable<AnimTypeClass*> Tiberium_ExplosiveAnim;

		Valueable<int> OverlayExplodeThreshold;

		NullableIdx<VocClass> DecloakSound;
		Nullable<int> CloakHeight;

		Valueable<bool> EnemyInsignia;
		Valueable<bool> EnemyWrench;

		Valueable<bool> ReturnStructures;

		Valueable<bool> TypeSelectUseDeploy;

		Valueable<bool> TeamRetaliate;

		Valueable<double> DeactivateDim_Powered;
		Valueable<double> DeactivateDim_EMP;
		Valueable<double> DeactivateDim_Operator;

		Valueable<double> BerserkROFMultiplier;

		// hunter seeker
		ValueableVector<BuildingTypeClass*> HunterSeekerBuildings;
		Valueable<int> HunterSeekerDetonateProximity;
		Valueable<int> HunterSeekerDescendProximity;
		Valueable<int> HunterSeekerAscentSpeed;
		Valueable<int> HunterSeekerDescentSpeed;
		Valueable<int> HunterSeekerEmergeSpeed;

		// drop pods
		Valueable<int> DropPodMinimum;
		Valueable<int> DropPodMaximum;
		ValueableVector<TechnoTypeClass*> DropPodTypes;
		Nullable<AnimTypeClass*> DropPodTrailer;

		Valueable<bool> AutoRepelAI;
		Valueable<bool> AutoRepelPlayer;

		Valueable<CSFText> MessageSilosNeeded;

		Valueable<bool> DegradeEnabled;
		Nullable<double> DegradePercentage;
		Valueable<int> DegradeAmountNormal;
		Valueable<int> DegradeAmountConsumer;

		// firestorm
		Valueable<AnimTypeClass*> FirestormActiveAnim;
		Valueable<AnimTypeClass*> FirestormIdleAnim;
		Valueable<AnimTypeClass*> FirestormGroundAnim;
		Valueable<AnimTypeClass*> FirestormAirAnim;
		Nullable<WarheadTypeClass*> FirestormWarhead;
		Valueable<double> DamageToFirestormDamageCoefficient;

		Valueable<bool> AlliedSolidTransparency;

		Valueable<bool> DamageAirConsiderBridges;

		Valueable<bool> DiskLaserAnimEnabled;

		Valueable<double> DisplayCreditsDelay;
		Valueable<double> StealthSpeakDelay;
		Valueable<double> SubterraneanSpeakDelay;

		// bounty
		ValueableVector<BuildingTypeClass*> BountyEnablers;
		Valueable<bool> BountyDisplay;

		Valueable<bool> UnitsUnsellable;

		Valueable<int> VeteranFlashTimer;

		Valueable<int> RandomCrateMoney;

		// [GlobalControls]
		Valueable<bool> DebugKeysEnabled;
		Valueable<bool> AllowParallelAIQueues;
		bool AllowBypassBuildLimit[3];

		Valueable<bool> IronCurtainFlash;
		Valueable<bool> RepairStopOnInsufficientFunds;
		Valueable<bool> ChronoInfantryCrush;

		Nullable<int> StartInMultiplayerUnitCost;
		Nullable<int> AIFriendlyDistance;
		Nullable<Mission> EMPAIRecoverMission;

		ExtData(RulesClass* OwnerObject) : Extension<RulesClass, ExtData>(OwnerObject),
			ElectricDeath(nullptr),
			EngineerDamage(0.0),
			EngineerAlwaysCaptureTech(true),
			TogglePowerAllowed(false),
			TogglePowerDelay(45),
			TogglePowerIQ(-1),
			CanMakeStuffUp(false),
			Tiberium_DamageEnabled(false),
			Tiberium_HealEnabled(false),
			Tiberium_ExplosiveWarhead(nullptr),
			Tiberium_ExplosiveAnim(nullptr),
			OverlayExplodeThreshold(0),
			DecloakSound(),
			CloakHeight(),
			EnemyInsignia(true),
			EnemyWrench(true),
			ReturnStructures(false),
			TypeSelectUseDeploy(true),
			TeamRetaliate(false),
			DeactivateDim_Powered(0.5),
			DeactivateDim_EMP(0.8),
			DeactivateDim_Operator(0.65),
			BerserkROFMultiplier(0.5),
			HunterSeekerBuildings(),
			HunterSeekerDetonateProximity(0),
			HunterSeekerDescendProximity(0),
			HunterSeekerAscentSpeed(0),
			HunterSeekerDescentSpeed(0),
			HunterSeekerEmergeSpeed(0),
			DropPodMinimum(0),
			DropPodMaximum(0),
			DropPodTypes(),
			DropPodTrailer(),
			DegradeAmountNormal(0),
			DegradeAmountConsumer(1),
			DisplayCreditsDelay(0.02),
			StealthSpeakDelay(0.0),
			SubterraneanSpeakDelay(0.0),
			BountyEnablers(),
			BountyDisplay(false),
			UnitsUnsellable(false),
			VeteranFlashTimer(0),
			RandomCrateMoney(900),
			DebugKeysEnabled(true),
			AllowParallelAIQueues(true),
			IronCurtainFlash(true),
			RepairStopOnInsufficientFunds(true),
			ChronoInfantryCrush(true),
			StartInMultiplayerUnitCost(),
			AIFriendlyDistance(),
			EMPAIRecoverMission()
		{
			MultiEngineer[0] = false; // Skirmish
			MultiEngineer[1] = false; // LAN
			MultiEngineer[2] = false; // WOnline

			// indexed by HouseClass::AIDifficulty, thus the reverse of the tag's order
			AllowBypassBuildLimit[0] = false; // Hard
			AllowBypassBuildLimit[1] = false; // Normal
			AllowBypassBuildLimit[2] = false; // Easy
		}

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);
		void LoadBeforeTypeData(RulesClass* pThis, CCINIClass* pINI);
		void LoadAfterTypeData(RulesClass* pThis, CCINIClass* pINI);
		void Initialize(CCINIClass* pINI);

		void InitializeAfterTypeData(RulesClass* pThis);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

private:
	static std::unique_ptr<ExtData> Data;

public:
	static void Allocate(RulesClass *pThis);
	static void Remove(RulesClass *pThis);

	static void LoadFromINIFile(RulesClass *pThis, CCINIClass *pINI);
	static void LoadBeforeTypeData(RulesClass *pThis, CCINIClass *pINI);
	static void LoadAfterTypeData(RulesClass *pThis, CCINIClass *pINI);

	static ExtData* Global()
	{
		return Data.get();
	}

	static DynamicVectorClass<BuildType> TabCameos[4];

	static void ClearCameos();

	static void Clear() {
		ClearCameos();
		Allocate(RulesClass::Instance);
	}

	static void PointerGotInvalid(void* ptr, bool removed) {
		Global()->InvalidatePointer(ptr, removed);
	}

	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static bool DetailsCurrentlyEnabled();
	static bool DetailsCurrentlyEnabled(int minDetailLevel);
};
