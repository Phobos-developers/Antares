#include "Body.h"

#include "../../Misc/SavegameDef.h"

AbstractExt::ExtContainer AbstractExt::ExtMap;

// =============================
// container

AbstractExt::ExtContainer::ExtContainer() : Container("AbstractClass") {
}

AbstractExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

#ifdef MAKE_GAME_SLOWER_FOR_NO_REASON
DEFINE_HOOK(0x4101B6, AbstractClass_CTOR, 0x1)
{
	GET(AbstractClass*, pItem, EAX);

	AbstractExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x4101F0, AbstractClass_DTOR, 0x6)
{
	GET(AbstractClass*, pItem, ECX);

	AbstractExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x410320, AbstractClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x410380, AbstractClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(AbstractClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	AbstractExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x4103D6, AbstractClass_Load_Suffix, 0x4)
{
	AbstractExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x410372, AbstractClass_Save_Suffix, 0x5)
{
	AbstractExt::ExtMap.SaveStatic();
	return 0;
}
#endif
