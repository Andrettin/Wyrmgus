/// Name
#define NAME "Stratagus"

/// Description
#define DESCRIPTION NAME " - Strategy Gaming Engine"

#ifndef StratagusMajorVersion
	#define StratagusMajorVersion 0
#endif

#ifndef StratagusMinorVersion
	#define StratagusMinorVersion 0
#endif

#ifndef StratagusPatchLevel
	#define StratagusPatchLevel 0
#endif

#define _version_stringify_(s) #s
#define _version_stringify(s) _version_stringify_(s)

#define _version_str1 _version_stringify(StratagusMajorVersion) "." _version_stringify(StratagusMinorVersion) "." _version_stringify(StratagusPatchLevel)

/// Engine version string
#define VERSION _version_str1

/// Stratagus version (1,2,3) -> 10203
#define StratagusVersion (StratagusMajorVersion * 10000 + StratagusMinorVersion * 100 + StratagusPatchLevel)

/// Homepage
#define HOMEPAGE "https://github.com/Andrettin/Wyrmgus"

/// License
#define LICENSE "GPL 2.0"

/// Copyright
#define COPYRIGHT "Copyright (c) 1998-2021 by The Stratagus Project"
