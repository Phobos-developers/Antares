#include "Body.h"
#include "../Rules/Body.h"
#include "../TechnoType/Body.h"

#include <HouseClass.h>

DEFINE_HOOK(0x70A990, TechnoClass_DrawVeterancy, 0x5)
{
	GET(TechnoClass *, T, ECX);
	GET_STACK(Point2D *, XY, 0x4);
	GET_STACK(RectangleStruct *, pRect, 0x8);

	Point2D offset = *XY;

	SHPStruct *iFile = FileSystem::PIPS_SHP;
	int iFrame = -1;
	TechnoTypeExt::ExtData *pTypeData = TechnoTypeExt::ExtMap.Find(T->GetTechnoType());

	bool canSee = T->Owner->IsAlliedWith(HouseClass::CurrentPlayer)
		|| HouseClass::IsCurrentPlayerObserver()
		|| pTypeData->Insignia_ShowEnemy.Get(RulesExt::Global()->EnemyInsignia);

	if(canSee) {
		// The frame the rank would use if InsigniaFrame does not override it:
		// 0 for a custom insignia SHP, else the two hardcoded pips.shp frames.
		int fallback = -1;

		if(SHPStruct *fCustom = pTypeData->Insignia.Get(T)) {
			iFile = fCustom;
			fallback = 0;
		} else {
			VeterancyStruct *XP = &T->Veterancy;
			if(XP->IsElite()) {
				fallback = 15;
			} else if(XP->IsVeteran()) {
				fallback = 14;
			}
		}

		// InsigniaFrame.%s applies to the custom and the default shape alike;
		// -1 means "keep the default frame index".
		iFrame = pTypeData->InsigniaFrame.Get(T);
		if(iFrame < 0) {
			iFrame = fallback;
		}
	}

	if(iFrame >= 0 && iFile) {
		offset.X += 5;
		offset.Y += 2;
		if(T->WhatAmI() != AbstractType::Infantry) {
			offset.X += 5;
			offset.Y += 4;
		}

		DSurface::Temp->DrawSHP(
			FileSystem::PALETTE_PAL, iFile, iFrame, &offset, pRect, BlitterFlags(0xE00), 0, -2, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}

	return 0x70AA5B;
}
