#include <AnimClass.h>
#include <FootClass.h>
#include <RulesClass.h>
#include <TechnoClass.h>

#include "../Ares.h"

// limbo the object first. only something that is still on the map after that
// needs every other object told about it.
DEFINE_HOOK(0x5F6612, ObjectClass_UnInit_SkipInvalidation, 0x9)
{
	GET(ObjectClass* const, pThis, ESI);

	if(!pThis->Limbo()) {
		pThis->AnnounceExpiredPointer(true);
	}

	return 0x5F6625;
}

// the Behind anim is created and destroyed for every drawn object, and
// nothing ever keeps a pointer to it
DEFINE_HOOK(0x725A1F, AnnounceInvalidPointer_SkipBehind, 0x5)
{
	GET(AnimClass* const, pThis, ESI);

	return (pThis->Type == RulesClass::Instance->Behind) ? 0x725C08 : 0;
}

// the last target is still read while the unit picks its next mission, so it
// has to be cleared before that happens rather than after
DEFINE_HOOK(0x707A47, TechnoClass_PointerGotInvalid_LastTarget, 0xA)
{
	GET(TechnoClass*, pThis, ESI);
	GET(void*, ptr, EBP);

	if(pThis->LastTarget == ptr) {
		pThis->LastTarget = nullptr;
	}

	return 0;
}

// issues 1002020, 896263, 895954: clear stale mind control pointer to prevent
// crashes when accessing properties of the destroyed controllers.
DEFINE_HOOK(0x707B09, TechnoClass_PointerGotInvalid_ResetMindControl, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(void*, ptr, EBP);

	if(pThis->MindControlledBy == ptr) {
		pThis->MindControlledBy = nullptr;
	}

	return 0;
}
