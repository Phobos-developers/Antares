#pragma once

#include <vector>

#include <GeneralStructures.h>

// the floating "+$500" ticker over objects that just earned or lost credits
class FlyingStrings
{
public:
	struct Item {
		CoordStruct Location;
		int RemainingTime;
		WORD Color;
		wchar_t Text[32];
	};

	static void Add(const wchar_t* pText, const CoordStruct& coords, WORD color, int duration);

	static void Draw();

	static void Clear();

private:
	static std::vector<Item> Items;
};
