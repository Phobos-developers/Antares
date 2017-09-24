#ifndef GEN_PREREQ_H
#define GEN_PREREQ_H

#include <CCINIClass.h>
#include <HouseClass.h>
#include <UnitTypeClass.h>

#include "_Enumerator.hpp"
#include "../Ares.CRT.h"
#include "../Utilities/Iterator.h"

#ifdef DEBUGBUILD
#include "../Misc/Debug.h"
#endif

class HouseClass;

class GenericPrerequisite;

class GenericPrerequisite : public Enumerable<GenericPrerequisite>
{
public:
	GenericPrerequisite(const char *Title) : Enumerable<GenericPrerequisite>(Title) { }

	virtual ~GenericPrerequisite() override = default;

	virtual void LoadFromINI(CCINIClass *pINI) override;

	static void AddDefaults();

	DynamicVectorClass<int> Prereqs;
};

class Prereqs
{
public:
	typedef Iterator<BuildingTypeClass*> BTypeIter;

	static void Parse(CCINIClass *pINI, const char* section, const char *key, DynamicVectorClass<int> *vec);

	static bool HouseOwnsGeneric(HouseClass *pHouse, signed int Index);
	static bool HouseOwnsSpecific(HouseClass *pHouse, signed int Index);
	static bool HouseOwnsPrereq(HouseClass *pHouse, signed int Index);

	static bool HouseOwnsAll(HouseClass *pHouse, DynamicVectorClass<int> *list);
	static bool HouseOwnsAny(HouseClass *pHouse, DynamicVectorClass<int> *list);

	static bool ListContainsGeneric(const BTypeIter &List, signed int Index);
	static bool ListContainsSpecific(const BTypeIter &List, signed int Index);
	static bool ListContainsPrereq(const BTypeIter &List, signed int Index);

	static bool ListContainsAll(const BTypeIter &List, DynamicVectorClass<int> *Requirements);
	static bool ListContainsAny(const BTypeIter &List, DynamicVectorClass<int> *Requirements);
};

#endif
