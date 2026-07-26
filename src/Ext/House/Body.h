#pragma once

#include "../_Container.hpp"
#include "../../Enum/Prerequisites.h"
#include "../../Utilities/Constructs.h"
#include "../../Utilities/Enums.h"
#include "../../Utilities/Iterator.h"
#include "../../Utilities/Template.h"

#include <Helpers/Template.h>

#include <HouseClass.h>

#include <bitset>

class BuildingTypeClass;
class FactoryClass;
class FootClass;
class SuperClass;
class SideClass;
class SuperWeaponTypeClass;
class TunnelTypeClass;

class HouseExt
{
public:
	using base_type = HouseClass;

	// the passengers currently inside one tunnel network of this house
	struct TunnelData {
		std::vector<FootClass*> Passengers;
		int MaxCap{ 0 };

		TunnelData() = default;

		explicit TunnelData(TunnelTypeClass const* pType);

		void RemovePassenger(void* ptr);

		bool Load(AresStreamReader &Stm, bool RegisterForChange);

		bool Save(AresStreamWriter &Stm) const;
	};

	// Tunnel entry points, surfaced for the interop API. The logic lives in
	// Building/Hooks.Tunnels.cpp next to the hooks that drive it.
	static TunnelData* FindTunnelFor(BuildingClass* pBuilding);
	static void AddTunnelPassenger(TunnelData* pTunnel, BuildingClass* pBuilding,
		FootClass* pPassenger);

	// how often a super weapon has been fired, for SW.Shots
	struct ShotStuff {
		int ShootAmount{ 0 };
		int LastCheckedFrame{ 0 };
	};

	enum class RequirementStatus {
		Forbidden = 1, // forbidden by special conditions (e.g. reqhouses) that's not likely to change in this session
		Incomplete = 2, // missing something (approp factory)
		Complete = 3, // OK
		Overridden = 4, // magic condition met, bypass prereq check
	};

	enum class BuildLimitStatus {
		ReachedPermanently = -1, // remove cameo
		ReachedTemporarily = 0, // black out cameo
		NotReached = 1, // don't do anything
	};

	enum class FactoryState {
		Unbuildable = 0, // the house is not allowed to build this at all
		NoFactory = 1, // there is no factory building for this
		Unpowered = 2, // there is a factory building, but it is offline
		Available = 3, // at least one factory building is as online as required
		Primary = 4 // the factory found is the primary one
	};

	// the state of the search, and the factory it settled on, if any
	struct FactoryCheckReturn {
		FactoryState State;
		BuildingClass* Factory;
	};

	class ExtData final : public Extension<HouseClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0x12345678;

		// data read from the INI (type-like)
		Nullable<bool> Degrades;

		// data for the house instance
		bool IonSensitive;

		int AuxPower; //!< Power granted or drained by active Battery super weapons and by trigger action 146.
		int BatteriesActive; //!< Number of currently active Battery super weapons.

		int SWLastIndex;

		int KeepAliveCount; //!< Number of owned objects that keep this house from being defeated.
		int KeepAliveBuildingsCount; //!< The buildings among them.

		BuildingClass *Factory_BuildingType;
		BuildingClass *Factory_InfantryType;
		BuildingClass *Factory_VehicleType;
		BuildingClass *Factory_NavyType;
		BuildingClass *Factory_AircraftType;

		// the types this house has reverse engineered and can thus build
		AresMap<TechnoTypeClass const*, bool> ReverseEngineered;

		std::bitset<32> StolenTech;
		IndexBitfield<HouseClass*> RadarPersist;

		bool NavalYardInfiltrated;
		bool AircraftFactoryInfiltrated;
		bool BuildingInfiltrated;

		ValueableVector<HouseTypeClass *> FactoryOwners_GatheredPlansOf;

		std::vector<BuildingClass*> Academies;

		std::vector<TunnelData> Tunnels;

		std::vector<BuildingTypeClass*> Battery_KeepOnline;
		std::vector<BuildingTypeClass*> Battery_Overpower;

		std::vector<ShotStuff> SWShotCounts;

		ExtData(HouseClass* OwnerObject) : Extension<HouseClass, ExtData>(OwnerObject),
			IonSensitive(false),
			AuxPower(0),
			BatteriesActive(0),
			SWLastIndex(-1),
			KeepAliveCount(0),
			KeepAliveBuildingsCount(0),
			Factory_BuildingType(nullptr),
			Factory_InfantryType(nullptr),
			Factory_VehicleType(nullptr),
			Factory_NavyType(nullptr),
			Factory_AircraftType(nullptr),
			StolenTech(0ull),
			RadarPersist(),
			NavalYardInfiltrated(false),
			AircraftFactoryInfiltrated(false),
			BuildingInfiltrated(false)
		{ }

		~ExtData();

		void LoadFromINIFile(CCINIClass* pINI);

		void InvalidatePointer(void *ptr, bool bRemoved) {
			AnnounceInvalidPointer(Factory_AircraftType, ptr);
			AnnounceInvalidPointer(Factory_BuildingType, ptr);
			AnnounceInvalidPointer(Factory_VehicleType, ptr);
			AnnounceInvalidPointer(Factory_NavyType, ptr);
			AnnounceInvalidPointer(Factory_InfantryType, ptr);

			if(bRemoved) {
				for(auto& tunnel : this->Tunnels) {
					tunnel.RemovePassenger(ptr);
				}
			}
		}

		TunnelData* FindTunnel(size_t index);

		void LoadFromStream(AresStreamReader &Stm);

		void SaveToStream(AresStreamWriter &Stm);

		void SetFirestormState(bool active);

		bool CheckBasePlanSanity();

		void UpdateTogglePower();

		ShotStuff ShotsAmount(int idxSWType) const;
		void UpdateShotsLastCheckedFrame(int idxSWType);
		void UpdateShootCount(int idxSWType);

		int GetSurvivorDivisor() const;
		InfantryTypeClass* GetCrew() const;
		InfantryTypeClass* GetEngineer() const;
		InfantryTypeClass* GetTechnician() const;
		InfantryTypeClass* GetDisguise() const;

		void UpdateAcademy(BuildingClass* pAcademy, bool added);
		void ApplyAcademy(TechnoClass* pTechno, AbstractType considerAs) const;

		bool KeepThisAlive(TechnoClass const* pTechno, AbstractType abs, bool added);

		//! Unlocks a type for this house by reverse engineering it.
		//! Applies Rubble-style substitution through ReversedAs, and rechecks the
		//! tech tree if the type only became buildable now.
		//! \returns true if the type was newly recorded. Callers use this to decide
		//! whether to announce and to spring the reverse-engineer triggers, so it is
		//! deliberately not "did the tech tree change".
		bool ReverseEngineer(TechnoTypeClass const* pVictimType);

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<HouseExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();

		bool InvalidateExtDataIgnorable(void* const ptr) const {
			auto const flags = static_cast<AbstractClass*>(ptr)->AbstractFlags;
			return (flags & AbstractFlags::Techno) == AbstractFlags::None;
		}
	};

	static ExtContainer ExtMap;
	static bool LoadGlobals(AresStreamReader& Stm);
	static bool SaveGlobals(AresStreamWriter& Stm);

	static int CountOwnedNowTotal(HouseClass const* pHouse, TechnoTypeClass const* pItem);
	static signed int BuildLimitRemaining(HouseClass const* pHouse, TechnoTypeClass const* pItem);
	static BuildLimitStatus CheckBuildLimit(HouseClass const* pHouse, TechnoTypeClass const* pItem, bool includeQueued);

	static RequirementStatus RequirementsMet(HouseClass const* pHouse, TechnoTypeClass const* pItem);
	static bool PrerequisitesMet(HouseClass const* pHouse, TechnoTypeClass const* pItem);
	static bool PrerequisitesListed(const Prereqs::BTypeIter &List, TechnoTypeClass const* pItem);

	static FactoryCheckReturn HasFactory(
		HouseClass const* pHouse, TechnoTypeClass const* pItem,
		bool allowOccupied, bool requirePower, bool requireCanBuild,
		bool anyFactory);

	static bool CheckFactoryOwners(HouseClass const* pHouse, TechnoTypeClass const* pItem);

	static bool IsAnyFirestormActive;
	static bool UpdateAnyFirestormActive(bool lastChange);

	// suppress repeated EVA announcements for the sensor array
	static CDTimerClass Timer_CloakedUnitDetected;
	static CDTimerClass Timer_SubterraneanUnitDetected;

	static signed int PrereqValidate(
		HouseClass const* pHouse, TechnoTypeClass const* pItem,
		bool buildLimitOnly, bool includeQueued);

	static bool IsDisabledFromShell(
		HouseClass const* pHouse, BuildingTypeClass const* pItem);

	static size_t FindOwnedIndex(
		HouseClass const* pHouse, int idxParentCountry,
		Iterator<TechnoTypeClass const*> items, size_t start = 0);

	static size_t FindBuildableIndex(
		HouseClass const* pHouse, int idxParentCountry,
		Iterator<TechnoTypeClass const*> items, size_t start = 0);

	template <typename T>
	static T* FindOwned(
		HouseClass const* const pHouse, int const idxParent,
		Iterator<T*> const items, size_t const start = 0)
	{
		auto const index = FindOwnedIndex(pHouse, idxParent, items, start);
		return index < items.size() ? items[index] : nullptr;
	}

	template <typename T>
	static T* FindBuildable(
		HouseClass const* const pHouse, int const idxParent,
		Iterator<T*> const items, size_t const start = 0)
	{
		auto const index = FindBuildableIndex(pHouse, idxParent, items, start);
		return index < items.size() ? items[index] : nullptr;
	}

	static SideClass* GetSide(HouseClass* pHouse);

	static HouseClass* GetHouseKind(OwnerHouseKind kind, bool allowRandom,
		HouseClass* pDefault, HouseClass* pInvoker = nullptr,
		HouseClass* pKiller = nullptr, HouseClass* pVictim = nullptr);

	// temporary storage for the 100-unit bug fix
	static std::vector<int> AIProduction_CreationFrames;
	static std::vector<int> AIProduction_Values;
	static std::vector<int> AIProduction_BestChoices;
};
