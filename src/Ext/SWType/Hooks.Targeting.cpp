#include "Body.h"

#include "../Building/Body.h"
#include "../House/Body.h"
#include "../Techno/Body.h"
#include "../../Misc/SWTypes.h"

#include <CellSpread.h>
#include <FactoryClass.h>
#include <Helpers/Enumerators.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <SuperClass.h>
#include <ScenarioClass.h>

#include <DiscreteDistributionClass.h>
#include <DiscreteSelectionClass.h>
#include <EventClass.h>

#pragma region Support

enum class SWTargetFlags {
	DisallowEmpty,
	AllowEmpty
};

struct TargetResult {
	CellStruct Target;
	SWTargetFlags Flags;
};

struct TargetingInfo {
	TargetingInfo(SuperClass* pSuper) :
		Super(pSuper),
		Owner(pSuper->Owner),
		TypeExt(SWTypeExt::ExtMap.Find(pSuper->Type)),
		NewType(TypeExt->GetNewSWType()),
		Data(NewType->GetTargetingData(TypeExt, Owner))
	{ }

	bool CanFireAt(CellStruct cell) const {
		return this->NewType->CanFireAt(this->Data, cell, false);
	}

	SuperClass* Super;
	HouseClass* Owner;
	SWTypeExt::ExtData* TypeExt;
	NewSWType* NewType;
	TargetingData Data;
};

CellStruct ConvertToCell(const CellStruct& result) {
	return result;
}

template <typename T>
CellStruct ConvertToCell(const T& result) {
	if(result) {
		return result->GetMapCoords();
	} else {
		return CellStruct::Empty;
	}
}

// delegate finding a target, convert to cell
template<typename Selector>
CellStruct GetTarget(const TargetingInfo& info, Selector selector) {
	return ConvertToCell(selector(info));
}

template<typename It, typename Valuator>
ObjectClass* GetTargetFirstMax(It first, It last, Valuator value) {
	ObjectClass* pTarget = nullptr;
	int maxValue = 0;

	for(auto it = first; it < last; ++it) {
		if(auto pItem = *it) {
			auto curValue = value(pItem, maxValue);

			if(curValue > maxValue) {
				pTarget = pItem;
				maxValue = curValue;
			}
		}
	}

	return pTarget;
}

template<typename It, typename Valuator>
ObjectClass* GetTargetAnyMax(It first, It last, Valuator value) {
	DiscreteSelectionClass<ObjectClass*> targets;

	for(auto it = first; it < last; ++it) {
		if(auto pItem = *it) {
			auto curValue = value(pItem, targets.GetRating());
			targets.Add(pItem, curValue);
		}
	}

	return targets.Select(ScenarioClass::Instance->Random);
}

template<typename It, typename Valuator>
ObjectClass* GetTargetShareAny(It first, It last, Valuator value) {
	DiscreteDistributionClass<ObjectClass*> targets;

	for(auto it = first; it < last; ++it) {
		if(auto pItem = *it) {
			auto curValue = value(pItem);
			targets.Add(pItem, curValue);
		}
	}

	return targets.Select(ScenarioClass::Instance->Random);
}

#pragma endregion

#pragma region Target picking functors

struct PickEmptyTarget {
	CellStruct operator()(const TargetingInfo& info) const {
		return CellStruct::Empty;
	}
};

struct PickIonCannonTarget {
	enum class CloakHandling {
		RandomizeCloaked = 0,
		AgnosticToCloak = 1,
		IgnoreCloaked = 2,
		RequireCloaked = 3
	};

	explicit PickIonCannonTarget(
		HouseClass* pEnemy, CloakHandling cloak = CloakHandling::IgnoreCloaked)
		: Enemy(pEnemy), Cloak(cloak)
	{ }

	CellStruct operator()(const TargetingInfo& info) const {
		auto const pOwner = info.Owner;
		auto const pEnemy = this->Enemy;

		const auto it = info.TypeExt->GetPotentialAITargets(pEnemy);

		auto const pResult = GetTargetAnyMax(it.begin(), it.end(), [=, &info](TechnoClass* pTechno, int curMax) {
			// original game code only compares owner and doesn't support nullptr
			auto const passedFilter = (!pEnemy || pTechno->Owner == pEnemy);

			if(passedFilter && pOwner->IsIonCannonEligibleTarget(pTechno)) {
				auto const cell = pTechno->GetMapCoords();
				if(!MapClass::Instance.IsWithinUsableArea(cell, true)) {
					return -1;
				}

				auto value = pTechno->GetIonCannonValue(pOwner->AIDifficulty);

				// cloak options
				if(this->Cloak != CloakHandling::AgnosticToCloak) {
					bool cloaked = TechnoExt::IsCloaked(pTechno);

					if(this->Cloak == CloakHandling::RandomizeCloaked) {
						// original behavior
						if(cloaked) {
							value = ScenarioClass::Instance->Random.RandomRanged(0, curMax + 10);
						}
					} else if(this->Cloak == CloakHandling::IgnoreCloaked) {
						// this prevents the 'targeting cloaked units bug'
						if(cloaked) {
							return -1;
						}
					} else if(this->Cloak == CloakHandling::RequireCloaked) {
						if(!cloaked) {
							return -1;
						}
					}
				}

				// do not do heavy lifting on objects that
				// would not be chosen anyhow
				if(value >= curMax && info.CanFireAt(cell)) {
					return value;
				}
			}

			return -1;
		});

		if(pResult) {
			return pResult->GetMapCoords();
		}

		return CellStruct::Empty;
	}

private:
	HouseClass* Enemy;
	CloakHandling Cloak;
};

struct PickPreferredTypeOrIonCannon {
	CellStruct operator()(const TargetingInfo& info) const {
		auto pOwner = info.Owner;
		auto type = pOwner->PreferredTargetType;
		if(type == QuarryType::Anything) {
			auto const pEnemy = HouseClass::Array.GetItemOrDefault(
				info.Owner->EnemyHouseIndex);
			return PickIonCannonTarget(pEnemy)(info);
		} else {
			return pOwner->PickTargetByType(type);
		}
	}
};

#pragma endregion

#pragma region Target Selectors

struct TargetSelector {
	//virtual TargetResult operator()(const TargetingInfo& info) const = 0;
	//virtual ~TargetSelector() = default;
};

struct NuclearMissileTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, PickPreferredTypeOrIonCannon()),
			SWTargetFlags::DisallowEmpty};
	}
};

struct LightningStormTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, PickPreferredTypeOrIonCannon()),
			SWTargetFlags::DisallowEmpty};
	}
};

struct PsychicDominatorTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetItem), SWTargetFlags::DisallowEmpty};
	}

private:
	static ObjectClass* FindTargetItem(const TargetingInfo& info) {
		auto it = info.TypeExt->GetPotentialAITargets();
		return GetTargetFirstMax(it.begin(), it.end(), [&info](TechnoClass* pTechno, int curMax) {
			if(pTechno->InLimbo) {
				return -1;
			}

			auto cell = pTechno->GetCell()->MapCoords;

			int value = 0;
			for(size_t i = 0; i < CellSpread::NumCells(3); ++i) {
				auto pCell = MapClass::Instance.GetCellAt(cell + CellSpread::GetCell(i));

				for(NextObject j(pCell->FirstObject); j && abstract_cast<FootClass*>(*j); ++j) {
					auto pFoot = static_cast<FootClass*>(*j);
					if(!info.Owner->IsAlliedWith(pFoot) && !pFoot->IsInAir() && pFoot->CanBePermaMindControlled()) {
						// original game does not consider cloak
						if(pFoot->CloakState != CloakState::Cloaked) {
							++value;
						}
					}
				}
			}

			// new check
			if(value <= curMax || !info.CanFireAt(cell)) {
				return -1;
			}

			return value;
		});
	}
};

struct GeneticMutatorTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetItem), SWTargetFlags::DisallowEmpty};
	}

private:
	static ObjectClass* FindTargetItem(const TargetingInfo& info) {
		auto it = info.TypeExt->GetPotentialAITargets();
		return GetTargetFirstMax(it.begin(), it.end(), [&info](TechnoClass* pTechno, int curMax) {
			if(pTechno->InLimbo) {
				return -1;
			}

			auto cell = pTechno->GetCell()->MapCoords;

			int value = 0;
			for(size_t i = 0; i < CellSpread::NumCells(1); ++i) {
				auto pCell = MapClass::Instance.GetCellAt(cell + CellSpread::GetCell(i));

				for(NextObject j(pCell->GetInfantry(pTechno->OnBridge)); j && abstract_cast<InfantryClass*>(*j); ++j) {
					auto pInf = static_cast<InfantryClass*>(*j);
					if(!info.Owner->IsAlliedWith(pInf) && !pInf->IsInAir()) {
						// original game does not consider cloak
						if(pInf->CloakState != CloakState::Cloaked) {
							++value;
						}
					}
				}
			}

			// new check
			if(value <= curMax || !info.CanFireAt(cell)) {
				return -1;
			}

			return value;
		});
	}
};

struct ParaDropTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetCoords), SWTargetFlags::DisallowEmpty};
	}

private:
	static CellStruct FindTargetCoords(const TargetingInfo& info) {
		auto pOwner = info.Owner;

		static const int SpaceSize = 5;

		auto target = CellStruct::Empty;
		if(pOwner->PreferredTargetType == QuarryType::Anything) {
			// if no enemy yet, reinforce own base
			auto pTargetPlayer = HouseClass::Array.GetItemOrDefault(pOwner->EnemyHouseIndex, info.Owner);

			target = MapClass::Instance.NearByLocation(
				pTargetPlayer->GetBaseCenter(), SpeedType::Foot, -1,
				MovementZone::Normal, false, SpaceSize, SpaceSize, false,
				false, false, true, CellStruct::Empty, false, false);

			if(target != CellStruct::Empty) {
				target += CellStruct{SpaceSize / 2, SpaceSize / 2};
			}

		} else {
			target = pOwner->PickTargetByType(pOwner->PreferredTargetType);
		}

		// new check
		if(!info.CanFireAt(target)) {
			return CellStruct::Empty;
		}

		return target;
	}
};

struct ForceShieldTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetCoords), SWTargetFlags::DisallowEmpty};
	}

private:
	static CellStruct FindTargetCoords(const TargetingInfo& info) {
		auto pOwner = info.Owner;

		if(pOwner->PreferredDefensiveCell != CellStruct::Empty) {
			if(RulesClass::Instance->AISuperDefenseFrames + pOwner->PreferredDefensiveCellStartTime > Unsorted::CurrentFrame) {
				// new check
				if(info.CanFireAt(pOwner->PreferredDefensiveCell)) {
					return pOwner->PreferredDefensiveCell;
				}
			}
		}

		return CellStruct::Empty;
	}
};

struct NoTargetTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{CellStruct::Empty, SWTargetFlags::AllowEmpty};
	}
};

struct BaseTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		// fire at the SW's owner's base cell
		auto cell = info.Owner->GetBaseCenter();

		// new check
		if(!info.CanFireAt(cell)) {
			cell = CellStruct::Empty;
		}

		return{cell, SWTargetFlags::DisallowEmpty};
	}
};

struct EnemyBaseTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		// fire at the owner's enemy base cell
		return{GetTarget(info, FindTargetCoords), SWTargetFlags::DisallowEmpty};
	}

	static CellStruct FindTargetCoords(const TargetingInfo& info) {
		if(auto pEnemy = HouseClass::Array.GetItemOrDefault(info.Owner->EnemyHouseIndex)) {
			auto cell = pEnemy->GetBaseCenter();

			if(info.CanFireAt(cell)) {
				return cell;
			}
		}
		return CellStruct::Empty;
	}
};

struct OffensiveTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		auto const pEnemy = HouseClass::Array.GetItemOrDefault(info.Owner->EnemyHouseIndex);
		return{GetTarget(info, PickIonCannonTarget(pEnemy)), SWTargetFlags::DisallowEmpty};
	}
};

struct StealthTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info,
			PickIonCannonTarget(nullptr, PickIonCannonTarget::CloakHandling::RequireCloaked)),
			SWTargetFlags::DisallowEmpty};
	}
};

struct SelfTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetCoords), SWTargetFlags::DisallowEmpty};
	}

private:
	static CellStruct FindTargetCoords(const TargetingInfo& info) {
		// find the first building providing super
		auto index = info.Super->Type->ArrayIndex;
		const auto& buildings = info.Owner->Buildings;

		// Ares < 0.9 didn't check power
		auto it = std::find_if(buildings.begin(), buildings.end(), [index, &info](BuildingClass* pBld) {
			auto const pExt = BuildingExt::ExtMap.Find(pBld);
			if(pExt->HasSuperWeapon(index, true) && pBld->IsPowerOnline()) {
				auto cell = pBld->GetMapCoords();

				if(info.CanFireAt(cell)) {
					return true;
				}
			}
			return false;
		});

		if(it != buildings.end()) {
			auto pBld = *it;
			auto Offset = CellStruct{pBld->Type->GetFoundationWidth() / 2,
				pBld->Type->GetFoundationHeight(false) / 2};
			return pBld->GetCell()->MapCoords + Offset;
		}

		return CellStruct::Empty;
	}
};

struct MultiMissileTargetSelector final : public TargetSelector {
	// classic TS code: fire at the enemy's most threatening building
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetItem), SWTargetFlags::DisallowEmpty};
	}

private:
	static ObjectClass* FindTargetItem(const TargetingInfo& info) {
		auto pOwner = info.Owner;
		auto pTargetPlayer = HouseClass::Array.GetItem(pOwner->EnemyHouseIndex);

		auto it = info.TypeExt->GetPotentialAITargets(pTargetPlayer);
		return GetTargetFirstMax(it.begin(), it.end(), [pOwner, &info](TechnoClass* pTechno, int curMax) {
			auto cell = pTechno->GetMapCoords();

			auto const value = TechnoExt::IsCloaked(pTechno)
				? ScenarioClass::Instance->Random.RandomRanged(0, 100)
				: MapClass::Instance.GetThreatPosed(cell, pOwner);

			if(value <= curMax || !info.CanFireAt(cell)) {
				return -1;
			}

			return value;
		});
	}
};

struct DropPodTargetSelector final : public TargetSelector {
	TargetResult operator()(const TargetingInfo& info) const {
		return{GetTarget(info, FindTargetCoords), SWTargetFlags::DisallowEmpty};
	}

private:
	static CellStruct FindTargetCoords(const TargetingInfo& info) {
		auto const zone = ScenarioClass::Instance->Random.RandomRanged(1, 4);
		auto const origin = info.Owner->PickRandomCellInZone(zone);

		auto const target = MapClass::Instance.NearByLocation(
			origin, SpeedType::Foot, -1, MovementZone::Normal, false, 1, 1, false,
			false, false, true, CellStruct::Empty, false, false);

		if(target == CellStruct::Empty || !info.CanFireAt(target)) {
			return CellStruct::Empty;
		}

		return target;
	}
};

struct LightningRandomTargetSelector final : public TargetSelector {
	// keep rolling cells until one of them is a valid target
	TargetResult operator()(const TargetingInfo& info) const {
		auto const& bounds = MapClass::Instance.MapCoordBounds;

		for(auto i = 0; i < 5; ++i) {
			auto cell = CellStruct::Empty;

			while(!MapClass::Instance.CoordinatesLegal(cell)) {
				auto& random = ScenarioClass::Instance->Random;
				cell.X = static_cast<short>(random.RandomRanged(0, bounds.Right));
				cell.Y = static_cast<short>(random.RandomRanged(0, bounds.Bottom));
			}

			if(info.CanFireAt(cell)) {
				return{cell, SWTargetFlags::AllowEmpty};
			}
		}

		return{CellStruct::Empty, SWTargetFlags::DisallowEmpty};
	}
};

#pragma endregion

TargetResult PickSuperWeaponTarget(const TargetingInfo& info) {
	// the preferred cell overrides whatever the mode would pick
	switch(info.TypeExt->GetAITargetingPreference()) {
	case SuperWeaponAITargetingPreference::Offensive:
		if(info.Owner->PreferredTargetCell != CellStruct::Empty) {
			return{info.Owner->PreferredTargetCell, SWTargetFlags::AllowEmpty};
		}
		break;
	case SuperWeaponAITargetingPreference::Defensive:
		if(info.Owner->PreferredDefensiveCell2 != CellStruct::Empty) {
			return{info.Owner->PreferredDefensiveCell2, SWTargetFlags::AllowEmpty};
		}
		break;
	}

	// all the different AI targeting modes
	switch(info.TypeExt->SW_AITargetingType) {
	case SuperWeaponAITargetingMode::Nuke:
		return NuclearMissileTargetSelector()(info);
	case SuperWeaponAITargetingMode::LightningStorm:
		return LightningStormTargetSelector()(info);
	case SuperWeaponAITargetingMode::PsychicDominator:
		return PsychicDominatorTargetSelector()(info);
	case SuperWeaponAITargetingMode::ParaDrop:
		return ParaDropTargetSelector()(info);
	case SuperWeaponAITargetingMode::GeneticMutator:
		return GeneticMutatorTargetSelector()(info);
	case SuperWeaponAITargetingMode::ForceShield:
		return ForceShieldTargetSelector()(info);
	case SuperWeaponAITargetingMode::NoTarget:
	case SuperWeaponAITargetingMode::HunterSeeker:
	case SuperWeaponAITargetingMode::Attack:
	case SuperWeaponAITargetingMode::LowPower:
	case SuperWeaponAITargetingMode::LowPowerAttack:
		return NoTargetTargetSelector()(info);
	case SuperWeaponAITargetingMode::Offensive:
		return OffensiveTargetSelector()(info);
	case SuperWeaponAITargetingMode::Stealth:
		return StealthTargetSelector()(info);
	case SuperWeaponAITargetingMode::Self:
		return SelfTargetSelector()(info);
	case SuperWeaponAITargetingMode::Base:
		return BaseTargetSelector()(info);
	case SuperWeaponAITargetingMode::MultiMissile:
		return MultiMissileTargetSelector()(info);
	case SuperWeaponAITargetingMode::EnemyBase:
		return EnemyBaseTargetSelector()(info);
	case SuperWeaponAITargetingMode::DropPod:
		return DropPodTargetSelector()(info);
	case SuperWeaponAITargetingMode::LightningRandom:
		return LightningRandomTargetSelector()(info);
	case SuperWeaponAITargetingMode::None:
	case SuperWeaponAITargetingMode::IronCurtain:
	default:
		return{CellStruct::Empty, SWTargetFlags::DisallowEmpty};
	}
}

bool SWTypeExt::TryFire(SuperClass* const pSuper, bool const manual) {
	auto const pOwner = pSuper->Owner;
	auto const pType = pSuper->Type;
	auto const pExt = SWTypeExt::ExtMap.Find(pType);

	// don't try to fire if we obviously haven't enough money
	if(!pOwner->CanTransactMoney(pExt->Money_Amount)) {
		return false;
	}

	if(!pExt->MeetsAITargetingConstraints(pOwner, manual)) {
		return false;
	}

	if(pExt->GetNewSWType()) {
		TargetingInfo info(pSuper);

		auto const result = PickSuperWeaponTarget(info);
		if(result.Target != CellStruct::Empty || result.Flags == SWTargetFlags::AllowEmpty) {
			return pOwner->Fire_SW(pType->ArrayIndex, result.Target);
		}
	}

	return false;
}

DEFINE_HOOK(0x5098F0, HouseClass_Update_AI_TryFireSW, 0x5) {
	GET(HouseClass*, pThis, ECX);

	// this method iterates over every available SW and checks
	// whether it should be fired automatically. the original
	// method would abort if this house is human-controlled.
	bool isHuman = pThis->IsControlledByHuman();

	for(const auto pSuper : pThis->Supers) {
		// a draining SW is already active and must not be relaunched
		if(pSuper->IsReady && pSuper->ChargeDrainState != ChargeDrainState::Draining) {
			auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);

			// fire if this is AI owned or the SW has auto fire set.
			if(!isHuman || pExt->SW_AutoFire) {
				SWTypeExt::TryFire(pSuper, false);
			}
		}
	}

	return 0x509AE7;
}

// SW.UseAITargeting makes a manually fired SW pick its own target
DEFINE_HOOK(0x4C78D6, Networking_RespondToEvent_SpecialPlace, 0x8) {
	GET(EventClass* const, pEvent, ESI);
	GET(HouseClass* const, pThis, EDI);

	auto const idxSW = pEvent->SpecialPlace.ID;
	auto const& cell = pEvent->SpecialPlace.Location;

	auto const pSuper = pThis->Supers.GetItem(idxSW);
	auto const pExt = SWTypeExt::ExtMap.Find(pSuper->Type);

	if(pExt->SW_UseAITargeting) {
		if(!SWTypeExt::TryFire(pSuper, true) && pThis == HouseClass::CurrentPlayer) {
			pExt->PrintMessage(pExt->Message_CannotFire, pThis);
		}
	} else {
		pThis->Fire_SW(idxSW, cell);
	}

	return 0x4C78F8;
}
