#pragma once

// Direction helpers YRpp does not provide: a runtime-width DirStruct constructor,
// operator+, and TranslateFixedPoint as a free function.
//
// The radian conversion uses 65535 with a sign-extended raw value, where YRpp
// uses 65536 unsigned. Ares 3.0p1 uses this form, and the difference skews every
// hunter-seeker heading and spotlight sweep by a fraction of a degree -- enough
// to desync multiplayer against a genuine client. Do not "simplify" it.

#include <Dir.h>
#include <YRMath.h>

// Verbatim from the pinned YRpp's GeneralStructures.h. Upstream's
// DirStruct::TranslateFixedPoint<BitsFrom, BitsTo> is the same expression with
// the widths made compile-time, so the two agree wherever both can be used.
inline unsigned int TranslateFixedPoint(size_t bitsFrom, size_t bitsTo, unsigned int value, unsigned int offset = 0) {
	const size_t MaskIn = ((1u << bitsFrom) - 1);
	const size_t MaskOut = ((1u << bitsTo) - 1);

	if(bitsFrom > bitsTo) {
		// converting down
		return (((((value & MaskIn) >> (bitsFrom - bitsTo - 1)) + 1) >> 1) + offset) & MaskOut;

	} else if(bitsFrom < bitsTo) {
		// converting up
		return (((value - offset) & MaskIn) << (bitsTo - bitsFrom)) & MaskOut;

	} else {
		return value & MaskOut;
	}
}

namespace AresDir {

	// the pin's DirStruct::value(), which is signed
	inline short Value(const DirStruct& dir) {
		return static_cast<short>(dir.Raw);
	}

	// the pin's DirStruct::radians(). See the note above: 65535, not 65536, and
	// the raw value is sign-extended first.
	inline double ToRadians(const DirStruct& dir) {
		return static_cast<double>(AresDir::Value(dir) - 65535 / 4) * -(Math::TwoPi / 65535);
	}

	// the pin's DirStruct(double) / DirStruct::radians(double)
	inline DirStruct FromRadians(double rad) {
		auto const value = static_cast<int>(rad * (65535 / Math::TwoPi));
		return DirStruct(static_cast<short>(65535 / 4 - value));
	}

	// the pin's DirStruct(size_t bits, short value)
	template <size_t Bits>
	inline DirStruct FromFacing(short value) {
		DirStruct ret;
		ret.SetValue<Bits>(static_cast<unsigned short>(value));
		return ret;
	}

	// the pin's DirStruct::operator+
	inline DirStruct Add(const DirStruct& lhs, const DirStruct& rhs) {
		return DirStruct(static_cast<unsigned short>(lhs.Raw + rhs.Raw));
	}
}
