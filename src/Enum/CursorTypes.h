#pragma once

#include "_Enumerator.hpp"

#include <MouseClass.h>

class CCINIClass;

class CursorType final : public Enumerable<CursorType>
{
public:
	struct ActionCursor
	{
		int Index;
		int Mode;
	};

	// entry 0 overrides every action, entry action+1 overrides a single one
	static const size_t ActionCursorCount = 74;

	CursorType(const char* pTitle);

	virtual ~CursorType() override;

	virtual void LoadFromINI(CCINIClass *pINI) override;

	virtual void LoadFromStream(AresStreamReader &Stm) override;

	virtual void SaveToStream(AresStreamWriter &Stm) override;

	static void Clear();

	static void LoadDefault();

	static bool LoadGlobals(AresStreamReader &Stm);

	static bool SaveGlobals(AresStreamWriter &Stm);

	static MouseCursor* GetCursor(MouseCursorType index);

	static void Select(MouseCursorType index);

	static void ClearActions();

	static void SetAction(MouseCursorType index, Action action, int mode);

	static void AddMappedAction(MouseCursorType index, bool fireIntoShroud, Action action);

	static const ActionCursor* FindAction(Action action);

	static int SelectedIndex;

	static MouseCursor* SelectedCursor;

	static ActionCursor ActionCursors[ActionCursorCount];

	MouseCursor Data;
};
