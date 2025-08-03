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
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski, Jimmy Salmon
//		and Andrettin
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

#include "upgrade/upgrade.h"

//Wyrmgus start
#include "action/action_build.h"
//Wyrmgus end
#include "action/action_train.h"
//Wyrmgus start
#include "action/action_upgradeto.h"
//Wyrmgus end
#include "ai/ai_local.h"
#include "age.h"
#include "character.h"
#include "commands.h"
#include "database/defines.h"
//Wyrmgus start
#include "editor.h"
#include "game/game.h"
//Wyrmgus end
#include "include/config.h"
#include "iolib.h"
#include "item/item_class.h"
#include "magic_domain.h"
#include "map/map.h"
#include "map/site.h"
#include "map/site_game_data.h"
//Wyrmgus start
#include "network/network.h"
//Wyrmgus end
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/government_type.h"
#include "player/player.h"
#include "religion/deity.h"
#include "script.h"
#include "script/condition/and_condition.h"
#include "script/factor.h"
//Wyrmgus start
#include "settings.h"
#include "translator.h"
//Wyrmgus end
#include "unit/unit.h"
#include "unit/unit_find.h"
//Wyrmgus start
#include "ui/interface.h"
#include "ui/ui.h"
//Wyrmgus end
#include "unit/unit_class.h"
//Wyrmgus start
#include "unit/unit_manager.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade_category.h"
#include "upgrade/upgrade_category_rank.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_modifier.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/string_util.h"
#include "util/util.h"
#include "util/vector_util.h"

bool CUpgrade::compare_encyclopedia_entries(const CUpgrade *lhs, const CUpgrade *rhs)
{
	std::string lhs_civilization_name;
	if (lhs->get_civilization() != nullptr && lhs->get_civilization() != defines::get()->get_neutral_civilization()) {
		lhs_civilization_name = lhs->get_civilization()->get_name();
	} else if (lhs->get_civilization_group() != nullptr) {
		lhs_civilization_name = lhs->get_civilization_group()->get_name();
	}

	std::string rhs_civilization_name;
	if (rhs->get_civilization() != nullptr && rhs->get_civilization() != defines::get()->get_neutral_civilization()) {
		rhs_civilization_name = rhs->get_civilization()->get_name();
	} else if (rhs->get_civilization_group() != nullptr) {
		rhs_civilization_name = rhs->get_civilization_group()->get_name();
	}

	if (lhs_civilization_name != rhs_civilization_name) {
		return lhs_civilization_name < rhs_civilization_name;
	}

	if (lhs->get_faction() != rhs->get_faction()) {
		if (lhs->get_faction() == nullptr || rhs->get_faction() == nullptr) {
			return lhs->get_faction() == nullptr;
		}

		return lhs->get_faction()->get_name() < rhs->get_faction()->get_name();
	}

	const wyrmgus::upgrade_class *lhs_class = lhs->get_upgrade_class();
	const wyrmgus::upgrade_class *rhs_class = rhs->get_upgrade_class();

	if (lhs_class != rhs_class) {
		if (lhs_class == nullptr || rhs_class == nullptr) {
			return lhs_class != nullptr;
		}

		for (int i = static_cast<int>(upgrade_category_rank::count) - 1; i > static_cast<int>(upgrade_category_rank::none); --i) {
			const upgrade_category_rank rank = static_cast<upgrade_category_rank>(i);

			const upgrade_category *lhs_category = lhs_class->get_category(rank);
			const upgrade_category *rhs_category = rhs_class->get_category(rank);
			if (lhs_category != rhs_category) {
				if (lhs_category == nullptr || rhs_category == nullptr) {
					return lhs_category != nullptr;
				}

				if (lhs_category->get_start_age() != rhs_category->get_start_age() && lhs_category->get_start_age()->get_priority() != rhs_category->get_start_age()->get_priority()) {
					return lhs_category->get_start_age()->get_priority() < rhs_category->get_start_age()->get_priority();
				}

				return lhs_category->get_name() < rhs_category->get_name();
			}
		}

		if (lhs_class->get_age() != rhs_class->get_age() && lhs_class->get_age()->get_priority() != rhs_class->get_age()->get_priority()) {
			return lhs_class->get_age()->get_priority() < rhs_class->get_age()->get_priority();
		}
	}

	if (lhs->get_item() != rhs->get_item()) {
		if (lhs->get_item() == nullptr || rhs->get_item() == nullptr) {
			return lhs->get_item() != nullptr;
		}

		return unit_type::compare_encyclopedia_entries(lhs->get_item(), rhs->get_item());
	}

	if (lhs->get_price() != rhs->get_price()) {
		return lhs->get_price() < rhs->get_price();
	}

	return named_data_entry::compare_encyclopedia_entries(lhs, rhs);
}

CUpgrade::CUpgrade(const std::string &identifier)
	: detailed_data_entry(identifier), Work(item_class::none), government_type(government_type::none)
{
}

CUpgrade::~CUpgrade()
{
}

void CUpgrade::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "parent") {
		CUpgrade *parent_upgrade = CUpgrade::get(value);
		this->set_parent(parent_upgrade);
	} else if (key == "button_key") {
		this->button_key = value;
	} else {
		data_entry::process_gsml_property(property);
	}
}

void CUpgrade::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->costs[resource] = std::stoi(value);
		});
	} else if (tag == "scaled_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->scaled_costs[resource] = std::stoi(value);
		});
	} else if (tag == "scaled_cost_unit_types") {
		for (const std::string &value : values) {
			this->scaled_cost_unit_types.push_back(unit_type::get(value));
		}
	} else if (tag == "scaled_cost_unit_classes") {
		for (const std::string &value : values) {
			this->scaled_cost_unit_classes.push_back(unit_class::get(value));
		}
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<upgrade_modifier>(this);

		scope.process(modifier.get());

		upgrade_modifier::UpgradeModifiers.push_back(modifier.get());
		this->modifiers.push_back(std::move(modifier));
	} else if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition<CPlayer>>();
		scope.process(this->preconditions.get());
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<CPlayer>>();
		scope.process(this->conditions.get());
	} else if (tag == "unit_preconditions") {
		this->unit_preconditions = std::make_unique<and_condition<CUnit>>();
		scope.process(this->unit_preconditions.get());
	} else if (tag == "unit_conditions") {
		this->unit_conditions = std::make_unique<and_condition<CUnit>>();
		scope.process(this->unit_conditions.get());
	} else if (tag == "ai_priority") {
		this->ai_priority = std::make_unique<factor<CPlayer>>();
		scope.process(this->ai_priority.get());
	} else if (tag == "affixed_item_classes") {
		for (const std::string &value : values) {
			this->affixed_item_classes.insert(enum_converter<item_class>::to_enum(value));
		}
	} else if (tag == "incompatible_affixes") {
		for (const std::string &value : values) {
			CUpgrade *other_upgrade = CUpgrade::get(value);
			this->incompatible_affixes.insert(other_upgrade);

			//add the upgrade to the incompatible affix's counterpart list here
			other_upgrade->incompatible_affixes.insert(this);
		}
	}
}

void CUpgrade::initialize()
{
	if (this->get_icon() == nullptr) {
		if (this->get_dynasty() != nullptr) {
			this->icon = this->get_dynasty()->get_icon();
		} else if (this->get_deity() != nullptr) {
			this->icon = this->get_deity()->get_icon();
		}
	}

	if (this->deity_domain != nullptr) {
		if (this->get_name().empty()) {
			this->set_name(this->deity_domain->get_name() + " Deity Domain");
		}

		if (this->get_description().empty()) {
			this->set_description(this->deity_domain->get_description());
		}

		if (this->get_background().empty()) {
			this->set_background(this->deity_domain->get_background());
		}

		if (this->get_quote().empty()) {
			this->set_quote(this->deity_domain->get_quote());
		}
	}

	if (this->get_civilization() != nullptr && this->get_civilization_group() != nullptr) {
		throw std::runtime_error("Upgrade \"" + this->get_identifier() + "\" has both a civilization and a civilization group.");
	}

	if (this->get_upgrade_class() != nullptr) { //if class is defined, then use this upgrade to help build the classes table, and add this upgrade to the civilization class table (if the civilization is defined)
		const wyrmgus::upgrade_class *upgrade_class = this->get_upgrade_class();
		if (this->get_faction() != nullptr) {
			this->get_faction()->set_class_upgrade(upgrade_class, this);
		} else if (this->get_civilization() != nullptr) {
			this->get_civilization()->set_class_upgrade(upgrade_class, this);
		} else if (this->civilization_group != nullptr) {
			this->civilization_group->set_class_upgrade(upgrade_class, this);
		}
	}

	if (this->get_government_type() != government_type::none) {
		CUpgrade::government_type_upgrades[this->get_government_type()] = this;
	}

	CclCommand("if not (GetArrayIncludes(Units, \"" + this->get_identifier() + "\")) then table.insert(Units, \"" + this->get_identifier() + "\") end"); //FIXME: needed at present to make upgrade data files work without scripting being necessary, but it isn't optimal to interact with a scripting table like "Units" in this manner (that table should probably be replaced with getting a list of unit types from the engine)

	data_entry::initialize();
}

void CUpgrade::check() const
{
	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->ai_priority != nullptr) {
		this->ai_priority->check();
	}
}

std::string CUpgrade::get_encyclopedia_text() const
{
	std::string text;

	if (this->get_civilization() != nullptr && this->get_civilization() != defines::get()->get_neutral_civilization()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Civilization: " + this->get_civilization()->get_link_string());
	} else if (this->get_civilization_group() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Civilization Group: " + this->get_civilization_group()->get_name());
	}

	if (this->get_faction() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Faction: " + this->get_faction()->get_link_string());
	}

	if (this->get_upgrade_class() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Class: " + this->get_upgrade_class()->get_name());
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	if (this->is_magic_affix()) {
		const std::string effects = GetUpgradeEffectsString(this);
		if (!effects.empty()) {
			named_data_entry::concatenate_encyclopedia_text(text, "Effects: " + effects);
		}

		std::vector<std::string> applies_to;

		for (const item_class item_class : this->get_affixed_item_classes()) {
			applies_to.push_back(string::get_plural_form(get_item_class_name(item_class)));
		}

		std::vector<const unit_type *> applies_to_unit_types;
		for (const auto &upgrade_modifier : this->get_modifiers()) {
			for (const unit_type *unit_type : upgrade_modifier->get_unit_types()) {
				applies_to_unit_types.push_back(unit_type);
			}
		}

		for (const unit_type *unit_type : unit_type::get_all()) {
			if (unit_type->is_template()) { //if is a template, continue
				continue;
			}

			if (vector::contains(unit_type->get_affixes(), this)) {
				applies_to_unit_types.push_back(unit_type);
			}
		}

		for (const unit_type *unit_type : applies_to_unit_types) {
			std::string unit_type_str = string::get_plural_form(unit_type->get_name());

			if ((unit_type->get_civilization() != nullptr && unit_type->get_civilization() != defines::get()->get_neutral_civilization()) || unit_type->get_civilization_group() != nullptr) {
				unit_type_str += " (";
				if (unit_type->get_civilization() != nullptr) {
					unit_type_str += unit_type->get_civilization()->get_name();
				} else {
					unit_type_str += unit_type->get_civilization_group()->get_name();
				}

				if (unit_type->get_faction() != nullptr) {
					unit_type_str += ": " + unit_type->get_faction()->get_name();
				}

				unit_type_str += ")";
			}

			applies_to.push_back(std::move(unit_type_str));
		}

		if (!applies_to.empty()) {
			std::sort(applies_to.begin(), applies_to.end());

			std::string applies_to_text;
			for (const std::string &str : applies_to) {
				if (!applies_to_text.empty()) {
					applies_to_text += ", ";
				}

				applies_to_text += str;
			}

			named_data_entry::concatenate_encyclopedia_text(text, "Available For: " + applies_to_text);
		}
	}

	if (this->get_conditions() != nullptr && this->is_magic_affix()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Conditions:\n" + this->get_conditions()->get_conditions_string(1, true));
	}

	return text;
}

std::string CUpgrade::get_link_string(const std::string &link_text, const bool highlight_as_fallback) const
{
	if (this->get_deity() != nullptr) {
		return this->get_deity()->get_link_string(link_text, highlight_as_fallback);
	}

	return data_type::get_link_string(link_text, highlight_as_fallback);
}

void CUpgrade::set_parent(const CUpgrade *parent_upgrade)
{
	if (!parent_upgrade->is_defined()) {
		throw std::runtime_error("Upgrade \"" + this->get_identifier() + "\" is inheriting features from non-defined parent \"" + parent_upgrade->get_identifier() + "\".");
	}

	if (this->get_name().empty()) {
		this->set_name(parent_upgrade->get_name());
	}
	this->icon = parent_upgrade->icon;
	this->upgrade_class = parent_upgrade->get_upgrade_class();
	this->set_description(parent_upgrade->get_description());
	this->set_quote(parent_upgrade->get_quote());
	this->set_background(parent_upgrade->get_background());
	this->effects_string = parent_upgrade->effects_string;
	this->requirements_string = parent_upgrade->get_requirements_string();
	this->costs = parent_upgrade->costs;
	this->scaled_costs = parent_upgrade->scaled_costs;
	this->GrandStrategyProductionEfficiencyModifier = parent_upgrade->GrandStrategyProductionEfficiencyModifier;
	this->affixed_item_classes = parent_upgrade->affixed_item_classes;
	this->MaxLimit = parent_upgrade->MaxLimit;
	this->magic_level = parent_upgrade->magic_level;
	this->ability = parent_upgrade->is_ability();
	this->weapon = parent_upgrade->is_weapon();
	this->shield = parent_upgrade->is_shield();
	this->boots = parent_upgrade->is_boots();
	this->arrows = parent_upgrade->is_arrows();
	this->item = parent_upgrade->item;
	this->magic_prefix = parent_upgrade->magic_prefix;
	this->magic_suffix = parent_upgrade->magic_suffix;
	this->RunicAffix = parent_upgrade->RunicAffix;
	this->Work = parent_upgrade->Work;
	this->UniqueOnly = parent_upgrade->UniqueOnly;
	this->scaled_cost_unit_types = parent_upgrade->scaled_cost_unit_types;
	this->scaled_cost_unit_classes = parent_upgrade->scaled_cost_unit_classes;

	for (const auto &modifier : parent_upgrade->get_modifiers()) {
		std::unique_ptr<upgrade_modifier> duplicated_modifier = modifier->duplicate(this);
		upgrade_modifier::UpgradeModifiers.push_back(duplicated_modifier.get());
		this->modifiers.push_back(std::move(duplicated_modifier));
	}
}

/**
**  Init upgrade/allow structures
*/
void InitUpgrades()
{
}

/**
**  Cleanup the upgrade module.
*/
void CleanUpgradeModifiers()
{
	upgrade_modifier::UpgradeModifiers.clear();
}

void CUpgrade::set_upgrade_class(wyrmgus::upgrade_class *upgrade_class)
{
	if (upgrade_class == this->get_upgrade_class()) {
		return;
	}

	if (this->get_upgrade_class() != nullptr) {
		this->get_upgrade_class()->remove_upgrade(this);
	}

	this->upgrade_class = upgrade_class;

	if (this->get_upgrade_class() != nullptr && !this->get_upgrade_class()->has_upgrade(this)) {
		this->get_upgrade_class()->add_upgrade(this);
	}
}

void CUpgrade::add_modifier(std::unique_ptr<const upgrade_modifier> &&modifier)
{
	this->modifiers.push_back(std::move(modifier));
}

int CUpgrade::get_time_cost() const
{
	return this->get_cost(defines::get()->get_time_resource());
}

int CUpgrade::get_price() const
{
	return resource::get_price(this->get_costs());
}

bool CUpgrade::check_unit_preconditions(const CUnit *unit) const
{
	if (this->unit_preconditions == nullptr) {
		return true;
	}

	return this->unit_preconditions->check(unit);
}

bool CUpgrade::check_unit_conditions(const CUnit *unit) const
{
	if (!this->check_unit_preconditions(unit)) {
		return false;
	}

	if (this->unit_conditions == nullptr) {
		return true;
	}

	return this->unit_conditions->check(unit);
}

int CUpgrade::calculate_ai_priority(const CPlayer *player) const
{
	if (this->ai_priority != nullptr) {
		return this->ai_priority->calculate(player);
	}

	return CUpgrade::default_ai_priority;
}

bool CUpgrade::check_drop_conditions(const CUnit *dropper, const CPlayer *dropper_player) const
{
	if (dropper != nullptr) {
		if (check_conditions(this, dropper)) {
			return true;
		}

		if (dropper->Type->BoolFlag[MARKET_INDEX].value && dropper_player != nullptr && !dropper_player->is_neutral_player()) {
			for (const CPlayer *trade_partner : dropper_player->get_recent_trade_partners()) {
				if (check_conditions(this, trade_partner)) {
					return true;
				}
			}
		}
	} else {
		if (check_conditions(this, dropper_player)) {
			return true;
		}
	}

	return false;
}

QString CUpgrade::get_upgrade_effects_qstring(bool multiline, unsigned indent) const
{
	return QString::fromStdString(GetUpgradeEffectsString(this, multiline, indent));
}

bool CUpgrade::has_researcher_for_civilization(const wyrmgus::civilization *civilization) const
{
	//check whether the upgrade has a valid researcher
	std::vector<const unit_type *> researchers = AiHelpers.get_researchers(this);

	const wyrmgus::upgrade_class *upgrade_class = this->get_upgrade_class();

	if (upgrade_class != nullptr) {
		for (const unit_class *researcher_class : AiHelpers.get_researcher_classes(upgrade_class)) {
			const unit_type *researcher = civilization->get_class_unit_type(researcher_class);

			if (researcher == nullptr) {
				continue;
			}

			researchers.push_back(researcher);
		}
	}

	for (const unit_type *researcher : researchers) {
		const unit_class *researcher_class = researcher->get_unit_class();

		if (researcher_class != nullptr) {
			if (researcher_class->get_preconditions() != nullptr && !researcher_class->get_preconditions()->check(civilization)) {
				continue;
			}

			if (researcher_class->get_conditions() != nullptr && !researcher_class->get_conditions()->check(civilization)) {
				continue;
			}
		}

		if (researcher->get_preconditions() != nullptr && !researcher->get_preconditions()->check(civilization)) {
			continue;
		}

		if (researcher->get_conditions() != nullptr && !researcher->get_conditions()->check(civilization)) {
			continue;
		}

		return true;
	}

	return false;
}

bool CUpgrade::is_available_for_civilization(const wyrmgus::civilization *civilization) const
{
	const wyrmgus::upgrade_class *upgrade_class = this->get_upgrade_class();

	if (upgrade_class != nullptr) {
		if (upgrade_class->get_preconditions() != nullptr && !upgrade_class->get_preconditions()->check(civilization)) {
			return false;
		}

		if (upgrade_class->get_conditions() != nullptr && !upgrade_class->get_conditions()->check(civilization)) {
			return false;
		}
	}

	if (this->get_preconditions() != nullptr && !this->get_preconditions()->check(civilization)) {
		return false;
	}

	if (this->get_conditions() != nullptr && !this->get_conditions()->check(civilization)) {
		return false;
	}

	if (!this->has_researcher_for_civilization(civilization)) {
		return false;
	}

	return true;
}

/**
**  Save state of the upgrade to file.
**
**  @param file  Output file.
*/
void SaveUpgrades(CFile &file)
{
	file.printf("\n-- -----------------------------------------\n");
	file.printf("-- MODULE: upgrades\n\n");

	//
	//  Save the allow
	//
	for (const unit_type *unit_type : unit_type::get_all()) {
		file.printf("DefineUnitAllow(\"%s\", ", unit_type->get_identifier().c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			if (p) {
				file.printf(", ");
			}
			file.printf("%d", CPlayer::Players[p]->Allow.Units[unit_type->Slot]);
		}
		file.printf(")\n");
	}
	file.printf("\n");

	//
	//  Save the upgrades
	//
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		file.printf("DefineAllow(\"%s\", \"", upgrade->get_identifier().c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			file.printf("%c", CPlayer::Players[p]->Allow.Upgrades[upgrade->ID]);
		}
		file.printf("\")\n");
	}
}

/*----------------------------------------------------------------------------
--  Ccl part of upgrades
----------------------------------------------------------------------------*/

//Wyrmgus start
/**
**  Define an upgrade.
**
**  @param l  Lua state.
*/
int CclDefineUpgrade(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string upgrade_ident = LuaToString(l, 1);
	CUpgrade *upgrade = CUpgrade::get_or_add(upgrade_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Parent")) {
			CUpgrade *parent_upgrade = CUpgrade::get(LuaToString(l, -1));
			upgrade->set_parent(parent_upgrade);
		} else if (!strcmp(value, "Name")) {
			upgrade->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			icon *icon = icon::get(LuaToString(l, -1));
			upgrade->icon = icon;
		} else if (!strcmp(value, "Class")) {
			upgrade->set_upgrade_class(upgrade_class::get(LuaToString(l, -1)));
		} else if (!strcmp(value, "Civilization")) {
			std::string civilization_name = LuaToString(l, -1);
			civilization *civilization = civilization::get(civilization_name);
			upgrade->civilization = civilization;
		} else if (!strcmp(value, "Faction")) {
			std::string faction_name = LuaToString(l, -1);
			faction *faction = faction::get(faction_name);
			upgrade->faction = faction;
		} else if (!strcmp(value, "Description")) {
			upgrade->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			upgrade->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			upgrade->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "EffectsString")) {
			upgrade->effects_string = LuaToString(l, -1);
		} else if (!strcmp(value, "RequirementsString")) {
			upgrade->requirements_string = LuaToString(l, -1);
		} else if (!strcmp(value, "MaxLimit")) {
			upgrade->MaxLimit = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MagicLevel")) {
			upgrade->magic_level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Year")) {
			upgrade->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Ability")) {
			upgrade->ability = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Weapon")) {
			upgrade->weapon = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Shield")) {
			upgrade->shield = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Boots")) {
			upgrade->boots = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Arrows")) {
			upgrade->arrows = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicPrefix")) {
			upgrade->magic_prefix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicSuffix")) {
			upgrade->magic_suffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RunicAffix")) {
			upgrade->RunicAffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "UniqueOnly")) {
			upgrade->UniqueOnly = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Work")) {
			upgrade->Work = enum_converter<item_class>::to_enum(LuaToString(l, -1));
		} else if (!strcmp(value, "Item")) {
			unit_type *item = unit_type::get(LuaToString(l, -1));
			upgrade->item = item;
		} else if (!strcmp(value, "Costs")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const resource *resource = resource::get(LuaToString(l, -1, j + 1));
				++j;
				
				upgrade->costs[resource] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ScaledCosts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const resource *resource = resource::get(LuaToString(l, -1, j + 1));
				++j;
				
				upgrade->scaled_costs[resource] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "GrandStrategyProductionEfficiencyModifier")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const resource *resource = resource::get(LuaToString(l, -1, j + 1));
				++j;
				
				upgrade->GrandStrategyProductionEfficiencyModifier[resource] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ItemAffix")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const item_class item_class = enum_converter<wyrmgus::item_class>::to_enum(LuaToString(l, -1, j + 1));
				
				upgrade->affixed_item_classes.insert(item_class);
			}
		} else if (!strcmp(value, "IncompatibleAffixes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUpgrade *affix = CUpgrade::get(LuaToString(l, -1, j + 1));
				upgrade->incompatible_affixes.insert(affix);

				affix->incompatible_affixes.insert(upgrade);
			}
		} else if (!strcmp(value, "ScaledCostUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const unit_type *scaled_cost_unit = unit_type::get(LuaToString(l, -1, j + 1));
				upgrade->scaled_cost_unit_types.push_back(scaled_cost_unit);
			}
		} else if (!strcmp(value, "WeaponClasses")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				upgrade->WeaponClasses.insert(enum_converter<item_class>::to_enum(LuaToString(l, -1, j + 1)));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	upgrade->set_defined(true);
	
	return 0;
}
//Wyrmgus end

/**
**  Define a new upgrade modifier.
**
**  @param l  List of modifiers.
*/
int CclDefineModifier(lua_State *l)
{
	const int args = lua_gettop(l);

	const std::string upgrade_ident = LuaToString(l, 1);
	CUpgrade *upgrade = CUpgrade::get(upgrade_ident);

	auto um = std::make_unique<upgrade_modifier>(upgrade);
	
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const char *key = LuaToString(l, j + 1, 1);
		if (!strcmp(key, "regeneration-rate")) {
			um->Modifier.Variables[HP_INDEX].Increase = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "cost")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const char *value = LuaToString(l, j + 1, 2);
			const resource *resource = resource::get(value);
			um->Modifier.set_cost(resource, LuaToNumber(l, j + 1, 3));
		} else if (!strcmp(key, "storing")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			const char *value = LuaToString(l, j + 1, 1);
			const resource *resource = resource::get(value);
			um->Modifier.set_storing(resource, LuaToNumber(l, j + 1, 2));
		} else if (!strcmp(key, "improve-production")) {
			const char *value = LuaToString(l, j + 1, 2);
			const resource *resource = resource::get(value);
			um->Modifier.set_improve_income(resource, LuaToNumber(l, j + 1, 3));
		//Wyrmgus start
		} else if (!strcmp(key, "resource-demand")) {
			const char *value = LuaToString(l, j + 1, 2);
			const resource *resource = resource::get(value);
			um->Modifier.set_resource_demand(resource, LuaToNumber(l, j + 1, 3));
		} else if (!strcmp(key, "unit-stock")) {
			std::string value = LuaToString(l, j + 1, 2);
			unit_type *unit_type = unit_type::get(value);
			um->Modifier.set_unit_stock(unit_type, LuaToNumber(l, j + 1, 3));
		//Wyrmgus end
		} else if (!strcmp(key, "allow-unit")) {
			const char *value = LuaToString(l, j + 1, 2);

			if (!strncmp(value, "unit", 4)) {
				um->ChangeUnits[UnitTypeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
			} else {
				LuaError(l, "unit expected");
			}
		} else if (!strcmp(key, "remove-upgrade")) {
			const char *value = LuaToString(l, j + 1, 2);
			CUpgrade *removed_upgrade = CUpgrade::get(value);
			um->removed_upgrades.push_back(removed_upgrade);
		//Wyrmgus end
		} else if (!strcmp(key, "apply-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->unit_types.push_back(unit_type::get(value));
		} else if (!strcmp(key, "convert-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->convert_to = unit_type::get(value);
		//Wyrmgus start
		} else if (!strcmp(key, "research-speed")) {
			um->SpeedResearch = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "change-civilization-to")) {
			const char *civilization_ident = LuaToString(l, j + 1, 2);
			const civilization *civilization = civilization::get(civilization_ident);
			um->change_civilization_to = civilization;
		} else if (!strcmp(key, "change-faction-to")) {
			const std::string faction_ident = LuaToString(l, j + 1, 2);
			um->change_faction_to = faction::get(faction_ident);
		//Wyrmgus end
		} else {
			int index = UnitTypeVar.VariableNameLookup[key]; // variable index;
			if (index != -1) {
				if (lua_rawlen(l, j + 1) == 3) {
					const char *value = LuaToString(l, j + 1, 3);
					if (!strcmp(value, "Percent")) {
						um->ModifyPercent[index] = LuaToNumber(l, j + 1, 2);
					//Wyrmgus start
					} else if (!strcmp(value, "Increase")) {
						um->Modifier.Variables[index].Increase = LuaToNumber(l, j + 1, 2);
					//Wyrmgus end
					}
				} else {
					lua_rawgeti(l, j + 1, 2);
					if (lua_istable(l, -1)) {
						DefineVariableField(l, um->Modifier.Variables[index], -1);
					} else if (lua_isnumber(l, -1)) {
						um->Modifier.Variables[index].Enable = 1;
						um->Modifier.Variables[index].Value = LuaToNumber(l, -1);
						um->Modifier.Variables[index].Max = LuaToNumber(l, -1);
					} else {
						LuaError(l, "bad argument type for '%s'\n" _C_ key);
					}
					lua_pop(l, 1);
				}
			} else {
				LuaError(l, "wrong tag: %s" _C_ key);
			}
		}
	}

	upgrade_modifier::UpgradeModifiers.push_back(um.get());
	upgrade->add_modifier(std::move(um));

	return 0;
}

/**
**  Define which units are allowed and how much.
*/
static int CclDefineUnitAllow(lua_State *l)
{
	const int args = lua_gettop(l);

	const char *ident = LuaToString(l, 0 + 1);

	if (strncmp(ident, "unit", 4)) {
		DebugPrint(" wrong ident %s\n" _C_ ident);
		return 0;
	}
	int id = UnitTypeIdByIdent(ident);

	int i = 0;
	for (int j = 1; j < args && i < PlayerMax; ++j) {
		AllowUnitId(*CPlayer::Players[i], id, LuaToNumber(l, j + 1));
		++i;
	}
	return 0;
}

/**
**  Define which units/upgrades are allowed.
*/
static int CclDefineAllow(lua_State *l)
{
	const int UnitMax = 65536; /// How many units supported
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		const char *ident = LuaToString(l, j + 1);
		++j;
		const char *ids = LuaToString(l, j + 1);

		int n = strlen(ids);
		if (n > PlayerMax) {
			fprintf(stderr, "%s: Allow string too long %d\n", ident, n);
			n = PlayerMax;
		}

		if (!strncmp(ident, "unit", 4)) {
			int id = UnitTypeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				if (ids[i] == 'A') {
					AllowUnitId(*CPlayer::Players[i], id, UnitMax);
				} else if (ids[i] == 'F') {
					AllowUnitId(*CPlayer::Players[i], id, 0);
				}
			}
		} else if (!strncmp(ident, "upgrade", 7)) {
			int id = UpgradeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				AllowUpgradeId(*CPlayer::Players[i], id, ids[i]);
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
	}
	return 0;
}

//Wyrmgus start
/**
** Acquire an ability
*/
static int CclAcquireAbility(lua_State *l)
{
	LuaCheckArgs(l, 2);
	
	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = &unit_manager::get()->GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	AbilityAcquire(*unit, CUpgrade::get(ident));
	return 0;
}

/**
** Set the unit's trait
*/
static int CclAcquireTrait(lua_State *l)
{
	LuaCheckArgs(l, 2);
	
	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = &unit_manager::get()->GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);

	unit->remove_traits();

	const std::string ident = LuaToString(l, 2);
	if (!ident.empty()) {
		unit->add_trait(CUpgrade::get(ident));
	}

	return 0;
}

static int CclGetUpgrades(lua_State *l)
{
	lua_createtable(l, CUpgrade::get_all().size(), 0);
	for (size_t i = 1; i <= CUpgrade::get_all().size(); ++i)
	{
		lua_pushstring(l, CUpgrade::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemPrefixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_prefixes;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->is_magic_prefix()) {
			item_prefixes.push_back(upgrade);
		}
	}
		
	lua_createtable(l, item_prefixes.size(), 0);
	for (size_t i = 1; i <= item_prefixes.size(); ++i)
	{
		lua_pushstring(l, item_prefixes[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemSuffixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_suffixes;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->is_magic_suffix() && !upgrade->RunicAffix) {
			item_suffixes.push_back(upgrade);
		}
	}
		
	lua_createtable(l, item_suffixes.size(), 0);
	for (size_t i = 1; i <= item_suffixes.size(); ++i)
	{
		lua_pushstring(l, item_suffixes[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetRunicSuffixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_suffixes;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->is_magic_suffix() && upgrade->RunicAffix) {
			item_suffixes.push_back(upgrade);
		}
	}
		
	lua_createtable(l, item_suffixes.size(), 0);
	for (size_t i = 1; i <= item_suffixes.size(); ++i)
	{
		lua_pushstring(l, item_suffixes[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetLiteraryWorks(lua_State *l)
{
	std::vector<const CUpgrade *> literary_works;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->Work != item_class::none) {
			literary_works.push_back(upgrade);
		}
	}
		
	lua_createtable(l, literary_works.size(), 0);
	for (size_t i = 1; i <= literary_works.size(); ++i)
	{
		lua_pushstring(l, literary_works[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get upgrade data.
**
**  @param l  Lua state.
*/
static int CclGetUpgradeData(lua_State *l)
{
	const int nargs = lua_gettop(l);
	if (nargs < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string upgrade_ident = LuaToString(l, 1);
	const CUpgrade *upgrade = CUpgrade::get(upgrade_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, upgrade->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Class")) {
		if (upgrade->get_upgrade_class() != nullptr) {
			lua_pushstring(l, upgrade->get_upgrade_class()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (upgrade->get_civilization() != nullptr) {
			lua_pushstring(l, upgrade->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (upgrade->get_faction() != nullptr) {
			lua_pushstring(l, upgrade->get_faction()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (upgrade->get_icon()) {
			lua_pushstring(l, upgrade->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, upgrade->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, upgrade->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, upgrade->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "RequirementsString")) {
		lua_pushstring(l, upgrade->get_requirements_string().c_str());
		return 1;
	} else if (!strcmp(data, "Ability")) {
		lua_pushboolean(l, upgrade->is_ability());
		return 1;
	} else if (!strcmp(data, "ItemPrefix")) {
		if (nargs == 2) { //check if the upgrade is a prefix for any item type
			if (upgrade->is_magic_prefix()) {
				lua_pushboolean(l, true);
				return 1;
			} else {
				lua_pushboolean(l, false);
				return 1;
			}
		} else {
			LuaCheckArgs(l, 3);
			const std::string item_class_name = LuaToString(l, 3);
			const item_class item_class = enum_converter<wyrmgus::item_class>::to_enum(item_class_name);
			lua_pushboolean(l, upgrade->is_magic_prefix() && upgrade->has_affixed_item_class(item_class));
			return 1;
		}
	} else if (!strcmp(data, "ItemSuffix")) {
		if (nargs == 2) { //check if the item is a suffix for any item type
			if (upgrade->is_magic_suffix()) {
				lua_pushboolean(l, true);
				return 1;
			} else {
				lua_pushboolean(l, false);
				return 1;
			}
		} else {
			LuaCheckArgs(l, 3);
			const std::string item_class_name = LuaToString(l, 3);
			const item_class item_class = enum_converter<wyrmgus::item_class>::to_enum(item_class_name);
			lua_pushboolean(l, upgrade->is_magic_suffix() && upgrade->has_affixed_item_class(item_class));
			return 1;
		}
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

/**
**  Register CCL features for upgrades.
*/
void UpgradesCclRegister()
{
	//Wyrmgus start
	lua_register(Lua, "DefineUpgrade", CclDefineUpgrade);
	//Wyrmgus end
	lua_register(Lua, "DefineModifier", CclDefineModifier);
	lua_register(Lua, "DefineAllow", CclDefineAllow);
	lua_register(Lua, "DefineUnitAllow", CclDefineUnitAllow);
	//Wyrmgus start
	lua_register(Lua, "AcquireAbility", CclAcquireAbility);
	lua_register(Lua, "AcquireTrait", CclAcquireTrait);
	lua_register(Lua, "GetUpgrades", CclGetUpgrades);
	lua_register(Lua, "GetItemPrefixes", CclGetItemPrefixes);
	lua_register(Lua, "GetItemSuffixes", CclGetItemSuffixes);
	lua_register(Lua, "GetRunicSuffixes", CclGetRunicSuffixes);
	lua_register(Lua, "GetLiteraryWorks", CclGetLiteraryWorks);
	lua_register(Lua, "GetUpgradeData", CclGetUpgradeData);
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
-- General/Map functions
----------------------------------------------------------------------------*/

// AllowStruct and UpgradeTimers will be static in the player so will be
// load/saved with the player struct

/**
**  UnitType ID by identifier.
**
**  @param ident  The unit-type identifier.
**  @return       Unit-type ID (int) or -1 if not found.
*/
int UnitTypeIdByIdent(const std::string &ident)
{
	const unit_type *type = unit_type::get(ident);

	if (type != nullptr) {
		return type->Slot;
	}

	throw std::runtime_error("No unit type found for identifier: \"" + ident + "\".");
}

/**
**  Upgrade ID by identifier.
**
**  @param ident  The upgrade identifier.
**  @return       Upgrade ID (int) or -1 if not found.
*/
int UpgradeIdByIdent(const std::string &ident)
{
	const CUpgrade *upgrade = CUpgrade::try_get(ident);

	if (upgrade) {
		return upgrade->ID;
	}
	DebugPrint(" fix this %s\n" _C_ ident.c_str());
	return -1;
}

/*----------------------------------------------------------------------------
-- Upgrades
----------------------------------------------------------------------------*/

/**
**  Convert unit-type to.
**
**  @param player  For this player.
**  @param src     From this unit-type.
**  @param dst     To this unit-type.
*/
void ConvertUnitTypeTo(CPlayer &player, const unit_type &src, const unit_type &dst)
{
	//Wyrmgus start
	if (player.AiEnabled && GameCycle > 0) {
		//if is AI player, convert all requests from the old unit type to the new one; FIXME: if already has requests of the new unit type, then the count of the old one should be added to the new one, instead of merely changing the type of the old one to the new one
		for (unsigned int i = 0; i < player.Ai->UnitTypeRequests.size(); ++i) {
			if (player.Ai->UnitTypeRequests[i].Type->Slot == src.Slot) {
				player.Ai->UnitTypeRequests[i].Type = &dst;
			}
		}

		for (unsigned int i = 0; i < player.Ai->UpgradeToRequests.size(); ++i) {
			if (player.Ai->UpgradeToRequests[i]->Slot == src.Slot) {
				player.Ai->UpgradeToRequests[i] = &dst;
			}
		}
		
		for (unsigned int i = 0; i < player.Ai->Force.Size(); ++i) {
			AiForce &force = player.Ai->Force[i];

			for (unsigned int j = 0; j < force.UnitTypes.size(); ++j) {
				if (force.UnitTypes[j].Type->Slot == src.Slot) {
					force.UnitTypes[j].Type = &dst;
				}
			}
		}

		for (unsigned int i = 0; i < player.Ai->UnitTypeBuilt.size(); ++i) {
			if (player.Ai->UnitTypeBuilt[i].Type->Slot == src.Slot) {
				player.Ai->UnitTypeBuilt[i].Type = &dst;
			}
		}
	}
	//Wyrmgus end
	for (int i = 0; i < player.GetUnitCount(); ++i) {
		CUnit &unit = player.GetUnit(i);

		//convert already existing units to this type.
		//Wyrmgus start
//		if (unit.Type == &src) {
		if (unit.Type == &src && unit.get_character() == nullptr) { //don't do this for persistent characters
		//Wyrmgus end
			CommandTransformIntoType(unit, dst);
			//  Convert trained units to this type.
			//  FIXME: what about buildings?
		//Wyrmgus start
//		} else {
		} else if (GameCycle > 0) {
		//Wyrmgus end
			//Wyrmgus start
			// convert transformation order
			if (unit.CriticalOrder != nullptr && unit.CriticalOrder->Action == UnitAction::TransformInto) {
				COrder_TransformInto *order = static_cast<COrder_TransformInto *>(unit.CriticalOrder.get());

				if (&order->GetUnitType() == &src) {
					order->ConvertUnitType(unit, dst);
				}
			}
			//Wyrmgus end
			
			for (size_t j = 0; j < unit.Orders.size(); ++j) {
				if (unit.Orders[j]->Action == UnitAction::Train) {
					COrder_Train &order = *static_cast<COrder_Train *>(unit.Orders[j].get());

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus start
				// convert building orders as well
				} else if (unit.Orders[j]->Action == UnitAction::Build) {
					COrder_Build &order = *static_cast<COrder_Build *>(unit.Orders[j].get());

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				// also convert upgrade orders
				} else if (unit.Orders[j]->Action == UnitAction::UpgradeTo) {
					COrder_UpgradeTo &order = *static_cast<COrder_UpgradeTo *>(unit.Orders[j].get());

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus end
				}
			}
		}
	}
}

/**
**  Apply the modifiers of an individual upgrade.
**
**  @param unit    Unit that will get the modifier applied
**  @param um      Upgrade modifier that does the effects
*/
void ApplyIndividualUpgradeModifier(CUnit &unit, const upgrade_modifier *um)
{
	assert_throw(um != nullptr);

	for (const CUpgrade *removed_upgrade : um->get_removed_upgrades()) {
		if (unit.GetIndividualUpgrade(removed_upgrade) > 0) {
			IndividualUpgradeLost(unit, removed_upgrade, true);
		}
	}

	if (um->Modifier.Variables[SUPPLY_INDEX].Value != 0) {
		if (unit.IsAlive()) {
			unit.Player->change_supply(um->Modifier.Variables[SUPPLY_INDEX].Value);

			if (defines::get()->is_population_enabled() && unit.get_settlement() != nullptr) {
				unit.get_settlement()->get_game_data()->change_housing(um->Modifier.Variables[SUPPLY_INDEX].Value);
			}
		}
	}

	um->apply_to_unit(&unit, 1);
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	const unit_type_variation *current_variation = unit.GetVariation();
	if (current_variation != nullptr) {
		if (!unit.can_have_variation(current_variation)) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < static_cast<int>(image_layer::count); ++i) {
		const unit_type_variation *current_layer_variation = unit.GetLayerVariation(i);
		if (current_layer_variation != nullptr) {
			if (!unit.can_have_variation(current_layer_variation)) {
				unit.ChooseVariation(nullptr, false, i);
			}
		}
	}

	unit.UpdateButtonIcons();
	//Wyrmgus end
	
	if (um->convert_to) {
		//Wyrmgus start
		//CommandTransformIntoType(unit, *um->convert_to);
		if (unit.get_character() == nullptr) { //don't do this for persistent characters
			CommandTransformIntoType(unit, *um->convert_to);
		}
		//Wyrmgus end
	}
}

void RemoveIndividualUpgradeModifier(CUnit &unit, const upgrade_modifier *um)
{
	assert_throw(um != nullptr);

	if (um->Modifier.Variables[SUPPLY_INDEX].Value != 0) {
		if (unit.IsAlive()) {
			unit.Player->change_supply(-um->Modifier.Variables[SUPPLY_INDEX].Value);

			if (defines::get()->is_population_enabled() && unit.get_settlement() != nullptr) {
				unit.get_settlement()->get_game_data()->change_housing(-um->Modifier.Variables[SUPPLY_INDEX].Value);
			}
		}
	}

	um->apply_to_unit(&unit, -1);
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	const unit_type_variation *current_variation = unit.GetVariation();
	if (current_variation != nullptr) {
		if (!unit.can_have_variation(current_variation)) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < static_cast<int>(image_layer::count); ++i) {
		const unit_type_variation *current_layer_variation = unit.GetLayerVariation(i);
		if (current_layer_variation != nullptr) {
			if (!unit.can_have_variation(current_layer_variation)) {
				unit.ChooseVariation(nullptr, false, i);
			}
		}
	}
	unit.UpdateButtonIcons();
	//Wyrmgus end
}

/**
**  Apply researched upgrades when map is loading
**
**  @return:   void
*/
void ApplyUpgrades()
{
	for (CUpgrade *upgrade : CUpgrade::get_all()) {
		for (int p = 0; p < PlayerMax; ++p) {
			if (CPlayer::Players[p]->Allow.Upgrades[upgrade->ID] == 'R') {
				const int id = upgrade->ID;
				CPlayer::Players[p]->UpgradeTimers.Upgrades[id] = upgrade->get_time_cost();
				AllowUpgradeId(*CPlayer::Players[p], id, 'R');  // research done

				for (const auto &modifier : upgrade->get_modifiers()) {
					modifier->apply_to_player(CPlayer::Players[p].get(), 1);
				}
			}
		}
	}
	
	for (int p = 0; p < PlayerMax; ++p) {
		CPlayer::Players[p]->check_age();
	}
}

void AbilityAcquire(CUnit &unit, const CUpgrade *upgrade, bool save)
{
	unit.Variable[LEVELUP_INDEX].Value -= 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	if (game::get()->is_persistency_enabled() && unit.get_character() != nullptr && save) {
		if (unit.Player == CPlayer::GetThisPlayer()) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
			unit.get_character()->add_ability(upgrade);
			unit.get_character()->save();
		}
	}
	IndividualUpgradeAcquire(unit, upgrade);
	unit.Player->UpdateLevelUpUnits();
}

void AbilityLost(CUnit &unit, CUpgrade *upgrade, bool lose_all)
{
	unit.Variable[LEVELUP_INDEX].Value += 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	unit.Variable[LEVELUP_INDEX].Enable = 1;
	if (game::get()->is_persistency_enabled() && unit.get_character() != nullptr) {
		if (vector::contains(unit.get_character()->get_abilities(), upgrade)) {
			if (unit.Player == CPlayer::GetThisPlayer()) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
				unit.get_character()->remove_ability(upgrade);
				unit.get_character()->save();
			}
		}
	}
	IndividualUpgradeLost(unit, upgrade);
	unit.Player->UpdateLevelUpUnits();
	
	if (lose_all && unit.GetIndividualUpgrade(upgrade) > 0) {
		AbilityLost(unit, upgrade, lose_all);
	}
}
//Wyrmgus end

void IndividualUpgradeAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing && !SaveGameLoading) {
		return;
	}
	//Wyrmgus end
	unit.SetIndividualUpgrade(upgrade, unit.GetIndividualUpgrade(upgrade) + 1);
	
	const deity *upgrade_deity = upgrade->get_deity();
	if (upgrade_deity != nullptr) {
		for (const magic_domain *domain : upgrade_deity->get_domains()) {
			const CUpgrade *domain_upgrade = domain->get_deity_domain_upgrade();
			if (unit.GetIndividualUpgrade(domain_upgrade) == 0) {
				IndividualUpgradeAcquire(unit, domain_upgrade);
			}
		}
		if (game::get()->is_persistency_enabled() && unit.get_character() != nullptr && !vector::contains(unit.get_character()->Deities, upgrade_deity) && unit.Player == CPlayer::GetThisPlayer()) {
			unit.get_character()->Deities.push_back(upgrade_deity);
			unit.get_character()->save();
		}
	}

	if (!upgrade->is_ability() || upgrade->WeaponClasses.empty() || upgrade->WeaponClasses.contains(unit.GetCurrentWeaponClass())) {
		for (const auto &modifier : upgrade->get_modifiers()) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (const unit_type *unit_type : unit_type::get_all()) {
				if (unit_type->is_template()) {
					continue;
				}

				if (modifier->applies_to(unit_type)) {
					applies_to_any_unit_types = true;
					if (unit_type == unit.Type) {
						applies_to_this = true;
						break;
					}
				}
			}
			if (applies_to_this || !applies_to_any_unit_types) { //if the modifier isn't designated as being for a specific unit type, or is designated for this unit's unit type, apply it
				ApplyIndividualUpgradeModifier(unit, modifier.get());
			}
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void IndividualUpgradeLost(CUnit &unit, const CUpgrade *upgrade, bool lose_all)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing && !SaveGameLoading) {
		return;
	}
	//Wyrmgus end
	unit.SetIndividualUpgrade(upgrade, unit.GetIndividualUpgrade(upgrade) - 1);

	const deity *upgrade_deity = upgrade->get_deity();
	if (upgrade_deity != nullptr) {
		for (const magic_domain *domain : upgrade_deity->get_domains()) {
			const CUpgrade *domain_upgrade = domain->get_deity_domain_upgrade();
			if (unit.GetIndividualUpgrade(domain_upgrade) > 0) {
				IndividualUpgradeLost(unit, domain_upgrade);
			}
		}
		if (game::get()->is_persistency_enabled() && unit.get_character() != nullptr && unit.Player == CPlayer::GetThisPlayer()) {
			vector::remove(unit.get_character()->Deities, upgrade_deity);
			unit.get_character()->save();
		}
	}

	//Wyrmgus start
	if (!upgrade->is_ability() || upgrade->WeaponClasses.empty() || upgrade->WeaponClasses.contains(unit.GetCurrentWeaponClass())) {
		for (const auto &modifier : upgrade->get_modifiers()) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (const unit_type *unit_type : unit_type::get_all()) {
				if (unit_type->is_template()) {
					continue;
				}

				if (modifier->applies_to(unit_type)) {
					applies_to_any_unit_types = true;
					if (unit_type == unit.Type) {
						applies_to_this = true;
						break;
					}
				}
			}
			if (applies_to_this || !applies_to_any_unit_types) { //if the modifier isn't designated as being for a specific unit type, or is designated for this unit's unit type, remove it
				RemoveIndividualUpgradeModifier(unit, modifier.get());
			}
		}
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
	
	if (lose_all && unit.GetIndividualUpgrade(upgrade) > 0) {
		IndividualUpgradeLost(unit, upgrade, lose_all);
	}
}

/*----------------------------------------------------------------------------
--  Allow(s)
----------------------------------------------------------------------------*/

// all the following functions are just map handlers, no specific notes

/**
**  Change allow for an unit-type.
**
**  @param player  Player to change
**  @param id      unit type id
**  @param units   maximum amount of units allowed
*/
//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units)
void AllowUnitId(CPlayer &player, int id, int units)
//Wyrmgus end
{
	player.Allow.Units[id] = units;
}

/**
**  Change allow for an upgrade.
**
**  @param player  Player to change
**  @param id      upgrade id
**  @param af      'A'llow/'F'orbid/'R'esearched
*/
void AllowUpgradeId(CPlayer &player, int id, char af)
{
	assert_throw(af == 'A' || af == 'F' || af == 'R');
	player.Allow.Upgrades[id] = af;
}

/**
**  Return the allow state of the unit.
**
**  @param player   Check state of this player.
**  @param id       Unit identifier.
**
**  @return the allow state of the unit.
*/
int UnitIdAllowed(const CPlayer &player, int id)
{
	assert_throw(id >= 0 && id < UnitTypeMax);
	return player.Allow.Units[id];
}

/**
**  Return the allow state of an upgrade.
**
**  @param player  Check state for this player.
**  @param id      Upgrade identifier.
**
**  @return the allow state of the upgrade.
*/
char UpgradeIdAllowed(const CPlayer &player, int id)
{
	assert_throw(id >= 0 && id < UpgradeMax);
	return player.Allow.Upgrades[id];
}

// ***************by string identifiers's

/**
**  Return the allow state of an upgrade.
**
**  @param player  Check state for this player.
**  @param ident   Upgrade identifier.
**
**  @note This function shouldn't be used during runtime, it is only for setup.
*/
char UpgradeIdentAllowed(const CPlayer &player, const std::string &ident)
{
	int id = UpgradeIdByIdent(ident);

	if (id != -1) {
		return UpgradeIdAllowed(player, id);
	}
	DebugPrint("Fix your code, wrong identifier '%s'\n" _C_ ident.c_str());
	return '-';
}

//Wyrmgus start
std::string GetUpgradeEffectsString(const CUpgrade *upgrade, const bool multiline, const size_t indent)
{
	std::string padding_string = ", ";
	if (multiline) {
		padding_string = "\n";

		if (indent > 0) {
			padding_string += std::string(indent, '\t');
		}
	}

	std::string upgrade_effects_string;

	bool first_element = true;
	//check if the upgrade makes modifications to any units
	for (const auto &modifier : upgrade->get_modifiers()) {
		if (!first_element) {
			upgrade_effects_string += padding_string;
		} else {
			first_element = false;
		}

		bool first_var = true;
		for (size_t var = 0; var < UnitTypeVar.GetNumberVariable(); ++var) {
			if (var == PRIORITY_INDEX || var == POINTS_INDEX) {
				continue;
			}

			if (var == STRENGTH_INDEX || var == DEXTERITY_INDEX || var == INTELLIGENCE_INDEX || var == CHARISMA_INDEX) { // don't show attributes for now
				continue;
			}

			if (modifier->Modifier.Variables[var].Value != 0) {
				if (!first_var) {
					upgrade_effects_string += padding_string;
				} else {
					first_var = false;
				}

				if (IsBooleanVariable(var) && modifier->Modifier.Variables[var].Value < 0) {
					upgrade_effects_string += "Lose ";
				}

				if (!IsBooleanVariable(var)) {
					if (modifier->Modifier.Variables[var].Value > 0) {
						upgrade_effects_string += "+";
					}
					upgrade_effects_string += std::to_string(modifier->Modifier.Variables[var].Value);
					if (IsPercentageVariable(var)) {
						upgrade_effects_string += "%";
					}
					upgrade_effects_string += " ";
				}

				upgrade_effects_string += GetVariableDisplayName(var);

				bool first_unit_type = true;

				for (const unit_type *unit_type : modifier->get_unit_types()) {
					if (unit_type->is_template()) {
						continue;
					}

					if (!first_unit_type) {
						upgrade_effects_string += ", ";
					} else {
						upgrade_effects_string += " for ";
						first_unit_type = false;
					}

					upgrade_effects_string += unit_type->GetNamePlural();
				}

				for (const unit_class *unit_class : modifier->get_unit_classes()) {
					if (unit_class->get_unit_types().empty()) {
						continue;
					}

					if (!first_unit_type) {
						upgrade_effects_string += ", ";
					} else {
						upgrade_effects_string += " for ";
						first_unit_type = false;
					}

					upgrade_effects_string += string::get_plural_form(unit_class->get_name());
				}
			}

			if (modifier->Modifier.Variables[var].Increase != 0) {
				if (!first_var) {
					upgrade_effects_string += padding_string;
				} else {
					first_var = false;
				}

				if (modifier->Modifier.Variables[var].Increase > 0) {
					upgrade_effects_string += "+";
				}
				upgrade_effects_string += std::to_string(modifier->Modifier.Variables[var].Increase);
				upgrade_effects_string += " ";

				upgrade_effects_string += GetVariableDisplayName(var, true);
			}
		}

		bool first_res = true;
		for (const auto &[resource, quantity] : modifier->Modifier.get_improve_incomes()) {
			if (!first_res) {
				upgrade_effects_string += padding_string;
			} else {
				first_res = false;
			}

			if (quantity > 0) {
				upgrade_effects_string += "+";
			}
			upgrade_effects_string += std::to_string(quantity);
			upgrade_effects_string += "%";
			upgrade_effects_string += " ";
			upgrade_effects_string += resource->get_name();
			upgrade_effects_string += " Processing";

			bool first_unit_type = true;

			for (const unit_type *unit_type : modifier->get_unit_types()) {
				if (unit_type->is_template()) {
					continue;
				}

				if (!first_unit_type) {
					upgrade_effects_string += ", ";
				} else {
					upgrade_effects_string += " for ";
					first_unit_type = false;
				}

				upgrade_effects_string += unit_type->GetNamePlural();
			}

			for (const unit_class *unit_class : modifier->get_unit_classes()) {
				if (unit_class->get_unit_types().empty()) {
					continue;
				}

				if (!first_unit_type) {
					upgrade_effects_string += ", ";
				} else {
					upgrade_effects_string += " for ";
					first_unit_type = false;
				}

				upgrade_effects_string += string::get_plural_form(unit_class->get_name());
			}
		}
	}

	return upgrade_effects_string;
}

bool IsPercentageVariable(const int var)
{
	switch (var) {
		case CHARGEBONUS_INDEX:
		case BACKSTAB_INDEX:
		case BONUS_AGAINST_INFANTRY_INDEX:
		case BONUSAGAINSTMOUNTED_INDEX:
		case BONUSAGAINSTBUILDINGS_INDEX:
		case BONUSAGAINSTAIR_INDEX:
		case BONUSAGAINSTGIANTS_INDEX:
		case BONUSAGAINSTDRAGONS_INDEX:
		case FIRERESISTANCE_INDEX:
		case COLDRESISTANCE_INDEX:
		case ARCANERESISTANCE_INDEX:
		case LIGHTNINGRESISTANCE_INDEX:
		case AIRRESISTANCE_INDEX:
		case EARTHRESISTANCE_INDEX:
		case WATERRESISTANCE_INDEX:
		case ACIDRESISTANCE_INDEX:
		case SHADOW_RESISTANCE_INDEX:
		case HACKRESISTANCE_INDEX:
		case PIERCERESISTANCE_INDEX:
		case BLUNTRESISTANCE_INDEX:
		case TIMEEFFICIENCYBONUS_INDEX:
		case RESEARCHSPEEDBONUS_INDEX:
		case TRADECOST_INDEX:
		case SALVAGEFACTOR_INDEX:
		case COST_MODIFIER_INDEX:
		case MUGGING_INDEX:
		case RAIDING_INDEX:
		case CAPTURE_HP_THRESHOLD_INDEX:
			return true;
		default:
			return false;
	}
}

bool IsBonusVariable(const int var)
{
	switch (var) {
		case GATHERINGBONUS_INDEX:
		case COPPERGATHERINGBONUS_INDEX:
		case SILVERGATHERINGBONUS_INDEX:
		case GOLDGATHERINGBONUS_INDEX:
		case IRONGATHERINGBONUS_INDEX:
		case MITHRILGATHERINGBONUS_INDEX:
		case LUMBERGATHERINGBONUS_INDEX:
		case STONEGATHERINGBONUS_INDEX:
		case COALGATHERINGBONUS_INDEX:
		case JEWELRYGATHERINGBONUS_INDEX:
		case FURNITUREGATHERINGBONUS_INDEX:
		case LEATHERGATHERINGBONUS_INDEX:
		case GEMSGATHERINGBONUS_INDEX:
		case SPEEDBONUS_INDEX:
		case RAIL_SPEED_BONUS_INDEX:
		case CHARGEBONUS_INDEX:
		case BACKSTAB_INDEX:
		case DAYSIGHTRANGEBONUS_INDEX:
		case NIGHTSIGHTRANGEBONUS_INDEX:
		case TIMEEFFICIENCYBONUS_INDEX:
		case RESEARCHSPEEDBONUS_INDEX:
		case GARRISONEDRANGEBONUS_INDEX:
		case AURA_RANGE_BONUS_INDEX:
			return true;
		default:
			return false;
	}
}

bool IsBooleanVariable(const int var)
{
	switch (var) {
		case DISEMBARKMENTBONUS_INDEX:
		case DESERTSTALK_INDEX:
		case FORESTSTALK_INDEX:
		case SWAMPSTALK_INDEX:
		case DEHYDRATIONIMMUNITY_INDEX:
		case LEADERSHIPAURA_INDEX:
		case REGENERATIONAURA_INDEX:
		case HYDRATINGAURA_INDEX:
		case ETHEREALVISION_INDEX:
		case GARRISONED_GATHERING_INDEX:
			return true;
		default:
			return false;
	}
}

bool IsKnowledgeVariable(const int var)
{
	switch (var) {
		case KNOWLEDGEMAGIC_INDEX:
		case KNOWLEDGEWARFARE_INDEX:
		case KNOWLEDGEMINING_INDEX:
			return true;
		default:
			return false;
	}
}

bool is_potentially_negative_variable(const int var_index)
{
	switch (var_index) {
		case DAYSIGHTRANGEBONUS_INDEX:
		case NIGHTSIGHTRANGEBONUS_INDEX:
			return true;
		default:
			return false;
	}
}

std::string GetVariableDisplayName(int var, bool increase)
{
	std::string variable_name = UnitTypeVar.VariableNameLookup[var];

	if (increase) {
		variable_name += "Increase";
		variable_name = FindAndReplaceString(variable_name, "HitPointsIncrease", "Regeneration");
		variable_name = FindAndReplaceString(variable_name, "HitPointBonusIncrease", "Regeneration");
		variable_name = FindAndReplaceString(variable_name, "GiveResourceIncrease", "ResourceReplenishment");
	}
	
	variable_name = FindAndReplaceString(variable_name, "BasicDamage", "Damage");
	variable_name = FindAndReplaceString(variable_name, "DaySightRangeBonus", "DaySight");
	variable_name = FindAndReplaceString(variable_name, "NightSightRangeBonus", "NightSight");
	variable_name = FindAndReplaceString(variable_name, "SightRange", "Sight");
	variable_name = FindAndReplaceString(variable_name, "AttackRange", "Range");
	variable_name = FindAndReplaceString(variable_name, "HitPointBonus", "HitPoints");
	variable_name = FindAndReplaceString(variable_name, "Supply", "FoodSupply");
	variable_name = FindAndReplaceString(variable_name, "Demand", "FoodCost");

	variable_name = SeparateCapitalizedStringElements(variable_name);

	variable_name = FindAndReplaceString(variable_name, "Backstab", "Backstab Bonus");
	variable_name = FindAndReplaceString(variable_name, "Capture Hp Threshold", "Capture HP Threshold");
	variable_name = FindAndReplaceString(variable_name, "Knowledge Magic", "Knowledge (Magic)");
	variable_name = FindAndReplaceString(variable_name, "Knowledge Warfare", "Knowledge (Warfare)");

	return _(variable_name.c_str());
}
//Wyrmgus end
