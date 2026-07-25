#include <GenericList.h>
#include "Debug.h"
#include "IniSectionIncludes.h"

#include <Helpers\Macro.h>

INIClass::INISection* IniSectionIncludes::includedSection = nullptr;

static char* SkipBlanks(char* pText)
{
	while(*pText && *pText <= ' ') {
		++pText;
	}
	return pText;
}

INIClass::INISection* IniSectionIncludes::GetInheritSection(INIClass* pINI, char* pText)
{
	pText = SkipBlanks(pText);
	if(*pText != ':') {
		return nullptr;
	}

	pText = SkipBlanks(pText + 1);
	if(*pText != '[') {
		return nullptr;
	}

	pText = SkipBlanks(pText + 1);

	auto pEnd = strpbrk(pText, "];");
	if(!pEnd || *pEnd != ']' || pEnd == pText) {
		return nullptr;
	}

	while(pEnd[-1] <= ' ') {
		if(--pEnd == pText) {
			return nullptr;
		}
	}

	*pEnd = '\0';

	if(auto const pSection = pINI->GetSection(pText)) {
		return pSection;
	}

	Debug::Log(Debug::Severity::Warning,
		"An INI section inherits from section '%s', which doesn't exist or has not been parsed yet.\n",
		pText);

	return nullptr;
}

void IniSectionIncludes::CopySection(INIClass* pINI, INIClass::INISection* pSource, const char* pDest)
{
	//browse through section entries and copy them over to the new section
	for(auto pNode = reinterpret_cast<GenericNode*>(pSource->Entries.First()); pNode->IsValid(); pNode = pNode->Next()) {
		auto const pEntry = static_cast<INIClass::INIEntry*>(pNode);
		pINI->WriteString(pDest, pEntry->Key, pEntry->Value); //simple but effective
	}
}

DEFINE_HOOK(0x525CA5, INIClass_Parse_IniSectionIncludes_PreProcess1, 0x8)
{
	GET(char*, pEnd, EAX);

	enum { NotASection = 0x525CAD, NextSection = 0x525D4D };

	if(!pEnd) {
		return NotASection;
	}

	GET_STACK(INIClass*, pINI, 0x28);
	IniSectionIncludes::includedSection = IniSectionIncludes::GetInheritSection(pINI, pEnd + 1);

	return NextSection;
}

DEFINE_HOOK(0x525DDB, INIClass_Parse_IniSectionIncludes_PreProcess2, 0x5)
{
	GET_STACK(INIClass*, pINI, 0x28);
	GET(char*, pEnd, EAX);

	*pEnd = '\0';
	IniSectionIncludes::includedSection = IniSectionIncludes::GetInheritSection(pINI, pEnd + 1);

	return 0x525DEA;
}

DEFINE_HOOK(0x525C28, INIClass_Parse_IniSectionIncludes_CopySection1, 0x7)
{
	if(IniSectionIncludes::includedSection) {
		GET_STACK(INIClass*, pINI, 0x28);
		LEA_STACK(const char*, pName, 0x79); //yes, 0x79

		IniSectionIncludes::CopySection(pINI, IniSectionIncludes::includedSection, pName);
		IniSectionIncludes::includedSection = nullptr; //reset, very important
	}

	return 0;
}

DEFINE_HOOK(0x525E44, INIClass_Parse_IniSectionIncludes_CopySection2, 0x7)
{
	if(IniSectionIncludes::includedSection) {
		GET_STACK(INIClass*, pINI, 0x28);
		GET(INIClass::INISection*, pSection, EBX);

		IniSectionIncludes::CopySection(pINI, IniSectionIncludes::includedSection, pSection->Name);
		IniSectionIncludes::includedSection = nullptr; //reset, very important
	}

	return 0;
}
