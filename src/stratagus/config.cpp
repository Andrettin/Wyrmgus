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
#include "animation/animation.h"
#include "character.h"
#include "config_operator.h"
#include "config_property.h"
#include "dynasty.h"
#include "economy/currency.h"
#include "game/game.h"
#include "game/trigger.h"
#include "hair_color.h"
#include "iocompat.h"
#include "iolib.h"
#include "language/language.h"
#include "language/word.h"
#include "literary_text.h"
#include "map/map_template.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "missile/missile_type.h"
#include "player_color.h"
#include "quest/campaign.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "school_of_magic.h"
#include "skin_color.h"
#include "sound/sound.h"
#include "species/species.h"
#include "species/species_category.h"
#include "species/species_category_rank.h"
#include "spell/spells.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "time/timeline.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "ui/icon.h"
#include "unit/historical_unit.h"
#include "unit/unit_type.h"
#include "util.h"
#include "world/plane.h"
#include "world/world.h"

#include <fstream>
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
	std::vector<std::string> data;
	std::vector<CConfigData *> config_data_elements;
	
	if (!CanAccessFile(filepath.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", filepath.c_str());
	}
	
	std::ifstream text_stream(filepath);
	std::string line;
	
	CConfigData *current_config_data = nullptr;
	int line_index = 1;
	while (std::getline(text_stream, line)) {
		try {
			std::vector<std::string> tokens = CConfigData::ParseLine(line);
			CConfigData::ParseTokens(tokens, &current_config_data, config_data_elements);
		} catch (std::exception &exception) {
			fprintf(stderr, "Error parsing config file \"%s\", line %i: %s.\n", filepath.c_str(), line_index, exception.what());
		}
		++line_index;
	}
	
	if (config_data_elements.empty()) {
		fprintf(stderr, "Could not parse output for config file \"%s\".\n", filepath.c_str());
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
std::vector<std::string> CConfigData::ParseLine(const std::string &line)
{
	std::vector<std::string> tokens;
	
	bool opened_quotation_marks = false;
	bool escaped = false;
	std::string current_string;
	
	for (const char &character : line) {
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
bool CConfigData::ParseEscapedCharacter(std::string &current_string, const char character)
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
void CConfigData::ParseTokens(const std::vector<std::string> &tokens, CConfigData **current_config_data, std::vector<CConfigData *> &config_data_elements)
{
	std::string key;
	CConfigOperator property_operator = CConfigOperator::None;
	std::string value;
	for (const std::string &token : tokens) {
		if (key.empty()) {
			if (token.size() >= 3 && token.front() == '[' && token[1] != '/' && token.back() == ']') { //opens a tag
				std::string tag_name = token;
				tag_name = FindAndReplaceString(tag_name, "[", "");
				tag_name = FindAndReplaceString(tag_name, "]", "");
				CConfigData *new_config_data = new CConfigData(tag_name);
				if ((*current_config_data) != nullptr) {
					new_config_data->Parent = (*current_config_data);
				}
				(*current_config_data) = new_config_data;
			} else if (token.size() >= 3 && token.front() == '[' && token[1] == '/' && token.back() == ']') { //closes a tag
				std::string tag_name = token;
				tag_name = FindAndReplaceString(tag_name, "[/", "");
				tag_name = FindAndReplaceString(tag_name, "]", "");
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
						throw std::runtime_error("Tried closing atag \"" + tag_name + "\" while the open tag was \"%s\".");
					}
				} else {
					throw std::runtime_error("Tried closing tag \"" + tag_name + "\" before any tag had been opened.");
				}
			} else { //key
				if ((*current_config_data) != nullptr) {
					key = token;
				} else {
					throw std::runtime_error("Tried defining key \"" + token + "\" before any tag had been opened.");
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
				throw std::runtime_error("Tried using operator \"" + token + "\" for key \"" + key + "\", but it is not a valid operator.");
			}
			
			continue;
		}
		
		//value
		if (key == "ident") {
			(*current_config_data)->Ident = token;
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
	for (size_t i = 0; i < config_data_list.size(); ++i) {
		CConfigData *config_data = config_data_list[i];
		std::string ident = config_data->Ident;
		ident = FindAndReplaceString(ident, "_", "-");
		
		if (ident.empty() && config_data->Tag != "button") {
			fprintf(stderr, "String identifier is empty for config data belonging to tag \"%s\".\n", config_data->Tag.c_str());
			continue;
		}
		
		if (config_data->Tag == "age") {
			CAge *age = CAge::GetOrAdd(ident);
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
			CCampaign *campaign = CCampaign::GetOrAdd(ident);
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
			Currency *currency = Currency::GetOrAdd(ident);
			if (!define_only) {
				currency->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "deity") {
			CDeity *deity = CDeity::GetOrAdd(ident);
			if (!define_only) {
				deity->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "deity_domain") {
			CDeityDomain *deity_domain = CDeityDomain::GetOrAdd(ident);
			if (!define_only) {
				deity_domain->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "dynasty") {
			CDynasty *dynasty = CDynasty::GetOrAdd(ident);
			if (!define_only) {
				dynasty->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "hair_color") {
			CHairColor *hair_color = CHairColor::GetOrAdd(ident);
			if (!define_only) {
				hair_color->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "historical_unit") {
			CHistoricalUnit *historical_unit = CHistoricalUnit::GetOrAdd(ident);
			if (!define_only) {
				historical_unit->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "icon") {
			CIcon *icon = CIcon::GetOrAdd(ident);
			if (!define_only) {
				icon->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "language") {
			CLanguage *language = CLanguage::GetOrAdd(ident);
			if (!define_only) {
				language->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "literary_text") {
			CLiteraryText *literary_text = CLiteraryText::GetOrAdd(ident);
			if (!define_only) {
				literary_text->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "map_template") {
			CMapTemplate *map_template = CMapTemplate::GetOrAdd(ident);
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
			CPantheon *pantheon = CPantheon::GetOrAdd(ident);
			if (!define_only) {
				pantheon->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "plane") {
			CPlane *plane = CPlane::GetOrAddPlane(ident);
			if (!define_only) {
				plane->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "player_color") {
			CPlayerColor *player_color = CPlayerColor::GetOrAdd(ident);
			if (!define_only) {
				player_color->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "religion") {
			CReligion *religion = CReligion::GetOrAdd(ident);
			if (!define_only) {
				religion->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "school_of_magic") {
			CSchoolOfMagic *school_of_magic = CSchoolOfMagic::GetOrAdd(ident);
			if (!define_only) {
				school_of_magic->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "season") {
			CSeason *season = CSeason::GetOrAdd(ident);
			if (!define_only) {
				season->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "season_schedule") {
			CSeasonSchedule *season_schedule = CSeasonSchedule::GetOrAdd(ident);
			if (!define_only) {
				season_schedule->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "site") {
			CSite *site = CSite::GetOrAdd(ident);
			if (!define_only) {
				site->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "skin_color") {
			CSkinColor *skin_color = CSkinColor::GetOrAdd(ident);
			if (!define_only) {
				skin_color->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "sound") {
			if (!define_only) {
				CSound::ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "species") {
			CSpecies *species = CSpecies::GetOrAdd(ident);
			if (!define_only) {
				species->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "species_category") {
			CSpeciesCategory *species_category = CSpeciesCategory::GetOrAdd(ident);
			if (!define_only) {
				species_category->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "species_category_rank") {
			CSpeciesCategoryRank *species_category_rank = CSpeciesCategoryRank::GetOrAdd(ident);
			if (!define_only) {
				species_category_rank->ProcessConfigData(config_data);
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
			CTimeline *timeline = CTimeline::GetOrAdd(ident);
			if (!define_only) {
				timeline->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "time_of_day") {
			CTimeOfDay *time_of_day = CTimeOfDay::GetOrAdd(ident);
			if (!define_only) {
				time_of_day->ProcessConfigData(config_data);
			}
		} else if (config_data->Tag == "time_of_day_schedule") {
			CTimeOfDaySchedule *time_of_day_schedule = CTimeOfDaySchedule::GetOrAdd(ident);
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
		} else if (config_data->Tag == "word") {
			CWord *word = CWord::GetOrAdd(ident);
			if (!define_only) {
				word->ProcessConfigData(config_data);
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
	
	for (const CConfigProperty &property : this->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		if (property.Key == "red") {
			color.r = std::stoi(property.Value) / 255.0;
		} else if (property.Key == "green") {
			color.g = std::stoi(property.Value) / 255.0;
		} else if (property.Key == "blue") {
			color.b = std::stoi(property.Value) / 255.0;
		} else if (property.Key == "alpha") {
			color.a = std::stoi(property.Value) / 255.0;
		} else {
			fprintf(stderr, "Invalid color property: \"%s\".\n", property.Key.c_str());
		}
	}
	
	return color;
}
