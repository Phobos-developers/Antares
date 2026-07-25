#pragma once

#include <TiberiumClass.h>

#include "../../Utilities/Template.h"

#include "../_Container.hpp"

class WarheadTypeClass;

class TiberiumExt
{
public:
	using base_type = TiberiumClass;

	class ExtData final : public Extension<TiberiumClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0xB16B00B5;

		Nullable<int> Damage;
		Nullable<WarheadTypeClass*> Warhead;

		Nullable<int> Heal_Step;
		Nullable<int> Heal_IStep;
		Nullable<int> Heal_UStep;
		Nullable<double> Heal_Delay;

		Nullable<WarheadTypeClass*> ExplosionWarhead;
		Nullable<int> ExplosionDamage;

		Valueable<int> DebrisChance;

		ExtData(TiberiumClass* OwnerObject) : Extension<TiberiumClass, ExtData>(OwnerObject),
			Damage(),
			Warhead(),
			Heal_Step(),
			Heal_IStep(),
			Heal_UStep(),
			Heal_Delay(),
			ExplosionWarhead(),
			ExplosionDamage(),
			DebrisChance(33)
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);
		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		double GetHealDelay() const;
		int GetHealStep(TechnoClass* pTechno) const;
		int GetDamage() const;
		WarheadTypeClass* GetWarhead() const;
		WarheadTypeClass* GetExplosionWarhead() const;
		int GetExplosionDamage() const;
		int GetDebrisChance() const;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TiberiumExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
