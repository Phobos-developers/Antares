#pragma once

#include <CCINIClass.h>
#include <BulletTypeClass.h>

#include "../_Container.hpp"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"
#include "../../Ares.h"

#include "../../Misc/Debug.h"

class BulletClass;
class ConvertClass;
class ParticleSystemTypeClass;

class BulletTypeExt
{
public:
	using base_type = BulletTypeClass;

	class ExtData final : public Extension<BulletTypeClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0xF00DF00D;

		// solid
		Valueable<bool> SubjectToBuildings;
		Valueable<int> SolidLevel;

		Valueable<bool> Parachuted;

		// added on 11.11.09 for #667 (part of Trenches)
		Valueable<bool> SubjectToTrenches; //! if false, this projectile/weapon *always* passes through to the occupants, regardless of UC.PassThrough

		// cache for the image animation's palette convert
		OptionalStruct<ConvertClass*> ImageConvert;

		Valueable<bool> Splits;
		Valueable<bool> RetargetSelf;
		Valueable<double> RetargetAccuracy;
		Valueable<double> AirburstSpread;
		Nullable<bool> AroundTarget; // aptly named, for both Splits and Airburst, defaulting to Splits
		Nullable<Leptons> BallisticScatterMin;
		Nullable<Leptons> BallisticScatterMax;

		Valueable<int> AnimLength;

		Valueable<ParticleSystemTypeClass*> AttachedSystem;

		ExtData(BulletTypeClass* OwnerObject) : Extension<BulletTypeClass, ExtData>(OwnerObject),
			SubjectToBuildings(false),
			SolidLevel(0),
			Parachuted(false),
			SubjectToTrenches(true),
			ImageConvert(),
			Splits(false),
			RetargetSelf(true),
			RetargetAccuracy(0.0),
			AirburstSpread(1.5),
			AnimLength(0),
			AttachedSystem(nullptr)
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		ConvertClass* GetConvert();

		bool HasSplitBehavior();

		BulletClass* CreateBullet(AbstractClass* pTarget, TechnoClass* pOwner, WeaponTypeClass* pWeapon) const;
		BulletClass* CreateBullet(AbstractClass* pTarget, TechnoClass* pOwner, int damage, WarheadTypeClass* pWarhead, int speed, int range, bool bright) const;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<BulletTypeExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
