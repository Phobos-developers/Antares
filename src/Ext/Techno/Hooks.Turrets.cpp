#include "Body.h"
#include "../TechnoType/Body.h"

#include <AircraftClass.h>
#include <CCFileClass.h>
#include <ObjectTypeClass.h>
#include <UnitClass.h>
#include <UnitTypeClass.h>

#include <FileFormats/HVA.h>
#include <FileFormats/VXL.h>

#include <algorithm>
#include <cstring>

namespace {
	void ClearVoxel(VoxelStruct& voxel)
	{
		if(auto const pVXL = voxel.VXL) {
			voxel.VXL = nullptr;
			GameDelete(pVXL);
		}

		if(auto const pHVA = voxel.HVA) {
			voxel.HVA = nullptr;
			GameDelete(pHVA);
		}
	}

	// hands the loaded pair over to its final home and frees whatever was there
	void AssignVoxel(VoxelStruct& dest, VoxelStruct& src)
	{
		if(&dest == &src) {
			return;
		}

		auto const pVXL = dest.VXL;
		dest.VXL = src.VXL;
		src.VXL = nullptr;
		if(pVXL) {
			GameDelete(pVXL);
		}

		auto const pHVA = dest.HVA;
		dest.HVA = src.HVA;
		src.HVA = nullptr;
		if(pHVA) {
			GameDelete(pHVA);
		}
	}

	// loads <name>.vxl and its .hva. returns whether the art counts as present,
	// which for an absent file is what the caller passes in.
	bool ReadVoxel(VoxelStruct& voxel, const char* pName, bool missingIsOk)
	{
		char filename[0x40];

		voxel.VXL = nullptr;
		voxel.HVA = nullptr;

		_snprintf_s(filename, _TRUNCATE, "%s.VXL", pName);

		VoxLib* pVXL = nullptr;
		{
			auto const pFile = UniqueGamePtr<CCFileClass>(GameCreate<CCFileClass>(filename));
			if(!pFile->Exists()) {
				return missingIsOk;
			}

			pVXL = static_cast<VoxLib*>(YRMemory::AllocateChecked(sizeof(VoxLib)));
			std::memset(pVXL, 0, sizeof(VoxLib));

			// the flag is raised when the file could not be parsed
			if(!pVXL->ReadFile(pFile.get(), false)) {
				pVXL->Initialized = true;
			}
		}

		_snprintf_s(filename, _TRUNCATE, "%s.HVA", pName);

		MotLib* pHVA = nullptr;
		{
			auto const pFile = UniqueGamePtr<CCFileClass>(GameCreate<CCFileClass>(filename));
			if(pFile->Exists()) {
				pHVA = static_cast<MotLib*>(YRMemory::AllocateChecked(sizeof(MotLib)));
				std::memset(pHVA, 0, sizeof(MotLib));

				if(!pHVA->ReadFile(pFile.get())) {
					pHVA->LoadedFailed = 1;
				}
			}
		}

		if(pHVA && !pVXL->Initialized && !pHVA->LoadedFailed) {
			auto const& tailer = pVXL->TailerData[pVXL->HeaderData->limb_number];
			pHVA->Scale(tailer.HVAMultiplier);

			voxel.VXL = pVXL;
			voxel.HVA = pHVA;
			return true;
		}

		if(pHVA) {
			GameDelete(pHVA);
		}
		GameDelete(pVXL);

		return false;
	}
}

DEFINE_HOOK(0x5F8084, ObjectTypeClass_UnloadTurretArt, 0x6)
{
	GET(ObjectTypeClass* const, pThis, ECX);

	if(auto const pExt = TechnoTypeExt::ExtMap.Find(static_cast<TechnoTypeClass*>(pThis))) {
		for(auto& voxel : pExt->Turrets) {
			ClearVoxel(voxel);
		}

		for(auto& voxel : pExt->Barrels) {
			ClearVoxel(voxel);
		}
	}

	return 0;
}

// the spawn-less body voxel gets storage of its own instead of borrowing the
// turret's, so a spawner can have both
DEFINE_HOOK(0x5F8277, ObjectTypeClass_Load3DArt_NoSpawnAlt1, 0x7)
{
	GET(ObjectTypeClass* const, pThis, ESI);

	if(!pThis || pThis->WhatAmI() != AbstractType::UnitType) {
		return 0x5F8640;
	}

	if(pThis->NoSpawnAlt) {
		auto const pExt = TechnoTypeExt::ExtMap.Find(static_cast<TechnoTypeClass*>(pThis));

		char filename[0x40];
		_snprintf_s(filename, _TRUNCATE, "%sWO", pThis->ImageFile);

		VoxelStruct voxel;
		auto const loaded = ReadVoxel(voxel, filename, false);

		AssignVoxel(pExt->NoSpawnAltImage, voxel);

		if(!loaded) {
			R->Stack8(0x13, 1);
		}

		ClearVoxel(voxel);
	}

	return 0x5F8287;
}

DEFINE_HOOK(0x5F848C, ObjectTypeClass_Load3DArt_NoSpawnAlt2, 0x6)
{
	return 0x5F8844;
}

DEFINE_HOOK(0x5F865F, ObjectTypeClass_Load3DArt_Turrets, 0x6)
{
	GET(ObjectTypeClass* const, pThis, ESI);

	auto const pType = static_cast<TechnoTypeClass*>(pThis);
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	pExt->Turrets.resize(static_cast<size_t>(
		std::max(0, pType->TurretCount - TechnoTypeClass::MaxWeapons)));

	char filename[0x40];

	for(auto i = 0; i < pType->TurretCount; ++i) {
		if(i) {
			_snprintf_s(filename, _TRUNCATE, "%sTUR%d", pType->ImageFile, i);
		} else {
			_snprintf_s(filename, _TRUNCATE, "%sTUR", pType->ImageFile);
		}

		VoxelStruct voxel;
		auto const loaded = ReadVoxel(voxel, filename, false);

		AssignVoxel(*pExt->GetTurretVoxel(i), voxel);
		ClearVoxel(voxel);

		if(!loaded) {
			return 0x5F868C;
		}
	}

	return 0x5F8844;
}

DEFINE_HOOK(0x5F887B, ObjectTypeClass_Load3DArt_Barrels, 0x6)
{
	GET(ObjectTypeClass* const, pThis, ESI);

	auto const pType = static_cast<TechnoTypeClass*>(pThis);
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	pExt->Barrels.resize(static_cast<size_t>(
		std::max(0, pType->TurretCount - TechnoTypeClass::MaxWeapons)));

	char filename[0x40];

	for(auto i = 0; i < pType->TurretCount; ++i) {
		if(i) {
			_snprintf_s(filename, _TRUNCATE, "%sBARL%d", pType->ImageFile, i);
		} else {
			_snprintf_s(filename, _TRUNCATE, "%sBARL", pType->ImageFile);
		}

		VoxelStruct voxel;
		auto const loaded = ReadVoxel(voxel, filename, true);

		AssignVoxel(*pExt->GetBarrelVoxel(i), voxel);
		ClearVoxel(voxel);

		if(!loaded) {
			return 0x5F8A6A;
		}
	}

	return 0x5F8A60;
}

DEFINE_HOOK(0x73B6E3, UnitClass_DrawVXL_NoSpawnAlt, 0x6)
{
	GET(UnitTypeClass* const, pType, EBX);

	R->EDX(&TechnoTypeExt::ExtMap.Find(pType)->NoSpawnAltImage);
	return 0x73B6E9;
}

DEFINE_HOOK(0x73B90E, UnitClass_DrawVXL_Barrels1, 0x7)
{
	GET(UnitTypeClass* const, pType, EBX);
	GET(int const, index, EAX);

	R->Stack(0x2C, R->ESI());

	auto const pVoxel = TechnoTypeExt::ExtMap.Find(pType)->GetBarrelVoxel(index);

	return (pVoxel->VXL && pVoxel->HVA) ? 0x73B928 : 0x73B94A;
}

DEFINE_HOOK(0x73BCCD, UnitClass_DrawVXL_Barrels2, 0x7)
{
	GET(UnitTypeClass* const, pType, EBX);
	GET(int const, index, ECX);

	R->EDX(TechnoTypeExt::ExtMap.Find(pType)->GetBarrelVoxel(index));
	return 0x73BCD4;
}

DEFINE_HOOK(0x73BD15, UnitClass_DrawVXL_Turrets, 0x7)
{
	GET(UnitTypeClass* const, pType, EBX);
	GET(int const, index, ESI);

	R->ECX(TechnoTypeExt::ExtMap.Find(pType)->GetTurretVoxel(index));
	return 0x73BD1C;
}

DEFINE_HOOK(0x73BD6A, UnitClass_DrawVXL_Barrels3, 0x7)
{
	GET(UnitTypeClass* const, pType, EBX);
	GET(int const, index, ESI);

	R->ECX(TechnoTypeExt::ExtMap.Find(pType)->GetBarrelVoxel(index));
	return 0x73BD71;
}

DEFINE_HOOK(0x413FFA, AircraftClass_Init_TurretROT, 0x6)
{
	GET(AircraftTypeClass* const, pType, EDX);

	R->EAX(TechnoTypeExt::ExtMap.Find(pType)->TurretROT.Get(pType->ROT));
	return 0x414000;
}

DEFINE_HOOK(0x735584, UnitClass_CTOR_TurretROT, 0x6)
{
	GET(UnitTypeClass* const, pType, ECX);

	R->EDX(TechnoTypeExt::ExtMap.Find(pType)->TurretROT.Get(pType->ROT));
	return 0x73558A;
}

// the cargo keeps the carryall's heading instead of always facing north, and
// gets its turret rate of turn back
DEFINE_HOOK(0x416C3A, AircraftClass_Carryall_Unload_Facing, 0x5)
{
	GET(AircraftClass* const, pThis, EDI);
	GET(FootClass* const, pCargo, ESI);
	GET(CoordStruct* const, pCoord, ECX);

	auto const facing = pThis->TurretFacing().GetFacing<256>();

	if(!pCargo->Unlimbo(*pCoord, static_cast<DirType>(facing))) {
		return 0x416C49;
	}

	auto const pType = pCargo->GetTechnoType();
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);

	pCargo->PrimaryFacing.SetROT(static_cast<short>(pType->ROT));
	pCargo->SecondaryFacing.SetROT(static_cast<short>(pExt->TurretROT.Get(pType->ROT)));

	return 0x416C5A;
}
