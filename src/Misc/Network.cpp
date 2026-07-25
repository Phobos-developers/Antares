#include "Network.h"

#include "Debug.h"
#include "Exception.h"
#include "../Ares.h"

#include "../Ext/Building/Body.h"
#include "../Ext/House/Body.h"

#include <BuildingClass.h>
#include <HouseClass.h>
#include "Networking.h"
#include <TargetClass.h>

int AresNetEvent::GetPayloadSize(byte const kind) {
	auto const EventLength = reinterpret_cast<byte const*>(0x8208EC);

	if(kind <= 0x2E) {
		return EventLength[kind];
	}

	switch(static_cast<AresNetEvent::Events>(kind)) {
	case AresNetEvent::Events::TrenchRedirectClick:
		return 10;
	case AresNetEvent::Events::FirewallToggle:
		return 5;
	}

	return 0;
}

DEFINE_HOOK(0x64B704, sub_64B660_PayloadSize, 0x8)
{
	GET(byte const, kind, EDI);

	auto const size = AresNetEvent::GetPayloadSize(kind);
	R->EDX(size);
	R->EBP(size);

	return (kind == 0x1F) ? 0x64B710u : 0x64B71Du;
}

DEFINE_HOOK(0x64BE83, sub_64BDD0_PayloadSize1, 0x8)
{
	GET(byte const, kind, EDI);

	auto const size = AresNetEvent::GetPayloadSize(kind);
	R->ECX(size);
	R->EBP(size);
	R->Stack(0x20, size);

	return (kind == 0x04) ? 0x64BF1Au : 0x64BE97u;
}

DEFINE_HOOK(0x64C314, sub_64BDD0_PayloadSize2, 0x8)
{
	GET(byte const, kind, ESI);

	auto const size = AresNetEvent::GetPayloadSize(kind);
	R->ECX(size);
	R->EBP(size + (kind == 0x04));

	return 0x64C321;
}

DEFINE_HOOK(0x4C6CCD, Networking_RespondToEvent, 0x0)
{
	GET(DWORD, EventKind, EAX);
	GET(EventClass *, Event, ESI);

	auto kind = static_cast<AresNetEvent::Events>(EventKind);
	if(kind >= AresNetEvent::Events::First) {
		// Received Ares event, do something about it
		switch(kind) {
			case AresNetEvent::Events::TrenchRedirectClick:
				AresNetEvent::Handlers::RespondToTrenchRedirectClick(Event);
				break;
			case AresNetEvent::Events::FirewallToggle:
				AresNetEvent::Handlers::RespondToFirewallToggle(Event);
				break;
		}
	}

	--EventKind;
	R->EAX(EventKind);
	return (EventKind > 0x2D)
	 ? 0x4C8109
	 : 0x4C6CD7
	;
}


DEFINE_HOOK(0x64CCBF, DoList_ReplaceReconMessage, 0x6)
{
	// mimic an increment because decrement happens in the middle of function cleanup and can't be erased nicely
	int &TempMutex = *reinterpret_cast<int*>(0xA8DAB4);
	++TempMutex;

	Debug::Log("Reconnection error detected!");
	if(MessageBoxW(Game::hWnd, L"Yuri's Revenge has detected a desynchronization!\n"
			L"Would you like to create a full error report for the developers?\n"
			L"Be advised that reports from at least two players are needed.", L"Reconnection Error!", MB_YESNO | MB_ICONERROR) == IDYES) {
		HCURSOR loadCursor = LoadCursor(nullptr, IDC_WAIT);
		SetClassLong(Game::hWnd, GCL_HCURSOR, reinterpret_cast<LONG>(loadCursor));
		SetCursor(loadCursor);

		std::wstring path = Exception::PrepareSnapshotDirectory();

		if(Debug::bLog) {
			Debug::Log("Copying debug log\n");
			std::wstring logCopy = path + L"\\debug.log";
			CopyFileW(Debug::LogFileTempName.c_str(), logCopy.c_str(), FALSE);
		}

		Debug::Log("Making a memory snapshot\n");
		Debug::FullDump(std::move(path));

		loadCursor = LoadCursor(nullptr, IDC_ARROW);
		SetClassLong(Game::hWnd, GCL_HCURSOR, reinterpret_cast<LONG>(loadCursor));
		SetCursor(loadCursor);
		Debug::FatalError("A desynchronization has occurred.\r\n"
			"%s"
			"A crash dump should have been created in your game's \\debug subfolder.\r\n"
			"Please submit that to the developers along with SYNC*.txt, debug.txt and syringe.log."
				, Debug::bParserErrorDetected ? "(One or more parser errors have been detected that might be responsible. Check the debug logs.)\r\n" : ""
		);
	}

	return 0x64CD11;
}


/*
 how to raise your own events
	AresEvent Event(static_cast<EventType>(AresNetworkEvent::aev_blah), U->Owner->ArrayIndex);
	memcpy(Event->DataBuffer, "Boom de yada", 0xkcd);
	Networking::AddEvent(Event);
*/

void AresNetEvent::Handlers::RaiseTrenchRedirectClick(BuildingClass *Source, CellStruct *Target) {
	AresEvent Event(
		static_cast<EventType>(AresNetEvent::Events::TrenchRedirectClick),
		Source->Owner->ArrayIndex);

	// the payload of an Ares event starts at DataBuffer, i.e. event+7, right after
	// Type/IsExecuted/HouseIndex/Frame -- shipped Ares 3.0p1 writes the packed cell
	// to event+7 and the packed building to event+0xC
	// (AresNetEvent::Handlers::RaiseTrenchRedirectClick, Ares.dll 0x1006B150), and
	// Networking_RespondToEvent reads them back from [esi+7] and [esi+0Ch]
	// (0x1006B578, 0x1006B588). The pinned YRpp's NetworkEvent::ExtraData sat at
	// event+14, past the FRAMEINFO fields, which put every Ares event 7 bytes off
	// the wire layout the shipped build uses.
	byte *payload = reinterpret_cast<byte*>(Event->DataBuffer);

	TargetClass const TargetCoords(*Target);
	memcpy(payload, &TargetCoords, sizeof(TargetCoords));
	payload += sizeof(TargetCoords);

	TargetClass const SourceObject(Source);
	memcpy(payload, &SourceObject, sizeof(SourceObject));
	payload += sizeof(SourceObject);

	Networking::AddEvent(Event);
}

void AresNetEvent::Handlers::RespondToTrenchRedirectClick(EventClass *Event) {
	TargetClass *ID = reinterpret_cast<TargetClass *>(Event->DataBuffer);
	if(CellClass * pTargetCell = ID->As_Cell()) {
		++ID;
		if(BuildingClass * pSourceBuilding = ID->As_Building()) {
			/*
				pSourceBuilding == selected building the soldiers are in
				pTargetCell == cell the user clicked on; event fires only on buildings which showed the enter cursor
			*/
			BuildingExt::ExtData* sourceBuildingExt = BuildingExt::ExtMap.Find(pSourceBuilding);
			BuildingClass* targetBuilding = pTargetCell->GetBuilding();
			sourceBuildingExt->doTraverseTo(targetBuilding); // check has happened before the enter cursor appeared
		}
	}

}

void AresNetEvent::Handlers::RaiseFirewallToggle(HouseClass *Source) {
	AresEvent Event(
		static_cast<EventType>(AresNetEvent::Events::FirewallToggle),
		Source->ArrayIndex);

	Networking::AddEvent(Event);
}

void AresNetEvent::Handlers::RespondToFirewallToggle(EventClass *Event) {
	if(HouseClass * pSourceHouse = HouseClass::Array.GetItem(Event->HouseIndex)) {
		HouseExt::ExtData *pData = HouseExt::ExtMap.Find(pSourceHouse);
		bool FS = pSourceHouse->FirestormActive;
		FS = !FS;
		pData->SetFirestormState(FS);
	}
}
