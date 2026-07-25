#include "Body.h"
#include "../BulletType/Body.h"

#include <AnimClass.h>
#include <BulletClass.h>
#include <ScenarioClass.h>
#include <TacticalClass.h>
#include <TechnoClass.h>
#include <VoxelAnimClass.h>
#include <WeaponTypeClass.h>

#include <cstdlib>
#include <cstring>

DEFINE_HOOK(0x4239F0, AnimClass_UpdateBounce_Damage, 0x8)
{
	GET(ObjectClass*, pObject, EDI);
	GET(AnimClass*, pThis, EBP);

	if(pObject) {
		auto const pType = pThis->Type;

		if(pType->DamageRadius >= 0 && pType->Warhead
			&& static_cast<int>(pType->Damage))
		{
			return 0x4239F8;
		}
	}

	return 0x423A92;
}

DEFINE_HOOK(0x424538, AnimClass_Update_DamageDelay, 0x6)
{
	GET(AnimClass*, pThis, ESI);

	auto const pType = pThis->Type;
	auto const pExt = AnimTypeExt::ExtMap.Find(pType);

	// terrain objects catch five times the damage
	auto const multiplier = (pThis->OwnerObject
		&& pThis->OwnerObject->WhatAmI() == AbstractType::Terrain) ? 5 : 1;

	int damage;
	bool spend;

	if(pExt->Damage_Delay > 0 && pType->Damage >= 1.0) {
		// the accumulator counts frames, and the full damage lands in one go
		spend = false;
		pThis->Accum += 1.0;

		if(pThis->Accum < pExt->Damage_Delay) {
			return 0x42465D;
		}

		damage = static_cast<int>(pType->Damage) * multiplier;
	} else {
		spend = true;
		pThis->Accum += multiplier * pType->Damage;

		if(pThis->Accum < 1.0) {
			return 0x42465D;
		}

		damage = static_cast<int>(pThis->Accum);
	}

	if(damage <= 0 || pThis->IsInert) {
		return 0x42465D;
	}

	pThis->Accum = spend ? pThis->Accum - damage : 0.0;

	if(auto const pWeapon = pExt->Weapon) {
		auto const pBulletExt = BulletTypeExt::ExtMap.Find(pWeapon->Projectile);

		if(auto const pBullet = pBulletExt->CreateBullet(pThis->GetCell(), nullptr,
			damage, pWeapon->Warhead, 0, 0, pWeapon->Bright))
		{
			pBullet->WeaponType = pWeapon;
			pBullet->Limbo();
			pBullet->Detonate(pThis->GetCoords());
			pBullet->UnInit();
		}

		return 0x42464C;
	}

	auto pWarhead = pType->Warhead;
	if(!pWarhead) {
		pWarhead = strcmp(pType->ID, "INVISO")
			? RulesClass::Instance->FlameDamage2
			: RulesClass::Instance->C4Warhead;
	}

	auto pOwner = pThis->Owner;
	if(!pOwner) {
		if(auto const pTechno = generic_cast<TechnoClass*>(pThis->OwnerObject)) {
			pOwner = pTechno->Owner;
		}
	}

	MapClass::DamageArea(pThis->GetCoords(), damage, nullptr, pWarhead, true, pOwner);

	return 0x42464C;
}

DEFINE_HOOK(0x74A884, VoxelAnimClass_Update_Damage, 0x6)
{
	GET(VoxelAnimClass*, pThis, EBX);

	auto const pType = pThis->Type;
	auto const radius = pType->DamageRadius;
	auto const damage = pType->Damage;
	auto const pWarhead = pType->Warhead;

	if(radius >= 0 && damage && pWarhead) {
		auto const coords = pThis->Bounce.GetCoords();

		auto pObject = MapClass::Instance.GetCellAt(coords)->FirstObject;

		while(pObject) {
			auto const pNext = pObject->NextObject;

			auto const distance = std::abs(pObject->Location.Y - coords.Y)
				+ std::abs(pObject->Location.X - coords.X);

			if(distance <= radius) {
				auto value = damage;
				pObject->ReceiveDamage(&value, TacticalClass::AdjustForZ(distance),
					pWarhead, nullptr, false, false, nullptr);
			}

			pObject = pNext;
		}
	}

	return 0x74A934;
}

DEFINE_HOOK(0x4232CE, AnimClass_Draw_SetPalette, 0x6)
{
	GET(AnimTypeClass *, AnimType, EAX);

	auto pData = AnimTypeExt::ExtMap.Find(AnimType);

	if(pData->Palette.Convert) {
		R->ECX<ConvertClass *>(pData->Palette.GetConvert());
		return 0x4232D4;
	}

	return 0;
}

DEFINE_HOOK(0x468379, BulletClass_Draw_SetAnimPalette, 0x6)
{
	GET(BulletClass *, Bullet, ESI);
	auto pExt = BulletTypeExt::ExtMap.Find(Bullet->Type);

	if(ConvertClass* Convert = pExt->GetConvert()) {
		R->EBX<ConvertClass *>(Convert);
		return 0x4683D7;
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x42511B, AnimClass_Expired_ScorchFlamer, 0x7)
DEFINE_HOOK_AGAIN(0x4250C9, AnimClass_Expired_ScorchFlamer, 0x7)
DEFINE_HOOK(0x42513F, AnimClass_Expired_ScorchFlamer, 0x7)
{
	GET(AnimClass*, pThis, ESI);
	auto pType = pThis->Type;

	CoordStruct crd = pThis->GetCoords();

	auto SpawnAnim = [&crd](AnimTypeClass* pType, int dist) {
		if(!pType) {
			return static_cast<AnimClass*>(nullptr);
		}

		CoordStruct crdAnim = crd;
		if(dist > 0) {
			auto crdNear = MapClass::GetRandomCoordsNear(crd, dist, false);
			crdAnim = MapClass::PickInfantrySublocation(crdNear, true);
		}

		auto count = ScenarioClass::Instance->Random.RandomRanged(1, 2);
		return GameCreate<AnimClass>(pType, crdAnim, 0, count, 0x600u, 0, false);
	};

	if(pType->Flamer) {
		// always create at least one small fire
		SpawnAnim(RulesClass::Instance->SmallFire, 64);

		// 50% to create another small fire
		if(ScenarioClass::Instance->Random.RandomRanged(0, 99) < 50) {
			SpawnAnim(RulesClass::Instance->SmallFire, 160);
		}

		// 50% chance to create a large fire
		if(ScenarioClass::Instance->Random.RandomRanged(0, 99) < 50) {
			SpawnAnim(RulesClass::Instance->LargeFire, 112);
		}

	} else if(pType->Scorch) {
		// creates a SmallFire anim that is attached to the same object
		// this anim is attached to.
		if(pThis->GetHeight() < 10) {
			switch(pThis->GetCell()->LandType) {
			case LandType::Water:
			case LandType::Beach:
			case LandType::Ice:
			case LandType::Rock:
				break;
			default:
				if(auto pAnim = SpawnAnim(RulesClass::Instance->SmallFire, 0)) {
					if(pThis->OwnerObject) {
						pAnim->SetOwnerObject(pThis->OwnerObject);
					}
				}
			}
		}
	}

	return 0;
}
