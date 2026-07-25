#pragma once

#include "../SWTypes.h"

class SW_SpyPlane : public NewSWType
{
public:
	virtual void LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI) override;
	virtual void Initialize(SWTypeExt::ExtData *pData) override;
	virtual bool Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer) override;
	virtual bool HandlesType(SuperWeaponType type) const override;
};
