#include "../Utilities/Checksummer.h"
#include <Utilities/Macro.h>   // STACK_OFFS
#include "../Ext/Abstract/Body.h"
#include <AnimClass.h>
#include <CellClass.h>
#include <ColorScheme.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>
#include <BuildingClass.h>
#include <AircraftClass.h>
#include "Networking.h"
#include <ScenarioClass.h>
#include <SessionClass.h>
#include <FPSCounter.h>
#include <GameOptionsClass.h>
#include "../Ares.version.h"

template<typename T>
void WriteLog(const T* it, int idx, DWORD checksum, FILE * F) {
	fprintf(F, "#%05d:\t%08X", idx, checksum);
}

template<>
void WriteLog(const AbstractClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<void>(it, idx, checksum, F);
	auto abs = it->WhatAmI();
	fprintf(F, "; Abs: %u (%s)", abs, AbstractClass::GetRTTIName(abs));
}

template<>
void WriteLog(const ObjectClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<AbstractClass>(it, idx, checksum, F);

	const char* typeID = "<None>";
	int typeIndex = -1;
	if(auto pType = it->GetType()) {
		typeID = pType->ID;
		typeIndex = pType->GetArrayIndex();
	}

	CoordStruct crd = it->GetCoords();
	CellStruct cell = CellClass::Coord2Cell(crd);

	fprintf(F, "; Type: %d (%s); Coords: %d,%d,%d (%d,%d); Health: %d; InLimbo: %u",
		typeIndex, typeID, crd.X, crd.Y, crd.Z, cell.X, cell.Y, it->Health, it->InLimbo);
}

template<>
void WriteLog(const MissionClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<ObjectClass>(it, idx, checksum, F);
	fprintf(F, "; Mission: %d; StartTime: %d",
		it->GetCurrentMission(), it->CurrentMissionStartTime);
}

template<>
void WriteLog(const RadioClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<MissionClass>(it, idx, checksum, F);
	//fprintf(F, "; LastCommand: %d", it->LastCommands[0]);
}

template<>
void WriteLog(const TechnoClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<RadioClass>(it, idx, checksum, F);

	const char* targetID = "<None>";
	int targetIndex = -1;
	CoordStruct targetCrd = {-1, -1, -1};
	if(auto pTarget = it->Target) {
		targetID = AbstractClass::GetRTTIName(pTarget->WhatAmI());
		targetIndex = pTarget->GetArrayIndex();
		targetCrd = pTarget->GetCoords();
	}

	fprintf(F, "; Facing: %d; Facing2: %d; Target: %s (%d; %d,%d)",
		it->PrimaryFacing.Current().GetFacing<8>(), it->SecondaryFacing.Current().GetFacing<8>(), targetID, targetIndex, targetCrd.X, targetCrd.Y);
}

template<>
void WriteLog(const FootClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<TechnoClass>(it, idx, checksum, F);

	const char* destID = "<None>";
	int destIndex = -1;
	CoordStruct destCrd = {-1, -1, -1};
	if(auto pDest = it->Destination) {
		destID = AbstractClass::GetRTTIName(pDest->WhatAmI());
		destIndex = pDest->GetArrayIndex();
		destCrd = pDest->GetCoords();
	}

	fprintf(F, "; Destination: %s (%d; %d,%d)",
		destID, destIndex, destCrd.X, destCrd.Y);
}

template<>
void WriteLog(const InfantryClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<FootClass>(it, idx, checksum, F);
	fprintf(F, "; Speed %d", Game::F2I(it->SpeedPercentage * 256));
}

template<>
void WriteLog(const UnitClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<FootClass>(it, idx, checksum, F);

	const auto& Loco = it->Locomotor;
	auto accum = Loco->Get_Speed_Accum();
	auto index = Loco->Get_Track_Index();
	auto number = Loco->Get_Track_Number();

	fprintf(F, "; Speed %d; TrackNumber: %d; TrackIndex: %d", accum, number, index);
}

template<>
void WriteLog(const AircraftClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<FootClass>(it, idx, checksum, F);
	fprintf(F, "; Speed %d; Height: %d", Game::F2I(it->SpeedPercentage * 256), it->GetHeight());
}

template<>
void WriteLog(const BuildingClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<TechnoClass>(it, idx, checksum, F);
}

template<>
void WriteLog(const AbstractTypeClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<AbstractClass>(it, idx, checksum, F);
	fprintf(F, "; ID: %s; Name: %s", it->ID, it->Name);
}

template<>
void WriteLog(const HouseClass* it, int idx, DWORD checksum, FILE * F) {
	WriteLog<void>(it, idx, checksum, F);

	fprintf(F, "; CurrentPlayer: %u; ColorScheme: %s; ID: %d; HouseType: %s; Edge: %d; StartingAllies: %u; Startspot: %d,%d; Visionary: %d; MapIsClear: %u; Money: %d",
		it->IsHumanPlayer, ColorScheme::Array.GetItem(it->ColorSchemeIndex)->ID,
		it->ArrayIndex, HouseTypeClass::Array.GetItem(it->Type->ArrayIndex)->Name,
		it->Edge, it->StartingAllies, it->StartingCell.X, it->StartingCell.Y, it->Visionary,
		it->MapIsClear, it->Available_Money());
}

// calls WriteLog and appends a newline
template<typename T>
void WriteLogLine(const T* it, int idx, DWORD checksum, FILE * F) {
	WriteLog(it, idx, checksum, F);
	fprintf(F, "\n");
}

template<typename T>
void LogItem(const T* it, int idx, FILE * F) {
	if(it->WhatAmI() != AnimClass::AbsID || it->Fetch_ID() != -2) {
		SafeChecksummer Ch;
		it->ComputeCRC(Ch);
		WriteLogLine(it, idx, Ch.Intermediate(), F);
	}
}

template<typename T>
void VectorLogger(const DynamicVectorClass<T> *Array, FILE * F, const char * Label = nullptr) {
	if(Label) {
		fprintf(F, "Checksums for [%s] (%d)\n", Label, Array ? Array->Count : -1);
	}
	if(Array) {
		for(auto i = 0; i < Array->Count; ++i) {
			auto it = Array->Items[i];
			LogItem(it, i, F);
		}
	} else {
		fprintf(F, "Array not initialized yet...\n");
	}
}

template<typename T>
void HouseLogger(const DynamicVectorClass<T> *Array, FILE * F, const char * Label = nullptr) {
	if(Array) {
		for(auto j = 0; j < HouseClass::Array.Count; ++j) {
			auto pHouse = HouseClass::Array.GetItem(j);
			fprintf(F, "-------------------- %s (%d) %s -------------------\n", pHouse->Type->Name, j, Label ? Label : "");

			for(auto i = 0; i < Array->Count; ++i) {
				auto it = Array->Items[i];

				if(it->Owner == pHouse) {
					LogItem(it, i, F);
				}
			}
		}
	} else {
		fprintf(F, "Array not initialized yet...\n");
	}
}

bool LogFrame(const char * LogFilename, EventClass *OffendingEvent = nullptr) {
	FILE* LogFile = nullptr;
	if(!fopen_s(&LogFile, LogFilename, "wt") && LogFile) {
		std::setvbuf(LogFile, nullptr, _IOFBF, 1024 * 1024); // 1024 kb buffer - should be sufficient for whole log

		fprintf(LogFile, "Ares synchronization log (version " VERSION_STR ")\n");

		for(auto ixF = 0; ixF < 0x100; ++ixF) {
			fprintf(LogFile, "CRC[%02X] = %08X\n", ixF, EventClass::LatestFramesCRC[ixF]);
		}

		fprintf(LogFile, "My Random Number: %08X\n", ScenarioClass::Instance->Random.Random());
		fprintf(LogFile, "My Frame: %08X\n", Unsorted::CurrentFrame);
		fprintf(LogFile, "Average FPS: %d\n", Game::F2I(FPSCounter::GetAverageFrameRate()));
		fprintf(LogFile, "Max MaxAhead: %d\n", *reinterpret_cast<int*>(0xA8B568));
		fprintf(LogFile, "Latency setting: %d\n", *reinterpret_cast<int*>(0xA8DB9C));
		fprintf(LogFile, "Game speed setting: %d\n", GameOptionsClass::Instance.GameSpeed);
		fprintf(LogFile, "FrameSendRate: %d\n", *reinterpret_cast<int*>(0xA8B554));
		
		if(OffendingEvent) {
			fprintf(LogFile, "\nOffending event:\n");
			fprintf(LogFile, "Type:         %X\n", static_cast<unsigned>(OffendingEvent->Type));
			fprintf(LogFile, "Frame:        %X\n", OffendingEvent->Frame);
			fprintf(LogFile, "ID:           %X\n", OffendingEvent->HouseIndex);
			fprintf(LogFile, "CRC:          %X\n", OffendingEvent->FrameInfo.CRC);
			fprintf(LogFile, "CommandCount: %hu\n", OffendingEvent->FrameInfo.CommandCount);
			fprintf(LogFile, "Delay:        %hhu\n", OffendingEvent->FrameInfo.Delay);
			fprintf(LogFile, "\n\n");
		}

		fprintf(LogFile, "\nTypes\n");
		HouseLogger(&InfantryClass::Array, LogFile, "Infantry");
		HouseLogger(&UnitClass::Array, LogFile, "Units");
		HouseLogger(&AircraftClass::Array, LogFile, "Aircraft");
		HouseLogger(&BuildingClass::Array, LogFile, "Buildings");

		fprintf(LogFile, "\nChecksums\n");
		VectorLogger(&HouseClass::Array, LogFile, "Houses");
		VectorLogger(&InfantryClass::Array, LogFile, "Infantry");
		VectorLogger(&UnitClass::Array, LogFile, "Units");
		VectorLogger(&AircraftClass::Array, LogFile, "Aircraft");
		VectorLogger(&BuildingClass::Array, LogFile, "Buildings");

		fprintf(LogFile, "\n");
		VectorLogger(&ObjectClass::CurrentObjects, LogFile, "Current Objects");
		VectorLogger(&LogicClass::Instance, LogFile, "Logics");

		fprintf(LogFile, "\nChecksums for Map Layers\n");
		for(auto ixL = 0; ixL < 5; ++ixL) {
			fprintf(LogFile, "Checksums for Layer %d\n", ixL);
			VectorLogger(&MapClass::ObjectsInLayers[ixL], LogFile);
		}

		fprintf(LogFile, "\nChecksums for Logics\n");
		VectorLogger(&LogicClass::Instance, LogFile);
		
		fprintf(LogFile, "\nChecksums for Abstracts\n");
		VectorLogger(&AbstractClass::Array, LogFile, "Abstracts");
		VectorLogger(&AbstractTypeClass::Array, LogFile, "AbstractTypes");

		fclose(LogFile);
		return true;
	} else {
		Debug::Log("Failed to open file for sync log. Error code %X.\n", errno);
		return false;
	}
}

DEFINE_HOOK(0x64DEA0, Multiplay_LogToSYNC_NOMPDEBUG, 0x0)
{
	GET(EventClass *, OffendingEvent, ECX);
	
	char LogFilename[0x40];
	_snprintf_s(LogFilename, _TRUNCATE, "SYNC%01d.TXT", HouseClass::CurrentPlayer->ArrayIndex);

	LogFrame(LogFilename, OffendingEvent);

	return 0x64DF3D;
}

DEFINE_HOOK(0x6516F0, Multiplay_LogToSync_MPDEBUG, 0x6)
{
	GET(int, SlotNumber, ECX);
	GET(EventClass *, OffendingEvent, EDX);
	
	char LogFilename[0x40];
	_snprintf_s(LogFilename, _TRUNCATE, "SYNC%01d_%03d.TXT", HouseClass::CurrentPlayer->ArrayIndex, SlotNumber);

	LogFrame(LogFilename, OffendingEvent);

	return 0x651781;
}

// replace the original checksummer functions with own implementations that do
// not exhibit the out of array bounds writes.

DEFINE_HOOK(0x4A1C10, Checksummer_Add_BYTE, 0x5)
{
	GET(Checksummer*, pThis, ECX);
	REF_STACK(const BYTE, value, STACK_OFFS(0x0, -0x4));

	pThis->Add(value);

	return 0x4A1C8E;
}

DEFINE_HOOK(0x4A1CA0, Checksummer_Add_bool, 0x5)
{
	GET(Checksummer*, pThis, ECX);
	REF_STACK(const bool, value, STACK_OFFS(0x0, -0x4));

	pThis->Add(value);

	return 0x4A1D23;
}

DEFINE_HOOK(0x4A1D30, Checksummer_Add_WORD, 0x5)
{
	GET(Checksummer*, pThis, ECX);
	REF_STACK(const WORD, value, STACK_OFFS(0x0, -0x4));

	pThis->Add(value);

	return 0x4A1D46;
}

DEFINE_HOOK(0x4A1D50, Checksummer_Add_DWORD, 0x8)
{
	GET(Checksummer*, pThis, ECX);
	REF_STACK(const DWORD, value, STACK_OFFS(0x0, -0x4));

	pThis->Add(value);

	return 0x4A1D64;
}

DEFINE_HOOK(0x4A1D70, Checksummer_Add_float, 0x8)
{
	GET(Checksummer*, pThis, ECX);
	REF_STACK(const float, value, STACK_OFFS(0x0, -0x4));

	pThis->Add(value);

	return 0x4A1D84;
}

DEFINE_HOOK(0x4A1D90, Checksummer_Add_double, 0x8)
{
	GET(Checksummer*, pThis, ECX);
	REF_STACK(const double, value, STACK_OFFS(0x0, -0x4));

	pThis->Add(value);

	return 0x4A1DAC;
}

DEFINE_HOOK(0x4A1DE0, Checksummer_Add_Buffer, 0x6)
{
	GET(Checksummer*, pThis, ECX);
	GET_STACK(const void*, data, STACK_OFFS(0x0, -0x4));
	GET_STACK(size_t, length, STACK_OFFS(0x0, -0x8));

	pThis->Add(data, length);

	R->EAX(pThis->GetValue());
	return 0x4A1FA6;
}

struct ChecksumSet
{
	int Rules;
	int Art;
	int AI;
};

// the checksums the join dialog recorded for the session, with the local ones
// as fallback
static ChecksumSet CalculateChecksums()
{
	auto const address = (SessionClass::Instance.GameMode == GameMode::LAN)
		? 0xAC026Cu : 0xB77E00u;

	auto ret = *reinterpret_cast<ChecksumSet const*>(address);

	if(!ret.Rules) {
		ret.Rules = reinterpret_cast<int(__cdecl*)()>(0x679D90)();
	}
	if(!ret.Art) {
		ret.Art = reinterpret_cast<int(__cdecl*)()>(0x679EC0)();
	}
	if(!ret.AI) {
		ret.AI = reinterpret_cast<int(__cdecl*)()>(0x679ED0)();
	}

	return ret;
}

DEFINE_HOOK(0x52E9AA, Frontend_WndProc_Checksum, 0x5)
{
	auto const mode = SessionClass::Instance.GameMode;

	if(mode == GameMode::LAN || mode == GameMode::Internet) {
		auto const sums = CalculateChecksums();

		Debug::Log("Rules checksum: %08X\n", sums.Rules);
		Debug::Log("Art checksum: %08X\n", sums.Art);
		Debug::Log("AI checksum: %08X\n", sums.AI);
	}

	return 0;
}
