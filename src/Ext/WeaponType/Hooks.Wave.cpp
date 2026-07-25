#include "Body.h"
#include "../Techno/Body.h"

#include <HouseClass.h>
#include <WarheadTypeClass.h>
#include <WaveClass.h>

// custom beam styles
DEFINE_HOOK(0x6FF449, TechnoClass_Fire_SonicWave, 0x5)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeapon, EBX);
	GET_BASE(AbstractClass* const, pTarget, 0x8);
	GET_BASE(BYTE const, idxWeapon, 0xC);

	REF_STACK(CoordStruct const, crdSrc, 0x44);
	REF_STACK(CoordStruct const, crdTgt, 0x88);

	auto const pData = WeaponTypeExt::ExtMap.Find(pWeapon);

	pThis->Wave = WeaponTypeExt::CreateWave(
		crdSrc, crdTgt, pThis, WaveType::Sonic, pTarget, idxWeapon, pData);

	return 0x6FF48A;
}

DEFINE_HOOK(0x6FF5F5, TechnoClass_Fire_OtherWaves, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeapon, EBX);
	GET(AbstractClass* const, pTarget, EDI);
	GET_BASE(BYTE const, idxWeapon, 0xC);

	REF_STACK(CoordStruct const, crdSrc, 0x44);
	REF_STACK(CoordStruct const, crdTgt, 0x88);

	auto const pData = WeaponTypeExt::ExtMap.Find(pWeapon);

	auto type = WaveType::Magnetron;

	if(pWeapon->IsMagBeam) {
		if(pThis->Wave) {
			return 0x6FF656;
		}
	} else {
		if(!pData->Wave_IsLaser && !pData->Wave_IsBigLaser) {
			return 0x6FF656;
		}

		// This reads backwards and is correct. Shipped Ares 3.0p1 computes the
		// wave type as `(pWeaponExt->Wave_IsBigLaser != 0) + 1`
		// (TechnoClass_Fire_OtherWaves, Ares.dll 0x10057B10, testing
		// WeaponTypeExt +0x3A = Wave_IsBigLaser; +0x39 is Wave_IsLaser), so
		// Wave.IsBigLaser must store 2 and plain Wave.IsLaser must store 1.
		// In the engine's enum 2 is WaveType::Laser and 1 is WaveType::BigLaser --
		// NonMagWaveMatrixes[1] is (-34,-44) and [2] is (-27,-34), i.e. slot 1 is
		// the larger wave (WaveClass_Draw_NonMagnetic, gamemd 0x761640). So
		// Wave.IsBigLaser genuinely selects the smaller engine wave in shipped
		// Ares; that is reproduced on purpose. Do not "fix" either half of this:
		// the enum labels and this ternary were both wrong in the old pin and
		// cancelled out, and correcting only one silently inverts the effect.
		type = pData->Wave_IsBigLaser ? WaveType::Laser : WaveType::BigLaser;
	}

	pThis->Wave = WeaponTypeExt::CreateWave(
		crdSrc, crdTgt, pThis, type, pTarget, idxWeapon, pData);

	return 0x6FF656;
}

// 763226, 6
DEFINE_HOOK(0x763226, WaveClass_DTOR, 0x6)
{
	GET(WaveClass *, Wave, EDI);
	WeaponTypeExt::WaveExt.erase(Wave);
	return 0;
}

// 760F50, 6
// complete replacement for WaveClass::Update
DEFINE_HOOK(0x760F50, WaveClass_Update, 0x6)
{
	GET(WaveClass *, pThis, ECX);

	auto pData = WeaponTypeExt::WaveExt.get_or_default(pThis);
	const WeaponTypeClass *Weap = pData->OwnerObject();

	if(!Weap) {
		return 0;
	}

	int Intensity;

	if(Weap->AmbientDamage) {
		CoordStruct coords;
		for(int i = 0; i < pThis->Cells.Count; ++i) {
			CellClass *Cell = pThis->Cells.GetItem(i);
			pThis->DamageArea(*Cell->GetCellCoords(&coords));
		}
	}

	switch(pThis->Type) {
		case WaveType::Sonic:
			pThis->Update_Wave();
			Intensity = pThis->WaveEC;
			--Intensity;
			pThis->WaveEC = Intensity;
			if(Intensity < 0) {
				pThis->UnInit();
			} else {
				SET_REG32(ECX, pThis);
				CALL(0x5F3E70); // ObjectClass::Update
			}
			break;
		case WaveType::BigLaser:
		case WaveType::Laser:
			Intensity = pThis->LaserEC;
			Intensity -= 6;
			pThis->LaserEC = Intensity;
			if(Intensity < 32) {
				pThis->UnInit();
			}
			break;
		case WaveType::Magnetron:
			pThis->Update_Wave();
			Intensity = pThis->WaveEC;
			--Intensity;
			pThis->WaveEC = Intensity;
			if(Intensity < 0) {
				pThis->UnInit();
			} else {
				SET_REG32(ECX, pThis);
				CALL(0x5F3E70); // ObjectClass::Update
			}
			break;
	}

	return 0x76101A;
}

// the colors are calculated once before the beam is drawn and then applied to
// every pixel it covers.
DEFINE_HOOK(0x75FA29, WaveClass_Draw_Colors, 0x6)
{
	GET(WaveClass* const, pThis, ESI);

	WeaponTypeExt::WaveColors = WeaponTypeExt::GetWaveColorData(pThis);

	return 0;
}

DEFINE_HOOK(0x760BC2, WaveClass_Draw2, 0x9)
{
	if(!WeaponTypeExt::WaveColors.Modified) {
		return 0;
	}

	GET(WaveClass* const, pThis, EBX);
	GET(WORD* const, pDest, EBP);

	*pDest = WeaponTypeExt::ModifyWaveColor(
		*pDest, pThis->LaserEC, WeaponTypeExt::WaveColors);

	return 0x760CAF;
}

DEFINE_HOOK(0x760DE2, WaveClass_Draw3, 0x9)
{
	if(!WeaponTypeExt::WaveColors.Modified) {
		return 0;
	}

	GET(WaveClass* const, pThis, EBX);
	GET(WORD* const, pDest, EDI);

	*pDest = WeaponTypeExt::ModifyWaveColor(
		*pDest, pThis->LaserEC, WeaponTypeExt::WaveColors);

	return 0x760ECB;
}

// 75EE57, 7
DEFINE_HOOK(0x75EE57, WaveClass_Draw_Sonic, 0x7)
{
	if(!WeaponTypeExt::WaveColors.Modified) {
		return 0;
	}

	GET(WORD* const, pDest, EDI);
	GET(DWORD const, offset, ECX);

	*pDest = WeaponTypeExt::ModifyWaveColor(
		pDest[offset], R->ESI(), WeaponTypeExt::WaveColors);

	return 0x75EF1C;
}

// 7601FB, 0B
DEFINE_HOOK(0x7601FB, WaveClass_Draw_Magnetron2, 0x0B)
{
	if(!WeaponTypeExt::WaveColors.Modified) {
		return 0;
	}

	GET(WORD* const, pDest, EBX);
	GET(DWORD const, offset, ECX);

	*pDest = WeaponTypeExt::ModifyWaveColor(
		pDest[offset], R->EBP(), WeaponTypeExt::WaveColors);

	return 0x760285;
}

// 760286, 5
DEFINE_HOOK(0x760286, WaveClass_Draw_Magnetron3, 0x5)
{
	return 0x7602D3;
}

// keep the index inside the square root table
DEFINE_HOOK(0x75EE2E, WaveClass_Draw_Green, 0x8)
{
	GET(int const, index, EDX);

	R->EDX(index > 0x15F8F ? 0x15F8F : index);

	return 0;
}

DEFINE_HOOK(0x7601C7, WaveClass_Draw_Magnetron, 0x8)
{
	GET(int const, index, EDX);

	R->EDX(index > 0x15F8F ? 0x15F8F : index);

	return 0;
}

// the Nod laser is drawn in full regardless of the detail level
DEFINE_HOOK(0x7609E3, WaveClass_Draw_NodLaser_Details, 0x5)
{
	R->EAX(2);
	return 0x7609E8;
}

// keep the buffer around instead of releasing it every time
DEFINE_HOOK(0x76110B, WaveClass_RecalculateAffectedCells_Clear, 0x5)
{
	GET(DynamicVectorClass<CellClass*>* const, pCells, EBP);

	pCells->Count = 0;

	return 0x761110;
}

// 75F38F, 6
DEFINE_HOOK(0x75F38F, WaveClass_DamageCell, 0x6)
{
	GET(WaveClass *, Wave, EBP);
	auto pData = WeaponTypeExt::WaveExt.get_or_default(Wave);
	R->EDI(R->EAX());
	R->EBX(pData->OwnerObject());
	return 0x75F39D;
}

DEFINE_HOOK(0x75F46E, WaveClass_DamageCell_Wall, 0x6)
{
	GET(WeaponTypeClass* const, pWeapon, EBX);

	return pWeapon->Warhead->Wall ? 0 : 0x75F47C;
}

DEFINE_HOOK(0x762B62, WaveClass_Update_Beam, 0x6)
{
	GET(WaveClass* const, pThis, ESI);

	auto const pTarget = pThis->Target;
	auto const pOwner = pThis->Owner;

	auto const pData = WeaponTypeExt::WaveExt.get_or_default(pThis);

	BYTE idxWeapon = 0;
	auto keepAlive = false;

	if(pTarget && pOwner && pThis->WaveEC != 19 && pOwner->Target == pTarget) {
		idxWeapon = TechnoExt::ExtMap.Find(pOwner)->idxSlot_Wave;

		if(pThis->Type == WaveType::Magnetron) {
			keepAlive = pOwner->IsCloseEnough(pTarget, idxWeapon);
		} else {
			auto const distance = pOwner->GetCoords().DistanceFrom(pTarget->GetCoords());
			keepAlive = pData->OwnerObject()->Range >= distance / Math::Sqrt2;
		}
	}

	if(!keepAlive) {
		pThis->IsTraveling = 0;
		pThis->ShouldEnd = 1;
	}

	if(!pThis->IsTraveling) {
		return 0x762D57;
	}

	CoordStruct crdSrc;
	pOwner->GetFLH(&crdSrc, idxWeapon, CoordStruct::Empty);

	auto const crdTgt = pTarget->GetCoords();
	auto const reversed = pData->IsWaveReversedAgainst(pTarget);

	if(pThis->Type == WaveType::Magnetron) {
		reversed
			? pThis->Draw_Magnetic(crdTgt, crdSrc)
			: pThis->Draw_Magnetic(crdSrc, crdTgt);
	} else {
		reversed
			? pThis->Draw_NonMagnetic(crdTgt, crdSrc)
			: pThis->Draw_NonMagnetic(crdSrc, crdTgt);
	}

	return 0x762D57;
}
