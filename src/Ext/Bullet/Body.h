#pragma once

#include <BulletClass.h>

#include "../_Container.hpp"
#include "../../Ares.h"

#include "../../Misc/Debug.h"

class ParticleSystemClass;
class SuperWeaponTypeClass;

class BulletExt
{
public:
	using base_type = BulletClass;

	class ExtData final : public Extension<BulletClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x2A2A2A2A;

		SuperWeaponTypeClass *NukeSW;
		ParticleSystemClass *AttachedSystem;

		ExtData(BulletClass* OwnerObject) : Extension(OwnerObject),
			NukeSW(nullptr),
			AttachedSystem(nullptr)
		{ }

		~ExtData() = default;

		bool DamageOccupants();

		void CreateAttachedParticleSys();

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<BulletExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
