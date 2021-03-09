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

#include <QMap>

#include <QCommandLineOption>

namespace wyrmgus {

void parameters::process()
{
	QCommandLineParser cmd_parser;

	// TODO Long version
	QMap<QString, QCommandLineOption> options {
		{ "start_editor", { "e", "Start editor instead of game." } },
		{ "depth", { "D", "Video mode depth (pixel per point).", "depth" } },
		{ "conf_file", { "c", "Configuration start file (default is 'stratagus.lua').", "config file" } },
		{
			"editor_conf_file",
			{ "E", "Editor configuration start file (default is 'editor.lua').", "editor config file" }
		},
		{ "data_path", { "d", "Specify a custom data path.", "data path" } },
		{ "test", { "t", "Check startup and exit (data files must respect this flag)." } },
		{ "fullscreen", { "F", "Full screen video mode." } },
		{ "game_options", { "G", "Game options passed to game scripts", "game options." } },
		{ "net_address", { "I", "Network address to use", "address." } },
		{ "disable_log", { "l", "Disable command log." } },
		{ "player_name", { "N", "Name of the player.", "name" } },
	};

	cmd_parser.addOptions(options.values());

  /**
#if defined(USE_OPENGL) || defined(USE_GLES)
		-o\t\tDo not use OpenGL or OpenGL ES 1.1\n"
		-O\t\tUse OpenGL or OpenGL ES 1.1\n"
#endif
		-p\t\tEnables debug messages printing in console\n"
		-P port\t\tNetwork port to use\n"
		-s sleep\tNumber of frames for the AI to sleep before it starts\n"
		-S speed\tSync speed (100 = 30 frames/s)\n"
		-u userpath\tPath where stratagus saves preferences, log and savegame\n"
		-v mode\t\tVideo mode resolution in format <xres>x<yres>\n"
		-W\t\tWindowed video mode\n"
#if defined(USE_OPENGL) || defined(USE_GLES)
		-x idx\t\tControls fullscreen scaling if your graphics card supports shaders.\n"\
		  \t\tPass 1 for nearest-neigubour, 2 for EPX/AdvMame, 3 for HQx, 4 for SAL, 5 for SuperEagle\n"\
		  \t\tYou can also use Ctrl+Alt+/ to cycle between these scaling algorithms at runtime.\n"
		  \t\tPass -1 to force old-school nearest neighbour scaling without shaders\n"\
		-Z\t\tUse OpenGL to scale the screen to the viewport (retro-style). Implies -O.\n"
#endif
		"map is relative to the root data path, use ./map for relative to cwd\n",
*/

	cmd_parser.setApplicationDescription("The free real time strategy game engine.");
	cmd_parser.addHelpOption();
	cmd_parser.addVersionOption();

	cmd_parser.process(*QApplication::instance());

	auto & data_path_option { options.find("data_path").value() };
	if (cmd_parser.isSet(data_path_option)) {
		database::get()->set_root_path(cmd_parser.value(data_path_option).toStdString());
	}

	if (cmd_parser.isSet(options.find("test").value())) {
		this->test_run = true;
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
			case 'D':
				Video.Depth = atoi(optarg);
				continue;
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
