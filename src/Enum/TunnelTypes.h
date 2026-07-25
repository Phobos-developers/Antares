#pragma once

#include "_Enumerator.hpp"

class CCINIClass;

class TunnelTypeClass final : public Enumerable<TunnelTypeClass>
{
public:
	TunnelTypeClass(const char* pTitle);

	virtual ~TunnelTypeClass() override;

	virtual void LoadFromINI(CCINIClass *pINI) override;

	virtual void LoadFromStream(AresStreamReader &Stm) override;

	virtual void SaveToStream(AresStreamWriter &Stm) override;

	// [TunnelTypes] is a numbered list: the item is named after the value, not the key
	static void LoadFromINIList(CCINIClass *pINI);

	int Passengers;
	double MaxSize;
};
