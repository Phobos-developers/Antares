#pragma once

#include <InfantryClass.h>

#include "../_Container.hpp"
#include "../../Ares.h"

#include "../../Misc/Debug.h"

class BuildingClass;

class InfantryExt
{
public:
	using base_type = InfantryClass;

	class ExtData final : public Extension<InfantryClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0xE1E2E3E4;


		ExtData(InfantryClass* OwnerObject) : Extension<InfantryClass, ExtData>(OwnerObject)
		{ }

		~ExtData() = default;

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		bool IsOccupant(); //!< Determines whether this InfantryClass is currently an occupant inside a BuildingClass.

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<InfantryExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static Action GetEngineerEnterEnemyBuildingAction(BuildingClass *pBld);
};
