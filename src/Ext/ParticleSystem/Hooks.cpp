#include "Body.h"

#include "../AnimType/Body.h"
#include "../Bullet/Body.h"
#include "../WeaponType/Body.h"

#include <AnimClass.h>
#include <EBolt.h>
#include <MapClass.h>
#include <ParticleTypeClass.h>
#include <RulesClass.h>
#include <ScenarioClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>
#include <YRMath.h>

#include <cmath>

namespace
{
	ParticleClass* SpawnParticle(
		ParticleSystemClass* pSystem, ParticleTypeClass* pType, CoordStruct* pCoords)
	{
		// 0x62E430 is the overload that takes an explicit type and a single
		// coord (the second one is fixed to COORD_NONE). 0x62E380 is the other
		// overload: it takes two coords and derives the type from the system,
		// and it is the one Ares hooks, so calling it would re-enter the hook.
		using func_t = ParticleClass* (__thiscall*)(
			ParticleSystemClass*, ParticleTypeClass*, CoordStruct*);
		return reinterpret_cast<func_t>(0x62E430)(pSystem, pType, pCoords);
	}
}

DEFINE_HOOK(0x62E2AD, ParticleSystemClass_Draw, 0x6)
{
	GET(ParticleSystemClass*, pThis, EDI);
	GET(ParticleSystemTypeClass*, pType, EAX);

	auto count = 0;

	if(pType->ParticleCap) {
		auto const pExt = ParticleSystemExt::ExtMap.Find(pThis);
		count = pThis->Particles.Count + static_cast<int>(pExt->MovementData.size());
	}

	R->ECX(count);

	return 0x62E2B3;
}

DEFINE_HOOK(0x62E380, ParticleSystemClass_SpawnParticle, 0xA)
{
	GET(ParticleSystemClass*, pThis, ECX);

	auto const pExt = ParticleSystemExt::ExtMap.Find(pThis);

	return pExt->Behave != ParticleSystemExt::BehaveKind::None ? 0x62E428 : 0;
}

DEFINE_HOOK(0x62FD60, ParticleSystemClass_Update, 0x9)
{
	GET(ParticleSystemClass*, pThis, ECX);

	auto const pExt = ParticleSystemExt::ExtMap.Find(pThis);

	return pExt->Handled() ? 0x62FE43 : 0;
}

DEFINE_HOOK(0x6D9427, TacticalClass_DrawUnits_ParticleSystems, 0x9)
{
	GET(Layer, layer, EAX);

	if(layer == Layer::Air) {
		ParticleSystemExt::UpdateInAir();
	}

	return layer == Layer::Ground ? 0x6D9430 : 0x6D95A1;
}

DEFINE_HOOK(0x425002, AnimClass_Expired_SpawnsParticle, 0x6)
{
	GET(AnimClass*, pThis, ESI);
	GET(AnimTypeClass*, pType, EAX);
	GET(int, count, ECX);

	auto const pParticleType = ParticleTypeClass::Array.Items[pType->SpawnsParticle];
	auto const pTypeExt = AnimTypeExt::ExtMap.Find(pType);

	auto const minimum = pTypeExt->SpawnsParticle_RangeMinimum.Get();
	auto const maximum = pTypeExt->SpawnsParticle_RangeMaximum.Get();

	if(!minimum && !maximum) {
		return 0;
	}

	auto const pScen = ScenarioClass::Instance;

	auto const center = pThis->GetCoords();
	auto const height = center.Z - MapClass::Instance.GetCellFloorHeight(center);

	auto const step = Math::TwoPi / count;
	auto angle = 0.0;

	for(auto index = count; index > 0; --index) {
		auto const distance = std::abs(pScen->Random.RandomRanged(minimum, maximum));
		auto const radians = pScen->Random.RandomDouble() * step + angle;

		auto const cosine = Math::cos(radians);
		auto const sine = Math::sin(radians);

		CoordStruct coords = {
			center.X + static_cast<int>(distance * cosine),
			center.Y - static_cast<int>(sine * distance),
			center.Z
		};
		coords.Z = MapClass::Instance.GetCellFloorHeight(coords) + height;

		SpawnParticle(*reinterpret_cast<ParticleSystemClass**>(0xA8ED78),
			pParticleType, &coords);

		angle += step;
	}

	return 0x42504D;
}

DEFINE_HOOK(0x4C2AFF, EBolt_Fire_Particles, 0x5)
{
	GET(EBolt*, pThis, ESI);

	auto const pData = WeaponTypeExt::BoltExt.get_or_default(pThis);
	if(!pData) {
		return 0;
	}

	auto const pSystemType = pData->Bolt_ParticleSystem.Get(
		RulesClass::Instance->DefaultSparkSystem);

	if(pSystemType) {
		GameCreate<ParticleSystemClass>(pSystemType, pThis->Point2, nullptr,
			pThis->Owner, CoordStruct::Empty, nullptr);
	}

	R->EAX(0);

	return 0x4C2B0C;
}

DEFINE_HOOK(0x5F4FE7, ObjectClass_Put, 0x8)
{
	GET(ObjectClass*, pThis, ESI);
	GET(ObjectTypeClass*, pType, EBX);

	if(pThis && pThis->WhatAmI() == AbstractType::Bullet) {
		auto const pExt = BulletExt::ExtMap.Find(static_cast<BulletClass*>(pThis));
		pExt->CreateAttachedParticleSys();
	}

	return pType ? 0x5F4FEF : 0x5F5210;
}

DEFINE_HOOK(0x70CBB0, TechnoClass_DealParticleDamage_AmbientDamage, 0x6)
{
	GET_BASE(WeaponTypeClass*, pWeapon, 0x14);

	if(!pWeapon->AmbientDamage) {
		return 0x70CC3E;
	}

	R->EDI(pWeapon);
	R->ESI(0);

	return R->EAX<int>() > 0 ? 0x70CBB9 : 0x70CBF7;
}

DEFINE_HOOK(0x7119D5, TechnoTypeClass_CTOR_NoInit_Particles, 0x6)
{
	GET(TechnoTypeClass*, pThis, ESI);

	auto const pBytes = reinterpret_cast<BYTE*>(pThis);
	*reinterpret_cast<DWORD*>(pBytes + 0x778) = 0x7F4F9Cu;
	*reinterpret_cast<DWORD*>(pBytes + 0x794) = 0x7F4F9Cu;

	return 0x711A00;
}
