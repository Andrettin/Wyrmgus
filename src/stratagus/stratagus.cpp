//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name stratagus.cpp - The main file. */
//
//      (c) Copyright 1998-2015 by Lutz Sammer, Francois Beerten,
//                                 Jimmy Salmon, Pali Rohár and cybermind
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

//@{

/**
** @mainpage
**
** @section Introduction Introduction
**
** Welcome to the source code documentation of the Stratagus engine.
** Extract the source documentation with doxygen (http://www.doxygen.org) tool.
**
** Any help to improve this documention is welcome. If you didn't
** understand something or you found an error or a wrong spelling
** or wrong grammar please write an email (including a patch :).
**
** @section Informations Informations
**
** Visit the https://launchpad.net/stratagus web page for the latest news and
** <A HREF="../index.html">Stratagus Info</A> for other documentations.
**
** @section Modules Modules
**
** This are the main modules of the Stratagus engine.
**
** @subsection Map Map
**
** Handles the map. A map is made from tiles.
**
** @see map.h @see map.cpp @see tileset.h @see tileset.cpp
**
** @subsection Unit Unit
**
** Handles units. Units are ships, flyers, buildings, creatures,
** machines.
**
** @see unit.h @see unit.cpp @see unittype.h @see unittype.cpp
**
** @subsection Missile Missile
**
** Handles missiles. Missiles are all other sprites on map
** which are no unit.
**
** @see missile.h @see missile.cpp
**
** @subsection Player Player
**
** Handles players, all units are owned by a player. A player
** could be controlled by a human or a computer.
**
** @see player.h @see player.cpp @see ::CPlayer
**
** @subsection Sound Sound
**
** Handles the high and low level of the sound. There are the
** background music support, voices and sound effects.
** Following low level backends are supported: OSS and SDL.
**
** @todo adpcm file format support for sound effects
** @todo better separation of low and high level, assembler mixing
** support.
** @todo Streaming support of ogg/mp3 files.
**
** @see sound.h @see sound.cpp
** @see script_sound.cpp @see sound_id.cpp @see sound_server.cpp
** @see unitsound.cpp
** @see sdl_audio.cpp
** @see ogg.cpp @see wav.cpp
**
** @subsection Video Video
**
** Handles the high and low level of the graphics.
** This also contains the sprite and linedrawing routines.
**
** See page @ref VideoModule for more information upon supported
** features and video platforms.
**
** @see video.h @see video.cpp
**
** @subsection Network Network
**
** Handles the high and low level of the network protocol.
** The network protocol is needed for multiplayer games.
**
** See page @ref NetworkModule for more information upon supported
** features and API.
**
** @see network.h @see network.cpp
**
** @subsection Pathfinder Pathfinder
**
** @see pathfinder.h @see pathfinder.cpp
**
** @subsection AI AI
**
** There are currently two AI's. The old one is very hardcoded,
** but does things like placing buildings better than the new.
** The old AI shouldn't be used.  The new is very flexible, but
** very basic. It includes none optimations.
**
** See page @ref AiModule for more information upon supported
** features and API.
**
** @see ai_local.h
** @see ai.h @see ai.cpp
**
** @subsection CCL CCL
**
** CCL is Craft Configuration Language, which is used to
** configure and customize Stratagus.
**
** @see script.h @see script.cpp
**
** @subsection Icon Icon
**
** @see icons.h @see icons.cpp
**
** @subsection Editor Editor
**
** This is the integrated editor, it shouldn't be a perfect
** editor. It is used to test new features of the engine.
**
** See page @ref EditorModule for more information upon supported
** features and API.
**
** @see editor.h @see editor.cpp
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <ctype.h>

#ifdef USE_BEOS
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

extern void beos_init(int argc, char **argv);

#endif

#ifdef MAC_BUNDLE
#define Button ButtonOSX
#include <Carbon/Carbon.h>
#undef Button
#endif

#include "SDL.h"

#include "stratagus.h"

#include "ai.h"
#include "editor.h"
//Wyrmgus start
#include "font.h"	// for grand strategy mode tooltip drawing
//Wyrmgus end
#include "game.h"
#include "guichan.h"
#include "interface.h"
#include "iocompat.h"
#include "iolib.h"
#include "map.h"
#include "netconnect.h"
#include "network.h"
#include "parameters.h"
#include "player.h"
#include "replay.h"
#include "results.h"
#include "settings.h"
#include "sound_server.h"
#include "title.h"
#include "translate.h"
#include "ui.h"
#include "unit_manager.h"
//Wyrmgus start
#include "upgrade.h"	// for grand strategy elements
//Wyrmgus end
#include "version.h"
#include "video.h"
#include "widgets.h"
#include "util.h"

#ifdef DEBUG
#include "missile.h" //for FreeBurningBuildingFrames
#endif

#ifdef USE_STACKTRACE
#include <stdexcept>
#include <stacktrace/call_stack.hpp>
#include <stacktrace/stack_exception.hpp>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef USE_WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

#if defined(USE_WIN32) && ! defined(NO_STDIO_REDIRECT)
#include "windows.h"
#define REDIRECT_OUTPUT
#endif

#if defined(USE_WIN32) && ! defined(REDIRECT_OUTPUT)
#include "SetupConsole_win32.h"
#endif

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::string StratagusLibPath;        /// Path for data directory

/// Name, Version, Copyright
//Wyrmgus start
//const char NameLine[] = NAME " V" VERSION ", " COPYRIGHT;
const char NameLine[] = NAME " v" VERSION ", " COPYRIGHT;
//Wyrmgus end

std::string CliMapName;          /// Filename of the map given on the command line
std::string MenuRace;

bool EnableDebugPrint;           /// if enabled, print the debug messages
bool EnableAssert;               /// if enabled, halt on assertion failures
bool EnableUnitDebug;            /// if enabled, a unit info dump will be created

/*============================================================================
==  MAIN
============================================================================*/

/**
**  Pre menu setup.
*/
void PreMenuSetup()
{
	//
	//  Initial menus require some gfx.
	//
	SetDefaultTextColors(FontYellow, FontWhite);

	LoadFonts();

	InitVideoCursors();

	if (MenuRace.empty()) {
		LoadCursors(PlayerRaces.Name[0]);
	} else {
		LoadCursors(MenuRace);
	}

	InitSettings();

	InitUserInterface();
	UI.Load();
}

/**
**  Run the guichan main menus loop.
**
**  @return          0 for success, else exit.
*/
static int MenuLoop()
{
	int status;

	initGuichan();
	InterfaceState = IfaceStateMenu;
	//  Clear screen
	Video.ClearScreen();
	Invalidate();

	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	CursorState = CursorStatePoint;
	GameCursor = UI.Point.Cursor;

	// FIXME delete this when switching to full guichan GUI
	const std::string filename = LibraryFileName("scripts/guichan.lua");
	status = LuaLoadFile(filename);

	// We clean up later in Exit
	return status;
}

//----------------------------------------------------------------------------

/**
**  Print headerline, copyright, ...
*/
static void PrintHeader()
{
	std::string CompileOptions =
#ifdef DEBUG
		"DEBUG "
#endif
#ifdef USE_ZLIB
		"ZLIB "
#endif
#ifdef USE_BZ2LIB
		"BZ2LIB "
#endif
#ifdef USE_VORBIS
		"VORBIS "
#endif
#ifdef USE_THEORA
		"THEORA "
#endif
#ifdef USE_FLUIDSYNTH
	"FLUIDSYNTH "
#endif
#ifdef USE_MIKMOD
		"MIKMOD "
#endif
#ifdef USE_MNG
		"MNG "
#endif
#ifdef USE_OPENGL
		"OPENGL "
#endif
#ifdef USE_GLES
		"GLES "
#endif
#ifdef USE_WIN32
		"WIN32 "
#endif
#ifdef USE_LINUX
		"LINUX "
#endif
#ifdef USE_BSD
		"BSD "
#endif
#ifdef USE_BEOS
		"BEOS "
#endif
#ifdef USE_MAC
		"MAC "
#endif
#ifdef USE_X11
		"X11 "
#endif
#ifdef USE_MAEMO
		"MAEMO "
#endif
#ifdef USE_TOUCHSCREEN
		"TOUCHSCREEN "
#endif
		"";

	fprintf(stdout,
			"%s\n  written by Lutz Sammer, Fabrice Rossi, Vladi Shabanski, Patrice Fortier,\n"
			"  Jon Gabrielson, Andreas Arens, Nehal Mistry, Jimmy Salmon, Pali Rohar,\n"
			"  cybermind and others.\n"
			"\t" HOMEPAGE "\n"
			"Compile options %s",
			NameLine, CompileOptions.c_str());
}

void PrintLicense()
{
	printf("\n"
		   "\n"
		   "Stratagus may be copied only under the terms of the GNU General Public License\n"
		   "which may be found in the Stratagus source kit.\n"
		   "\n"
		   "DISCLAIMER:\n"
		   "This software is provided as-is.  The author(s) can not be held liable for any\n"
		   "damage that might arise from the use of this software.\n"
		   "Use it at your own risk.\n"
		   "\n");
}


/**
**  Exit the game.
**
**  @param err  Error code to pass to shell.
*/
void Exit(int err)
{
	if (GameRunning) {
		StopGame(GameExit);
		return;
	}

	StopMusic();
	QuitSound();
	NetworkQuitGame();

	ExitNetwork1();
#ifdef DEBUG
	CleanModules();
	FreeBurningBuildingFrames();
	FreeSounds();
	FreeGraphics();
	FreePlayerColors();
	FreeButtonStyles();
	FreeAllContainers();
	freeGuichan();
	DebugPrint("Frames %lu, Slow frames %d = %ld%%\n" _C_
			   FrameCounter _C_ SlowFrameCounter _C_
			   (SlowFrameCounter * 100) / (FrameCounter ? FrameCounter : 1));
	lua_settop(Lua, 0);
	lua_close(Lua);
	DeInitVideo();
#endif

	fprintf(stdout, "%s", _("Thanks for playing Stratagus.\n"));
	exit(err);
}

/**
**  Do a fatal exit.
**  Called on out of memory or crash.
**
**  @param err  Error code to pass to shell.
*/
void ExitFatal(int err)
{
#ifdef USE_STACKTRACE
	throw stacktrace::stack_runtime_error((const char*)err);
#endif
	exit(err);
}

/**
**  Display the usage.
*/
static void Usage()
{
	PrintHeader();
	printf(
		"\n\nUsage: %s [OPTIONS] [map.smp|map.smp.gz]\n"
		"\t-a\t\tEnables asserts check in engine code (for debugging)\n"
		"\t-c file.lua\tConfiguration start file (default stratagus.lua)\n"
		"\t-d datapath\tPath to stratagus data (default current directory)\n"
		"\t-D depth\tVideo mode depth = pixel per point\n"
		"\t-e\t\tStart editor (instead of game)\n"
		"\t-E file.lua\tEditor configuration start file (default editor.lua)\n"
		"\t-F\t\tFull screen video mode\n"
		"\t-h\t\tHelp shows this page\n"
		"\t-i\t\tEnables unit info dumping into log (for debugging)\n"
		"\t-I addr\t\tNetwork address to use\n"
		"\t-l\t\tDisable command log\n"
		"\t-N name\t\tName of the player\n"
#if defined(USE_OPENGL) || defined(USE_GLES)
		"\t-o\t\tDo not use OpenGL or OpenGL ES 1.1\n"
		"\t-O\t\tUse OpenGL or OpenGL ES 1.1\n"
#endif
		"\t-p\t\tEnables debug messages printing in console\n"
		"\t-P port\t\tNetwork port to use\n"
		"\t-s sleep\tNumber of frames for the AI to sleep before it starts\n"
		"\t-S speed\tSync speed (100 = 30 frames/s)\n"
		"\t-u userpath\tPath where stratagus saves preferences, log and savegame\n"
		"\t-v mode\t\tVideo mode resolution in format <xres>x<yres>\n"
		"\t-W\t\tWindowed video mode\n"
#if defined(USE_OPENGL) || defined(USE_GLES)
		"\t-Z\t\tUse OpenGL to scale the screen to the viewport (retro-style). Implies -O.\n"
#endif
		"map is relative to StratagusLibPath=datapath, use ./map for relative to cwd\n",
		Parameters::Instance.applicationName.c_str());
}

#ifdef REDIRECT_OUTPUT

static std::string stdoutFile;
static std::string stderrFile;

static void CleanupOutput()
{
	fclose(stdout);
	fclose(stderr);

	struct stat st;
	if (stat(stdoutFile.c_str(), &st) == 0 && st.st_size == 0) {
		unlink(stdoutFile.c_str());
	}
	if (stat(stderrFile.c_str(), &st) == 0 && st.st_size == 0) {
		unlink(stderrFile.c_str());
	}
}

static void RedirectOutput()
{
	char path[MAX_PATH];
	int pathlen;

	pathlen = GetModuleFileName(NULL, path, sizeof(path));
	while (pathlen > 0 && path[pathlen] != '\\') {
		--pathlen;
	}
	path[pathlen] = '\0';

	stdoutFile = std::string(path) + "\\stdout.txt";
	stderrFile = std::string(path) + "\\stderr.txt";

	if (!freopen(stdoutFile.c_str(), "w", stdout)) {
		printf("freopen stdout failed");
	}
	if (!freopen(stderrFile.c_str(), "w", stderr)) {
		printf("freopen stderr failed");
	}
	atexit(CleanupOutput);
}
#endif

void ParseCommandLine(int argc, char **argv, Parameters &parameters)
{
	for (;;) {
		switch (getopt(argc, argv, "ac:d:D:eE:FhiI:lN:oOP:ps:S:u:v:WZ?")) {
			case 'a':
				EnableAssert = true;
				continue;
			case 'c':
				parameters.luaStartFilename = optarg;
				continue;
			case 'd': {
				StratagusLibPath = optarg;
				size_t index;
				while ((index = StratagusLibPath.find('\\')) != std::string::npos) {
					StratagusLibPath[index] = '/';
				}
				continue;
			}
			case 'D':
				Video.Depth = atoi(optarg);
				continue;
			case 'e':
				Editor.Running = EditorCommandLine;
				continue;
			case 'E':
				parameters.luaEditorStartFilename = optarg;
				continue;
			case 'F':
				VideoForceFullScreen = 1;
				Video.FullScreen = 1;
				continue;
			case 'i':
				EnableUnitDebug = true;
				continue;
			case 'I':
				CNetworkParameter::Instance.localHost = optarg;
				continue;
			case 'l':
				CommandLogDisabled = true;
				continue;
			case 'N':
				parameters.LocalPlayerName = optarg;
				continue;
#if defined(USE_OPENGL) || defined(USE_GLES)
			case 'o':
				ForceUseOpenGL = 1;
				UseOpenGL = 0;
				if (ZoomNoResize) {
					fprintf(stderr, "Error: -Z only works with OpenGL enabled\n");
					Usage();
					ExitFatal(-1);
				}
				continue;
			case 'O':
				ForceUseOpenGL = 1;
				UseOpenGL = 1;
				continue;
#endif
			case 'P':
				CNetworkParameter::Instance.localPort = atoi(optarg);
				continue;
			case 'p':
				EnableDebugPrint = true;
				continue;
			case 's':
				AiSleepCycles = atoi(optarg);
				continue;
			case 'S':
				VideoSyncSpeed = atoi(optarg);
				continue;
			case 'u':
				Parameters::Instance.SetUserDirectory(optarg);
				continue;
			case 'v': {
				char *sep = strchr(optarg, 'x');
				if (!sep || !*(sep + 1)) {
					fprintf(stderr, "%s: incorrect format of video mode resolution -- '%s'\n", argv[0], optarg);
					Usage();
					ExitFatal(-1);
				}
				Video.Height = atoi(sep + 1);
				*sep = 0;
				Video.Width = atoi(optarg);
				if (!Video.Height || !Video.Width) {
					fprintf(stderr, "%s: incorrect format of video mode resolution -- '%sx%s'\n", argv[0], optarg, sep + 1);
					Usage();
					ExitFatal(-1);
				}
#if defined(USE_OPENGL) || defined(USE_GLES)
				if (ZoomNoResize) {
					Video.ViewportHeight = Video.Height;
					Video.ViewportWidth = Video.Width;
					Video.Height = 0;
					Video.Width = 0;
				}
#endif
				continue;
			}
			case 'W':
				VideoForceFullScreen = 1;
				Video.FullScreen = 0;
				continue;
#if defined(USE_OPENGL) || defined(USE_GLES)
			case 'Z':
				ForceUseOpenGL = 1;
				UseOpenGL = 1;
				ZoomNoResize = 1;
				Video.ViewportHeight = Video.Height;
				Video.ViewportWidth = Video.Width;
				Video.Height = 0;
				Video.Width = 0;
				continue;
#endif
			case -1:
				break;
			case '?':
			case 'h':
			default:
				Usage();
				ExitFatal(-1);
		}
		break;
	}

	if (argc - optind > 1) {
		fprintf(stderr, "too many files\n");
		Usage();
		ExitFatal(-1);
	}

	if (argc - optind) {
		size_t index;
		CliMapName = argv[optind];
		while ((index = CliMapName.find('\\')) != std::string::npos) {
			CliMapName[index] = '/';
		}
	}
}

#ifdef USE_WIN32
static LONG WINAPI CreateDumpFile(EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE hFile = CreateFile("crash.dmp", GENERIC_READ | GENERIC_WRITE,    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,     NULL);
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ClientPointers = TRUE;
	mei.ExceptionPointers = ExceptionInfo;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
	fprintf(stderr, "Stratagus crashed!\n");
	fprintf(stderr, "A mini dump file \"crash.dmp\" has been created in the Stratagus folder.\n");
	//Wyrmgus start
//	fprintf(stderr, "Please send it to our bug tracker: https://bugs.launchpad.net/stratagus\n");
	fprintf(stderr, "Please send it to our bug tracker: https://github.com/Andrettin/Wyrmgus/issues\n");
//	fprintf(stderr, "and tell us what have you done to cause this bug.\n");
	fprintf(stderr, "and tell us what was happening when this bug occurred.\n");
	//Wyrmgus end
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

/**
**  The main program: initialise, parse options and arguments.
**
**  @param argc  Number of arguments.
**  @param argv  Vector of arguments.
*/
int stratagusMain(int argc, char **argv)
{
#ifdef REDIRECT_OUTPUT
	RedirectOutput();
#endif
#ifdef USE_BEOS
	//  Parse arguments for BeOS
	beos_init(argc, argv);
#endif
#ifdef USE_WIN32
	SetUnhandledExceptionFilter(CreateDumpFile);
#endif
#if defined(USE_WIN32) && ! defined(REDIRECT_OUTPUT)
	SetupConsole();
#endif
	//  Setup some defaults.
#ifndef MAC_BUNDLE
	StratagusLibPath = ".";
#else
	freopen("/tmp/stdout.txt", "w", stdout);
	freopen("/tmp/stderr.txt", "w", stderr);
	// Look for the specified data set inside the application bundle
	// This should be a subdir of the Resources directory
	CFURLRef pluginRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(),
												 CFSTR(MAC_BUNDLE_DATADIR), NULL, NULL);
	CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef,  kCFURLPOSIXPathStyle);
	const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
	Assert(pathPtr);
	StratagusLibPath = pathPtr;
#endif
#ifdef USE_STACKTRACE
	try {
#endif
	Parameters &parameters = Parameters::Instance;
	parameters.SetDefaultValues();
	parameters.SetLocalPlayerNameFromEnv();

	if (argc > 0) {
		parameters.applicationName = argv[0];
	}

	// FIXME: Parse options before or after scripts?
	ParseCommandLine(argc, argv, parameters);
	// Init the random number generator.
	InitSyncRand();

	makedir(parameters.GetUserDirectory().c_str(), 0777);

	// Init Lua and register lua functions!
	InitLua();
	LuaRegisterModules();

	// Initialise AI module
	InitAiModule();

	LoadCcl(parameters.luaStartFilename);

	PrintHeader();
	PrintLicense();

	// Setup video display
	InitVideo();

	// Setup sound card
	if (!InitSound()) {
		InitMusic();
	}

#ifndef DEBUG           // For debug it's better not to have:
	srand(time(NULL));  // Random counter = random each start
#endif

	//  Show title screens.
	SetDefaultTextColors(FontYellow, FontWhite);
	LoadFonts();
	SetClipping(0, 0, Video.Width - 1, Video.Height - 1);
	Video.ClearScreen();
	ShowTitleScreens();

	// Init player data
	ThisPlayer = NULL;
	//Don't clear the Players structure as it would erase the allowed units.
	// memset(Players, 0, sizeof(Players));
	NumPlayers = 0;

	UnitManager.Init(); // Units memory management
	PreMenuSetup();     // Load everything needed for menus

	MenuLoop();

	Exit(0);
#ifdef USE_STACKTRACE
	} catch (const std::exception &e) {
		fprintf(stderr, "Stratagus crashed!\n");
		//Wyrmgus start
//		fprintf(stderr, "Please send this call stack to our bug tracker: https://bugs.launchpad.net/stratagus\n");
		fprintf(stderr, "Please send this call stack to our bug tracker: https://github.com/Andrettin/Wyrmgus/issues\n");
//		fprintf(stderr, "and tell us what have you done to cause this bug.\n");
		fprintf(stderr, "and tell us what was happening when this bug occurred.\n");
		//Wyrmgus end
		fprintf(stderr, " === exception state traceback === \n");
		fprintf(stderr, "%s", e.what());
		exit(1);
	}
#endif
	return 0;
}

//Wyrmgus start
//Grand Strategy elements

bool GrandStrategy = false;				///if the game is in grand strategy mode
std::string GrandStrategyWorld;
int WorldMapOffsetX;
int WorldMapOffsetY;
bool GrandStrategyMapWidthIndent;
bool GrandStrategyMapHeightIndent;
CGrandStrategyGame GrandStrategyGame;

/**
**  Clean up the GrandStrategy elements.
*/
void CGrandStrategyGame::Clean()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (this->WorldMapTiles[x][y]) {
				delete this->WorldMapTiles[x][y];
			}
		}
	}
	this->WorldMapWidth = 0;
	this->WorldMapHeight = 0;
	
	for (int i = 0; i < WorldMapTerrainTypeMax; ++i) {
		if (this->TerrainTypes[i]) {
			delete this->TerrainTypes[i];
		}
	}
	
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->Provinces[i]) {
			delete this->Provinces[i];
		}
	}
	this->ProvinceCount = 0;
}

/**
**  Draw the grand strategy map.
*/
void CGrandStrategyGame::DrawMap()
{
	int grand_strategy_map_width = UI.MapArea.EndX - UI.MapArea.X;
	int grand_strategy_map_height = UI.MapArea.EndY - UI.MapArea.Y;
	
	int width_indent = 0;
	int height_indent = 0;
	if (GrandStrategyMapWidthIndent) {
		width_indent = -32;
	}
	if (GrandStrategyMapHeightIndent) {
		height_indent = -32;
	}
	
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64)) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64)) && y < GetWorldMapHeight(); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile) {
				if (GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->BaseTile != -1) { // should be changed into a more dynamic setting than being based on GrandStrategyWorld
					GrandStrategyGame.BaseTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				}
				
				GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				
				if (GrandStrategyGame.WorldMapTiles[x][y]->HasResource(GoldCost, false)) {
					GrandStrategyGame.GoldMineGraphics->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				}
				
				int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
				if (province_id != -1) {
					int civilization = GrandStrategyGame.Provinces[province_id]->Civilization;
					if (civilization != -1 && GrandStrategyGame.Provinces[province_id]->Owner[0] != -1 && GrandStrategyGame.Provinces[province_id]->Owner[1] != -1) {
						//draw the province's settlement
						if (GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y) {
							int player_color = PlayerRaces.FactionColors[GrandStrategyGame.Provinces[province_id]->Owner[0]][GrandStrategyGame.Provinces[province_id]->Owner[1]];
							
							GrandStrategyGame.SettlementGraphics[civilization]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
						}
					}
				}
			}
		}
	}
	
	//if is clicking on a tile, draw a square on its borders
	if (UI.MapArea.Contains(CursorScreenPos) && GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]->Terrain != -1 && (MouseButtons & LeftButton)) {
		int tile_screen_x = ((GrandStrategyGame.GetTileUnderCursor().x - WorldMapOffsetX) * 64) + UI.MapArea.X + width_indent;
		int tile_screen_y = ((GrandStrategyGame.GetTileUnderCursor().y - WorldMapOffsetY) * 64) + UI.MapArea.Y + height_indent;
			
//		clamp(&tile_screen_x, 0, Video.Width);
//		clamp(&tile_screen_y, 0, Video.Height);
			
		Video.DrawRectangle(ColorWhite, tile_screen_x, tile_screen_y, 64, 64);
	}
	
	//draw fog over terra incognita
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + floor(static_cast<double>(grand_strategy_map_width / 64))); ++x) {
		for (int y = WorldMapOffsetY; y <= std::min((WorldMapOffsetY + static_cast<int>(floor(static_cast<double>(grand_strategy_map_height / 64)))), (GetWorldMapHeight() - 1)); ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
				GrandStrategyGame.FogTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 16, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 16, true);
			}
		}
	}
}

/**
**  Draw the grand strategy tile tooltip.
*/
void CGrandStrategyGame::DrawTileTooltip(int x, int y)
{
	std::string tile_tooltip;
	
	int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner[0] != -1 && GrandStrategyGame.Provinces[province_id]->Owner[1] != -1 && GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y) {
		tile_tooltip += "Settlement";
		if (!GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName().empty()) {
			tile_tooltip += " of ";
			tile_tooltip += GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName();
		}
		tile_tooltip += " (";
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		tile_tooltip += ")";
	} else if (GrandStrategyGame.WorldMapTiles[x][y]->HasResource(GoldCost, false)) {
		tile_tooltip += "Gold Mine";
		tile_tooltip += " (";
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		tile_tooltip += ")";
	} else {
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
	}	
	
	if (province_id != -1) {
		tile_tooltip += ", ";
		tile_tooltip += GrandStrategyGame.Provinces[province_id]->GetCulturalName();
		
		if (GrandStrategyGame.Provinces[province_id]->Owner[0] != -1 && GrandStrategyGame.Provinces[province_id]->Owner[1] != -1) {
			tile_tooltip += ", ";
			tile_tooltip += PlayerRaces.FactionNames[GrandStrategyGame.Provinces[province_id]->Owner[0]][GrandStrategyGame.Provinces[province_id]->Owner[1]];
		}
	}
	tile_tooltip += " (";
	tile_tooltip += std::to_string((_Longlong)x);
	tile_tooltip += ", ";
	tile_tooltip += std::to_string((_Longlong)y);
	tile_tooltip += ")";
	CLabel(GetGameFont()).Draw(UI.StatusLine.TextX, UI.StatusLine.TextY, tile_tooltip);
}

/**
**  Draw the grand strategy tile tooltip.
*/
Vec2i CGrandStrategyGame::GetTileUnderCursor()
{
	Vec2i tile_under_cursor(0, 0);
	
	int width_indent = 0;
	int height_indent = 0;
	if (GrandStrategyMapWidthIndent) {
		width_indent = -32;
	}
	if (GrandStrategyMapHeightIndent) {
		height_indent = -32;
	}
			
	tile_under_cursor.x = WorldMapOffsetX + ((CursorScreenPos.x - UI.MapArea.X - width_indent) / 64);
	tile_under_cursor.y = WorldMapOffsetY + ((CursorScreenPos.y - UI.MapArea.Y - height_indent) / 64);
	
	return tile_under_cursor;
}

/**
**  Get whether the tile has a resource
*/
bool WorldMapTile::HasResource(int resource, bool ignore_prospection)
{
	if (resource == this->Resource && (this->ResourceProspected || ignore_prospection)) {
		return true;
	}
	return false;
}

/**
**  Get the province's cultural name.
*/
std::string CProvince::GetCulturalName()
{
	if (this->Owner[0] != -1 && this->Owner[1] != -1 && !this->Water && !this->FactionCulturalNames[this->Owner[0]][this->Owner[1]].empty() && this->Civilization == this->Owner[0]) {
		return this->FactionCulturalNames[this->Owner[0]][this->Owner[1]];
	} else if (!this->Water && this->Civilization != -1 && !this->CulturalNames[this->Civilization].empty()) {
		return this->CulturalNames[this->Civilization];
	} else if (
		this->Water && this->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner[1] != -1
		&& !GrandStrategyGame.Provinces[this->ReferenceProvince]->Water
		&& !this->FactionCulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner[0]][GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner[1]].empty()
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization == GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner[0]
	) {
		return this->FactionCulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner[0]][GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner[1]];
	} else if (
		this->Water && this->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization != -1
		&& !this->CulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization].empty()
	) {
		return this->CulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization];
	} else {
		return this->Name;
	}
}

/**
**  Get the province's cultural settlement name.
*/
std::string CProvince::GetCulturalSettlementName()
{
	if (!this->Water && this->Owner[0] != -1 && this->Owner[1] != -1 && !this->FactionCulturalSettlementNames[this->Owner[0]][this->Owner[1]].empty() && this->Civilization == this->Owner[0]) {
		return this->FactionCulturalSettlementNames[this->Owner[0]][this->Owner[1]];
	} else if (!this->Water && this->Civilization != -1 && !this->CulturalSettlementNames[this->Civilization].empty()) {
		return this->CulturalSettlementNames[this->Civilization];
	} else {
		return this->SettlementName;
	}
}

/**
**  Generate a province name for the civilization.
*/
std::string CProvince::GenerateProvinceName(int civilization)
{
	std::string province_name;
	
	if (
		!PlayerRaces.ProvinceNames[civilization][0].empty()
		|| !PlayerRaces.ProvinceNamePrefixes[civilization][0].empty()
		|| PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
	) {
		int ProvinceNameCount = 0;
		std::string ProvinceNames[PersonalNameMax];
		int ProvinceNamePrefixCount = 0;
		std::string ProvinceNamePrefixes[PersonalNameMax];
		int ProvinceNameSuffixCount = 0;
		std::string ProvinceNameSuffixes[PersonalNameMax];
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNames[civilization][i].empty()) {
				break;
			}
			ProvinceNames[ProvinceNameCount] = PlayerRaces.ProvinceNames[civilization][i];
			ProvinceNameCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNamePrefixes[civilization][i].empty()) {
				break;
			}
			ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.ProvinceNamePrefixes[civilization][i];
			ProvinceNamePrefixCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.ProvinceNameSuffixes[civilization][i].empty()) {
				break;
			}
			ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.ProvinceNameSuffixes[civilization][i];
			ProvinceNameSuffixCount += 1;
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixProvinceName) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						ProvinceNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						ProvinceNamePrefixCount += 1;
					}
				} else {
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
						ProvinceNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
						ProvinceNamePrefixCount += 1;
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					ProvinceNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixProvinceName) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					ProvinceNamePrefixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					ProvinceNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixProvinceName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					ProvinceNamePrefixes[ProvinceNamePrefixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					ProvinceNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixProvinceName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					ProvinceNameSuffixes[ProvinceNameSuffixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					ProvinceNameSuffixCount += 1;
				}
			}
		}
		
		if (ProvinceNameCount > 0 || ProvinceNamePrefixCount > 0 || ProvinceNameSuffixCount > 0) {
			int ProvinceNameProbability = ProvinceNameCount * 10000 / (ProvinceNameCount + (ProvinceNamePrefixCount * ProvinceNameSuffixCount));
			if (SyncRand(10000) < ProvinceNameProbability) {
				province_name = ProvinceNames[SyncRand(ProvinceNameCount)];
			} else {
				std::string prefix = ProvinceNamePrefixes[SyncRand(ProvinceNamePrefixCount)];
				std::string suffix = ProvinceNameSuffixes[SyncRand(ProvinceNameSuffixCount)];
				
				if (PlayerRaces.RequiresPlural(prefix, civilization)) {
					suffix = PlayerRaces.GetPluralForm(suffix, civilization);
				}
				
				suffix[0] = tolower(suffix[0]);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				province_name = prefix;
				province_name += suffix;
			}
		}
	}
	
	province_name = TransliterateText(province_name);
	
	return province_name;
}

/**
**  Generate a settlement name for the civilization.
**
**  @param l  Lua state.
*/
std::string CProvince::GenerateSettlementName(int civilization)
{
	std::string settlement_name;
	
	if (
		!PlayerRaces.SettlementNames[civilization][0].empty()
		|| !PlayerRaces.SettlementNamePrefixes[civilization][0].empty()
		|| PlayerRaces.LanguageNouns[civilization][0]
		|| PlayerRaces.LanguageVerbs[civilization][0]
		|| PlayerRaces.LanguageAdjectives[civilization][0]
	) {
		int SettlementNameCount = 0;
		std::string SettlementNames[PersonalNameMax];
		int SettlementNamePrefixCount = 0;
		std::string SettlementNamePrefixes[PersonalNameMax];
		int SettlementNameSuffixCount = 0;
		std::string SettlementNameSuffixes[PersonalNameMax];
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNames[civilization][i].empty()) {
				break;
			}
			SettlementNames[SettlementNameCount] = PlayerRaces.SettlementNames[civilization][i];
			SettlementNameCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNamePrefixes[civilization][i].empty()) {
				break;
			}
			SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.SettlementNamePrefixes[civilization][i];
			SettlementNamePrefixCount += 1;
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (PlayerRaces.SettlementNameSuffixes[civilization][i].empty()) {
				break;
			}
			SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.SettlementNameSuffixes[civilization][i];
			SettlementNameSuffixCount += 1;
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNouns[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->PrefixSettlementName) {
				if (PlayerRaces.LanguageNouns[civilization][i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
						SettlementNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
						SettlementNamePrefixCount += 1;
					}
				} else {
					if (!PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixSingular) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularGenitive;
						SettlementNamePrefixCount += 1;
					}
					if (!PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive.empty() && PlayerRaces.LanguageNouns[civilization][i]->PrefixPlural) {
						SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralGenitive;
						SettlementNamePrefixCount += 1;
					}
				}
			}
			if (PlayerRaces.LanguageNouns[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageNouns[civilization][i]->SingularNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixSingular) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->SingularNominative;
					SettlementNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageNouns[civilization][i]->PluralNominative.empty() && PlayerRaces.LanguageNouns[civilization][i]->SuffixPlural) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNouns[civilization][i]->PluralNominative;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageVerbs[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->PrefixSettlementName) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					SettlementNamePrefixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageVerbs[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePresent;
					SettlementNameSuffixCount += 1;
				}
				if (!PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageVerbs[civilization][i]->ParticiplePast;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageAdjectives[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->PrefixSettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageAdjectives[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageAdjectives[civilization][i]->Word.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageAdjectives[civilization][i]->Word;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		for (int i = 0; i < LanguageWordMax; ++i) {
			if (!PlayerRaces.LanguageNumerals[civilization][i]) {
				break;
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->PrefixSettlementName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					SettlementNamePrefixes[SettlementNamePrefixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					SettlementNamePrefixCount += 1;
				}
			}
			if (PlayerRaces.LanguageNumerals[civilization][i]->SuffixSettlementName) {
				if (!PlayerRaces.LanguageNumerals[civilization][i]->Word.empty()) {
					SettlementNameSuffixes[SettlementNameSuffixCount] = PlayerRaces.LanguageNumerals[civilization][i]->Word;
					SettlementNameSuffixCount += 1;
				}
			}
		}
		
		if (SettlementNameCount > 0 || SettlementNamePrefixCount > 0 || SettlementNameSuffixCount > 0) {
			int SettlementNameProbability = SettlementNameCount * 10000 / (SettlementNameCount + (SettlementNamePrefixCount * SettlementNameSuffixCount));
			if (SyncRand(10000) < SettlementNameProbability) {
				settlement_name = SettlementNames[SyncRand(SettlementNameCount)];
			} else {
				std::string prefix = SettlementNamePrefixes[SyncRand(SettlementNamePrefixCount)];
				std::string suffix = SettlementNameSuffixes[SyncRand(SettlementNameSuffixCount)];
				
				if (PlayerRaces.RequiresPlural(prefix, civilization)) {
					suffix = PlayerRaces.GetPluralForm(suffix, civilization);
				}
				
				suffix[0] = tolower(suffix[0]);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				settlement_name = prefix;
				settlement_name += suffix;
			}
		}
	}
	
	settlement_name = TransliterateText(settlement_name);
	
	return settlement_name;
}

/**
**  "Translate" (that is, adapt) a province name from one culture (civilization) to another.
*/
std::string CProvince::TranslateProvinceName(std::string province_name, int civilization)
{
	std::string new_province_name;

	// try to translate the entire name, as a particular translation for it may exist
	if (!PlayerRaces.ProvinceNameTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.ProvinceNameTranslations[civilization][i][0].empty() && PlayerRaces.ProvinceNameTranslations[civilization][i][0] == province_name) {
				new_province_name = PlayerRaces.ProvinceNameTranslations[civilization][i][1];
				return new_province_name;
			}
		}
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (!PlayerRaces.ProvinceNamePrefixTranslations[civilization][0][0].empty() && !PlayerRaces.ProvinceNameSuffixTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0].empty() && PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0] == province_name.substr(0, PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0].size())) {
				for (int j = 0; j < PersonalNameMax; ++j) {
					if (!PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][0].empty() && PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][0] == province_name.substr(PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][0].size(), PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][0].size())) {
						new_province_name = PlayerRaces.ProvinceNamePrefixTranslations[civilization][i][1];
						new_province_name += PlayerRaces.ProvinceNameSuffixTranslations[civilization][j][1];
						return new_province_name;
					}
				}
			}
		}
	}
	
	return new_province_name;
}

/**
**  "Translate" (that is, adapt) a settlement name from one culture (civilization) to another.
*/
std::string CProvince::TranslateSettlementName(std::string settlement_name, int civilization)
{
	std::string new_settlement_name;

	// try to translate the entire name, as a particular translation for it may exist
	if (!PlayerRaces.SettlementNameTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.SettlementNameTranslations[civilization][i][0].empty() && PlayerRaces.SettlementNameTranslations[civilization][i][0] == settlement_name) {
				new_settlement_name = PlayerRaces.SettlementNameTranslations[civilization][i][1];
				return new_settlement_name;
			}
		}
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (!PlayerRaces.SettlementNamePrefixTranslations[civilization][0][0].empty() && !PlayerRaces.SettlementNameSuffixTranslations[civilization][0][0].empty()) {
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0].empty() && PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0] == settlement_name.substr(0, PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0].size())) {
				for (int j = 0; j < PersonalNameMax; ++j) {
					if (!PlayerRaces.SettlementNameSuffixTranslations[civilization][j][0].empty() && PlayerRaces.SettlementNameSuffixTranslations[civilization][j][0] == settlement_name.substr(PlayerRaces.SettlementNamePrefixTranslations[civilization][i][0].size(), PlayerRaces.SettlementNameSuffixTranslations[civilization][j][0].size())) {
						new_settlement_name = PlayerRaces.SettlementNamePrefixTranslations[civilization][i][1];
						new_settlement_name += PlayerRaces.SettlementNameSuffixTranslations[civilization][j][1];
						return new_settlement_name;
					}
				}
			}
		}
	}
	
	return new_settlement_name;
}

/**
**  Get the width of the world map.
*/
int GetWorldMapWidth()
{
	return GrandStrategyGame.WorldMapWidth;
}

/**
**  Get the height of the world map.
*/
int GetWorldMapHeight()
{
	return GrandStrategyGame.WorldMapHeight;
}

/**
**  Get the terrain type of a world map tile.
*/
std::string GetWorldMapTileTerrain(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
		return "";
	}
	
	return GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
}

/**
**  Get the terrain variation of a world map tile.
*/
int GetWorldMapTileTerrainVariation(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1);
	
	return GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1;
}

/**
**  Get the graphic tile of a world map tile.
*/
std::string GetWorldMapTileProvinceName(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Province != -1) {
		return GrandStrategyGame.Provinces[GrandStrategyGame.WorldMapTiles[x][y]->Province]->Name;
	} else {
		return "";
	}
}

/**
**  Get the ID of a world map terrain type
*/
int GetWorldMapTerrainTypeId(std::string terrain_type_name)
{
	for (int i = 0; i < WorldMapTerrainTypeMax; ++i) {
		if (!GrandStrategyGame.TerrainTypes[i]) {
			break;
		}
		
		if (GrandStrategyGame.TerrainTypes[i]->Name == terrain_type_name) {
			return i;
		}
	}
	return -1;
}

/**
**  Get the ID of a province
*/
int GetProvinceId(std::string province_name)
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (!GrandStrategyGame.Provinces[i]) {
			break;
		}
		
		if (!GrandStrategyGame.Provinces[i]->Name.empty() && GrandStrategyGame.Provinces[i]->Name == province_name) {
			return i;
		}
	}
	return -1;
}

/**
**  Set the size of the world map.
*/
void SetWorldMapSize(int width, int height)
{
	Assert(width <= WorldMapWidthMax);
	Assert(height <= WorldMapHeightMax);
	GrandStrategyGame.WorldMapWidth = width;
	GrandStrategyGame.WorldMapHeight = height;
	
	//create new world map tile objects for the size, if necessary
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (!GrandStrategyGame.WorldMapTiles[x][y]) {
				WorldMapTile *world_map_tile = new WorldMapTile;
				GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
				GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
				GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
			}
		}
	}
}

/**
**  Set the terrain type of a world map tile.
*/
void SetWorldMapTileTerrain(int x, int y, int terrain)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	//if tile doesn't exist, create it now
	if (!GrandStrategyGame.WorldMapTiles[x][y]) {
		WorldMapTile *world_map_tile = new WorldMapTile;
		GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Terrain = terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//randomly select a variation for the world map tile
		if (GrandStrategyGame.TerrainTypes[terrain]->Variations > 0) {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = SyncRand(GrandStrategyGame.TerrainTypes[terrain]->Variations);
		} else {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
		}
	}
}

void SetWorldMapTileProvince(int x, int y, std::string province_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	GrandStrategyGame.WorldMapTiles[x][y]->Province = GetProvinceId(province_name);
}

/**
**  Set the terrain type of a world map tile.
*/
void CalculateWorldMapTileGraphicTile(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	
	int terrain = GrandStrategyGame.WorldMapTiles[x][y]->Terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//set the GraphicTile for this world map tile
		std::string graphic_tile = "tilesets/world/terrain/";
		graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
		
		if (GrandStrategyGame.TerrainTypes[terrain]->HasTransitions) {
			graphic_tile += "/";
			graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
			if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north";
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south";
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west";
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east";
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_outer";
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_outer";
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_outer";
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southeast_outer";
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_south";
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_east";
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_southwest_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = std::min(GrandStrategyGame.WorldMapTiles[x][y]->Variation, 1);
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_northeast_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_southwest_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_southeast_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_southeast_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northwest_outer_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_northeast_outer_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southwest_outer_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_southeast_outer_northwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_north_southwest_inner_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_south_northwest_inner_northeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_west_northeast_inner_southeast_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_east_northwest_inner_southwest_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else if (GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y) && GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)) {
				graphic_tile += "_outer";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			} else {
				graphic_tile += "_inner";
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
			}
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1) {
			graphic_tile += "_";
			graphic_tile += std::to_string((_Longlong)GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1);
		}
		
		graphic_tile += ".png";
		
		if (!CanAccessFile(graphic_tile.c_str())) {
			graphic_tile = FindAndReplaceString(graphic_tile, "2", "1");
		}
		
		if (CGraphic::Get(graphic_tile) == NULL) {
			CGraphic *tile_graphic = CGraphic::New(graphic_tile, 64, 64);
			tile_graphic->Load();
		}
		GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile = CGraphic::Get(graphic_tile);
	}
}

void AddWorldMapResource(std::string resource_name, int x, int y, bool discovered)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		for (int i = 0; i < WorldMapResourceMax; ++i) {
			if (GrandStrategyGame.WorldMapResources[resource][i][0] == -1 && GrandStrategyGame.WorldMapResources[resource][i][1] == -1 && GrandStrategyGame.WorldMapResources[resource][i][2] == 0) { //if this spot for a world map resource is blank
				GrandStrategyGame.WorldMapResources[resource][i][0] = x;
				GrandStrategyGame.WorldMapResources[resource][i][1] = y;
				GrandStrategyGame.WorldMapResources[resource][i][2] = discovered ? 1 : 0;
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = resource;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected = discovered;
				break;
			}
		}
	}
}

void SetWorldMapResourceProspected(std::string resource_name, int x, int y, bool discovered)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		for (int i = 0; i < WorldMapResourceMax; ++i) {
			if (GrandStrategyGame.WorldMapResources[resource][i][0] == x && GrandStrategyGame.WorldMapResources[resource][i][1] == y) {
				GrandStrategyGame.WorldMapResources[resource][i][2] = discovered ? 1 : 0;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected = discovered;
				break;
			}
		}
	}
}

/**
**  Get the cultural name of a province
*/
std::string GetProvinceCulturalName(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		return GrandStrategyGame.Provinces[province_id]->GetCulturalName();
	}
	
	return "";
}

/**
**  Get the cultural name of a province pertaining to a particular civilization
*/
std::string GetProvinceCivilizationCulturalName(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			return GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization];
		}
	}
	
	return "";
}

/**
**  Get the cultural name of a province pertaining to a particular faction
*/
std::string GetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				return GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[civilization][faction];
			}
		}
	}
	
	return "";
}

/**
**  Get the cultural name of a province
*/
std::string GetProvinceCulturalSettlementName(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		return GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName();
	}
	
	return "";
}

/**
**  Get the cultural settlement name of a province pertaining to a particular civilization
*/
std::string GetProvinceCivilizationCulturalSettlementName(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			return GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization];
		}
	}
	
	return "";
}

/**
**  Get the cultural settlement name of a province pertaining to a particular faction
*/
std::string GetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				return GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[civilization][faction];
			}
		}
	}
	
	return "";
}

void SetProvinceName(std::string old_province_name, std::string new_province_name)
{
	int province_id = GetProvinceId(old_province_name);
	
	if (province_id == -1 || !GrandStrategyGame.Provinces[province_id]) { //if province doesn't exist, create it now
		CProvince *province = new CProvince;
		province_id = GrandStrategyGame.ProvinceCount;
		GrandStrategyGame.Provinces[province_id] = province;
		GrandStrategyGame.ProvinceCount += 1;
	}
	
	GrandStrategyGame.Provinces[province_id]->Name = new_province_name;
}

void SetProvinceWater(std::string province_name, bool water)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->Water = water;
	}
}

void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->Owner[0] = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		GrandStrategyGame.Provinces[province_id]->Owner[1] = PlayerRaces.GetFactionIndexByName(GrandStrategyGame.Provinces[province_id]->Owner[0], faction_name);
	}
}

void SetProvinceCivilization(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	int old_civilization = GrandStrategyGame.Provinces[province_id]->Civilization;
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		GrandStrategyGame.Provinces[province_id]->Civilization = civilization;
			
		if (civilization != -1) {
			// create a new cultural name for the province, if there isn't any
			if (
				GrandStrategyGame.Provinces[province_id]->Owner[1] != -1
				&& GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[GrandStrategyGame.Provinces[province_id]->Owner[0]][GrandStrategyGame.Provinces[province_id]->Owner[1]].empty()
				&& GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization].empty()
			) {
				std::string new_province_name = "";
				// first see if can translate the cultural name of the old civilization
				if (old_civilization != -1 && !GrandStrategyGame.Provinces[province_id]->CulturalNames[old_civilization].empty()) {
					new_province_name = GrandStrategyGame.Provinces[province_id]->TranslateProvinceName(GrandStrategyGame.Provinces[province_id]->CulturalNames[old_civilization], civilization);
				}
				if (new_province_name == "") { // try to translate any cultural name
					for (int i = 0; i < MAX_RACES; ++i) {
						if (!GrandStrategyGame.Provinces[province_id]->CulturalNames[i].empty()) {
							new_province_name = GrandStrategyGame.Provinces[province_id]->TranslateProvinceName(GrandStrategyGame.Provinces[province_id]->CulturalNames[i], civilization);
							if (!new_province_name.empty()) {
								break;
							}
						}
					}
				}
				if (new_province_name == "") { // if trying to translate all cultural names failed, generate a new name
					new_province_name = GrandStrategyGame.Provinces[province_id]->GenerateProvinceName(civilization);
				}
				if (new_province_name != "") {
					GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization] = new_province_name;
				}
			}
			
			// create a new cultural name for the province's settlement, if there isn't any
			if (
				GrandStrategyGame.Provinces[province_id]->Owner[1] != -1
				&& GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[GrandStrategyGame.Provinces[province_id]->Owner[0]][GrandStrategyGame.Provinces[province_id]->Owner[1]].empty()
				&& GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization].empty()
			) {
				std::string new_settlement_name = "";
				// first see if can translate the cultural name of the old civilization
				if (old_civilization != -1 && !GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[old_civilization].empty()) {
					new_settlement_name = GrandStrategyGame.Provinces[province_id]->TranslateSettlementName(GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[old_civilization], civilization);
				}
				if (new_settlement_name == "") { // try to translate any cultural name
					for (int i = 0; i < MAX_RACES; ++i) {
						if (!GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[i].empty()) {
							new_settlement_name = GrandStrategyGame.Provinces[province_id]->TranslateSettlementName(GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[i], civilization);
							if (!new_settlement_name.empty()) {
								break;
							}
						}
					}
				}
				if (new_settlement_name == "") { // if trying to translate all cultural names failed, generate a new name
					new_settlement_name = GrandStrategyGame.Provinces[province_id]->GenerateSettlementName(civilization);
				}
				if (new_settlement_name != "") {
					GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization] = new_settlement_name;
				}
			}
		}
	}
}

void SetProvinceSettlementName(std::string province_name, std::string settlement_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SettlementName = settlement_name;
	}
}

void SetProvinceSettlementLocation(std::string province_name, int x, int y)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SettlementLocation.x = x;
		GrandStrategyGame.Provinces[province_id]->SettlementLocation.y = y;
	}
}

void SetProvinceCulturalName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization] = province_cultural_name;
		}
	}
}

void SetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[civilization][faction] = province_cultural_name;
			}
		}
	}
}

void SetProvinceCulturalSettlementName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization] = province_cultural_name;
		}
	}
}

void SetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[civilization][faction] = province_cultural_name;
			}
		}
	}
}

void SetProvinceReferenceProvince(std::string province_name, std::string reference_province_name)
{
	int province_id = GetProvinceId(province_name);
	int reference_province_id = GetProvinceId(reference_province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && reference_province_id != -1) {
		GrandStrategyGame.Provinces[province_id]->ReferenceProvince = reference_province_id;
	}
}

/**
**  Clean the grand strategy variables.
*/
void CleanGrandStrategyGame()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]) {
				GrandStrategyGame.WorldMapTiles[x][y]->Terrain = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Province = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
//				GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile = "";
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected = false;
				GrandStrategyGame.WorldMapTiles[x][y]->Position.x = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Position.y = -1;
			}
		}
	}
	for (int i = 0; i < ProvinceMax; ++i) {
		if (GrandStrategyGame.Provinces[i]) {
			GrandStrategyGame.Provinces[i]->Name = "";
			GrandStrategyGame.Provinces[i]->SettlementName = "";
			GrandStrategyGame.Provinces[i]->Civilization = -1;
			GrandStrategyGame.Provinces[i]->Owner[0] = -1;
			GrandStrategyGame.Provinces[i]->Owner[1] = -1;
			GrandStrategyGame.Provinces[i]->ReferenceProvince = -1;
			GrandStrategyGame.Provinces[i]->Water = false;
			GrandStrategyGame.Provinces[i]->SettlementLocation.x = -1;
			GrandStrategyGame.Provinces[i]->SettlementLocation.y = -1;
			for (int j = 0; j < MAX_RACES; ++j) {
				GrandStrategyGame.Provinces[i]->CulturalNames[j] = "";
				GrandStrategyGame.Provinces[i]->CulturalSettlementNames[j] = "";
				for (int k = 0; k < FactionMax; ++k) {
					GrandStrategyGame.Provinces[i]->FactionCulturalNames[j][k] = "";
					GrandStrategyGame.Provinces[i]->FactionCulturalSettlementNames[j][k] = "";
				}
			}
		}
	}
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			GrandStrategyGame.WorldMapResources[i][j][0] = -1;
			GrandStrategyGame.WorldMapResources[i][j][1] = -1;
			GrandStrategyGame.WorldMapResources[i][j][2] = 0;
		}
	}
	GrandStrategyGame.WorldMapWidth = 0;
	GrandStrategyGame.WorldMapHeight = 0;
	GrandStrategyGame.ProvinceCount = 0;
}

void InitializeGrandStrategyGame()
{
	//set the base tile here, if it hasn't been gotten yet
	std::string base_tile_filename;
	if (GrandStrategyWorld == "Nidavellir") {
		base_tile_filename = "tilesets/world/terrain/dark_plains.png";
	} else {
		base_tile_filename = "tilesets/world/terrain/plains.png";
	}
	if (CGraphic::Get(base_tile_filename) == NULL) {
		CGraphic *base_tile_graphic = CGraphic::New(base_tile_filename, 64, 64);
		base_tile_graphic->Load();
	}
	GrandStrategyGame.BaseTile = CGraphic::Get(base_tile_filename);
	
	//do the same for the fog tile now
	std::string fog_graphic_tile = "tilesets/world/terrain/fog.png";
	if (CGraphic::Get(fog_graphic_tile) == NULL) {
		CGraphic *fog_tile_graphic = CGraphic::New(fog_graphic_tile, 96, 96);
		fog_tile_graphic->Load();
	}
	GrandStrategyGame.FogTile = CGraphic::Get(fog_graphic_tile);
	
	//and for the gold mine
	std::string gold_mine_graphics_file = "tilesets/world/sites/gold_mine.png";
	if (CGraphic::Get(gold_mine_graphics_file) == NULL) {
		CGraphic *gold_mine_graphics = CGraphic::New(gold_mine_graphics_file, 64, 64);
		gold_mine_graphics->Load();
	}
	GrandStrategyGame.GoldMineGraphics = CGraphic::Get(gold_mine_graphics_file);
	
	for (int i = 0; i < MAX_RACES; ++i) {
		std::string settlement_graphics_file = "tilesets/world/sites/";
		settlement_graphics_file += PlayerRaces.Name[i];
		settlement_graphics_file += "_settlement.png";
		if (!CanAccessFile(settlement_graphics_file.c_str()) && !PlayerRaces.ParentCivilization[i].empty()) {
			settlement_graphics_file = FindAndReplaceString(settlement_graphics_file, PlayerRaces.Name[i], PlayerRaces.ParentCivilization[i]);
		}
		if (CanAccessFile(settlement_graphics_file.c_str())) {
			if (CPlayerColorGraphic::Get(settlement_graphics_file) == NULL) {
				CPlayerColorGraphic *settlement_graphics = CPlayerColorGraphic::New(settlement_graphics_file, 64, 64);
				settlement_graphics->Load();
			}
			GrandStrategyGame.SettlementGraphics[i] = CPlayerColorGraphic::Get(settlement_graphics_file);
		}
	}
}
//Wyrmgus end

//@}
