#include "IniIteratorChar.h"

#include <Helpers\Macro.h>

const char* const IniIteratorChar::iteratorChar = "+";
const char* const IniIteratorChar::iteratorReplacementFormat = "var_%d";

int IniIteratorChar::iteratorValue = 0;

DEFINE_HOOK(0x5260A2, INIClass_Parse_IteratorChar1, 0x6)
{
	GET(CCINIClass::INIEntry*, entry, ESI);

	if(strcmp(entry->Key, IniIteratorChar::iteratorChar) == 0) {
		char buffer[0x10];
		sprintf_s(buffer, IniIteratorChar::iteratorReplacementFormat,
			IniIteratorChar::iteratorValue++);

		auto const oldKey = entry->Key;
		entry->Key = CRT::strdup(buffer);
		CRT::free(oldKey);
	}

	return 0;
}

DEFINE_HOOK(0x525D23, INIClass_Parse_IteratorChar2, 0x5)
{
	GET(char*, value, ESI);
	LEA_STACK(char*, key, 0x78)

	if(strcmp(key, IniIteratorChar::iteratorChar) == 0) {
		char buffer[0x200];
		strcpy_s(buffer, value);

		int len = sprintf_s(key, 512,
			IniIteratorChar::iteratorReplacementFormat,
			IniIteratorChar::iteratorValue++);

		if(len >= 0) {
			char* newValue = &key[len + 1];
			strcpy_s(newValue, 512 - len - 1, buffer);
			R->ESI<char*>(newValue);
		}
	}

	return 0;
}
