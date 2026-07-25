#include "Body.h"

#include <AnimClass.h>
#include <OverlayTypeClass.h>
#include <MouseClass.h>
#include <ScenarioClass.h>
#include <Helpers\Macro.h>

// this was only a leftover stub from TS. reimplemented
// using the same mechanism.
DEFINE_HOOK(0x489270, CellChainReact, 0x5)
{
	GET(CellStruct*, cell, ECX);

	static const int reactChanceMultiplier = 5;
	static const int spreadChance = 80;
	static const int minDelay = 15;
	static const int maxDelay = 120;

	auto pCell = MapClass::Instance.GetCellAt(*cell);
	auto idxTib = pCell->GetContainedTiberiumIndex();

	TiberiumClass* pTib = TiberiumClass::Array.GetItemOrDefault(idxTib);
	OverlayTypeClass* pOverlay = OverlayTypeClass::Array.GetItemOrDefault(pCell->OverlayTypeIndex);

	if(pTib && pOverlay && pOverlay->ChainReaction && pCell->OverlayData > 1) {
		CoordStruct crd = pCell->GetCoords();

		if(ScenarioClass::Instance->Random.RandomRanged(0, 99) < reactChanceMultiplier * pCell->OverlayData) {
			bool wasFullGrown = (pCell->OverlayData >= 11);

			unsigned char delta = pCell->OverlayData / 2;
			int damage = pTib->Power * delta;

			// remove some of the tiberium
			pCell->OverlayData -= delta;
			pCell->MarkForRedraw();

			// get the warhead
			auto pExt = TiberiumExt::ExtMap.Find(pTib);
			auto pWarhead = pExt->GetExplosionWarhead();

			// create an explosion
			if(auto pType = MapClass::SelectDamageAnimation(4 * damage, pWarhead, pCell->LandType, crd)) {
				GameCreate<AnimClass>(pType, crd, 0, 1, 0x600, 0);
			}

			// damage the area, without affecting tiberium
			MapClass::DamageArea(crd, damage, nullptr, pWarhead, false, nullptr);

			// spawn some animation on the neighbour cells
			if(auto pType = AnimTypeClass::Find("INVISO")) {
				for(size_t i = 0; i<8; ++i) {
					auto pNeighbour = pCell->GetNeighbourCell(static_cast<FacingType>(i));

					if(pCell->GetContainedTiberiumIndex() != -1 && pNeighbour->OverlayData > 2) {
						if(ScenarioClass::Instance->Random.RandomRanged(0, 99) < spreadChance) {
							int delay = ScenarioClass::Instance->Random.RandomRanged(minDelay, maxDelay);
							crd = pNeighbour->GetCoords();

							GameCreate<AnimClass>(pType, crd, delay, 1, 0x600, 0);
						}
					}
				}
			}

			if(wasFullGrown) {
				pTib->RegisterForGrowth(cell);
			}
		}
	}

	return 0;
}

// hook up the area damage delivery with chain reactions
DEFINE_HOOK(0x48964F, DamageArea_ChainReaction, 0x5)
{
	GET(CellClass*, pCell, EBX);
	pCell->ChainReaction();
	return 0;
}

DEFINE_HOOK(0x424DD3, AnimClass_ReInit_TiberiumChainReaction_Chance, 0x6)
{
	GET(TiberiumClass*, pTib, EDI);
	auto pExt = TiberiumExt::ExtMap.Find(pTib);

	bool react = ScenarioClass::Instance->Random.RandomRanged(0, 99) < pExt->GetDebrisChance();
	return react ? 0x424DF9 : 0x424E9B;
}

DEFINE_HOOK(0x424EC5, AnimClass_ReInit_TiberiumChainReaction_Damage, 0x6)
{
	GET(TiberiumClass*, pTib, EDI);
	auto pExt = TiberiumExt::ExtMap.Find(pTib);

	R->Stack(0x0, pExt->GetExplosionWarhead());
	R->EDX(pExt->GetExplosionDamage());

	return 0x424ECB;
}
