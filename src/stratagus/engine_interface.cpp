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
#include "faction.h"
#include "game.h"
#include "game/difficulty.h"
#include "game_concept.h"
#include "item/unique_item.h"
#include "literary_text.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/world.h"
#include "quest/achievement.h"
#include "quest/campaign.h"
#include "quest/quest.h"
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
#include "util/qvariant_util.h"

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

QString engine_interface::get_save_path() const
{
	std::filesystem::path save_path = GetSaveDir();
	save_path.make_preferred();
	return QString::fromStdString(save_path.string());
}

void engine_interface::call_lua_command(const QString &command)
{
	this->post([command]() {
		try {
			CclCommand(command.toStdString());
		} catch (const std::exception &exception) {
			exception::report(exception);
		}
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

			if (key_event->key() == Qt::Key_Tab || key_event->key() == Qt::Key_Backtab) {
				return true; //consume tab events to prevent tab focus switching
			}

			return false; //return false so that QML may also process the key event
		}
		default:
			return false;
	}
}

QVariantList engine_interface::get_visible_campaigns() const
{
	return container::to_qvariant_list(campaign::get_all_visible());
}

QVariantList engine_interface::get_playable_civilizations() const
{
	std::vector<civilization *> playable_civilizations;

	for (civilization *civilization : civilization::get_all()) {
		if (!civilization->is_playable()) {
			continue;
		}

		playable_civilizations.push_back(civilization);
	}

	std::sort(playable_civilizations.begin(), playable_civilizations.end(), [](const civilization *lhs, const civilization *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});

	return container::to_qvariant_list(playable_civilizations);
}

void engine_interface::load_map_infos()
{
	this->clear_map_infos();

	this->sync([this]() { //must be synchronized as it uses Lua and alters the map singleton
		try {
			const std::vector<std::filesystem::path> map_paths = database::get()->get_maps_paths();

			for (const std::filesystem::path &map_path : map_paths) {
				if (!std::filesystem::exists(map_path)) {
					continue;
				}

				std::filesystem::recursive_directory_iterator dir_iterator(map_path);

				for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
					if (!dir_entry.is_regular_file() || dir_entry.path().extension() != ".smp") {
						continue;
					}

					CMap::get()->get_info()->Clear();

					LuaLoadFile(dir_entry.path().string());
					CMap::get()->get_info()->set_presentation_filepath(dir_entry.path());

					this->map_infos.push_back(CMap::get()->get_info()->duplicate());
				}
			}

			CMap::get()->get_info()->Clear();
		} catch (const std::exception &exception) {
			exception::report(exception);
		}
	});
}

void engine_interface::clear_map_infos()
{
	this->map_infos.clear();
}

QStringList engine_interface::get_map_worlds() const
{
	std::set<std::string> map_worlds;

	for (const auto &map_info : this->map_infos) {
		map_worlds.insert(map_info->MapWorld);
	}

	QStringList map_world_qstring_list = container::to_qstring_list(map_worlds);

	std::sort(map_world_qstring_list.begin(), map_world_qstring_list.end(), [](const QString &lhs, const QString &rhs) {
		if (lhs == "Custom" || rhs == "Custom") {
			return lhs != "Custom";
		}

		if (lhs == "Random" || rhs == "Random") {
			return lhs != "Random";
		}

		return lhs < rhs;
	});

	return map_world_qstring_list;
}

QVariantList engine_interface::get_map_infos(const QString &world) const
{
	std::vector<map_info *> map_infos;

	const std::string world_str = world.toStdString();

	for (const auto &map_info : this->map_infos) {
		if (map_info->MapWorld != world_str) {
			continue;
		}

		map_infos.push_back(map_info.get());
	}

	return container::to_qvariant_list(map_infos);
}

QVariantList engine_interface::get_difficulties() const
{
	QVariantList difficulties;

	for (int i = static_cast<int>(difficulty::none) + 1; i < static_cast<int>(difficulty::count); ++i) {
		difficulties.push_back(i);
	}

	return difficulties;
}

QString engine_interface::get_difficulty_name(const int difficulty_index) const
{
	const difficulty difficulty = static_cast<wyrmgus::difficulty>(difficulty_index);
	return QString::fromStdString(wyrmgus::get_difficulty_name(difficulty));
}

QVariantList engine_interface::get_achievements() const
{
	std::vector<achievement *> achievements;

	for (achievement *achievement : achievement::get_all()) {
		if (achievement->is_hidden()) {
			continue;
		}

		achievements.push_back(achievement);
	}

	return container::to_qvariant_list(achievements);
}

QVariantList engine_interface::get_legacy_quests() const
{
	std::vector<quest *> quests;

	for (quest *quest : quest::get_all()) {
		if (quest->is_hidden()) {
			continue;
		}

		if (quest->World.empty()) {
			continue;
		}

		if (quest->Map.empty()) {
			continue;
		}

		quests.push_back(quest);
	}

	return container::to_qvariant_list(quests);
}

QVariantList engine_interface::get_custom_heroes() const
{
	return container::to_qvariant_list(character::get_custom_heroes());
}

void engine_interface::create_custom_hero(const QString &name, const QString &surname, const QVariant &civilization, const QVariant &unit_type, const QVariant &trait, const QString &variation_identifier)
{
	character::create_custom_hero(name.toStdString(), surname.toStdString(), qvariant::to_object<wyrmgus::civilization>(civilization), qvariant::to_object<wyrmgus::unit_type>(unit_type), qvariant::to_object<CUpgrade>(trait), variation_identifier.toStdString());
}

void engine_interface::delete_custom_hero(const QVariant &hero)
{
	character::remove_custom_hero(qvariant::to_object<character>(hero));
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

QVariantList engine_interface::get_faction_encyclopedia_entries() const
{
	return container::to_qvariant_list(faction::get_encyclopedia_entries());
}

QVariantList engine_interface::get_game_concept_encyclopedia_entries() const
{
	return container::to_qvariant_list(game_concept::get_encyclopedia_entries());
}

QVariantList engine_interface::get_item_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_item_encyclopedia_entries());
}

QVariantList engine_interface::get_literary_text_encyclopedia_entries() const
{
	return container::to_qvariant_list(literary_text::get_encyclopedia_entries());
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

QObject *engine_interface::get_link_target(const QString &link_str) const
{
	try {
		const QStringList link_str_list = link_str.split(':');

		if (link_str_list.size() != 2) {
			throw std::runtime_error("Invalid link string: \"" + link_str.toStdString() + "\".");
		}

		const std::string link_type = link_str_list.at(0).toStdString();
		const std::string link_target = link_str_list.at(1).toStdString();

		QObject *object = nullptr;

		if (link_type == "character") {
			object = character::get(link_target);
		} else if (link_type == "civilization") {
			object = civilization::get(link_target);
		} else if (link_type == "deity") {
			object = deity::get(link_target);
		} else if (link_type == "faction") {
			object = faction::get(link_target);
		} else if (link_type == "game_concept") {
			object = game_concept::get(link_target);
		} else if (link_type == "literary_text") {
			object = literary_text::get(link_target);
		} else if (link_type == "unit_type") {
			object = unit_type::get(link_target);
		} else if (link_type == "upgrade") {
			object = CUpgrade::get(link_target);
		} else if (link_type == "world") {
			object = world::get(link_target);
		} else {
			throw std::runtime_error("Invalid link type: \"" + link_type + "\".");
		}

		return object;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return nullptr;
	}
}

}
