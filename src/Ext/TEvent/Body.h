#pragma once

#include "../_Container.hpp"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"

#include <Helpers/Template.h>

#include <TEventClass.h>

class HouseClass;
class ObjectClass;
class SuperClass;
class TechnoTypeClass;

class TEventExt
{
public:
	using base_type = TEventClass;

	// what SuperNearWaypoint is sprung with
	struct SuperTarget {
		SuperClass* Super;
		CellStruct Cell;
	};

	class ExtData final : public Extension<TEventClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x61616161;

		OptionalStruct<TechnoTypeClass*> TechnoType;

		ExtData(TEventClass* OwnerObject) : Extension<TEventClass, ExtData>(OwnerObject),
			TechnoType()
		{ }

		~ExtData() = default;

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		// support
		TechnoTypeClass* GetTechnoType();

		// handling events
		bool TechTypeExists(int count, HouseClass* pOwner);
	};

	class ExtContainer final : public Container<TEventExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static bool HasOccured(
		TEventClass* pEvent, TriggerEvent eventType, HouseClass* pOwner,
		ObjectClass* pAttachedTo, void* pSource, bool* ret);

	static bool GetAttachFlags(TriggerEvent eventKind, int* ret);
	static bool GetPersistable(TriggerEvent eventKind, bool* ret);
	static bool GetSaveMode(TriggerEvent eventKind, int* ret);

	static HouseClass* ResolveHouseParam(int param, HouseClass* pOwnerHouse = nullptr);
};
