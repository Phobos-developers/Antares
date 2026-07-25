#pragma once

#include <ParticleTypeClass.h>

#include "../_Container.hpp"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"

class ParticleTypeExt
{
public:
	using base_type = ParticleTypeClass;

	class ExtData final : public Extension<ParticleTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x9A27719E;

		CustomPalette Palette;

		Valueable<double> DamageRange;

		ExtData(ParticleTypeClass* OwnerObject) : Extension<ParticleTypeClass, ExtData>(OwnerObject),
			Palette(CustomPalette::PaletteMode::Temperate),
			DamageRange(0.0)
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

	class ExtContainer final : public Container<ParticleTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
