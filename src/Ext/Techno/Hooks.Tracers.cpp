#include "Body.h"
#include "../TechnoType/Body.h"
#include "../../Misc/Debug.h"

#if 0
A_FINE_HOOK(0x6F2B40, TechnoClass_CTOR_Log, 0x6)
{
	GET(TechnoClass *, pThis, ECX);
	Debug::Log("CTOR: %X", pThis);
	Debug::DumpStack(R, 0x20);
	return 0;
}

A_FINE_HOOK(0x6F4500, TechnoClass_DTOR_Log, 0x5)
{
	GET(TechnoClass *, pThis, ECX);
	Debug::Log("DTOR: %X", pThis);
	Debug::DumpStack(R, 0x20);
	return 0;
}

A_FINE_HOOK(0x4720E8, CaptureManagerClass_FreeUnit, 0xa)
{
	GET(TechnoClass *, T, ESI);
	GET(DWORD, mgr, EDI);
	Debug::Log("Freeing %s from mind control (mgr = %X)\n", T->GetType()->ID, mgr);
	Debug::DumpStack(R, 0x20);
	return 0;
}

A_FINE_HOOK(0x471E34, CaptureManagerClass_CaptureUnit, 0x6)
{
	GET(TechnoClass *, T, ESI);
	GET(DWORD, mgr, EBX);
	Debug::Log("Putting %s under mind control (mgr = %X)\n", T->GetType()->ID, mgr);
	Debug::DumpStack(R, 0x20);
	return 0;
}
#endif
