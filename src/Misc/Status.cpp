#include "../Ares.version.h"
#include "../Ares.h"

#include <Drawing.h>

#ifdef IS_RELEASE_VER
#define RELEASE 1
#else
#define RELEASE 0
#endif

namespace {
	// Surface::DrawText would route this through Fancy_Text_Print_Wide
	// (0x4A60E0); the splash uses the simple printer, the six-point gradient
	// font, and colours derived from the surface format rather than the
	// RGB565 constants.
	void PrintStatus(wchar_t const* const pText, int const x, int const y,
		int const red, int const green)
	{
		auto const pSurface = DSurface::Hidden;

		RectangleStruct rect = {0, 0, pSurface->Width, pSurface->Height};
		Point2D location = {x, y};
		Point2D ret = {0, 0};

		Simple_Text_Print_Wide(&ret, pText, pSurface, &rect, &location,
			static_cast<unsigned int>(Drawing::RGB_To_Int(red, green, 0)), 0,
			TextPrintType::Point6Grad | TextPrintType::NoShadow, 1);
	}
}

DEFINE_HOOK(0x531413, Game_Start, 0x5)
{
	int topActive = RELEASE ? 500 : 460;

	PrintStatus(L"Ares is active.", 10, topActive, 0, 255);
#if !RELEASE
	PrintStatus(L"This is a testing version, NOT a final product.", 20, 480, 255, 0);
	PrintStatus(L"Bugs are to be expected.", 20, 500, 255, 0);
#endif
	PrintStatus(L"Ares is © The Ares Contributors 2007 - 2021.", 10, 520, 0, 255);

	wchar_t wVersion[256];
	wsprintfW(wVersion, L"%hs", DISPLAY_STRVER);

	PrintStatus(wVersion, 10, 540, 255, 255);
	return 0;
}

DEFINE_HOOK(0x74FDC0, GetModuleVersion, 0x5)
{
	R->EAX<const char *>(VERSION_INTERNAL);
	return 0x74FEEF;
}

DEFINE_HOOK(0x74FAE0, GetModuleInternalVersion, 0x5)
{
	R->EAX<const char *>(DISPLAY_STRMINI);
	return 0x74FC7B;
}

DEFINE_HOOK(0x532017, DlgProc_MainMenu_Version, 0x5)
{
	GET(HWND, hWnd, ESI);

	// account for longer version numbers
	const int MinimumWidth = 168;

	RECT Rect;
	if(GetWindowRect(hWnd, &Rect)) {
		int width = Rect.right - Rect.left;

		if(width < MinimumWidth) {
			// extend to the left by the difference
			Rect.left -= (MinimumWidth - width);

			// if moved out of screen, move right by this amount
			if(Rect.left < 0) {
				Rect.right += -Rect.left;
				Rect.left = 0;
			}

			MoveWindow(hWnd, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, FALSE);
		}
	}

	return 0;
}

// end the loading screen as early as possible, ignoring the delay.
DEFINE_HOOK(0x52CA37, InitGame_Delay, 0x5)
{
	return 0x52CA65;
}
