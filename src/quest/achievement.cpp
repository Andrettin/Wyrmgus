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
//      (c) Copyright 2017-2021 by Andrettin
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

#include "quest/achievement.h"

#include "character.h"
#include "game/difficulty.h"
#include "game/game.h" //for the GameName variable
#include "player/player.h"
#include "player/player_color.h"
#include "quest/quest.h"
#include "script.h"
#include "unit/unit_type.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/string_conversion_util.h"

#include <QSettings>

namespace wyrmgus {

std::filesystem::path achievement::get_achievements_filepath()
{
	std::filesystem::path filepath = database::get_user_data_path() / "achievements.txt";
	filepath.make_preferred();
	return filepath;
}

void achievement::load_achievements()
{
	const std::filesystem::path achievements_filepath = achievement::get_achievements_filepath();

	if (!std::filesystem::exists(achievements_filepath)) {
		return;
	}

	const QSettings data(path::to_qstring(achievements_filepath), QSettings::IniFormat);

	for (const QString &key : data.childKeys()) {
		try {
			achievement *achievement = achievement::get(key.toStdString());
			achievement->obtain(false, false);
		} catch (const std::exception &exception) {
			exception::report(exception);
			log::log_error("Failed to load data for achievement \"" + key.toStdString() + "\".");
		}
	}
}

void achievement::save_achievements()
{
	//save achievements
	const std::filesystem::path achievements_filepath = achievement::get_achievements_filepath();

	sml_data data;

	for (const achievement *achievement : achievement::get_all()) {
		if (achievement->is_obtained()) {
			data.add_property(achievement->get_identifier(), string::from_bool(true));
		}
	}

	try {
		data.print_to_file(achievements_filepath, true);
	} catch (const std::exception &exception) {
		exception::report(exception);
		log::log_error("Failed to save achievements file.");
	}
}

void achievement::check_achievements()
{
	bool changed = false;

	for (achievement *achievement : achievement::get_all()) {
		if (achievement->is_obtained()) {
			continue;
		}

		if (achievement->can_obtain()) {
			achievement->obtain(false, true);
			changed = true;
		}
	}

	if (changed) {
		achievement::save_achievements();
	}
}

achievement::achievement(const std::string &identifier) : named_data_entry(identifier), difficulty(difficulty::none)
{
}

void achievement::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "description") {
		this->description = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void achievement::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "required_quests") {
		for (const std::string &value : values) {
			this->RequiredQuests.push_back(quest::get(value));
		}
	} else {
		data_entry::process_sml_scope(scope);
	}
}

bool achievement::can_obtain() const
{
	if (this->is_obtained() || this->unobtainable) {
		return false;
	}

	for (const quest *required_quest : this->RequiredQuests) {
		if (!required_quest->is_completed() || (this->get_difficulty() != difficulty::none && required_quest->get_highest_completed_difficulty() < this->get_difficulty())) {
			return false;
		}
	}

	if (this->character != nullptr) {
		if (this->character_type != nullptr && this->character->get_unit_type() != this->character_type) {
			return false;
		}
		if (this->character_level > 0 && this->character->get_level() < this->character_level) {
			return false;
		}
	} else if (this->character_type != nullptr || this->character_level > 0) {
		bool found_hero = false;
		for (const wyrmgus::character *hero : character::get_custom_heroes()) {
			if (this->character_type && hero->get_unit_type() != this->character_type) {
				continue;
			}

			if (this->character_level > 0 && hero->get_level() < this->character_level) {
				continue;
			}

			found_hero = true;
			break;
		}

		if (!found_hero) {
			return false;
		}
	}

	return true;
}

void achievement::obtain(const bool save, const bool display)
{
	if (this->is_obtained()) {
		return;
	}

	this->obtained = true;

	if (save) {
		achievement::save_achievements();
	}

	if (display) {
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Achievement Unlocked!\", \"You have unlocked the " + this->get_name() + " achievement.\", nil, \"" + this->get_icon()->get_identifier() + "\", \"" + (this->get_player_color() ? this->get_player_color()->get_identifier() : "") + "\") end;");
	}
}

int achievement::get_progress() const
{
	if (this->unobtainable) {
		return 0;
	}

	int progress = 0;

	for (const quest *required_quest : this->RequiredQuests) {
		if (required_quest->is_completed() && (this->get_difficulty() == difficulty::none || required_quest->get_highest_completed_difficulty() >= this->get_difficulty())) {
			progress++;
		}
	}

	if (this->character != nullptr) {
		if (this->character_level > 0) {
			progress += std::min(this->character->get_level(), this->character_level);
		}
	} else if (this->character_level > 0) {
		int highest_level = 0;

		for (const wyrmgus::character *hero : character::get_custom_heroes()) {
			highest_level = std::max(highest_level, hero->get_level());
			if (highest_level >= this->character_level) {
				highest_level = this->character_level;
				break;
			}
		}

		progress += highest_level;
	}

	return progress;
}

int achievement::get_progress_max() const
{
	if (this->unobtainable) {
		return 0;
	}

	int progress_max = 0;
	progress_max += this->RequiredQuests.size();
	progress_max += this->character_level;

	return progress_max;
}

}

void SetAchievementObtained(const std::string &achievement_ident, const bool save, const bool display)
{
	achievement *achievement = achievement::try_get(achievement_ident);

	if (achievement == nullptr) {
		return;
	}

	achievement->obtain(save, display);
}

void save_achievements()
{
	achievement::save_achievements();
}

void check_achievements()
{
	achievement::check_achievements();
}
