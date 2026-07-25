#include "Body.h"

#include "../../Utilities/TemplateDef.h"

ParticleTypeExt::ExtContainer ParticleTypeExt::ExtMap;

void ParticleTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	const char* pID = this->OwnerObject()->ID;

	INI_EX exINI(pINI);

	this->Palette.LoadFromINI(pINI, pID, "Palette");

	this->DamageRange.Read(exINI, pID, "DamageRange");
}

// =============================
// container

ParticleTypeExt::ExtContainer::ExtContainer() : Container("ParticleTypeClass") {
}

ParticleTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// load / save

template <typename T>
void ParticleTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->Palette)
		.Process(this->DamageRange);
}

void ParticleTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<ParticleTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void ParticleTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<ParticleTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container hooks

DEFINE_HOOK(0x644DBB, ParticleTypeClass_CTOR, 0x5)
{
	GET(ParticleTypeClass*, pItem, ESI);

	ParticleTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x645A3B, ParticleTypeClass_SDDTOR, 0x7)
{
	GET(ParticleTypeClass*, pItem, ESI);

	ParticleTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x6457A0, ParticleTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x645660, ParticleTypeClass_SaveLoad_Prefix, 0x7)
{
	GET_STACK(ParticleTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	ParticleTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x64578C, ParticleTypeClass_Load_Suffix, 0x5)
{
	ParticleTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x64580A, ParticleTypeClass_Save_Suffix, 0x7)
{
	ParticleTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK(0x645405, ParticleTypeClass_LoadFromINI, 0x5)
{
	GET(ParticleTypeClass*, pItem, ESI);
	GET_STACK(CCINIClass*, pINI, 0xE0);

	ParticleTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

static_assert(sizeof(ParticleTypeExt::ExtData) == 0x40, "ParticleTypeExt::ExtData must match the 3.0p1 layout");

static_assert(offsetof(ParticleTypeExt::ExtData, Palette) == 0x08, "ParticleTypeExt::ExtData layout slipped");
static_assert(offsetof(ParticleTypeExt::ExtData, DamageRange) == 0x18, "ParticleTypeExt::ExtData layout slipped");
