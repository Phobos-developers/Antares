#include "Body.h"
#include "./../../Ares.h"
#include "./../../Ares.CRT.h"

#include "./../../Utilities/TemplateDef.h"
#include "./../../Utilities/INIParser.h"

#include <algorithm>

//Static init
CampaignExt::ExtContainer CampaignExt::ExtMap;

DynamicVectorClass<CampaignClass*>* const CampaignExt::Campaigns =
	reinterpret_cast<DynamicVectorClass<CampaignClass*>*>(0xA83CF8);
int CampaignExt::lastSelectedCampaign;

void CampaignExt::ExtData::Initialize(CCINIClass* pINI)
{
	auto pThis = this->OwnerObject();

	if(!_strcmpi(pThis->ID, "ALL1")) {
		this->HoverSound = "AlliedCampaignSelect";
	} else if(!_strcmpi(pThis->ID, "SOV1")) {
		this->HoverSound = "SovietCampaignSelect";
	} else if(!_strcmpi(pThis->ID, "TUT1")) {
		this->HoverSound = "BootCampSelect";
	}
};

void CampaignExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	const char* section = this->OwnerObject()->get_ID();

	INI_EX exINI(pINI);

	this->DebugOnly.Read(exINI, section, "DebugOnly");

	this->HoverSound.Read(pINI, section, "HoverSound", "");

	this->Summary.Read(exINI, section, "Summary");
}

int CampaignExt::CountVisible() {
	if(Ares::UISettings::ShowDebugCampaigns) {
		return Campaigns->Count;
	}

	return std::count_if(Campaigns->begin(), Campaigns->end(), [](CampaignClass* pItem) {
		return CampaignExt::ExtMap.Find(pItem)->IsVisible();
	});
}

// =============================
// container

CampaignExt::ExtContainer::ExtContainer() : Container("CampaignClass") {
}

CampaignExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK_AGAIN(0x46CF3D, CampaignClass_CTOR, 0x5)
DEFINE_HOOK(0x46CC03, CampaignClass_CTOR, 0x5)
{
	GET(CampaignClass*, pItem, ESI);

	CampaignExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x46D0B6, CampaignClass_DTOR, 0x6)
DEFINE_HOOK(0x46CC36, CampaignClass_DTOR, 0x6)
{
	GET(CampaignClass*, pItem, ESI);

	CampaignExt::ExtMap.Remove(pItem);
	return 0;
}

// read Ares properties
DEFINE_HOOK(0x46CD56, CampaignClass_LoadFromINI, 0x7)
{
	GET(CCINIClass*, pINI, EDI);
	GET(CampaignClass*, pThis, EBX);

	if(pThis) {
		CampaignExt::ExtMap.Find(pThis)->LoadFromINI(pINI);
	}
	return 0;
}

static_assert(sizeof(CampaignExt::ExtData) == 0x80, "CampaignExt::ExtData must match the 3.0p1 layout");

static_assert(offsetof(CampaignExt::ExtData, DebugOnly) == 0x08, "CampaignExt::ExtData layout slipped");
static_assert(offsetof(CampaignExt::ExtData, HoverSound) == 0x09, "CampaignExt::ExtData layout slipped");
static_assert(offsetof(CampaignExt::ExtData, Summary) == 0x2C, "CampaignExt::ExtData layout slipped");
