#pragma once

#include <CCINIClass.h>
#include <SideClass.h>

#include "../../Ares.h"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"
#include "../../Misc/EVAVoices.h"

class AircraftTypeClass;
class BuildingTypeClass;
class ColorScheme;
class InfantryTypeClass;
class InfantryClass;
class UnitTypeClass;
class VoxClass;

#include "../_Container.hpp"

class SideExt
{
public:
	using base_type = SideClass;

	class ExtData final : public Extension<SideClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x06D106D1;

		int ArrayIndex;

		Nullable<InfantryTypeClass*> Disguise;
		Nullable<InfantryTypeClass*> Crew;
		Nullable<InfantryTypeClass*> Engineer;
		Nullable<InfantryTypeClass*> Technician;
		Nullable<int> SurvivorDivisor;
		NullableVector<BuildingTypeClass*> BaseDefenses;
		NullableVector<int> BaseDefenseCounts;
		NullableVector<TechnoTypeClass*> ParaDropTypes;
		NullableVector<int> ParaDropNum;
		ValueableIdx<AircraftTypeClass> ParaDropPlane;
		Nullable<AnimTypeClass*> Parachute_Anim;
		Valueable<ColorStruct> ToolTipTextColor;
		ValueableIdx<ColorScheme> MessageTextColorIndex;
		Valueable<int> SidebarMixFileIndex;
		Valueable<bool> SidebarYuriFileNames;
		ValueableIdx<EVAVoices> EVAIndex;
		Valueable<UnitTypeClass*> HunterSeeker;

		AresFixedString<0x20> ScoreCampaignBackground;
		AresFixedString<0x20> ScoreCampaignTransition;
		AresFixedString<0x20> ScoreCampaignAnimation;
		AresFixedString<0x20> ScoreCampaignPalette;
		AresFixedString<0x20> ScoreMultiplayBackground;
		AresFixedString<0x20> ScoreMultiplayBars;
		AresFixedString<0x20> ScoreMultiplayPalette;

		AresFixedString<0x20> ScoreCampaignThemeUnderPar;
		AresFixedString<0x20> ScoreCampaignThemeOverPar;
		AresFixedString<0x20> ScoreMultiplayThemeWin;
		AresFixedString<0x20> ScoreMultiplayThemeLose;

		AresFixedString<0x20> GraphicalTextImage;
		AresFixedString<0x20> GraphicalTextPalette;

		AresFixedString<0x20> DialogBackgroundImage;
		AresFixedString<0x20> DialogBackgroundPalette;

		ExtData(SideClass* OwnerObject) : Extension<SideClass, ExtData>(OwnerObject),
			ArrayIndex(-1),
			ParaDropPlane(-1),
			ToolTipTextColor(),
			MessageTextColorIndex(-1),
			EVAIndex(-1),
			HunterSeeker(nullptr),
			ScoreCampaignThemeUnderPar("SCORE"),
			ScoreCampaignThemeOverPar("SCORE"),
			ScoreMultiplayThemeWin("SCORE"),
			ScoreMultiplayThemeLose("SCORE")
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);
		void Initialize(CCINIClass* pINI);
		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		int GetSurvivorDivisor() const;
		int GetDefaultSurvivorDivisor() const;

		InfantryTypeClass* GetCrew() const;
		InfantryTypeClass* GetDefaultCrew() const;

		InfantryTypeClass* GetEngineer() const;
		InfantryTypeClass* GetTechnician() const;

		InfantryTypeClass* GetDisguise() const;
		InfantryTypeClass* GetDefaultDisguise() const;

		Iterator<int> GetBaseDefenseCounts() const;
		Iterator<int> GetDefaultBaseDefenseCounts() const;

		Iterator<BuildingTypeClass*> GetBaseDefenses() const;
		Iterator<BuildingTypeClass*> GetDefaultBaseDefenses() const;

		Iterator<TechnoTypeClass*> GetParaDropTypes() const;
		Iterator<InfantryTypeClass*> GetDefaultParaDropTypes() const;

		Iterator<int> GetParaDropNum() const;
		Iterator<int> GetDefaultParaDropNum() const;

		AnimTypeClass* GetParachuteAnim() const;

		const char* GetMultiplayerScoreBarFilename(unsigned int index) const;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<SideExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	//Hacks required in other classes:
	//- TechnoTypeClass (Stolen Tech)
	//- HouseClass (Stolen Tech)
	//- VoxClass (EVA)

	static ExtContainer ExtMap;
	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static int CurrentLoadTextColor;

	static UniqueGamePtr<SHPStruct> GraphicalTextImage;
	static UniqueGamePtr<BytePalette> GraphicalTextPalette;
	static UniqueGamePtr<ConvertClass> GraphicalTextConvert;

	static UniqueGamePtr<SHPStruct> DialogBackgroundImage;
	static UniqueGamePtr<BytePalette> DialogBackgroundPalette;
	static UniqueGamePtr<ConvertClass> DialogBackgroundConvert;

	static void UpdateGlobalFiles();

	static DWORD LoadTextColor(REGISTERS* R, DWORD dwReturnAddress);
	static DWORD MixFileYuriFiles(REGISTERS* R, DWORD dwReturnAddress1, DWORD dwReturnAddress2);

	static SHPStruct* GetGraphicalTextImage();
	static ConvertClass* GetGraphicalTextConvert();
};
