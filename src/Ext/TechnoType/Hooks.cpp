#include "Body.h"
#include "../Building/Body.h"
//include "Side.h"
#include "../../Enum/Prerequisites.h"
#include "../../Misc/Debug.h"
#include "../Rules/Body.h"
#include "../../Utilities/TemplateDef.h"

#include <HouseTypeClass.h>
#include <InfantryTypeClass.h>

// =============================
// other hooks

DEFINE_HOOK(0x71136F, TechnoTypeClass_CTOR_Initialize, 0x6)
{
	GET(TechnoTypeClass*, pThis, ESI);

	pThis->WeaponCount = 0;
	pThis->Bunkerable = false;
	pThis->Parasiteable = false;
	pThis->ImmuneToPoison = false;
	pThis->ConsideredAircraft = false;
	pThis->Organic = false;

	return 0;
}

DEFINE_HOOK(0x523932, InfantryTypeClass_CTOR_Initialize, 0x8)
{
	GET(InfantryTypeClass*, pThis, ESI);

	for(auto& sequence : pThis->Sequence->Sequences) {
		sequence.StartFrame = 0;
		sequence.CountFrames = 0;
		sequence.FacingMultiplier = 0;
		sequence.Facing = static_cast<SequenceFacing>(-1);
		sequence.SoundCount = 0;
		sequence.Sound1StartFrame = 0;
		sequence.Sound1Index = 0;
		sequence.Sound2StartFrame = 0;
		sequence.Sound2Index = 0;
	}

	return 0x523970;
}

DEFINE_HOOK(0x732D47, TacticalClass_CollectSelectedIDs, 0x5)
{
	// the game created this vector on its own heap and with its own vtable,
	// so the game's destructor frees it again.
	GET(DynamicVectorClass<const char*>*, pNames, EBX);

	auto Add = [pNames](TechnoTypeClass* pType) {
		if(auto pExt = TechnoTypeExt::ExtMap.Find(pType)) {
			const char* id = pExt->GetSelectionGroupID();

			if(std::none_of(pNames->begin(), pNames->end(), [id](const char* pID) {
				return !_strcmpi(pID, id);
			})) {
				pNames->AddItem(id);
			}
		}
	};

	bool useDeploy = RulesExt::Global()->TypeSelectUseDeploy;

	for(auto pObject : ObjectClass::CurrentObjects) {
		// add this object's id used for grouping
		if(TechnoTypeClass* pType = pObject->GetTechnoType()) {
			Add(pType);

			// optionally do the same the original game does, but support the new grouping feature.
			if(useDeploy) {
				if(pType->DeploysInto) {
					Add(pType->DeploysInto);
				}
				if(pType->UndeploysInto) {
					Add(pType->UndeploysInto);
				}
			}
		}
	}

	R->EAX(pNames);
	return 0x732FD9;
}

DEFINE_HOOK(0x7327AA, TechnoClass_PlayerOwnedAliveAndNamed_GroupAs, 0x8)
{
	GET(TechnoClass*, pThis, ESI);
	GET(const char*, pID, EDI);

	bool ret = TechnoTypeExt::HasSelectionGroupID(pThis->GetTechnoType(), pID);

	R->EAX<int>(ret);
	return 0x7327B2;
}

DEFINE_HOOK_AGAIN(0x4ABD9D, DisplayClass_LeftMouseButtonUp_GroupAs, 0xA)
DEFINE_HOOK_AGAIN(0x4ABE58, DisplayClass_LeftMouseButtonUp_GroupAs, 0xA)
DEFINE_HOOK(0x4ABD6C, DisplayClass_LeftMouseButtonUp_GroupAs, 0xA)
{
	GET(ObjectClass*, pThis, ESI);
	R->EAX(TechnoTypeExt::GetSelectionGroupID(pThis->GetType()));
	return R->Origin() + 13;
}

DEFINE_HOOK(0x6DA665, sub_6DA5C0_GroupAs, 0xA)
{
	GET(ObjectClass*, pThis, ESI);
	R->EAX(TechnoTypeExt::GetSelectionGroupID(pThis->GetType()));
	return R->Origin() + 13;
}

/*
A_FINE_HOOK(0x5F8480, ObjectTypeClass_Load3DArt, 0x6)
{
	GET(ObjectTypeClass *, O, ESI);
	if(O->WhatAmI() == abs_UnitType) {
		TechnoTypeExt::ExtData *pData = TechnoTypeExt::ExtMap.Find(reinterpret_cast<TechnoTypeClass*>(O));
		if(pData->WaterAlt) {
			return 0x5F848C;
		}
	}
	return 0;
}
*/

DEFINE_HOOK(0x715320, TechnoTypeClass_LoadFromINI_EarlyReader, 0x6)
{
	GET(CCINIClass *, pINI, EDI);
	GET(TechnoTypeClass *, pType, EBP);

	INI_EX exINI(pINI);
	TechnoTypeExt::ExtData *pData = TechnoTypeExt::ExtMap.Find(pType);

	pData->WaterImage.Read(exINI, pType->ID, "WaterImage");

	return 0;
}


DEFINE_HOOK(0x4444E2, BuildingClass_KickOutUnit_FindAlternateKickout, 0x6)
{
	GET(BuildingClass *, Src, ESI);
	GET(BuildingClass *, Tst, EBP);
	GET(TechnoClass *, Production, EDI);

	auto pData = TechnoTypeExt::ExtMap.Find(Production->GetTechnoType());

	if(Src != Tst
	 && Tst->GetCurrentMission() == Mission::Guard
	 && Tst->Type->Factory == Src->Type->Factory
	 && Tst->Type->Naval == Src->Type->Naval
	 && pData->CanBeBuiltAt(Tst->Type)
	 && !Tst->Factory)
	{
		return 0x44451F;
	}

	return 0x444508;
}

DEFINE_HOOK(0x444DBC, BuildingClass_KickOutUnit_Infantry, 0x5) {
	GET(TechnoClass*, Production, EDI);
	GET(BuildingClass*, Factory, ESI);

	// turn it off
	--Unsorted::ScenarioInit;

	auto pFactoryData = BuildingExt::ExtMap.Find(Factory);
	pFactoryData->KickOutClones(Production);

	// turn it back on so the game can turn it off again
	++Unsorted::ScenarioInit;

	return 0;
}

DEFINE_HOOK(0x4445F6, BuildingClass_KickOutUnit_Clone_NonNavalUnit, 0x5) {
	GET(TechnoClass*, Production, EDI);
	GET(BuildingClass*, Factory, ESI);

	// turn it off
	--Unsorted::ScenarioInit;

	auto pFactoryData = BuildingExt::ExtMap.Find(Factory);
	pFactoryData->KickOutClones(Production);

	// turn it back on so the game can turn it off again
	++Unsorted::ScenarioInit;

	return 0x444971;
}

DEFINE_HOOK(0x44441A, BuildingClass_KickOutUnit_Clone_NavalUnit, 0x6) {
	GET(TechnoClass*, Production, EDI);
	GET(BuildingClass*, Factory, ESI);

	auto pFactoryData = BuildingExt::ExtMap.Find(Factory);
	pFactoryData->KickOutClones(Production);

	return 0;
}

DEFINE_HOOK(0x4449DF, BuildingClass_KickOutUnit_PreventClone, 0x6)
{
	return 0x444A53;
}
