#pragma once

#include <StringTable.h>

#include "Ares.h"
#include "../Misc/Debug.h"

#include <CommandClass.h>

class FPSCounterCommandClass : public CommandClass
{
public:
	//CommandClass
	virtual const char* GetName() const override
	{
		return "FPSCounter";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return StringTable::LoadString("TXT_FPS_COUNTER");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_DEVELOPMENT");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return StringTable::LoadString("TXT_FPS_COUNTER_DESC");
	}

	virtual void Execute(WWKey eInput) const override
	{
		Ares::bFPSCounter = !Ares::bFPSCounter;
	}
};
