#pragma once

#include "../SWTypes.h"

class SW_Battery : public NewSWType
{
public:
	virtual const char* GetTypeString() const override
	{
		return "Battery";
	}

	virtual void Initialize(SWTypeExt::ExtData *pData) override;
	virtual void LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI) override;
	virtual bool Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer) override;
	virtual void Deactivate(SuperClass* pThis, CellStruct cell, bool isPlayer) override;
};
