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
//      (c) Copyright 2015-2022 by Andrettin
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

#include "character.h"

#include "ai/ai_local.h" //for using AiHelpers
#include "character_title.h"
#include "gender.h"
#include "grand_strategy.h"
#include "item/persistent_item.h"
#include "item/unique_item.h"
#include "map/historical_location.h"
#include "map/map_template.h"
#include "map/site.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "province.h"
#include "religion/deity.h"
#include "script.h"
#include "spell/spell.h"
#include "time/timeline.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/log_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

static int CclDefineCharacter(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string character_ident = LuaToString(l, 1);
	wyrmgus::character *character = wyrmgus::character::try_get(character_ident);
	bool redefinition = false;
	if (!character) {
		if (LoadingPersistentHeroes) {
			wyrmgus::log::log_error("Character \"" + character_ident + "\" has persistent data, but doesn't exist.");
			return 0;
		}
		character = wyrmgus::character::get_or_add(character_ident, nullptr);
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
			character->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "AlternateNames")) { // alternate names the character may have, used for building the civilization's personal names
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				alternate_names.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ExtraName")) {
			character->ExtraName = LuaToString(l, -1);
		} else if (!strcmp(value, "FamilyName")) {
			character->surname = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			character->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			character->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			character->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Variation")) {
			character->variation = LuaToString(l, -1);
		} else if (!strcmp(value, "HairVariation")) {
			character->variation = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
			if (character->get_unit_type() == nullptr || character->get_unit_type() == unit_type || character->get_unit_type()->CanExperienceUpgradeTo(unit_type)) {
				character->unit_type = unit_type;
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(trait_ident);
			character->trait = upgrade;
		} else if (!strcmp(value, "BirthDate")) {
			CclGetDate(l, &character->BirthDate);
		} else if (!strcmp(value, "StartDate")) {
			CclGetDate(l, &character->StartDate);
		} else if (!strcmp(value, "DeathDate")) {
			CclGetDate(l, &character->DeathDate);
		} else if (!strcmp(value, "Civilization")) {
			character->civilization = wyrmgus::civilization::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Faction")) {
			wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1));
			character->default_faction = faction;
		} else if (!strcmp(value, "Father")) {
			const std::string father_ident = LuaToString(l, -1);
			wyrmgus::character *father = wyrmgus::character::get(father_ident);
			character->father = father;
		} else if (!strcmp(value, "Mother")) {
			const std::string mother_ident = LuaToString(l, -1);
			wyrmgus::character *mother = wyrmgus::character::get(mother_ident);
			character->mother = mother;
		} else if (!strcmp(value, "Children")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string child_ident = LuaToString(l, -1, j + 1);
				wyrmgus::character *child = wyrmgus::character::get(child_ident);
				character->add_child(child);
			}
		} else if (!strcmp(value, "Gender")) {
			character->gender = wyrmgus::string_to_gender(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			character->icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "HeroicIcon")) {
			character->heroic_icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Level")) {
			character->level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ExperiencePercent")) {
			character->ExperiencePercent = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Conditions")) {
			character->Conditions = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "Abilities")) {
			character->abilities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string ability_ident = LuaToString(l, -1, j + 1);
				CUpgrade *ability = CUpgrade::try_get(ability_ident);
				if (ability != nullptr) {
					character->abilities.push_back(ability);
				} else {
					fprintf(stderr, "Ability \"%s\" doesn't exist.", ability_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Deities")) {
			character->Deities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string deity_ident = LuaToString(l, -1, j + 1);
				wyrmgus::deity *deity = wyrmgus::deity::get(deity_ident);
				character->Deities.push_back(deity);
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
			character->items.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				std::unique_ptr<wyrmgus::persistent_item> item;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for items)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "type")) {
						const std::string item_ident = LuaToString(l, -1, k + 1);
						wyrmgus::unit_type *item_type = wyrmgus::unit_type::try_get(item_ident);
						if (item_type != nullptr) {
							item = std::make_unique<wyrmgus::persistent_item>(item_type, character);
						} else {
							fprintf(stderr, "Item type \"%s\" doesn't exist.\n", item_ident.c_str());
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
						wyrmgus::spell *spell = wyrmgus::spell::get(spell_ident);
						item->Spell = spell;
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
						item->name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "unique")) {
						std::string unique_ident = LuaToString(l, -1, k + 1);
						wyrmgus::unique_item *unique_item = wyrmgus::unique_item::try_get(unique_ident);
						item->unique = unique_item;
						if (unique_item == nullptr) {
							fprintf(stderr, "Unique item \"%s\" doesn't exist.\n", unique_ident.c_str());
						}
					} else if (!strcmp(value, "bound")) {
						item->bound = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "identified")) {
						item->identified = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "equipped")) {
						item->equipped = LuaToBoolean(l, -1, k + 1);
					} else {
						printf("\n%s\n", character->Ident.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);

				if (item != nullptr) {
					character->add_item(std::move(item));
				}
			}
		} else if (!strcmp(value, "ForbiddenUpgrades")) {
			character->ForbiddenUpgrades.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string unit_type_ident = LuaToString(l, -1, j + 1);
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
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
				wyrmgus::faction *historical_faction = wyrmgus::faction::get(historical_faction_name);

				character->HistoricalFactions.push_back(std::pair<CDate, wyrmgus::faction *>(date, historical_faction));
			}
		} else if (!strcmp(value, "HistoricalLocations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				auto location = std::make_unique<wyrmgus::historical_location>();
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &location->Date);
				lua_pop(l, 1);
				++j;
				
				location->map_template = wyrmgus::map_template::get(LuaToString(l, -1, j + 1));
				++j;
				
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) { //coordinates
					Vec2i pos;
					CclGetPos(l, &pos.x, &pos.y);
					location->pos = pos;
				} else { //site ident
					std::string site_ident = LuaToString(l, -1);
					location->site = wyrmgus::site::get(site_ident);
					location->map_template = location->site->get_map_template();
					location->pos = location->site->get_pos();
				}
				lua_pop(l, 1);

				character->HistoricalLocations.push_back(std::move(location));
			}
		} else if (!strcmp(value, "HistoricalTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const wyrmgus::character_title title = string_to_character_title(LuaToString(l, -1, j + 1));
				if (title == wyrmgus::character_title::none) {
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
				wyrmgus::faction *title_faction = wyrmgus::faction::get(title_faction_name);

				if (start_date.Year != 0 && end_date.Year != 0) { // don't put in the faction's historical data if a blank year was given
					title_faction->HistoricalMinisters[std::make_tuple(start_date, end_date, title)] = character;
				}
				character->HistoricalTitles.push_back(std::make_tuple(start_date, end_date, title_faction, title));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclDefineCustomHero(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string hero_ident = LuaToString(l, 1);
	string::replace(hero_ident, '-', '_');
	character *hero = character::get_custom_hero(hero_ident);
	if (hero == nullptr) {
		auto new_hero = make_qunique<character>(hero_ident);
		new_hero->moveToThread(QApplication::instance()->thread());
		hero = new_hero.get();
		character::custom_heroes_by_identifier[hero_ident] = std::move(new_hero);
		character::custom_heroes.push_back(hero);
	} else {
		fprintf(stderr, "Custom hero \"%s\" is being redefined.\n", hero_ident.c_str());
	}
	hero->custom = true;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			hero->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "ExtraName")) {
			hero->ExtraName = LuaToString(l, -1);
		} else if (!strcmp(value, "FamilyName")) {
			hero->surname = LuaToString(l, -1);
		} else if (!strcmp(value, "Dynasty")) { // for backwards compatibility
			hero->surname = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			hero->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Variation")) {
			hero->variation = LuaToString(l, -1);
		} else if (!strcmp(value, "HairVariation")) {
			hero->variation = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
			hero->unit_type = unit_type;
			if (hero->level < hero->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value) {
				hero->level = hero->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value;
			}
		} else if (!strcmp(value, "Trait")) {
			std::string trait_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(trait_ident);
			hero->trait = upgrade;
		} else if (!strcmp(value, "Civilization")) {
			hero->civilization = wyrmgus::civilization::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			hero->gender = wyrmgus::string_to_gender(LuaToString(l, -1));
		} else if (!strcmp(value, "Level")) {
			hero->level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ExperiencePercent")) {
			hero->ExperiencePercent = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Abilities")) {
			hero->abilities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string ability_ident = LuaToString(l, -1, j + 1);
				CUpgrade *ability = CUpgrade::try_get(ability_ident);
				if (ability != nullptr) {
					hero->abilities.push_back(ability);
				} else {
					fprintf(stderr, "Ability \"%s\" doesn't exist.", ability_ident.c_str());
				}
			}
		} else if (!strcmp(value, "Deities")) {
			hero->Deities.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string deity_ident = LuaToString(l, -1, j + 1);
				wyrmgus::deity *deity = wyrmgus::deity::get(deity_ident);
				hero->Deities.push_back(deity);
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
				std::unique_ptr<wyrmgus::persistent_item> item;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for items)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "type")) {
						const std::string item_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::unit_type *item_type = wyrmgus::unit_type::try_get(item_ident);
						if (item_type != nullptr) {
							item = std::make_unique<wyrmgus::persistent_item>(item_type, hero);
						} else {
							fprintf(stderr, "Item type \"%s\" doesn't exist.\n", item_ident.c_str());
							item.reset();
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
						wyrmgus::spell *spell = wyrmgus::spell::get(spell_ident);
						item->Spell = spell;
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
						item->name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "unique")) {
						std::string unique_ident = LuaToString(l, -1, k + 1);
						wyrmgus::unique_item *unique_item = wyrmgus::unique_item::try_get(unique_ident);
						item->unique = unique_item;
						if (unique_item == nullptr) {
							fprintf(stderr, "Unique item \"%s\" doesn't exist.\n", unique_ident.c_str());
						}
					} else if (!strcmp(value, "bound")) {
						item->bound = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "identified")) {
						item->identified = LuaToBoolean(l, -1, k + 1);
					} else if (!strcmp(value, "equipped")) {
						item->equipped = LuaToBoolean(l, -1, k + 1);
					} else {
						printf("\n%s\n", hero->Ident.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);

				if (item != nullptr) {
					hero->add_item(std::move(item));
				}
			}
		} else if (!strcmp(value, "ForbiddenUpgrades")) {
			hero->ForbiddenUpgrades.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string unit_type_ident = LuaToString(l, -1, j + 1);
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
				hero->ForbiddenUpgrades.push_back(unit_type);
			}
		} else if (!strcmp(value, "Icon")) {
			hero->icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "HeroicIcon")) {
			hero->heroic_icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (hero->get_gender() == wyrmgus::gender::none) { //if no gender was set, have the hero be the same gender as the unit type (if the unit type has it predefined)
		if (hero->get_unit_type() != nullptr && hero->get_unit_type()->get_gender() != wyrmgus::gender::none) {
			hero->gender = hero->get_unit_type()->get_gender();
		}
	}
	
	//check if the abilities are correct for this hero's unit type
	if (hero->abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > hero->get_unit_type()->Slot) {
		int ability_count = (int) hero->abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (std::find(AiHelpers.LearnableAbilities[hero->get_unit_type()->Slot].begin(), AiHelpers.LearnableAbilities[hero->get_unit_type()->Slot].end(), hero->abilities[i]) == AiHelpers.LearnableAbilities[hero->get_unit_type()->Slot].end()) {
				hero->abilities.erase(std::remove(hero->abilities.begin(), hero->abilities.end(), hero->abilities[i]), hero->abilities.end());
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

	const std::string identifier = LuaToString(l, 1);
	const character *character = character::try_get(identifier);

	if (character == nullptr) {
		log::log_error("Failed to get character data: \"" + identifier + "\" is not a valid character.");
		lua_pushnil(l);
		return 0;
	}

	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, character->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "FamilyName")) {
		lua_pushstring(l, character->get_surname().c_str());
		return 1;
	} else if (!strcmp(data, "FullName")) {
		lua_pushstring(l, character->get_full_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, character->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, character->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, character->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (character->get_civilization()) {
			lua_pushstring(l, character->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (character->get_default_faction() != nullptr) {
			lua_pushstring(l, character->get_default_faction()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "BirthDate")) {
		if (character->BirthDate.Year != 0) {
			lua_pushstring(l, character->BirthDate.ToDisplayString(character->get_calendar()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "BirthYear")) {
		lua_pushnumber(l, character->BirthDate.Year);
		return 1;
	} else if (!strcmp(data, "StartDate")) {
		if (character->StartDate.Year != 0) {
			lua_pushstring(l, character->StartDate.ToDisplayString(character->get_calendar()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "StartYear")) {
		lua_pushnumber(l, character->StartDate.Year);
		return 1;
	} else if (!strcmp(data, "DeathDate")) {
		if (character->DeathDate.Year != 0) {
			lua_pushstring(l, character->DeathDate.ToDisplayString(character->get_calendar()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DeathYear")) {
		lua_pushnumber(l, character->DeathDate.Year);
		return 1;
	} else if (!strcmp(data, "Gender")) {
		lua_pushstring(l, wyrmgus::gender_to_string(character->get_gender()).c_str());
		return 1;
	} else if (!strcmp(data, "Level")) {
		lua_pushnumber(l, character->get_level());
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (character->get_unit_type() != nullptr) {
			lua_pushstring(l, character->get_unit_type()->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Trait")) {
		if (character->get_trait() != nullptr) {
			lua_pushstring(l, character->get_trait()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Deity")) {
		if (character->get_deity() != nullptr) {
			lua_pushstring(l, character->get_deity()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Deities")) {
		lua_createtable(l, character->Deities.size(), 0);
		for (size_t i = 1; i <= character->Deities.size(); ++i)
		{
			lua_pushstring(l, character->Deities[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Father")) {
		if (character->get_father() != nullptr) {
			lua_pushstring(l, character->get_father()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Mother")) {
		if (character->get_mother() != nullptr) {
			lua_pushstring(l, character->get_mother()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Children")) {
		lua_createtable(l, character->get_children().size(), 0);
		for (size_t i = 1; i <= character->get_children().size(); ++i)
		{
			lua_pushstring(l, character->get_children()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (character->get_icon() != nullptr) {
			lua_pushstring(l, character->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "HairVariation")) {
		lua_pushstring(l, character->get_variation().c_str());
		return 1;
	} else if (!strcmp(data, "IsUsable")) {
		lua_pushboolean(l, character->IsUsable());
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, character->get_abilities().size(), 0);
		for (size_t i = 1; i <= character->get_abilities().size(); ++i)
		{
			lua_pushstring(l, character->get_abilities()[i-1]->get_identifier().c_str());
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
	character *character = character::get_custom_hero(character_name);
	if (!character) {
		LuaError(l, "Custom hero \"%s\" doesn't exist." _C_ character_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, character->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "FamilyName")) {
		lua_pushstring(l, character->get_surname().c_str());
		return 1;
	} else if (!strcmp(data, "FullName")) {
		lua_pushstring(l, character->get_full_name().c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (character->get_civilization() != nullptr) {
			lua_pushstring(l, character->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Gender")) {
		lua_pushstring(l, wyrmgus::gender_to_string(character->get_gender()).c_str());
		return 1;
	} else if (!strcmp(data, "Level")) {
		lua_pushnumber(l, character->get_level());
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (character->get_unit_type() != nullptr) {
			lua_pushstring(l, character->get_unit_type()->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Trait")) {
		if (character->get_trait() != nullptr) {
			lua_pushstring(l, character->get_trait()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (character->get_icon() != nullptr) {
			lua_pushstring(l, character->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetCharacters(lua_State *l)
{
	std::vector<std::string> character_names;
	for (const character *character : character::get_all()) {
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
	for (const character *hero : character::get_custom_heroes()) {
		character_names.push_back(hero->get_identifier());
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
		lua_pushstring(l, GrandStrategyGame.Heroes[i-1]->get_full_name().c_str());
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

	wyrmgus::character *character = wyrmgus::character::try_get(ident);
	if (!character) {
		return 0;
	}

	// Parse the list:
	const int args = lua_rawlen(l, 2);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, 2, j + 1);
		++j;

		if (!strcmp(value, "deity")) {
			wyrmgus::deity *deity = wyrmgus::deity::get(LuaToString(l, 2, j + 1));
			character->Deities.push_back(deity);
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
