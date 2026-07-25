#include "Body.h"

#include <CellClass.h>
#include <FileSystem.h>
#include <MapClass.h>
#include <ParticleClass.h>
#include <ParticleSystemClass.h>
#include <TacticalClass.h>

#include <cmath>

namespace
{
	static const size_t MaxDamagedObjects = 256;

	struct ObjectCollector {
		ObjectClass* Items[MaxDamagedObjects];
		int Count;
		CoordStruct Center;
		double RangeSquared;

		bool Contains(ObjectClass* pObject) const {
			for(auto index = 0; index < this->Count; ++index) {
				if(this->Items[index] == pObject) {
					return true;
				}
			}
			return false;
		}

		void Collect(ObjectClass* pObject) {
			auto const deltaX = static_cast<double>(pObject->Location.X - this->Center.X);
			auto const deltaY = static_cast<double>(pObject->Location.Y - this->Center.Y);
			auto const deltaZ = static_cast<double>(pObject->Location.Z - this->Center.Z);

			if(this->RangeSquared < deltaZ * deltaZ + deltaY * deltaY + deltaX * deltaX) {
				return;
			}

			if(!this->Contains(pObject) && this->Count < static_cast<int>(MaxDamagedObjects)) {
				this->Items[this->Count++] = pObject;
			}
		}
	};

	// walks outward ring by ring and picks up everything the gas reaches
	void CollectObjectsInRange(CellStruct const center, unsigned int cells, ObjectCollector& collector)
	{
		auto const maxRadius = cells > 256u ? 256u : cells;

		short x = 0;
		short y = 0;
		unsigned int radius = 0;
		unsigned short previous = 0;
		bool advancing = true;
		bool wrapped = true;

		for(;;) {
			previous = static_cast<unsigned short>(x);

			auto const cellX = static_cast<short>(x + center.X);
			auto const cellY = static_cast<short>(y + center.Y);
			auto const index = (static_cast<int>(cellY) << 9) + static_cast<int>(cellX);

			if(static_cast<unsigned int>(index) <= 0x3FFFFu) {
				CellStruct const cell = { cellX, cellY };

				if(auto const pCell = MapClass::Instance.TryGetCellAt(cell)) {
					auto pObject = (static_cast<bool>(pCell->Flags & CellFlags::BridgeHead))
						? pCell->AltObject : pCell->FirstObject;

					while(pObject) {
						auto const pNext = pObject->NextObject;
						collector.Collect(pObject);
						pObject = pNext;
					}
				}
			}

			if(radius > maxRadius) {
				break;
			}

			if((!x && !y) || (x == 1 && y == static_cast<short>(radius))) {
				++radius;
				x = radius ? -1 : 0;
				y = static_cast<short>(-static_cast<int>(radius));
				advancing = true;
				wrapped = true;
			} else if(advancing) {
				x = static_cast<short>(x + 1);
				advancing = !x;
			} else if(x < 0) {
				x = static_cast<short>(-static_cast<int>(previous));
				advancing = wrapped;
			} else {
				auto const next = static_cast<short>(y + 1);
				previous = static_cast<unsigned short>(next);

				auto const step = static_cast<short>(next <= 0 ? next : -next);
				auto const raw = ~static_cast<int>(2 * (static_cast<int>(radius)
					+ static_cast<unsigned short>(step)));

				x = static_cast<short>(raw);

				auto const decision = static_cast<int>(static_cast<short>(raw))
					- static_cast<int>(step);
				advancing = decision >= 0;

				if(decision < 0) {
					if(decision == -1) {
						x = static_cast<short>(raw + 1);
					} else {
						x = static_cast<short>(-static_cast<int>(radius)
							- static_cast<int>(step) / 2);
					}
				}

				wrapped = advancing;
				y = next > 0 ? static_cast<short>(-static_cast<int>(step)) : step;
			}

			if(radius > maxRadius) {
				break;
			}
		}
	}
}

DEFINE_HOOK(0x62D015, ParticleClass_Draw_Palette, 0x6)
{
	GET(ParticleClass*, pThis, EDI);

	auto pConvert = ParticleTypeExt::ExtMap.Find(pThis->Type)->Palette.Convert.get();
	if(!pConvert) {
		pConvert = FileSystem::ANIM_PAL;
	}

	R->EDX(pConvert);

	return 0x62D01B;
}

DEFINE_HOOK(0x62C23D, ParticleClass_Update_Gas_DamageRange, 0x6)
{
	GET(ParticleClass*, pThis, EBP);

	auto const pType = pThis->Type;
	auto const range = ParticleTypeExt::ExtMap.Find(pType)->DamageRange.Get();

	if(range <= 0.0) {
		return 0;
	}

	ObjectCollector collector;
	collector.Count = 0;
	collector.Center = pThis->Location;
	collector.RangeSquared = (range * 256.0) * (range * 256.0);

	CellStruct cell;
	pThis->GetMapCoords(&cell);

	CollectObjectsInRange(cell, static_cast<unsigned int>(std::ceil(range)), collector);

	auto const pSystem = pThis->ParticleSystem;
	auto const damage = pType->Damage;
	auto const pWarhead = pType->Warhead;
	auto const pHouse = pSystem ? pSystem->OwnerHouse : nullptr;

	for(auto index = 0; index < collector.Count; ++index) {
		auto const pObject = collector.Items[index];

		if(pObject->IsAlive && pObject->Health > 0) {
			auto const distance = TacticalClass::AdjustForZ(
				std::abs(collector.Center.X - pObject->Location.X)
				+ std::abs(collector.Center.Y - pObject->Location.Y));

			auto value = damage;
			pObject->ReceiveDamage(&value, distance, pWarhead, nullptr, false, false, pHouse);
		}
	}

	return 0x62C313;
}
