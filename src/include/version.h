/// Name
#define NAME "Stratagus"

/// Description
#define DESCRIPTION NAME " - Strategy Gaming Engine"

constexpr int StratagusMajorVersion = 4;
constexpr int StratagusMinorVersion = 0;
constexpr int StratagusPatchLevel = 0;
constexpr int StratagusPatchLevel2 = 0;

#define _version_stringify_(s) #s
#define _version_stringify(s) _version_stringify_(s)

#define _version_str1 _version_stringify(StratagusMajorVersion) "." _version_stringify(StratagusMinorVersion) "." _version_stringify(StratagusPatchLevel)

#if StratagusPatchLevel2 > 0
#define _version_str2 _version_str1 "." _version_stringify(StratagusPatchLevel2)
#else
#define _version_str2 _version_str1
#endif

/// Engine version string
#define VERSION _version_str2

/// Stratagus version (1,2,3) -> 10203
constexpr int StratagusVersion = (StratagusMajorVersion * 10000 + StratagusMinorVersion * 100 + StratagusPatchLevel);

/// Homepage
#define HOMEPAGE "https://github.com/Andrettin/Wyrmgus"

/// License
constexpr const char *LICENSE = "GPL 2.0";

/// Copyright
#define COPYRIGHT "Copyright (c) 1998-2019 by The Stratagus Project"
