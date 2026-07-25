#include "Body.h"
#include <Utilities/Macro.h>   // STACK_OFFS

#include <FactoryClass.h>
#include <SuperClass.h>
#include "../../Misc/Networking.h"
#include <BuildingTypeClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>

DynamicVectorClass<BuildType> RulesExt::TabCameos[4];

void RulesExt::ClearCameos() {
	for(auto& cameos : RulesExt::TabCameos) {
		cameos.Clear();
		cameos.CapacityIncrement = 100;
		cameos.Reserve(100);
	}
}

// returns whether this cameo is gone and has to be dropped from the strip.
// abandons whatever the player had queued up for it on the way out.
static bool CameoIsGone(BuildType const& cameo)
{
	auto const pType = ObjectTypeClass::GetTechnoType(cameo.ItemType, cameo.ItemIndex);

	auto isGone = true;

	if(pType) {
		auto const pFactory = pType->FindFactory(true, false, false, HouseClass::CurrentPlayer);
		if(pFactory) {
			isGone = pFactory->Owner->CanBuild(pType, false, true) == CanBuildResult::Unbuildable;
		}
	} else if(HouseClass::CurrentPlayer->Supers.ValidIndex(cameo.ItemIndex)) {
		isGone = !HouseClass::CurrentPlayer->Supers[cameo.ItemIndex]->IsPresent;
	}

	if(!isGone) {
		return false;
	}

	if(cameo.CurrentFactory) {
		EventClass Event(
			HouseClass::CurrentPlayer->ArrayIndex, EventType::Abandon,
			static_cast<int>(cameo.ItemType), cameo.ItemIndex, pType ? pType->Naval : 0);
		Networking::AddEvent(&Event);
	}

	if(cameo.ItemType == BuildingClass::AbsID || cameo.ItemType == BuildingTypeClass::AbsID) {
		MouseClass::Instance.CurrentBuilding = nullptr;
		MouseClass::Instance.CurrentBuildingType = nullptr;
		MouseClass::Instance.CurrentBuildingOwnerArrayIndex = 0xFFFFFFFF;
		MouseClass::Instance.SetActiveFoundation(nullptr);
	}

	if(pType) {
		auto const factoryOf = pType->WhatAmI();

		if(HouseClass::CurrentPlayer->GetPrimaryFactory(factoryOf, pType->Naval, BuildCat::DontCare)) {
			EventClass Event(
				HouseClass::CurrentPlayer->ArrayIndex, EventType::AbandonAll,
				static_cast<int>(cameo.ItemType), cameo.ItemIndex, pType->Naval);
			Networking::AddEvent(&Event);
		}
	}

	return true;
}

// initializing sidebar
DEFINE_HOOK(0x6A4EA5, SidebarClass_CTOR_InitCameosList, 0x6)
{
	RulesExt::ClearCameos();

	return 0;
}

// zeroing in preparation for load
DEFINE_HOOK(0x6A4FD8, SidebarClass_Load_InitCameosList, 0x6)
{
	RulesExt::ClearCameos();

	return 0;
}

// set factory for cameo
DEFINE_HOOK(0x6A61B1, SidebarClass_SetFactoryForObject, 0x0)
{
	enum { Found = 0x6A6210 , NotFound = 0x6A61E6 };

	GET(int, TabIndex, EAX);
	GET(AbstractType, ItemType, EDI);
	GET(int, ItemIndex, EBP);
	GET_STACK(FactoryClass *, Factory, STACK_OFFS(0xC, -0x4));

	for(auto& cameo : RulesExt::TabCameos[TabIndex]) {
		if(cameo.ItemIndex == ItemIndex && cameo.ItemType == ItemType) {
			cameo.CurrentFactory = Factory;
			auto &Tab = MouseClass::Instance.Tabs[TabIndex];
			Tab.NeedsRedraw = true;
			Tab.IsBuilding = true;
			MouseClass::Instance.RedrawSidebar(0);
			return Found;
		}
	}

	return NotFound;
}

// don't check for 75 cameos in active tab
DEFINE_HOOK(0x6A63B7, SidebarClass_AddCameo_SkipSizeCheck, 0x0)
{
	enum { AlreadyExists = 0x6A65FF, NewlyAdded = 0x6A63FD };

	GET_STACK(int, TabIndex, 0x18);
	GET(AbstractType, ItemType, ESI);
	GET(int, ItemIndex, EBP);

	for(auto const& cameo : RulesExt::TabCameos[TabIndex]) {
		if(cameo.ItemIndex == ItemIndex && cameo.ItemType == ItemType) {
			return AlreadyExists;
		}
	}

	R->EDI<StripClass *>(&MouseClass::Instance.Tabs[TabIndex]);

	return NewlyAdded;
}

DEFINE_HOOK(0x6A8710, StripClass_AddCameo_ReplaceItAll, 0x0)
{
	GET(StripClass *, pStrip, ECX);
	GET_STACK(AbstractType, ItemType, 0x4);
	GET_STACK(int, ItemIndex, 0x8);

	auto &cameos = RulesExt::TabCameos[pStrip->Index];

	BuildType newCameo(ItemIndex, ItemType);
	if(ItemType == BuildingTypeClass::AbsID) {
		newCameo.IsAlt = static_cast<BYTE>(ObjectTypeClass::GetBuildCat(ItemType, ItemIndex));
	}

	if(cameos.AddItem(newCameo)) {
		auto const old_end = cameos.end() - 1;
		auto const it = std::lower_bound(cameos.begin(), old_end, newCameo);
		std::copy_backward(it, old_end, cameos.end());
		*it = newCameo;
	}

	++pStrip->CameoCount;

	return 0x6A87E7;
}

// pointer #1
DEFINE_HOOK(0x6A8D1C, StripClass_MouseMove_GetCameos1, 0x0)
{
	GET(int, CameoCount, EAX);
	GET(StripClass *, pStrip, EBX);

	if(CameoCount < 1) {
		return 0x6A8D8B;
	}

	R->EDI<BuildType *>(RulesExt::TabCameos[pStrip->Index].Items);

	return 0x6A8D23;
}

// pointer #2
DEFINE_HOOK(0x6A8DB5, StripClass_MouseMove_GetCameos2, 0x0)
{
	GET(int, CameoCount, EAX);
	GET(StripClass *, pStrip, EBX);

	if(CameoCount < 1) {
		return 0x6A8F64;
	}

	auto ptr = reinterpret_cast<byte *>(RulesExt::TabCameos[pStrip->Index].Items);
	ptr += 0x10;
	R->EBP<byte *>(ptr);

	return 0x6A8DC0;
}

// pointer #3
DEFINE_HOOK(0x6A8F6C, StripClass_MouseMove_GetCameos3, 0x0)
{
	GET(StripClass *, pStrip, ESI);
	GET_STACK(int, unused, 0x20);

	if(pStrip->CameoCount < 1) {
		return 0x6A902D;
	}

	auto ptr = reinterpret_cast<byte *>(RulesExt::TabCameos[pStrip->Index].Items);
	ptr += 0x1C;
	R->ESI<byte *>(ptr);
	R->EBP<int>(unused);

	return 0x6A8F7C;
}

// don't check for <= 75, pointer
DEFINE_HOOK(0x6A9304, StripClass_GetTip_NoLimit, 0x0)
{
	GET(int, CameoIndex, EAX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	auto ptr = reinterpret_cast<byte *>(&cameos.Items[CameoIndex]);
	ptr -= 0x58;
	R->EAX<byte *>(ptr);

	return 0x6A9316;
}

DEFINE_HOOK(0x6A95C8, StripClass_Draw_Status, 0x0)
{
	GET(int, CameoIndex, EAX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	R->EDX<DWORD *>(&cameos.Items[CameoIndex].unknown_10);

	return 0x6A95D3;
}

DEFINE_HOOK(0x6A9747, StripClass_Draw_GetCameo1, 0x0)
{
	GET(int, CameoIndex, ECX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];
	auto &Item = cameos.Items[CameoIndex];

	auto ptr = reinterpret_cast<byte *>(&Item);
	ptr -= 0x58;
	R->EAX<byte *>(ptr);
	R->Stack<byte *>(0x30, ptr);

	R->ECX(Item.ItemType);

	return (Item.ItemType == AbstractType::Special)
		? 0x6A9936
		: 0x6A9761
	;
}

DEFINE_HOOK(0x6A9866, StripClass_Draw_Status_1, 0x0)
{
	GET(int, CameoIndex, ECX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	return (cameos.Items[CameoIndex].unknown_10 == 1)
		? 0x6A9874
		: 0x6A98CF
	;
}

DEFINE_HOOK(0x6A9886, StripClass_Draw_Status_2, 0x0)
{
	GET(int, CameoIndex, EAX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];
	auto &Item = cameos.Items[CameoIndex];

	R->EDI<DWORD *>(&Item.unknown_10);
	R->EAX<DWORD>(Item.unknown_10);

	return 0x6A9893;
}

DEFINE_HOOK(0x6A99BE, StripClass_Draw_BreakDrawLoop, 0x5)
{
	R->Stack8(0x12, 0);
	return 0x6AA01C;
}

DEFINE_HOOK(0x6A9B4F, StripClass_Draw_TestFlashFrame, 0x0)
{
	GET(int, CameoIndex, EAX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	R->EAX(Unsorted::CurrentFrame);

	return (cameos.Items[CameoIndex].FlashEndFrame > Unsorted::CurrentFrame)
		? 0x6A9B67
		: 0x6A9BC5
	;
}

DEFINE_HOOK(0x6A9EBA, StripClass_Draw_Status_3, 0x0)
{
	GET(int, CameoIndex, EAX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	return (cameos.Items[CameoIndex].unknown_10 == 2)
		? 0x6A9ECC
		: 0x6AA01C
	;
}

// complete replacement of StripClass::Recalc
DEFINE_HOOK(0x6AA600, StripClass_RecheckCameos, 0x0)
{
	GET(StripClass *, pStrip, ECX);

	if(Unsorted::ArmageddonMode || pStrip->CameoCount <= 0) {
		R->EAX(0);
		return 0x6AACAE;
	}

	auto &cameos = RulesExt::TabCameos[pStrip->Index];

	// whatever sits in the upper left corner right now anchors the scrolling
	auto const anchor = cameos.Items[2 * pStrip->TopRowIndex];

	auto const kept = std::remove_if(cameos.begin(), cameos.end(), CameoIsGone);
	auto const count = static_cast<int>(kept - cameos.begin());
	cameos.Count = count;

	if(count >= pStrip->CameoCount) {
		R->EAX(0);
		return 0x6AACAE;
	}

	pStrip->CameoCount = count;

	if(count > 0) {
		MouseClass::Instance.UpdateScrollButtons();
	} else {
		auto const pTabButton = reinterpret_cast<void *>(0xB07C48 + 0x60 * pStrip->Index);
		(*reinterpret_cast<void(__thiscall ***)(void *)>(pTabButton))[0x3C / 4](pTabButton);

		auto idxFilled = 0;
		while(MouseClass::Instance.Tabs[idxFilled].CameoCount <= 0) {
			if(++idxFilled >= 4) {
				break;
			}
		}

		if(idxFilled >= 4) {
			MouseClass::Instance.UpdateScrollButtons();

			if(auto const pShape = *reinterpret_cast<SHPStruct **>(0xB0B478)) {
				MouseClass::Instance.unknown_5398 = 0xFFFFFFFF;
				MouseClass::Instance.unknown_5394 = pShape->Frames;
			}
		} else if(pStrip->Index == MouseClass::Instance.ActiveTabIndex) {
			MouseClass::Instance.SetTab(idxFilled);
		}
	}

	// keep the anchor cameo in view
	auto const it = std::lower_bound(cameos.begin(), cameos.end(), anchor);
	auto const row = static_cast<int>(it - cameos.begin()) / 2;

	auto excess = pStrip->CameoCount - MouseClass::Instance.GetVisibleCameoCount();
	if(excess < 0) {
		excess = 0;
	}

	auto top = excess / 2;
	if(top >= row) {
		top = row;
	}
	pStrip->TopRowIndex = top;

	MouseClass::Instance.SidebarBackgroundNeedsRedraw = true;

	R->EAX(1);
	return 0x6AACAE;
}

DEFINE_HOOK(0x6AAD2F, SelectClass_ProcessInput_LoadCameoData1, 0x0)
{
	GET(int, CameoIndex, ESI);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];
	if(CameoIndex >= cameos.Count) {
		return 0x6AB94F;
	}

	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);

	R->Stack<int>(STACK_OFFS(0xAC, 0x80), CameoIndex);

	auto &Item = cameos.Items[CameoIndex];
	R->Stack<int>(STACK_OFFS(0xAC, 0x98), Item.ItemIndex);
	R->Stack<FactoryClass *>(STACK_OFFS(0xAC, 0x94), Item.CurrentFactory);
	R->Stack<int>(STACK_OFFS(0xAC, 0x88), Item.IsAlt);
	R->EBP(Item.ItemType);

	auto ptr = reinterpret_cast<byte *>(&Item);
	ptr -= 0x58;
	R->EBX<byte *>(ptr);

	return 0x6AAD66;
}

DEFINE_HOOK(0x6AB0B0, SelectClass_ProcessInput_LoadCameo2, 0x0)
{
	GET(int, CameoIndex, ESI);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];
	auto &Item = cameos.Items[CameoIndex];

	R->EAX<DWORD *>(&Item.unknown_10);

	return 0x6AB0BE;
}

DEFINE_HOOK(0x6AB49D, SelectClass_ProcessInput_FixOffset1, 0x0)
{
	R->EDI<void *>(nullptr);
	R->ECX<void *>(nullptr);

	return 0x6AB4A4;
}

DEFINE_HOOK(0x6AB4E8, SelectClass_ProcessInput_FixOffset2, 0x0)
{
	GET_STACK(int, idx, 0x14);
	R->ECX<int>(idx);

	R->EDX<void *>(nullptr);

	return 0x6AB4EF;
}

DEFINE_HOOK(0x6AB577, SelectClass_ProcessInput_FixOffset3, 0x0)
{
	GET(int, CameoIndex, ESI);
	GET_STACK(FactoryClass *, SavedFactory, 0x18);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	auto &Item = cameos.Items[CameoIndex];
	Item.unknown_10 = 1;

	auto Progress = (Item.CurrentFactory)
		? Item.CurrentFactory->GetProgress()
		: 0
	;

	R->EAX<int>(Progress);
	R->EBP<void *>(nullptr);

	if(Item.unknown_10 == 1) { // hi, welcome to dumb ideas
		if(Item.Progress.Value > Progress) {
			Progress = (Progress + Item.Progress.Value) / 2;
		}
	}
	Item.Progress.Value = Progress;

	int Frames = SavedFactory->GetBuildTimeFrames();

	R->EAX<int>(Frames);
	R->ECX<void *>(nullptr);

	return 0x6AB5C6;
}

DEFINE_HOOK(0x6AB620, SelectClass_ProcessInput_FixOffset4, 0x0)
{
	R->ECX<void *>(nullptr);

	return 0x6AB627;
}

DEFINE_HOOK(0x6AB741, SelectClass_ProcessInput_FixOffset5, 0x0)
{
	R->EDX<void *>(nullptr);

	return 0x6AB748;
}

DEFINE_HOOK(0x6AB802, SelectClass_ProcessInput_FixOffset6, 0x0)
{
	GET(int, CameoIndex, EAX);

	auto &cameos = RulesExt::TabCameos[MouseClass::Instance.ActiveTabIndex];

	cameos.Items[CameoIndex].unknown_10 = 1;

	return 0x6AB814;
}

DEFINE_HOOK(0x6AB825, SelectClass_ProcessInput_FixOffset7, 0x0)
{
	R->ECX<int>(R->EBP<int>());
	R->EDX<void *>(nullptr);

	return 0x6AB82A;
}

DEFINE_HOOK(0x6AB920, SelectClass_ProcessInput_FixOffset8, 0x0)
{
	R->ECX<void *>(nullptr);

	return 0x6AB927;
}

DEFINE_HOOK(0x6AB92F, SelectClass_ProcessInput_FixOffset9, 0x0)
{
	R->EBX<byte *>(R->EBX<byte *>() + 0x6C);

	return 0x6AB936;
}

DEFINE_HOOK(0x6ABBCB, StripClass_AbandonCameosFromFactory_GetPointer1, 0x0)
{
	GET(int, CameoCount, EAX);
	GET(StripClass *, pStrip, ESI);

	if(CameoCount < 1) {
		return 0x6ABC2F;
	}

	auto ptr = reinterpret_cast<byte *>(RulesExt::TabCameos[pStrip->Index].Items);
	ptr += 0xC;
	R->ESI<byte *>(ptr);

	return 0x6ABBD2;
}

// don't limit to 75
DEFINE_HOOK(0x6AC6D9, SidebarClass_FlashCameo, 0x0)
{
	GET(unsigned int, TabIndex, EAX);
	GET(int, ItemIndex, ESI);
	GET_STACK(int, Duration, 0x10);

	auto &cameos = RulesExt::TabCameos[TabIndex];
	for(auto i = 0; i < cameos.Count; ++i) {
		auto &cameo = cameos[i];
		if(cameo.ItemIndex == ItemIndex) {
			cameo.FlashEndFrame = Unsorted::CurrentFrame + Duration;
			break;
		}
	}

	return 0x6AC71A;
}
