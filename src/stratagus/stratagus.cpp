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
//      (c) Copyright 1998-2022 by Lutz Sammer, Francois Beerten,
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

#include "stratagus.h"

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

#include "ai.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "database/database.h"
#include "database/preferences.h"
#include "editor.h"
#include "engine_interface.h"
#include "game/game.h"
#include "guichan.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/direction.h"
#include "map/map.h"
#include "network/netconnect.h"
#include "network/network.h"
#include "parameters.h"
#include "player/player.h"
#include "replay.h"
#include "results.h"
#include "script.h"
#include "settings.h"
#include "sound/music_player.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "time/calendar.h"
#include "time/timeline.h"
#include "title.h"
#include "translator.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit_manager.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/thread_pool.h"
#include "util/util.h"
#include "version.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/video.h"
#include "widgets.h"

#include "missile.h" //for FreeBurningBuildingFrames

#include <qcoro/core/qcorotimer.h>

#ifdef USE_WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

#ifdef __MORPHOS__
unsigned long __stack = 1000000;
__attribute__ ((section(".text"))) UBYTE VString[] = "$VER: Wyrmsun " VERSION "\r\n";
#endif

bool EnableDebugPrint;				/// if enabled, print the debug messages

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
	cursor::set_current_cursor(nullptr, true);

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

	//Name, Version, Copyright
	const std::string name_line = (QApplication::applicationName() + " v" + QApplication::applicationVersion() + ", " + COPYRIGHT).toStdString();

	fprintf(stdout,
			"%s\n  written by Lutz Sammer, Fabrice Rossi, Vladi Shabanski, Patrice Fortier,\n"
			"  Jon Gabrielson, Andreas Arens, Nehal Mistry, Jimmy Salmon, Pali Rohar,\n"
			"  cybermind, Andrettin and others.\n"
			"\t" HOMEPAGE "\n"
			"Compile options %s",
			name_line.c_str(), CompileOptions.c_str());
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
QCoro::Task<void> Exit(const int err)
{
	if (GameRunning && err != EXIT_FAILURE) {
		StopGame(GameExit);
		co_return;
	}
	
	co_await NetworkQuitGame();

	QMetaObject::invokeMethod(QApplication::instance(), [err] {
		QApplication::exit(err);
	}, Qt::QueuedConnection);
}

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
	const std::filesystem::path path = parameters::get()->GetUserDirectory();
	const std::string path_str = path::to_string(path);

	makedir(path_str.c_str(), 0777);
	
	stdoutFile = path_str + "\\stdout.txt";
	stderrFile = path_str + "\\stderr.txt";

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

#ifdef USE_WIN32
static LONG WINAPI CreateDumpFile(EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE hFile = CreateFile(L"crash.dmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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

void stratagus_on_exit_cleanup()
{
	music_player::get()->stop();
	QuitSound();

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

	fprintf(stdout, "%s", _("Thanks for playing " NAME ".\n"));
}

/**
**  The main program: initialise, parse options and arguments.
**
**  @param argc  Number of arguments.
**  @param argv  Vector of arguments.
*/
QCoro::Task<void> stratagusMain(int argc, char **argv)
{
	Q_UNUSED(argc)
	Q_UNUSED(argv)

#ifdef USE_BEOS
	//  Parse arguments for BeOS
	beos_init(argc, argv);
#endif
#ifdef USE_WIN32
	SetUnhandledExceptionFilter(CreateDumpFile);
#endif

	parameters *parameters = parameters::get();

	RedirectOutput();

	makedir(path::to_string(parameters->GetUserDirectory()).c_str(), 0777);

	// Init Lua and register lua functions!
	InitLua();
	LuaRegisterModules();

	for (size_t p = CPlayer::Players.size(); p < PlayerMax; ++p) {
		auto player = make_qunique<CPlayer>(static_cast<int>(p));

		player->moveToThread(QApplication::instance()->thread());

		CPlayer::Players.push_back(std::move(player));
	}

	// Initialise AI module
	InitAiModule();

	LoadCcl(parameters->luaStartFilename, parameters->luaScriptArguments);

	PrintHeader();
	PrintLicense();

	// Setup video display
	InitVideo();

	//setup sound
	InitSound();

	//  Show title screens.
	SetClipping(0, 0, Video.Width - 1, Video.Height - 1);
	Video.ClearScreen();

	// Init player data
	CPlayer::SetThisPlayer(nullptr);
	//Don't clear the Players structure as it would erase the allowed units.
	// memset(Players, 0, sizeof(Players));

	NumPlayers = 0;

	unit_manager::get()->init();	// Units memory management
	PreMenuSetup();		// Load everything needed for menus

	MenuLoop();

	if (parameters::get()->is_test_run()) {
		co_await Exit(EXIT_SUCCESS);
		co_return;
	}

	CurrentCursorState = CursorState::Point;
	CursorOn = cursor_on::unknown;

	//this needs to be called so that FrameTicks is set, and we don't wait forever on WaitEventsOneFrame
	SetVideoSync();

	SetCallbacks(&GuichanCallbacks);

	int exit_code = EXIT_SUCCESS;

	try {
		while (GameResult != GameExit) {
			if (GameRunning || CEditor::get()->is_running()) {
				co_await QCoro::sleepFor(std::chrono::milliseconds(1000));
			} else {
				UpdateDisplay();
				CheckMusicFinished();

				co_await WaitEventsOneFrame();
			}
		}
	} catch (...) {
		exception::report(std::current_exception());
		exit_code = EXIT_FAILURE;
	}

	co_await Exit(exit_code);
}

void load_database(const bool initial_definition)
{
	database::get()->load(initial_definition).then([]() {
		//do nothing
	}, [](const std::exception &exception) {
		exception::report(std::current_exception());
		log::log_error("Error loading database.");
		std::terminate();
	});
}

void load_defines()
{
	try {
		//load the preferences before the defines, as the latter depend on the preferences
		preferences::get()->load();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error loading preferences."));
	}

	try {
		database::get()->load_defines();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error loading defines."));
	}
}

void initialize_database()
{
	try {
		database::get()->initialize();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error initializing database."));
	}
}

void save_preferences()
{
	preferences::get()->save();
}

int get_difficulty_index()
{
	return preferences::get()->get_difficulty_index();
}

void set_difficulty_index(const int difficulty_index)
{
	preferences::get()->set_difficulty_index(difficulty_index);
}
