#pragma once

#include "Networking.h"

class AresNetEvent {
public:
	enum class Events : unsigned char {
		TrenchRedirectClick = 0x60,
		FirewallToggle = 0x61,

		First = TrenchRedirectClick,
		Last = FirewallToggle
	};

	// the number of bytes that follow the event kind on the wire
	static int GetPayloadSize(byte kind);

	class Handlers {
	public:
		static void RaiseTrenchRedirectClick(BuildingClass *Source, CellStruct *Target);
		static void RespondToTrenchRedirectClick(EventClass *Event);

		static void RaiseFirewallToggle(HouseClass *Source);
		static void RespondToFirewallToggle(EventClass *Event);
	};
};
