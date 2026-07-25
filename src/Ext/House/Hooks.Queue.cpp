#include "Body.h"
#include "../Rules/Body.h"
#include "../TechnoType/Body.h"
#include "../../Ares.h"

#include <FactoryClass.h>

// =============================
// multiqueue hooks

DEFINE_HOOK(0x4502F4, BuildingClass_Update_Factory, 0x6)
{
	GET(BuildingClass *, B, ESI);
	HouseClass * H = B->Owner;

	if(H->Production && !RulesExt::Global()->AllowParallelAIQueues) {
		HouseExt::ExtData *pData = HouseExt::ExtMap.Find(H);
		BuildingClass **curFactory = nullptr;
		switch(B->Type->Factory) {
			case AbstractType::BuildingType:
				curFactory = &pData->Factory_BuildingType;
				break;
			case AbstractType::UnitType:
				curFactory = B->Type->Naval
				 ? &pData->Factory_NavyType
				 : &pData->Factory_VehicleType;
				break;
			case AbstractType::InfantryType:
				curFactory = &pData->Factory_InfantryType;
				break;
			case AbstractType::AircraftType:
				curFactory = &pData->Factory_AircraftType;
				break;
		}
		if(!curFactory) {
			Game::RaiseError(E_POINTER);
		} else if(!*curFactory) {
			*curFactory = B;
			return 0;
		} else if(*curFactory != B) {
			return 0x4503CA;
		}
	}

	return 0;
}

DEFINE_HOOK(0x4CA07A, FactoryClass_AbandonProduction, 0x8)
{
	GET(FactoryClass *, F, ESI);
	HouseClass * H = F->Owner;

	HouseExt::ExtData *pData = HouseExt::ExtMap.Find(H);
	TechnoClass * T = F->Object;
	switch(T->WhatAmI()) {
		case AbstractType::Building:
			pData->Factory_BuildingType = nullptr;
			break;
		case AbstractType::Unit:
			(T->GetTechnoType()->Naval
			 ? pData->Factory_NavyType
			 : pData->Factory_VehicleType) = nullptr;
			break;
		case AbstractType::Infantry:
			pData->Factory_InfantryType = nullptr;
			break;
		case AbstractType::Aircraft:
			pData->Factory_AircraftType = nullptr;
			break;
	}

	return 0;
}

DEFINE_HOOK(0x444119, BuildingClass_KickOutUnit_UnitType, 0x6)
{
	GET(UnitClass *, U, EDI);

	GET(BuildingClass *, Factory, ESI);

	HouseExt::ExtData *pData = HouseExt::ExtMap.Find(Factory->Owner);

	(U->Type->Naval
	 ? pData->Factory_NavyType
	 : pData->Factory_VehicleType) = nullptr;
	return 0;
}


DEFINE_HOOK(0x444131, BuildingClass_KickOutUnit_InfantryType, 0x6)
{
	GET(HouseClass  *, H, EAX);

	HouseExt::ExtMap.Find(H)->Factory_InfantryType = nullptr;
	return 0;
}

DEFINE_HOOK(0x44531F, BuildingClass_KickOutUnit_BuildingType, 0xA)
{
	GET(HouseClass  *, H, EAX);

	HouseExt::ExtMap.Find(H)->Factory_BuildingType = nullptr;
	return 0;
}

DEFINE_HOOK(0x443CCA, BuildingClass_KickOutUnit_AircraftType, 0xA)
{
	GET(HouseClass  *, H, EDX);

	HouseExt::ExtMap.Find(H)->Factory_AircraftType = nullptr;
	return 0;
}

// complete replacement
DEFINE_HOOK(0x50B370, HouseClass_ShouldDisableCameo, 0x5)
{
	GET(HouseClass const* const, pThis, ECX);
	GET_STACK(TechnoTypeClass const* const, pType, 0x4);

	auto ret = false;

	if(pType) {
		auto const abs = pType->WhatAmI();
		auto const pFactory = pThis->GetPrimaryFactory(
			abs, pType->Naval, BuildCat::DontCare);

		// special logic for AirportBound
		if(abs == AbstractType::AircraftType) {
			auto const pAType = static_cast<AircraftTypeClass const*>(pType);
			if(pAType->AirportBound) {
				auto ownedAircraft = 0;
				auto queuedAircraft = 0;

				for(auto const& pAircraft : RulesClass::Instance->PadAircraft) {
					ownedAircraft += pThis->CountOwnedAndPresent(pAircraft);
					if(pFactory) {
						queuedAircraft += pFactory->CountTotal(pAircraft);
					}
				}

				// #896082: also check BuildLimit, and not always return the
				// result of this comparison directly. originally, it would
				// return false here, too, allowing more units than the
				// BuildLimit permitted.
				if(ownedAircraft + queuedAircraft >= pThis->AirportDocks) {
					R->EAX(true);
					return 0x50B669;
				}
			}
		}

		auto queued = 0;
		if(pFactory) {
			queued = pFactory->CountTotal(pType);

			// #1286800: build limit > 1 and queues
			// the object in production is counted twice: it appears in this
			// factory queue, and it is already counted in the house's counters.
			// this only affects positive build limits, for negative ones
			// players could queue up one more than BuildLimit.
			if(auto const pObject = pFactory->Object) {
				if(pObject->GetType() == pType && pType->BuildLimit > 0) {
					--queued;
				}
			}
		}

		// #1521738: to stay consistent, use the new method to calculate this
		if(HouseExt::BuildLimitRemaining(pThis, pType) - queued <= 0) {
			ret = true;
		} else {
			auto const state = HouseExt::HasFactory(
				pThis, pType, true, true, false, true).State;
			ret = (state < HouseExt::FactoryState::Available);
		}
	}

	R->EAX(ret);
	return 0x50B669;
}

// complete replacement: the factory that would produce this type for this
// house, if there is a usable one at all.
DEFINE_HOOK(0x5F7900, ObjectTypeClass_FindFactory, 0x5)
{
	GET(TechnoTypeClass const* const, pThis, ECX);
	GET_STACK(bool const, allowOccupied, 0x4);
	GET_STACK(bool const, requirePower, 0x8);
	GET_STACK(bool const, requireCanBuild, 0xC);
	GET_STACK(HouseClass const* const, pHouse, 0x10);

	auto const found = HouseExt::HasFactory(
		pHouse, pThis, allowOccupied, requirePower, requireCanBuild, false);

	R->EAX(found.State >= HouseExt::FactoryState::Available
		? found.Factory : nullptr);

	return 0x5F7A89;
}
