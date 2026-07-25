#pragma once

#include "../SWTypes.h"

class SW_EMPulse : public NewSWType
{
public:
	virtual const char* GetTypeString() const override
	{
		return "EMPulse";
	}

	virtual void LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI) override;
	virtual void Initialize(SWTypeExt::ExtData *pData) override;
	virtual bool Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer) override;

private:
	virtual bool IsLaunchSite(SWTypeExt::ExtData* pSWType, BuildingClass* pBuilding) const override;
	virtual std::pair<double, double> GetLaunchSiteRange(SWTypeExt::ExtData* pSWType, BuildingClass* pBuilding) const override;
};
