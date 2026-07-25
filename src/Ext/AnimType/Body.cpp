#include "Body.h"
#include "../House/Body.h"
#include "../../Ares.h"
#include "../../Utilities/TemplateDef.h"
#include <AnimClass.h>
#include <HouseTypeClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <WeaponTypeClass.h>

AnimTypeExt::ExtContainer AnimTypeExt::ExtMap;

void AnimTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	const char* pID = this->OwnerObject()->ID;

	INI_EX exINI(pINI);

	this->MakeInfantryOwner.Read(exINI, pID, "MakeInfantryOwner");

	this->Palette.LoadFromINI(pINI, pID, "CustomPalette");

	this->SpawnsParticle_RangeMinimum.Read(exINI, pID, "SpawnsParticle.RangeMinimum");
	this->SpawnsParticle_RangeMaximum.Read(exINI, pID, "SpawnsParticle.RangeMaximum");

	this->Weapon.Read(exINI, pID, "Weapon");
	this->Damage_Delay.Read(exINI, pID, "Damage.Delay");
}

OwnerHouseKind AnimTypeExt::SetMakeInfOwner(AnimClass *pAnim, HouseClass *pInvoker, HouseClass *pVictim, HouseClass *pKiller)
{
	auto pAnimData = AnimTypeExt::ExtMap.Find(pAnim->Type);

	auto newOwner = HouseExt::GetHouseKind(pAnimData->MakeInfantryOwner, true,
		nullptr, pInvoker, pKiller, pVictim);

	if(newOwner) {
		pAnim->Owner = newOwner;
		if(pAnim->Type->MakeInfantry > -1) {
			pAnim->LightConvert = ColorScheme::Array.Items[newOwner->ColorSchemeIndex]->LightConvert;
		}
	}

	return pAnimData->MakeInfantryOwner;
}

// =============================
// container

AnimTypeExt::ExtContainer::ExtContainer() : Container("AnimTypeClass") {
}

AnimTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// load / save

template <typename T>
void AnimTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->MakeInfantryOwner)
		.Process(this->Palette)
		.Process(this->SpawnsParticle_RangeMinimum)
		.Process(this->SpawnsParticle_RangeMaximum)
		.Process(this->Weapon)
		.Process(this->Damage_Delay);
}

void AnimTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<AnimTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void AnimTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<AnimTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container hooks

DEFINE_HOOK(0x42784B, AnimTypeClass_CTOR, 0x5)
{
	GET(AnimTypeClass*, pItem, EAX);

	AnimTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x428EA8, AnimTypeClass_SDDTOR, 0x5)
{
	GET(AnimTypeClass*, pItem, ECX);

	AnimTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x428970, AnimTypeClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x428800, AnimTypeClass_SaveLoad_Prefix, 0xA)
{
	GET_STACK(AnimTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	AnimTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK_AGAIN(0x42892C, AnimTypeClass_Load_Suffix, 0x6)
DEFINE_HOOK(0x428958, AnimTypeClass_Load_Suffix, 0x6)
{
	AnimTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x42898A, AnimTypeClass_Save_Suffix, 0x3)
{
	AnimTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x4287E9, AnimTypeClass_LoadFromINI, 0xA)
DEFINE_HOOK(0x4287DC, AnimTypeClass_LoadFromINI, 0xA)
{
	GET(AnimTypeClass*, pItem, ESI);
	GET_STACK(CCINIClass*, pINI, 0xBC);

	AnimTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

static_assert(sizeof(AnimTypeExt::ExtData) == 0x40, "AnimTypeExt::ExtData must match the 3.0p1 layout");

static_assert(offsetof(AnimTypeExt::ExtData, MakeInfantryOwner) == 0x08, "AnimTypeExt::ExtData layout slipped");
static_assert(offsetof(AnimTypeExt::ExtData, Palette) == 0x0C, "AnimTypeExt::ExtData layout slipped");
static_assert(offsetof(AnimTypeExt::ExtData, SpawnsParticle_RangeMinimum) == 0x18, "AnimTypeExt::ExtData layout slipped");
static_assert(offsetof(AnimTypeExt::ExtData, Weapon) == 0x20, "AnimTypeExt::ExtData layout slipped");
static_assert(offsetof(AnimTypeExt::ExtData, Damage_Delay) == 0x24, "AnimTypeExt::ExtData layout slipped");
