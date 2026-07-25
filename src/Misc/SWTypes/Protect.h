#pragma once

#include "../SWTypes.h"

class SW_Protect : public NewSWType
{
public:
	virtual const char* GetTypeString() const override
	{
		return "Protect";
	}

	virtual void LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI) override;
	virtual void Initialize(SWTypeExt::ExtData *pData) override;
	virtual bool CanFireAt(TargetingData const& data, CellStruct cell, bool manual) const override;
	virtual bool Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer) override;
	virtual bool HandlesType(SuperWeaponType type) const override;

	virtual AnimTypeClass* GetAnim(const SWTypeExt::ExtData* pData) const override;
	virtual SWRange GetRange(const SWTypeExt::ExtData* pData) const override;
};
