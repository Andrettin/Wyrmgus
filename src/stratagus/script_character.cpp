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
/**@name script_character.cpp - The character ccl functions. */
//
//      (c) Copyright 2015-2016 by Andrettin
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

#include "character.h"

#include "grand_strategy.h"
#include "player.h"
#include "province.h"
#include "quest.h"
#include "script.h"
#include "spells.h"
#include "unittype.h"
#include "upgrade.h"

#include "../ai/ai_local.h" //for using AiHelpers

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Define a character.
**
**  @param l  Lua state.
*/
static int CclDefineCharacter(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string character_ident = LuaToString(l, 1);
	CCharacter *character = GetCharacter(character_ident);
	bool redefinition = false;
	if (!character) {
		character = new CCharacter;
		character->Ident = character_ident;
		Characters[character_ident] = character;
	} else {
		redefinition = true;
	}
	
	std::string faction_ident;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			character->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "ExtraName")) {
			character->ExtraName = LuaToString(l, -1);
		} else if (!strcmp(value, "FamilyName")) {
			character->FamilyName = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			character->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			character->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			character->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Variation")) { //to keep backwards compatibility
			character->HairVariation = LuaToString(l, -1);
		} else if (!strcmp(value, "HairVariation")) {
			character->HairVariation = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				if (character->Type == NULL || character->Type == UnitTypes[unit_type_id] || character->Type->CanExperienceUpgradeTo(UnitTypes[unit_type_id])) {
					character->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
					if (character->Level < character->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
						character->Level = character->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
					}
				}
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(trait_ident);
			if (upgrade_id != -1) {
				character->Trait = AllUpgrades[upgrade_id];
			} else {
				LuaError(l, "Trait upgrade \"%s\" doesn't exist." _C_ trait_ident.c_str());
			}
		} else if (!strcmp(value, "Year")) {
			character->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DeathYear")) {
			character->DeathYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ViolentDeath")) {
			character->ViolentDeath = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			character->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Faction")) {
			faction_ident = LuaToString(l, -1);
		} else if (!strcmp(value, "ProvinceOfOrigin")) {
			character->ProvinceOfOriginName = LuaToString(l, -1);
		} else if (!strcmp(value, "Father")) {
			std::string father_ident = LuaToString(l, -1);
			CCharacter *father = GetCharacter(father_ident);
			if (father) {
				if (father->Gender == MaleGender) {
					character->Father = const_cast<CCharacter *>(&(*father));
					if (!father->IsParentOf(character_ident)) { //check whether the character has already been set as a child of the father
						father->Children.push_back(character);
					}
					// see if the father's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
					for (size_t i = 0; i < father->Children.size(); ++i) {
						if (father->Children[i]->Ident != character_ident) {
							if (!character->IsSiblingOf(father->Children[i]->Ident)) {
								character->Siblings.push_back(father->Children[i]);
							}
							if (!father->Children[i]->IsSiblingOf(character_ident)) {
								father->Children[i]->Siblings.push_back(character);
							}
						}
					}
				} else {
					LuaError(l, "Character \"%s\" set to be the biological father of \"%s\", but isn't male." _C_ father_ident.c_str() _C_ character_ident.c_str());
				}
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ father_ident.c_str());
			}
		} else if (!strcmp(value, "Mother")) {
			std::string mother_ident = LuaToString(l, -1);
			CCharacter *mother = GetCharacter(mother_ident);
			if (mother) {
				if (mother->Gender == FemaleGender) {
					character->Mother = const_cast<CCharacter *>(&(*mother));
					if (!mother->IsParentOf(character_ident)) { //check whether the character has already been set as a child of the mother
						mother->Children.push_back(character);
					}
					// see if the mother's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
					for (size_t i = 0; i < mother->Children.size(); ++i) {
						if (mother->Children[i]->Ident != character_ident) {
							if (!character->IsSiblingOf(mother->Children[i]->Ident)) {
								character->Siblings.push_back(mother->Children[i]);
							}
							if (!mother->Children[i]->IsSiblingOf(character_ident)) {
								mother->Children[i]->Siblings.push_back(character);
							}
						}
					}
				} else {
					LuaError(l, "Character \"%s\" set to be the biological mother of \"%s\", but isn't female (gender is \"%s\")." _C_ mother_ident.c_str() _C_ character_ident.c_str() _C_ GetGenderNameById(mother->Gender).c_str());
				}
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ mother_ident.c_str());
			}
		} else if (!strcmp(value, "Gender")) {
			character->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			character->Icon.Name = LuaToString(l, -1);
			character->Icon.Icon = NULL;
			character->Icon.Load();
			character->Icon.Icon->Load();
		} else if (!strcmp(value, "HeroicIcon")) {
			character->HeroicIcon.Name = LuaToString(l, -1);
			character->HeroicIcon.Icon = NULL;
			character->HeroicIcon.Load();
			character->HeroicIcon.Icon->Load();
		} else if (!strcmp(value, "Level")) {
			character->Level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ExperiencePercent")) {
			character->ExperiencePercent = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Deity")) {
			int deity_id = PlayerRaces.GetDeityIndexByIdent(LuaToString(l, -1));
			if (deity_id != -1) {
				CDeity *deity = PlayerRaces.Deities[deity_id];
				character->Deity = deity;
				if (character->Icon.Name.empty() && !deity->Icon.Name.empty()) {
					character->Icon.Name = deity->Icon.Name;
					character->Icon.Icon = NULL;
					character->Icon.Load();
				}
			} else {
				LuaError(l, "Deity doesn't exist.");
			}
		} else if (!strcmp(value, "Conditions")) {
			character->Conditions = new LuaCallback(l, -1);
		} else if (!strcmp(value, "Abilities")) {
			character->Abilities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string ability_ident = LuaToString(l, -1, j + 1);
				int ability_id = UpgradeIdByIdent(ability_ident);
				if (ability_id != -1) {
					character->Abilities.push_back(AllUpgrades[ability_id]);
				} else {
					fprintf(stderr, "Ability \"%s\" doesn't exist.", ability_ident.c_str());
				}
			}
		} else if (!strcmp(value, "ReadWorks")) {
			character->ReadWorks.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				int work_id = UpgradeIdByIdent(work_ident);
				if (work_id != -1) {
					character->ReadWorks.push_back(AllUpgrades[work_id]);
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "AuthoredWorks")) {
			character->AuthoredWorks.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				int work_id = UpgradeIdByIdent(work_ident);
				if (work_id != -1) {
					character->AuthoredWorks.push_back(AllUpgrades[work_id]);
					AllUpgrades[work_id]->Author = character;
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "LiteraryAppearances")) {
			character->LiteraryAppearances.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				int work_id = UpgradeIdByIdent(work_ident);
				if (work_id != -1) {
					character->LiteraryAppearances.push_back(AllUpgrades[work_id]);
					AllUpgrades[work_id]->Characters.push_back(character);
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "ConsumedElixirs")) {
			character->ConsumedElixirs.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string elixir_ident = LuaToString(l, -1, j + 1);
				int elixir_id = UpgradeIdByIdent(elixir_ident);
				if (elixir_id != -1) {
					character->ConsumedElixirs.push_back(AllUpgrades[elixir_id]);
				} else {
					fprintf(stderr, "Elixir \"%s\" doesn't exist.", elixir_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Items")) {
			character->Items.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CPersistentItem *item = new CPersistentItem;
				character->Items.push_back(item);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for items)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "type")) {
						std::string item_ident = LuaToString(l, -1, k + 1);
						int item_type_id = UnitTypeIdByIdent(item_ident);
						if (item_type_id != -1) {
							item->Type = const_cast<CUnitType *>(&(*UnitTypes[item_type_id]));
						} else {
							fprintf(stderr, "Item type \"%s\" doesn't exist.\n", item_ident.c_str());
							character->Items.erase(std::remove(character->Items.begin(), character->Items.end(), item), character->Items.end());
							delete item;
							break;
						}
					} else if (!strcmp(value, "prefix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Prefix = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Item prefix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "suffix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Suffix = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Item suffix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "spell")) {
						std::string spell_ident = LuaToString(l, -1, k + 1);
						SpellType *spell = SpellTypeByIdent(spell_ident);
						if (spell != NULL) {
							item->Spell = const_cast<SpellType *>(&(*spell));
						} else {
							fprintf(stderr, "Spell \"%s\" doesn't exist.", spell_ident.c_str());
						}
					} else if (!strcmp(value, "work")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Work = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Literary work \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "elixir")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Elixir = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Elixir \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "name")) {
						item->Name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "unique")) {
						std::string unique_ident = LuaToString(l, -1, k + 1);
						CUniqueItem *unique_item = GetUniqueItem(unique_ident);
						item->Unique = unique_item;
						if (unique_item != NULL) {
							item->Name = unique_item->Name;
							if (unique_item->Type != NULL) {
								item->Type = unique_item->Type;
							} else {
								fprintf(stderr, "Unique item \"%s\" has no type.\n", item->Name.c_str());
							}
							item->Prefix = unique_item->Prefix;
							item->Suffix = unique_item->Suffix;
							item->Spell = unique_item->Spell;
							item->Work = unique_item->Work;
							item->Elixir = unique_item->Elixir;
						} else {
							fprintf(stderr, "Unique item \"%s\" doesn't exist.\n", unique_ident.c_str());
						}
					} else if (!strcmp(value, "bound")) {
						item->Bound = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "identified")) {
						item->Identified = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "equipped")) {
						bool is_equipped = LuaToBoolean(l, -1, k + 1);
						if (is_equipped && GetItemClassSlot(item->Type->ItemClass) != -1) {
							character->EquippedItems[GetItemClassSlot(item->Type->ItemClass)].push_back(item);
						}
					} else {
						printf("\n%s\n", character->Ident.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "ForbiddenUpgrades")) {
			memset(character->ForbiddenUpgrades, 0, sizeof(character->ForbiddenUpgrades));
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string unit_type_ident = LuaToString(l, -1, j + 1);
				int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
				if (unit_type_id != -1) {
					character->ForbiddenUpgrades[unit_type_id] = true;
				} else {
					LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
				}
			}
		} else if (!strcmp(value, "HistoricalTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int title = GetCharacterTitleIdByName(LuaToString(l, -1, j + 1));
				if (title == -1) {
					LuaError(l, "Character title doesn't exist.");
				}
				++j;
				int start_year = LuaToNumber(l, -1, j + 1);
				++j;
				int end_year = LuaToNumber(l, -1, j + 1);
				++j;
				
				if (title != CharacterTitleGovernor) {
					std::string title_civilization_name = LuaToString(l, -1, j + 1);
					int title_civilization = PlayerRaces.GetRaceIndexByName(title_civilization_name.c_str());
					++j;
					std::string title_faction_name = LuaToString(l, -1, j + 1);
					int title_faction = PlayerRaces.GetFactionIndexByName(title_civilization, title_faction_name);
					if (title_faction == -1) {
						LuaError(l, "Faction \"%s\" doesn't exist." _C_ title_faction_name.c_str());
					}
					if (start_year != 0 && end_year != 0 && IsMinisterialTitle(title)) { // don't put in the faction's historical data if a blank year was given
						PlayerRaces.Factions[title_civilization][title_faction]->HistoricalMinisters[std::tuple<int, int, int>(start_year, end_year, title)] = character;
					}
					character->HistoricalTitles.push_back(std::tuple<int, int, CFaction *, int>(start_year, end_year, PlayerRaces.Factions[title_civilization][title_faction], title));
				} else {
					std::string title_province_name = LuaToString(l, -1, j + 1);
					CProvince *title_province = GetProvince(title_province_name);
					if (title_province == NULL) {
						LuaError(l, "Province \"%s\" doesn't exist." _C_ title_province_name.c_str());
					}
					if (start_year != 0 && end_year != 0) { // don't put in the province's historical data if a blank year was given
						title_province->HistoricalGovernors[std::tuple<int, int>(start_year, end_year)] = character;
					}
					character->HistoricalProvinceTitles.push_back(std::tuple<int, int, CProvince *, int>(start_year, end_year, title_province, title));
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (character->Civilization != -1 && !faction_ident.empty()) { //we have to set the faction here, because Lua tables are in an arbitrary order, and the character needs its civilization to have been set before it can find its faction
		int faction = PlayerRaces.GetFactionIndexByName(character->Civilization, faction_ident);
		if (faction != -1) {
			character->Faction = faction;
		} else {
			LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_ident.c_str());
		}
	}
	
	if (!redefinition) {
		if (character->Type->BoolFlag[FAUNA_INDEX].value) {
			character->Type->PersonalNames[character->Gender].push_back(character->Name);
		} else if (character->Civilization != -1) {
			int language = character->GetLanguage();
			if (language == PlayerRaces.GetCivilizationLanguage(character->Civilization)) {
				PlayerRaces.Civilizations[character->Civilization]->PersonalNames[character->Gender].push_back(character->Name);
			} else if (character->Faction != -1) {
				int base_faction = character->Faction;
				while (PlayerRaces.Factions[character->Civilization][base_faction]->ParentFaction != -1) {
					base_faction = PlayerRaces.Factions[character->Civilization][base_faction]->ParentFaction;
				}
				PlayerRaces.Factions[character->Civilization][base_faction]->PersonalNames[character->Gender].push_back(character->Name);
			}
		}
	}
	
	if (character->Trait == NULL) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (character->Type != NULL && character->Type->Traits.size() == 1) {
			character->Trait = character->Type->Traits[0];
		}
	}
		
	if (character->Gender == NoGender) { //if no gender was set, have the character be the same gender as the unit type (if the unit type has it predefined)
		if (character->Type != NULL && character->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
			character->Gender = character->Type->DefaultStat.Variables[GENDER_INDEX].Value;
		}
	}
		
	//check if the abilities are correct for this character's unit type
	if (character->Type != NULL && character->Abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > character->Type->Slot) {
		int ability_count = (int) character->Abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (std::find(AiHelpers.LearnableAbilities[character->Type->Slot].begin(), AiHelpers.LearnableAbilities[character->Type->Slot].end(), character->Abilities[i]) == AiHelpers.LearnableAbilities[character->Type->Slot].end()) {
				character->Abilities.erase(std::remove(character->Abilities.begin(), character->Abilities.end(), character->Abilities[i]), character->Abilities.end());
			}
		}
	}

	character->UpdateAttributes();
	
	return 0;
}

/**
**  Define a custom hero.
**
**  @param l  Lua state.
*/
static int CclDefineCustomHero(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string hero_ident = LuaToString(l, 1);
	CCharacter *hero = GetCustomHero(hero_ident);
	if (!hero) {
		hero = new CCharacter;
		hero->Ident = hero_ident;
		CustomHeroes[hero_ident] = hero;
	} else {
		fprintf(stderr, "Custom hero \"%s\" is being redefined.\n", hero_ident.c_str());
	}
	hero->Custom = true;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			hero->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "ExtraName")) {
			hero->ExtraName = LuaToString(l, -1);
		} else if (!strcmp(value, "FamilyName")) {
			hero->FamilyName = LuaToString(l, -1);
		} else if (!strcmp(value, "Dynasty")) { // for backwards compatibility
			hero->FamilyName = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			hero->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Variation")) { //to keep backwards compatibility
			hero->HairVariation = LuaToString(l, -1);
		} else if (!strcmp(value, "HairVariation")) {
			hero->HairVariation = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				hero->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
				if (hero->Level < hero->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
					hero->Level = hero->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
				}
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(trait_ident);
			if (upgrade_id != -1) {
				hero->Trait = AllUpgrades[upgrade_id];
			} else {
				LuaError(l, "Trait upgrade \"%s\" doesn't exist." _C_ trait_ident.c_str());
			}
		} else if (!strcmp(value, "Civilization")) {
			hero->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			hero->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Level")) {
			hero->Level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ExperiencePercent")) {
			hero->ExperiencePercent = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Abilities")) {
			hero->Abilities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string ability_ident = LuaToString(l, -1, j + 1);
				int ability_id = UpgradeIdByIdent(ability_ident);
				if (ability_id != -1) {
					hero->Abilities.push_back(AllUpgrades[ability_id]);
				} else {
					fprintf(stderr, "Ability \"%s\" doesn't exist.", ability_ident.c_str());
				}
			}
		} else if (!strcmp(value, "ReadWorks")) {
			hero->ReadWorks.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				int work_id = UpgradeIdByIdent(work_ident);
				if (work_id != -1) {
					hero->ReadWorks.push_back(AllUpgrades[work_id]);
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "ConsumedElixirs")) {
			hero->ConsumedElixirs.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string elixir_ident = LuaToString(l, -1, j + 1);
				int elixir_id = UpgradeIdByIdent(elixir_ident);
				if (elixir_id != -1) {
					hero->ConsumedElixirs.push_back(AllUpgrades[elixir_id]);
				} else {
					fprintf(stderr, "Elixir \"%s\" doesn't exist.", elixir_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Items")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CPersistentItem *item = new CPersistentItem;
				hero->Items.push_back(item);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for items)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "type")) {
						std::string item_ident = LuaToString(l, -1, k + 1);
						int item_type_id = UnitTypeIdByIdent(item_ident);
						if (item_type_id != -1) {
							item->Type = const_cast<CUnitType *>(&(*UnitTypes[item_type_id]));
						} else {
							fprintf(stderr, "Item type \"%s\" doesn't exist.\n", item_ident.c_str());
							hero->Items.erase(std::remove(hero->Items.begin(), hero->Items.end(), item), hero->Items.end());
							delete item;
							break;
						}
					} else if (!strcmp(value, "prefix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Prefix = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Item prefix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "suffix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Suffix = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Item suffix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "spell")) {
						std::string spell_ident = LuaToString(l, -1, k + 1);
						SpellType *spell = SpellTypeByIdent(spell_ident);
						if (spell != NULL) {
							item->Spell = const_cast<SpellType *>(&(*spell));
						} else {
							fprintf(stderr, "Spell \"%s\" doesn't exist.", spell_ident.c_str());
						}
					} else if (!strcmp(value, "work")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Work = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Literary work \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "elixir")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						int upgrade_id = UpgradeIdByIdent(upgrade_ident);
						if (upgrade_id != -1) {
							item->Elixir = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
						} else {
							fprintf(stderr, "Elixir \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "name")) {
						item->Name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "unique")) {
						std::string unique_ident = LuaToString(l, -1, k + 1);
						CUniqueItem *unique_item = GetUniqueItem(unique_ident);
						item->Unique = unique_item;
						if (unique_item != NULL) {
							item->Name = unique_item->Name;
							if (unique_item->Type != NULL) {
								item->Type = unique_item->Type;
							} else {
								fprintf(stderr, "Unique item \"%s\" has no type.\n", item->Name.c_str());
							}
							item->Prefix = unique_item->Prefix;
							item->Suffix = unique_item->Suffix;
							item->Spell = unique_item->Spell;
							item->Work = unique_item->Work;
							item->Elixir = unique_item->Elixir;
						} else {
							fprintf(stderr, "Unique item \"%s\" doesn't exist.\n", unique_ident.c_str());
						}
					} else if (!strcmp(value, "bound")) {
						item->Bound = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "identified")) {
						item->Identified = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "equipped")) {
						bool is_equipped = LuaToBoolean(l, -1, k + 1);
						if (is_equipped && GetItemClassSlot(item->Type->ItemClass) != -1) {
							hero->EquippedItems[GetItemClassSlot(item->Type->ItemClass)].push_back(item);
						}
					} else {
						printf("\n%s\n", hero->Ident.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "QuestsInProgress")) {
			hero->QuestsInProgress.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string quest_name = LuaToString(l, -1, j + 1);
				if (GetQuest(quest_name) != NULL) {
					hero->QuestsInProgress.push_back(GetQuest(quest_name));
				} else {
					LuaError(l, "Quest \"%s\" doesn't exist." _C_ quest_name.c_str());
				}
			}
		} else if (!strcmp(value, "QuestsCompleted")) {
			hero->QuestsCompleted.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string quest_name = LuaToString(l, -1, j + 1);
				if (GetQuest(quest_name) != NULL) {
					hero->QuestsCompleted.push_back(GetQuest(quest_name));
				} else {
					LuaError(l, "Quest \"%s\" doesn't exist." _C_ quest_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForbiddenUpgrades")) {
			memset(hero->ForbiddenUpgrades, 0, sizeof(hero->ForbiddenUpgrades));
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string unit_type_ident = LuaToString(l, -1, j + 1);
				int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
				if (unit_type_id != -1) {
					hero->ForbiddenUpgrades[unit_type_id] = true;
				} else {
					LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Icon")) {
			hero->Icon.Name = LuaToString(l, -1);
			hero->Icon.Icon = NULL;
			hero->Icon.Load();
			hero->Icon.Icon->Load();
		} else if (!strcmp(value, "HeroicIcon")) {
			hero->HeroicIcon.Name = LuaToString(l, -1);
			hero->HeroicIcon.Icon = NULL;
			hero->HeroicIcon.Load();
			hero->HeroicIcon.Icon->Load();
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (hero->Gender == NoGender) { //if no gender was set, have the hero be the same gender as the unit type (if the unit type has it predefined)
		if (hero->Type != NULL && hero->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
			hero->Gender = hero->Type->DefaultStat.Variables[GENDER_INDEX].Value;
		}
	}
	
	//check if the abilities are correct for this hero's unit type
	if (hero->Abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > hero->Type->Slot) {
		int ability_count = (int) hero->Abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (std::find(AiHelpers.LearnableAbilities[hero->Type->Slot].begin(), AiHelpers.LearnableAbilities[hero->Type->Slot].end(), hero->Abilities[i]) == AiHelpers.LearnableAbilities[hero->Type->Slot].end()) {
				hero->Abilities.erase(std::remove(hero->Abilities.begin(), hero->Abilities.end(), hero->Abilities[i]), hero->Abilities.end());
			}
		}
	}
	
	return 0;
}

/**
**  Define a grand strategy hero.
**
**  @param l  Lua state.
*/
static int CclDefineGrandStrategyHero(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string hero_full_name = TransliterateText(LuaToString(l, 1));
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (!hero) {
		hero = new CGrandStrategyHero;
		GrandStrategyGame.Heroes.push_back(hero);
		GrandStrategyHeroStringToIndex[hero_full_name] = GrandStrategyGame.Heroes.size() - 1;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			hero->Name = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "ExtraName")) {
			hero->ExtraName = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "FamilyName")) {
			hero->FamilyName = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Dynasty")) { // for backwards compatibility
			hero->FamilyName = TransliterateText(LuaToString(l, -1));
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				hero->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(trait_ident);
			if (upgrade_id != -1) {
				hero->Trait = AllUpgrades[upgrade_id];
			} else {
				LuaError(l, "Trait upgrade \"%s\" doesn't exist." _C_ trait_ident.c_str());
			}
		} else if (!strcmp(value, "Year")) {
			hero->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DeathYear")) {
			hero->DeathYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			hero->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "ProvinceOfOrigin")) {
			hero->ProvinceOfOriginName = LuaToString(l, -1);
		} else if (!strcmp(value, "Father")) {
			std::string father_name = TransliterateText(LuaToString(l, -1));
			CGrandStrategyHero *father = GrandStrategyGame.GetHero(father_name);
			if (father) {
				if (father->Gender == MaleGender) {
					hero->Father = const_cast<CGrandStrategyHero *>(&(*father));
					if (!father->IsParentOf(hero_full_name)) { //check whether the hero has already been set as a child of the father
						father->Children.push_back(hero);
					}
					// see if the father's other children aren't already included in the hero's siblings, and if they aren't, add them (and add the hero to the siblings' sibling list, of course)
					for (size_t i = 0; i < father->Children.size(); ++i) {
						if (father->Children[i]->Ident != hero_full_name) {
							if (!hero->IsSiblingOf(father->Children[i]->Ident)) {
								hero->Siblings.push_back(father->Children[i]);
							}
							if (!father->Children[i]->IsSiblingOf(hero_full_name)) {
								father->Children[i]->Siblings.push_back(hero);
							}
						}
					}
				} else {
					LuaError(l, "Hero \"%s\" set to be the biological father of \"%s\", but isn't male." _C_ father_name.c_str() _C_ hero_full_name.c_str());
				}
			} else {
				LuaError(l, "Hero \"%s\" doesn't exist." _C_ father_name.c_str());
			}
		} else if (!strcmp(value, "Mother")) {
			std::string mother_name = TransliterateText(LuaToString(l, -1));
			CGrandStrategyHero *mother = GrandStrategyGame.GetHero(mother_name);
			if (mother) {
				if (mother->Gender == FemaleGender) {
					hero->Mother = const_cast<CGrandStrategyHero *>(&(*mother));
					if (!mother->IsParentOf(hero_full_name)) { //check whether the hero has already been set as a child of the mother
						mother->Children.push_back(hero);
					}
					// see if the mother's other children aren't already included in the hero's siblings, and if they aren't, add them (and add the hero to the siblings' sibling list, of course)
					for (size_t i = 0; i < mother->Children.size(); ++i) {
						if (mother->Children[i]->Ident != hero_full_name) {
							if (!hero->IsSiblingOf(mother->Children[i]->Ident)) {
								hero->Siblings.push_back(mother->Children[i]);
							}
							if (!mother->Children[i]->IsSiblingOf(hero_full_name)) {
								mother->Children[i]->Siblings.push_back(hero);
							}
						}
					}
				} else {
					LuaError(l, "Hero \"%s\" set to be the biological mother of \"%s\", but isn't female." _C_ mother_name.c_str() _C_ hero_full_name.c_str());
				}
			} else {
				LuaError(l, "Hero \"%s\" doesn't exist." _C_ mother_name.c_str());
			}
		} else if (!strcmp(value, "Gender")) {
			hero->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "HairVariation")) {
			hero->HairVariation = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	hero->Initialize();
	hero->UpdateAttributes();
	
	return 0;
}

/**
**  Get character data.
**
**  @param l  Lua state.
*/
static int CclGetCharacterData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string character_name = LuaToString(l, 1);
	CCharacter *character = GetCharacter(character_name);
	if (!character) {
		LuaError(l, "Character \"%s\" doesn't exist." _C_ character_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, character->Name.c_str());
		return 1;
	} else if (!strcmp(data, "FamilyName")) {
		lua_pushstring(l, character->FamilyName.c_str());
		return 1;
	} else if (!strcmp(data, "FullName")) {
		lua_pushstring(l, character->GetFullName().c_str());
		return 1;
	} else if (!strcmp(data, "ProvinceOfOrigin")) {
		lua_pushstring(l, character->ProvinceOfOriginName.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, character->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, character->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, character->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (character->Civilization != -1) {
			lua_pushstring(l, PlayerRaces.Name[character->Civilization].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (character->Faction != -1) {
			lua_pushstring(l, PlayerRaces.Factions[character->Civilization][character->Faction]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Year")) {
		lua_pushnumber(l, character->Year);
		return 1;
	} else if (!strcmp(data, "DeathYear")) {
		lua_pushnumber(l, character->DeathYear);
		return 1;
	} else if (!strcmp(data, "ViolentDeath")) {
		lua_pushboolean(l, character->ViolentDeath);
		return 1;
	} else if (!strcmp(data, "Gender")) {
		lua_pushstring(l, GetGenderNameById(character->Gender).c_str());
		return 1;
	} else if (!strcmp(data, "Level")) {
		lua_pushnumber(l, character->Level);
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (character->Type != NULL) {
			lua_pushstring(l, character->Type->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Trait")) {
		if (character->Trait != NULL) {
			lua_pushstring(l, character->Trait->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Deity")) {
		if (character->Deity != NULL) {
			lua_pushstring(l, character->Deity->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Father")) {
		if (character->Father != NULL) {
			lua_pushstring(l, character->Father->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Mother")) {
		if (character->Mother != NULL) {
			lua_pushstring(l, character->Mother->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, character->GetIcon().Name.c_str());
		return 1;
	} else if (!strcmp(data, "BaseIcon")) {
		lua_pushstring(l, character->Icon.Name.c_str());
		return 1;
	} else if (!strcmp(data, "HairVariation")) {
		lua_pushstring(l, character->HairVariation.c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}
	
	return 0;
}

/**
**  Get custom hero data.
**
**  @param l  Lua state.
*/
static int CclGetCustomHeroData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string character_name = LuaToString(l, 1);
	CCharacter *character = GetCustomHero(character_name);
	if (!character) {
		LuaError(l, "Custom hero \"%s\" doesn't exist." _C_ character_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, character->Name.c_str());
		return 1;
	} else if (!strcmp(data, "FamilyName")) {
		lua_pushstring(l, character->FamilyName.c_str());
		return 1;
	} else if (!strcmp(data, "FullName")) {
		lua_pushstring(l, character->GetFullName().c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (character->Civilization != -1) {
			lua_pushstring(l, PlayerRaces.Name[character->Civilization].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Gender")) {
		lua_pushstring(l, GetGenderNameById(character->Gender).c_str());
		return 1;
	} else if (!strcmp(data, "Level")) {
		lua_pushnumber(l, character->Level);
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (character->Type != NULL) {
			lua_pushstring(l, character->Type->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Trait")) {
		if (character->Trait != NULL) {
			lua_pushstring(l, character->Trait->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, character->GetIcon().Name.c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetCharacters(lua_State *l)
{
	std::vector<std::string> character_names;
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		character_names.push_back(iterator->first);
	}
	
	lua_createtable(l, character_names.size(), 0);
	for (size_t i = 1; i <= character_names.size(); ++i)
	{
		lua_pushstring(l, character_names[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetCustomHeroes(lua_State *l)
{
	std::vector<std::string> character_names;
	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
		character_names.push_back(iterator->first);
	}
	
	lua_createtable(l, character_names.size(), 0);
	for (size_t i = 1; i <= character_names.size(); ++i)
	{
		lua_pushstring(l, character_names[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetGrandStrategyHeroes(lua_State *l)
{
	lua_createtable(l, GrandStrategyGame.Heroes.size(), 0);
	for (size_t i = 1; i <= GrandStrategyGame.Heroes.size(); ++i)
	{
		lua_pushstring(l, GrandStrategyGame.Heroes[i-1]->GetFullName().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for characters.
*/
void CharacterCclRegister()
{
	lua_register(Lua, "DefineCharacter", CclDefineCharacter);
	lua_register(Lua, "DefineCustomHero", CclDefineCustomHero);
	lua_register(Lua, "DefineGrandStrategyHero", CclDefineGrandStrategyHero);
	lua_register(Lua, "GetCharacterData", CclGetCharacterData);
	lua_register(Lua, "GetCustomHeroData", CclGetCustomHeroData);
	lua_register(Lua, "GetCharacters", CclGetCharacters);
	lua_register(Lua, "GetCustomHeroes", CclGetCustomHeroes);
	lua_register(Lua, "GetGrandStrategyHeroes", CclGetGrandStrategyHeroes);
}

//@}
