#include "Body.h"
#include "../TechnoType/Body.h"
#include "../AnimType/Body.h"
#include "../House/Body.h"
#include "../WeaponType/Body.h"
#include "../../Utilities/TemplateDef.h"

#include <BulletClass.h>
#include <ParticleSystemTypeClass.h>

BulletTypeExt::ExtContainer BulletTypeExt::ExtMap;

// =============================
// member funcs

void BulletTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	auto pThis = this->OwnerObject();

	INI_EX exINI(pINI);

	this->SubjectToBuildings.Read(exINI, pThis->ID, "SubjectToBuildings");
	this->SolidLevel.Read(exINI, pThis->ID, "SolidLevel");
	this->Parachuted.Read(exINI, pThis->ID, "Parachuted");

	this->SubjectToTrenches.Read(exINI, pThis->ID, "SubjectToTrenches");

	this->ImageConvert.clear();

	this->AirburstSpread.Read(exINI, pThis->ID, "AirburstSpread");
	this->RetargetAccuracy.Read(exINI, pThis->ID, "RetargetAccuracy");
	this->Splits.Read(exINI, pThis->ID, "Splits");
	this->RetargetSelf.Read(exINI, pThis->ID, "RetargetSelf");
	this->AroundTarget.Read(exINI, pThis->ID, "AroundTarget");

	this->BallisticScatterMin.Read(exINI, pThis->ID, "BallisticScatter.Min");
	this->BallisticScatterMax.Read(exINI, pThis->ID, "BallisticScatter.Max");

	this->AnimLength.Read(exINI, pThis->ID, "AnimLength");

	this->AttachedSystem.Read(exINI, pThis->ID, "AttachedSystem");
}

// get the custom palette of the animation this bullet type uses
ConvertClass* BulletTypeExt::ExtData::GetConvert()
{
	// cache the palette's convert
	if(this->ImageConvert.empty()) {
		ConvertClass* pConvert = nullptr;
		if(auto pAnimType = AnimTypeClass::Find(this->OwnerObject()->ImageFile)) {
			auto pData = AnimTypeExt::ExtMap.Find(pAnimType);
			pConvert = pData->Palette.GetConvert();
		}
		this->ImageConvert = pConvert;
	}

	return this->ImageConvert;
}

bool BulletTypeExt::ExtData::HasSplitBehavior()
{
	// behavior in FS: Splits defaults to Airburst.
	return this->OwnerObject()->Airburst || this->Splits;
}

BulletClass* BulletTypeExt::ExtData::CreateBullet(AbstractClass* pTarget, TechnoClass* pOwner, WeaponTypeClass* pWeapon) const
{
	auto pExt = WeaponTypeExt::ExtMap.Find(pWeapon);
	return this->CreateBullet(pTarget, pOwner, pWeapon->Damage, pWeapon->Warhead,
		pWeapon->Speed, pExt->GetProjectileRange(), pWeapon->Bright);
}

BulletClass* BulletTypeExt::ExtData::CreateBullet(AbstractClass* pTarget, TechnoClass* pOwner,
	int damage, WarheadTypeClass* pWarhead, int speed, int range, bool bright) const
{
	auto pBullet = this->OwnerObject()->CreateBullet(pTarget, pOwner, damage, pWarhead, speed, bright);

	if(pBullet) {
		pBullet->Range = range;
	}

	return pBullet;
}

// =============================
// load / save

template <typename T>
void BulletTypeExt::ExtData::Serialize(T& Stm) {
	Stm
		.Process(this->SubjectToBuildings)
		.Process(this->SolidLevel)
		.Process(this->Parachuted)
		.Process(this->SubjectToTrenches)
		.Process(this->Splits)
		.Process(this->RetargetSelf)
		.Process(this->RetargetAccuracy)
		.Process(this->AirburstSpread)
		.Process(this->AroundTarget)
		.Process(this->BallisticScatterMin)
		.Process(this->BallisticScatterMax)
		.Process(this->AnimLength)
		.Process(this->AttachedSystem);
}

void BulletTypeExt::ExtData::LoadFromStream(AresStreamReader &Stm) {
	Extension<BulletTypeClass, ExtData>::LoadFromStream(Stm);
	this->Serialize(Stm);

	// the palette convert is not part of the stream, it is resolved again on demand
	this->ImageConvert.clear();
}

void BulletTypeExt::ExtData::SaveToStream(AresStreamWriter &Stm) {
	Extension<BulletTypeClass, ExtData>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

BulletTypeExt::ExtContainer::ExtContainer() : Container("BulletTypeClass") {
}

BulletTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x46BDD9, BulletTypeClass_CTOR, 0x5)
{
	GET(BulletTypeClass*, pItem, EAX);

	BulletTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x46C8B6, BulletTypeClass_SDDTOR, 0x6)
{
	GET(BulletTypeClass*, pItem, ESI);

	BulletTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x46C730, BulletTypeClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x46C6A0, BulletTypeClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(BulletTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BulletTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x46C722, BulletTypeClass_Load_Suffix, 0x4)
{
	BulletTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x46C74A, BulletTypeClass_Save_Suffix, 0x3)
{
	BulletTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x46C429, BulletTypeClass_LoadFromINI, 0xA)
DEFINE_HOOK(0x46C41C, BulletTypeClass_LoadFromINI, 0xA)
{
	GET(BulletTypeClass*, pItem, ESI);
	GET_STACK(CCINIClass*, pINI, 0x90);

	BulletTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}

static_assert(sizeof(BulletTypeExt::ExtData) == 0x80, "BulletTypeExt::ExtData must match the 3.0p1 layout");

// anchors: sizeof alone cannot catch a layout slip, because the 64 byte alignment
// rounds it up. these pin the start, the middle and the end of the block.
static_assert(offsetof(BulletTypeExt::ExtData, SubjectToBuildings) == 0x08, "BulletTypeExt::ExtData layout slipped");
static_assert(offsetof(BulletTypeExt::ExtData, ImageConvert) == 0x14, "BulletTypeExt::ExtData layout slipped");
static_assert(offsetof(BulletTypeExt::ExtData, RetargetSelf) == 0x1D, "BulletTypeExt::ExtData layout slipped");
static_assert(offsetof(BulletTypeExt::ExtData, AroundTarget) == 0x30, "BulletTypeExt::ExtData layout slipped");
static_assert(offsetof(BulletTypeExt::ExtData, AttachedSystem) == 0x48, "BulletTypeExt::ExtData layout slipped");
