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
//      (c) Copyright 1998-2021 by Lutz Sammer, Francois Beerten,
//      Jimmy Salmon, Pali Rohár, cybermind and Andrettin
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
** @section Information Information
**
** Visit the https://github.com/Andrettin/Wyrmgus web page for the latest news and
** <A HREF="../index.html">Stratagus Info</A> for other documentation.
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
** @see unit.h @see unit.cpp @see unit_type.h @see unit_type.cpp
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
** @see icon.h @see icon.cpp
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

#include "stratagus.h"

#include "ai.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "database/database.h"
#include "database/preferences.h"
#include "editor.h"
#include "game.h"
#include "guichan.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map.h"
#include "netconnect.h"
#include "network.h"
#include "parameters.h"
#include "player.h"
#include "replay.h"
#include "results.h"
#include "script.h"
#include "settings.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "time/calendar.h"
#include "time/timeline.h"
#include "title.h"
#include "translate.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit_manager.h"
#include "version.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/video.h"
#include "widgets.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/util.h"

#include "missile.h" //for FreeBurningBuildingFrames

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

#ifdef __MORPHOS__
unsigned long __stack = 1000000;
__attribute__ ((section(".text"))) UBYTE VString[] = "$VER: Wyrmsun " VERSION "\r\n";
#endif

/// Name, Version, Copyright
const char NameLine[] = NAME " v" VERSION ", " COPYRIGHT;

std::string CliMapName;				/// Filename of the map given on the command line
std::string MenuRace;

bool EnableDebugPrint;				/// if enabled, print the debug messages
bool EnableAssert;					/// if enabled, halt on assertion failures
bool EnableUnitDebug;				/// if enabled, a unit info dump will be created

/*============================================================================
==  MAIN
============================================================================*/

void PreMenuSetup()
{
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
	current_interface_state = interface_state::menu;
	//  Clear screen
	Video.ClearScreen();

	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	CurrentCursorState = CursorState::Point;
	GameCursor = nullptr;

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
#ifdef __MORPHOS__
		"MORPHOS "
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
#ifdef USE_TOUCHSCREEN
		"TOUCHSCREEN "
#endif
		"";

	fprintf(stdout,
			"%s\n  written by Lutz Sammer, Fabrice Rossi, Vladi Shabanski, Patrice Fortier,\n"
			"  Jon Gabrielson, Andreas Arens, Nehal Mistry, Jimmy Salmon, Pali Rohar,\n"
			"  cybermind, Andrettin and others.\n"
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
	CleanModules();
	FreeBurningBuildingFrames();
	FreeButtonStyles();
	FreeAllContainers();
	freeGuichan();
	DebugPrint("Frames %lu, Slow frames %d = %ld%%\n" _C_
			   FrameCounter _C_ SlowFrameCounter _C_
			   (SlowFrameCounter * 100) / (FrameCounter ? FrameCounter : 1));
	lua_settop(Lua, 0);
	lua_close(Lua);
	Lua = nullptr;
	DeInitVideo();

	fprintf(stdout, "%s", _("Thanks for playing " NAME ".\n"));

	QMetaObject::invokeMethod(QApplication::instance(), [err] { QApplication::exit(err); }, Qt::QueuedConnection);
}

/**
**  Do a fatal exit.
**  Called on out of memory or crash.
**
**  @param err  Error code to pass to shell.
*/
void ExitFatal(int err)
{
	log::log_stacktrace();
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
		"\t-G \"options\"\tGame options (passed to game scripts)\n"
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
		"\t-x idx\t\tControls fullscreen scaling if your graphics card supports shaders.\n"\
		"\t  \t\tPass 1 for nearest-neigubour, 2 for EPX/AdvMame, 3 for HQx, 4 for SAL, 5 for SuperEagle\n"\
		"\t  \t\tYou can also use Ctrl+Alt+/ to cycle between these scaling algorithms at runtime.\n"
		"\t  \t\tPass -1 to force old-school nearest neighbour scaling without shaders\n"\
		"\t-Z\t\tUse OpenGL to scale the screen to the viewport (retro-style). Implies -O.\n"
#endif
		"map is relative to the root data path, use ./map for relative to cwd\n",
		Parameters::Instance.applicationName.c_str());
}

#ifdef REDIRECT_OUTPUT

static std::string stdoutFile;
static std::string stderrFile;

static void CleanupOutput()
{
	std::cerr.clear();
	fclose(stdout);
	fclose(stderr);

	struct stat st;
	if (stat(stdoutFile.c_str(), &st) == 0 && st.st_size == 0) {
		std::filesystem::remove(stdoutFile);
	}
	if (stat(stderrFile.c_str(), &st) == 0 && st.st_size == 0) {
		std::filesystem::remove(stderrFile);
	}
}

static void RedirectOutput()
{
	std::string path = Parameters::Instance.GetUserDirectory();

	makedir(path.c_str(), 0777);
	
	stdoutFile = path + "\\stdout.txt";
	stderrFile = path + "\\stderr.txt";

	//if the log file is larger than the max log size, delete it before opening it for writing
	if (std::filesystem::exists(stderrFile) && std::filesystem::file_size(stderrFile) > wyrmgus::log::max_size) {
		std::filesystem::remove(stderrFile);
	}

	if (!freopen(stdoutFile.c_str(), "w", stdout)) {
		printf("freopen stdout failed");
	}
	if (!freopen(stderrFile.c_str(), "a", stderr)) {
		printf("freopen stderr failed");
	}
	atexit(CleanupOutput);
}
#endif

static void ParseCommandLine(int argc, char **argv, Parameters &parameters)
{
	for (;;) {
		switch (getopt(argc, argv, "ac:d:D:eE:FG:hiI:lN:oOP:ps:S:u:v:Wx:Z?-")) {
			case 'a':
				EnableAssert = true;
				continue;
			case 'c':
				parameters.luaStartFilename = optarg;
				if (strlen(optarg) > 4 &&
					!(strstr(optarg, ".lua") == optarg + strlen(optarg) - 4)) {
						parameters.luaStartFilename += ".lua";
				}
				continue;
			case 'd': {
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
			case 'G':
				parameters.luaScriptArguments = optarg;
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
		fprintf(stderr, "too many map files. if you meant to pass game arguments, these go after '--'\n");
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
	HANDLE hFile = CreateFile("crash.dmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	MINIDUMP_EXCEPTION_INFORMATION mei{};
	mei.ThreadId = GetCurrentThreadId();
	mei.ClientPointers = TRUE;
	mei.ExceptionPointers = ExceptionInfo;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
	fprintf(stderr, "" NAME " crashed!\n");
	fprintf(stderr, "A mini dump file \"crash.dmp\" has been created in the Stratagus folder.\n");
	fprintf(stderr, "Please send this call stack to our bug tracker: " HOMEPAGE "/issues\n");
	fprintf(stderr, "and tell us what caused this bug to occur.\n");
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

/**
**  The main program: initialise, parse options and arguments.
**
**  @param argc  Number of arguments.
**  @param argv  Vector of arguments.
*/
void stratagusMain(int argc, char **argv)
{
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

	Parameters &parameters = Parameters::Instance;
	parameters.SetDefaultValues();
	parameters.SetLocalPlayerNameFromEnv();

#ifdef REDIRECT_OUTPUT
	RedirectOutput();
#endif

	if (argc > 0) {
		parameters.applicationName = argv[0];
	}

	// FIXME: Parse options before or after scripts?
	ParseCommandLine(argc, argv, parameters);

	makedir(parameters.GetUserDirectory().c_str(), 0777);

	// Init Lua and register lua functions!
	InitLua();
	LuaRegisterModules();

	for (size_t p = CPlayer::Players.size(); p < PlayerMax; ++p) {
		CPlayer::Players.push_back(new CPlayer);
	}

	// Initialise AI module
	InitAiModule();

	LoadCcl(parameters.luaStartFilename, parameters.luaScriptArguments);

	PrintHeader();
	PrintLicense();

	// Setup video display
	InitVideo();

	//setup sound
	if (InitSound()) {
		InitMusic();
	}

#ifndef DEBUG			// For debug it's better not to have:
	srand(time(nullptr));	// Random counter = random each start
#endif

	//  Show title screens.
	SetClipping(0, 0, Video.Width - 1, Video.Height - 1);
	Video.ClearScreen();
	ShowTitleScreens();

	// Init player data
	CPlayer::SetThisPlayer(nullptr);
	//Don't clear the Players structure as it would erase the allowed units.
	// memset(Players, 0, sizeof(Players));

	NumPlayers = 0;

	wyrmgus::unit_manager::get()->init();	// Units memory management
	PreMenuSetup();		// Load everything needed for menus

	try {
		MenuLoop();
	} catch (const std::exception &exception) {
		wyrmgus::exception::report(exception);
		Exit(EXIT_FAILURE);
		return;
	}

	Exit(0);
}

//Wyrmgus start
int GetReverseDirection(int direction)
{
	if (direction == North) {
		return South;
	} else if (direction == Northeast) {
		return Southwest;
	} else if (direction == East) {
		return West;
	} else if (direction == Southeast) {
		return Northwest;
	} else if (direction == South) {
		return North;
	} else if (direction == Southwest) {
		return Northeast;
	} else if (direction == West) {
		return East;
	} else if (direction == Northwest) {
		return Southeast;
	}
	return -1;
}

std::string GetDirectionNameById(int direction)
{
	if (direction == North) {
		return "north";
	} else if (direction == Northeast) {
		return "northeast";
	} else if (direction == East) {
		return "east";
	} else if (direction == Southeast) {
		return "southeast";
	} else if (direction == South) {
		return "south";
	} else if (direction == Southwest) {
		return "southwest";
	} else if (direction == West) {
		return "west";
	} else if (direction == Northwest) {
		return "northwest";
	}
	return "";
}

int GetDirectionIdByName(const std::string &direction)
{
	if (direction == "north") {
		return North;
	} else if (direction == "northeast") {
		return Northeast;
	} else if (direction == "east") {
		return East;
	} else if (direction == "southeast") {
		return Southeast;
	} else if (direction == "south") {
		return South;
	} else if (direction == "southwest") {
		return Southwest;
	} else if (direction == "west") {
		return West;
	} else if (direction == "northwest") {
		return Northwest;
	} else {
		return -1;
	}
}

int GetDirectionFromOffset(int x, int y)
{
	if (x < 0 && y == 0) {
		return West;
	} else if (x > 0 && y == 0) {
		return East;
	} else if (y < 0 && x == 0) {
		return North;
	} else if (y > 0 && x == 0) {
		return South;
	} else if (x < 0 && y < 0) {
		return Northwest;
	} else if (x > 0 && y < 0) {
		return Northeast;
	} else if (x < 0 && y > 0) {
		return Southwest;
	} else if (x > 0 && y > 0) {
		return Southeast;
	}

	return -1;
}

Vec2i GetDirectionOffset(int direction)
{
	Vec2i offset(0, 0);
			
	if (direction == North || direction == Northwest || direction == Northeast) {
		offset.y = -1;
	} else if (direction == South || direction == Southwest || direction == Southeast) {
		offset.y = 1;
	}
	if (direction == West || direction == Northwest || direction == Southwest) {
		offset.x = -1;
	} else if (direction == East || direction == Northeast || direction == Southeast) {
		offset.x = 1;
	}

	return offset;
}
//Wyrmgus end

void load_database(const bool initial_definition)
{
	try {
		wyrmgus::database::get()->load(initial_definition);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error loading database."));
	}
}

void load_defines()
{
	try {
		//load the preferences before the defines, as the latter depend on the preferences
		wyrmgus::preferences::get()->load();
		wyrmgus::database::get()->load_defines();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error loading defines."));
	}
}

void initialize_database()
{
	try {
		wyrmgus::database::get()->initialize();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error initializing database."));
	}
}

void save_preferences()
{
	wyrmgus::preferences::get()->save();
}
