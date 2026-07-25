#include "Body.h"
#include "../../Utilities/TemplateDef.h"

#include <WarheadTypeClass.h>

//Static init
TiberiumExt::ExtContainer TiberiumExt::ExtMap;

void TiberiumExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	const char* section = this->OwnerObject()->get_ID();

	INI_EX exINI(pINI);

	this->Damage.Read(exINI, section, "Damage");
	this->Warhead.Read(exINI, section, "Warhead");

	this->Heal_Step.Read(exINI, section, "Heal.Step");
	this->Heal_IStep.Read(exINI, section, "Heal.IStep");
	this->Heal_UStep.Read(exINI, section, "Heal.UStep");
	this->Heal_Delay.Read(exINI, section, "Heal.Delay");

	this->ExplosionWarhead.Read(exINI, section, "ExplosionWarhead");
	this->ExplosionDamage.Read(exINI, section, "ExplosionDamage");

	this->DebrisChance.Read(exINI, section, "Debris.Chance");
}

double TiberiumExt::ExtData::GetHealDelay() const
{
	return this->Heal_Delay.Get(RulesClass::Instance->TiberiumHeal);
}

int TiberiumExt::ExtData::GetHealStep(TechnoClass* pTechno) const
{
	auto pType = pTechno->GetTechnoType();
	int step = pType->GetRepairStep();

	switch(pType->WhatAmI()) {
	case InfantryTypeClass::AbsID:
		step = this->Heal_IStep.Get(step);
		break;
	case UnitTypeClass::AbsID:
		step = this->Heal_UStep.Get(step);
		break;
	default:
		step = this->Heal_Step.Get(step);
		break;
	}

	return step;
}

int TiberiumExt::ExtData::GetDamage() const
{
	int damage = this->OwnerObject()->Power / 10;
	if(damage < 1) {
		damage = 1;
	}

	return this->Damage.Get(damage);
}

WarheadTypeClass* TiberiumExt::ExtData::GetWarhead() const
{
	return this->Warhead.Get(RulesClass::Instance->C4Warhead);
}

WarheadTypeClass* TiberiumExt::ExtData::GetExplosionWarhead() const
{
	return this->ExplosionWarhead.Get(RulesClass::Instance->C4Warhead);
}

int TiberiumExt::ExtData::GetExplosionDamage() const
{
	return this->ExplosionDamage.Get(RulesClass::Instance->TiberiumExplosionDamage);
}

int TiberiumExt::ExtData::GetDebrisChance() const
{
	return this->DebrisChance;
}

// =============================
// load / save

template <typename T>
void TiberiumExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->Damage)
		.Process(this->Warhead)
		.Process(this->Heal_Step)
		.Process(this->Heal_IStep)
		.Process(this->Heal_UStep)
		.Process(this->Heal_Delay)
		.Process(this->ExplosionWarhead)
		.Process(this->ExplosionDamage)
		.Process(this->DebrisChance);
}

void TiberiumExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<TiberiumClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TiberiumExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<TiberiumClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

TiberiumExt::ExtContainer::ExtContainer() : Container("TiberiumClass") {
}

TiberiumExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x721876, TiberiumClass_CTOR, 0x5)
{
	GET(TiberiumClass*, pThis, ESI);

	TiberiumExt::ExtMap.FindOrAllocate(pThis);
	return 0;
}

DEFINE_HOOK(0x72193A, TiberiumClass_DTOR, 0x6)
{
	GET(TiberiumClass*, pThis, ESI);

	TiberiumExt::ExtMap.Remove(pThis);
	return 0;
}

DEFINE_HOOK_AGAIN(0x7220D0, TiberiumClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x721E80, TiberiumClass_SaveLoad_Prefix, 0x7)
{
	GET_STACK(TiberiumClass*, pThis, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TiberiumExt::ExtMap.PrepareStream(pThis, pStm);

	return 0;
}

DEFINE_HOOK(0x72208C, TiberiumClass_Load_Suffix, 0x7)
{
	TiberiumExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x72212C, TiberiumClass_Save_Suffix, 0x5)
{
	TiberiumExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x721CDC, TiberiumClass_LoadFromINI, 0xA)
DEFINE_HOOK_AGAIN(0x721CE9, TiberiumClass_LoadFromINI, 0xA)
DEFINE_HOOK(0x721C7B, TiberiumClass_LoadFromINI, 0xA)
{
	GET(TiberiumClass*, pThis, ESI);
	GET(CCINIClass*, pINI, EBX);

	TiberiumExt::ExtMap.LoadFromINI(pThis, pINI);
	return 0;
}

static_assert(sizeof(TiberiumExt::ExtData) == 0x80, "TiberiumExt::ExtData must match the 3.0p1 layout");

static_assert(offsetof(TiberiumExt::ExtData, Damage) == 0x08, "TiberiumExt::ExtData layout slipped");
static_assert(offsetof(TiberiumExt::ExtData, Heal_Delay) == 0x30, "TiberiumExt::ExtData layout slipped");
static_assert(offsetof(TiberiumExt::ExtData, ExplosionWarhead) == 0x40, "TiberiumExt::ExtData layout slipped");
static_assert(offsetof(TiberiumExt::ExtData, DebrisChance) == 0x50, "TiberiumExt::ExtData layout slipped");
