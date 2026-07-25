#pragma once

#include <StringTable.h>

#include "../Ares.h"
#include "../Misc/Debug.h"

#include <MessageListClass.h>

class DumperTypesCommandClass : public AresCommandClass
{
public:
	//CommandClass
	virtual const char* GetName() const override
	{
		return "DumpTypes";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return StringTable::LoadString("TXT_DUMP_TYPES");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_DEVELOPMENT");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return StringTable::LoadString("TXT_DUMP_TYPES_DESC");
	}

	template <typename T>
	void LogType(const char* pSection) const {
		Debug::Log("[%s]\n", pSection);

		int i = 0;
		for(auto pItem : T::Array) {
			Debug::Log("%d = %s\n", i++, pItem->get_ID());
		}
	}

	virtual void Execute(WWKey eInput) const override
	{
		if(this->CheckDebugDeactivated()) {
			return;
		}

		Debug::Log("Dumping all Types\n\n");

		Debug::Log("Dumping Rules Types\n\n");

		LogType<AnimTypeClass>("Animations");
		LogType<WeaponTypeClass>("WeaponTypes");
		LogType<WarheadTypeClass>("Warheads");
		LogType<BulletTypeClass>("Projectiles");

		LogType<HouseTypeClass>("Countries");

		LogType<InfantryTypeClass>("InfantryTypes");
		LogType<UnitTypeClass>("VehicleTypes");
		LogType<AircraftTypeClass>("AircraftTypes");
		LogType<BuildingTypeClass>("BuildingTypes");

		LogType<SuperWeaponTypeClass>("SuperWeaponTypes");
		LogType<SmudgeTypeClass>("SmudgeTypes");
		LogType<OverlayTypeClass>("OverlayTypes");
//		LogType<TerrainTypeClass>("TerrainTypes"); // needs class map in YRPP
		LogType<ParticleTypeClass>("Particles");
		LogType<ParticleSystemTypeClass>("ParticleSystems");

/*
		Debug::Log("Dumping Art Types\n\n");
		Debug::Log("[Movies]\n");
		for(int i = 0; i < MovieInfo::Array.Count; ++i) {
			Debug::Log("%d = %s\n", i, MovieInfo::Array.GetItem(i).Name);
		}
*/

		Debug::Log("Dumping AI Types\n\n");
		LogType<ScriptTypeClass>("ScriptTypes");
		LogType<TeamTypeClass>("TeamTypes");
		LogType<TaskForceClass>("TaskForces");

		Debug::Log("[AITriggerTypes]\n");
		for(auto const& pItem : AITriggerTypeClass::Array) {
			char Buffer[1024];
			pItem->FormatForSaving(Buffer, sizeof(Buffer));
			Debug::Log("%s\n", Buffer);
		}

		Debug::Log("[AITriggerTypesEnable]\n");
		for(auto const& pItem : AITriggerTypeClass::Array) {
			Debug::Log("%X = %s\n", pItem->get_ID(), pItem->IsEnabled ? "yes" : "no");
		}

		MessageListClass::Instance.PrintMessage(L"Type data dumped");
	}
};
