#include "Body.h"

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>

DEFINE_HOOK(0x489235, GetTotalDamage_Verses, 0x8)
{
	GET(WarheadTypeClass*, pWH, EDI);
	GET(int, armor, EDX);
	GET(int, damage, ECX);

	auto const pExt = WarheadTypeExt::ExtMap.Find(pWH);
	R->EAX(static_cast<int>(damage * pExt->Verses[armor].Verses));

	return 0x489249;
}

DEFINE_HOOK(0x6F36E3, TechnoClass_SelectWeapon_Verses, 0x5)
{
	GET(TechnoClass*, pTarget, EBP);
	GET_STACK(WeaponTypeClass*, pPrimary, 0x10);
	GET_STACK(WeaponTypeClass*, pSecondary, 0x14);

	auto const armor = pTarget->GetTechnoType()->Armor;

	if(WarheadTypeExt::ExtMap.Find(pPrimary->Warhead)->GetVerses(armor).Verses == 0.0) {
		return 0x6F37AD;
	}

	return WarheadTypeExt::ExtMap.Find(pSecondary->Warhead)->GetVerses(armor).Verses == 0.0
		? 0x6F3745
		: 0x6F3754
	;
}

DEFINE_HOOK(0x6F7D3D, TechnoClass_CanAutoTargetObject_Verses, 0x7)
{
	GET(WarheadTypeClass*, pWH, ECX);
	GET(int, armor, EAX);

	return WarheadTypeExt::ExtMap.Find(pWH)->Verses[armor].PassiveAcquire
		? 0x6F7D55
		: 0x6F894F
	;
}

DEFINE_HOOK(0x6FCB6A, TechnoClass_CanFire_Verses, 0x7)
{
	GET(WarheadTypeClass*, pWH, EDI);
	GET(int, armor, EAX);

	auto const& verses = WarheadTypeExt::ExtMap.Find(pWH)->Verses[armor];

	return verses.ForceFire || verses.Verses != 0.0
		? 0x6FCB8D
		: 0x6FCB7E
	;
}

DEFINE_HOOK(0x708AF7, TechnoClass_ShouldRetaliate_Verses, 0x7)
{
	GET(WarheadTypeClass*, pWH, ECX);
	GET(int, armor, EAX);

	return WarheadTypeExt::ExtMap.Find(pWH)->Verses[armor].Retaliate
		? 0x708B0B
		: 0x708B17
	;
}

DEFINE_HOOK(0x70CEA0, TechnoClass_EvalThreatRating_Verses1, 0x6)
{
	GET(WarheadTypeClass*, pWH, EAX);
	GET(TechnoTypeClass*, pType, EBX);
	GET(TechnoClass*, pTarget, ESI);
	GET(AbstractClass*, pThis, EDI);

	auto value = R->Stack<double>(0x18)
		* WarheadTypeExt::ExtMap.Find(pWH)->GetVerses(pType->Armor).Verses;

	if(pTarget->Target == pThis) {
		value = -value;
	}

	R->Stack(0x10, value);

	return 0x70CED2;
}

DEFINE_HOOK(0x70CF45, TechnoClass_EvalThreatRating, 0xB)
{
	GET(WarheadTypeClass*, pWH, ECX);
	GET(int, armor, EAX);

	auto const verses = WarheadTypeExt::ExtMap.Find(pWH)->Verses[armor].Verses;
	R->Stack(0x10, R->Stack<double>(0x10) + R->Stack<double>(0x30) * verses);

	return 0x70CF58;
}

DEFINE_HOOK(0x75DDCC, Verses_OrigParser, 0x7)
{
	// should really be doing something smarter due to westwood's weirdass code, but cannot be bothered atm
	// will fix if it is reported to actually break things
	// this breaks 51E33D which stops infantry with verses (heavy=0 & steel=0) from targeting non-infantry at all
	// (whoever wrote that code must have quite a few gears missing in his head)
	return 0x75DE98;
}
