#pragma once

// define this to switch to release version
#define IS_RELEASE_VER

#define VERSION_MAJOR 21
#define VERSION_MINOR 352
#define VERSION_REVISION 1218

// Pinned, not derived. 0.A computed this from the version triple, but 3.0p1
// writes a fixed 0x1414D121: ExeRun stores it to Game::Savegame_Magic
// (0x10003B30), Game_Save_SavegameInformation stamps it into the save header
// (0x1006BE28), and LoadOptionsClass_GetFileInfo compares against it
// (0x1006BE76). The version triple above would give 0x151604C2, so leaving the
// old formula in place would make our saves unreadable by real 3.0p1 and vice
// versa. Whatever build froze it, it must not move with the version any more.
#define SAVEGAME_MAGIC 0x1414D121

#define wstr(x) wstr_(x)
#define wstr_(x) L ## #x
#define str(x) str_(x)
#define str_(x) #x

#define VERSION_PREFIX "Yuri's Revenge 1.001 + Ares version "
#define VERSION_STR str(VERSION_MAJOR) "." str(VERSION_MINOR) "." str(VERSION_REVISION)
#define VERSION_WSTR wstr(VERSION_MAJOR) L"." wstr(VERSION_MINOR) L"." wstr(VERSION_REVISION)

// Alternative version display name for release versions
#ifdef IS_RELEASE_VER

// the numeric triple only reaches the VERSIONINFO resource, which is not in the
// IDB, so it is the one part of this file inferred rather than read back: "3.0p1"
// mapped the way "0.A" was mapped to 0, 10, 0.
#define PRODUCT_MAJOR 3
#define PRODUCT_MINOR 0
#define PRODUCT_REVISION 1
#define PRODUCT_STR "3.0p1"
#define DISPLAY_STR PRODUCT_STR

#else

#define PRODUCT_MAJOR VERSION_MAJOR
#define PRODUCT_MINOR VERSION_MINOR
#define PRODUCT_REVISION VERSION_REVISION
#define PRODUCT_STR VERSION_STR
#define DISPLAY_STR VERSION_STR

#endif // IS_RELEASE_VER

// "Yuri's Revenge 1.001 + Ares version: $ver"
#define VERSION_STRING VERSION_PREFIX VERSION_STR
#define DISPLAY_STRING VERSION_PREFIX DISPLAY_STR

// "Ares version: $ver"
#define VERSION_STRVER "Ares version: " VERSION_STR
#define DISPLAY_STRVER "Ares version: " DISPLAY_STR

// "Ares/$ver"
#define VERSION_STREX "Ares/" VERSION_STR
#define DISPLAY_STREX "Ares/" DISPLAY_STR

// "1.001/Ares $ver"
#define VERSION_STRMINI "1.001/Ares " VERSION_STR
#define DISPLAY_STRMINI "1.001/Ares " DISPLAY_STR

#define VERSION_INTERNAL "Ares r" VERSION_STR
