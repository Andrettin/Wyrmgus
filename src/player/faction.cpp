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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "player/faction.h"

#include "ai/ai_force_template.h"
#include "ai/ai_force_type.h"
#include "character.h"
#include "character_title.h"
#include "fallback_name_generator.h"
#include "language/name_generator.h"
#include "luacallback.h"
#include "map/region.h"
#include "map/site.h"
#include "player/civilization.h"
#include "player/diplomacy_state.h"
#include "player/faction_history.h"
#include "player/faction_tier.h"
#include "player/faction_type.h"
#include "player/government_type.h"
#include "player/player.h"
#include "player/player_color.h"
#include "script/condition/and_condition.h"
#include "translator.h"
#include "ui/button.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/set_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

//factions must be initialized after regions, so that they have all their sites, for adding neutral factions to target sites
const std::set<std::string> faction::database_dependencies = { region::class_identifier };

bool faction::compare_encyclopedia_entries(const faction *lhs, const faction *rhs)
{
	const wyrmgus::civilization *lhs_civilization = lhs->get_civilization();
	const wyrmgus::civilization *rhs_civilization = rhs->get_civilization();

	if (lhs_civilization != rhs_civilization) {
		if (lhs_civilization == nullptr || rhs_civilization == nullptr) {
			return lhs_civilization == nullptr;
		}

		return lhs_civilization->get_name() < rhs_civilization->get_name();
	}

	if (lhs->get_type() != rhs->get_type()) {
		return lhs->get_type() < rhs->get_type();
	}

	return named_data_entry::compare_encyclopedia_entries(lhs, rhs);
}

void faction::process_title_names(title_name_map &title_names, const gsml_data &scope)
{
	scope.for_each_child([&](const gsml_data &child_scope) {
		faction::process_title_name_scope(title_names, child_scope);
	});

	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const wyrmgus::government_type government_type = enum_converter<wyrmgus::government_type>::to_enum(key);
		title_names[government_type][faction_tier::none] = value;
	});
}

void faction::process_title_name_scope(title_name_map &title_names, const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const wyrmgus::government_type government_type = enum_converter<wyrmgus::government_type>::to_enum(tag);

	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const faction_tier tier = enum_converter<faction_tier>::to_enum(key);
		title_names[government_type][tier] = value;
	});
}

void faction::process_character_title_name_scope(character_title_name_map &character_title_names, const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const character_title title_type = enum_converter<character_title>::to_enum(tag);

	scope.for_each_child([&](const gsml_data &child_scope) {
		faction::process_character_title_name_scope(character_title_names[title_type], child_scope);
	});

	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const wyrmgus::government_type government_type = enum_converter<wyrmgus::government_type>::to_enum(key);
		character_title_names[title_type][government_type][faction_tier::none][gender::none] = value;
	});
}

void faction::process_character_title_name_scope(std::map<government_type, std::map<faction_tier, std::map<gender, std::string>>> &character_title_names, const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const wyrmgus::government_type government_type = enum_converter<wyrmgus::government_type>::to_enum(tag);

	scope.for_each_child([&](const gsml_data &child_scope) {
		faction::process_character_title_name_scope(character_title_names[government_type], child_scope);
	});
}

void faction::process_character_title_name_scope(std::map<faction_tier, std::map<gender, std::string>> &character_title_names, const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const faction_tier faction_tier = enum_converter<wyrmgus::faction_tier>::to_enum(tag);

	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = enum_converter<archimedes::gender>::to_enum(key);
		character_title_names[faction_tier][gender] = value;
	});
}

faction::faction(const std::string &identifier)
	: detailed_data_entry(identifier), type(faction_type::none), default_tier(faction_tier::barony), min_tier(faction_tier::none), max_tier(faction_tier::none), default_government_type(government_type::monarchy)
{
}

faction::~faction()
{
}

void faction::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "develops_to") {
		for (const std::string &value : values) {
			faction *other_faction = faction::get(value);
			this->develops_to.push_back(other_faction);
			other_faction->develops_from.push_back(this);
		}
	} else if (tag == "class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const unit_class *unit_class = unit_class::get(key);
			const unit_type *unit_type = unit_type::get(value);
			this->set_class_unit_type(unit_class, unit_type);
		});
	} else if (tag == "class_upgrades") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const upgrade_class *upgrade_class = upgrade_class::get(key);
			const CUpgrade *upgrade = CUpgrade::get(value);
			this->set_class_upgrade(upgrade_class, upgrade);
		});
	} else if (tag == "core_settlements") {
		for (const std::string &value : values) {
			const site *settlement = site::get(value);
			this->core_settlements.push_back(settlement);
		}
	} else if (tag == "neutral_building_classes") {
		for (const std::string &value : values) {
			const unit_class *building_class = unit_class::get(value);
			this->neutral_building_classes.push_back(building_class);
		}
	} else if (tag == "neutral_target_sites") {
		for (const std::string &value : values) {
			site *site = site::get(value);
			this->neutral_target_sites.push_back(site);
		}
	} else if (tag == "neutral_target_regions") {
		for (const std::string &value : values) {
			const region *region = region::get(value);
			this->neutral_target_regions.push_back(region);
		}
	} else if (tag == "neutral_site_conditions") {
		auto conditions = std::make_unique<and_condition<CUnit>>();
		database::process_gsml_data(conditions, scope);
		this->neutral_site_conditions = std::move(conditions);
	} else if (tag == "neutral_site_spawn_conditions") {
		auto conditions = std::make_unique<and_condition<CUnit>>();
		database::process_gsml_data(conditions, scope);
		this->neutral_site_spawn_conditions = std::move(conditions);
	} else if (tag == "title_names") {
		faction::process_title_names(this->title_names, scope);
	} else if (tag == "character_title_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const character_title title_type = enum_converter<character_title>::to_enum(key);
			character_title_names[title_type][government_type::none][faction_tier::none][gender::none] = value;
		});

		scope.for_each_child([&](const gsml_data &child_scope) {
			faction::process_character_title_name_scope(this->character_title_names, child_scope);
		});
	} else if (tag == "preconditions") {
		auto preconditions = std::make_unique<and_condition<CPlayer>>();
		database::process_gsml_data(preconditions, scope);
		this->preconditions = std::move(preconditions);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<CPlayer>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "force_templates") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			auto force_template = std::make_unique<ai_force_template>();
			database::process_gsml_data(force_template, child_scope);
			this->ai_force_templates[force_template->get_force_type()].push_back(std::move(force_template));
		});
	} else if (tag == "unit_class_names") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const unit_class *unit_class = unit_class::get(tag);

			if (this->unit_class_name_generators.find(unit_class) == this->unit_class_name_generators.end()) {
				this->unit_class_name_generators[unit_class] = std::make_unique<name_generator>();
			}

			this->unit_class_name_generators[unit_class]->add_names(child_scope.get_values());
		});
	} else if (tag == "ship_names") {
		if (this->ship_name_generator == nullptr) {
			this->ship_name_generator = std::make_unique<name_generator>();
		}

		this->ship_name_generator->add_names(values);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void faction::initialize()
{
	if (this->civilization != nullptr) {
		this->civilization->add_faction(this);

		for (const site *core_settlement : this->get_core_settlements()) {
			this->get_civilization()->sites.push_back(core_settlement);
		}
	}

	if (this->has_neutral_type()) {
		faction::neutral_factions.push_back(this);

		const std::vector<site *> target_sites = this->get_all_neutral_target_sites();

		for (site *site : target_sites) {
			site->add_neutral_faction(this);
		}
	}

	if (this->get_type() == faction_type::tribe) {
		this->definite_article = true;
		this->default_government_type = government_type::tribe;
	}

	std::sort(this->AiBuildingTemplates.begin(), this->AiBuildingTemplates.end(), [](const std::unique_ptr<CAiBuildingTemplate> &a, const std::unique_ptr<CAiBuildingTemplate> &b) {
		return a->get_priority() > b->get_priority();
	});

	for (auto &kv_pair : this->ai_force_templates) {
		std::sort(kv_pair.second.begin(), kv_pair.second.end(), [](const std::unique_ptr<ai_force_template> &a, const std::unique_ptr<ai_force_template> &b) {
			return a->get_priority() > b->get_priority();
		});
	}

	if (this->get_parent_faction() != nullptr) {
		const faction *parent_faction = this->get_parent_faction();

		if (this->upgrade == nullptr) { //if the faction has no faction upgrade, inherit that of its parent faction
			this->upgrade = parent_faction->upgrade;
		}

		//inherit button icons from parent civilization, for button actions which none are specified
		for (auto iterator = parent_faction->ButtonIcons.begin(); iterator != parent_faction->ButtonIcons.end(); ++iterator) {
			if (this->ButtonIcons.find(iterator->first) == this->ButtonIcons.end()) {
				this->ButtonIcons[iterator->first] = iterator->second;
			}
		}
	}

	if (this->get_min_tier() == faction_tier::none) {
		this->min_tier = this->get_default_tier();
	}

	if (this->get_max_tier() == faction_tier::none) {
		this->max_tier = this->get_default_tier();
	}

	if (this->civilization != nullptr) {
		if (!this->civilization->is_initialized()) {
			this->civilization->initialize();
		}

		this->civilization->add_names_from(this);
	}

	fallback_name_generator::get()->add_unit_class_names(this->unit_class_name_generators);
	if (this->ship_name_generator != nullptr) {
		fallback_name_generator::get()->add_ship_names(this->ship_name_generator->get_names());
	}

	unit_class::propagate_unit_class_names(this->unit_class_name_generators, this->ship_name_generator);

	data_entry::initialize();
}

void faction::check() const
{
	if (this->get_adjective().empty()) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no adjective.");
	}

	if (this->get_civilization() == nullptr) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no civilization.");
	}

	if (this->get_type() == faction_type::none) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no type.");
	}

	if (this->get_default_tier() == faction_tier::none) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no default tier.");
	}

	if (this->get_civilization()->is_playable() && this->is_playable() && !this->has_neutral_type()) {
		if (this->get_icon() == nullptr) {
			throw std::runtime_error("Faction \"" + this->get_identifier() + "\" is playable and non-neutral, but has no icon.");
		}

		if (this->develops_from.empty() && this->get_develops_to().empty()) {
			throw std::runtime_error("Faction \"" + this->get_identifier() + "\" is playable and non-neutral, but neither develops from nor to any other faction.");
		}

		if (this->get_type() == faction_type::tribe && this->get_develops_to().empty()) {
			throw std::runtime_error("Tribal faction \"" + this->get_identifier() + "\" is playable and non-neutral, but does not develop to any other faction.");
		}

		if (this->get_type() == faction_type::polity && !this->is_government_type_valid(government_type::tribe) && this->develops_from.empty() && this->get_min_tier() >= faction_tier::duchy) {
			throw std::runtime_error("Polity faction \"" + this->get_identifier() + "\" is playable, cannot have a tribal government, and has a minimum faction tier of duchy or above, but does not develop from any other faction.");
		}
	}

	for (const faction *other_faction : this->get_develops_to()) {
		try {
			assert_throw(this->has_neutral_type() == other_faction->has_neutral_type());
			assert_throw(other_faction->get_max_tier() >= this->get_max_tier());

			if (other_faction->get_civilization() != this->get_civilization()) {
				assert_throw(vector::contains(this->get_civilization()->get_develops_to(), other_faction->get_civilization()));
				assert_throw(other_faction->get_civilization()->is_playable() || !this->get_civilization()->is_playable());
				assert_throw(other_faction->get_civilization()->get_conditions() == nullptr || other_faction->get_civilization()->get_conditions()->check(this->get_civilization()));
				assert_throw(other_faction->get_preconditions() == nullptr || other_faction->get_preconditions()->check(this->get_civilization()));
				assert_throw(other_faction->get_conditions() == nullptr || other_faction->get_conditions()->check(this->get_civilization()));
			}

			switch (other_faction->get_type()) {
				case faction_type::tribe:
					//if we are a polity, we can't become a tribe
					assert_throw(this->get_type() != faction_type::polity);
					break;
				case faction_type::polity:
					//if the current faction is a tribal one, the target faction must have the tribe government type be valid for them, as otherwise it won't be possible to transition from one into the other
					assert_throw(this->get_type() != faction_type::tribe || other_faction->is_government_type_valid(government_type::tribe));
					break;
				case faction_type::minor_tribe:
					//neutral factions which aren't already minor tribes can't become minor tribes
					assert_throw(this->get_type() == faction_type::minor_tribe);
					break;
				case faction_type::mercenary_company:
					assert_throw(this->get_type() == faction_type::minor_tribe || this->get_type() == faction_type::mercenary_company);
					break;
				case faction_type::holy_order:
					assert_throw(this->get_type() == faction_type::holy_order);
					break;
				case faction_type::notable_house:
					assert_throw(this->get_type() == faction_type::minor_tribe || this->get_type() == faction_type::notable_house);
					break;
				case faction_type::trading_company:
					assert_throw(this->get_type() == faction_type::trading_company);
					break;
				default:
					assert_throw(false);
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Faction \"" + other_faction->get_identifier() + "\" is not a valid target for faction \"" + this->get_identifier() + "\" to develop to."));
		}
	}

	if (this->get_develops_to().size() > button::get_faction_button_count()) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" can develop to " + std::to_string(this->get_develops_to().size()) + " different factions, but there are only buttons for a faction to develop to " + std::to_string(button::get_faction_button_count()) + " different ones.");
	}

	for (const site *core_settlement : this->get_core_settlements()) {
		if (!core_settlement->is_settlement()) {
			throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has site \"" + core_settlement->get_identifier() + "\" set as one of its core settlements, but the latter is not a settlement.");
		}
	}

	for (const auto &kv_pair : this->ai_force_templates) {
		for (const std::unique_ptr<ai_force_template> &force_template : kv_pair.second) {
			force_template->check();
		}
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	for (const auto &[unit_class, unit_type] : this->class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}

	for (const auto &[upgrade_class, upgrade] : this->class_upgrades) {
		assert_throw(upgrade->get_upgrade_class() == upgrade_class);
	}
}

data_entry_history *faction::get_history_base()
{
	return this->history.get();
}

void faction::reset_history()
{
	this->history = std::make_unique<faction_history>(this->get_default_tier(), this->get_default_government_type(), this->get_default_capital());
}

std::string faction::get_encyclopedia_text() const
{
	std::string text;

	if (this->get_civilization() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Civilization: " + this->get_civilization()->get_link_string());
	}

	named_data_entry::concatenate_encyclopedia_text(text, "Type: " + get_faction_type_name(this->get_type()));
	named_data_entry::concatenate_encyclopedia_text(text, "Color: " + this->get_color()->get_name());

	if (this->get_upgrade() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Effects: " + this->get_upgrade()->get_effects_string());
	}

	std::vector<const faction *> develops_from = this->develops_from;
	std::sort(develops_from.begin(), develops_from.end(), named_data_entry::compare_encyclopedia_entries);

	if (!develops_from.empty()) {
		std::string develops_from_text = _("Develops From");
		develops_from_text += ": ";
		for (size_t i = 0; i < develops_from.size(); ++i) {
			if (i > 0) {
				develops_from_text += ", ";
			}

			const faction *faction = develops_from[i];
			develops_from_text += faction->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(develops_from_text));
	}

	std::vector<const faction *> develops_to = this->get_develops_to();
	std::sort(develops_to.begin(), develops_to.end(), named_data_entry::compare_encyclopedia_entries);

	if (!develops_to.empty()) {
		std::string develops_to_text = _("Develops To");
		develops_to_text += ": ";
		for (size_t i = 0; i < develops_to.size(); ++i) {
			if (i > 0) {
				develops_to_text += ", ";
			}

			const faction *faction = develops_to[i];
			develops_to_text += faction->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(develops_to_text));
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	return text;
}

bool faction::has_neutral_type() const
{
	return is_faction_type_neutral(this->get_type());
}

std::string_view faction::get_title_name(const wyrmgus::government_type government_type, const faction_tier tier) const
{
	if (this->get_type() != faction_type::polity) {
		return string::empty_str;
	}

	const auto find_iterator = this->title_names.find(government_type);
	if (find_iterator != this->title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	return this->get_civilization()->get_title_name(government_type, tier);
}

std::string_view faction::get_character_title_name(const character_title title_type, const wyrmgus::government_type government_type, const faction_tier tier, const gender gender) const
{
	const auto find_iterator = this->character_title_names.find(title_type);
	if (find_iterator != this->character_title_names.end()) {
		auto secondary_find_iterator = find_iterator->second.find(government_type);
		if (secondary_find_iterator == find_iterator->second.end()) {
			secondary_find_iterator = find_iterator->second.find(government_type::none);
		}

		if (secondary_find_iterator != find_iterator->second.end()) {
			auto tertiary_find_iterator = secondary_find_iterator->second.find(tier);
			if (tertiary_find_iterator == secondary_find_iterator->second.end()) {
				tertiary_find_iterator = secondary_find_iterator->second.find(faction_tier::none);
			}

			if (tertiary_find_iterator != secondary_find_iterator->second.end()) {
				auto quaternary_find_iterator = tertiary_find_iterator->second.find(gender);
				if (quaternary_find_iterator == tertiary_find_iterator->second.end()) {
					quaternary_find_iterator = tertiary_find_iterator->second.find(gender::none);
				}

				if (quaternary_find_iterator != tertiary_find_iterator->second.end()) {
					return quaternary_find_iterator->second;
				}
			}
		}
	}

	return this->get_civilization()->get_character_title_name(title_type, this->get_type(), government_type, tier, gender);
}

std::string faction::get_name(const government_type government_type, const faction_tier tier) const
{
	if (this->uses_short_name(government_type)) {
		return this->get_titled_name(government_type, tier);
	}

	return this->get_name();
}

std::string faction::get_titled_name(const government_type government_type, const faction_tier tier) const
{
	if (this->uses_simple_name()) {
		return this->get_name();
	}

	const std::string title_name = std::string(this->get_title_name(government_type, tier));
	if (this->uses_short_name(government_type)) {
		return this->get_adjective() + " " + title_name;
	} else {
		std::string str = title_name + " of ";
		if (this->definite_article) {
			str += "the ";
		}
		str += this->get_name();
		return str;
	}
}

int faction::get_force_type_weight(const ai_force_type force_type) const
{
	if (force_type == ai_force_type::none) {
		throw std::runtime_error("Error in faction::get_force_type_weight: the force_type is none.");
	}
	
	const auto find_iterator = this->ai_force_type_weights.find(force_type);
	if (find_iterator != this->ai_force_type_weights.end()) {
		return find_iterator->second;
	}
	
	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->get_force_type_weight(force_type);
	}
	
	return this->civilization->get_force_type_weight(force_type);
}

CCurrency *faction::GetCurrency() const
{
	if (this->Currency != nullptr) {
		return this->Currency;
	}
	
	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->GetCurrency();
	}
	
	return this->civilization->GetCurrency();
}

bool faction::is_government_type_valid(const government_type government_type) const
{
	if (this->get_civilization()->get_conditions() != nullptr && !this->get_civilization()->get_conditions()->check(government_type)) {
		return false;
	}

	if (this->get_preconditions() != nullptr && !this->get_preconditions()->check(government_type)) {
		return false;
	}

	if (this->get_conditions() != nullptr && !this->get_conditions()->check(government_type)) {
		return false;
	}

	return true;
}

bool faction::uses_simple_name() const
{
	return this->simple_name || this->get_type() != faction_type::polity;
}

bool faction::uses_short_name(const government_type government_type) const
{
	if (this->get_type() == faction_type::polity && government_type == government_type::tribe) {
		return true;
	}

	return this->short_name;
}

bool faction::uses_definite_article(const government_type government_type) const
{
	if (this->uses_short_name(government_type)) {
		return true;
	}

	return this->definite_article;
}

const std::vector<std::unique_ptr<ai_force_template>> &faction::get_ai_force_templates(const ai_force_type force_type) const
{
	if (force_type == ai_force_type::none) {
		throw std::runtime_error("Error in faction::get_ai_force_templates: the force_type is none.");
	}
	
	const auto find_iterator = this->ai_force_templates.find(force_type);
	if (find_iterator != this->ai_force_templates.end()) {
		return find_iterator->second;
	}
	
	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->get_ai_force_templates(force_type);
	}
	
	return this->civilization->get_ai_force_templates(force_type);
}

const std::vector<std::unique_ptr<CAiBuildingTemplate>> &faction::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->GetAiBuildingTemplates();
	}
	
	return this->civilization->GetAiBuildingTemplates();
}

const name_generator *faction::get_unit_class_name_generator(const unit_class *unit_class) const
{
	const auto find_iterator = this->unit_class_name_generators.find(unit_class);
	if (find_iterator != this->unit_class_name_generators.end() && find_iterator->second->get_name_count() >= name_generator::minimum_name_count) {
		return find_iterator->second.get();
	}

	if (unit_class->is_ship() && this->ship_name_generator != nullptr && this->ship_name_generator->get_name_count() >= name_generator::minimum_name_count) {
		return this->ship_name_generator.get();
	}

	return this->get_civilization()->get_unit_class_name_generator(unit_class);
}

const unit_type *faction::get_class_unit_type(const unit_class *unit_class) const
{
	if (unit_class == nullptr) {
		return nullptr;
	}

	const auto find_iterator = this->class_unit_types.find(unit_class);
	if (find_iterator != this->class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->get_class_unit_type(unit_class);
	}

	return this->civilization->get_class_unit_type(unit_class);
}

bool faction::is_class_unit_type(const unit_type *unit_type) const
{
	return unit_type == this->get_class_unit_type(unit_type->get_unit_class());
}

const CUpgrade *faction::get_class_upgrade(const upgrade_class *upgrade_class) const
{
	if (upgrade_class == nullptr) {
		return nullptr;
	}

	auto find_iterator = this->class_upgrades.find(upgrade_class);
	if (find_iterator != this->class_upgrades.end()) {
		return find_iterator->second;
	}

	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->get_class_upgrade(upgrade_class);
	}

	return this->get_civilization()->get_class_upgrade(upgrade_class);
}

const std::vector<CFiller> &faction::get_ui_fillers() const
{
	if (!this->ui_fillers.empty()) {
		return this->ui_fillers;
	}

	if (this->get_parent_faction() != nullptr) {
		return this->get_parent_faction()->get_ui_fillers();
	}

	return this->get_civilization()->get_ui_fillers();
}

std::string faction::get_requirements_string() const
{
	if (this->get_upgrade() != nullptr) {
		if (!this->get_upgrade()->get_requirements_string().empty()) {
			return this->get_upgrade()->get_requirements_string();
		}

		if (this->get_upgrade()->get_conditions() != nullptr) {
			return "\n" + this->get_upgrade()->get_conditions()->get_conditions_string(1, false);
		}
	}

	std::string str;

	if (this->has_neutral_type()) {
		if (this->get_neutral_site_conditions() != nullptr) {
			str += "\n" + this->get_neutral_site_conditions()->get_conditions_string(1, false);
		}

		if (this->get_neutral_site_spawn_conditions() != nullptr) {
			str += "\n" + this->get_neutral_site_spawn_conditions()->get_conditions_string(1, false);
		}
	} else {
		if (this->get_civilization()->get_conditions() != nullptr) {
			str += "\n" + this->get_civilization()->get_conditions()->get_conditions_string(1, false);
		}

		if (this->get_conditions() != nullptr) {
			str += "\n" + this->get_conditions()->get_conditions_string(1, false);
		}
	}

	return str;
}

void faction::remove_dynasty(const wyrmgus::dynasty *dynasty)
{
	vector::remove(this->dynasties, dynasty);
}

std::vector<site *> faction::get_all_neutral_target_sites() const
{
	mutable_site_set target_site_set = container::to_set<std::vector<site *>, mutable_site_set>(this->get_neutral_target_sites());

	for (const region *target_region : this->get_neutral_target_regions()) {
		set::merge(target_site_set, target_region->get_sites());
	}

	return container::to_vector(std::move(target_site_set));
}

}
