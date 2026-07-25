#pragma once

#define str(x) str_(x)
#define str_(x) #x

// the less we rely on macros, the better off we will be!

// Public C ABI. Spelled the same way Phobos spells it, so the two projects' interop
// headers read alike:
//   DEFINE_EXPORT(HRESULT, GetAntaresAPI, uint32_t want, AntaresAPI_v1** ppApi)
#define DEFINE_EXPORT(ret, name, ...) \
		extern "C" __declspec(dllexport) ret __stdcall name(__VA_ARGS__)

// STACK_OFFS(cur, wanted) reads as "the frame was `cur` deep at hook entry and
// the variable sits at [esp+wanted]". Note YRpp's STACK_OFFSET has the opposite
// sign; Ares's 68 stack reads are all written in this form.
#define STACK_OFFS(cur_offset, wanted_offset) \
		(cur_offset - wanted_offset)
