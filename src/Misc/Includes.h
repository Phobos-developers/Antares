#ifndef INCLUSIONS_H
#define INCLUSIONS_H

#include <ArrayClasses.h>
#include <CCFileClass.h>
#include <CCINIClass.h>

class Includes
{
public:
	static int LastReadIndex;
	static DynamicVectorClass<CCINIClass*> LoadedINIs;
	static DynamicVectorClass<char*> LoadedINIFiles;
};

#endif
