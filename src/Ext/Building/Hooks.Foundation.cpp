#include "Body.h"

#include "../BuildingType/Body.h"

#include <MouseClass.h>

DEFINE_HOOK(0x45EC90, BuildingTypeClass_GetFoundationWidth, 0x6)
{
	GET(BuildingTypeClass*, pThis, ECX);

	if(pThis->Foundation == BuildingTypeExt::CustomFoundation) {
		auto const pExt = BuildingTypeExt::ExtMap.Find(pThis);

		R->EAX(pExt->CustomWidth);
		return 0x45EC9D;
	}

	return 0;
}

DEFINE_HOOK(0x45ECA0, BuildingTypeClass_GetFoundationHeight, 0x6)
{
	GET(BuildingTypeClass*, pThis, ECX);

	if(pThis->Foundation == BuildingTypeExt::CustomFoundation) {
		auto const pExt = BuildingTypeExt::ExtMap.Find(pThis);

		auto height = pExt->CustomHeight;
		if(R->Stack8(0x4) && pThis->Bib) {
			++height;
		}

		R->EAX(height);
		return 0x45ECDA;
	}

	return 0;
}

DEFINE_HOOK(0x656584, RadarClass_GetFoundationShape, 0x6)
{
	GET(RadarClass*, pThis, ECX);
	GET(BuildingTypeClass*, pType, EAX);

	auto fnd = pType->Foundation;
	DynamicVectorClass<Point2D>* ret = nullptr;

	if(fnd >= Foundation::_1x1 && fnd <= Foundation::_0x0) {
		// in range of default foundations
		ret = &pThis->FoundationTypePixels[static_cast<int>(fnd)];
	} else if(auto pExt = BuildingTypeExt::ExtMap.Find(pType)) {
		// custom foundation
		ret = &pExt->FoundationRadarShape;
	} else {
		// default if everything fails
		ret = &pThis->FoundationTypePixels[static_cast<int>(Foundation::_2x2)];
	}

	R->EAX(ret);
	return 0x656595;
}

DEFINE_HOOK(0x6563B0, RadarClass_UpdateFoundationShapes_Custom, 0x5)
{
	// update each building type foundation
	for(auto const& pType : BuildingTypeClass::Array) {
		if(auto pExt = BuildingTypeExt::ExtMap.Find(pType)) {
			pExt->UpdateFoundationRadarShape();
		}
	}

	return 0;
}

DEFINE_HOOK(0x568411, MapClass_AddContentAt_Foundation_P1, 0x0)
{
	GET(BuildingClass *, pThis, EDI);

	R->EBP(pThis->GetFoundationData(false));

	return 0x568432;
}

DEFINE_HOOK(0x568565, MapClass_AddContentAt_Foundation_OccupyHeight, 0x5)
{
	GET(BuildingClass *, pThis, EDI);
	GET(int, ShadowHeight, EBP);
	GET_STACK(CellStruct *, MainCoords, 0x8B4);

	auto const& AffectedCells = BuildingExt::GetCoveredCells(
		pThis, *MainCoords, ShadowHeight);

	auto &Map = MapClass::Instance;

	for(auto const& cell : AffectedCells) {
		if(auto pCell = Map.TryGetCellAt(cell)) {
			++pCell->OccupyHeightsCoveringMe;
		}
	}

	return 0x568697;
}

DEFINE_HOOK(0x568841, MapClass_RemoveContentAt_Foundation_P1, 0x0)
{
	GET(BuildingClass *, pThis, EDI);

	R->EBP(pThis->GetFoundationData(false));

	return 0x568862;
}

DEFINE_HOOK(0x568997, MapClass_RemoveContentAt_Foundation_OccupyHeight, 0x5)
{
	GET(BuildingClass *, pThis, EDX);
	GET(int, ShadowHeight, EBP);
	GET_STACK(CellStruct *, MainCoords, 0x8B4);

	auto const& AffectedCells = BuildingExt::GetCoveredCells(
		pThis, *MainCoords, ShadowHeight);

	auto &Map = MapClass::Instance;

	for(auto const& cell : AffectedCells) {
		if(auto const pCell = Map.TryGetCellAt(cell)) {
			if(pCell->OccupyHeightsCoveringMe > 0) {
				--pCell->OccupyHeightsCoveringMe;
			}
		}
	}

	return 0x568ADC;
}


DEFINE_HOOK(0x4A8C77, DisplayClass_ProcessFoundation1_UnlimitBuffer, 0x5)
{
	GET_STACK(CellStruct const*, Foundation, 0x18);
	GET(DisplayClass *, Display, EBX);

	DWORD Len = BuildingExt::FoundationLength(Foundation);

	BuildingExt::TempFoundationData1.assign(Foundation, Foundation + Len);

	Display->CurrentFoundation_Data = BuildingExt::TempFoundationData1.data();

	auto const bounds = Display->FoundationBoundsSize(
		BuildingExt::TempFoundationData1.data());

	R->Stack<CellStruct>(0x18, bounds);
	R->EAX<CellStruct *>(R->lea_Stack<CellStruct *>(0x18));

	return 0x4A8C9E;
}

DEFINE_HOOK(0x4A8DD7, DisplayClass_ProcessFoundation2_UnlimitBuffer, 0x5)
{
	GET_STACK(CellStruct const*, Foundation, 0x18);
	GET(DisplayClass *, Display, EBX);

	DWORD Len = BuildingExt::FoundationLength(Foundation);

	BuildingExt::TempFoundationData2.assign(Foundation, Foundation + Len);

	Display->CurrentFoundationCopy_Data = BuildingExt::TempFoundationData2.data();

	auto const bounds = Display->FoundationBoundsSize(
		BuildingExt::TempFoundationData2.data());

	R->Stack<CellStruct>(0x18, bounds);
	R->EAX<CellStruct *>(R->lea_Stack<CellStruct *>(0x18));

	return 0x4A8DFE;
}

static void GetFoundationBounds(
	CellStruct const* const pFoundation, CellStruct& origin, CellStruct& size)
{
	int left = 0, top = 0, right = 0, bottom = 0;

	if(pFoundation->X != 0x7FFF || pFoundation->Y != 0x7FFF) {
		left = 512;
		top = 512;
		right = -512;
		bottom = -512;

		for(auto pCell = pFoundation; pCell->X != 0x7FFF || pCell->Y != 0x7FFF; ++pCell) {
			if(pCell->X < left) {
				left = pCell->X;
			}
			if(pCell->X > right) {
				right = pCell->X;
			}
			if(pCell->Y < top) {
				top = pCell->Y;
			}
			if(pCell->Y > bottom) {
				bottom = pCell->Y;
			}
		}
	}

	origin.X = static_cast<short>(left);
	origin.Y = static_cast<short>(top);
	size.X = static_cast<short>(right >= left ? right - left + 1 : 1);
	size.Y = static_cast<short>(bottom >= top ? bottom - top + 1 : 1);
}

DEFINE_HOOK_AGAIN(0x6D5573, sub_6D5030_CustomFoundation, 0x6)
DEFINE_HOOK(0x6D50FB, sub_6D5030_CustomFoundation, 0x5)
{
	auto const primary = (R->Origin() == 0x6D50FBu);

	auto const pFoundation = primary
		? MouseClass::Instance.CurrentFoundation_Data
		: MouseClass::Instance.CurrentFoundationCopy_Data;

	CellStruct origin, size;
	GetFoundationBounds(pFoundation, origin, size);

	R->Stack(0x14, origin);
	R->Stack(0x18, size);
	R->EAX(*reinterpret_cast<DWORD const*>(&size));
	R->ESI(static_cast<int>(origin.Y));

	return primary ? 0x6D5116u : 0x6D558Fu;
}
