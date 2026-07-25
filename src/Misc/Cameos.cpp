#include "../Ext/TechnoType/Body.h"
#include <Utilities/Macro.h>   // STACK_OFFS
#include "../Ext/SWType/Body.h"

#include <HouseClass.h>
#include <PCX.h>

// bugfix #277 revisited: VeteranInfantry and friends don't show promoted cameos
DEFINE_HOOK(0x712045, TechnoTypeClass_GetCameo, 0x5)
{
	// egads and gadzooks
	retfunc<SHPStruct const*> ret(R, 0x7120C6);

	GET(TechnoTypeClass const* const, pThis, ECX);

	auto const pCameo = pThis->Cameo;
	auto const pAlt = pThis->AltCameo;

	auto const pData = TechnoTypeExt::ExtMap.Find(pThis);

	return ret(pData->CameoIsElite(HouseClass::CurrentPlayer) ? pAlt : pCameo);
}

// a global var ewww
ConvertClass * CurrentDrawnConvert = nullptr;
BSurface * CameoPCX = nullptr;

DEFINE_HOOK(0x6A9948, StripClass_Draw_SuperWeapon, 0x6)
{
	GET(SuperWeaponTypeClass *, pSW, EAX);
	if(SWTypeExt::ExtData *pData = SWTypeExt::ExtMap.Find(pSW)) {
		CurrentDrawnConvert = pData->CameoPal.GetConvert();
	}
	return 0;
}

DEFINE_HOOK(0x6A9A2A, StripClass_Draw_Main, 0x6)
{
	GET_STACK(TechnoTypeClass *, pTech, STACK_OFFS(0x4C4, 0x458));

	ConvertClass *pPalette = nullptr;
	if(pTech) {
		if(TechnoTypeExt::ExtData *pData = TechnoTypeExt::ExtMap.Find(pTech)) {
			pPalette = pData->CameoPal.GetConvert();
		}
	} else if(CurrentDrawnConvert) {
		pPalette = CurrentDrawnConvert;
		CurrentDrawnConvert = nullptr;
	}

	if(!pPalette) {
		pPalette = FileSystem::CAMEO_PAL;
	}
	R->EDX<ConvertClass *>(pPalette);
	return 0x6A9A30;
}


DEFINE_HOOK(0x6A9952, StripClass_Draw_SuperWeapon_PCX, 0x6)
{
	GET(SuperWeaponTypeClass *, pSW, EAX);
	auto pData = SWTypeExt::ExtMap.Find(pSW);
	CameoPCX = pData->SidebarPCX.GetSurface();
	return 0;
}

DEFINE_HOOK(0x6A980A, StripClass_Draw_TechnoType_PCX, 0x8)
{
	GET(TechnoTypeClass const* const, pType, EBX);

	auto const pData = TechnoTypeExt::ExtMap.Find(pType);

	auto const eliteCameo = pData->CameoIsElite(HouseClass::CurrentPlayer)
		&& pData->AltCameoPCX.Exists();

	const auto& pcxFile = eliteCameo ? pData->AltCameoPCX : pData->CameoPCX;
	CameoPCX = pcxFile.GetSurface();

	return 0;
}

DEFINE_HOOK(0x6A99F3, StripClass_Draw_SkipSHPForPCX, 0x6)
{
	return (CameoPCX)
		? 0x6A9A43
		: 0;
}

DEFINE_HOOK(0x6A9A43, StripClass_Draw_DrawPCX, 0x6)
{
	if(CameoPCX) {
		GET(int, TLX, ESI);
		GET(int, TLY, EBP);
		RectangleStruct bounds = { TLX, TLY, 60, 48 };
		PCX::Instance.BlitToSurface(&bounds, DSurface::Sidebar, CameoPCX);
		CameoPCX = nullptr;
	}
	return 0;
}
