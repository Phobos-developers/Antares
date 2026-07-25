#pragma once

#include <AnimTypeClass.h>

#include "../_Container.hpp"
#include "../../Utilities/Enums.h"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"

class AnimClass;
class HouseClass;
class WeaponTypeClass;

class AnimTypeExt
{
public:
	using base_type = AnimTypeClass;

	class ExtData final : public Extension<AnimTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0xEEEEEEEE;

		Valueable<OwnerHouseKind> MakeInfantryOwner;

		CustomPalette Palette;

		Valueable<Leptons> SpawnsParticle_RangeMinimum;
		Valueable<Leptons> SpawnsParticle_RangeMaximum;

		Valueable<WeaponTypeClass*> Weapon;
		Valueable<int> Damage_Delay;

		ExtData(AnimTypeClass* OwnerObject) : Extension<AnimTypeClass, ExtData>(OwnerObject),
			MakeInfantryOwner(OwnerHouseKind::Invoker),
			Palette(CustomPalette::PaletteMode::Temperate),
			SpawnsParticle_RangeMinimum(Leptons(0)),
			SpawnsParticle_RangeMaximum(Leptons(0)),
			Weapon(nullptr),
			Damage_Delay(0)
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<AnimTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static OwnerHouseKind SetMakeInfOwner(AnimClass *pAnim, HouseClass *pInvoker, HouseClass *pVictim, HouseClass *pKiller);
};
