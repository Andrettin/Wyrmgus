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
//      (c) Copyright 2015-2020 by Andrettin
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

#include "character.h"

#include "ai/ai_local.h" //for using AiHelpers
#include "civilization.h"
#include "grand_strategy.h"
#include "map/historical_location.h"
#include "map/map_template.h"
#include "map/site.h"
#include "player.h"
#include "province.h"
#include "quest.h"
#include "religion/deity.h"
#include "script.h"
#include "spells.h"
#include "time/timeline.h"
#include "unit/unittype.h"
#include "upgrade/upgrade.h"

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
	CCharacter *character = CCharacter::GetCharacter(character_ident, false);
	bool redefinition = false;
	if (!character) {
		if (LoadingPersistentHeroes) {
			fprintf(stderr, "Character \"%s\" has persistent data, but doesn't exist.", character_ident.c_str());
			return 0;
		}
		character = CCharacter::GetOrAddCharacter(character_ident);
	} else {
		redefinition = true;
		if (!LoadingPersistentHeroes) {
			fprintf(stderr, "Character \"%s\" is being redefined.\n", character_ident.c_str());
		}
	}
	
	std::string faction_ident;
	std::vector<std::string> alternate_names;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			character->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "AlternateNames")) { // alternate names the character may have, used for building the civilization's personal names
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				alternate_names.push_back(LuaToString(l, -1, j + 1));
			}
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
			CUnitType *unit_type = CUnitType::get(unit_type_ident);
			if (character->Type == nullptr || character->Type == unit_type || character->Type->CanExperienceUpgradeTo(unit_type)) {
				character->Type = unit_type;
				if (character->Level < character->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
					character->Level = character->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
				}
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(trait_ident);
			character->Trait = upgrade;
		} else if (!strcmp(value, "BirthDate")) {
			CclGetDate(l, &character->BirthDate);
		} else if (!strcmp(value, "StartDate")) {
			CclGetDate(l, &character->StartDate);
		} else if (!strcmp(value, "DeathDate")) {
			CclGetDate(l, &character->DeathDate);
		} else if (!strcmp(value, "ViolentDeath")) {
			character->ViolentDeath = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			character->civilization = stratagus::civilization::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Faction")) {
			CFaction *faction = PlayerRaces.GetFaction(LuaToString(l, -1));
			if (faction != nullptr) {
				character->Faction = faction;
			} else {
				LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_ident.c_str());
			}
		} else if (!strcmp(value, "Father")) {
			std::string father_ident = LuaToString(l, -1);
			CCharacter *father = CCharacter::GetCharacter(father_ident);
			if (father) {
				if (father->Gender == MaleGender || !father->Initialized) {
					character->Father = father;
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
			CCharacter *mother = CCharacter::GetCharacter(mother_ident);
			if (mother) {
				if (mother->Gender == FemaleGender || !mother->Initialized) {
					character->Mother = mother;
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
		} else if (!strcmp(value, "Children")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string child_ident = LuaToString(l, -1, j + 1);
				CCharacter *child = CCharacter::GetCharacter(child_ident);
				if (child) {
					if (character->Gender == MaleGender) {
						child->Father = character;
					} else {
						child->Mother = character;
					}
					if (!character->IsParentOf(child_ident)) { //check whether the character has already been set as a parent of the child
						character->Children.push_back(child);
					}
					// see if the character's other children aren't already included in the child's siblings, and if they aren't, add them (and add the character to the siblings' sibling list too)
					for (size_t i = 0; i < character->Children.size(); ++i) {
						if (character->Children[i] != child) {
							if (!child->IsSiblingOf(character->Children[i]->Ident)) {
								child->Siblings.push_back(character->Children[i]);
							}
							if (!character->Children[i]->IsSiblingOf(child_ident)) {
								character->Children[i]->Siblings.push_back(child);
							}
						}
					}
				} else {
					LuaError(l, "Character \"%s\" doesn't exist." _C_ child_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Gender")) {
			character->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			character->Icon.Name = LuaToString(l, -1);
			character->Icon.Icon = nullptr;
			character->Icon.Load();
		} else if (!strcmp(value, "HeroicIcon")) {
			character->HeroicIcon.Name = LuaToString(l, -1);
			character->HeroicIcon.Icon = nullptr;
			character->HeroicIcon.Load();
		} else if (!strcmp(value, "Level")) {
			character->Level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ExperiencePercent")) {
			character->ExperiencePercent = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Deity")) {
			CDeity *deity = CDeity::GetDeity(LuaToString(l, -1));
			if (deity) {
				character->Deity = deity;
				if (character->Icon.Name.empty() && !deity->Icon.Name.empty()) {
					character->Icon.Name = deity->Icon.Name;
					character->Icon.Icon = nullptr;
					character->Icon.Load();
				}
			}
		} else if (!strcmp(value, "Conditions")) {
			character->Conditions = new LuaCallback(l, -1);
		} else if (!strcmp(value, "Abilities")) {
			character->Abilities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string ability_ident = LuaToString(l, -1, j + 1);
				CUpgrade *ability = CUpgrade::try_get(ability_ident);
				if (ability != nullptr) {
					character->Abilities.push_back(ability);
				} else {
					fprintf(stderr, "Ability \"%s\" doesn't exist.", ability_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Deities")) {
			character->Deities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string deity_ident = LuaToString(l, -1, j + 1);
				CDeity *deity = CDeity::GetDeity(deity_ident);
				if (deity) {
					character->Deities.push_back(deity);
				}
			}
		} else if (!strcmp(value, "ReadWorks")) {
			character->ReadWorks.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				CUpgrade *work = CUpgrade::try_get(work_ident);
				if (work != nullptr) {
					character->ReadWorks.push_back(work);
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "AuthoredWorks")) {
			character->AuthoredWorks.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				CUpgrade *work = CUpgrade::try_get(work_ident);
				if (work != nullptr) {
					character->AuthoredWorks.push_back(work);
					work->Author = character;
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "LiteraryAppearances")) {
			character->LiteraryAppearances.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				CUpgrade *work = CUpgrade::try_get(work_ident);
				if (work != nullptr) {
					character->LiteraryAppearances.push_back(work);
					work->Characters.push_back(character);
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "ConsumedElixirs")) {
			character->ConsumedElixirs.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string elixir_ident = LuaToString(l, -1, j + 1);
				CUpgrade *elixir = CUpgrade::try_get(elixir_ident);
				if (elixir != nullptr) {
					character->ConsumedElixirs.push_back(elixir);
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
				item->Owner = character;
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
						CUnitType *item_type = CUnitType::try_get(item_ident);
						if (item_type != nullptr) {
							item->Type = item_type;
						} else {
							fprintf(stderr, "Item type \"%s\" doesn't exist.\n", item_ident.c_str());
							character->Items.erase(std::remove(character->Items.begin(), character->Items.end(), item), character->Items.end());
							delete item;
							break;
						}
					} else if (!strcmp(value, "prefix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Prefix = upgrade;
						} else {
							fprintf(stderr, "Item prefix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "suffix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Suffix = upgrade;
						} else {
							fprintf(stderr, "Item suffix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "spell")) {
						std::string spell_ident = LuaToString(l, -1, k + 1);
						CSpell *spell = CSpell::GetSpell(spell_ident);
						if (spell != nullptr) {
							item->Spell = const_cast<CSpell *>(&(*spell));
						} else {
							fprintf(stderr, "Spell \"%s\" doesn't exist.", spell_ident.c_str());
						}
					} else if (!strcmp(value, "work")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Work = upgrade;
						} else {
							fprintf(stderr, "Literary work \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "elixir")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Elixir = upgrade;
						} else {
							fprintf(stderr, "Elixir \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "name")) {
						item->Name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "unique")) {
						std::string unique_ident = LuaToString(l, -1, k + 1);
						CUniqueItem *unique_item = GetUniqueItem(unique_ident);
						item->Unique = unique_item;
						if (unique_item != nullptr) {
							item->Name = unique_item->Name;
							if (unique_item->Type != nullptr) {
								item->Type = unique_item->Type;
							} else {
								fprintf(stderr, "Unique item \"%s\" has no type.\n", unique_item->Ident.c_str());
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
			character->ForbiddenUpgrades.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string unit_type_ident = LuaToString(l, -1, j + 1);
				CUnitType *unit_type = CUnitType::get(unit_type_ident);
				character->ForbiddenUpgrades.push_back(unit_type);
			}
		} else if (!strcmp(value, "HistoricalFactions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string historical_faction_name = LuaToString(l, -1, j + 1);
				CFaction *historical_faction = PlayerRaces.GetFaction(historical_faction_name);
				if (!historical_faction) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ historical_faction_name.c_str());
				}
				
				character->HistoricalFactions.push_back(std::pair<CDate, CFaction *>(date, historical_faction));
			}
		} else if (!strcmp(value, "HistoricalLocations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CHistoricalLocation *historical_location = new CHistoricalLocation;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &historical_location->Date);
				lua_pop(l, 1);
				++j;
				
				historical_location->MapTemplate = CMapTemplate::get(LuaToString(l, -1, j + 1));
				++j;
				
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) { //coordinates
					CclGetPos(l, &historical_location->Position.x, &historical_location->Position.y);
				} else { //site ident
					std::string site_ident = LuaToString(l, -1);
					historical_location->Site = CSite::GetSite(site_ident);
					if (!historical_location->Site) {
						LuaError(l, "Site \"%s\" doesn't exist.\n" _C_ site_ident.c_str());
					}
					historical_location->MapTemplate = historical_location->Site->MapTemplate;
					historical_location->Position = historical_location->Site->Position;
				}
				lua_pop(l, 1);

				character->HistoricalLocations.push_back(historical_location);
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
				CDate start_date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &start_date);
				lua_pop(l, 1);
				++j;
				CDate end_date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &end_date);
				lua_pop(l, 1);
				++j;
				
				std::string title_faction_name = LuaToString(l, -1, j + 1);
				CFaction *title_faction = PlayerRaces.GetFaction(title_faction_name);
				if (!title_faction) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ title_faction_name.c_str());
				}
				if (start_date.Year != 0 && end_date.Year != 0 && IsMinisterialTitle(title)) { // don't put in the faction's historical data if a blank year was given
					title_faction->HistoricalMinisters[std::tuple<CDate, CDate, int>(start_date, end_date, title)] = character;
				}
				character->HistoricalTitles.push_back(std::tuple<CDate, CDate, CFaction *, int>(start_date, end_date, title_faction, title));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (!redefinition) {
		if (character->Type->BoolFlag[FAUNA_INDEX].value) {
			character->Type->PersonalNames[character->Gender].push_back(character->Name);
			for (size_t i = 0; i < alternate_names.size(); ++i) {
				character->Type->PersonalNames[character->Gender].push_back(alternate_names[i]);
			}
		} else if (character->civilization) {
			character->civilization->PersonalNames[character->Gender].push_back(character->Name);
			for (size_t i = 0; i < alternate_names.size(); ++i) {
				character->civilization->PersonalNames[character->Gender].push_back(alternate_names[i]);
			}
		}
	}
	
	if (character->Trait == nullptr) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (character->Type != nullptr && character->Type->Traits.size() == 1) {
			character->Trait = character->Type->Traits[0];
		}
	}
	
	if (character->Gender == NoGender) { //if no gender was set, have the character be the same gender as the unit type (if the unit type has it predefined)
		if (character->Type != nullptr && character->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
			character->Gender = character->Type->DefaultStat.Variables[GENDER_INDEX].Value;
		}
	}
	
	//check if the abilities are correct for this character's unit type
	if (character->Type != nullptr && character->Abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > character->Type->Slot) {
		int ability_count = (int) character->Abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (std::find(AiHelpers.LearnableAbilities[character->Type->Slot].begin(), AiHelpers.LearnableAbilities[character->Type->Slot].end(), character->Abilities[i]) == AiHelpers.LearnableAbilities[character->Type->Slot].end()) {
				character->Abilities.erase(std::remove(character->Abilities.begin(), character->Abilities.end(), character->Abilities[i]), character->Abilities.end());
			}
		}
	}

	character->GenerateMissingDates();
	character->UpdateAttributes();
	
	character->Initialized = true;
	
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
			CUnitType *unit_type = CUnitType::get(unit_type_ident);
			hero->Type = unit_type;
			if (hero->Level < hero->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
				hero->Level = hero->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(trait_ident);
			hero->Trait = upgrade;
		} else if (!strcmp(value, "Civilization")) {
			hero->civilization = stratagus::civilization::get(LuaToString(l, -1));
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
				CUpgrade *ability = CUpgrade::try_get(ability_ident);
				if (ability != nullptr) {
					hero->Abilities.push_back(ability);
				} else {
					fprintf(stderr, "Ability \"%s\" doesn't exist.", ability_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Deities")) {
			hero->Deities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string deity_ident = LuaToString(l, -1, j + 1);
				CDeity *deity = CDeity::GetDeity(deity_ident);
				if (deity) {
					hero->Deities.push_back(deity);
				}
			}
		} else if (!strcmp(value, "ReadWorks")) {
			hero->ReadWorks.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string work_ident = LuaToString(l, -1, j + 1);
				CUpgrade *work = CUpgrade::try_get(work_ident);
				if (work != nullptr) {
					hero->ReadWorks.push_back(work);
				} else {
					fprintf(stderr, "Work \"%s\" doesn't exist.", work_ident.c_str());
				}
			}
		} else if (!strcmp(value, "ConsumedElixirs")) {
			hero->ConsumedElixirs.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string elixir_ident = LuaToString(l, -1, j + 1);
				CUpgrade *elixir = CUpgrade::try_get(elixir_ident);
				if (elixir != nullptr) {
					hero->ConsumedElixirs.push_back(elixir);
				} else {
					fprintf(stderr, "Elixir \"%s\" doesn't exist.", elixir_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Items")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CPersistentItem *item = new CPersistentItem;
				item->Owner = hero;
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
						CUnitType *item_type = CUnitType::try_get(item_ident);
						if (item_type != nullptr) {
							item->Type = item_type;
						} else {
							fprintf(stderr, "Item type \"%s\" doesn't exist.\n", item_ident.c_str());
							hero->Items.erase(std::remove(hero->Items.begin(), hero->Items.end(), item), hero->Items.end());
							delete item;
							break;
						}
					} else if (!strcmp(value, "prefix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Prefix = upgrade;
						} else {
							fprintf(stderr, "Item prefix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "suffix")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Suffix = upgrade;
						} else {
							fprintf(stderr, "Item suffix \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "spell")) {
						std::string spell_ident = LuaToString(l, -1, k + 1);
						CSpell *spell = CSpell::GetSpell(spell_ident);
						if (spell != nullptr) {
							item->Spell = const_cast<CSpell *>(&(*spell));
						} else {
							fprintf(stderr, "Spell \"%s\" doesn't exist.", spell_ident.c_str());
						}
					} else if (!strcmp(value, "work")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Work = upgrade;
						} else {
							fprintf(stderr, "Literary work \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "elixir")) {
						std::string upgrade_ident = LuaToString(l, -1, k + 1);
						CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							item->Elixir = upgrade;
						} else {
							fprintf(stderr, "Elixir \"%s\" doesn't exist.", upgrade_ident.c_str());
						}
					} else if (!strcmp(value, "name")) {
						item->Name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "unique")) {
						std::string unique_ident = LuaToString(l, -1, k + 1);
						CUniqueItem *unique_item = GetUniqueItem(unique_ident);
						item->Unique = unique_item;
						if (unique_item != nullptr) {
							item->Name = unique_item->Name;
							if (unique_item->Type != nullptr) {
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
				if (GetQuest(quest_name) != nullptr) {
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
				if (GetQuest(quest_name) != nullptr) {
					hero->QuestsCompleted.push_back(GetQuest(quest_name));
				} else {
					LuaError(l, "Quest \"%s\" doesn't exist." _C_ quest_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForbiddenUpgrades")) {
			hero->ForbiddenUpgrades.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string unit_type_ident = LuaToString(l, -1, j + 1);
				CUnitType *unit_type = CUnitType::get(unit_type_ident);
				hero->ForbiddenUpgrades.push_back(unit_type);
			}
		} else if (!strcmp(value, "Icon")) {
			hero->Icon.Name = LuaToString(l, -1);
			hero->Icon.Icon = nullptr;
			hero->Icon.Load();
		} else if (!strcmp(value, "HeroicIcon")) {
			hero->HeroicIcon.Name = LuaToString(l, -1);
			hero->HeroicIcon.Icon = nullptr;
			hero->HeroicIcon.Load();
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (hero->Gender == NoGender) { //if no gender was set, have the hero be the same gender as the unit type (if the unit type has it predefined)
		if (hero->Type != nullptr && hero->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
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
	CCharacter *character = CCharacter::GetCharacter(character_name);
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
		if (character->civilization) {
			lua_pushstring(l, character->civilization->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (character->Faction != nullptr) {
			lua_pushstring(l, character->Faction->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "BirthDate")) {
		if (character->BirthDate.Year != 0) {
			lua_pushstring(l, character->BirthDate.ToDisplayString(character->GetCalendar()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "BirthYear")) {
		lua_pushnumber(l, character->BirthDate.Year);
		return 1;
	} else if (!strcmp(data, "StartDate")) {
		if (character->StartDate.Year != 0) {
			lua_pushstring(l, character->StartDate.ToDisplayString(character->GetCalendar()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "StartYear")) {
		lua_pushnumber(l, character->StartDate.Year);
		return 1;
	} else if (!strcmp(data, "DeathDate")) {
		if (character->DeathDate.Year != 0) {
			lua_pushstring(l, character->DeathDate.ToDisplayString(character->GetCalendar()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DeathYear")) {
		lua_pushnumber(l, character->DeathDate.Year);
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
		if (character->Type != nullptr) {
			lua_pushstring(l, character->Type->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Trait")) {
		if (character->Trait != nullptr) {
			lua_pushstring(l, character->Trait->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Deity")) {
		if (character->Deity != nullptr) {
			lua_pushstring(l, character->Deity->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Deities")) {
		lua_createtable(l, character->Deities.size(), 0);
		for (size_t i = 1; i <= character->Deities.size(); ++i)
		{
			lua_pushstring(l, character->Deities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Father")) {
		if (character->Father != nullptr) {
			lua_pushstring(l, character->Father->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Mother")) {
		if (character->Mother != nullptr) {
			lua_pushstring(l, character->Mother->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Children")) {
		lua_createtable(l, character->Children.size(), 0);
		for (size_t i = 1; i <= character->Children.size(); ++i)
		{
			lua_pushstring(l, character->Children[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Siblings")) {
		lua_createtable(l, character->Siblings.size(), 0);
		for (size_t i = 1; i <= character->Siblings.size(); ++i)
		{
			lua_pushstring(l, character->Siblings[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
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
	} else if (!strcmp(data, "IsUsable")) {
		lua_pushboolean(l, character->IsUsable());
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, character->Abilities.size(), 0);
		for (size_t i = 1; i <= character->Abilities.size(); ++i)
		{
			lua_pushstring(l, character->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
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
		if (character->civilization) {
			lua_pushstring(l, character->civilization->get_identifier().c_str());
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
		if (character->Type != nullptr) {
			lua_pushstring(l, character->Type->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Trait")) {
		if (character->Trait != nullptr) {
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
	for (const CCharacter *character : CCharacter::Characters) {
		character_names.push_back(character->Ident);
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

/**
**  Parse character temporary data
**
**  @param l  Lua state.
*/
static int CclCharacter(lua_State *l)
{
	const std::string ident = LuaToString(l, 1);

	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	CCharacter *character = CCharacter::GetCharacter(ident);
	if (!character) {
		return 0;
	}

	// Parse the list:
	const int args = lua_rawlen(l, 2);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, 2, j + 1);
		++j;

		if (!strcmp(value, "deity")) {
			CDeity *deity = CDeity::GetDeity(LuaToString(l, 2, j + 1));
			if (deity) {
				character->Deities.push_back(deity);
			}
		} else {
			fprintf(stderr, "Character: Unsupported tag: %s\n", value);
		}
	}

	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for characters.
*/
void CharacterCclRegister()
{
	lua_register(Lua, "DefineCharacter", CclDefineCharacter);
	lua_register(Lua, "DefineCustomHero", CclDefineCustomHero);
	lua_register(Lua, "GetCharacterData", CclGetCharacterData);
	lua_register(Lua, "GetCustomHeroData", CclGetCustomHeroData);
	lua_register(Lua, "GetCharacters", CclGetCharacters);
	lua_register(Lua, "GetCustomHeroes", CclGetCustomHeroes);
	lua_register(Lua, "GetGrandStrategyHeroes", CclGetGrandStrategyHeroes);
	lua_register(Lua, "Character", CclCharacter);
}
