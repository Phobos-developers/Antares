#include "Ares.h"
#include "Utilities\Parser.h"
#include <CCINIClass.h>

#include "VersionHelpers.h"

// [GlobalControls] itself moved into RulesExt::ExtData in 3.0p1, where it is
// parsed and streamed with everything else; the shipped RulesClass_Addition
// (0x1002D4B0) calls RulesExt::LoadFromINIFile and nothing more. Only the
// Ares.ini graphics config is left here.
byte Ares::GlobalControls::GFX_DX_Force = 0;

CCINIClass *Ares::GlobalControls::INI = nullptr;

void Ares::GlobalControls::LoadConfig() {
	if(INI->ReadString("Graphics.Advanced", "DirectX.Force", Ares::readDefval, Ares::readBuffer)) {
		if(!_strcmpi(Ares::readBuffer, "hardware")) {
			GFX_DX_Force = GFX_DX_HW;
		} else if(!_strcmpi(Ares::readBuffer, "emulation")) {
			GFX_DX_Force = GFX_DX_EM;
		}
	}
	if(IsWindowsVistaOrGreater()) {
		GFX_DX_Force = 0;
	}
}

DEFINE_HOOK(0x6BC0CD, LoadRA2MD, 0x5)
{
	Ares::GlobalControls::INI = Ares::OpenConfig("Ares.ini");
	Ares::GlobalControls::LoadConfig();
	return 0;
}
