#include <RadarClass.h>
#include <Surface.h>

#include "../Ares.h"

// the two radar surfaces are drawn to without being locked, which is a
// no-op on some drivers and garbage on others
static void LockRadarSurfaces(RadarClass* const pThis)
{
	reinterpret_cast<Surface*>(pThis->unknown_121C)->Lock(0, 0);
	reinterpret_cast<Surface*>(pThis->unknown_1220)->Lock(0, 0);
}

static void UnlockRadarSurfaces(RadarClass* const pThis)
{
	reinterpret_cast<Surface*>(pThis->unknown_1220)->Unlock();
	reinterpret_cast<Surface*>(pThis->unknown_121C)->Unlock();
}

DEFINE_HOOK(0x65731F, RadarClass_UpdateMinimap_Lock, 0x6)
{
	GET(RadarClass* const, pThis, ESI);

	LockRadarSurfaces(pThis);

	return 0;
}

DEFINE_HOOK(0x65757C, RadarClass_UpdateMinimap_Unlock, 0x8)
{
	GET(RadarClass* const, pThis, ESI);

	UnlockRadarSurfaces(pThis);

	return (R->AL() != 0) ? 0x657584 : 0x6576A5;
}

DEFINE_HOOK(0x657CF2, MapClass_MinimapChanged_Lock1, 0x6)
{
	LockRadarSurfaces(&RadarClass::Instance);

	return 0;
}

DEFINE_HOOK(0x657D35, MapClass_MinimapChanged_Unlock1, 0x7)
{
	UnlockRadarSurfaces(&RadarClass::Instance);

	return 0;
}

DEFINE_HOOK(0x657D3D, MapClass_MinimapChanged_Lock2, 0x6)
{
	LockRadarSurfaces(&RadarClass::Instance);

	return 0;
}

DEFINE_HOOK(0x657D8A, MapClass_MinimapChanged_Unlock2, 0x7)
{
	UnlockRadarSurfaces(&RadarClass::Instance);

	return 0;
}
