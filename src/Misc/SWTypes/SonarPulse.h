#pragma once

#include "../SWTypes.h"

class SW_SonarPulse : public NewSWType
{
public:
	virtual const char* GetTypeString() const override
	{
		return "SonarPulse";
	}

	virtual void Initialize(SWTypeExt::ExtData *pData) override;
	virtual void LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI) override;
	virtual bool Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer) override;
	virtual SuperWeaponFlags Flags() const override;

	virtual SWRange GetRange(const SWTypeExt::ExtData* pData) const override;
};
