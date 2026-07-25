#include "../Ext/Rules/Body.h"

/***
 * the following hooks replace the original checks that disable certain visual
 * effects when the frame rate drops below a certain limit. the issue with them
 * was that they only take into account the settings from the rulesmd.ini, but
 * ignore the game speed setting. that means if the frame rate is supposed to
 * be 20 frames or less (DetailMinFrameRateNormal=15, DetailBufferZoneWidth=5),
 * then the effects are still disabled. lasers draw as ugly lines, non-damaging
 * particles don't render, spotlights aren't created, ...
 ***/

DEFINE_HOOK(0x48A634, FlashbangWarheadAt_Details, 0x5)
{
	auto const details = RulesExt::DetailsCurrentlyEnabled();
	return details ? 0x48A64Au : 0x48A641u;
}

DEFINE_HOOK(0x5FF86E, SpotLightClass_Draw_Details, 0x5)
{
	auto const details = RulesExt::DetailsCurrentlyEnabled();
	return details ? 0x5FF87Fu : 0x5FFF77u;
}

DEFINE_HOOK(0x422FCC, AnimClass_Draw_Details, 0x5)
{
	auto const details = RulesExt::DetailsCurrentlyEnabled();
	return details ? 0x422FECu : 0x422FD9u;
}

DEFINE_HOOK(0x550BCA, LaserDrawClass_Draw_InHouseColor_Details, 0x5)
{
	auto const details = RulesExt::DetailsCurrentlyEnabled();
	return details ? 0x550BD7u : 0x550BE5u;
}

DEFINE_HOOK(0x62CEC9, ParticleClass_Draw_Details, 0x5)
{
	auto const details = RulesExt::DetailsCurrentlyEnabled();
	return details ? 0x62CEEAu : 0x62CED6u;
}

DEFINE_HOOK(0x6D7847, TacticalClass_DrawPixelEffects_Details, 0x5)
{
	auto const details = RulesExt::DetailsCurrentlyEnabled();
	return details ? 0x6D7858u : 0x6D7BF2u;
}

/***
 * the following hooks replace pairs of comparisons that throw away the
 * value they just compared and call the same function a second time to get
 * it back. every one of these calls is virtual and several are hooked, so
 * the repeat is not free - and, if a hook ever makes the result depend on
 * anything but its input, not even reliable. the values are already in
 * registers, so pick between those instead.
 ***/

DEFINE_HOOK(0x458E1E, BuildingClass_GetOccupyRangeBonus_Demacroize, 0xA)
{
	GET(int const, height, EDI);
	GET(int const, width, EAX);

	R->EAX(Math::min(width, height));

	return 0x458E2D;
}

DEFINE_HOOK(0x6F90F8, TechnoClass_SelectAutoTarget_Demacroize, 0x6)
{
	GET(int const, secondary, EDI);
	GET(int const, primary, EAX);

	R->EAX(Math::max(primary, secondary));

	return 0x6F9116;
}

DEFINE_HOOK(0x70133E, TechnoClass_GetWeaponRange_Demacroize, 0x5)
{
	GET(int const, passengerRange, EDI);
	GET(int const, range, EBX);

	R->EAX(Math::min(range, passengerRange));

	return 0x701388;
}

DEFINE_HOOK(0x707EEA, TechnoClass_GetGuardRange_Demacroize, 0x6)
{
	GET(int const, secondary, EBX);
	GET(int const, primary, EAX);

	R->EAX(Math::max(primary, secondary));

	return 0x707F08;
}
