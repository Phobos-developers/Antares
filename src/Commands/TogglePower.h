#pragma once

#include <StringTable.h>

#include "../Ext/Rules/Body.h"

#include <CommandClass.h>
#include <MapClass.h>

class TogglePowerCommandClass : public CommandClass
{
public:
	//CommandClass
	virtual const char* GetName() const override
	{
		return "TogglePower";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return StringTable::LoadString("TXT_TOGGLE_POWER");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_INTERFACE");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return StringTable::LoadString("TXT_TOGGLE_POWER_DESC");
	}

	virtual void Execute(WWKey eInput) const override
	{
		if(RulesExt::Global()->TogglePowerAllowed) {
			MapClass::Instance.SetTogglePowerMode(-1);
		}
	}
};
