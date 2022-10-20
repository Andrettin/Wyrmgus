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

#include "player/civilization.h"

#include "ai/ai_force_template.h"
#include "ai/ai_force_type.h"
#include "ai/ai_local.h"
#include "character.h"
#include "character_title.h"
#include "database/defines.h"
#include "name_generator.h"
#include "player/civilization_group.h"
#include "player/faction.h"
#include "player/faction_tier.h"
#include "player/faction_type.h"
#include "player/government_type.h"
#include "player/player.h"
#include "script/condition/and_condition.h"
#include "script.h"
#include "time/calendar.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/cursor.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_class.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/path_util.h"
#include "util/string_util.h"
#include "util/string_conversion_util.h"
#include "util/vector_util.h"
#include "video/video.h"

namespace wyrmgus {

civilization::civilization(const std::string &identifier) : civilization_base(identifier)
{
}

civilization::~civilization()
{
}

void civilization::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "button_icons") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const ButtonCmd button_action = GetButtonActionIdByName(key);
			if (button_action != ButtonCmd::None) {
				PlayerRaces.ButtonIcons[this->ID][button_action].Name = value;
				PlayerRaces.ButtonIcons[this->ID][button_action].Icon = nullptr;
				PlayerRaces.ButtonIcons[this->ID][button_action].Load();
			} else {
				throw std::runtime_error("Button action \"" + key + "\" doesn't exist.");
			}
		});
	} else if (tag == "title_names") {
		faction::process_title_names(this->title_names, scope);
	} else if (tag == "character_title_names") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			this->process_character_title_name_scope(child_scope);
		});
	} else if (tag == "ui_fillers") {
		this->ui_fillers.clear();

		scope.for_each_child([&](const gsml_data &child_scope) {
			CFiller filler = CFiller();
			const std::string filler_file = child_scope.get_property_value("file");
			if (filler_file.empty()) {
				throw std::runtime_error("Filler graphic file is empty.");
			}
			filler.G = CGraphic::New(filler_file);

			const QPoint pos = child_scope.get_child("pos").to_point();
			filler.X = pos.x();
			filler.Y = pos.y();
			this->ui_fillers.push_back(std::move(filler));
		});
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<CPlayer>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "force_type_weights") {
		this->ai_force_type_weights.clear();

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const ai_force_type force_type = string_to_ai_force_type(key);
			this->ai_force_type_weights[force_type] = std::stoi(value);
		});
	} else if (tag == "force_templates") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			auto force_template = std::make_unique<ai_force_template>();
			database::process_gsml_data(force_template, child_scope);
			this->ai_force_templates[force_template->get_force_type()].push_back(std::move(force_template));
		});
	} else if (tag == "ai_building_templates") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			auto building_template = std::make_unique<CAiBuildingTemplate>();

			child_scope.for_each_property([&](const gsml_property &property) {
				const std::string &key = property.get_key();
				const std::string &value = property.get_value();

				if (key == "unit_class") {
					const unit_class *unit_class = unit_class::get(value);
					building_template->set_unit_class(unit_class);
				} else if (key == "priority") {
					building_template->set_priority(std::stoi(value));
				} else if (key == "per_settlement") {
					building_template->set_per_settlement(string::to_bool(value));
				} else {
					throw std::runtime_error("Invalid AI building template property: " + child_scope.get_tag() + ".");
				}
			});

			this->AiBuildingTemplates.push_back(std::move(building_template));
		});
	} else if (tag == "develops_from") {
		for (const std::string &value : values) {
			civilization *other_civilization = civilization::get(value);
			this->develops_from.push_back(other_civilization);
			other_civilization->develops_to.push_back(this);
		}
	} else {
		civilization_base::process_gsml_scope(scope);
	}
}

void civilization::initialize()
{
	std::sort(this->AiBuildingTemplates.begin(), this->AiBuildingTemplates.end(), [](const std::unique_ptr<CAiBuildingTemplate> &a, const std::unique_ptr<CAiBuildingTemplate> &b) {
		return a->get_priority() > b->get_priority();
	});

	for (auto &kv_pair : this->ai_force_templates) {
		std::sort(kv_pair.second.begin(), kv_pair.second.end(), [](const std::unique_ptr<ai_force_template> &a, const std::unique_ptr<ai_force_template> &b) {
			return a->get_priority() > b->get_priority();
		});
	}

	if (this->get_parent_civilization() != nullptr) {
		if (!this->get_parent_civilization()->is_initialized()) {
			this->get_parent_civilization()->initialize();
		}

		const civilization *parent_civilization = this->get_parent_civilization();
		const int parent_civilization_id = parent_civilization->ID;

		if (this->interface_style == nullptr) {
			this->interface_style = parent_civilization->interface_style;
		}

		if (this->upgrade == nullptr && parent_civilization->get_upgrade() != nullptr) { //if the civilization has no civilization upgrade, inherit that of its parent civilization
			this->upgrade = parent_civilization->get_upgrade();
		}

		//inherit button icons from the parent civilization, for button actions for which none are specified
		for (std::map<ButtonCmd, IconConfig>::iterator iterator = PlayerRaces.ButtonIcons[parent_civilization_id].begin(); iterator != PlayerRaces.ButtonIcons[parent_civilization_id].end(); ++iterator) {
			if (PlayerRaces.ButtonIcons[this->ID].find(iterator->first) == PlayerRaces.ButtonIcons[this->ID].end()) {
				PlayerRaces.ButtonIcons[this->ID][iterator->first] = iterator->second;
			}
		}
	}

	if (PlayerRaces.ButtonIcons[this->ID].find(ButtonCmd::Move) != PlayerRaces.ButtonIcons[this->ID].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 1,\n";
		button_definition += "\tAction = \"move\",\n";
		button_definition += "\tPopup = \"popup_commands\",\n";
		button_definition += "\tKey = \"m\",\n";
		button_definition += "\tHint = _(\"~!Move\"),\n";
		button_definition += "\tForUnit = {\"" + this->get_identifier() + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}

	if (PlayerRaces.ButtonIcons[this->ID].find(ButtonCmd::Stop) != PlayerRaces.ButtonIcons[this->ID].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 2,\n";
		button_definition += "\tAction = \"stop\",\n";
		button_definition += "\tPopup = \"popup_commands\",\n";
		button_definition += "\tKey = \"s\",\n";
		button_definition += "\tHint = _(\"~!Stop\"),\n";
		button_definition += "\tForUnit = {\"" + this->get_identifier() + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}

	if (PlayerRaces.ButtonIcons[this->ID].find(ButtonCmd::Attack) != PlayerRaces.ButtonIcons[this->ID].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 3,\n";
		button_definition += "\tAction = \"attack\",\n";
		button_definition += "\tPopup = \"popup_commands\",\n";
		button_definition += "\tKey = \"a\",\n";
		button_definition += "\tHint = _(\"~!Attack\"),\n";
		button_definition += "\tForUnit = {\"" + this->get_identifier() + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}

	if (PlayerRaces.ButtonIcons[this->ID].find(ButtonCmd::Patrol) != PlayerRaces.ButtonIcons[this->ID].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 4,\n";
		button_definition += "\tAction = \"patrol\",\n";
		button_definition += "\tPopup = \"popup_commands\",\n";
		button_definition += "\tKey = \"p\",\n";
		button_definition += "\tHint = _(\"~!Patrol\"),\n";
		button_definition += "\tForUnit = {\"" + this->get_identifier() + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}

	if (PlayerRaces.ButtonIcons[this->ID].find(ButtonCmd::StandGround) != PlayerRaces.ButtonIcons[this->ID].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 5,\n";
		button_definition += "\tAction = \"stand-ground\",\n";
		button_definition += "\tPopup = \"popup_commands\",\n";
		button_definition += "\tKey = \"t\",\n";
		button_definition += "\tHint = _(\"S~!tand Ground\"),\n";
		button_definition += "\tForUnit = {\"" + this->get_identifier() + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}

	civilization_base::initialize();
}

void civilization::check() const
{
	if (this != defines::get()->get_neutral_civilization()) {
		if (this->get_species() == nullptr) {
			throw std::runtime_error("Civilization \"" + this->get_identifier() + "\" has no species.");
		}

		if (this->get_language() == nullptr) {
			throw std::runtime_error("Civilization \"" + this->get_identifier() + "\" has no language.");
		}
	}

	for (const civilization *other_civilization : this->get_develops_from()) {
		if (vector::contains(other_civilization->get_develops_from(), this)) {
			throw std::runtime_error("Civilizations \"" + this->get_identifier() + "\" and \"" + other_civilization->get_identifier() + "\" are both set to develop from each other.");
		}
	}

	for (const auto &kv_pair : this->ai_force_templates) {
		for (const std::unique_ptr<ai_force_template> &force_template : kv_pair.second) {
			force_template->check();
		}
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	civilization_base::check();
}

std::string civilization::get_encyclopedia_text() const
{
	std::string text;

	std::vector<const civilization *> develops_from = this->get_develops_from();
	std::sort(develops_from.begin(), develops_from.end(), named_data_entry::compare_encyclopedia_entries);

	if (!develops_from.empty()) {
		std::string develops_from_text = "Develops From: ";
		for (size_t i = 0; i < develops_from.size(); ++i) {
			if (i > 0) {
				develops_from_text += ", ";
			}

			const civilization *civilization = develops_from[i];
			develops_from_text += civilization->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(develops_from_text));
	}

	std::vector<const civilization *> develops_to = this->get_develops_to();
	std::sort(develops_to.begin(), develops_to.end(), named_data_entry::compare_encyclopedia_entries);

	if (!develops_to.empty()) {
		std::string develops_to_text = "Develops To: ";
		for (size_t i = 0; i < develops_to.size(); ++i) {
			if (i > 0) {
				develops_to_text += ", ";
			}

			const civilization *civilization = develops_to[i];
			develops_to_text += civilization->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(develops_to_text));
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	return text;
}

int civilization::get_force_type_weight(const ai_force_type force_type) const
{
	if (force_type == ai_force_type::none) {
		throw std::runtime_error("Error in civilization::get_force_type_weight: the force_type is none.");
	}
	
	const auto find_iterator = this->ai_force_type_weights.find(force_type);
	if (find_iterator != this->ai_force_type_weights.end()) {
		return find_iterator->second;
	}
	
	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_force_type_weight(force_type);
	}
	
	return 1;
}

calendar *civilization::get_calendar() const
{
	if (this->calendar != nullptr) {
		return this->calendar;
	}
	
	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_calendar();
	}
	
	return nullptr;
}

CCurrency *civilization::GetCurrency() const
{
	if (this->Currency) {
		return this->Currency;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetCurrency();
	}
	
	return nullptr;
}

const std::filesystem::path &civilization::get_encyclopedia_background_file() const
{
	if (!this->encyclopedia_background_file.empty()) {
		return this->encyclopedia_background_file;
	}

	if (this->get_parent_civilization() != nullptr) {
		return this->get_parent_civilization()->get_encyclopedia_background_file();
	}

	return this->encyclopedia_background_file;
}

QString civilization::get_encyclopedia_background_file_qstring() const
{
	return path::to_qstring(this->get_encyclopedia_background_file());
}

void civilization::set_encyclopedia_background_file(const std::filesystem::path &filepath)
{
	this->encyclopedia_background_file = database::get()->get_graphics_filepath(filepath);
}

std::string_view civilization::get_title_name(const government_type government_type, const faction_tier tier) const
{
	const auto find_iterator = this->title_names.find(government_type);
	if (find_iterator != this->title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	switch (government_type) {
		case government_type::tribe:
			switch (tier) {
				case faction_tier::barony:
				case faction_tier::viscounty:
					return "Band";
				case faction_tier::county:
				case faction_tier::marquisate:
					return "Clan";
				case faction_tier::duchy:
				case faction_tier::grand_duchy:
				case faction_tier::kingdom:
					return "Tribe";
				case faction_tier::empire:
					return "Horde";
				default:
					break;
			}
			break;
		case government_type::monarchy:
			switch (tier) {
				case faction_tier::barony:
					return "Barony";
				case faction_tier::viscounty:
					return "Viscounty";
				case faction_tier::county:
					return "County";
				case faction_tier::marquisate:
					return "Marquisate";
				case faction_tier::duchy:
					return "Duchy";
				case faction_tier::grand_duchy:
					return "Grand Duchy";
				case faction_tier::kingdom:
					return "Kingdom";
				case faction_tier::empire:
					return "Empire";
				default:
					break;
			}
			break;
		case government_type::republic:
			return "Republic";
		case government_type::theocracy:
			return "Theocracy";
		default:
			break;
	}

	return string::empty_str;
}

std::string_view civilization::get_character_title_name(const character_title title_type, const faction_type faction_type, wyrmgus::government_type government_type, const faction_tier tier, const gender gender) const
{
	auto find_iterator = this->character_title_names.find(title_type);
	if (find_iterator != this->character_title_names.end()) {
		auto secondary_find_iterator = find_iterator->second.find(faction_type);
		if (secondary_find_iterator == find_iterator->second.end()) {
			secondary_find_iterator = find_iterator->second.find(faction_type::none);
		}

		if (secondary_find_iterator != find_iterator->second.end()) {
			auto tertiary_find_iterator = secondary_find_iterator->second.find(government_type);
			if (tertiary_find_iterator == secondary_find_iterator->second.end()) {
				tertiary_find_iterator = secondary_find_iterator->second.find(government_type::none);
			}

			if (tertiary_find_iterator != secondary_find_iterator->second.end()) {
				auto quaternary_find_iterator = tertiary_find_iterator->second.find(tier);
				if (quaternary_find_iterator == tertiary_find_iterator->second.end()) {
					quaternary_find_iterator = tertiary_find_iterator->second.find(faction_tier::none);
				}

				if (quaternary_find_iterator != tertiary_find_iterator->second.end()) {
					auto quinary_find_iterator = quaternary_find_iterator->second.find(gender);
					if (quinary_find_iterator == quaternary_find_iterator->second.end()) {
						quinary_find_iterator = quaternary_find_iterator->second.find(gender::none);
					}

					if (quinary_find_iterator != quaternary_find_iterator->second.end()) {
						return quinary_find_iterator->second;
					}
				}
			}
		}
	}

	switch (title_type) {
		case character_title::ruler:
			switch (faction_type) {
				case faction_type::tribe:
					if (gender == gender::female) {
						return "Chieftess";
					} else {
						return "Chieftain";
					}
				case faction_type::polity:
					switch (government_type) {
						case government_type::tribe:
							if (gender == gender::female) {
								return "Chieftess";
							} else {
								return "Chieftain";
							}
						case government_type::monarchy:
							switch (tier) {
								case faction_tier::barony:
									if (gender == gender::female) {
										return "Baroness";
									} else {
										return "Baron";
									}
								case faction_tier::viscounty:
									if (gender == gender::female) {
										return "Viscountess";
									} else {
										return "Viscount";
									}
								case faction_tier::county:
									if (gender == gender::female) {
										return "Countess";
									} else {
										return "Count";
									}
								case faction_tier::marquisate:
									if (gender == gender::female) {
										return "Marquise";
									} else {
										return "Marquis";
									}
								case faction_tier::duchy:
									if (gender == gender::female) {
										return "Duchess";
									} else {
										return "Duke";
									}
								case faction_tier::grand_duchy:
									if (gender == gender::female) {
										return "Grand Duchess";
									} else {
										return "Grand Duke";
									}
								case faction_tier::kingdom:
									if (gender == gender::female) {
										return "Queen";
									} else {
										return "King";
									}
								case faction_tier::empire:
									if (gender == gender::female) {
										return "Empress";
									} else {
										return "Emperor";
									}
								default:
									break;
							}
							break;
						case government_type::republic:
							return "Consul";
						case government_type::theocracy:
							if (gender == gender::female) {
								return "High Priestess";
							} else {
								return "High Priest";
							}
						default:
							break;
					}
					break;
				default:
					break;
			}
			break;
		case character_title::chancellor:
			return "Chancellor";
		case character_title::marshal:
			return "Marshal";
		case character_title::treasurer:
			return "Treasurer";
		default:
			break;
	}

	return string::empty_str;
}

void civilization::process_character_title_name_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const character_title title_type = enum_converter<character_title>::to_enum(tag);

	scope.for_each_child([&](const gsml_data &child_scope) {
		this->process_character_title_name_scope(title_type, child_scope);
	});
}

void civilization::process_character_title_name_scope(const character_title title_type, const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const faction_type faction_type = enum_converter<wyrmgus::faction_type>::to_enum(tag);

	scope.for_each_child([&](const gsml_data &child_scope) {
		faction::process_character_title_name_scope(this->character_title_names[title_type][faction_type], child_scope);
	});

	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const wyrmgus::government_type government_type = enum_converter<wyrmgus::government_type>::to_enum(key);
		character_title_names[title_type][faction_type][government_type][faction_tier::none][gender::none] = value;
	});
}

const std::vector<std::unique_ptr<ai_force_template>> &civilization::get_ai_force_templates(const ai_force_type force_type) const
{
	static const std::vector<std::unique_ptr<ai_force_template>> empty_vector;

	if (force_type == ai_force_type::none) {
		throw std::runtime_error("Error in civilization::get_ai_force_templates: the force_type is none.");
	}
	
	const auto find_iterator = this->ai_force_templates.find(force_type);
	if (find_iterator != this->ai_force_templates.end()) {
		return find_iterator->second;
	}
	
	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_ai_force_templates(force_type);
	}
	
	return empty_vector;
}

const std::vector<std::unique_ptr<CAiBuildingTemplate>> &civilization::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetAiBuildingTemplates();
	}
	
	return this->AiBuildingTemplates;
}

cursor *civilization::get_cursor(const cursor_type type) const
{
	cursor *cursor = civilization_base::get_cursor(type);
	if (cursor != nullptr) {
		return cursor;
	}

	if (this->get_parent_civilization() != nullptr) {
		return this->get_parent_civilization()->get_cursor(type);
	}

	return cursor::get_cursor_by_type(type);
}

const unit_type *civilization::get_class_unit_type(const unit_class *unit_class) const
{
	if (unit_class == nullptr) {
		return nullptr;
	}

	const unit_type *unit_type = civilization_base::get_class_unit_type(unit_class);
	if (unit_type != nullptr) {
		return unit_type;
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_class_unit_type(unit_class);
	}

	return nullptr;
}

const CUpgrade *civilization::get_class_upgrade(const upgrade_class *upgrade_class) const
{
	if (upgrade_class == nullptr) {
		return nullptr;
	}

	const CUpgrade *upgrade = civilization_base::get_class_upgrade(upgrade_class);
	if (upgrade != nullptr) {
		return upgrade;
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_class_upgrade(upgrade_class);
	}

	return nullptr;
}

const population_type *civilization::get_class_population_type(const population_class *population_class) const
{
	if (population_class == nullptr) {
		return nullptr;
	}

	const population_type *population_type = civilization_base::get_class_population_type(population_class);

	if (population_type != nullptr) {
		return population_type;
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_class_population_type(population_class);
	}

	return nullptr;
}

bool civilization::is_tech_tree_entry(const unit_class *unit_class) const
{
	if (!unit_class->is_on_tech_tree()) {
		return false;
	}

	if (unit_class->get_tech_tree_parent_unit_class() != nullptr && !this->is_tech_tree_entry(unit_class->get_tech_tree_parent_unit_class())) {
		return false;
	}

	if (unit_class->get_tech_tree_parent_upgrade_class() != nullptr && !this->is_tech_tree_entry(unit_class->get_tech_tree_parent_upgrade_class())) {
		return false;
	}

	if (unit_class->get_preconditions() != nullptr && !unit_class->get_preconditions()->check(this)) {
		return false;
	}

	if (unit_class->get_conditions() != nullptr && !unit_class->get_conditions()->check(this)) {
		return false;
	}

	const unit_type *unit_type = this->get_class_unit_type(unit_class);

	if (unit_type == nullptr) {
		return false;
	}

	if (unit_type->get_preconditions() != nullptr && !unit_type->get_preconditions()->check(this)) {
		return false;
	}

	if (unit_type->get_conditions() != nullptr && !unit_type->get_conditions()->check(this)) {
		return false;
	}

	return true;
}

bool civilization::is_tech_tree_entry(const upgrade_class *upgrade_class) const
{
	if (!upgrade_class->is_on_tech_tree()) {
		return false;
	}

	if (upgrade_class->get_tech_tree_parent_unit_class() != nullptr && !this->is_tech_tree_entry(upgrade_class->get_tech_tree_parent_unit_class())) {
		return false;
	}

	if (upgrade_class->get_tech_tree_parent_upgrade_class() != nullptr && !this->is_tech_tree_entry(upgrade_class->get_tech_tree_parent_upgrade_class())) {
		return false;
	}

	if (upgrade_class->get_preconditions() != nullptr && !upgrade_class->get_preconditions()->check(this)) {
		return false;
	}

	if (upgrade_class->get_conditions() != nullptr && !upgrade_class->get_conditions()->check(this)) {
		return false;
	}

	const CUpgrade *upgrade = this->get_class_upgrade(upgrade_class);

	if (upgrade == nullptr) {
		return false;
	}

	if (upgrade->get_preconditions() != nullptr && !upgrade->get_preconditions()->check(this)) {
		return false;
	}

	if (upgrade->get_conditions() != nullptr && !upgrade->get_conditions()->check(this)) {
		return false;
	}

	return true;
}

QVariantList civilization::get_tech_tree_entries() const
{
	std::vector<const data_entry *> entries;

	for (const unit_class *unit_class : unit_class::get_all()) {
		if (!this->is_tech_tree_entry(unit_class)) {
			continue;
		}

		const unit_type *unit_type = this->get_class_unit_type(unit_class);
		entries.push_back(unit_type);
	}

	for (const upgrade_class *upgrade_class : upgrade_class::get_all()) {
		if (!this->is_tech_tree_entry(upgrade_class)) {
			continue;
		}

		const CUpgrade *upgrade = this->get_class_upgrade(upgrade_class);
		entries.push_back(upgrade);
	}

	std::sort(entries.begin(), entries.end(), [](const data_entry *lhs, const data_entry *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});

	return container::to_qvariant_list(entries);
}

QString civilization::generate_male_personal_name() const
{
	const name_generator *name_generator = this->get_personal_name_generator(gender::male);

	if (name_generator != nullptr) {
		return QString::fromStdString(name_generator->generate_name());
	}

	return "";
}

QString civilization::generate_female_personal_name() const
{
	const name_generator *name_generator = this->get_personal_name_generator(gender::female);

	if (name_generator != nullptr) {
		return QString::fromStdString(name_generator->generate_name());
	}

	return "";
}

QString civilization::generate_male_surname() const
{
	const name_generator *name_generator = this->get_surname_generator(gender::male);

	if (name_generator != nullptr) {
		return QString::fromStdString(name_generator->generate_name());
	}

	return "";
}

QString civilization::generate_female_surname() const
{
	const name_generator *name_generator = this->get_surname_generator(gender::female);

	if (name_generator != nullptr) {
		return QString::fromStdString(name_generator->generate_name());
	}

	return "";
}

QVariantList civilization::get_custom_hero_unit_types() const
{
	std::vector<const unit_type *> unit_types;

	for (const unit_class *unit_class : unit_class::get_all()) {
		const unit_type *unit_type = this->get_class_unit_type(unit_class);

		if (unit_type == nullptr) {
			continue;
		}

		if (!unit_type->BoolFlag[ORGANIC_INDEX].value) {
			//only organic units can be custom heroes
			continue;
		}

		if (unit_type->DefaultStat.Variables[LEVEL_INDEX].Value != 1) {
			//only level 1 units can be custom heroes
			continue;
		}

		if (AiHelpers.get_trainers(unit_type).empty() && AiHelpers.get_trainer_classes(unit_class).empty()) {
			//only allow units that can be trained
			continue;
		}

		unit_types.push_back(unit_type);
	}

	std::sort(unit_types.begin(), unit_types.end(), [](const unit_type *lhs, const unit_type *rhs) {
		return lhs->get_name() < rhs->get_name();
	});

	return container::to_qvariant_list(unit_types);
}

}
