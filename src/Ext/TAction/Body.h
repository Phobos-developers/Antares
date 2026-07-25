#pragma once

#include "../_Container.hpp"
#include "../../Utilities/Template.h"

#include <Helpers/Template.h>

#include <TActionClass.h>

class HouseClass;

class TActionExt
{
public:
	using base_type = TActionClass;

	class ExtData final : public Extension<TActionClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x91919191;

		ExtData(TActionClass* const OwnerObject) : Extension<TActionClass, ExtData>(OwnerObject)
		{ }

		~ExtData() = default;

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		// executing actions
		static bool ActivateFirestorm(TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location);
		static bool DeactivateFirestorm(TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location);
		static bool AuxiliaryPower(TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location);
		static bool KillDriversOf(TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location);
		static bool SetEVAVoice(TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location);
		static bool SetGroup(TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TActionExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static bool Execute(
		TActionClass* pAction, HouseClass* pHouse, ObjectClass* pObject,
		TriggerClass* pTrigger, CellStruct const& location, bool* ret);

	static bool GetMode(TriggerAction actionKind, int* ret);
	static bool GetFlags(TriggerAction actionKind, int* ret);
};
