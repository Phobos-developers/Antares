#include "Body.h"

#include <AnimClass.h>
#include <CellClass.h>
#include <IonBlastClass.h>
#include <MapClass.h>
#include <RulesClass.h>

#include <algorithm>

DEFINE_HOOK(0x53CB91, IonBlastClass_DTOR, 0x6)
{
	GET(IonBlastClass*, pThis, ECX);

	WarheadTypeExt::IonExt.erase(pThis);

	return 0;
}

DEFINE_HOOK(0x53CC63, IonBlastClass_Update_Beam, 0x6)
{
	GET(IonBlastClass*, pThis, EBX);

	auto const pExt = WarheadTypeExt::IonExt.get_or_default(pThis);
	if(!pExt) {
		return 0;
	}

	auto const pRules = RulesClass::Instance;
	auto const& location = pThis->Location;
	auto const pCell = MapClass::Instance.GetCellAt(location);

	CoordStruct blastCoords = { location.X, location.Y, location.Z + 5 };

	auto const pBlast = (pCell->LandType == LandType::Water)
		? pRules->SplashList.GetItem(pRules->SplashList.Count - 1)
		: pExt->IonCannon_Blast.Get(pRules->IonBlast);

	if(pBlast) {
		GameCreate<AnimClass>(pBlast, blastCoords);
	}

	if(auto const pBeam = pExt->IonCannon_Beam.Get(pRules->IonBeam)) {
		GameCreate<AnimClass>(pBeam, blastCoords);
	}

	auto const pWarhead = pExt->IonCannon_Warhead.Get(pRules->IonCannonWarhead);
	auto const damage = pExt->IonCannon_Damage.Get(pRules->IonCannonDamage);

	if(pWarhead) {
		if(static_cast<bool>(pCell->Flags & CellFlags::BridgeHead)) {
			CoordStruct bridge = {
				location.X, location.Y, location.Z + CellClass::BridgeHeight };
			MapClass::DamageArea(bridge, damage, nullptr, pWarhead, true, nullptr);
		}

		MapClass::DamageArea(location, damage, nullptr, pWarhead, true, nullptr);
		MapClass::FlashbangWarheadAt(damage, pWarhead, location);
	}

	return pExt->IonCannon_Rock ? 0x53CE0A : 0x53D302;
}

DEFINE_HOOK(0x53CC0D, IonBlastClass_Update_DTOR, 0x5)
{
	GET(IonBlastClass *, IB, EBX);
	WarheadTypeExt::IonExt.erase(IB);
	return 0;
}

DEFINE_HOOK(0x53CBF5, IonBlastClass_Update_Duration, 0x5)
{
	GET(IonBlastClass *, IB, EBX);

	int Ripple_Radius = 79;
	if(auto pData = WarheadTypeExt::IonExt.get_or_default(IB)) {
		Ripple_Radius = std::min(Ripple_Radius, pData->Ripple_Radius + 1);
	}

	if(IB->Lifetime < Ripple_Radius) {
//		++IB->Lifetime;
		return 0x53CC3A;
	} else {
		return 0x53CBFA;
	}
}
