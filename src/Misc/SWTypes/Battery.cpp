#include "Battery.h"
#include "../../Ext/House/Body.h"

#include "../../Utilities/INIParser.h"
#include "../../Utilities/TemplateDef.h"

#include <HouseClass.h>

#include <algorithm>

// for each item of the source, drop the first matching item of the target.
// duplicates are intentional: two active batteries listing the same type
// keep the type listed twice.
static void RemoveEachOnce(
	std::vector<BuildingTypeClass*>& target,
	const std::vector<BuildingTypeClass*>& source)
{
	for(auto const& pType : source) {
		auto const it = std::find(target.begin(), target.end(), pType);
		if(it != target.end()) {
			target.erase(it);
		}
	}
}

void SW_Battery::Initialize(SWTypeExt::ExtData *pData)
{
	pData->SW_AITargetingType = SuperWeaponAITargetingMode::LowPower;
}

void SW_Battery::LoadFromINI(SWTypeExt::ExtData *pData, CCINIClass *pINI)
{
	auto pSW = pData->OwnerObject();

	const char * section = pSW->ID;

	pSW->Action = Action::None;
	pSW->UseChargeDrain = true;

	INI_EX exINI(pINI);

	pData->Battery_Power.Read(exINI, section, "Battery.Power");
	pData->Battery_KeepOnline.Read(exINI, section, "Battery.KeepOnline");
	pData->Battery_Overpower.Read(exINI, section, "Battery.Overpower");
}

bool SW_Battery::Activate(SuperClass* pThis, CellStruct Coords, bool IsPlayer)
{
	if(!pThis->IsPresent) {
		return false;
	}

	auto const pData = SWTypeExt::ExtMap.Find(pThis->Type);
	auto const pOwner = pThis->Owner;
	auto const pExt = HouseExt::ExtMap.Find(pOwner);

	++pExt->BatteriesActive;
	pExt->AuxPower += pData->Battery_Power;

	pExt->Battery_KeepOnline.insert(pExt->Battery_KeepOnline.end(),
		pData->Battery_KeepOnline.begin(), pData->Battery_KeepOnline.end());
	pExt->Battery_Overpower.insert(pExt->Battery_Overpower.end(),
		pData->Battery_Overpower.begin(), pData->Battery_Overpower.end());

	pOwner->RecheckPower = true;

	return true;
}

void SW_Battery::Deactivate(SuperClass* pThis, CellStruct cell, bool isPlayer)
{
	auto const pData = SWTypeExt::ExtMap.Find(pThis->Type);
	auto const pOwner = pThis->Owner;
	auto const pExt = HouseExt::ExtMap.Find(pOwner);

	--pExt->BatteriesActive;
	pExt->AuxPower -= pData->Battery_Power;

	RemoveEachOnce(pExt->Battery_KeepOnline, pData->Battery_KeepOnline);
	RemoveEachOnce(pExt->Battery_Overpower, pData->Battery_Overpower);

	pOwner->RecheckPower = true;
}
