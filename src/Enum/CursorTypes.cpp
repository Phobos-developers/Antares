#include "CursorTypes.h"

#include "../Utilities/TemplateDef.h"

#include <Helpers/Macro.h>

Enumerable<CursorType>::container_t Enumerable<CursorType>::Array;

int CursorType::SelectedIndex = 0;

MouseCursor* CursorType::SelectedCursor = nullptr;

CursorType::ActionCursor CursorType::ActionCursors[CursorType::ActionCursorCount];

// set once the game has loaded the cursor shapes and animated them at least once
static bool& MouseShapesAnimated = *reinterpret_cast<bool*>(0xABF2DD);

static void* const& MouseShapes = *reinterpret_cast<void**>(0xABF294);

// the vanilla cursor table, in engine order. index N here is cursor N.
static const char* const DefaultCursorNames[std::extent_v<std::remove_reference_t<decltype(MouseCursor::Cursors)>>] = {
	"Default",
	"MoveN",
	"MoveNE",
	"MoveE",
	"MoveSE",
	"MoveS",
	"MoveSW",
	"MoveW",
	"MoveNW",
	"NoMoveN",
	"NoMoveNE",
	"NoMoveE",
	"NoMoveSE",
	"NoMoveS",
	"NoMoveSW",
	"NoMoveW",
	"NoMoveNW",
	"Select",
	"Move",
	"NoMove",
	"Attack",
	"AttackOutOfRange",
	"CUR_16",
	"DesolatorDeploy",
	"CUR_18",
	"Enter",
	"NoEnter",
	"Deploy",
	"NoDeploy",
	"CUR_1D",
	"Sell",
	"SellUnit",
	"NoSell",
	"Repair",
	"EngineerRepair",
	"NoRepair",
	"CUR_24",
	"Disguise",
	"IvanBomb",
	"MindControl",
	"RemoveSquid",
	"Crush",
	"SpyTech",
	"SpyPower",
	"CUR_2C",
	"GIDeploy",
	"CUR_2E",
	"Paradrop",
	"CUR_30",
	"CUR_31",
	"LightningStorm",
	"Detonate",
	"Demolish",
	"Nuke",
	"CUR_36",
	"Power",
	"CUR_38",
	"IronCurtain",
	"Chronosphere",
	"Disarm",
	"CUR_3C",
	"Scroll",
	"ScrollESW",
	"ScrollSW",
	"ScrollNSW",
	"ScrollNW",
	"ScrollNEW",
	"ScrollNE",
	"ScrollNES",
	"ScrollES",
	"CUR_46",
	"AttackMove",
	"CUR_48",
	"InfantryAbsorb",
	"NoMindControl",
	"CUR_4B",
	"CUR_4C",
	"CUR_4D",
	"Beacon",
	"ForceShield",
	"NoForceShield",
	"GeneticMutator",
	"AirStrike",
	"PsychicDominator",
	"PsychicReveal",
	"SpyPlane"
};

// the cursors appended after the vanilla table. Source is the index whose data
// is copied, or -1 for the one entry with a literal payload.
struct DefaultCursorExtra {
	const char* Name;
	int Source;
};

static const DefaultCursorExtra DefaultCursorExtras[] = {
	{ "Tote", 54 },
	{ "EngineerDamage", 51 },
	{ "TogglePower", 55 },
	{ "NoTogglePower", 60 },
	{ "InfantryHeal", -1 },
	{ "UnitRepair", 33 },
	{ "TakeVehicle", 25 },
	{ "Sabotage", 25 },
	{ "RepairTrench", 33 }
};

static const size_t DefaultCursorExtraCount =
	sizeof(DefaultCursorExtras) / sizeof(*DefaultCursorExtras);

static const MouseCursor DefaultInfantryHeal(
	355, 1, 0, -1, -1, MouseHotSpotX::Center, MouseHotSpotY::Middle);

const char * Enumerable<CursorType>::GetMainSection()
{
	return "MouseCursors";
}

CursorType::CursorType(const char* const pTitle)
	: Enumerable<CursorType>(pTitle),
	Data(0, 1, 0, -1, -1, MouseHotSpotX::Left, MouseHotSpotY::Top)
{ }

CursorType::~CursorType() = default;

void CursorType::Clear()
{
	Array.clear();
	CursorType::LoadDefault();

	CursorType::SelectedIndex = 0;
	CursorType::SelectedCursor = &Array[0]->Data;
}

void CursorType::LoadDefault()
{
	for(size_t i = 0; i < std::extent_v<std::remove_reference_t<decltype(MouseCursor::Cursors)>>; ++i) {
		CursorType::FindOrAllocate(DefaultCursorNames[i])->Data = MouseCursor::Cursors[i];
	}

	MouseCursor buffer[DefaultCursorExtraCount];

	for(size_t i = 0; i < DefaultCursorExtraCount; ++i) {
		int const source = DefaultCursorExtras[i].Source;
		buffer[i] = (source < 0)
			? DefaultInfantryHeal
			: Array[static_cast<size_t>(source)]->Data;
	}

	for(size_t i = 0; i < DefaultCursorExtraCount; ++i) {
		CursorType::FindOrAllocate(DefaultCursorExtras[i].Name)->Data = buffer[i];
	}
}

void CursorType::LoadFromINI(CCINIClass *pINI)
{
	const char *section = Enumerable<CursorType>::GetMainSection();

	if(!pINI->ReadString(section, this->Name, Ares::readDefval, Ares::readBuffer)) {
		return;
	}

	if(auto const pOther = CursorType::Find(Ares::readBuffer)) {
		this->Data = pOther->Data;
		return;
	}

	char* context = nullptr;

	if(auto const pFrame = strtok_s(Ares::readBuffer, ",", &context)) {
		Parser<int>::TryParse(pFrame, &this->Data.Frame);
	}
	if(auto const pCount = strtok_s(nullptr, ",", &context)) {
		Parser<int>::TryParse(pCount, &this->Data.Count);
	}
	if(auto const pInterval = strtok_s(nullptr, ",", &context)) {
		Parser<int>::TryParse(pInterval, &this->Data.Interval);
	}
	if(auto const pMiniFrame = strtok_s(nullptr, ",", &context)) {
		Parser<int>::TryParse(pMiniFrame, &this->Data.MiniFrame);
	}
	if(auto const pMiniCount = strtok_s(nullptr, ",", &context)) {
		Parser<int>::TryParse(pMiniCount, &this->Data.MiniCount);
	}
	if(auto const pHotX = strtok_s(nullptr, ",", &context)) {
		MouseCursorHotSpotX::Parse(pHotX, &this->Data.HotX);
	}
	if(auto const pHotY = strtok_s(nullptr, ",", &context)) {
		MouseCursorHotSpotY::Parse(pHotY, &this->Data.HotY);
	}
}

void CursorType::LoadFromStream(AresStreamReader &Stm)
{
	Stm
		.Process(this->Data);
}

void CursorType::SaveToStream(AresStreamWriter &Stm)
{
	Stm
		.Process(this->Data);
}

bool CursorType::LoadGlobals(AresStreamReader &Stm)
{
	if(!Enumerable<CursorType>::LoadGlobals(Stm)) {
		return false;
	}

	if(!Stm.Load(CursorType::SelectedIndex)) {
		return false;
	}

	CursorType::SelectedCursor = CursorType::GetCursor(
		static_cast<MouseCursorType>(CursorType::SelectedIndex));

	return true;
}

bool CursorType::SaveGlobals(AresStreamWriter &Stm)
{
	if(!Enumerable<CursorType>::SaveGlobals(Stm)) {
		return false;
	}

	Stm.Save(CursorType::SelectedIndex);

	return true;
}

MouseCursor* CursorType::GetCursor(MouseCursorType index)
{
	auto const idx = static_cast<size_t>(index);

	// out of range indices are reinterpreted as pointers, as they are in 3.0p1
	return (idx < Array.size())
		? &Array[idx]->Data
		: reinterpret_cast<MouseCursor*>(idx)
	;
}

void CursorType::Select(MouseCursorType index)
{
	CursorType::SelectedCursor = CursorType::GetCursor(index);
	CursorType::SelectedIndex = static_cast<int>(index);
}

void CursorType::ClearActions()
{
	for(auto& action : CursorType::ActionCursors) {
		action.Index = 0;
		action.Mode = 0;
	}
}

void CursorType::SetAction(MouseCursorType index, Action action, int mode)
{
	auto& item = CursorType::ActionCursors[static_cast<int>(action) + 1];
	item.Index = static_cast<int>(index);
	item.Mode = mode;
}

void CursorType::AddMappedAction(MouseCursorType index, bool fireIntoShroud, Action action)
{
	auto& item = CursorType::ActionCursors[static_cast<int>(action) + 1];
	item.Index = static_cast<int>(index);
	item.Mode = (fireIntoShroud ? 0 : 1) + 1;
}

const CursorType::ActionCursor* CursorType::FindAction(Action action)
{
	auto index = static_cast<size_t>(0);

	if(!CursorType::ActionCursors[0].Index) {
		index = static_cast<size_t>(static_cast<int>(action) + 1);

		if(index >= CursorType::ActionCursorCount) {
			return nullptr;
		}
	}

	auto const& item = CursorType::ActionCursors[index];
	return item.Index ? &item : nullptr;
}

DEFINE_HOOK(0x5BDC8C, MouseClass_UpdateCursor, 0x7)
{
	GET(MouseClass* const, pThis, EBP);
	GET(MouseCursorType const, index, EAX);
	GET_STACK(bool const, miniMap, 0x24);

	auto const pCursor = CursorType::GetCursor(index);
	auto const isMini = miniMap && pCursor->MiniFrame >= 0;

	if(MouseShapesAnimated && (!MouseShapes
		|| (pThis->MouseCursorIndex == index && pThis->MouseCursorIsMini == isMini)))
	{
		return 0x5BDCD8;
	}

	CursorType::SelectedCursor = pCursor;
	CursorType::SelectedIndex = static_cast<int>(index);

	R->ESI(pCursor);
	R->EBX(isMini);

	return 0x5BDCE3;
}

DEFINE_HOOK(0x5BDBC4, MouseClass_GetCursorCurrentFrame, 0x7)
{
	GET(MouseCursorType const, index, EAX);

	R->EAX(CursorType::GetCursor(index));

	return 0x5BDBD4;
}

DEFINE_HOOK(0x5BE974, MouseClass_GetCursorFirstFrame, 0x7)
{
	GET(MouseCursorType const, index, EAX);

	R->EAX(CursorType::GetCursor(index)->Frame);

	return 0x5BE9A4;
}

DEFINE_HOOK(0x5BE994, MouseClass_GetCursorFrameCount, 0x7)
{
	GET(MouseCursorType const, index, EAX);

	R->EAX(CursorType::GetCursor(index)->Count);

	return 0x5BE9A4;
}

DEFINE_HOOK(0x5BDB90, MouseClass_GetCursorFirstFrame_Minimap, 0xB)
{
	GET_STACK(MouseCursorType const, index, 0x4);
	GET_STACK(bool const, miniMap, 0x8);

	auto const pCursor = CursorType::GetCursor(index);

	R->EAX((miniMap && pCursor->MiniFrame >= 0) ? pCursor->MiniFrame : pCursor->Frame);

	return 0x5BDBB6;
}

DEFINE_HOOK(0x5BDC1B, MouseClass_GetCursorHotSpot, 0x7)
{
	GET(MouseCursorType const, index, EAX);

	auto const pCursor = CursorType::GetCursor(index);

	R->ESI(pCursor);
	R->ECX(pCursor->HotX);

	return 0x5BDC29;
}

DEFINE_HOOK(0x5BDADF, MouseClass_UpdateCursorMinimapState_UseCursor, 0x0)
{
	R->EBP(CursorType::SelectedCursor);

	return R->DL()
		? 0x5BDAEC
		: 0x5BDAFA
	;
}

DEFINE_HOOK(0x5BDDC8, MouseClass_Update_AnimateCursor, 0x6)
{
	auto const pCursor = CursorType::SelectedCursor;

	R->EBX(pCursor);

	return pCursor->Interval
		? 0x5BDDED
		: 0x5BDF13
	;
}

DEFINE_HOOK(0x5BDE64, MouseClass_Update_AnimateCursor2, 0x6)
{
	GET(MouseClass const* const, pThis, ESI);

	R->ECX(CursorType::SelectedCursor);

	return pThis->MouseCursorIsMini
		? 0x5BDE84
		: 0x5BDE92
	;
}

static_assert(offsetof(MouseClass, MouseCursorIsMini) == 0x555C, "MouseClass layout slipped");
static_assert(offsetof(MouseClass, MouseCursorIndex) == 0x5560, "MouseClass layout slipped");

static_assert(sizeof(MouseCursor) == 0x1C, "MouseCursor must match the 3.0p1 payload");
static_assert(offsetof(MouseCursor, HotY) == 0x18, "MouseCursor layout slipped");

static_assert(sizeof(CursorType) == 0x40, "CursorType must match the 3.0p1 layout plus the vptr");
static_assert(offsetof(CursorType, Data) == 0x24, "CursorType layout slipped");
