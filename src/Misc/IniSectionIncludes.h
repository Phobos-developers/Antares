#pragma once

#include <CCINIClass.h>

//temporary information holder
class IniSectionIncludes
{
public:
	static INIClass::INISection* includedSection;

	static INIClass::INISection* GetInheritSection(INIClass* pINI, char* pText);
	static void CopySection(INIClass* pINI, INIClass::INISection* pSource, const char* pDest);
};
