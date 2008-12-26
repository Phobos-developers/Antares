#include "WeaponTypeExt.h"
#include "TechnoExt.h"
#include <ScenarioClass.h>

EXT_P_DEFINE(WeaponTypeClass);
typedef stdext::hash_map<BombClass *, WeaponTypeClassExt::WeaponTypeClassData *> hash_bombExt;
typedef stdext::hash_map<WaveClass *, WeaponTypeClassExt::WeaponTypeClassData *> hash_waveExt;
typedef stdext::hash_map<RadSiteClass *, WeaponTypeClassExt::WeaponTypeClassData *> hash_radsiteExt;

hash_bombExt WeaponTypeClassExt::BombExt;
hash_waveExt WeaponTypeClassExt::WaveExt;
hash_radsiteExt WeaponTypeClassExt::RadSiteExt;

void __stdcall WeaponTypeClassExt::Create(WeaponTypeClass* pThis)
{
	if(!CONTAINS(Ext_p, pThis))
	{
		ALLOC(ExtData, pData);

		pData->Weapon_Loaded  = 1;
		pData->Is_Initialized = 0;
		pData->Weapon_Source  = pThis;

		pData->Beam_Duration     = 15;
		pData->Beam_Amplitude    = 40.0;
		pData->Beam_Color = ColorStruct(255, 255, 255);
		pData->Wave_IsHouseColor = 0;

		pData->Wave_IsLaser      = 0;
		pData->Wave_IsBigLaser   = 0;
		pData->Wave_Color      = ColorStruct(255, 255, 255);
		pData->Beam_IsHouseColor = 0;
/*
		pData->Wave_InitialIntensity = 160;
		pData->Wave_IntensityStep = -6;
		pData->Wave_FinalIntensity = 32; // yeah, they don't match well. Tell that to WW.
*/

		// these can't be initialized to meaningful values, rules class is not loaded from ini yet
		pData->Ivan_KillsBridges = 1;
		pData->Ivan_Detachable   = 1;
		pData->Ivan_Damage       = 0;
		pData->Ivan_Delay        = 0;
		pData->Ivan_TickingSound = -1;
		pData->Ivan_AttachSound  = -1;
		pData->Ivan_WH           = NULL;
		pData->Ivan_Image        = NULL;
		pData->Ivan_FlickerRate  = 30;

		pData->Rad_WH = NULL;
		pData->Rad_Color = ColorStruct(0, 240, 0);
		pData->Rad_Duration_Multiple = 1;
		pData->Rad_Application_Delay = 16;
		pData->Rad_Level_Max = 500;
		pData->Rad_Level_Delay = 16;
		pData->Rad_Light_Delay = 16;
		pData->Rad_Level_Factor = 1.0;
		pData->Rad_Light_Factor = 1.0;
		pData->Rad_Tint_Factor = 1.0;

		Ext_p[pThis] = pData;
	}
}

void __stdcall WeaponTypeClassExt::Delete(WeaponTypeClass* pThis)
{
	if(CONTAINS(Ext_p, pThis))
	{
		DEALLOC(Ext_p, pThis);
	}
}

void __stdcall WeaponTypeClassExt::Load(WeaponTypeClass* pThis, IStream* pStm)
{
	if(CONTAINS(Ext_p, pThis))
	{
		Create(pThis);

		ULONG out;
		pStm->Read(&Ext_p[pThis], sizeof(ExtData), &out);
	}
}

void __stdcall WeaponTypeClassExt::Save(WeaponTypeClass* pThis, IStream* pStm)
{
	if(CONTAINS(Ext_p, pThis))
	{
		ULONG out;
		pStm->Write(&Ext_p[pThis], sizeof(ExtData), &out);
	}
}

void WeaponTypeClassExt::WeaponTypeClassData::Initialize(WeaponTypeClass* pThis)
{
	RulesClass * Rules = RulesClass::Global();
	this->Ivan_Damage       = Rules->get_IvanDamage();
	this->Ivan_Delay        = Rules->get_IvanTimedDelay();
	this->Ivan_TickingSound = Rules->get_BombTickingSound();
	this->Ivan_AttachSound  = Rules->get_BombAttachSound();
	this->Ivan_WH           = Rules->get_IvanWarhead();
	this->Ivan_Image        = Rules->get_BOMBCURS_SHP();
	this->Ivan_FlickerRate  = Rules->get_IvanIconFlickerRate();
	if(pThis->get_IsRadBeam() || pThis->get_IsRadEruption())
	{
		if(pThis->get_Warhead()->get_Temporal())
		{
			this->Beam_Color = *Rules->get_ChronoBeamColor(); // Well, a RadEruption Temporal will look pretty funny
		}
		else
		{
			this->Beam_Color = *Rules->get_RadColor();
		}
	}

	if(pThis->get_IsMagBeam())
	{
		this->Wave_Color = ColorStruct(0xB0, 0, 0xD0); // rp2 values
	}
	else if(pThis->get_IsSonic())
	{
		this->Wave_Color = ColorStruct(255, 255, 255); // dunno the actual default
	}

	this->Wave_Reverse[idxVehicle] = pThis->get_IsMagBeam();
	this->Wave_Reverse[idxAircraft] = 0;
	this->Wave_Reverse[idxBuilding] = 0;
	this->Wave_Reverse[idxInfantry] = 0;
	this->Wave_Reverse[idxOther] = 0;

	this->Rad_WH = Rules->get_RadSiteWarhead();
	this->Rad_Color = *Rules->get_RadColor();
	this->Rad_Duration_Multiple = Rules->get_RadDurationMultiple();
	this->Rad_Application_Delay = Rules->get_RadApplicationDelay();
	this->Rad_Level_Max = Rules->get_RadLevelMax();
	this->Rad_Level_Delay = Rules->get_RadLevelDelay();
	this->Rad_Light_Delay = Rules->get_RadLightDelay();
	this->Rad_Level_Factor = Rules->get_RadLevelFactor();
	this->Rad_Light_Factor = Rules->get_RadLightFactor();
	this->Rad_Tint_Factor = Rules->get_RadTintFactor();

	this->Is_Initialized = 1;
}

void __stdcall WeaponTypeClassExt::LoadFromINI(WeaponTypeClass* pThis, CCINIClass* pINI)
{
	const char * section = pThis->get_ID();
	if(!CONTAINS(Ext_p, pThis) || !pINI->GetSection(section))
	{
		return;
	}

	ExtData *pData = Ext_p[pThis];

	if(!pData->Is_Initialized)
	{
		pData->Initialize(pThis);
	}

	if(pThis->get_Damage() == 0 && pData->Weapon_Loaded)
	{
		// blargh
		// this is the ugly case of a something that apparently isn't loaded from ini yet, wonder why
		pData->Weapon_Loaded = 0;
		pThis->LoadFromINI(pINI);
		return;
	}

	PARSE_BUF();

	ColorStruct tmpColor;

	pData->Beam_Duration     = pINI->ReadInteger(section, "Beam.Duration", pData->Beam_Duration);
	pData->Beam_Amplitude    = pINI->ReadDouble(section, "Beam.Amplitude", pData->Beam_Amplitude);
	pData->Beam_IsHouseColor = pINI->ReadBool(section, "Beam.IsHouseColor", pData->Beam_IsHouseColor);
	if(!pData->Beam_IsHouseColor)
	{
		PARSE_COLOR("Beam.Color", pData->Beam_Color, tmpColor);
	}

	pData->Wave_IsLaser      = pINI->ReadBool(section, "Wave.IsLaser", pData->Wave_IsLaser);
	pData->Wave_IsBigLaser   = pINI->ReadBool(section, "Wave.IsBigLaser", pData->Wave_IsBigLaser);
	pData->Wave_IsHouseColor = pINI->ReadBool(section, "Wave.IsHouseColor", pData->Wave_IsHouseColor);
	if(!pData->Wave_IsHouseColor)
	{
		PARSE_COLOR("Wave.Color", pData->Wave_Color, tmpColor);
	}

	pData->Wave_Reverse[idxVehicle]   = 
		pINI->ReadBool(section, "Wave.ReverseAgainstVehicles", pData->Wave_Reverse[idxVehicle]);
	pData->Wave_Reverse[idxAircraft]  = 
		pINI->ReadBool(section, "Wave.ReverseAgainstAircraft", pData->Wave_Reverse[idxAircraft]);
	pData->Wave_Reverse[idxBuilding] = 
		pINI->ReadBool(section, "Wave.ReverseAgainstBuildings", pData->Wave_Reverse[idxBuilding]);
	pData->Wave_Reverse[idxInfantry]  = 
		pINI->ReadBool(section, "Wave.ReverseAgainstInfantry", pData->Wave_Reverse[idxInfantry]);
	pData->Wave_Reverse[idxOther]  = 
		pINI->ReadBool(section, "Wave.ReverseAgainstOthers", pData->Wave_Reverse[idxOther]);

/*
	pData->Wave_InitialIntensity = pINI->ReadInteger(section, "Wave.InitialIntensity", pData->Wave_InitialIntensity);
	pData->Wave_IntensityStep    = pINI->ReadInteger(section, "Wave.IntensityStep", pData->Wave_IntensityStep);
	pData->Wave_FinalIntensity   = pINI->ReadInteger(section, "Wave.FinalIntensity", pData->Wave_FinalIntensity);
*/

	if(!pThis->get_Warhead())
	{
		DEBUGLOG("Weapon %s doesn't have a Warhead yet, what gives?\n", section);
		return;
	}

	if(pThis->get_Warhead()->get_IvanBomb())
	{
		pData->Ivan_KillsBridges = pINI->ReadBool(section, "IvanBomb.DestroysBridges", pData->Ivan_KillsBridges);
		pData->Ivan_Detachable   = pINI->ReadBool(section, "IvanBomb.Detachable", pData->Ivan_Detachable);

		pData->Ivan_Damage       = pINI->ReadInteger(section, "IvanBomb.Damage", pData->Ivan_Damage);
		pData->Ivan_Delay        = pINI->ReadInteger(section, "IvanBomb.Delay", pData->Ivan_Delay);

		int flicker = pINI->ReadInteger(section, "IvanBomb.FlickerRate", pData->Ivan_FlickerRate);
		if(flicker)
		{
			pData->Ivan_FlickerRate  = flicker;
		}

		PARSE_SND("IvanBomb.TickingSound", pData->Ivan_TickingSound);

		PARSE_SND("IvanBomb.AttachSound", pData->Ivan_AttachSound);

		PARSE_WH("IvanBomb.Warhead", pData->Ivan_WH);
		
		pINI->ReadString(section, "IvanBomb.Image", "", buffer, 256);
		if(strlen(buffer))
		{
			SHPStruct *image = FileSystem::LoadSHPFile(buffer);
			if(image)
			{
				DEBUGLOG("Loading Ivan Image %s succeeded: %d frames\n", buffer, image->Frames);
				pData->Ivan_Image        = image;
			}
			else
			{
			}
		}
	}
	
	if(pThis->get_RadLevel()) {
//		pData->Beam_Duration     = pINI->ReadInteger(section, "Beam.Duration", pData->Beam_Duration);

		PARSE_WH("Rad.Warhead", pData->Rad_WH);
		PARSE_COLOR("Rad.Color", pData->Rad_Color, tmpColor);
		pData->Rad_Duration_Multiple = pINI->ReadInteger(section, "Rad.DurationMultiple", pData->Rad_Duration_Multiple);
		pData->Rad_Application_Delay = pINI->ReadInteger(section, "Rad.ApplicationDelay", pData->Rad_Application_Delay);
		pData->Rad_Level_Max    = pINI->ReadInteger(section, "Rad.LevelMax", pData->Rad_Level_Max);
		pData->Rad_Level_Delay  = pINI->ReadInteger(section, "Rad.LevelDelay", pData->Rad_Level_Delay);
		pData->Rad_Light_Delay  = pINI->ReadInteger(section, "Rad.LightDelay", pData->Rad_Light_Delay);
		pData->Rad_Level_Factor = pINI->ReadDouble(section, "Rad.LevelFactor", pData->Rad_Level_Factor);
		pData->Rad_Light_Factor = pINI->ReadDouble(section, "Rad.LightFactor", pData->Rad_Light_Factor);
		pData->Rad_Tint_Factor  = pINI->ReadDouble(section, "Rad.TintFactor", pData->Rad_Tint_Factor);
	}
}

// 6FD64A, 6
DEFINE_HOOK(6FD64A, TechnoClass_FireRadBeam1, 6)
{
	byte idxWeapon = *(byte *)(R->get_StackVar32(0x18) + 0xC);
/*
	TechnoClass *Techno = (TechnoClass *)R->get_StackVar32(0x14);

	TechnoClassExt::Ext_p[Techno]->idxSlot_Beam = idxWeapon;
*/
	R->set_StackVar32(0x0, idxWeapon);
	return 0;
}

// 6FD79C, 6
// custom RadBeam colors
DEFINE_HOOK(6FD79C, TechnoClass_FireRadBeam2, 6)
{
	GET(RadBeam *, Rad, ESI);
	WeaponTypeClass* Source = (WeaponTypeClass *)R->get_StackVar32(0xC);
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::Ext_p, Source));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::Ext_p[Source];
	if(pData->Beam_IsHouseColor)
	{
		GET(TechnoClass *, SourceUnit, EDI);
		Rad->set_Color(SourceUnit->get_Owner()->get_Color());
	}
	else
	{
		Rad->set_Color(&pData->Beam_Color);
	}
	Rad->set_Period(pData->Beam_Duration);
	Rad->set_Amplitude(pData->Beam_Amplitude);
	return 0x6FD7A8;
}

// 438F8F, 6
// custom ivan bomb attachment 1
DEFINE_HOOK(438F8F, BombListClass_Add1, 6)
{
	GET(BombClass *, Bomb, ESI);

	BulletClass* Bullet = (BulletClass *)R->get_StackVar32(0x0);
	WeaponTypeClass *Source = Bullet->get_WeaponType();
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::Ext_p, Source));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::Ext_p[Source];

	WeaponTypeClassExt::BombExt[Bomb] = pData;
	Bomb->set_DetonationFrame(Unsorted::CurrentFrame + pData->Ivan_Delay);
	Bomb->set_TickSound(pData->Ivan_TickingSound);
	return 0;
}

// 438FD1, 5
// custom ivan bomb attachment 2
DEFINE_HOOK(438FD1, BombListClass_Add2, 5)
{
	GET(BombClass *, Bomb, ESI);
	BulletClass* Bullet = (BulletClass *)R->get_StackVar32(0x0);
	GET(TechnoClass *, Owner, EBP);
	WeaponTypeClass *Source = Bullet->get_WeaponType();
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::Ext_p, Source));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::Ext_p[Source];
	RET_UNLESS(Owner->get_Owner()->ControlledByPlayer());

	if(pData->Ivan_AttachSound != -1)
	{
		VocClass::PlayAt(pData->Ivan_AttachSound, Bomb->get_TargetUnit()->get_Location(), Bomb->get_Audio());
	}

	return 0;
}

// 438FD7, 7
// custom ivan bomb attachment 3
DEFINE_HOOK(438FD7, BombListClass_Add3, 7)
{
	return 0x439022;
}

// 6F5230, 5
// custom ivan bomb drawing 1
DEFINE_HOOK(6F5230, TechnoClass_DrawExtras1, 5)
{
	GET(TechnoClass *, pThis, EBP);
	BombClass * Bomb = pThis->get_AttachedBomb();

	RET_UNLESS(CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];

	if(pData->Ivan_Image->Frames < 2)
	{
		R->set_EAX(0);
		return 0x6F5235;
	}

	int frame = 
	(Unsorted::CurrentFrame - Bomb->get_PlantingFrame())
		/ (pData->Ivan_Delay / (pData->Ivan_Image->Frames - 1)); // -1 so that last iteration has room to flicker

	if(Unsorted::CurrentFrame % (2 * pData->Ivan_FlickerRate) >= pData->Ivan_FlickerRate)
	{
		++frame;
	}

	if( frame >= pData->Ivan_Image->Frames )
	{
		frame = pData->Ivan_Image->Frames - 1;
	}
	else if(frame == pData->Ivan_Image->Frames - 1 )
	{
		--frame;
	}

	R->set_EAX(frame);

	return 0x6F5235;
}

// 6F523C, 5
// custom ivan bomb drawing 2
DEFINE_HOOK(6F523C, TechnoClass_DrawExtras2, 5)
{
	GET(TechnoClass *, pThis, EBP);
	BombClass * Bomb = pThis->get_AttachedBomb();

	RET_UNLESS(CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];

	if(!pData->Ivan_Image)
	{
		return 0;
	}

	DWORD pImage = (DWORD)pData->Ivan_Image;

	R->set_ECX(pImage);
	return 0x6F5247;
}

// 6FCBAD, 6
// custom ivan bomb disarm 1
DEFINE_HOOK(6FCBAD, TechnoClass_GetObjectActivityState, 6)
{
	GET(TechnoClass *, Target, EBP);
	BombClass *Bomb = Target->get_AttachedBomb();
	RET_UNLESS(Bomb && CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];
	if(!pData->Ivan_Detachable)
	{
		return 0x6FCBBE;
	}
	return 0;
}

// 51E488, 5
DEFINE_HOOK(51E488, InfantryClass_GetCursorOverObject2, 5)
{
	GET(TechnoClass *, Target, ESI);
	BombClass *Bomb = Target->get_AttachedBomb();
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];
	if(!pData->Ivan_Detachable)
	{
		return 0x51E49E;
	}
	return 0;
}

// 438799, 6
// custom ivan bomb detonation 1
DEFINE_HOOK(438799, BombClass_Detonate1, 6)
{
	GET(BombClass *, Bomb, ESI);
	
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];

	R->set_StackVar32(0x4, (DWORD)pData->Ivan_WH);
	R->set_EDX((DWORD)pData->Ivan_Damage);
	return 0x43879F;
}

// 438843, 6
// custom ivan bomb detonation 2
DEFINE_HOOK(438843, BombClass_Detonate2, 6)
{
	GET(BombClass *, Bomb, ESI);
	
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];

	R->set_EDX((DWORD)pData->Ivan_WH);
	R->set_ECX((DWORD)pData->Ivan_Damage);
	return 0x438849;
}

// 438879, 6
// custom ivan bomb detonation 3
DEFINE_HOOK(438879, BombClass_Detonate3, 6)
{
	GET(BombClass *, Bomb, ESI);

	RET_UNLESS(CONTAINS(WeaponTypeClassExt::BombExt, Bomb));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::BombExt[Bomb];
	return pData->Ivan_KillsBridges ? 0 : 0x438989;
}

// 4393F2, 5
// custom ivan bomb cleanup
DEFINE_HOOK(4393F2, BombClass_SDDTOR, 5)
{
	GET(BombClass *, Bomb, ECX);
	hash_bombExt::iterator i = WeaponTypeClassExt::BombExt.find(Bomb);
	if(i != WeaponTypeClassExt::BombExt.end())
	{
		WeaponTypeClassExt::BombExt[Bomb] = 0;
		WeaponTypeClassExt::BombExt.erase(Bomb);
	}
	return 0;
}

// custom beam styles
// 6FF5F5, 6
DEFINE_HOOK(6FF5F5, TechnoClass_Fire, 6)
{
	GET(WeaponTypeClass *, Source, EBX);
	GET(TechnoClass *, Owner, ESI);
	GET(TechnoClass *, Target, EDI);

	byte idxWeapon = R->get_BaseVar8(0xC);

	TechnoClassExt::Ext_p[Owner]->idxSlot_Wave = idxWeapon;

	RET_UNLESS(CONTAINS(WeaponTypeClassExt::Ext_p, Source));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::Ext_p[Source];

	RET_UNLESS(pData->Wave_IsLaser || pData->Wave_IsBigLaser);

	DWORD xyzS = R->lea_StackVar(0x44); // need address of stack vars
	DWORD xyzT = R->lea_StackVar(0x88);

	CoordStruct *xyzSrc = (CoordStruct *)xyzS, *xyzTgt = (CoordStruct *)xyzT;

	WaveClass *Wave = new WaveClass(xyzSrc, xyzTgt, Owner, pData->Wave_IsBigLaser ? 2 : 1, Target);
	WeaponTypeClassExt::WaveExt[Wave] = pData;
	Owner->set_Wave(Wave);
	return 0x6FF650;
}

// 75E963, 6
DEFINE_HOOK(75E963, WaveClass_CTOR, 6)
{
	GET(WaveClass *, Wave, ESI);
	DWORD Type = R->get_ECX();
	if(Type == wave_Laser || Type == wave_BigLaser)
	{
		return 0;
	}
	GET(WeaponTypeClass *, Weapon, EBX);
	RET_UNLESS(Weapon && CONTAINS(WeaponTypeClassExt::Ext_p, Weapon));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::Ext_p[Weapon];
	WeaponTypeClassExt::WaveExt[Wave] = pData;
	return 0;
}

/*
// 75EB87, 0A // fsdblargh, a single instruction spanning 10 bytes
XPORT_FUNC(WaveClass_CTOR2)
{
	GET(WaveClass *, Wave, ESI);
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::WaveExt, Wave));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::WaveExt[Wave];
//	Wave->set_WaveIntensity(pData->Wave_InitialIntensity);
	return 0x75EB91;
}
*/

// 763226, 6
DEFINE_HOOK(763226, WaveClass_DTOR, 6)
{
	GET(WaveClass *, Wave, EDI);
	hash_waveExt::iterator i = WeaponTypeClassExt::WaveExt.find(Wave);
	if(i != WeaponTypeClassExt::WaveExt.end())
	{
			WeaponTypeClassExt::WaveExt.erase(i);
	}

/*
 * dangerous - is the dtor sure to be called before the owner creates a new wave ?
	hash_waveSlots::iterator j = WeaponTypeClassExt::WaveSlots.find(Wave->get_Owner());
	if(j != WeaponTypeClassExt::WaveSlots.end())
	{
		WeaponTypeClassExt::WaveSlots.erase(j);
	}
*/

	return 0;
}

// 760F50, 6
// complete replacement for WaveClass::Update
DEFINE_HOOK(760F50, WaveClass_Update, 6)
{
	GET(WaveClass *, pThis, ECX);

	RET_UNLESS(CONTAINS(WeaponTypeClassExt::WaveExt, pThis));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::WaveExt[pThis];

	int Intensity;

	if(pData->Weapon_Source->get_AmbientDamage())
	{
		CoordStruct coords;
		for(int i = 0; i < pThis->get_Cells()->get_Count(); ++i)
		{
			pThis->DamageArea(pThis->get_Cells()->GetItem(i)->Get3DCoords3(&coords));
		}
	}

	switch(pThis->get_Type())
	{
		case wave_Sonic:
			pThis->Update_Wave();
			Intensity = pThis->get_WaveIntensity();
			--Intensity;
			pThis->set_WaveIntensity(Intensity);
			if(Intensity < 0)
			{
				pThis->UnInit();
			}
			else
			{
				SET_REG32(ECX, pThis);
				CALL(0x5F3E70); // ObjectClass::Update
			}
			break;
		case wave_BigLaser:
		case wave_Laser:
			Intensity = pThis->get_LaserIntensity();
			Intensity -= 6;
			pThis->set_LaserIntensity(Intensity);
			if(Intensity < 32)
			{
				pThis->UnInit();
			}
			break;
		case wave_Magnetron:
			pThis->Update_Wave();
			Intensity = pThis->get_WaveIntensity();
			--Intensity;
			pThis->set_WaveIntensity(Intensity);
			if(Intensity < 0)
			{
				pThis->UnInit();
			}
			else
			{
				SET_REG32(ECX, pThis);
				CALL(0x5F3E70); // ObjectClass::Update
			}
			break;
	}

	return 0x76101A;
}

/*
// 760FFC, 5
// Alt beams update
XPORT_FUNC(WaveClass_UpdateLaser)
{
	GET(WaveClass *, Wave, ESI);
	Wave->Update_Beam();
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::WaveExt, Wave));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::WaveExt[Wave];
	int intense = Wave->get_WaveIntensity() + pData->Wave_IntensityStep;
	Wave->set_WaveIntensity(intense);
	return intense >= pData->Wave_FinalIntensity ? 0x761016 : 0x76100C;
}
*/

// 760BC2, 6
DEFINE_HOOK(760BC2, WaveClass_Draw2, 6)
{
	GET(WaveClass *, Wave, EBX);
	GET(WORD *, dest, EBP);

	WeaponTypeClassExt::ModifyWaveColor(dest, dest, Wave->get_LaserIntensity(), Wave);

	return 0x760CAF;
}

// 760DE2, 6
DEFINE_HOOK(760DE2, WaveClass_Draw3, 6)
{
	GET(WaveClass *, Wave, EBX);
	GET(WORD *, dest, EDI);

	WeaponTypeClassExt::ModifyWaveColor(dest, dest, Wave->get_LaserIntensity(), Wave);

	return 0x760ECB;
}

// 75EE57, 7
DEFINE_HOOK(75EE57, WaveClass_Draw_Sonic, 7)
{
	DWORD pWave = R->get_StackVar32(0x4);
	WaveClass * Wave = (WaveClass *)pWave;
	DWORD src = R->get_EDI();
	DWORD offs = src + R->get_ECX() * 2;

	WeaponTypeClassExt::ModifyWaveColor((WORD *)offs, (WORD *)src, /* Wave->get_WaveIntensity() */ R->get_ESI(), Wave);

	return 0x75EF1C;
}

// 7601FB, 0B
DEFINE_HOOK(7601FB, WaveClass_Draw_Magnetron, 0B)
{
	DWORD pWave = R->get_StackVar32(0x8);
	WaveClass * Wave = (WaveClass *)pWave;
	DWORD src = R->get_EBX();
	DWORD offs = src + R->get_ECX() * 2;

	WeaponTypeClassExt::ModifyWaveColor((WORD *)offs, (WORD *)src, /*Wave->get_WaveIntensity(), */ R->get_EBP(), Wave);

	return 0x760285;
}

// 760286, 5
DEFINE_HOOK(760286, WaveClass_Draw_Magnetron2, 5)
{
	return 0x7602D3;
}

void WeaponTypeClassExt::ModifyWaveColor(WORD *src, WORD *dst, int Intensity, WaveClass *Wave)
{
	RETZ_UNLESS(CONTAINS(WeaponTypeClassExt::WaveExt, Wave));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::WaveExt[Wave];

	ColorStruct *CurrentColor = (pData->Wave_IsHouseColor && Wave->get_Owner())
		? Wave->get_Owner()->get_Owner()->get_Color()
		: &pData->Wave_Color;

	ColorStruct initial = Drawing::WordColor(*src);

	ColorStruct modified = initial;

// ugly hack to fix byte wraparound problems
#define upcolor(c) \
	int _ ## c = initial. c + (Intensity * CurrentColor-> c ) / 256; \
	_ ## c = min(_ ## c, 255); \
	modified. c = (BYTE)_ ## c;

	upcolor(R);
	upcolor(G);
	upcolor(B);

	WORD color = Drawing::Color16bit(&modified);

	*dst = color;
}

// 762C5C, 6
DEFINE_HOOK(762C5C, WaveClass_Update_Wave, 6)
{
	GET(WaveClass *, Wave, ESI);
	TechnoClass *Firer = Wave->get_Owner();
	TechnoClass *Target = Wave->get_Target();
	if(!Target || !Firer)
	{
		return 0x762D57;
	}

	RET_UNLESS(CONTAINS(WeaponTypeClassExt::WaveExt, Wave));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::WaveExt[Wave];
	int weaponIdx = TechnoClassExt::Ext_p[Firer]->idxSlot_Wave;

	CoordStruct xyzSrc, xyzTgt;
	Firer->GetFLH(&xyzSrc, weaponIdx, 0, 0, 0);
	Target->GetCoords__(&xyzTgt); // not GetCoords() ! 

	char idx = WeaponTypeClassExt:: AbsIDtoIdx(Target->WhatAmI());

	bool reversed = pData->Wave_Reverse[idx];

	if(Wave->get_Type() == wave_Magnetron)
	{
		reversed
			? Wave->Draw_Magnetic(&xyzTgt, &xyzSrc)
			: Wave->Draw_Magnetic(&xyzSrc, &xyzTgt);
	}
	else
	{
		reversed
			? Wave->Draw_NonMagnetic(&xyzTgt, &xyzSrc)
			: Wave->Draw_NonMagnetic(&xyzSrc, &xyzTgt);
	}

	return 0x762D57;
}

// 75F38F, 6
DEFINE_HOOK(75F38F, WaveClass_DamageCell, 6)
{
	GET(WaveClass *, Wave, EBP);
	RET_UNLESS(CONTAINS(WeaponTypeClassExt::WaveExt, Wave));
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::WaveExt[Wave];
	R->set_EDI(R->get_EAX());
	R->set_EBX((DWORD)pData->Weapon_Source);
	return 0x75F39D;
}


DEFINE_HOOK(4691D5, RadSite_Create_1, 5)
{
	GET(RadSiteClass *, Rad, EDI);
	GET(BulletClass *, B, ESI);

	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::Ext_p[B->get_WeaponType()];
	WeaponTypeClassExt::RadSiteExt[Rad] = pData;

	// replacing code we abused
	GET(DWORD, XY, EAX);
	Rad->set_BaseCell((CellStruct*)&XY);
	return 0x4691DA;
}

DEFINE_HOOK(65B593, RadSiteClass_Radiate_0, 6)
{
	GET(RadSiteClass *, Rad, ECX);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	R->set_ECX(pData->Rad_Level_Delay);
	R->set_EAX(pData->Rad_Light_Delay);

	return 0x65B59F;
}


DEFINE_HOOK(65B5CE, RadSiteClass_Radiate_1, 6)
{
	GET(RadSiteClass *, Rad, ESI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	R->set_EAX(0);
	R->set_EBX(0);
	R->set_EDX(0);

	ColorStruct clr = pData->Rad_Color;

	// NOT MY HACK, HELLO WESTWEIRD
	if(ScenarioClass::Global()->get_Theater() == th_Snow) {
		clr.R = clr.B = 0x80;
	}

	R->set_DL(clr.G);
	R->set_EBP(R->get_EDX());

	R->set_BL(clr.B);

	R->set_AL(clr.R);

	return 0x65B604;
}

DEFINE_HOOK(65B63E, RadSiteClass_Radiate_2, 6)
{
	GET(RadSiteClass *, Rad, EDI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	double factor = pData->Rad_Light_Factor;
	__asm { fmul factor };

	return 0x65B644;
}

DEFINE_HOOK(65B6A0, RadSiteClass_Radiate_3, 6)
{
	GET(RadSiteClass *, Rad, EDI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	double factor = pData->Rad_Tint_Factor;
	__asm { fmul factor };
	return 0x65B6A6;
}

DEFINE_HOOK(65B6CA, RadSiteClass_Radiate_4, 6)
{
	GET(RadSiteClass *, Rad, EDI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	double factor = pData->Rad_Tint_Factor;
	__asm { fmul factor };
	return 0x65B6D0;
}


DEFINE_HOOK(65B6F2, RadSiteClass_Radiate_5, 6)
{
	GET(RadSiteClass *, Rad, EDI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	double factor = pData->Rad_Tint_Factor;
	__asm { fmul factor };
	return 0x65B6F8;
}


DEFINE_HOOK(65B73A, RadSiteClass_Radiate_6, 5)
{
	if(!R->get_EAX()) {
		R->set_EAX(1);
	}
	return 0;
}


DEFINE_HOOK(65B843, RadSiteClass_Update_1, 6)
{
	GET(RadSiteClass *, Rad, ESI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	R->set_ECX(pData->Rad_Level_Delay);
	return 0x65B849;
}

DEFINE_HOOK(65B8B9, RadSiteClass_Update_2, 6)
{
	GET(RadSiteClass *, Rad, ESI);
	WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];

	R->set_ECX(pData->Rad_Light_Delay);
	return 0x65B8BF;
}

/* -- doesn't work logically
FINE_HOOK(487CB0, CellClass_GetRadLevel, 5)
{
	GET(CellClass *, Cell, ECX);
	RadSiteClass * Rad = Cell->get_RadSite();
	if(!Rad) {
		R->set_EAX(0);
	} else {
		WeaponTypeClassExt::WeaponTypeClassData *pData = WeaponTypeClassExt::RadSiteExt[Rad];
		double RadLevel = Cell->get_RadLevel();
		if(pData->Rad_Level_Max < RadLevel) {
			RadLevel = pData->Rad_Level_Max;
		}
		R->set_EAX(FloatToInt(RadLevel));
	}
	return 0x487E39;
}
*/