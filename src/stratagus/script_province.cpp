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
/**@name script_province.cpp - The province ccl functions. */
//
//      (c) Copyright 2016 by Andrettin
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

#include "province.h"

#include "player.h"
#include "script.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Define a province.
**
**  @param l  Lua state.
*/
static int CclDefineProvince(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string province_name = LuaToString(l, 1);
	CProvince *province = GetProvince(province_name);
	if (!province) {
		province = new CProvince;
		province->Name = province_name;
		province->ID = Provinces.size();
		Provinces.push_back(province);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "NameWord")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			int name_language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1, j + 1));
			++j;
			int name_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<std::string> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			int grammatical_number = GrammaticalNumberSingular;
			if (GetGrammaticalNumberIdByName(LuaToString(l, -1, j + 1)) != -1) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
			}
				
			if (name_language != -1 && name_word_type != -1) {
				std::string name_word = LuaToString(l, -1, j + 1);
				LanguageWord *name_element = PlayerRaces.Languages[name_language]->GetWord(name_word, name_word_type, word_meanings);
				
				if (name_element != NULL) {
					if (name_element->NameTypes[grammatical_number].find("province") == name_element->NameTypes[grammatical_number].end()) {
						name_element->NameTypes[grammatical_number]["province"] = 0;
					}
					name_element->NameTypes[grammatical_number]["province"] += 1;
				} else {
					LuaError(l, "The name of the province %s is set to be a compound formed by \"%s\" (%s, %s), but the latter doesn't exist" _C_ province_name.c_str() _C_ name_word.c_str() _C_ PlayerRaces.Languages[name_language]->Name.c_str() _C_ GetWordTypeNameById(name_word_type).c_str());
				}
			} else {
				LuaError(l, "The name of the province %s's compound elements are incorrectly set, as either the language or the word type set for one of the element words given is incorrect" _C_ province_name.c_str());
			}
		} else if (!strcmp(value, "NameCompoundElements")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string affix_type_name = LuaToString(l, -1, j + 1);
				int affix_type = GetAffixTypeIdByName(affix_type_name);
				if (affix_type == -1) {
					LuaError(l, "Affix type \"%s\" doesn't exist." _C_ affix_type_name.c_str());
				}
				++j;
				
				int affix_language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1, j + 1));
				++j;
				int affix_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				
				std::vector<std::string> word_meanings;
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) {
					const int subargs = lua_rawlen(l, -1);
					for (int k = 0; k < subargs; ++k) {
						word_meanings.push_back(LuaToString(l, -1, k + 1));
					}
					
					++j;
				}
				lua_pop(l, 1);

				int grammatical_number = GrammaticalNumberSingular;
				if (GetGrammaticalNumberIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_number_name = LuaToString(l, -1, j + 1);
					grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
					if (grammatical_number == -1) {
						LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
					}
					++j;
				}
				
				int grammatical_case = GrammaticalCaseNominative;
				if (GetGrammaticalCaseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_case_name = LuaToString(l, -1, j + 1);
					grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
					if (grammatical_case == -1) {
						LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
					}
					++j;
				}

				if (affix_language != -1 && affix_word_type != -1) {
					std::string affix_word = LuaToString(l, -1, j + 1);
					LanguageWord *compound_element = PlayerRaces.Languages[affix_language]->GetWord(affix_word, affix_word_type, word_meanings);
					
					if (compound_element != NULL) {
						if (compound_element->AffixNameTypes[WordJunctionTypeCompound][affix_type][grammatical_number][grammatical_case].find("province") == compound_element->AffixNameTypes[WordJunctionTypeCompound][affix_type][grammatical_number][grammatical_case].end()) {
							compound_element->AffixNameTypes[WordJunctionTypeCompound][affix_type][grammatical_number][grammatical_case]["province"] = 0;
						}
						compound_element->AffixNameTypes[WordJunctionTypeCompound][affix_type][grammatical_number][grammatical_case]["province"] += 1;
					} else {
						LuaError(l, "The name of the province %s is set to be a compound formed by \"%s\" (%s, %s), but the latter doesn't exist" _C_ province_name.c_str() _C_ affix_word.c_str() _C_ PlayerRaces.Languages[affix_language]->Name.c_str() _C_ GetWordTypeNameById(affix_word_type).c_str());
					}
				} else {
					LuaError(l, "The name of the province %s's compound elements are incorrectly set, as either the language or the word type set for one of the element words given is incorrect" _C_ province_name.c_str());
				}
			}
		} else if (!strcmp(value, "SettlementName")) {
			province->SettlementName = LuaToString(l, -1);
		} else if (!strcmp(value, "World")) {
			province->World = LuaToString(l, -1);
		} else if (!strcmp(value, "Water")) {
			province->Water = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Coastal")) {
			province->Coastal = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SettlementLocation")) {
			CclGetPos(l, &province->SettlementLocation.x, &province->SettlementLocation.y);
		} else if (!strcmp(value, "Map")) {
			province->Map = LuaToString(l, -1);
		} else if (!strcmp(value, "SettlementTerrain")) {
			province->SettlementTerrain = LuaToString(l, -1);
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->CulturalNames[civilization] = cultural_name;
			}			
		} else if (!strcmp(value, "FactionCulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->FactionCulturalNames[PlayerRaces.Factions[civilization][faction]] = cultural_name;
			}
		} else if (!strcmp(value, "CulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->CulturalSettlementNames[civilization] = cultural_name;
			}
		} else if (!strcmp(value, "FactionCulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->FactionCulturalSettlementNames[PlayerRaces.Factions[civilization][faction]] = cultural_name;
			}
		} else if (!strcmp(value, "Tiles")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				Vec2i tile;
				CclGetPos(l, &tile.x, &tile.y);
				province->Tiles.push_back(tile);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "Claims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				
				province->FactionClaims.push_back(PlayerRaces.Factions[civilization][faction]);
			}			
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Get province data.
**
**  @param l  Lua state.
*/
static int CclGetProvinceData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string province_name = LuaToString(l, 1);
	CProvince *province = GetProvince(province_name);
	if (!province) {
		LuaError(l, "Province \"%s\" doesn't exist." _C_ province_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, province->Name.c_str());
		return 1;
	} else if (!strcmp(data, "SettlementName")) {
		lua_pushstring(l, province->SettlementName.c_str());
		return 1;
	} else if (!strcmp(data, "World")) {
		lua_pushstring(l, province->World.c_str());
		return 1;
	} else if (!strcmp(data, "Water")) {
		lua_pushboolean(l, province->Water);
		return 1;
	} else if (!strcmp(data, "Coastal")) {
		lua_pushboolean(l, province->Coastal);
		return 1;
	} else if (!strcmp(data, "Map")) {
		lua_pushstring(l, province->Map.c_str());
		return 1;
	} else if (!strcmp(data, "SettlementTerrain")) {
		lua_pushstring(l, province->SettlementTerrain.c_str());
		return 1;
	} else if (!strcmp(data, "SettlementLocationX")) {
		lua_pushnumber(l, province->SettlementLocation.x);
		return 1;
	} else if (!strcmp(data, "SettlementLocationY")) {
		lua_pushnumber(l, province->SettlementLocation.y);
		return 1;
	} else if (!strcmp(data, "Tiles")) {
		lua_createtable(l, province->Tiles.size() * 2, 0);
		for (size_t i = 1; i <= province->Tiles.size() * 2; ++i)
		{
			lua_pushnumber(l, province->Tiles[(i-1) / 2].x);
			lua_rawseti(l, -2, i);
			++i;

			lua_pushnumber(l, province->Tiles[(i-1) / 2].y);
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetProvinces(lua_State *l)
{
	lua_createtable(l, Provinces.size(), 0);
	for (size_t i = 1; i <= Provinces.size(); ++i)
	{
		lua_pushstring(l, Provinces[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for provinces.
*/
void ProvinceCclRegister()
{
	lua_register(Lua, "DefineProvince", CclDefineProvince);
	lua_register(Lua, "GetProvinceData", CclGetProvinceData);
	lua_register(Lua, "GetProvinces", CclGetProvinces);
}

//@}
