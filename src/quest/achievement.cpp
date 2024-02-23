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
//      (c) Copyright 2017-2022 by Andrettin
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
#include "engine_interface.h"
#include "game/difficulty.h"
#include "player/player.h"
#include "player/player_color.h"
#include "quest/quest.h"
#include "script.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/string_conversion_util.h"

#pragma warning(push, 0)
#include <QSettings>
#pragma warning(pop)

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
		} catch (...) {
			exception::report(std::current_exception());
			log::log_error("Failed to load data for achievement \"" + key.toStdString() + "\".");
		}
	}
}

void achievement::save_achievements()
{
	//save achievements
	const std::filesystem::path achievements_filepath = achievement::get_achievements_filepath();

	gsml_data data;

	for (const achievement *achievement : achievement::get_all()) {
		if (achievement->is_obtained()) {
			data.add_property(achievement->get_identifier(), string::from_bool(true));
		}
	}

	try {
		data.print_to_file(achievements_filepath);
	} catch (...) {
		exception::report(std::current_exception());
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

void achievement::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "description") {
		this->description = value;
	} else {
		data_entry::process_gsml_property(property);
	}
}

void achievement::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "required_quests") {
		for (const std::string &value : values) {
			this->required_quests.push_back(quest::get(value));
		}
	} else if (tag == "reward_abilities") {
		for (const std::string &value : values) {
			this->reward_abilities.push_back(CUpgrade::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

std::string achievement::get_rewards_string() const
{
	std::string str;

	bool first = true;

	for (const CUpgrade *ability : this->reward_abilities) {
		if (first) {
			first = false;
		} else {
			str += ", ";
		}

		str += ability->get_name();
	}

	return str;
}

bool achievement::can_obtain() const
{
	if (this->is_obtained() || this->unobtainable) {
		return false;
	}

	for (const quest *required_quest : this->required_quests) {
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

		for (const wyrmgus::character *hero : character::get_all_with_custom()) {
			if (!hero->is_playable()) {
				continue;
			}

			if (this->character_type && hero->get_unit_type() != this->character_type) {
				continue;
			}

			if (this->character_level > 0) {
				if (hero->get_level() < this->character_level || hero->get_level() == hero->get_base_level()) {
					continue;
				}
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

	this->apply_rewards();

	if (save) {
		achievement::save_achievements();
	}

	if (display) {
		emit engine_interface::get()->achievementUnlockedDialogOpened(this);
	}
}

int achievement::get_progress() const
{
	if (this->unobtainable) {
		return 0;
	}

	int progress = 0;

	for (const quest *required_quest : this->required_quests) {
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

		for (const wyrmgus::character *hero : character::get_all_with_custom()) {
			if (!hero->is_playable()) {
				continue;
			}

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
	progress_max += this->required_quests.size();
	progress_max += this->character_level;

	return progress_max;
}

void achievement::apply_rewards() const
{
	if (!this->reward_abilities.empty()) {
		assert_throw(this->character != nullptr);

		for (const CUpgrade *upgrade : this->reward_abilities) {
			this->character->add_bonus_ability(upgrade);
		}
	}
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
