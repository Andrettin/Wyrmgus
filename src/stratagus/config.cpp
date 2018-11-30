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
/**@name config.cpp - The config source file. */
//
//      (c) Copyright 2018 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "config.h"

#include "age.h"
#include "animation.h"
#include "calendar/calendar.h"
#include "calendar/timeline.h"
#include "character.h"
#include "game.h"
#include "icons.h"
#include "map/map_template.h"
#include "map/terrain_type.h"
#include "missile.h"
#include "religion/deity.h"
#include "season.h"
#include "season_schedule.h"
#include "sound.h"
#include "time_of_day.h"
#include "time_of_day_schedule.h"
#include "ui/button_action.h"
#include "unittype.h"
#include "util.h"

#include <fstream>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Parse a configuration data file
**
**	@param	define_only	Whether the elements in the configuration data should only be defined with their ident, without their properties being processed
*/
void CConfigData::ParseConfigData(const std::string &filepath, const bool define_only)
{
	std::vector<std::string> data;
	std::vector<CConfigData *> output;
	
	std::ifstream text_stream(filepath);
	std::string line;
	
	while(std::getline(text_stream, line)) {
		std::vector<std::string> line_data = SplitString(line, " \t", "#");
		
		for (size_t i = 0; i < line_data.size(); ++i) {
			data.push_back(line_data[i]);
		}
	}
	
	CConfigData *config_data = nullptr;
	std::string key;
	std::string value;
	for (size_t i = 0; i < data.size(); ++i) {
		std::string str = data[i];
		if (str[0] == '[' && str[1] != '/') { //opens a tag
			std::string tag_name = str;
			tag_name = FindAndReplaceString(tag_name, "[", "");
			tag_name = FindAndReplaceString(tag_name, "]", "");
			CConfigData *new_config_data = new CConfigData(tag_name);
			if (config_data) {
				new_config_data->Parent = config_data;
			}
			config_data = new_config_data;
		} else if (str[0] == '[' && str[1] == '/') { //closes a tag
			if (config_data && FindAndReplaceString(str, "/", "") == ("[" + config_data->Tag + "]")) { //closes current tag
				if (config_data->Parent == nullptr) {
					output.push_back(config_data);
					config_data = nullptr;
				} else {
					CConfigData *parent_config_data = config_data->Parent;
					parent_config_data->Children.push_back(config_data);
					config_data = parent_config_data;
				}
			}
		} else if (key.empty()) { //key
			if (str.find('=') != std::string::npos) {
				std::vector<std::string> key_value_strings = SplitString(str, "=");
				for (size_t j = 0; j < key_value_strings.size(); ++j) {
					if (key.empty()) {
						key = key_value_strings[j];
					} else {
						std::string value = key_value_strings[j];
						if (key == "ident") {
							config_data->Ident = value;
						} else {
							config_data->Properties.push_back(std::pair<std::string, std::string>(key, value));
						}
						key.clear();
					}
				}
			} else {
				key = str;
			}
		} else if (!key.empty() && str.length() == 1 && str[0] == '=') { //operator
			continue;
		} else if (!key.empty()) { //value
			std::string value = str;
			value = FindAndReplaceString(value, "=", "");
			if (key == "ident") {
				config_data->Ident = value;
			} else {
				config_data->Properties.push_back(std::pair<std::string, std::string>(key, value));
			}
			key.clear();
		}
	}
	
	ProcessConfigData(output, define_only);
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data_list	The list of configuration data
**	@param	define_only			Whether the elements in the configuration data should only be defined with their ident, without their properties being processed
*/
void CConfigData::ProcessConfigData(const std::vector<CConfigData *> &config_data_list, const bool define_only)
{
	for (size_t i = 0; i < config_data_list.size(); ++i) {
		CConfigData *config_data = config_data_list[i];
		std::string ident = config_data->Ident;
		ident = FindAndReplaceString(ident, "_", "-");
		
		if (config_data->Tag == "age") {
			CAge *age = CAge::GetOrAddAge(ident);
			if (!define_only) {
				age->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "animations") {
			CAnimations *animations = AnimationsByIdent(ident);
			if (!animations) {
				animations = new CAnimations;
				AnimationMap[ident] = animations;
				animations->Ident = ident;
			}
			if (!define_only) {
				animations->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "button") {
			if (!define_only) {
				ButtonAction::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "calendar") {
			CCalendar *calendar = CCalendar::GetOrAddCalendar(ident);
			if (!define_only) {
				calendar->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "character") {
			CCharacter *character = GetCharacter(ident);
			if (!character) {
				if (LoadingHistory) {
					continue; //don't load the history for characters that are no longer in the character database
				}
				character = new CCharacter;
				character->Ident = ident;
				Characters[ident] = character;
			}
			if (!define_only) {
				character->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "deity") {
			CDeity *deity = CDeity::GetOrAddDeity(ident);
			if (!define_only) {
				deity->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "icon") {
			CIcon *icon = CIcon::New(ident);
			if (!define_only) {
				icon->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "map_template") {
			CMapTemplate *map_template = CMapTemplate::GetOrAddMapTemplate(ident);
			if (!define_only) {
				map_template->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "missile_type") {
			MissileType *missile_type = MissileTypeByIdent(ident);
			if (!missile_type) {
				missile_type = NewMissileTypeSlot(ident);
			}
			if (!define_only) {
				missile_type->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "season") {
			CSeason *season = CSeason::GetOrAddSeason(ident);
			if (!define_only) {
				season->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "season_schedule") {
			CSeasonSchedule *season_schedule = CSeasonSchedule::GetOrAddSeasonSchedule(ident);
			if (!define_only) {
				season_schedule->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "sound") {
			if (!define_only) {
				CSound::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "terrain_type") {
			CTerrainType *terrain_type = CTerrainType::GetOrAddTerrainType(ident);
			if (!define_only) {
				terrain_type->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "timeline") {
			CTimeline *timeline = CTimeline::GetOrAddTimeline(ident);
			if (!define_only) {
				timeline->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "time_of_day") {
			CTimeOfDay *time_of_day = CTimeOfDay::GetOrAddTimeOfDay(ident);
			if (!define_only) {
				time_of_day->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "time_of_day_schedule") {
			CTimeOfDaySchedule *time_of_day_schedule = CTimeOfDaySchedule::GetOrAddTimeOfDaySchedule(ident);
			if (!define_only) {
				time_of_day_schedule->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "unit_type") {
			CUnitType *unit_type = UnitTypeByIdent(ident);
			if (!unit_type) {
				unit_type = NewUnitTypeSlot(ident);
			}
			if (!define_only) {
				unit_type->ProcessConfigData(config_data);
			}
		} else {
			fprintf(stderr, "Invalid data type: \"%s\".\n", config_data->Tag.c_str());
		}
	}
}

//@}
