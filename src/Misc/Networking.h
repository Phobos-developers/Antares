#pragma once

// Ares's own event-queue helper.

#include <EventClass.h>
#include <Fundamentals.h>

#include <cstring>

// Storage for an event Ares raises itself. EventClass has no default constructor
// -- its constructors are thunks that fill specific game event shapes, and Ares's
// own kinds (0x60 TrenchRedirectClick, 0x61 FirewallToggle) have none. Zeroing the
// whole record is what keeps the bytes nobody writes identical on every peer.
class AresEvent
{
public:
	explicit AresEvent(EventType type, int houseIndex)
	{
		std::memset(this->Bytes, 0, sizeof(this->Bytes));
		auto& event = **this;
		event.Type = type;
		event.HouseIndex = static_cast<char>(houseIndex);
	}

	EventClass& operator * () { return reinterpret_cast<EventClass&>(this->Bytes); }
	EventClass* operator -> () { return reinterpret_cast<EventClass*>(this->Bytes); }
	operator EventClass* () { return reinterpret_cast<EventClass*>(this->Bytes); }

private:
	unsigned char Bytes[sizeof(EventClass)];
};

class Networking
{
public:
	// Stamps the current frame and queues the event, exactly as the shipped
	// Ares 3.0p1 handlers do inline (see AresNetEvent::Handlers::RaiseTrenchRedirectClick
	// at Ares.dll 0x1006B150: it writes Unsorted::CurrentFrame to event+3, refuses
	// when OutList.Count reaches 128, copies 0x6F bytes to OutList.Array[Tail],
	// stores timeGetTime() into Timings[Tail], then advances Tail modulo 128).
	static bool AddEvent(EventClass* pEvent)
	{
		pEvent->Frame = static_cast<unsigned int>(Unsorted::CurrentFrame);
		return EventClass::OutList.Add(*pEvent);
	}
};
