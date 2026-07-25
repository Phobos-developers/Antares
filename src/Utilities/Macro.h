#pragma once

#define str(x) str_(x)
#define str_(x) #x

// the less we rely on macros, the better off we will be!

// STACK_OFFS(cur, wanted) reads as "the frame was `cur` deep at hook entry and
// the variable sits at [esp+wanted]". Note YRpp's STACK_OFFSET has the opposite
// sign; Ares's 68 stack reads are all written in this form.
#define STACK_OFFS(cur_offset, wanted_offset) \
		(cur_offset - wanted_offset)
