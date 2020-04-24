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
//      (c) Copyright 2019-2020 by Andrettin
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
//

#include "stratagus.h"

#include "campaign.h"

#include "civilization.h"
#include "config.h"
#include "map/map_template.h"
#include "player.h"
#include "quest.h"
#include "util/string_util.h"

namespace stratagus {

campaign *campaign::CurrentCampaign = nullptr;

void campaign::initialize_all()
{
	campaign::sort_instances([](campaign *a, campaign *b) {
		if (a->GetSpecies() != b->GetSpecies()) {
			return a->GetSpecies() < b->GetSpecies();
		} else if (a->GetStartDate() != b->GetStartDate()) {
			return a->GetStartDate() < b->GetStartDate();
		} else {
			return a->get_identifier() < b->get_identifier();
		}
	});
}

void campaign::SetCurrentCampaign(campaign *campaign)
{
	if (campaign == campaign::CurrentCampaign) {
		return;
	}
	
	campaign::CurrentCampaign = campaign;
}

campaign *campaign::GetCurrentCampaign()
{
	return campaign::CurrentCampaign;
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
			this->Description = value;
		} else if (key == "faction") {
			value = FindAndReplaceString(value, "_", "-");
			CFaction *faction = PlayerRaces.GetFaction(value);
			if (faction) {
				this->Faction = faction;
			} else {
				fprintf(stderr, "Invalid faction: \"%s\".\n", value.c_str());
			}
		} else if (key == "hidden") {
			this->Hidden = string::to_bool(value);
		} else if (key == "sandbox") {
			this->Sandbox = string::to_bool(value);
		} else if (key == "start_date") {
			value = FindAndReplaceString(value, "_", "-");
			this->StartDate = CDate::FromString(value);
		} else if (key == "required_quest") {
			value = FindAndReplaceString(value, "_", "-");
			CQuest *quest = GetQuest(value);
			if (quest) {
				this->RequiredQuests.push_back(quest);
			} else {
				fprintf(stderr, "Invalid quest: \"%s\".\n", value.c_str());
			}
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

std::string campaign::GetSpecies() const
{
	if (this->Faction && this->Faction->civilization) {
		return PlayerRaces.Species[this->Faction->civilization->ID];
	}

	return std::string();
}

bool campaign::IsAvailable() const
{
	if (this->IsHidden()) {
		return false;
	}

	for (CQuest *quest : this->RequiredQuests) {
		if (!quest->IsCompleted()) {
			return false;
		}
	}

	return true;
}

}

/**
**	@brief	Set the current campaign
**
**	@param	campaign_ident	The campaign's string identifier
*/
void SetCurrentCampaign(const std::string &campaign_ident)
{
	if (campaign_ident.empty()) {
		stratagus::campaign::SetCurrentCampaign(nullptr);
		return;
	}
	
	stratagus::campaign *campaign = stratagus::campaign::get(campaign_ident);
	stratagus::campaign::SetCurrentCampaign(campaign);
}

/**
**	@brief	Get the current campaign
**
**	@return	The string identifier of the current campaign
*/
std::string GetCurrentCampaign()
{
	const stratagus::campaign *current_campaign = stratagus::campaign::GetCurrentCampaign();
	
	if (!current_campaign) {
		return "";
	} else {
		return current_campaign->GetIdent();
	}
}
