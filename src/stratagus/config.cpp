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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "config.h"

#include "animation/animation.h"
#include "character.h"
#include "currency.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map_template.h"
#include "map/site.h"
#include "missile.h"
#include "quest/campaign.h"
#include "script/trigger.h"
#include "sound/sound.h"
#include "ui/button.h"
#include "unit/unit_type.h"
#include "util/util.h"

#include <boost/tokenizer.hpp>

static boost::escaped_list_separator<char> ConfigTokenizerSeparator("\\", " \t\r", "\"");
static boost::tokenizer<boost::escaped_list_separator<char>> ConfigTokenizer(std::string(), ConfigTokenizerSeparator);
	
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
		
		if (config_data->Tag == "button") {
			if (!define_only) {
				wyrmgus::button::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "campaign") {
			wyrmgus::campaign *campaign = wyrmgus::campaign::get_or_add(ident, nullptr);
			if (!define_only) {
				campaign->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "character") {
			wyrmgus::character *character = wyrmgus::character::get_or_add(ident, nullptr);
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
		} else if (config_data->Tag == "map_template") {
			wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(ident, nullptr);
			if (!define_only) {
				map_template->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "missile_type") {
			wyrmgus::missile_type *missile_type = wyrmgus::missile_type::get_or_add(ident, nullptr);
			if (!define_only) {
				missile_type->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "site") {
			wyrmgus::site *site = wyrmgus::site::get_or_add(ident, nullptr);
			if (!define_only) {
				site->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "unit_type") {
			wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get_or_add(ident, nullptr);
			if (!define_only) {
				unit_type->ProcessConfigData(config_data);
			}
		} else {
			fprintf(stderr, "Invalid data type: \"%s\".\n", config_data->Tag.c_str());
		}
	}
}
