#include "FlyingStrings.h"

#include "../Ares.h"

#include <algorithm>

#include <Drawing.h>
#include <Surface.h>
#include <TacticalClass.h>

std::vector<FlyingStrings::Item> FlyingStrings::Items;

void FlyingStrings::Add(
	const wchar_t* const pText, const CoordStruct& coords,
	WORD const color, int const duration)
{
	auto& item = FlyingStrings::Items.emplace_back();

	if(pText) {
		wcsncpy_s(item.Text, pText, _TRUNCATE);
	} else {
		item.Text[0] = L'\0';
	}

	item.Location = coords;
	item.RemainingTime = duration + 70;
	item.Color = color;
}

void FlyingStrings::Draw()
{
	for(auto& item : FlyingStrings::Items) {
		Point2D pos;
		if(TacticalClass::Instance->CoordsToClient(&item.Location, &pos)) {
			// hold the anchor for six frames, then rise a pixel a frame
			if(item.RemainingTime < 70) {
				pos.Y += item.RemainingTime - 70;
			}

			Point2D unused;
			Simple_Text_Print_Wide(&unused, item.Text, DSurface::Temp,
				&DSurface::ViewBounds, &pos, item.Color, 0,
				TextPrintType::Center, 1);
		}
	}

	auto const it = std::remove_if(
		FlyingStrings::Items.begin(), FlyingStrings::Items.end(),
		[](Item& item) { return --item.RemainingTime < 0; });

	FlyingStrings::Items.erase(it, FlyingStrings::Items.end());
}

void FlyingStrings::Clear()
{
	FlyingStrings::Items.clear();
}

DEFINE_HOOK(0x6D4684, TacticalClass_Draw_FlyingStrings, 0x6)
{
	FlyingStrings::Draw();
	return 0;
}
