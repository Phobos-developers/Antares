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

// Letting the anim expire above means it outlives the building, so the pointer
// invalidation it runs when it finally dies lands in Detach -- which puts the
// Idle and Active anims back on a building that has since been sold, erased or
// destroyed. Under the game's own outright delete this ran while the building
// was still there, so it never showed.
DEFINE_HOOK_AGAIN(0x44E997, BuildingClass_Detach_SkipAnimRestore, 0x6) // Idle
DEFINE_HOOK(0x44E9FA, BuildingClass_Detach_SkipAnimRestore, 0x6) // Active
{
	enum {
		SkipIdleAnim = 0x44E9A4u,
		SkipActiveAnim = 0x44EA07u
	};

	GET(BuildingClass* const, pThis, ESI);

	if(!pThis->InLimbo) {
		return 0;
	}

	return (R->Origin() == 0x44E997u) ? SkipIdleAnim : SkipActiveAnim;
}

// the assaulter killed the occupant, not the other way round
DEFINE_HOOK(0x4586D6, BuildingClass_KillOccupiers, 0x9)
{
	GET(ObjectClass* const, pOccupant, ECX);
	GET(TechnoClass* const, pAssaulter, EBP);

	pOccupant->RegisterDestruction(pAssaulter);

	return 0x4586DF;
}
