#include "Api.h"
#include "Features.h"

#include "../Ext/Building/Body.h"
#include "../Ext/House/Body.h"
#include "../Ext/HouseType/Body.h"
#include "../Ext/Rules/Body.h"
#include "../Ext/SWType/Body.h"
#include "../Ext/Techno/Body.h"
#include "../Ext/TechnoType/Body.h"
#include "../Ext/WarheadType/Body.h"
#include "../Ext/WeaponType/Body.h"
#include "../Misc/EVAVoices.h"

#include <HouseClass.h>
#include <TechnoClass.h>

// Thunks. Each one is a thin __stdcall wrapper so the table stays a stable ABI
// no matter how the thing behind it is spelled internally -- which matters,
// because several of these have names a consumer would never guess.

namespace
{
	bool __stdcall ConvertTypeTo(TechnoClass* const pThis, TechnoTypeClass* const pToType)
	{
		return pThis && pToType && TechnoExt::UpdateType(pThis, pToType);
	}

	void __stdcall SpawnSurvivors(FootClass* const pThis, TechnoClass* const pKiller,
		bool const select, bool const ignoreDefenses)
	{
		if(pThis) {
			TechnoExt::SpawnSurvivors(pThis, pKiller, select, ignoreDefenses);
		}
	}

	bool __stdcall ReverseEngineer(BuildingClass* const pThis, TechnoClass* const pVictim)
	{
		return pThis && pVictim
			&& BuildingExt::ExtMap.Find(pThis)->ReverseEngineer(pVictim);
	}

	bool __stdcall MeetsAITargetingConstraints(SuperWeaponTypeClass* const pType,
		HouseClass* const pOwner, bool const manual)
	{
		return pType && pOwner
			&& SWTypeExt::ExtMap.Find(pType)->MeetsAITargetingConstraints(pOwner, manual);
	}

	bool __stdcall IsSuperWeaponAvailable(SuperWeaponTypeClass* const pType, HouseClass* const pHouse)
	{
		return pType && pHouse && SWTypeExt::ExtMap.Find(pType)->IsAvailable(pHouse);
	}

	bool __stdcall ApplyPermaMindControl(WarheadTypeClass* const pWH, HouseClass* const pOwner,
		AbstractClass* const pTarget)
	{
		return pWH && pTarget && WarheadTypeExt::applyPermaMC(pWH, pOwner, pTarget);
	}

	bool __stdcall DetailsCurrentlyEnabled()
	{
		return RulesExt::DetailsCurrentlyEnabled();
	}

	EBolt* __stdcall CreateElectricBolt(WeaponTypeClass* const pWeapon)
	{
		return pWeapon ? WeaponTypeExt::CreateBolt(pWeapon) : nullptr;
	}

	int __stdcall FindEVAIndex(const char* const pID)
	{
		return pID ? EVAVoices::FindIndex(pID) : -1;
	}

	bool __stdcall CameoIsElite(TechnoTypeClass* const pType, HouseClass* const pHouse)
	{
		return pType && pHouse && TechnoTypeExt::ExtMap.Find(pType)->CameoIsElite(pHouse);
	}

	CDTimerClass* __stdcall GetDisableWeaponTimer(TechnoClass* const pThis)
	{
		return pThis ? &TechnoExt::ExtMap.Find(pThis)->DisableWeaponTimer : nullptr;
	}

	bool* __stdcall GetDriverKilled(TechnoClass* const pThis)
	{
		return pThis ? &TechnoExt::ExtMap.Find(pThis)->DriverKilled : nullptr;
	}

	bool* __stdcall GetInfiltrated(HouseClass* const pHouse, AntaresFactory const factory)
	{
		if(!pHouse) {
			return nullptr;
		}

		// two of these are the game's own, the rest are ours
		switch(factory) {
		case AntaresFactory::WarFactory:
			return &pHouse->WarFactoryInfiltrated;
		case AntaresFactory::Barracks:
			return &pHouse->BarracksInfiltrated;
		case AntaresFactory::NavalYard:
			return &HouseExt::ExtMap.Find(pHouse)->NavalYardInfiltrated;
		case AntaresFactory::AircraftFactory:
			return &HouseExt::ExtMap.Find(pHouse)->AircraftFactoryInfiltrated;
		case AntaresFactory::ConstructionYard:
			return &HouseExt::ExtMap.Find(pHouse)->BuildingInfiltrated;
		}

		return nullptr;
	}

	bool __stdcall IsPsionicsImmune(TechnoTypeClass* const pType, VeterancyStruct const* const pVeterancy)
	{
		if(!pType || !pVeterancy) {
			return false;
		}

		auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
		return pType->ImmuneToPsionics
			|| pExt->HasAbility(AresAbility::PsionicsImmune, *pVeterancy);
	}

	bool __stdcall GetOperators(TechnoTypeClass* const pType, InfantryTypeClass* const** const ppItems,
		int* const pCount, bool* const pAnyAllowed)
	{
		if(!pType) {
			return false;
		}

		auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

		if(ppItems) {
			*ppItems = pExt->Operator.data();
		}
		if(pCount) {
			*pCount = static_cast<int>(pExt->Operator.size());
		}
		if(pAnyAllowed) {
			*pAnyAllowed = pExt->IsAPromiscuousWhoreAndLetsAnyoneRideIt;
		}

		return true;
	}

	bool __stdcall IsVeteranBuilding(HouseTypeClass* const pCountry, BuildingTypeClass* const pType)
	{
		return pCountry && pType
			&& HouseTypeExt::ExtMap.Find(pCountry)->VeteranBuildings.Contains(pType);
	}

	AlphaShapeClass* __stdcall FindAlphaShape(ObjectClass* const pObject)
	{
		return pObject ? TechnoExt::Alpha.get_or_default(pObject) : nullptr;
	}

	bool __stdcall DisableFeature(AntaresFeature const feature)
	{
		return Interop::Disable(feature);
	}

	bool __stdcall IsFeatureDisabled(AntaresFeature const feature)
	{
		return Interop::IsDisabled(feature);
	}

	AntaresAPI_v1 const TheAPI = {
		sizeof(AntaresAPI_v1), 1, 0, 0,

		&ConvertTypeTo,
		&SpawnSurvivors,
		&ReverseEngineer,
		&MeetsAITargetingConstraints,
		&IsSuperWeaponAvailable,
		&ApplyPermaMindControl,
		&DetailsCurrentlyEnabled,
		&CreateElectricBolt,
		&FindEVAIndex,
		&CameoIsElite,

		&GetDisableWeaponTimer,
		&GetDriverKilled,
		&GetInfiltrated,
		&IsPsionicsImmune,
		&GetOperators,
		&IsVeteranBuilding,
		&FindAlphaShape,

		&DisableFeature,
		&IsFeatureDisabled,
	};
}

// A __stdcall export carries its decoration into the export table as
// _GetAntaresAPI@8, and GetProcAddress will not find that under the plain name.
// Alias it so a consumer can simply ask for "GetAntaresAPI"; both names resolve to
// the same function, and the decorated one stays for anything already using it.
#pragma comment(linker, "/EXPORT:GetAntaresAPI=_GetAntaresAPI@8")

DEFINE_EXPORT(HRESULT, GetAntaresAPI, uint32_t wantMajor, AntaresAPI_v1** ppApi)
{
	if(!ppApi) {
		return E_POINTER;
	}

	*ppApi = nullptr;

	if(wantMajor != TheAPI.major) {
		return E_NOTIMPL;
	}

	*ppApi = const_cast<AntaresAPI_v1*>(&TheAPI);
	return S_OK;
}
