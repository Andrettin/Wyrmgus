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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "quest/campaign.h"

#include "database/preferences.h"
#include "game/game.h"
#include "include/config.h"
#include "map/map_template.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "quest/quest.h"
#include "species/species.h"
#include "time/calendar.h"
#include "time/timeline.h"
#include "util/container_util.h"
#include "util/string_conversion_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void campaign::initialize_all()
{
	data_type::initialize_all();

	campaign::sort_instances([](const campaign *a, const campaign *b) {
		if (a->get_timeline() != b->get_timeline()) {
			if (a->get_timeline() == nullptr || b->get_timeline() == nullptr) {
				return a->get_timeline() == nullptr;
			}

			return a->get_timeline()->get_point_of_divergence() < b->get_timeline()->get_point_of_divergence();
		} else if (a->get_start_date() != b->get_start_date()) {
			return a->get_start_date() < b->get_start_date();
		} else {
			return a->get_identifier() < b->get_identifier();
		}
	});
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void campaign::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "description") {
			this->set_description(value);
		} else if (key == "faction") {
			wyrmgus::faction *faction = faction::get(value);
			this->faction = faction;
		} else if (key == "hidden") {
			this->hidden = string::to_bool(value);
		} else if (key == "start_date") {
			this->start_date = string::to_date(value);
		} else if (key == "required_quest") {
			wyrmgus::quest *quest = quest::get(value);
			this->required_quests.push_back(quest);
		} else {
			fprintf(stderr, "Invalid campaign property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "map_template") {
			map_template *map_template = nullptr;
			Vec2i start_pos(0, 0);
			Vec2i map_size(0, 0);
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "map_template") {
					map_template = map_template::get(value);
					if (map_size.x == 0) {
						map_size.x = map_template->get_width();
					}
					if (map_size.y == 0) {
						map_size.y = map_template->get_height();
					}
				} else if (key == "start_x") {
					start_pos.x = std::stoi(value);
				} else if (key == "start_y") {
					start_pos.y = std::stoi(value);
				} else if (key == "width") {
					map_size.x = std::stoi(value);
				} else if (key == "height") {
					map_size.y = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid image property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!map_template) {
				fprintf(stderr, "Campaign map template has no map template.\n");
				continue;
			}
			
			this->map_templates.push_back(map_template);
			this->MapTemplateStartPos.push_back(start_pos);
			this->MapSizes.push_back(map_size);
		} else {
			fprintf(stderr, "Invalid campaign property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void campaign::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "required_quests") {
		for (const std::string &value : values) {
			this->required_quests.push_back(quest::get(value));
		}
	} else if (tag == "required_map_templates") {
		for (const std::string &value : values) {
			this->required_map_templates.insert(map_template::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void campaign::initialize()
{
	if (this->start_date_calendar != nullptr) {
		if (!this->start_date_calendar->is_initialized()) {
			this->start_date_calendar->initialize();
		}

		this->start_date = this->start_date.addYears(this->start_date_calendar->get_year_offset());
		this->start_date_calendar = nullptr;
	}

	for (const wyrmgus::quest *quest : this->get_required_quests()) {
		connect(quest, &quest::completed_changed, this, &campaign::available_changed);
	}

	named_data_entry::initialize();
}

const species *campaign::get_species() const
{
	if (this->get_faction() != nullptr && this->get_faction()->get_civilization() != nullptr) {
		return this->get_faction()->get_civilization()->get_species();
	}

	return nullptr;
}

bool campaign::is_available() const
{
	if (this->is_hidden()) {
		return false;
	}

	for (const wyrmgus::quest *quest : this->get_required_quests()) {
		if (!quest->is_completed()) {
			return false;
		}
	}

	return true;
}

void campaign::remove_map_template(map_template *map_template)
{
	vector::remove(this->map_templates, map_template);
}

}

std::string GetCurrentCampaign()
{
	const campaign *current_campaign = game::get()->get_current_campaign();
	
	if (!current_campaign) {
		return "";
	} else {
		return current_campaign->GetIdent();
	}
}
