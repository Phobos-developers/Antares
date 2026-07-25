#include "Body.h"
#include "../BuildingType/Body.h"

#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <Drawing.h>
#include <HouseClass.h>
#include <StringTable.h>
#include <Surface.h>
#include <UnitClass.h>

#include <cstdio>

// enemies see EnemyUIName instead of the real name, unless they infiltrated
// this building. allies and observers always get the real name.
DEFINE_HOOK(0x459ED0, BuildingClass_GetUIName, 0x6)
{
	GET(BuildingClass* const, pThis, ECX);

	auto const pType = pThis->Type;
	auto pName = pType->UIName;

	if(!HouseClass::IsCurrentPlayerObserver()) {
		auto const pPlayer = HouseClass::CurrentPlayer;

		if(!pPlayer || !pPlayer->IsAlliedWith(pThis->Owner)) {
			if(!pThis->DisplayProductionTo.Contains(pPlayer)) {
				auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

				if(pExt->EnemyUIName.isset()) {
					pName = pExt->EnemyUIName.Get();
				}
			}
		}
	}

	R->EAX(pName);
	return 0x459ED9;
}

// the text drawn over a selected object: fakes announce themselves, power
// plants show the house's power, silos its credits and factories say primary.
DEFINE_HOOK(0x70AA60, TechnoClass_DrawExtraInfo, 0x6)
{
	GET(TechnoClass* const, pThis, ECX);
	GET_STACK(Point2D* const, pLocation, 0x8);
	GET_STACK(RectangleStruct* const, pBounds, 0xC);

	BuildingTypeClass* pBuildingType = nullptr;
	BuildingTypeExt::ExtData* pBuildingExt = nullptr;

	if(pThis && pThis->WhatAmI() == BuildingClass::AbsID) {
		pBuildingType = static_cast<BuildingClass*>(pThis)->Type;
		pBuildingExt = BuildingTypeExt::ExtMap.Find(pBuildingType);
	}

	auto const pOwner = pThis->Owner;

	auto const allied = HouseClass::CurrentPlayer
		&& HouseClass::CurrentPlayer->IsAlliedWith(pOwner);

	auto const show = allied && !HouseClass::IsCurrentPlayerObserver();

	wchar_t const* pText = nullptr;
	wchar_t buffer[0x80];

	if(pBuildingExt && pBuildingExt->Fake) {
		pText = StringTable::LoadString("TXT_FAKE");
	} else if(pBuildingType && pBuildingType->PowerBonus > 0) {
		auto const pFormat = StringTable::LoadString("TXT_POWER_DRAIN2");
		_snwprintf_s(buffer, _TRUNCATE, pFormat,
			pOwner->Power_Output(), pOwner->Power_Drain());
		pText = buffer;
	} else if(pBuildingType && pBuildingType->Storage > 0 && !show) {
		auto const pFormat = StringTable::LoadString("TXT_MONEY_FORMAT_1");
		_snwprintf_s(buffer, _TRUNCATE, pFormat, pOwner->Available_Money());
		pText = buffer;
	} else if(pThis->IsPrimaryFactory && show) {
		auto const single = pBuildingType && pBuildingType->GetFoundationWidth() == 1;
		pText = StringTable::LoadString(single ? "TXT_PRI" : "TXT_PRIMARY");
	}

	if(pText) {
		TextPrintType const flags = TextPrintType::Center
			| TextPrintType::FullShadow | TextPrintType::Efnt;

		auto box = Drawing::GetTextDimensions(pText, *pLocation,
			static_cast<WORD>(flags), 4, 2);
		auto rect = Drawing::Intersect(box, *pBounds, nullptr, nullptr);

		auto const color = static_cast<unsigned int>(
			static_cast<WORD>(Drawing::RGB_To_Int(pOwner->Color)));

		DSurface::Temp->FillRect(&rect, 0);
		DSurface::Temp->DrawRect(&rect, color);

		Point2D dummy;
		Simple_Text_Print_Wide(&dummy, pText, DSurface::Temp, pBounds,
			pLocation, color, 0, flags, 1);
	}

	return 0x70AD4C;
}

// the value of the current selection, as shown by the select-by-type keyboard
// commands. decoys are worth what they pretend to be worth.
DEFINE_HOOK(0x731E08, sub_731D90_FakeOf, 0x6)
{
	auto value = 0;

	for(auto const& pObject : ObjectClass::CurrentObjects) {
		if(auto const pTechno = generic_cast<TechnoClass*>(pObject)) {
			auto pType = pTechno->GetTechnoType();
			auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

			if(pExt->FakeOf) {
				pType = pExt->FakeOf;
			}

			value += pType->GetActualCost(pTechno->Owner);
		}
	}

	R->EBX(value);
	return 0x731E4D;
}

// name the weapon a multi-weapon unit currently uses instead of assembling the
// name from the gunner and the transport.
DEFINE_HOOK(0x746B89, UnitClass_GetUIName, 0x8)
{
	GET(UnitClass* const, pThis, ESI);

	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	wchar_t const* pName = nullptr;

	auto const index = static_cast<size_t>(pThis->CurrentWeaponNumber);
	if(index < pExt->WeaponUINames.size()) {
		pName = pExt->WeaponUINames[index];
	}

	R->EAX(pName);

	return pName ? 0x746C78 : 0;
}
