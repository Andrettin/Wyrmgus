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

#include "character.h"
#include "civilization.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "game.h"
#include "item/unique_item.h"
#include "map/map_layer.h"
#include "map/world.h"
#include "religion/deity.h"
#include "parameters.h"
#include "results.h"
#include "script.h"
#include "sound/music_player.h"
#include "sound/music_type.h"
#include "sound/sound.h"
#include "ui/interface.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/queue_util.h"

namespace wyrmgus {

engine_interface::engine_interface()
{
	connect(static_cast<QGuiApplication *>(QApplication::instance()), &QGuiApplication::applicationStateChanged, this, [this](const Qt::ApplicationState state) {
		//reset key modifiers if the application has become active or inactive (e.g. due to an Alt-Tab)
		if (state == Qt::ApplicationActive || state == Qt::ApplicationInactive) {
			this->post([]() {
				KeyModifiers = 0;
			});
		}
	});
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
	try {
		const sound *sound = sound::get(sound_identifier.toStdString());

		this->post([sound]() {
			PlayGameSound(sound, MaxSampleVolume);
		});
	} catch (const std::exception &exception) {
		exception::report(exception);
	}
}

void engine_interface::play_music(const QString &type_str)
{
	try {
		const music_type type = string_to_music_type(type_str.toStdString());

		this->post([type]() {
			music_player::get()->play_music_type(type);
		});
	} catch (const std::exception &exception) {
		exception::report(exception);
	}
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

bool engine_interface::eventFilter(QObject *source, QEvent *event)
{
	Q_UNUSED(source)

	switch (event->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseMove: {
			const QMouseEvent *mouse_event = static_cast<QMouseEvent *>(event);
			this->store_input_event(std::make_unique<QMouseEvent>(*mouse_event));
			return true;
		}
		case QEvent::HoverEnter:
		case QEvent::HoverLeave:
		case QEvent::HoverMove: {
			const QHoverEvent *hover_event = static_cast<QHoverEvent *>(event);
			this->store_input_event(std::make_unique<QHoverEvent>(*hover_event));
			return true;
		}
		case QEvent::KeyPress:
		case QEvent::KeyRelease: {
			const QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
			this->store_input_event(std::make_unique<QKeyEvent>(*key_event));
			return false; //return false so that QML may also process the key event
		}
		default:
			return false;
	}
}

QVariantList engine_interface::get_building_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_building_encyclopedia_entries());
}

QVariantList engine_interface::get_character_encyclopedia_entries() const
{
	return container::to_qvariant_list(character::get_encyclopedia_entries());
}

QVariantList engine_interface::get_civilization_encyclopedia_entries() const
{
	return container::to_qvariant_list(civilization::get_encyclopedia_entries());
}

QVariantList engine_interface::get_deity_encyclopedia_entries() const
{
	return container::to_qvariant_list(deity::get_encyclopedia_entries());
}

QVariantList engine_interface::get_item_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_item_encyclopedia_entries());
}

QVariantList engine_interface::get_magic_prefix_encyclopedia_entries() const
{
	return container::to_qvariant_list(CUpgrade::get_magic_prefix_encyclopedia_entries());
}

QVariantList engine_interface::get_magic_suffix_encyclopedia_entries() const
{
	return container::to_qvariant_list(CUpgrade::get_magic_suffix_encyclopedia_entries());
}

QVariantList engine_interface::get_technology_encyclopedia_entries() const
{
	return container::to_qvariant_list(CUpgrade::get_technology_encyclopedia_entries());
}

QVariantList engine_interface::get_unique_item_encyclopedia_entries() const
{
	return container::to_qvariant_list(unique_item::get_encyclopedia_entries());
}

QVariantList engine_interface::get_unit_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_unit_encyclopedia_entries());
}

QVariantList engine_interface::get_world_encyclopedia_entries() const
{
	return container::to_qvariant_list(world::get_encyclopedia_entries());
}

}
