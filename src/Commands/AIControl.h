#pragma once

#include <StringTable.h>

#include "Commands.h"

#include <HouseClass.h>
#include <MessageListClass.h>
#include <RulesClass.h>

class AIControlCommandClass : public AresCommandClass
{
public:
	//CommandClass
	virtual const char* GetName() const override
	{
		return "AIControl";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return StringTable::LoadString("TXT_AI_CONTROL");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_DEVELOPMENT");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return StringTable::LoadString("TXT_AI_CONTROL_DESC");
	}

	virtual void Execute(WWKey eInput) const override
	{
		if(this->CheckDebugDeactivated()) {
			return;
		}

		HouseClass* pPlayer = HouseClass::CurrentPlayer;

		if(pPlayer->IsHumanPlayer && pPlayer->IsInPlayerControl) {
			//let AI assume control
			pPlayer->IsHumanPlayer = pPlayer->IsInPlayerControl = false;
			pPlayer->Production = pPlayer->AutocreateAllowed = true;

			//give full capabilities
			pPlayer->IQLevel = RulesClass::Instance->MaxIQLevels;
			pPlayer->IQLevel2 = RulesClass::Instance->MaxIQLevels;
			pPlayer->AIDifficulty = AIDifficulty::Hard;	//brutal!

			//notify
			MessageListClass::Instance.PrintMessage(L"AI assumed control!");

		} else {
			//re-assume control
			pPlayer->IsHumanPlayer = pPlayer->IsInPlayerControl = true;
			pPlayer->Production = pPlayer->AutocreateAllowed = false;

			//make it a vegetable
			pPlayer->IQLevel = 0;
			pPlayer->IQLevel2 = 0;
			pPlayer->AIDifficulty = AIDifficulty::Normal;

			//notify
			MessageListClass::Instance.PrintMessage(L"Player assumed control!");
		}
	}
};
