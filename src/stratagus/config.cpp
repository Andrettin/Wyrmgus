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
/**@name config.cpp - The config file. */
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

#include "character.h"
#include "deity.h"
#include "game.h"
#include "terrain_type.h"
#include "util.h"

#include <fstream>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CConfigData::ParseConfigData(std::string filepath)
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
	
	CConfigData *config_data = NULL;
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
				if (config_data->Parent == NULL) {
					output.push_back(config_data);
					config_data = NULL;
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
	
	ProcessConfigData(output);
}

void CConfigData::ProcessConfigData(const std::vector<CConfigData *> &config_data_list)
{
	for (size_t i = 0; i < config_data_list.size(); ++i) {
		CConfigData *config_data = config_data_list[i];
		std::string ident = config_data->Ident;
		ident = FindAndReplaceString(ident, "_", "-");
		
		if (config_data->Tag == "character") {
			CCharacter *character = GetCharacter(ident);
			if (!character) {
				character = new CCharacter;
				character->Ident = ident;
				Characters[ident] = character;
			} else {
				if (!LoadingHistory) {
					fprintf(stderr, "Character \"%s\" is being redefined.\n", ident.c_str());
				}
			}
			character->ProcessConfigData(config_data);
		} else if (config_data->Tag == "deity") {
			CDeity *deity = CDeity::GetDeity(ident);
			if (!deity) {
				deity = new CDeity;
				deity->Ident = ident;
				CDeity::Deities.push_back(deity);
			}
			deity->ProcessConfigData(config_data);
		} else if (config_data->Tag == "terrain_type") {
			CTerrainType *terrain_type = CTerrainType::GetOrAddTerrainType(ident);
			terrain_type->ProcessConfigData(config_data);
		}
	}
}

//@}
