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
//      (c) Copyright 2021 by Andrettin
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

#include "engine_interface.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "game.h"
#include "map/map_layer.h"
#include "parameters.h"
#include "results.h"
#include "script.h"
#include "sound/sound.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "util/queue_util.h"

namespace wyrmgus {

engine_interface::engine_interface()
{
}

engine_interface::~engine_interface()
{
}

parameters *engine_interface::get_parameters() const
{
	return parameters::get();
}

defines *engine_interface::get_defines() const
{
	return defines::get();
}

preferences *engine_interface::get_preferences() const
{
	return preferences::get();
}

game *engine_interface::get_game() const
{
	return game::get();
}

void engine_interface::run_event_loop()
{
	//run the commands posted from the Qt thread

	while (true) {
		std::function<void()> command;

		{
			std::lock_guard lock(this->command_mutex);

			if (this->posted_commands.empty()) {
				break;
			}

			command = queue::take(this->posted_commands);
		}

		command();
	}
}

void engine_interface::post(const std::function<void()> &function)
{
	std::lock_guard lock(this->command_mutex);
	this->posted_commands.push(function);
}

void engine_interface::call_lua_command(const QString &command)
{
	this->post([command]() {
		CclCommand(command.toStdString());
	});
}

void engine_interface::play_sound(const QString &sound_identifier)
{
	const sound *sound = sound::get(sound_identifier.toStdString());

	this->post([sound]() {
		PlayGameSound(sound, MaxSampleVolume);
	});
}

void engine_interface::exit()
{
	this->post([]() {
		if (Editor.Running) {
			Editor.Running = EditorNotRunning;
		} else {
			StopGame(GameExit);
		}
	});
}

}
