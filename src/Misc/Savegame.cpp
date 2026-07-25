#include "SavegameDef.h"
#include <Utilities/Macro.h>   // STACK_OFFS

#include "../Ares.h"
#include "../Ares.version.h"

#include <LoadOptionsClass.h>

DEFINE_HOOK(0x67D300, SaveGame_Start, 0x5)
{
	Ares::SaveGame();
	return 0;
}

const byte SaveGame_ReturnCode[] = {
	0x85, 0xC0,       // test eax, eax
	0x5F,             // pop edi
	0x5E,             // pop esi
	0x5D,             // pop ebp
	0x5B,             // pop ebx
	0x0F, 0x9D, 0xC0, // setnl al
	0x83, 0xC4, 0x08, // add esp, 8
	0xC3              // retn
};

DEFINE_HOOK(0x67E42E, SaveGame, 0x5)
{
	GET(HRESULT, Status, EAX);

	if(SUCCEEDED(Status)) {
		GET(IStream *, pStm, ESI);

		Status = Ares::SaveGameData(pStm);
		R->EAX<HRESULT>(Status);
	}

	return reinterpret_cast<DWORD>(SaveGame_ReturnCode);
}

DEFINE_HOOK(0x67E730, LoadGame_Start, 0x5)
{
	Ares::LoadGame();
	return 0;
}

DEFINE_HOOK(0x67F7C8, LoadGame_End, 0x5)
{
	GET(IStream *, pStm, ESI);

	Ares::LoadGameData(pStm);

	return 0;
}

DEFINE_HOOK(0x67D04E, Game_Save_SavegameInformation, 0x7)
{
	REF_STACK(SavegameInformation, Info, STACK_OFFS(0x4A4, 0x3F4));

	// remember the Ares version and a mod id
	Info.Version = Ares::UISettings::ModIdentifier;
	Info.InternalVersion = SAVEGAME_MAGIC;
	sprintf_s(Info.ExecutableName.data(), "GAMEMD.EXE + %s", DISPLAY_STREX);

	return 0;
}

DEFINE_HOOK(0x559F31, LoadOptionsClass_GetFileInfo, 0x9)
{
	REF_STACK(SavegameInformation, Info, STACK_OFFS(0x400, 0x3F4));

	// compare equal if same mod and same Ares version (or compatible)
	auto same = (Info.Version == Ares::UISettings::ModIdentifier
		&& Info.InternalVersion == SAVEGAME_MAGIC);

	R->ECX(&Info);
	return same ? 0x559F60u : 0x559F48u;
}

// log message uses wrong format specifier
DEFINE_HOOK(0x67CEFE, Game_Save_FixLog, 0x7)
{
	GET(const char*, pFilename, EDI);
	GET(const wchar_t*, pSaveName, ESI);

	Debug::Log("\nSAVING GAME [%s - %ls]\n", pFilename, pSaveName);

	return 0x67CF0D;
}

// #895374: skip the code that removes the crates
DEFINE_HOOK(0x483BF1, CellClass_Load_Crates, 0x7)
{
	return 0x483BFE;
}
