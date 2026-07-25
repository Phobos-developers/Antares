#include "Body.h"

#include <Helpers\Macro.h>

DEFINE_HOOK(0x6DD8D7, TActionClass_Execute, 0xA)
{
	GET(TActionClass* const, pAction, ESI);
	GET(ObjectClass* const, pObject, ECX);

	GET_STACK(HouseClass* const, pHouse, 0x254);
	GET_STACK(TriggerClass* const, pTrigger, 0x25C);
	GET_STACK(CellStruct const*, pLocation, 0x260);

	enum { Handled = 0x6DFDDD, Default = 0x6DD8E7u };

	// check for actions handled in Ares.
	auto ret = false;
	if(TActionExt::Execute(
		pAction, pHouse, pObject, pTrigger, *pLocation, &ret))
	{
		// returns true or false
		R->AL(ret ? 1 : 0);
		return Handled;
	}

	// replicate the original instructions, using underflow
	auto const value = static_cast<unsigned int>(pAction->ActionKind) - 1;
	R->EDX(value);
	return (value > 144u) ? Handled : Default;
}

// what to parse the new actions' arguments as
DEFINE_HOOK(0x6E3B60, TActionClass_GetMode, 0x8)
{
	GET(TriggerAction const, actionKind, ECX);

	auto mode = 0;
	if(TActionExt::GetMode(actionKind, &mode)) {
		R->EAX(mode);
		return 0x6E3C4B;
	}

	// replicate the original instructions, using underflow
	auto const value = static_cast<unsigned int>(actionKind) - 1;
	R->EAX(value);
	return (value > 143u) ? 0x6E3C49u : 0x6E3B6Eu;
}

// what the new actions attach to
DEFINE_HOOK(0x6E3EE0, TActionClass_GetFlags, 0x5)
{
	GET(TriggerAction const, actionKind, ECX);

	auto flags = 0;
	if(TActionExt::GetFlags(actionKind, &flags)) {
		R->EAX(flags);
		return 0x6E3EFE;
	}

	return 0;
}

// only objects that actually died count as destroyed
DEFINE_HOOK(0x6E20D8, TActionClass_DestroyAttached_Loop, 0x5)
{
	GET(DamageState const, state, EAX);

	return (state < DamageState::NowDead) ? 0x6E20E0u : 0u;
}
