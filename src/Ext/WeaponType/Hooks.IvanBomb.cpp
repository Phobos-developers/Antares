#include <BombListClass.h>

#include "Body.h"
#include "../Techno/Body.h"

#include <Helpers/Iterators.h>
#include <BulletClass.h>
#include <HouseClass.h>
#include <WarheadTypeClass.h>

// custom ivan bomb attachment
// bugfix #385: Only InfantryTypes can use Ivan Bombs
DEFINE_HOOK(0x438E86, BombListClass_Plant_AllTechnos, 0x5)
{
	GET(TechnoClass *, Source, EBP);
	switch(Source->WhatAmI()) {
		case AbstractType::Aircraft:
		case AbstractType::Infantry:
		case AbstractType::Unit:
		case AbstractType::Building:
			return 0x438E97;
		default:
			return 0x439022;
	}
}

DEFINE_HOOK(0x438FD7, BombListClass_Plant_AttachSound, 0x7)
{
	return 0x439022;
}

DEFINE_HOOK(0x438A00, BombClass_GetCurrentFrame, 0x6)
{
	GET(BombClass*, pThis, ECX);

	auto pData = WeaponTypeExt::BombExt.get_or_default(pThis);
	if(!pData) {
		return 0;
	}

	SHPStruct* pSHP = pData->Ivan_Image.Get(RulesClass::Instance->BOMBCURS_SHP);
	int frame = 0;

	if(pSHP->Frames >= 2) {
		if(pThis->DeathBomb == FALSE) {
			int const delay = pData->Ivan_Delay.Get(RulesClass::Instance->IvanTimedDelay);
			int const rate = pData->Ivan_FlickerRate.Get(RulesClass::Instance->IvanIconFlickerRate);

			int const lifetime = Unsorted::CurrentFrame - pThis->PlantingFrame;

			// the shape holds a dark and a lit frame for every stage, so there
			// are half as many stages as there are frames
			int const stages = pSHP->Frames / 2;
			int const last = stages - 1;

			if(rate > 0) {
				int stage = lifetime / (delay / stages);
				if(stage > last) {
					stage = last;
				}

				// the lit frame is the odd one of the pair
				frame = 2 * stage;
				if(Unsorted::CurrentFrame % (2 * rate) >= rate) {
					++frame;
				}
			} else {
				// with the flicker off the pairs are stepped through twice as
				// fast, and only the dark frames are ever shown
				frame = lifetime / (delay / (2 * stages));
				if(frame > last) {
					frame = last;
				}
			}
		} else {
			// DeathBombs (that don't exist) use the last frame
			frame = pSHP->Frames - 1;
		}
	}

	R->EAX(frame);
	return 0x438A62;
}

// 6F523C, 5
// custom ivan bomb drawing
DEFINE_HOOK(0x6F523C, TechnoClass_DrawExtras_IvanBombImage, 0x5)
{
	GET(TechnoClass*, pThis, EBP);
	auto pBomb = pThis->AttachedBomb;

	auto pData = WeaponTypeExt::BombExt.get_or_default(pBomb);

	if(SHPStruct* pImage = pData->Ivan_Image.Get(RulesClass::Instance->BOMBCURS_SHP)) {
		R->ECX(pImage);
		return 0x6F5247;
	}
	return 0;
}

// 6FCBAD, 6
// custom ivan bomb disarm 1
DEFINE_HOOK(0x6FCBAD, TechnoClass_CanFire_IvanBomb, 0x6)
{
	GET(TechnoClass *, Target, EBP);
	GET(WarheadTypeClass *, Warhead, EDI);
	if(Warhead->BombDisarm) {
		if(BombClass *Bomb = Target->AttachedBomb) {
			auto pData = WeaponTypeExt::BombExt.get_or_default(Bomb);
			if(!pData->Ivan_Detachable) {
				return 0x6FCBBE;
			}
		}
	}
	return 0;
}

// whether the local player may set off the bomb attached to this object
static bool BombCanDetonate(TechnoClass const* const pThis)
{
	auto const pBomb = pThis->AttachedBomb;

	if(!pBomb || !pBomb->OwnerHouse->IsControlledByCurrentPlayer()) {
		return false;
	}

	auto const pExt = WeaponTypeExt::BombExt.get_or_default(pBomb);

	return (pBomb->IsDeathBomb() == FALSE)
		? pExt->Ivan_CanDetonateTimeBomb.Get(RulesClass::Instance->CanDetonateTimeBomb)
		: pExt->Ivan_CanDetonateDeathBomb.Get(RulesClass::Instance->CanDetonateDeathBomb);
}

// 6FFEC0, 5
DEFINE_HOOK(0x6FFEC0, TechnoClass_GetActionOnObject_IvanBombsA, 0x5)
{
	GET(TechnoClass const* const, pThis, ECX);
	GET_STACK(ObjectClass const* const, pTarget, 0x4);

	if(pThis != pTarget || ObjectClass::CurrentObjects.Count != 1
		|| !BombCanDetonate(pThis))
	{
		return 0;
	}

	R->EAX(Action::Detonate);

	return 0x7005EF;
}

// 51E488, 5
DEFINE_HOOK(0x51E488, InfantryClass_GetActionOnObject2, 0x5)
{
	GET(TechnoClass *, Target, ESI);
	BombClass *Bomb = Target->AttachedBomb;

	auto pData = WeaponTypeExt::BombExt.get_or_default(Bomb);
	if(!pData->Ivan_Detachable) {
		return 0x51E49E;
	}
	return 0;
}

// 438799, 6
// custom ivan bomb detonation 1
DEFINE_HOOK(0x438799, BombClass_Detonate1, 0x6)
{
	GET(BombClass *, Bomb, ESI);

	auto pData = WeaponTypeExt::BombExt.get_or_default(Bomb);

	R->Stack<WarheadTypeClass *>(0x4, pData->Ivan_WH.Get(RulesClass::Instance->IvanWarhead));
	R->EDX(pData->Ivan_Damage.Get(RulesClass::Instance->IvanDamage));
	return 0x43879F;
}

// 438843, 6
// custom ivan bomb detonation 2
DEFINE_HOOK(0x438843, BombClass_Detonate2, 0x6)
{
	GET(BombClass *, Bomb, ESI);

	auto pData = WeaponTypeExt::BombExt.get_or_default(Bomb);

	R->EDX<WarheadTypeClass *>(pData->Ivan_WH.Get(RulesClass::Instance->IvanWarhead));
	R->ECX(pData->Ivan_Damage.Get(RulesClass::Instance->IvanDamage));
	return 0x438849;
}

// 438879, 6
// custom ivan bomb detonation 3
DEFINE_HOOK(0x438879, BombClass_Detonate3, 0x6)
{
	GET(BombClass *, Bomb, ESI);

	auto pData = WeaponTypeExt::BombExt.get_or_default(Bomb);
	return pData->Ivan_KillsBridges ? 0 : 0x438989;
}

// 4393F2, 5
// custom ivan bomb cleanup
DEFINE_HOOK(0x4393F2, BombClass_SDDTOR, 0x5)
{
	GET(BombClass *, Bomb, ECX);
	WeaponTypeExt::BombExt.erase(Bomb);
	return 0;
}

/* this is a wtf: it unsets target if the unit can no longer affect its current target.
 * Makes sense, except Aircraft that lose the target so crudely in the middle of the attack
 * (i.e. ivan bomb weapon) go wtfkerboom with an IE
 */
DEFINE_HOOK(0x6FA4C6, TechnoClass_Update_ZeroOutTarget, 0x5)
{
	GET(TechnoClass *, T, ESI);
	return (T->WhatAmI() == AbstractType::Aircraft) ? 0x6FA4D1 : 0;
}

DEFINE_HOOK(0x46934D, BulletClass_DetonateAt_IvanBombs, 0x6)
{
	GET(BulletClass *, pBullet, ESI);

	if(TechnoClass* pOwner = generic_cast<TechnoClass *>(pBullet->Owner)) {
		if(auto pExt = WeaponTypeExt::ExtMap.Find(pBullet->GetWeaponType())) {

			// single target or spread switch
			if(pBullet->WH->CellSpread < 0.5f) {

				// single target
				if(auto pTarget = generic_cast<TechnoClass*>(pBullet->Target)) {
					pExt->PlantBomb(pOwner, pTarget);
				}
			} else {

				// cell spread
				int Spread = static_cast<int>(pBullet->WH->CellSpread);

				CoordStruct tgtCoords = pBullet->GetTargetCoords();

				CellStruct centerCoords = MapClass::Instance.GetCellAt(tgtCoords)->MapCoords;

				CellSpreadIterator<TechnoClass>{}(centerCoords, Spread,
					[pOwner, pExt](TechnoClass* pTechno)
				{
					if(pTechno != pOwner && !pTechno->AttachedBomb) {
						pExt->PlantBomb(pOwner, pTechno);
					}
					return true;
				});
			}
		} else {
			Debug::Log(Debug::Severity::Warning, "IvanBomb bullet without attached WeaponType.\n");
		}
	}

	return 0x469AA4;
}

// the detonate action is decided at the top of the function now
DEFINE_HOOK(0x6FFF9E, TechnoClass_GetActionOnObject_IvanBombsB, 0x8)
{
	return 0x700006;
}

// berserk objects still obey a click that sets off their own bomb
DEFINE_HOOK(0x51F1D8, InfantryClass_ActionOnObject_IvanBombs, 0x6)
{
	return 0x51F1EA;
}

DEFINE_HOOK(0x7388EB, UnitClass_ActionOnObject_IvanBombs, 0x6)
{
	return 0x7388FD;
}

// #896027: do not announce pointers as expired to bombs
// if the pointed to object is staying in-game.
DEFINE_HOOK(0x725961, AnnounceInvalidPointer_BombCloak, 0x6)
{
	GET(bool, remove, EDI);
	return remove ? 0 : 0x72596C;
}
