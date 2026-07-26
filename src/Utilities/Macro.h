#pragma once

#define str(x) str_(x)
#define str_(x) #x

// the less we rely on macros, the better off we will be!

// Public C ABI:
//   DEFINE_EXPORT(HRESULT, GetAntaresAPI, uint32_t want, AntaresAPI_v1** ppApi)
//
// __cdecl, not __stdcall. extern "C" only suppresses C++ mangling; on x86 the
// calling convention still decorates the exported name, and __stdcall would put
// _GetAntaresAPI@8 in the export table -- which GetProcAddress will not find under
// the plain name. __cdecl exports undecorated, and matches EXPORT_FUNC, which is
// how every other symbol this DLL exports is already spelled.
//
// Note Phobos's identically-named macro is __stdcall, so a consumer must take the
// convention from the declaration rather than assume it.
#define DEFINE_EXPORT(ret, name, ...) \
		extern "C" __declspec(dllexport) ret __cdecl name(__VA_ARGS__)

// STACK_OFFS(cur, wanted) reads as "the frame was `cur` deep at hook entry and
// the variable sits at [esp+wanted]". Note YRpp's STACK_OFFSET has the opposite
// sign; Ares's 68 stack reads are all written in this form.
#define STACK_OFFS(cur_offset, wanted_offset) \
		(cur_offset - wanted_offset)
