#pragma once

#include <ParticleSystemClass.h>

#include "../_Container.hpp"

#include <vector>

class ParticleTypeClass;

class ParticleSystemExt
{
public:
	using base_type = ParticleSystemClass;

	// which of the Ares particle handlers takes over a system, if any
	enum class BehaveKind : int {
		None = 0,
		Spark = 1,
		Railgun = 2,
		Smoke = 3
	};

	struct MovementDataItem {
		Vector3D<float> Location;
		Vector3D<float> Velocity;
		float Speed;
		float ColorFactor;
		int ColorIndex;
		int Duration;
		BYTE Expired;
		ColorStruct Color;
	};

	struct DrawDataItem {
		CoordStruct Location;
		int VelocityX;
		int VelocityY;
		float VelocityZ;
		int Order;
		int ImageFrame;
		int Duration;
		ParticleTypeClass* LinkedParticleType;
		BYTE Translucency;
		BYTE Expired;
		BYTE Unknown2A;
		BYTE Unknown2B;

		bool Load(AresStreamReader &Stm, bool RegisterForChange);
		bool Save(AresStreamWriter &Stm) const;
	};

	class ExtData final : public Extension<ParticleSystemClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x9A271C7E;

		BehaveKind Behave;
		ParticleTypeClass* HeldParticleType;

		std::vector<MovementDataItem> MovementData;
		std::vector<DrawDataItem> DrawData;

		ExtData(ParticleSystemClass* OwnerObject) : Extension<ParticleSystemClass, ExtData>(OwnerObject),
			Behave(BehaveKind::None),
			HeldParticleType(nullptr),
			MovementData(),
			DrawData()
		{
			this->InitializeConstants();
		}

		~ExtData() = default;

		void InitializeConstants();

		// runs the Ares replacement for this system's behaviour, if it has one.
		// returns whether the vanilla update has to be skipped entirely.
		bool Handled();

		void HandleSpark();
		void HandleRailgun();
		void HandleSmoke();

		void DrawInAir(bool ignoreShroud);

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<ParticleSystemExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static void UpdateInAir();
};
