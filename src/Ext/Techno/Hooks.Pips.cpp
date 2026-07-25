#include "Body.h"
#include "../BuildingType/Body.h"
#include "../House/Body.h"
#include "../TechnoType/Body.h"
#include "../../Enum/TunnelTypes.h"

#include <BuildingClass.h>
#include <FileSystem.h>
#include <HouseClass.h>
#include <InfantryTypeClass.h>
#include <Surface.h>

#include <algorithm>
#include <vector>

namespace {
	// the frames to draw, one per pip slot
	std::vector<int> PipFrames;

	struct PipState {
		size_t Count;
		int GapAfter;
	};

	HouseExt::TunnelData* FindTunnel(BuildingTypeClass* const pType, HouseClass* const pHouse)
	{
		auto const index = static_cast<size_t>(
			BuildingTypeExt::ExtMap.Find(pType)->Tunnel);

		if(index >= TunnelTypeClass::Array.size()) {
			return nullptr;
		}

		return HouseExt::ExtMap.Find(pHouse)->FindTunnel(index);
	}

	// walks the passenger chain back to front, giving every passenger one pip of
	// its own kind plus as many filler pips as it takes up space
	int AssignPassengerPips(FootClass* const pPassenger, int const basePip,
		bool const bySize, int const maxPips, int& index)
	{
		auto const pType = pPassenger->GetTechnoType();
		auto const what = pPassenger->WhatAmI();

		auto pip = static_cast<int>(PipIndex::Green);
		if(what == AbstractType::Infantry) {
			pip = static_cast<int>(static_cast<InfantryTypeClass*>(pType)->Pip);
		} else if(what == AbstractType::Unit) {
			pip = basePip;
		}

		auto const size = bySize ? static_cast<int>(pType->Size) : 1;

		auto ret = size;
		if(auto const pNext = static_cast<FootClass*>(pPassenger->NextObject)) {
			ret = AssignPassengerPips(pNext, basePip, bySize, maxPips, index);
		}

		auto room = maxPips - index;
		if(room > 0) {
			if(room >= size) {
				room = size;
			}

			if(room > 0) {
				PipFrames[static_cast<size_t>(index++)] = pip;

				for(auto rest = room - 1; rest > 0; --rest) {
					PipFrames[static_cast<size_t>(index++)] = static_cast<int>(PipIndex::White);
				}
			}
		}

		return ret;
	}

	PipState GetPassengerPips(TechnoClass* const pThis)
	{
		auto const pType = pThis->GetTechnoType();
		auto const maxPips = std::max(0, pType->GetPipMax());

		auto basePip = static_cast<int>(PipIndex::Blue);
		auto sized = 1;

		if(pThis->WhatAmI() == AbstractType::Building) {
			auto const pBuildingType = static_cast<BuildingClass*>(pThis)->Type;

			if(pBuildingType->UnitAbsorb || pBuildingType->InfantryAbsorb) {
				basePip = static_cast<int>(PipIndex::Red);
				sized = 0;
			}

			if(auto const pTunnel = FindTunnel(pBuildingType, pThis->Owner)) {
				auto const cap = std::min(pTunnel->MaxCap, maxPips);
				PipFrames.assign(static_cast<size_t>(std::max(0, cap)),
					static_cast<int>(PipIndex::Empty));

				auto const count = std::min(static_cast<size_t>(std::max(0, cap)),
					pTunnel->Passengers.size());

				for(size_t i = 0; i < count; ++i) {
					auto const pPassenger = pTunnel->Passengers[i];

					auto pip = static_cast<int>(PipIndex::Red);
					if(pPassenger && pPassenger->WhatAmI() == AbstractType::Infantry) {
						pip = static_cast<int>(
							static_cast<InfantryTypeClass*>(pPassenger->GetTechnoType())->Pip);
					}

					PipFrames[i] = pip;
				}

				return { PipFrames.size(), 0 };
			}
		}

		auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

		PipFrames.assign(static_cast<size_t>(maxPips), static_cast<int>(PipIndex::Empty));

		if(auto const pPassenger = pThis->Passengers.GetFirstPassenger()) {
			auto index = 0;
			AssignPassengerPips(pPassenger, basePip,
				pExt->Passengers_BySize && sized, maxPips, index);
		}

		return { PipFrames.size(), pType->Gunner ? 1 : 0 };
	}
}

DEFINE_HOOK(0x709D38, TechnoClass_DrawPipscale_Passengers, 0x7)
{
	GET(TechnoTypeClass* const, pType, EAX);

	if(pType->PipScale != PipScale::Passengers) {
		return 0x70A083;
	}

	GET(TechnoClass* const, pThis, EBP);
	GET(int const, deltaY, ESI);

	GET_STACK(SHPStruct* const, pShapes, 0x1C);
	GET_STACK(RectangleStruct* const, pBounds, 0x80);
	GET_STACK(int const, deltaX, 0x58);
	GET_STACK(int const, baseX, 0x50);
	GET_STACK(int const, baseY, 0x54);

	auto const state = GetPassengerPips(pThis);

	Point2D position = { baseX, baseY };

	for(size_t i = 0; i < state.Count; ++i) {
		DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pShapes, PipFrames[i],
			&position, pBounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);

		position.X += deltaX;
		position.Y += deltaY;

		// the gunner's pip is set apart from the passengers'
		if(static_cast<int>(i + 1) == state.GapAfter) {
			position.X += deltaX;
			position.Y += deltaY;
		}
	}

	return 0x70A4EC;
}
