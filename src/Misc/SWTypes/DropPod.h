#pragma once

#include "../SWTypes.h"

class SW_DropPod : public NewSWType
{
public:
	virtual const char* GetTypeString() const override
	{
		return "DropPod";
	}

	virtual void Initialize(SWTypeExt::ExtData *pData) override;
	virtual void LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI) override;
	virtual bool Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer) override;
};
