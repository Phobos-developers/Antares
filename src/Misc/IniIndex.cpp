#include <CCINIClass.h>
#include "../Utilities/Checksummer.h"
#include <GenericList.h>

#include <Helpers\Macro.h>

#include <algorithm>

// the game indexes both its sections and a section's entries by the checksum of
// their name. these are the searches the game performs for every single flag it
// reads, so they are reimplemented here.

namespace
{
	struct IndexNode
	{
		int Key;
		void* Value;
	};

	struct IndexList
	{
		IndexNode* Nodes;
		int Count;
		int Capacity;
		bool IsSorted;
		IndexNode* Archive;
	};

	IndexList* GetSectionIndex(INIClass* pINI)
	{
		return reinterpret_cast<IndexList*>(&pINI->SectionIndex);
	}

	IndexList* GetEntryIndex(INIClass::INISection* pSection)
	{
		return reinterpret_cast<IndexList*>(&pSection->EntryIndex);
	}

	int HashName(const char* pName)
	{
		Checksummer crc;
		crc.Add(pName);
		return static_cast<int>(crc.GetValue());
	}

	IndexNode* FetchNode(IndexList* pList, int key)
	{
		if(auto const pArchive = pList->Archive) {
			if(pArchive->Key == key) {
				return pArchive;
			}
		}

		if(!pList->Count) {
			return nullptr;
		}

		if(!pList->IsSorted) {
			std::sort(pList->Nodes, pList->Nodes + pList->Count,
				[](const IndexNode& lhs, const IndexNode& rhs) { return lhs.Key < rhs.Key; });
			pList->Archive = nullptr;
			pList->IsSorted = true;
		}

		auto const pEnd = pList->Nodes + pList->Count;
		auto const pFound = std::lower_bound(pList->Nodes, pEnd, key,
			[](const IndexNode& node, int value) { return node.Key < value; });

		if(pFound == pEnd || key < pFound->Key) {
			return nullptr;
		}

		pList->Archive = pFound;
		return pFound;
	}

	void RemoveNode(IndexList* pList, int key)
	{
		if(auto const pFound = FetchNode(pList, key)) {
			auto const pEnd = pList->Nodes + pList->Count;
			memcpy(pFound, pFound + 1,
				reinterpret_cast<char*>(pEnd) - reinterpret_cast<char*>(pFound + 1));

			pList->Nodes[pList->Count - 1].Key = 0;
			pList->Nodes[pList->Count - 1].Value = nullptr;
			--pList->Count;
			pList->Archive = nullptr;
		}
	}

	INIClass::INISection* FindSection(INIClass* pINI, const char* pSection)
	{
		if(pINI->CurrentSectionName == pSection) {
			return pINI->CurrentSection;
		}

		auto const pNode = FetchNode(GetSectionIndex(pINI), HashName(pSection));
		if(!pNode || !pNode->Value) {
			return nullptr;
		}

		pINI->CurrentSection = static_cast<INIClass::INISection*>(pNode->Value);
		pINI->CurrentSectionName = const_cast<char*>(pSection);

		return static_cast<INIClass::INISection*>(pNode->Value);
	}

	const char* FindValue(
		INIClass* pINI, const char* pSection, const char* pKey, const char* pDefault)
	{
		auto const pFound = FindSection(pINI, pSection);
		if(!pFound) {
			return pDefault;
		}

		auto const pNode = FetchNode(GetEntryIndex(pFound), HashName(pKey));
		if(!pNode) {
			return pDefault;
		}

		auto const pEntry = static_cast<INIClass::INIEntry*>(pNode->Value);
		return pEntry ? pEntry->Value : pDefault;
	}

	INIClass::INISection* lastSection = nullptr;
	GenericNode* lastEntry = nullptr;
	int lastIndex = 0;
}

DEFINE_HOOK(0x528A10, INIClass_GetString, 0x5)
{
	GET(INIClass*, pThis, ECX);
	GET_STACK(const char*, pSection, 0x4);
	GET_STACK(const char*, pKey, 0x8);
	GET_STACK(const char*, pDefault, 0xC);
	GET_STACK(char*, pBuffer, 0x10);
	GET_STACK(size_t, length, 0x14);

	size_t count = 0;

	if(pBuffer) {
		if(length >= 2 && pSection && pKey) {
			if(auto pValue = FindValue(pThis, pSection, pKey, pDefault)) {
				while(*pValue && *pValue <= ' ') {
					++pValue;
				}

				count = strlen(pValue);
				while(count && pValue[count - 1] <= ' ') {
					--count;
				}

				if(count > length - 1) {
					count = length - 1;
				}

				memcpy(pBuffer, pValue, count);
			}

			pBuffer[count] = '\0';
		}
	}

	R->EAX(count);
	return 0x528BFA;
}

DEFINE_HOOK(0x526CC0, INIClass_Section_GetKeyName, 0x7)
{
	GET(INIClass*, pThis, ECX);
	GET_STACK(const char*, pSection, 0x4);
	GET_STACK(int, index, 0x8);

	const char* pKey = nullptr;

	if(auto const pFound = FindSection(pThis, pSection)) {
		if(index < GetEntryIndex(pFound)->Count) {
			GenericNode* pNode = nullptr;

			if(pFound == lastSection && lastIndex + 1 == index
				&& lastEntry->IsValid())
			{
				pNode = lastEntry->Next();
			} else {
				pNode = reinterpret_cast<GenericNode*>(pFound->Entries.First());
				for(int i = index; i > 0; --i) {
					pNode = pNode->Next();
				}
				lastSection = pFound;
			}

			lastIndex = index;
			lastEntry = pNode;
			pKey = static_cast<INIClass::INIEntry*>(pNode)->Key;
		}
	}

	R->EAX(pKey);
	return 0x526D8A;
}

// a key that is defined twice within the same section replaces the previous
// definition instead of being added next to it
DEFINE_HOOK(0x5260D9, INIClass_Parse_Override, 0x7)
{
	GET_STACK(INIClass::INISection*, pSection, 0x38);
	GET(int, key, EAX);

	auto const pList = GetEntryIndex(pSection);

	if(auto const pNode = FetchNode(pList, key)) {
		if(auto const pEntry = static_cast<INIClass::INIEntry*>(pNode->Value)) {
			RemoveNode(pList, key);
			GameDelete(pEntry);
		}
	}

	return 0;
}
