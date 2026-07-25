#pragma once

#include <StringTable.h>

#include "Ares.h"
#include "Commands/Commands.h"

#include "../Ext/House/Body.h"
#include "../Ext/HouseType/Body.h"
#include "../Misc/Debug.h"

#include <BuildingTypeClass.h>
#include <HouseClass.h>
#include <MessageListClass.h>

class AIBasePlanCommandClass : public AresCommandClass
{
public:
	//CommandClass
	virtual const char* GetName() const override
	{
		return "DumpAIBasePlan";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return StringTable::LoadString("TXT_DUMP_AI_BASE_PLAN");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_DEVELOPMENT");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return StringTable::LoadString("TXT_DUMP_AI_BASE_PLAN_DESC");
	}

	virtual void Execute(WWKey eInput) const override
	{
		if(this->CheckDebugDeactivated()) {
			return;
		}

		Debug::Log("AI Base Plans:\n");
		for(int i = 0; i < HouseClass::Array.Count; ++i) {
			auto H = HouseClass::Array.GetItem(i);
			if(!H->IsControlledByHuman()) {
				Debug::Log("#%02d: country %25s:\n", i, H->Type->ID);
				const auto& b = H->Base.BaseNodes;
				for(int j = 0; j < b.Count; ++j) {
					const auto& n = b[j];
					auto idx = n.BuildingTypeIndex;
					if(idx >= 0) {
						auto lbl = BuildingTypeClass::Array.GetItem(idx)->ID;
						Debug::Log("\tNode #%03d: %s @ (%05d, %05d), Attempts so far: %d, Placed: %d\n"
							, j, lbl, n.MapCoords.X, n.MapCoords.Y, n.Attempts, n.Placed);
					} else {
						Debug::Log("\tNode #%03d: Special %d @ (%05d, %05d), Attempts so far: %d, Placed: %d\n"
							, j, idx, n.MapCoords.X, n.MapCoords.Y, n.Attempts, n.Placed);
					}
				}
				Debug::Log("\n");
			}
		}

		MessageListClass::Instance.PrintMessage(L"Dumped AI Base Plan");
	}
};
