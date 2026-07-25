#pragma once

// Ares's own extensions to the game's enums -- values past the end of a game
// range. They are constexpr constants of the game's enum type rather than
// separate enums, so they stay usable as switch labels and call arguments.
//
// THE VALUES ARE LOAD-BEARING. They reach map files and savegames. Never
// renumber them; only ever append.

#include <GeneralDefinitions.h>

namespace AresTriggerEvent {
	// --- Ares extensions -----------------------------------------------------
	// gamemd's own TEventType enum ends at TEVENT_RA2_TECHTYPE_DOESNT_EXIST = 0x3D
	// with TEVENT_COUNT = 0x3E, so 62.. is free for Ares. TeventExt::HasOccured
	// (Ares.dll 0x10050400) dispatches `EventKind - 62` through a 27-entry byte
	// table at 0x10050A20, which fixes the range as exactly 62..88.
	inline constexpr auto UnderEMP = static_cast<TriggerEvent>(0x3E);  // 62
	inline constexpr auto UnderEMP_ByHouse = static_cast<TriggerEvent>(0x3F);  // 63
	inline constexpr auto RemoveEMP = static_cast<TriggerEvent>(0x40);  // 64
	inline constexpr auto RemoveEMP_ByHouse = static_cast<TriggerEvent>(0x41);  // 65
	inline constexpr auto EnemyInSpotlightNow = static_cast<TriggerEvent>(0x42);  // 66
	inline constexpr auto DriverKiller = static_cast<TriggerEvent>(0x43);  // 67
	inline constexpr auto DriverKilled_ByHouse = static_cast<TriggerEvent>(0x44);  // 68
	inline constexpr auto VehicleTaken = static_cast<TriggerEvent>(0x45);  // 69
	inline constexpr auto VehicleTaken_ByHouse = static_cast<TriggerEvent>(0x46);  // 70
	inline constexpr auto Abducted = static_cast<TriggerEvent>(0x47);  // 71
	inline constexpr auto Abducted_ByHouse = static_cast<TriggerEvent>(0x48);  // 72
	inline constexpr auto AbductSomething = static_cast<TriggerEvent>(0x49);  // 73
	inline constexpr auto AbductSomething_OfHouse = static_cast<TriggerEvent>(0x4A);  // 74
	inline constexpr auto SuperActivated = static_cast<TriggerEvent>(0x4B);  // 75
	inline constexpr auto SuperDeactivated = static_cast<TriggerEvent>(0x4C);  // 76
	inline constexpr auto SuperNearWaypoint = static_cast<TriggerEvent>(0x4D);  // 77
	inline constexpr auto ReverseEngineered = static_cast<TriggerEvent>(0x4E);  // 78
	inline constexpr auto ReverseEngineerAnything = static_cast<TriggerEvent>(0x4F);  // 79
	inline constexpr auto ReverseEngineerType = static_cast<TriggerEvent>(0x50);  // 80
	inline constexpr auto HouseOwnTechnoType = static_cast<TriggerEvent>(0x51);  // 81
	inline constexpr auto HouseDoesntOwnTechnoType = static_cast<TriggerEvent>(0x52);  // 82
	inline constexpr auto AttackedOrDestroyedByAnybody = static_cast<TriggerEvent>(0x53);  // 83
	inline constexpr auto AttackedOrDestroyedByHouse = static_cast<TriggerEvent>(0x54);  // 84
	inline constexpr auto DestroyedByHouse = static_cast<TriggerEvent>(0x55);  // 85
	inline constexpr auto TechnoTypeDoesntExistMoreThan = static_cast<TriggerEvent>(0x56);  // 86
	inline constexpr auto AllKeepAlivesDestroyed = static_cast<TriggerEvent>(0x57);  // 87
	inline constexpr auto AllKeepAlivesBuildingDestroyed = static_cast<TriggerEvent>(0x58);  // 88 (IDB spells this "Kepp"; normalised)
}

namespace AresTriggerAction {
	// --- Ares extensions -----------------------------------------------------
	// gamemd's own TActionType enum ends at TACTION_YR_JUMP_CAMERA_HOME = 0x91
	// with TACTION_COUNT = 0x92, so 146.. is free for Ares.
	inline constexpr auto AuxiliaryPower = static_cast<TriggerAction>(0x92);  // 146
	inline constexpr auto KillDriversOf = static_cast<TriggerAction>(0x93);  // 147
	inline constexpr auto SetEVAVoice = static_cast<TriggerAction>(0x94);  // 148
	inline constexpr auto SetGroup = static_cast<TriggerAction>(0x95);  // 149
}

namespace AresSuperWeaponType {
	// --- Ares extensions -----------------------------------------------------
	// gamemd's own SpecialWeaponType ends at SPC_Psychic_Reveal = 0xB with
	// SPC_COUNT = 0xC, matching SWTypeExt::FirstCustomType == 12. The 18 slots
	// below are the NewSWType registration order (Ares.dll NewSWType_Reg
	// 0x1006D630, 18-slot array at 0x100C31C8), ending at SW_Battery = 29.
	// This registration order is savegame-semantic: append only.
	inline constexpr auto FirstCustomType = static_cast<SuperWeaponType>(12);
	inline constexpr auto SonarPulse = static_cast<SuperWeaponType>(12);
	inline constexpr auto UnitDelivery = static_cast<SuperWeaponType>(13);
	inline constexpr auto GenericWarhead = static_cast<SuperWeaponType>(14);
	inline constexpr auto Firewall = static_cast<SuperWeaponType>(15);
	inline constexpr auto Protect = static_cast<SuperWeaponType>(16);
	inline constexpr auto Reveal = static_cast<SuperWeaponType>(17);
	inline constexpr auto ParaDropAres = static_cast<SuperWeaponType>(18);
	inline constexpr auto SpyPlaneAres = static_cast<SuperWeaponType>(19);
	inline constexpr auto ChronoSphereAres = static_cast<SuperWeaponType>(20);
	inline constexpr auto ChronoWarpAres = static_cast<SuperWeaponType>(21);
	inline constexpr auto GeneticMutatorAres = static_cast<SuperWeaponType>(22);
	inline constexpr auto PsychicDominatorAres = static_cast<SuperWeaponType>(23);
	inline constexpr auto LightningStormAres = static_cast<SuperWeaponType>(24);
	inline constexpr auto NuclearMissileAres = static_cast<SuperWeaponType>(25);
	inline constexpr auto HunterSeeker = static_cast<SuperWeaponType>(26);
	inline constexpr auto DropPod = static_cast<SuperWeaponType>(27);
	inline constexpr auto EMPulse = static_cast<SuperWeaponType>(28);
	inline constexpr auto Battery = static_cast<SuperWeaponType>(29);
}
