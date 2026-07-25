#include <AnimClass.h>
#include <BuildingClass.h>
#include <FootClass.h>

#include "../../Ares.h"

static void DestroyAnim(AnimClass*& pAnim)
{
	if(pAnim) {
		pAnim->UnInit();
		pAnim = nullptr;
	}
}

// deleting the anim outright leaves everything that still points at it with a
// dangling pointer. let it expire instead.
DEFINE_HOOK(0x451A28, BuildingClass_PlayAnim_Destroy, 0x7)
{
	GET(AnimClass* const, pAnim, ECX);

	pAnim->UnInit();

	return 0x451A2F;
}

DEFINE_HOOK(0x451E40, BuildingClass_DestroyNthAnim_Destroy, 0x7)
{
	GET(BuildingClass* const, pThis, ECX);
	GET_STACK(int const, index, 0x4);

	if(index == -2) {
		for(auto& pAnim : pThis->Anims) {
			DestroyAnim(pAnim);
		}
	} else {
		DestroyAnim(pThis->Anims[index]);
	}

	return 0x451E93;
}

// the assaulter killed the occupant, not the other way round
DEFINE_HOOK(0x4586D6, BuildingClass_KillOccupiers, 0x9)
{
	GET(ObjectClass* const, pOccupant, ECX);
	GET(TechnoClass* const, pAssaulter, EBP);

	pOccupant->RegisterDestruction(pAssaulter);

	return 0x4586DF;
}
