#include "Body.h"
#include <Utilities/Macro.h>   // STACK_OFFS
#include "../Techno/Body.h"

#include <ConvertClass.h>
#include <Drawing.h>

static int BoltColor1;
static int BoltColor2;
static int BoltColor3;

DEFINE_HOOK(0x6FD469, TechnoClass_FireEBolt, 0x9)
{
	//GET(TechnoClass*, pThis, EDI);
	GET_STACK(WeaponTypeClass*, pWeapon, STACK_OFFS(0x30, -0x8));

	R->EAX(WeaponTypeExt::CreateBolt(pWeapon));
	R->ESI(0);

	return 0x6FD480;
}

DEFINE_HOOK(0x4C2951, EBolt_DTOR, 0x5)
{
	GET(EBolt *, Bolt, ECX);
	WeaponTypeExt::BoltExt.erase(Bolt);
	return 0;
}

// the colors are picked once and then applied to each of the three arcs
DEFINE_HOOK(0x4C1F33, EBolt_Draw_Colors, 0x7)
{
	GET(EBolt* const, pThis, ECX);
	GET_BASE(int const, index, 0x20);

	auto const pDrawer = *reinterpret_cast<ConvertClass**>(0x87F6C4);
	auto const pColors = static_cast<byte*>(pDrawer->PaletteData);   // Midpoint in the pinned YRpp

	if(pDrawer->BytesPerPixel == 1) {
		BoltColor1 = BoltColor2 = pColors[index];
		BoltColor3 = pColors[15];
	} else {
		auto const pWordColors = reinterpret_cast<WORD*>(pColors);
		BoltColor1 = BoltColor2 = pWordColors[index];
		BoltColor3 = pWordColors[15];
	}

	if(auto const pData = WeaponTypeExt::BoltExt.get_or_default(pThis)) {
		if(pData->Bolt_Color1.isset()) {
			BoltColor1 = Drawing::RGB_To_Int(pData->Bolt_Color1.Get());
		}

		if(pData->Bolt_Color2.isset()) {
			BoltColor2 = Drawing::RGB_To_Int(pData->Bolt_Color2.Get());
		}

		if(pData->Bolt_Color3.isset()) {
			BoltColor3 = Drawing::RGB_To_Int(pData->Bolt_Color3.Get());
		}
	}

	return 0x4C1F66;
}

DEFINE_HOOK(0x4C24BE, EBolt_Draw_Color1, 0x5)
{
	R->EAX(BoltColor1);
	return 0x4C24E4;
}

DEFINE_HOOK(0x4C25CB, EBolt_Draw_Color2, 0x5)
{
	R->Stack(0x18, BoltColor2);
	return 0x4C25FD;
}

DEFINE_HOOK(0x4C26CF, EBolt_Draw_Color3, 0x5)
{
	R->EAX(BoltColor3);
	return 0x4C26EE;
}
