#include <HouseTypeClass.h>
#include <IsometricTileTypeClass.h>
#include <TagTypeClass.h>
#include <TriggerTypeClass.h>

#include "Debug.h"
#include "../Ares.h"

DEFINE_HOOK(0x547043, IsometricTileTypeClass_ReadFromFile, 0x6)
{
	GET(int, FileSize, EBX);
	GET(IsometricTileTypeClass *, pTileType, ESI);

	if(FileSize == 0) {
		const char * tile = pTileType->ID;
		if(strlen(tile) > 9) {
			Debug::FatalErrorAndExit("Maximum allowed length for tile names, excluding the extension, is 9 characters.\n"
					"The tileset using filename '%s' exceeds this limit - the game cannot proceed.", tile);
		} else {
			Debug::FatalErrorAndExit("The tileset '%s' contains a file that could not be loaded for some reason - make sure the file exists.", tile);
		}
	}
	return 0;
}

DEFINE_HOOK(0x41088D, AbstractTypeClass_CTOR_IDTooLong, 0x6)
{
	GET(const char*, pID, EAX);

	if(strlen(pID) > 24) {
		Debug::Log("Tried to create a type with ID '%s' which is longer than the maximum length of %u.", pID, 24);
	}

	return 0;
}

DEFINE_HOOK(0x7272B5, TriggerTypeClass_LoadFromINI_House, 0x6)
{
	GET(int, idxHouse, EAX);

	if(idxHouse < 0) {
		GET(TriggerTypeClass*, pThis, EBP);
		GET(const char*, pName, ESI);

		Debug::Log("TriggerType '%s' refers to a house named '%s', which does not exist. "
			"In case no house is needed, use '<none>' explicitly.", pThis->ID, pName);

		R->EDX(0);
	} else {
		R->EDX(HouseTypeClass::Array.Items[idxHouse]);
	}

	return 0x7272C1;
}
