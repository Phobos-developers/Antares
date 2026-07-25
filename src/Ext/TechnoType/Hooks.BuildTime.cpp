#include "Body.h"

#include "../../Utilities/TemplateDef.h"

#include <BuildingTypeClass.h>
#include <HouseClass.h>

namespace {
	// HouseClass::GetBuildTimeMult is not bound in YRpp yet
	double GetBuildTimeMult(HouseClass* pHouse, TechnoTypeClass* pType)
	{
		return reinterpret_cast<double(__thiscall*)(HouseClass*, TechnoTypeClass*)>(
			0x50C0A0)(pHouse, pType);
	}

	// HouseClass::GetFactoryCount is not bound in YRpp yet
	int GetFactoryCount(HouseClass* pHouse, AbstractType absType, bool naval)
	{
		return reinterpret_cast<int(__thiscall*)(HouseClass*, AbstractType, bool)>(
			0x500910)(pHouse, absType, naval);
	}

	int GetBuildTime(TechnoTypeExt::ExtData* pExt, HouseClass* pHouse)
	{
		auto const pType = pExt->OwnerObject();
		auto const absType = pType->WhatAmI();

		auto const speed = static_cast<int>(
			GetBuildTimeMult(pHouse, pType) * pType->GetBuildSpeed());
		auto const cost = static_cast<int>(
			pType->BuildTimeMultiplier * static_cast<float>(speed));

		auto const power = pHouse->GetPowerPercentage();
		auto const penalty = pExt->BuildTime_LowPowerPenalty.Get(
			RulesClass::Instance->LowPowerPenaltyModifier);
		auto const minLowPower = pExt->BuildTime_MinLowPower.Get(
			RulesClass::Instance->MinLowPowerProductionSpeed);
		auto const maxLowPower = pExt->BuildTime_MaxLowPower.Get(
			RulesClass::Instance->MaxLowPowerProductionSpeed);

		auto rate = 1.0 - penalty * (1.0 - power);

		if(minLowPower > rate) {
			rate = minLowPower;
		}

		if(power < 1.0 && rate > maxLowPower) {
			rate = maxLowPower;
		}

		if(rate < 0.01) {
			rate = 0.01;
		}

		auto time = static_cast<int>(cost / rate);

		auto const multiple = pExt->BuildTime_MultipleFactory.Get(
			RulesClass::Instance->MultipleFactory);

		if(multiple > 0.0) {
			auto const naval = absType == AbstractType::UnitType && pType->Naval;

			for(auto i = GetFactoryCount(pHouse, absType, naval) - 1; i > 0; --i) {
				time = static_cast<int>(time * multiple);
			}
		}

		if(absType == AbstractType::BuildingType && !pExt->BuildTime_Speed.isset()
			&& static_cast<BuildingTypeClass*>(pType)->Wall)
		{
			return static_cast<int>(
				time * RulesClass::Instance->WallBuildSpeedCoefficient);
		}

		return time;
	}
}

DEFINE_HOOK(0x6F47A0, TechnoClass_GetBuildTime, 0x5)
{
	GET(TechnoClass* const, pThis, ECX);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	R->EAX(GetBuildTime(pExt, pThis->Owner));

	return 0x6F4955;
}

DEFINE_HOOK(0x711EE0, TechnoTypeClass_GetBuildSpeed, 0x6)
{
	GET(TechnoTypeClass* const, pThis, ECX);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis);
	auto const cost = pExt->BuildTime_Cost.Get(pThis->Cost);
	auto const speed = pExt->BuildTime_Speed.Get(RulesClass::Instance->BuildSpeed);

	R->EAX(static_cast<int>(speed * cost / 1000.0 * 900.0));

	return 0x711EDE;
}

DEFINE_HOOK(0x50BEB0, HouseClass_GetCostMult, 0x6)
{
	GET(HouseClass* const, pThis, ECX);
	GET_STACK(TechnoTypeClass* const, pType, 0x4);

	auto bonus = 1.0;

	switch(pType->WhatAmI()) {
	case AbstractType::InfantryType:
		bonus = 1.0 - pThis->CostInfantryMult;
		break;
	case AbstractType::UnitType:
		bonus = 1.0 - pThis->CostUnitsMult;
		break;
	case AbstractType::AircraftType:
		bonus = 1.0 - pThis->CostAircraftMult;
		break;
	case AbstractType::BuildingType:
		bonus = 1.0 - (static_cast<BuildingTypeClass*>(pType)->BuildCat == BuildCat::Combat
			? pThis->CostDefensesMult : pThis->CostBuildingsMult);
		break;
	default:
		break;
	}

	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	double const mult = 1.0 - bonus * pExt->FactoryPlant_Multiplier;

	__asm { fld mult }

	return 0x50BF1E;
}

DEFINE_HOOK(0x6AB8BB, SelectClass_ProcessInput_BuildTime, 0x6)
{
	GET(BuildingTypeClass* const, pType, ESI);

	auto isWall = pType->Wall;

	if(isWall && TechnoTypeExt::ExtMap.Find(pType)->BuildTime_Speed.isset()) {
		isWall = false;
	}

	R->EAX(isWall ? 1u : 0u);

	return 0x6AB8C1;
}
