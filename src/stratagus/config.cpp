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

#include "animation/animation.h"
#include "character.h"
#include "config_operator.h"
#include "config_property.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "missile/missile_type.h"
#include "sound/sound.h"
#include "spell/spells.h"
#include "time/calendar.h"
#include "trigger/trigger.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "unit/unit_type.h"
#include "util.h"

#pragma warning(push, 0)
#include <core/os/file_access.h>
#pragma warning(pop)

#include <stdexcept>

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
	std::vector<String> data;
	std::vector<CConfigData *> config_data_elements;
	
	if (!CanAccessFile(filepath.c_str())) {
		print_error("File \"" + String(filepath.c_str()) + "\" not found.");
	}
	
	FileAccess *file = FileAccess::open(filepath.c_str(), FileAccess::READ);
	
	if (file == nullptr) {
		throw std::runtime_error("Failed to open file: " + filepath);
	}
	
	String line;
	
	CConfigData *current_config_data = nullptr;
	int line_index = 1;
	bool is_eof = false;
	while (!is_eof) {
		line = file->get_line();
		is_eof = file->eof_reached();
		
		try {
			std::vector<String> tokens = CConfigData::ParseLine(line);
			CConfigData::ParseTokens(tokens, &current_config_data, config_data_elements);
		} catch (std::exception &exception) {
			print_error("Error parsing config file \"" + String(filepath.c_str()) + "\", line " + String::num_int64(line_index) + ": " + exception.what() + ".");
		}
		++line_index;
	}
	
	file->close();
	memdelete(file);
	
	if (config_data_elements.empty()) {
		print_error("Could not parse output for config file \"" + String(filepath.c_str()) + "\".");
		return;
	}
	
	ProcessConfigData(config_data_elements, define_only);
}

/**
**	@brief	Parse a line in a configuration data file
**
**	@param	line	The line to be parsed
**
**	@return	A vector holding the line's tokens
*/
std::vector<String> CConfigData::ParseLine(const String &line)
{
	std::vector<String> tokens;
	
	bool opened_quotation_marks = false;
	bool escaped = false;
	String current_string;
	
	for (size_t i = 0; i < line.length(); ++i) {
		const wchar_t character = line[i];
		if (!escaped) {
			if (character == '\"') {
				opened_quotation_marks = !opened_quotation_marks;
				continue;
			} else if (character == '\\') {
				escaped = true; //escape character, so that e.g. newlines can be properly added to text
				continue;
			}
		}
		
		if (!opened_quotation_marks) {
			if (character == '#') {
				break; //ignore what is written after the comment symbol ('#'), as well as the symbol itself, unless it occurs within quotes
			}
			
			//whitespace, carriage returns and etc. separate tokens, if they occur outside of quotes
			if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
				if (!current_string.empty()) {
					tokens.push_back(current_string);
					current_string.clear();
				}
				
				continue;
			}
		}
		
		if (escaped) {
			escaped = false;
			
			if (CConfigData::ParseEscapedCharacter(current_string, character)) {
				continue;
			}
		}
		
		current_string += character;
	}
	
	if (!current_string.empty()) {
		tokens.push_back(current_string);
	}
	
	return tokens;
}

/**
**	@brief	Parse an escaped character in a configuration data file line
**
**	@param	current_string	The string currently being built from the parsing
**	@param	character		The character
**
**	@return	True if an escaped character was added to the string, or false otherwise
*/
bool CConfigData::ParseEscapedCharacter(String &current_string, const wchar_t character)
{
	if (character == 'n') {
		current_string += '\n';
	} else if (character == 't') {
		current_string += '\t';
	} else if (character == 'r') {
		current_string += '\r';
	} else if (character == '\"') {
		current_string += '\"';
	} else if (character == '\\') {
		current_string += '\\';
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Parse the tokens from a configuration data file line
**
**	@param	tokens					The tokens to be parsed
**	@param	current_config_data		The current config data in the processing
**	@param	config_data_elements	The config data elements added so far for this file
*/
void CConfigData::ParseTokens(const std::vector<String> &tokens, CConfigData **current_config_data, std::vector<CConfigData *> &config_data_elements)
{
	String key;
	CConfigOperator property_operator = CConfigOperator::None;
	String value;
	for (const String &token : tokens) {
		if (key.empty()) {
			if (token.size() >= 3 && token[0] == '[' && token[1] != '/' && token[token.length() - 1] == ']') { //opens a tag
				String tag_name = token;
				tag_name = tag_name.replace("[", "");
				tag_name = tag_name.replace("]", "");
				bool modification = false;
				if (tag_name[0] == '+' || tag_name[tag_name.length() - 1] == '+') {
					tag_name = tag_name.replace("+", "");
					modification = true;
				}
				CConfigData *new_config_data = new CConfigData(tag_name);
				new_config_data->Modification = modification;
				if ((*current_config_data) != nullptr) {
					new_config_data->Parent = (*current_config_data);
				}
				(*current_config_data) = new_config_data;
			} else if (token.size() >= 3 && token[0] == '[' && token[1] == '/' && token[token.length() - 1] == ']') { //closes a tag
				String tag_name = token;
				tag_name = tag_name.replace("[/", "");
				tag_name = tag_name.replace("]", "");
				if ((*current_config_data) != nullptr) { //closes current tag
					if (tag_name == (*current_config_data)->Tag) {
						if ((*current_config_data)->Parent == nullptr) {
							config_data_elements.push_back((*current_config_data));
							(*current_config_data) = nullptr;
						} else {
							CConfigData *parent_config_data = (*current_config_data)->Parent;
							parent_config_data->Sections.push_back((*current_config_data));
							(*current_config_data) = parent_config_data;
						}
					} else {
						throw std::runtime_error("Tried closing a tag \"" + std::string(tag_name.utf8().get_data()) + "\" while the open tag was \"%s\".");
					}
				} else {
					throw std::runtime_error("Tried closing tag \"" + std::string(tag_name.utf8().get_data()) + "\" before any tag had been opened.");
				}
			} else { //key
				if ((*current_config_data) != nullptr) {
					key = token;
				} else {
					throw std::runtime_error("Tried defining key \"" + std::string(token.utf8().get_data()) + "\" before any tag had been opened.");
				}
			}
			
			continue;
		}
		
		if (property_operator == CConfigOperator::None) { //operator
			if (token == "=") {
				property_operator = CConfigOperator::Assignment;
			} else if (token == "+=") {
				property_operator = CConfigOperator::Addition;
			} else if (token == "-=") {
				property_operator = CConfigOperator::Subtraction;
			} else {
				throw std::runtime_error("Tried using operator \"" + std::string(token.utf8().get_data()) + "\" for key \"" + key.utf8().get_data() + "\", but it is not a valid operator.");
			}
			
			continue;
		}
		
		//value
		if (key == "ident") {
			(*current_config_data)->Ident = token;
		} else if (key == "alias") {
			(*current_config_data)->Aliases.push_back(token);
		} else {
			(*current_config_data)->Properties.push_back(CConfigProperty(key, property_operator, token));
		}
		key.clear();
		property_operator = CConfigOperator::None;
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
	for (CConfigData *config_data : config_data_list) {
		String ident = config_data->Ident;
		ident = ident.replace("_", "-"); //for backwards compatibility with the Lua code
		
		if (ident.empty() && config_data->Tag != "button") {
			print_error("String identifier is empty for config data belonging to tag \"" + config_data->Tag + "\".");
			continue;
		}
		
		bool can_add_new = !config_data->Modification;
		if (LoadingHistory) {
			can_add_new = false;
		}
		
		if (!can_add_new && define_only) {
			continue;
		}
		
		//only load the history for characters that are already in the character database
		const std::map<std::string, std::function<DataElement *(const std::string &)>> &function_map = can_add_new ? CConfigData::DataTypeGetOrAddFunctions : CConfigData::DataTypeGetFunctions;
		
		std::map<std::string, std::function<DataElement *(const std::string &)>>::const_iterator find_iterator = function_map.find(config_data->Tag.utf8().get_data());
		
		if (find_iterator != function_map.end()) {
			DataElement *data_element = find_iterator->second(ident.utf8().get_data());
			if (!data_element) {
				continue;
			}
			if (!define_only) {
				data_element->ProcessConfigData(config_data);
			}
			
			//add aliases for the data element
			std::map<std::string, std::function<void(const std::string &, const std::string &)>>::const_iterator add_alias_find_iterator = CConfigData::DataTypeAddAliasFunctions.find(config_data->Tag.utf8().get_data());
			
			if (add_alias_find_iterator != CConfigData::DataTypeAddAliasFunctions.end()) {
				for (const String &alias : config_data->Aliases) {
					add_alias_find_iterator->second(ident.utf8().get_data(), alias.utf8().get_data());
				}
			}
		} else if (config_data->Tag == "animations") {
			CAnimations *animations = AnimationsByIdent(ident.utf8().get_data());
			if (!animations) {
				animations = new CAnimations;
				AnimationMap[ident.utf8().get_data()] = animations;
				animations->Ident = ident.utf8().get_data();
			}
			if (!define_only) {
				animations->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "button") {
			if (!define_only) {
				ButtonAction::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "button_level") {
			CButtonLevel *button_level = CButtonLevel::GetOrAddButtonLevel(ident.utf8().get_data());
			if (!define_only) {
				button_level->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "missile_type") {
			MissileType *missile_type = MissileTypeByIdent(ident.utf8().get_data());
			if (!missile_type) {
				missile_type = NewMissileTypeSlot(ident.utf8().get_data());
			}
			if (!define_only) {
				missile_type->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "sound") {
			if (!define_only) {
				CSound::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "spell") {
			CSpell *spell = CSpell::GetOrAddSpell(ident.utf8().get_data());
			if (!define_only) {
				spell->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "trigger") {
			CTrigger *trigger = CTrigger::GetOrAddTrigger(ident.utf8().get_data());
			if (!define_only) {
				trigger->ProcessConfigData(config_data);
			}
		} else {
			print_error("Invalid data type: \"" + config_data->Tag + "\".");
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
	
	for (const CConfigProperty &property : this->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
			continue;
		}
		
		if (property.Key == "red") {
			color.r = property.Value.to_int() / 255.0;
		} else if (property.Key == "green") {
			color.g = property.Value.to_int() / 255.0;
		} else if (property.Key == "blue") {
			color.b = property.Value.to_int() / 255.0;
		} else if (property.Key == "alpha") {
			color.a = property.Value.to_int() / 255.0;
		} else {
			print_error("Invalid color property: \"" + property.Key + "\".");
		}
	}
	
	return color;
}
