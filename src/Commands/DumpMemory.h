#pragma once

#include <StringTable.h>

#include "Commands.h"

#include "../Misc/Debug.h"
#include "../UI/Dialogs.h"

#include <MessageListClass.h>

#include <string>

class MemoryDumperCommandClass : public CommandClass
{
public:
	//CommandClass
	virtual const char* GetName() const override
	{
		return "DumpMemory";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return StringTable::LoadString("TXT_DUMP_MEMORY");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_DEVELOPMENT");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return StringTable::LoadString("TXT_DUMP_MEMORY_DESC");
	}

	virtual void Execute(WWKey eInput) const override
	{
		Dialogs::TakeMouse();

		HCURSOR loadCursor = LoadCursor(nullptr, IDC_WAIT);
		SetClassLong(Game::hWnd, GCL_HCURSOR, reinterpret_cast<LONG>(loadCursor));
		SetCursor(loadCursor);

		MessageListClass::Instance.PrintMessage(L"Dumping process memory...");

		std::wstring filename = Debug::FullDump();

		Debug::Log("Process memory dumped to %ls\n", filename);

		filename = L"Process memory dumped to " + filename;

		MessageListClass::Instance.PrintMessage(filename.c_str());

		loadCursor = LoadCursor(nullptr, IDC_ARROW);
		SetClassLong(Game::hWnd, GCL_HCURSOR, reinterpret_cast<LONG>(loadCursor));
		SetCursor(loadCursor);

		Dialogs::ReturnMouse();
	}
};
