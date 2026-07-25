#pragma once

#include <CCINIClass.h>
#include <HouseTypeClass.h>

#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"
#include "../../Ares.CRT.h"
#include "../_Container.hpp"

class AircraftTypeClass;
class AnimTypeClass;
class ColorScheme;
class BuildingTypeClass;
class TechnoTypeClass;

class HouseTypeExt
{
public:
	using base_type = HouseTypeClass;

	class ExtData final : public Extension<HouseTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0xAFFEAFFE;

		static const int ObserverBackgroundWidth = 121;
		static const int ObserverBackgroundHeight = 96;

		static const int ObserverFlagPCXX = 70;
		static const int ObserverFlagPCXY = 70;
		static const int ObserverFlagPCXWidth = 45;
		static const int ObserverFlagPCXHeight = 21;

		AresPCXFile FlagFile; //Flag
		AresFixedString<0x20> LoadScreenBackground; //LoadScreen
		AresFixedString<0x20> LoadScreenPalette; //LoadScreen palette
		AresFixedString<0x20> TauntFile; //Taunt filename format (should contain %d !!!)
		Valueable<CSFText> LoadScreenName; // country name
		Valueable<CSFText> LoadScreenSpecialName; // country's special weapon/unit
		Valueable<CSFText> LoadScreenBrief; // country's load short text
		Valueable<CSFText> StatusText; // for this country's Skirmish STT
		ValueableIdx<ColorScheme> LoadTextColor; //The text color used for non-Campaign modes
		Valueable<int> RandomSelectionWeight; //This country gets added this many times into the list of legible countries for random selection.
		Nullable<int> AIRandomSelectionWeight; //The weight used instead when the AI picks a random country.
		Valueable<int> CountryListIndex; //The index this country will appear in the selection list.

		ValueableVector<BuildingTypeClass *> Powerplants;
		ValueableVector<TechnoTypeClass*> ParaDropTypes;
		ValueableVector<int> ParaDropNum;
		ValueableIdx<AircraftTypeClass> ParaDropPlane;
		Valueable<AnimTypeClass*> Parachute_Anim;

		ValueableVector<BuildingTypeClass*> VeteranBuildings;

		AresPCXFile ObserverBackground;
		SHPStruct *ObserverBackgroundSHP;

		AresPCXFile ObserverFlag;
		SHPStruct *ObserverFlagSHP;
		Valueable<bool> ObserverFlagYuriPAL;
		bool SettingsInherited;

		Nullable<bool> Degrades;
		Nullable<bool> CanBeDriven;

		NullableVector<TechnoTypeClass*> StartInMultiplayer_Types;
		Valueable<bool> StartInMultiplayer_WithConst;

		Valueable<bool> GivesBounty;

		ExtData(HouseTypeClass* OwnerObject) : Extension<HouseTypeClass, ExtData>(OwnerObject),
			LoadTextColor(-1),
			RandomSelectionWeight(1),
			CountryListIndex(100),
			ParaDropPlane(-1),
			Parachute_Anim(nullptr),
			VeteranBuildings(),
			ObserverBackgroundSHP(nullptr),
			ObserverFlagSHP(nullptr),
			ObserverFlagYuriPAL(false),
			SettingsInherited(false),
			StartInMultiplayer_WithConst(false),
			GivesBounty(true)
		{
			this->InitializeConstants();
		}

		~ExtData() = default;

		void InitializeConstants();
		void LoadFromINIFile(CCINIClass* pINI);
		void LoadFromRulesFile(CCINIClass *pINI);
		void Initialize(CCINIClass* pINI);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		AircraftTypeClass* GetParadropPlane();
		bool GetParadropContent(Iterator<TechnoTypeClass*>&, Iterator<int>&);
		AnimTypeClass* GetParachuteAnim();

		Iterator<BuildingTypeClass*> GetPowerplants() const;
		Iterator<BuildingTypeClass*> GetDefaultPowerplants() const;

		void InheritSettings(HouseTypeClass *pThis);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<HouseTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static int PickRandomCountry(bool isHuman);
};
