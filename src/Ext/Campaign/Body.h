#pragma once

#include "../../Ares.h"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Template.h"
#include "../_Container.hpp"

#include <CCINIClass.h>
#include <ColorScheme.h>
#include <VocClass.h>
#include <CampaignClass.h>

class CampaignExt
{
public:
	using base_type = CampaignClass;

	class ExtData final : public Extension<CampaignClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x22441133;

		Valueable<bool> DebugOnly;
		AresFixedString<0x20> HoverSound;
		Valueable<CSFText> Summary;

		ExtData(CampaignClass* OwnerObject) : Extension<CampaignClass, ExtData>(OwnerObject),
			DebugOnly(false)
		{ }

		~ExtData() = default;

		void LoadFromINIFile(CCINIClass* pINI);
		void Initialize(CCINIClass* pINI);
		void InvalidatePointer(void *ptr, bool bRemoved) { }

		bool IsVisible() const {
			return !this->DebugOnly || Ares::UISettings::ShowDebugCampaigns;
		}
	};

	class ExtContainer final : public Container<CampaignExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
	static DynamicVectorClass<CampaignClass*>* const Campaigns;

	static int lastSelectedCampaign;

	static int CountVisible();
};
