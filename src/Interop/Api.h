#pragma once

// Antares interop ABI.
//
// Other Syringe DLLs cannot reach into Antares the way they reach into Ares.
// Ares exports its ~1400 hook handlers and nothing else, so consumers such as
// Phobos resolve everything they actually want by hardcoded RVA and read our
// ExtData through shadow structs full of char[0x131] padding. None of that
// survives a recompile, which is exactly what Antares is.
//
// This is the supported alternative: one export hands back a table of function
// pointers, and ExtData is reached through accessors returning real YRpp
// objects, so no consumer has to know a single offset.
//
// THE TABLE IS AN ABI. Append only, never reorder, never change a signature.
// Growth is detected through `size`; a consumer asks for the major it was built
// against and must check `size` before touching anything added later.

#include "../Utilities/Macro.h"

#include <windows.h>  // HRESULT, and the calling-convention keywords
// CDTimerClass is a `using` alias, so it cannot be forward-declared. Timer.h is
// not self-contained either; this is the header that pulls it in the right order.
#include <GeneralStructures.h>

#include "../Utilities/Iterator.h"  // passed by value, so it must be complete

#include <cstdint>

class AbstractClass;
class AircraftTypeClass;
class AlphaShapeClass;
class BuildingClass;
class BuildingTypeClass;
class CellClass;
class EBolt;
class FootClass;
class HouseClass;
class HouseTypeClass;
class InfantryTypeClass;
class ObjectClass;
class SuperWeaponTypeClass;
class TechnoClass;
class TechnoTypeClass;
class WarheadTypeClass;
class WeaponTypeClass;
struct VeterancyStruct;

//! Which factory a house may have had infiltrated.
enum class AntaresFactory : uint32_t
{
	WarFactory = 0,   //!< non-Naval vehicles
	NavalYard = 1,    //!< Naval vehicles
	Barracks = 2,
	AircraftFactory = 3,
	ConstructionYard = 4,
};

//! Subsystems a consumer can take over. A feature only stops behaviour -- it
//! never changes an ExtData layout, so savegames are unaffected either way.
//!
//! This list is deliberately short. A subsystem only appears once its hooks have
//! been reviewed one at a time and standing down is known to leave the game in a
//! consistent state; several obvious candidates did not survive that check.
//! Values are append-only.
enum class AntaresFeature : uint32_t
{
	EBolt = 0,   //!< the coloured electric bolt draw, leaving the game's own colours

	//! ObjectClass's per-frame alpha shape update. The shape map itself keeps being
	//! maintained from the AlphaShapeClass constructor and destructor, so a consumer
	//! that drives the update itself can still create and destroy shapes normally and
	//! read them back through FindAlphaShape. Does not cover the anim path.
	AlphaImage = 1,

	Count
};

//! Version 1 of the interop table. Fields are appended, never reordered.
struct AntaresAPI_v1
{
	uint32_t size;    //!< sizeof(AntaresAPI_v1) as Antares built it
	uint32_t major;
	uint32_t minor;
	uint32_t patch;

	// --- behaviour -----------------------------------------------------------
	bool  (__stdcall* ConvertTypeTo)(TechnoClass* pThis, TechnoTypeClass* pToType);
	void  (__stdcall* SpawnSurvivors)(FootClass* pThis, TechnoClass* pKiller, bool select, bool ignoreDefenses);
	bool  (__stdcall* ReverseEngineer)(BuildingClass* pThis, TechnoClass* pVictim);
	bool  (__stdcall* MeetsAITargetingConstraints)(SuperWeaponTypeClass* pType, HouseClass* pOwner, bool manual);
	bool  (__stdcall* IsSuperWeaponAvailable)(SuperWeaponTypeClass* pType, HouseClass* pHouse);
	bool  (__stdcall* ApplyPermaMindControl)(WarheadTypeClass* pWH, HouseClass* pOwner, AbstractClass* pTarget);
	bool  (__stdcall* DetailsCurrentlyEnabled)();
	EBolt* (__stdcall* CreateElectricBolt)(WeaponTypeClass* pWeapon);
	int   (__stdcall* FindEVAIndex)(const char* pID);
	bool  (__stdcall* CameoIsElite)(TechnoTypeClass* pType, HouseClass* pHouse);
	void  (__stdcall* SendParadropPlane)(HouseClass* pOwner, CellClass* pTarget,
		AircraftTypeClass* pPlaneType, Iterator<TechnoTypeClass*> types, Iterator<int> nums);

	//! The tunnel network this building belongs to, or null if it is not a tunnel.
	//! The returned pointer is opaque; pass it straight back to AddTunnelPassenger.
	void* (__stdcall* FindTunnel)(BuildingClass* pBuilding);
	void  (__stdcall* AddTunnelPassenger)(void* pTunnel, BuildingClass* pBuilding, FootClass* pPassenger);

	// --- extension data ------------------------------------------------------
	// Pointers into live ExtData. Valid until the owning object dies; never
	// cached across a save/load. Null when the object has no extension.
	CDTimerClass* (__stdcall* GetDisableWeaponTimer)(TechnoClass* pThis);
	bool*         (__stdcall* GetDriverKilled)(TechnoClass* pThis);
	bool*         (__stdcall* GetInfiltrated)(HouseClass* pHouse, AntaresFactory factory);

	//! Ares stores this as a veteran/elite pair; Antares resolves it through the
	//! ability set, so it has to be a call rather than a field.
	bool (__stdcall* IsPsionicsImmune)(TechnoTypeClass* pType, VeterancyStruct const* pVeterancy);

	//! Operator= -- the infantry that must ride along for the unit to work.
	//! `pAnyAllowed` receives whether any passenger will do.
	bool (__stdcall* GetOperators)(TechnoTypeClass* pType, InfantryTypeClass* const** ppItems,
		int* pCount, bool* pAnyAllowed);

	bool (__stdcall* IsVeteranBuilding)(HouseTypeClass* pCountry, BuildingTypeClass* pType);

	//! The ObjectClass -> AlphaShapeClass map Antares maintains.
	AlphaShapeClass* (__stdcall* FindAlphaShape)(ObjectClass* pObject);

	// --- feature handover ----------------------------------------------------
	//! Stand a subsystem down so the caller can own it. Returns false if the
	//! feature is unknown to this build, in which case nothing changed.
	bool (__stdcall* DisableFeature)(AntaresFeature feature);
	bool (__stdcall* IsFeatureDisabled)(AntaresFeature feature);
};

//! Hand back the interop table.
//! \param wantMajor The major version the caller was built against.
//! \param ppApi Receives the table. Owned by Antares; do not free.
//! \retval S_OK Table returned.
//! \retval E_POINTER ppApi was null.
//! \retval E_NOTIMPL This build does not implement the requested major.
DEFINE_EXPORT(HRESULT, GetAntaresAPI, uint32_t wantMajor, AntaresAPI_v1** ppApi);
