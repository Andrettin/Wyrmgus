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

#include "ai.h"

#include "stratagus.h"

#include "parameters.h"

#include "database/database.h"

#include <QCommandLineParser>

/// Filename of the map given on the command line
std::string CliMapName;

namespace wyrmgus {

void set_retroscale() {
	Video.ViewportHeight = Video.Height;
	Video.ViewportWidth = Video.Width;
	Video.Height = 0;
	Video.Width = 0;
}

void parameters::process()
{
	QCommandLineParser cmd_parser;

	QList<QCommandLineOption> options {
		{ { "e", "editor" }, "Start editor instead of game." },
		{ { "D", "video-depth" }, "Video mode depth (pixel per point).", "depth" },
		{ { "c", "config" }, "Configuration start file (default is 'stratagus.lua').", "config file" },
		{
			{ "E", "editor-config" },
			"Editor configuration start file (default is 'editor.lua').",
			"editor config file"
		},
		{ { "d", "data-path" }, "Specify a custom data path.", "data path" },
		{ { "t", "test-run" }, "Check startup and exit (data files must respect this flag)." },
		{ { "F", "full-screen" }, "Full screen video mode." },
		{ { "G", "game-options" }, "Game options passed to game scripts", "game options" },
		{ { "I", "ip-address" }, "Network address to use", "address" },
		{ { "l", "no-command-log" }, "Disable command log." },
		{ { "N", "player-name" }, "Name of the player.", "name" },
#if defined(USE_OPENGL) || defined(USE_GLES)
		{ { "o", "no-opengl" }, "Do not use OpenGL or OpenGL ES 1.1." }, // FIXME Not implemented
		{ { "O", "opengl" }, "Use OpenGL or OpenGL ES 1.1." }, // FIXME Not implemented
#endif
		{ { "p", "print-debug" }, "Enables debug messages printing in console." },
		{ { "P", "port" }, "Network port to use.", "port" },
		{ { "s", "sleep" }, "Number of frames for the AI to sleep before it starts.", "frames" },
		{ { "S", "sync-rate" }, "Sync speed (100 = 30 frames/s).", "speed" },
		{ { "u", "user-path" }, "Path where wyrmgus saves preferences, log and savegame", "path" },
		{ { "m", "video-mode" }, "Video mode resolution in format <xres>x<yres>.", "mode" },
		{ { "W", "windowed" }, "Windowed video mode." },
#if defined(USE_OPENGL) || defined(USE_GLES)
		{
			{ "x", "scaling-mode" },
			"Controls fullscreen scaling if your graphics card supports shaders. "
				"Pass 1 for nearest-neigubour, 2 for EPX/AdvMame, 3 for HQx, 4 for SAL, 5 for SuperEagle. "
				"You can also use Ctrl+Alt+/ to cycle between these scaling algorithms at runtime. "
				"Pass -1 to force old-school nearest neighbour scaling without shaders.",
			"idx"
		},
		{ { "Z", "retro-scale" }, "Use OpenGL to scale the screen to the viewport (retro-style). Implies -O." },
#endif
	};
	cmd_parser.addOptions(options);

	cmd_parser.addPositionalArgument("mapfile", "A custom map file to start on.");

	cmd_parser.setApplicationDescription("The free real time strategy game engine.");
	cmd_parser.addHelpOption();
	cmd_parser.addVersionOption();

	cmd_parser.process(*QApplication::instance());

	auto app_name { QApplication::applicationName().toStdString() };

	QString option { "c" };
	if (cmd_parser.isSet(option)) {
		// FIXME Use appropriate type for properly suffix processing
		QString filename { cmd_parser.value(option) };
		if (!filename.endsWith(".lua")) filename += ".lua";
		this->luaStartFilename = filename.toStdString();
	}

	option = "d" ;
	if (cmd_parser.isSet(option)) {
		database::get()->set_root_path(cmd_parser.value(option).toStdString());
	}

	option = "D";
	if (cmd_parser.isSet(option)) {
		Video.Depth = cmd_parser.value(option).toInt();
	}

	if (cmd_parser.isSet("e")) {
		Editor.Running = EditorCommandLine;
	}

	option = "E";
	if (cmd_parser.isSet(option)) {
		this->luaEditorStartFilename = cmd_parser.value(option).toStdString();
	}

	if (cmd_parser.isSet("F")) {
		if (cmd_parser.isSet("W")) {
			throw std::runtime_error(
					app_name +
					": \"windowed\" and \"full-screen\" is mutual exclusive options.");
		}
		VideoForceFullScreen = 1;
		Video.FullScreen = 1;
	}

	option = "G";
	if (cmd_parser.isSet(option)) {
		this->luaScriptArguments = cmd_parser.value(option).toStdString();
	}

	option = "I";
	if (cmd_parser.isSet(option)) {
		CNetworkParameter::Instance.localHost = cmd_parser.value(option).toStdString();
	}

	if (cmd_parser.isSet("l")) {
		CommandLogDisabled = true;
	}

	option = "N";
	if (cmd_parser.isSet(option)) {
		this->LocalPlayerName = cmd_parser.value(option).toStdString();
	}

	option = "P";
	if (cmd_parser.isSet(option)) {
		CNetworkParameter::Instance.localPort = cmd_parser.value(option).toUInt();
	}

	if (cmd_parser.isSet("p")) {
		EnableDebugPrint = true;
	}

	option = "s";
	if (cmd_parser.isSet(option)) {
		AiSleepCycles = cmd_parser.value(option).toInt();
	}

	option = "S";
	if (cmd_parser.isSet(option)) {
		VideoSyncSpeed = cmd_parser.value(option).toInt();
	}

	if (cmd_parser.isSet("t")) {
		this->test_run = true;
	}

	option = "u";
	if (cmd_parser.isSet(option)) {
		this->SetUserDirectory(cmd_parser.value(option).toStdString());
	}
	else {
		this->SetDefaultUserDirectory();
	}

	option = "m";
	if (cmd_parser.isSet(option)) {
		auto mode { cmd_parser.value(option) };
		const QRegularExpression vmode_regex { "^(\\d+)x(\\d+)$" };

		const auto vmode_match { vmode_regex.match(mode) };
		if (!vmode_match.hasMatch()) {
			throw std::runtime_error(
					app_name +
					": incorrect format of video mode resolution -- '" +
					mode.toStdString() + "'");
		}

		Video.Width = vmode_match.captured(1).toInt();
		Video.Height = vmode_match.captured(2).toInt();

#if defined(USE_OPENGL) || defined(USE_GLES)
		if (ZoomNoResize) {
			set_retroscale();
		}
#endif
	}

	if (cmd_parser.isSet("W")) {
		VideoForceFullScreen = 1;
		Video.FullScreen = 0;
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (cmd_parser.isSet("Z")) {
		ZoomNoResize = 1;
		set_retroscale();
	}
#endif

	const auto pos_args { cmd_parser.positionalArguments() };

	if (pos_args.length() > 1) {
		throw std::runtime_error("Too many map files (at most one expected).");
	}
	else if (pos_args.length() == 1) {
		CliMapName = pos_args[0].toStdString();
		std::replace(CliMapName.begin(), CliMapName.end(), '\\', '/');
	}
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
