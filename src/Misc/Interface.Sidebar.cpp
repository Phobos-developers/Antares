#include "../Ext/House/Body.h"

#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <FactoryClass.h>
#include <HouseClass.h>
#include <MouseClass.h>
#include "Networking.h"
#include <SidebarClass.h>

struct ToolTip
{
	int             GadgetID;
	RectangleStruct Bounds;
	wchar_t const*  Text;
	bool            NoDelay;
	PROTECTED_PROPERTY(BYTE, padding_19[3]);
};

class ToolTipManager
{
public:
	static ToolTipManager* Global()
		{ return *reinterpret_cast<ToolTipManager**>(0x887368); }

	bool Add(ToolTip const* pToolTip)
		{ JMP_THIS(0x724580); }

	void Remove(int gadgetID)
		{ JMP_THIS(0x724730); }
};

static void __fastcall ClearSidebarTabObject(BuildingClass* pBuilding)
	{ JMP_STD(0x734270); }

// ObjectClass::Who_Can_Build_Me, vtable offset 0x190
static BuildingClass* WhoCanBuildMe(ObjectClass* pThis, bool inTheory, bool legal)
{
	using func_t = BuildingClass* (__thiscall*)(ObjectClass*, bool, bool);
	auto const vtable = *reinterpret_cast<func_t const**>(pThis);
	return vtable[0x190 / 4](pThis, inTheory, legal);
}

// ==========================================================================
// the cameo buttons. all four strips share the single button array the game
// reserved for the first one, and the buttons get rebound whenever a strip
// takes over the sidebar.

DEFINE_HOOK(0x6A8220, StripClass_Initialize, 0x7)
{
	GET(StripClass* const, pThis, ECX);
	GET_STACK(int const, index, 0x4);

	pThis->Index = index;

	auto const count = MouseClass::Instance.GetVisibleCameoCount();
	auto const baseX = pThis->Location.X;
	auto const baseY = pThis->Location.Y + 1;

	for(auto i = 0; i < count; ++i) {
		auto& button = SelectClass::Array()[i];

		button.Index = i;
		button.ID = SelectClass::ButtonID;
		button.Strip = pThis;
		button.X = baseX + SelectClass::CameoPitchX() * (i & 1);
		button.Y = baseY + SelectClass::CameoPitchY() * (i & ~1);
		button.Width = SelectClass::CameoWidth;
		button.Height = SelectClass::CameoHeight;
	}

	return 0x6A8329;
}

DEFINE_HOOK(0x6A8330, StripClass_EnableInput, 0x5)
{
	GET(StripClass* const, pThis, ECX);

	auto const count = MouseClass::Instance.GetVisibleCameoCount();

	for(auto i = 0; i < count; ++i) {
		auto const pButton = &SelectClass::Array()[i];

		pButton->Zap();
		pButton->Strip = pThis;
		MouseClass::Instance.AddButton(reinterpret_cast<GadgetClass*>(pButton));
	}

	MouseClass::Instance.SidebarBackgroundNeedsRedraw = true;

	return 0x6A83DA;
}

DEFINE_HOOK(0x6A83E0, StripClass_DisableInput, 0x6)
{
	for(auto i = 0; i < 4 * 60; ++i) {
		MouseClass::Instance.RemoveButton(reinterpret_cast<GadgetClass*>(&SelectClass::Array()[i]));
	}

	return 0x6A8415;
}

DEFINE_HOOK(0x6A93F0, StripClass_Activate, 0x6)
{
	GET(StripClass* const, pThis, ECX);

	pThis->AllowedToDraw = true;
	pThis->Activate();

	return 0x6A94A0;
}

DEFINE_HOOK(0x6A94B0, StripClass_Deactivate, 0x6)
{
	GET(StripClass* const, pThis, ECX);

	pThis->AllowedToDraw = false;
	pThis->Deactivate();

	return 0x6A94E9;
}

DEFINE_HOOK(0x6A96D9, StripClass_Draw_Strip, 0x7)
{
	GET(StripClass* const, pThis, EDI);
	GET(int const, row, ECX);
	GET(int const, column, EDX);

	R->EAX<SelectClass*>(&SelectClass::Array()[column + 2 * row]);

	return pThis->IsScrolling
		? 0x6A9703
		: 0x6A9714
	;
}

DEFINE_HOOK(0x6ABF44, sub_6ABD30_Strip1, 0x5)
{
	R->ESI<SelectClass*>(SelectClass::Array());

	return 0x6ABF49;
}

DEFINE_HOOK(0x6ABFB2, sub_6ABD30_Strip2, 0x6)
{
	auto const pNext = R->ESI<byte*>() + 4 * 60 * 0x38;

	R->ESI<byte*>(pNext);
	R->Stack<byte*>(0x10, pNext);

	return (pNext < reinterpret_cast<byte*>(0xB0B300))
		? 0x6ABF66
		: 0x6ABFC4
	;
}

DEFINE_HOOK(0x6AC02F, sub_6ABD30_Strip3, 0x8)
{
	GET_STACK(int const, count, 0x14);

	for(auto i = 0; i < 4 * 60; ++i) {
		ToolTipManager::Global()->Remove(i + 1000);
	}

	for(auto i = 0; i < count; ++i) {
		auto const& button = SelectClass::Array()[i];

		ToolTip tooltip;
		tooltip.Text = nullptr;
		tooltip.GadgetID = i + 1000;
		tooltip.Bounds.X = button.X;
		tooltip.Bounds.Y = button.Y;
		tooltip.Bounds.Width = button.Width;
		tooltip.Bounds.Height = button.Height;
		tooltip.NoDelay = true;

		ToolTipManager::Global()->Add(&tooltip);
	}

	return 0x6AC0A7;
}

// ==========================================================================
// switching strips. the inlined copies all become calls, because the button
// array has to be rebound and not just handed over.

DEFINE_HOOK(0x6A64C9, SidebarClass_AddCameo_Strip, 0x6)
{
	GET(SidebarClass* const, pThis, EBX);
	GET(int const, idxTab, EDI);

	pThis->SetTab(idxTab);

	return 0x6A65D6;
}

DEFINE_HOOK(0x6A75B9, SidebarClass_SetActiveTab_Strip1, 0x6)
{
	GET(SidebarClass* const, pThis, EBP);

	pThis->Tabs[pThis->ActiveTabIndex].RemoveButtons();

	return 0x6A7602;
}

DEFINE_HOOK(0x6A7619, SidebarClass_SetActiveTab_Strip2, 0x6)
{
	GET(SidebarClass* const, pThis, EBP);

	pThis->Tabs[pThis->ActiveTabIndex].AddButtons();

	return 0x6A76CA;
}

DEFINE_HOOK(0x6A793F, SidebarClass_Update_Strip1, 0x6)
{
	GET(SidebarClass* const, pThis, ESI);

	pThis->Tabs[pThis->ActiveTabIndex].RemoveButtons();

	return 0x6A7988;
}

DEFINE_HOOK(0x6A79A0, SidebarClass_Update_Strip2, 0x6)
{
	GET(SidebarClass* const, pThis, ESI);

	pThis->Tabs[pThis->ActiveTabIndex].AddButtons();

	return 0x6A7A51;
}

DEFINE_HOOK(0x6A7EEE, sub_6A7D70_Strip1, 0x6)
{
	GET(SidebarClass* const, pThis, ESI);

	pThis->Tabs[pThis->ActiveTabIndex].AddButtons();

	return 0x6A7F9F;
}

DEFINE_HOOK(0x6A801C, sub_6A7D70_Strip2, 0x6)
{
	GET(SidebarClass* const, pThis, ESI);

	pThis->Tabs[pThis->ActiveTabIndex].Deactivate();

	return 0x6A8061;
}

// ==========================================================================
// production queues

DEFINE_HOOK(0x4CA0E3, FactoryClass_AbandonProduction_Invalidate, 0x6)
{
	GET(FactoryClass* const, pThis, ESI);

	if(pThis->Owner == HouseClass::CurrentPlayer) {
		if(auto const pObject = pThis->Object) {
			if(pObject->WhatAmI() == BuildingClass::AbsID) {
				ClearSidebarTabObject(static_cast<BuildingClass*>(pObject));
			}
		}
	}

	return 0;
}

// cases 14 and 15 used to fall through into the ones below them
DEFINE_HOOK(0x5005CC, HouseClass_SetFactoryCreatedManually, 0x6)
{
	GET(HouseClass* const, pThis, ECX);

	pThis->InfantryType_53D1 = 1;

	return 0x500612;
}

DEFINE_HOOK(0x50067C, HouseClass_ClearFactoryCreatedManually, 0x6)
{
	GET(HouseClass* const, pThis, ECX);

	pThis->InfantryType_53D1 = 0;

	return 0x5006C0;
}

DEFINE_HOOK(0x5007BE, HouseClass_SetFactoryCreatedManually2, 0x6)
{
	GET(HouseClass* const, pThis, ECX);

	pThis->InfantryType_53D1 = static_cast<BYTE>(R->DL());

	return 0x50080D;
}

// defenses live in their own queue, but share the building factory
DEFINE_HOOK(0x509140, HouseClass_Update_Factories_Queues, 0x5)
{
	GET(HouseClass* const, pThis, ECX);
	GET_STACK(AbstractType const, factoryOf, 0x4);
	GET_STACK(bool const, isNaval, 0x8);
	GET_STACK(BuildCat const, buildCat, 0xC);

	if(factoryOf == BuildingTypeClass::AbsID && buildCat == BuildCat::DontCare) {
		pThis->Update_FactoriesQueues(
			BuildingTypeClass::AbsID, isNaval, BuildCat::Combat);
	}

	MouseClass::Instance.SidebarBackgroundNeedsRedraw = true;

	return 0;
}

DEFINE_HOOK(0x535DB6, SetStructureTabCommandClass_Execute_Power, 0x6)
{
	GET(ObjectClass* const, pObject, EAX);

	R->EAX<BuildingClass*>(WhoCanBuildMe(pObject, false, true));

	return 0x535DC2;
}

DEFINE_HOOK(0x535E76, SetDefenseTabCommandClass_Execute_Power, 0x6)
{
	GET(ObjectClass* const, pObject, EAX);

	R->EAX<BuildingClass*>(WhoCanBuildMe(pObject, false, true));

	return 0x535E82;
}

DEFINE_HOOK(0x6A9822, StripClass_Draw_Power, 0x5)
{
	GET(FactoryClass* const, pThis, ECX);

	auto done = pThis->IsDone();

	if(done) {
		if(auto const pObject = pThis->Object) {
			if(pObject->WhatAmI() == BuildingClass::AbsID) {
				done = WhoCanBuildMe(pObject, true, true) != nullptr;
			}
		}
	}

	R->EAX<bool>(done);

	return 0x6A9827;
}

DEFINE_HOOK(0x6AB312, SidebarClass_ProcessCameoClick_Power, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pType = pThis->GetTechnoType();

	auto const found = HouseExt::HasFactory(
		pThis->Owner, pType, false, true, false, true);

	if(found.State == HouseExt::FactoryState::Unpowered) {
		return 0x6AB95A;
	}

	R->EAX(found.Factory);

	return 0x6AB320;
}

// shift queues up five of a kind at once
DEFINE_HOOK(0x6AB773, SelectClass_ProcessInput_ProduceUnsuspended, 0xA)
{
	GET(EventClass* const, pEvent, EAX);
	GET_STACK(byte const, modifiers, 0xB8);

	auto count = 4 * (modifiers & 1) | 1;

	while(count--) {
		Networking::AddEvent(pEvent);
	}

	return 0x6AB7CC;
}
