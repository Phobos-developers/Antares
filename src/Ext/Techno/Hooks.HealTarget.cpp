#include "Body.h"

#include <HouseClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>

// a heal weapon has business only with a target that is actually damaged
static bool IsDamagedTechno(AbstractClass* const pTarget)
{
	if(!pTarget || (pTarget->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) {
		return false;
	}

	auto const pObject = static_cast<ObjectClass*>(pTarget);
	return pObject->GetHealthPercentage() < RulesClass::Instance->ConditionGreen;
}

DEFINE_HOOK(0x51C913, InfantryClass_CanFire_Heal, 0x7)
{
	GET(AbstractClass* const, pTarget, EDI);

	return IsDamagedTechno(pTarget) ? 0x51C947 : 0x51C939;
}

DEFINE_HOOK(0x520731, InfantryClass_UpdateFiringState_Heal, 0x5)
{
	GET(InfantryClass* const, pThis, EBP);

	if(!IsDamagedTechno(pThis->Target)) {
		pThis->SetTarget(nullptr);
	}

	return 0x52094C;
}

DEFINE_HOOK(0x736E8E, UnitClass_UpdateFiringState_Heal, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	if(!IsDamagedTechno(pThis->Target)) {
		pThis->SetTarget(nullptr);
	}

	return 0x737063;
}

DEFINE_HOOK(0x741113, UnitClass_CanFire_Heal, 0xA)
{
	GET(ObjectClass* const, pTarget, EDI);

	return pTarget->GetHealthPercentage() < RulesClass::Instance->ConditionGreen
		? 0x741121 : 0x74113A;
}

DEFINE_HOOK(0x6F7FC5, TechnoClass_CanAutoTargetObject_Heal, 0x7)
{
	return 0x6F7FDF;
}

DEFINE_HOOK_AGAIN(0x6F8F1F, TechnoClass_FindTargetType_Heal, 0x6)
DEFINE_HOOK(0x6F8EE3, TechnoClass_FindTargetType_Heal, 0x6)
{
	R->EBX(R->EBX() | 0x403Cu);
	return 0x6F8F25;
}

DEFINE_HOOK(0x6FA361, TechnoClass_Update_LoseTarget, 0x5)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pHouse = (R->BL() != 0)
		? R->EDI<HouseClass*>() : pThis->Owner;

	auto isAlly = false;

	if(auto const pTarget = pThis->Target) {
		if((pTarget->AbstractFlags & AbstractFlags::Object) != AbstractFlags::None) {
			isAlly = pHouse->IsAlliedWith(pTarget->GetOwningHouse());
		}
	}

	return (isAlly == (pThis->CombatDamage(-1) < 0)) ? 0x6FA472 : 0x6FA39D;
}
