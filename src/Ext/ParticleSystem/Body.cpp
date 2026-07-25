#include "Body.h"

#include "../../Utilities/TemplateDef.h"

#include <ParticleClass.h>
#include <ParticleTypeClass.h>

ParticleSystemExt::ExtContainer ParticleSystemExt::ExtMap;

void ParticleSystemExt::ExtData::InitializeConstants()
{
	auto const pSystemType = this->OwnerObject()->Type;
	if(!pSystemType) {
		return;
	}

	auto const index = pSystemType->HoldsWhat;
	if(index < 0) {
		return;
	}

	auto const pType = ParticleTypeClass::Array.Items[index];
	this->HeldParticleType = pType;

	if(pType->UseLineTrail || pType->AlphaImage) {
		return;
	}

	// ParticleTypeClass calls its first two behaviours Gas and Smoke, the system
	// type calls them Smoke and Gas. swap them before comparing the two.
	auto held = static_cast<int>(pType->BehavesLike);
	if(held <= 1) {
		held = (held == 0) ? 1 : 0;
	}

	if(static_cast<int>(pSystemType->BehavesLike) != held) {
		return;
	}

	switch(pSystemType->BehavesLike) {
	case BehavesLike::Smoke:
		this->Behave = BehaveKind::Smoke;
		break;
	case BehavesLike::Spark:
		this->Behave = BehaveKind::Spark;
		break;
	case BehavesLike::Railgun:
		this->Behave = BehaveKind::Railgun;
		break;
	default:
		break;
	}
}

// =============================
// container

ParticleSystemExt::ExtContainer::ExtContainer() : Container("ParticleSystemClass") {
}

ParticleSystemExt::ExtContainer::~ExtContainer() = default;

// =============================
// load / save

bool ParticleSystemExt::DrawDataItem::Load(AresStreamReader &Stm, bool RegisterForChange)
{
	if(!Stm.Load(*this)) {
		return false;
	}

	if(RegisterForChange) {
		Swizzle swizzle(this->LinkedParticleType);
	}

	return true;
}

bool ParticleSystemExt::DrawDataItem::Save(AresStreamWriter &Stm) const
{
	Stm.Save(*this);
	return true;
}

template <typename T>
void ParticleSystemExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->Behave)
		.Process(this->HeldParticleType)
		.Process(this->MovementData)
		.Process(this->DrawData);
}

void ParticleSystemExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<ParticleSystemClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void ParticleSystemExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<ParticleSystemClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container hooks

DEFINE_HOOK(0x62DF05, ParticleSystemClass_CTOR, 0x5)
{
	GET(ParticleSystemClass*, pItem, ESI);

	ParticleSystemExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x62E26B, ParticleSystemClass_DTOR, 0x6)
{
	GET(ParticleSystemClass*, pItem, EDI);

	ParticleSystemExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x630090, ParticleSystemClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x62FF20, ParticleSystemClass_SaveLoad_Prefix, 0x7)
{
	GET_STACK(ParticleSystemClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	ParticleSystemExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x630088, ParticleSystemClass_Load_Suffix, 0x5)
{
	ParticleSystemExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x6300F3, ParticleSystemClass_Save_Suffix, 0x6)
{
	ParticleSystemExt::ExtMap.SaveStatic();
	return 0;
}

// =============================
// pointer invalidation

DEFINE_HOOK(0x72590E, AnnounceInvalidPointer_Particle, 0x9)
{
	GET(AbstractType const, absID, EBX);

	if(absID == AbstractType::Particle) {
		GET(ParticleClass* const, pItem, ESI);

		if(auto const pSystem = pItem->ParticleSystem) {
			auto& particles = pSystem->Particles;
			for(auto i = 0; i < particles.Count; ++i) {
				if(particles.Items[i] == pItem) {
					particles.RemoveItem(i);
					break;
				}
			}
		}

		return 0x725C08;
	}

	return absID == AbstractType::ParticleSystem ? 0x725917 : 0x7259DA;
}

static_assert(sizeof(ParticleSystemExt::ExtData) == 0x40, "ParticleSystemExt::ExtData must match the 3.0p1 layout");

static_assert(offsetof(ParticleSystemExt::ExtData, Behave) == 0x08, "ParticleSystemExt::ExtData layout slipped");
static_assert(offsetof(ParticleSystemExt::ExtData, HeldParticleType) == 0x0C, "ParticleSystemExt::ExtData layout slipped");
static_assert(offsetof(ParticleSystemExt::ExtData, MovementData) == 0x10, "ParticleSystemExt::ExtData layout slipped");
static_assert(offsetof(ParticleSystemExt::ExtData, DrawData) == 0x1C, "ParticleSystemExt::ExtData layout slipped");

// both vectors stream their elements as raw 44 byte blobs, so the stride is part
// of the savegame format. the swizzle anchor is DrawDataItem::LinkedParticleType.
static_assert(sizeof(ParticleSystemExt::MovementDataItem) == 0x2C, "ParticleSystemExt::MovementDataItem must match the 3.0p1 stride");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, Velocity) == 0x0C, "ParticleSystemExt::MovementDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, Speed) == 0x18, "ParticleSystemExt::MovementDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, ColorFactor) == 0x1C, "ParticleSystemExt::MovementDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, ColorIndex) == 0x20, "ParticleSystemExt::MovementDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, Duration) == 0x24, "ParticleSystemExt::MovementDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, Expired) == 0x28, "ParticleSystemExt::MovementDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::MovementDataItem, Color) == 0x29, "ParticleSystemExt::MovementDataItem layout slipped");

static_assert(sizeof(ParticleSystemExt::DrawDataItem) == 0x2C, "ParticleSystemExt::DrawDataItem must match the 3.0p1 stride");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, VelocityX) == 0x0C, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, VelocityZ) == 0x14, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, Order) == 0x18, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, ImageFrame) == 0x1C, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, Duration) == 0x20, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, LinkedParticleType) == 0x24, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, Translucency) == 0x28, "ParticleSystemExt::DrawDataItem layout slipped");
static_assert(offsetof(ParticleSystemExt::DrawDataItem, Expired) == 0x29, "ParticleSystemExt::DrawDataItem layout slipped");
