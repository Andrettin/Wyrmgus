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
//      (c) Copyright 2018-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "config.h"

#include "age.h"
#include "animation.h"
#include "campaign.h"
#include "character.h"
#include "currency.h"
#include "game.h"
#include "hair_color.h"
#include "icon.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map_template.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "missile.h"
#include "include/plane.h"
#include "player_color.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "school_of_magic.h"
#include "skin_color.h"
#include "sound.h"
#include "spells.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "time/timeline.h"
#include "trigger.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "unit/historical_unit.h"
#include "unit/unittype.h"
#include "util.h"
#include "world.h"

#include <boost/tokenizer.hpp>

#include <fstream>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

static boost::escaped_list_separator<char> ConfigTokenizerSeparator("\\", " \t\r", "\"");
static boost::tokenizer<boost::escaped_list_separator<char>> ConfigTokenizer(std::string(), ConfigTokenizerSeparator);
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Parse a configuration data file
**
**	@param	filepath	The path to the file holding the config data
**	@param	define_only	Whether the elements in the configuration data should only be defined with their ident, without their properties being processed
*/
void CConfigData::ParseConfigData(const std::string &filepath, const bool define_only)
{
	std::vector<std::string> data;
	std::vector<CConfigData *> output;
	
	if (!CanAccessFile(filepath.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", filepath.c_str());
	}
	
	std::ifstream text_stream(filepath);
	std::string line;
	
	while (std::getline(text_stream, line)) {
		CConfigData::ParseLine(line, data);
	}
	
	if (data.empty()) {
		fprintf(stderr, "Could not get string data for config file \"%s\".\n", filepath.c_str());
		return;
	}
	
	CConfigData *config_data = nullptr;
	std::string key;
	std::string value;
	for (size_t i = 0; i < data.size(); ++i) {
		std::string str = data[i];
		if (str.size() >= 2 && str[0] == '[' && str[1] != '/') { //opens a tag
			std::string tag_name = str;
			tag_name = FindAndReplaceString(tag_name, "[", "");
			tag_name = FindAndReplaceString(tag_name, "]", "");
			CConfigData *new_config_data = new CConfigData(tag_name);
			if (config_data) {
				new_config_data->Parent = config_data;
			}
			config_data = new_config_data;
		} else if (str.size() >= 2 && str[0] == '[' && str[1] == '/') { //closes a tag
			std::string tag_name = str;
			tag_name = FindAndReplaceString(tag_name, "[/", "");
			tag_name = FindAndReplaceString(tag_name, "]", "");
			if (config_data) { //closes current tag
				if (tag_name == config_data->Tag) {
					if (config_data->Parent == nullptr) {
						output.push_back(config_data);
						config_data = nullptr;
					} else {
						CConfigData *parent_config_data = config_data->Parent;
						parent_config_data->Children.push_back(config_data);
						config_data = parent_config_data;
					}
				} else {
					fprintf(stderr, "Error parsing config file \"%s\": Tried closing tag \"%s\" while the open tag was \"%s\".\n", filepath.c_str(), tag_name.c_str(), config_data->Tag.c_str());
				}
			} else {
				fprintf(stderr, "Error parsing config file \"%s\": Tried closing a tag (\"%s\") before any tag had been opened.\n", filepath.c_str(), tag_name.c_str());
			}
		} else if (key.empty()) { //key
			if (config_data) {
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
			} else {
				fprintf(stderr, "Error parsing config file \"%s\": Tried defining key \"%s\" before any tag had been opened.\n", filepath.c_str(), str.c_str());
			}
		} else if (!key.empty() && str.length() == 1 && str[0] == '=') { //operator
			continue;
		} else if (!key.empty()) { //value
			if (config_data) {
				std::string value = str;
				value = FindAndReplaceString(value, "=", "");
				if (key == "ident") {
					config_data->Ident = value;
				} else {
					config_data->Properties.push_back(std::pair<std::string, std::string>(key, value));
				}
				key.clear();
			} else {
				fprintf(stderr, "Error parsing config file \"%s\": Tried assigning value \"%s\" to key \"%s\" without any tag being opened.\n", filepath.c_str(), str.c_str(), key.c_str());
			}
		}
	}
	
	if (output.empty()) {
		fprintf(stderr, "Could not parse output for config file \"%s\".\n", filepath.c_str());
		return;
	}
	
	ProcessConfigData(output, define_only);
}

/**
**	@brief	Parse a line in a configuration data file
**
**	@param	line	The line to be parsed
**	@param	data	The vector holding the data file's output
*/
void CConfigData::ParseLine(std::string &line, std::vector<std::string> &data)
{
	size_t comment_pos = 0;
	while ((comment_pos = line.find('#', comment_pos)) != std::string::npos) {
		size_t quotation_mark_pos = 0;
		bool opened_quotation_marks = false;
		while (((quotation_mark_pos = line.find('\"', quotation_mark_pos)) != std::string::npos) && quotation_mark_pos < comment_pos) {
			opened_quotation_marks = !opened_quotation_marks;
			++quotation_mark_pos;
		}
		if (!opened_quotation_marks) {
			line = line.substr(0, comment_pos); //ignore what is written after the comment symbol ('#'), unless the symbol occurs within quotes
			break;
		}
		++comment_pos;
	}
	
	ConfigTokenizer.assign(line);
	
	for (const std::string &token : ConfigTokenizer) {
		data.push_back(token);
	}
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
		
		if (ident.empty() && config_data->Tag != "button") {
			fprintf(stderr, "String identifier is empty for config data belonging to tag \"%s\".\n", config_data->Tag.c_str());
			continue;
		}
		
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
		} else if (config_data->Tag == "button_level") {
			CButtonLevel *button_level = CButtonLevel::GetOrAddButtonLevel(ident);
			if (!define_only) {
				button_level->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "calendar") {
			CCalendar *calendar = CCalendar::GetOrAddCalendar(ident);
			if (!define_only) {
				calendar->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "campaign") {
			CCampaign *campaign = CCampaign::GetOrAddCampaign(ident);
			if (!define_only) {
				campaign->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "character") {
			CCharacter *character = nullptr;
			if (LoadingHistory) {
				//only load the history for characters that are already in the character database
				character = CCharacter::GetCharacter(ident);
			} else {
				character = CCharacter::GetOrAddCharacter(ident);
			}
			if (!character) {
				continue;
			}
			if (!define_only) {
				character->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "currency") {
			CCurrency *currency = CCurrency::GetOrAddCurrency(ident);
			if (!define_only) {
				currency->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "deity") {
			CDeity *deity = CDeity::GetOrAddDeity(ident);
			if (!define_only) {
				deity->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "deity_domain") {
			CDeityDomain *deity_domain = CDeityDomain::GetOrAddDeityDomain(ident);
			if (!define_only) {
				deity_domain->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "hair_color") {
			CHairColor *hair_color = CHairColor::GetOrAddHairColor(ident);
			if (!define_only) {
				hair_color->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "historical_unit") {
			CHistoricalUnit *historical_unit = CHistoricalUnit::GetOrAddHistoricalUnit(ident);
			if (!define_only) {
				historical_unit->ProcessConfigData(config_data);
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
		} else if (config_data->Tag == "pantheon") {
			CPantheon *pantheon = CPantheon::GetOrAddPantheon(ident);
			if (!define_only) {
				pantheon->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "plane") {
			CPlane *plane = CPlane::GetOrAddPlane(ident);
			if (!define_only) {
				plane->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "player_color") {
			CPlayerColor *player_color = CPlayerColor::GetOrAddPlayerColor(ident);
			if (!define_only) {
				player_color->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "school_of_magic") {
			CSchoolOfMagic *school_of_magic = CSchoolOfMagic::GetOrAddSchoolOfMagic(ident);
			if (!define_only) {
				school_of_magic->ProcessConfigData(config_data);
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
		} else if (config_data->Tag == "site") {
			CSite *site = CSite::GetOrAddSite(ident);
			if (!define_only) {
				site->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "skin_color") {
			CSkinColor *skin_color = CSkinColor::GetOrAddSkinColor(ident);
			if (!define_only) {
				skin_color->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "sound") {
			if (!define_only) {
				CSound::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "spell") {
			CSpell *spell = CSpell::GetOrAddSpell(ident);
			if (!define_only) {
				spell->ProcessConfigData(config_data);
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
		} else if (config_data->Tag == "trigger") {
			CTrigger *trigger = CTrigger::GetOrAddTrigger(ident);
			if (!define_only) {
				trigger->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "unit_type") {
			CUnitType *unit_type = UnitTypeByIdent(ident);
			if (!unit_type) {
				unit_type = NewUnitTypeSlot(ident);
			}
			if (!define_only) {
				unit_type->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "upgrade") {
			CUpgrade *upgrade = CUpgrade::New(ident);
			if (!define_only) {
				upgrade->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "world") {
			CWorld *world = CWorld::GetOrAddWorld(ident);
			if (!define_only) {
				world->ProcessConfigData(config_data);
			}
		} else {
			fprintf(stderr, "Invalid data type: \"%s\".\n", config_data->Tag.c_str());
		}
	}
}

/**
**	@brief	Process color configuration data
**
**	@return	The color
*/
Color CConfigData::ProcessColor() const
{
	Color color;
	
	for (size_t i = 0; i < this->Properties.size(); ++i) {
		std::string key = this->Properties[i].first;
		std::string value = this->Properties[i].second;
		
		if (key == "red") {
			color.r = std::stoi(value) / 255.0;
		} else if (key == "green") {
			color.g = std::stoi(value) / 255.0;
		} else if (key == "blue") {
			color.b = std::stoi(value) / 255.0;
		} else if (key == "alpha") {
			color.a = std::stoi(value) / 255.0;
		} else {
			fprintf(stderr, "Invalid color property: \"%s\".\n", key.c_str());
		}
	}
	
	return color;
}
