#include "TunnelTypes.h"

#include "../Utilities/TemplateDef.h"

Enumerable<TunnelTypeClass>::container_t Enumerable<TunnelTypeClass>::Array;

const char * Enumerable<TunnelTypeClass>::GetMainSection()
{
	return "TunnelTypes";
}

TunnelTypeClass::TunnelTypeClass(const char* const pTitle)
	: Enumerable<TunnelTypeClass>(pTitle), Passengers(0), MaxSize(0.0)
{ }

TunnelTypeClass::~TunnelTypeClass() = default;

void TunnelTypeClass::LoadFromINIList(CCINIClass *pINI)
{
	const char *section = Enumerable<TunnelTypeClass>::GetMainSection();

	int len = pINI->GetKeyCount(section);
	for(int i = 0; i < len; ++i) {
		char buffer[32];
		buffer[0] = 0;

		const char *Key = pINI->GetKeyName(section, i);
		if(pINI->ReadString(section, Key, Ares::readDefval, buffer)) {
			FindOrAllocate(buffer);
		}
	}

	for(size_t i = 0; i < Array.size(); ++i) {
		Array[i]->LoadFromINI(pINI);
	}
}

void TunnelTypeClass::LoadFromINI(CCINIClass *pINI)
{
	const char *section = this->Name;

	if(!pINI->GetSection(section)) {
		return;
	}

	this->Passengers = pINI->ReadInteger(section, "Passengers", this->Passengers);
	this->MaxSize = pINI->ReadDouble(section, "MaxSize", this->MaxSize);
}

void TunnelTypeClass::LoadFromStream(AresStreamReader &Stm)
{
	Stm
		.Process(this->Passengers)
		.Process(this->MaxSize);
}

void TunnelTypeClass::SaveToStream(AresStreamWriter &Stm)
{
	Stm
		.Process(this->Passengers)
		.Process(this->MaxSize);
}

static_assert(sizeof(TunnelTypeClass) == 0x30, "TunnelTypeClass must match the 3.0p1 layout");
static_assert(offsetof(TunnelTypeClass, Name) == 0x04, "TunnelTypeClass layout slipped");
static_assert(offsetof(TunnelTypeClass, Passengers) == 0x24, "TunnelTypeClass layout slipped");
static_assert(offsetof(TunnelTypeClass, MaxSize) == 0x28, "TunnelTypeClass layout slipped");
