#pragma once

#include <ScriptTypeClass.h>

class TeamClass;

class ScriptExt
{
public:
	// the script actions Ares takes over
	enum class ScriptAction : int {
		GarrisonStructure = 64,
		AuxiliaryPower = 65,
		KillDrivers = 66,
		TakeVehicles = 67,
		ConvertType = 68,
		SonarReveal = 69,
		DisableWeapons = 70
	};

	static bool Handled(TeamClass* pTeam, ScriptActionNode* pAction, bool flag);

private:
	static void TakeVehicles(TeamClass* pTeam, ScriptActionNode* pAction, bool flag);
};
