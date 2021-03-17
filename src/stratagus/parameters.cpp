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
//      (c) Copyright 1998-2021 by Joris Dauphin and Andrettin
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

#include "stratagus.h"

#include "parameters.h"

#include "database/database.h"

#include <QCommandLineParser>

namespace wyrmgus {

void parameters::process()
{
	QCommandLineParser cmd_parser;

	// TODO Long version
	QList<QCommandLineOption> options {
		{ "e", "Start editor instead of game." },
		{ "D", "Video mode depth (pixel per point).", "depth" },
		{ "c", "Configuration start file (default is 'stratagus.lua').", "config file" },
		{ "E", "Editor configuration start file (default is 'editor.lua').", "editor config file" },
		{ "d", "Specify a custom data path.", "data path" },
		{ "t", "Check startup and exit (data files must respect this flag)." },
		{ "F", "Full screen video mode." },
		{ "G", "Game options passed to game scripts", "game options." },
		{ "I", "Network address to use", "address." },
		{ "l", "Disable command log." },
		{ "N", "Name of the player.", "name" },
#if defined(USE_OPENGL) || defined(USE_GLES)
		{ "o", "Do not use OpenGL or OpenGL ES 1.1." },
		{ "O", "Use OpenGL or OpenGL ES 1.1." },
#endif
		{ "p", "Enables debug messages printing in console." },
		{ "P", "Network port to use.", "port" },
		{ "s", "Number of frames for the AI to sleep before it starts.", "frames" },
		{ "S", "Sync speed (100 = 30 frames/s).", "speed" },
		{ "u", "Path where wyrmgus saves preferences, log and savegame", "path" },
		// FIXME { "v", "Video mode resolution in format <xres>x<yres>.", "mode" },
		{ "W", "Windowed video mode." },
#if defined(USE_OPENGL) || defined(USE_GLES)
		{
			"x",
			"Controls fullscreen scaling if your graphics card supports shaders. "
				"Pass 1 for nearest-neigubour, 2 for EPX/AdvMame, 3 for HQx, 4 for SAL, 5 for SuperEagle. "
				"You can also use Ctrl+Alt+/ to cycle between these scaling algorithms at runtime. "
				"Pass -1 to force old-school nearest neighbour scaling without shaders.",
			"idx"
		},
		{ "Z", "Use OpenGL to scale the screen to the viewport (retro-style). Implies -O." },
#endif
	};
	cmd_parser.addOptions(options);

	cmd_parser.setApplicationDescription("The free real time strategy game engine.");
	cmd_parser.addHelpOption();
	cmd_parser.addVersionOption();

	cmd_parser.process(*QApplication::instance());

	if (cmd_parser.isSet("d")) {
		database::get()->set_root_path(cmd_parser.value("d").toStdString());
	}

	if (cmd_parser.isSet("t")) {
		this->test_run = true;
	}

	// TODO
	if (cmd_parser.isSet("D")) {
		qDebug() << cmd_parser.value("D").toInt();
	}

	if (cmd_parser.isSet("c")) {
		// FIXME Use appropriate type for properly suffix processing
		QString filename { cmd_parser.value("c") };
		if (!filename.endsWith(".lua")) filename += ".lua";
		this->luaStartFilename = filename.toStdString();
	}

	/**
			case 'c':
				parameters->luaStartFilename = optarg;
				if (strlen(optarg) > 4 &&
					!(strstr(optarg, ".lua") == optarg + strlen(optarg) - 4)) {
						parameters->luaStartFilename += ".lua";
				}
				continue;
			case 'd': {
				continue;
			}
			case 'e':
				Editor.Running = EditorCommandLine;
				continue;
			case 'E':
				parameters->luaEditorStartFilename = optarg;
				continue;
			case 'F':
				VideoForceFullScreen = 1;
				Video.FullScreen = 1;
				continue;
			case 'G':
				parameters->luaScriptArguments = optarg;
				continue;
			case 'I':
				CNetworkParameter::Instance.localHost = optarg;
				continue;
			case 'l':
				CommandLogDisabled = true;
				continue;
			case 'N':
				parameters->LocalPlayerName = optarg;
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
			case 't':
				continue;
			case 'u':
				parameters->SetUserDirectory(optarg);
				continue;
			case 'v': {
				char *sep = strchr(optarg, 'x');
				if (!sep || !*(sep + 1)) {
					throw std::runtime_error(std::string(argv[0]) + ": incorrect format of video mode resolution -- '" + optarg + "'");
				}
				Video.Height = atoi(sep + 1);
				*sep = 0;
				Video.Width = atoi(optarg);
				if (!Video.Height || !Video.Width) {
					throw std::runtime_error(std::string(argv[0]) + ": incorrect format of video mode resolution -- '" + optarg + "x + " + (sep + 1) + "'");
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
				Usage();
				Exit(EXIT_SUCCESS);
				break;
			default:
				throw std::runtime_error("Invalid command line arguments.");
		}
		break;
	}

	if (argc - optind > 1) {
		throw std::runtime_error("too many map files. if you meant to pass game arguments, these go after '--'");
	}

	if (argc - optind) {
		size_t index;
		CliMapName = argv[optind];
		while ((index = CliMapName.find('\\')) != std::string::npos) {
			CliMapName[index] = '/';
		}
	}
*/

	this->SetDefaultUserDirectory();
}

void parameters::SetDefaultUserDirectory()
{
#ifdef USE_GAME_DIR
	userDirectory = database::get()->get_root_path().string();
#elif USE_WIN32
	userDirectory = getenv("APPDATA");
#elif __MORPHOS__
	userDirectory = "home";
#else
	userDirectory = getenv("HOME");
#endif

	if (!userDirectory.empty()) {
		userDirectory += "/";
	}

#ifdef USE_GAME_DIR
#elif USE_WIN32
	userDirectory += "Stratagus";
#elif defined(USE_MAC)
	userDirectory += "Library/Stratagus";
#else
	userDirectory += ".stratagus";
#endif
}

static std::string GetLocalPlayerNameFromEnv()
{
	const char *userName = nullptr;

#ifdef USE_WIN32
	userName = getenv("USERNAME");
#else
	userName = getenv("USER");
#endif

	if (userName && userName[0]) {
		return userName;
	} else {
		return "Anonymous";
	}
}

void parameters::SetLocalPlayerNameFromEnv()
{
	LocalPlayerName = GetLocalPlayerNameFromEnv();
}

}
