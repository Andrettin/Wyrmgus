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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "stratagus.h"

#include "civilization.h"

#include "ai/ai_force_template.h"
#include "ai/ai_force_type.h"
#include "character.h"
#include "civilization_group.h"
#include "database/defines.h"
#include "faction.h"
#include "faction_type.h"
#include "government_type.h"
#include "player.h"
#include "script.h"
#include "sound/sound.h"
#include "time/calendar.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/cursor.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/exception_util.h"
#include "util/string_util.h"
#include "util/string_conversion_util.h"
#include "video/video.h"

namespace wyrmgus {

civilization::civilization(const std::string &identifier) : civilization_base(identifier)
{
}

civilization::~civilization()
{
}

void civilization::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "adjective") {
		this->Adjective = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void civilization::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "button_icons") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const ButtonCmd button_action = GetButtonActionIdByName(key);
			if (button_action != ButtonCmd::None) {
				PlayerRaces.ButtonIcons[this->ID][button_action].Name = value;
				PlayerRaces.ButtonIcons[this->ID][button_action].Icon = nullptr;
				PlayerRaces.ButtonIcons[this->ID][button_action].Load();
			} else {
				exception::throw_with_trace(std::runtime_error("Button action \"" + key + "\" doesn't exist."));
			}
		});
	} else if (tag == "unit_sounds") {
		if (this->unit_sound_set == nullptr) {
			this->unit_sound_set = std::make_unique<wyrmgus::unit_sound_set>();
		}

		database::process_sml_data(this->unit_sound_set, scope);
	} else if (tag == "not_enough_resource_sounds") {
		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			const sound *sound = sound::get(value);
			this->not_enough_resource_sounds[resource] = sound;
		});
	} else if (tag == "title_names") {
		faction::process_title_names(this->title_names, scope);
	} else if (tag == "character_title_names") {
		scope.for_each_child([&](const sml_data &child_scope) {
			this->process_character_title_name_scope(child_scope);
		});
	} else if (tag == "ui_fillers") {
		this->ui_fillers.clear();

		scope.for_each_child([&](const sml_data &child_scope) {
			CFiller filler = CFiller();
			const std::string filler_file = child_scope.get_property_value("file");
			if (filler_file.empty()) {
				exception::throw_with_trace(std::runtime_error("Filler graphic file is empty."));
			}
			filler.G = CGraphic::New(filler_file);

			const QPoint pos = child_scope.get_child("pos").to_point();
			filler.X = pos.x();
			filler.Y = pos.y();
			this->ui_fillers.push_back(std::move(filler));
		});
	} else if (tag == "force_type_weights") {
		this->ai_force_type_weights.clear();

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const ai_force_type force_type = string_to_ai_force_type(key);
			this->ai_force_type_weights[force_type] = std::stoi(value);
		});
	} else if (tag == "force_templates") {
		scope.for_each_child([&](const sml_data &child_scope) {
			auto force_template = std::make_unique<ai_force_template>();
			database::process_sml_data(force_template, child_scope);
			this->ai_force_templates[force_template->get_force_type()].push_back(std::move(force_template));
		});
	} else if (tag == "ai_building_templates") {
		scope.for_each_child([&](const sml_data &child_scope) {
			auto building_template = std::make_unique<CAiBuildingTemplate>();

			child_scope.for_each_property([&](const sml_property &property) {
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
					exception::throw_with_trace(std::runtime_error("Invalid AI building template property: " + child_scope.get_tag() + "."));
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
		civilization_base::process_sml_scope(scope);
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

		if (this->get_interface().empty()) {
			this->interface = parent_civilization->interface;
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

		//unit sounds
		if (parent_civilization->unit_sound_set != nullptr) {
			if (this->unit_sound_set == nullptr) {
				this->unit_sound_set = std::make_unique<wyrmgus::unit_sound_set>();
			}

			if (this->unit_sound_set->Selected.Name.empty()) {
				this->unit_sound_set->Selected = parent_civilization->unit_sound_set->Selected;
			}
			if (this->unit_sound_set->Acknowledgement.Name.empty()) {
				this->unit_sound_set->Acknowledgement = parent_civilization->unit_sound_set->Acknowledgement;
			}
			if (this->unit_sound_set->Attack.Name.empty()) {
				this->unit_sound_set->Attack = parent_civilization->unit_sound_set->Attack;
			}
			if (this->unit_sound_set->Build.Name.empty()) {
				this->unit_sound_set->Build = parent_civilization->unit_sound_set->Build;
			}
			if (this->unit_sound_set->Ready.Name.empty()) {
				this->unit_sound_set->Ready = parent_civilization->unit_sound_set->Ready;
			}
			if (this->unit_sound_set->Repair.Name.empty()) {
				this->unit_sound_set->Repair = parent_civilization->unit_sound_set->Repair;
			}
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (this->unit_sound_set->Harvest[j].Name.empty()) {
					this->unit_sound_set->Harvest[j] = parent_civilization->unit_sound_set->Harvest[j];
				}
			}
			if (this->unit_sound_set->Help.Name.empty()) {
				this->unit_sound_set->Help = parent_civilization->unit_sound_set->Help;
			}
			if (this->unit_sound_set->Dead[ANIMATIONS_DEATHTYPES].Name.empty()) {
				this->unit_sound_set->Dead[ANIMATIONS_DEATHTYPES] = parent_civilization->unit_sound_set->Dead[ANIMATIONS_DEATHTYPES];
			}
		}

		if (this->help_town_sound == nullptr) {
			this->help_town_sound = parent_civilization->help_town_sound;
		}

		if (this->work_complete_sound == nullptr) {
			this->work_complete_sound = parent_civilization->work_complete_sound;
		}

		if (this->research_complete_sound == nullptr) {
			this->research_complete_sound = parent_civilization->research_complete_sound;
		}

		if (this->not_enough_food_sound == nullptr) {
			this->not_enough_food_sound = parent_civilization->not_enough_food_sound;
		}

		for (const auto &kv_pair : parent_civilization->not_enough_resource_sounds) {
			if (!this->not_enough_resource_sounds.contains(kv_pair.first)) {
				this->not_enough_resource_sounds[kv_pair.first] = kv_pair.second;
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

	if (this->unit_sound_set != nullptr) {
		this->unit_sound_set->map_sounds();
	}

	civilization_base::initialize();
}

void civilization::check() const
{
	if (this != defines::get()->get_neutral_civilization()) {
		if (this->get_species() == nullptr) {
			exception::throw_with_trace(std::runtime_error("Civilization \"" + this->get_identifier() + "\" has no species."));
		}

		if (this->get_language() == nullptr) {
			exception::throw_with_trace(std::runtime_error("Civilization \"" + this->get_identifier() + "\" has no language."));
		}
	}

	for (const auto &kv_pair : this->ai_force_templates) {
		for (const std::unique_ptr<ai_force_template> &force_template : kv_pair.second) {
			force_template->check();
		}
	}

	data_entry::check();
}

int civilization::GetUpgradePriority(const CUpgrade *upgrade) const
{
	if (!upgrade) {
		fprintf(stderr, "Error in civilization::GetUpgradePriority: the upgrade is null.\n");
	}
	
	if (this->UpgradePriorities.find(upgrade) != this->UpgradePriorities.end()) {
		return this->UpgradePriorities.find(upgrade)->second;
	}
	
	return 100;
}

int civilization::get_force_type_weight(const ai_force_type force_type) const
{
	if (force_type == ai_force_type::none) {
		exception::throw_with_trace(std::runtime_error("Error in civilization::get_force_type_weight: the force_type is none."));
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

cursor *civilization::get_cursor(const cursor_type type) const
{
	auto find_iterator = this->cursors.find(type);
	if (find_iterator != this->cursors.end()) {
		return find_iterator->second;
	}

	if (this->get_parent_civilization() != nullptr) {
		return this->get_parent_civilization()->get_cursor(type);
	}

	return cursor::get_cursor_by_type(type);
}

std::string_view civilization::get_title_name(const government_type government_type, const faction_tier tier) const
{
	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator != this->title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	switch (government_type) {
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
		case character_title::head_of_state:
			switch (faction_type) {
				case faction_type::tribe:
					if (gender == gender::female) {
						return "Chieftess";
					} else {
						return "Chieftain";
					}
				case faction_type::polity:
					switch (government_type) {
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
		case character_title::head_of_government:
			return "Prime Minister";
		case character_title::education_minister:
			//return "Education Minister"; //education minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Master Educator";
		case character_title::finance_minister:
			//return "Finance Minister"; //finance minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Treasurer";
		case character_title::foreign_minister:
			//return "Foreign Minister"; //foreign minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Chancellor";
		case character_title::intelligence_minister:
			//return "Intelligence Minister"; //intelligence minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Spymaster";
		case character_title::interior_minister:
			//return "Interior Minister"; //interior minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "High Constable";
		case character_title::justice_minister:
			//return "Justice Minister"; //justice minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Master of Laws";
		case character_title::war_minister:
			//return "War Minister"; //war minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Marshal";
		case wyrmgus::character_title::governor:
			return "Governor";
		case character_title::mayor:
			return "Mayor";
		default:
			break;
	}

	return string::empty_str;
}

void civilization::process_character_title_name_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const wyrmgus::character_title title_type = GetCharacterTitleIdByName(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		this->process_character_title_name_scope(title_type, child_scope);
	});
}

void civilization::process_character_title_name_scope(const character_title title_type, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const faction_type faction_type = string_to_faction_type(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		faction::process_character_title_name_scope(this->character_title_names[title_type][faction_type], child_scope);
	});

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const wyrmgus::government_type government_type = string_to_government_type(key);
		character_title_names[title_type][faction_type][government_type][faction_tier::none][gender::none] = value;
	});
}

const std::vector<std::unique_ptr<ai_force_template>> &civilization::get_ai_force_templates(const ai_force_type force_type) const
{
	static const std::vector<std::unique_ptr<ai_force_template>> empty_vector;

	if (force_type == ai_force_type::none) {
		exception::throw_with_trace(std::runtime_error("Error in civilization::get_ai_force_templates: the force_type is none."));
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

unit_type *civilization::get_class_unit_type(const unit_class *unit_class) const
{
	if (unit_class == nullptr) {
		return nullptr;
	}

	auto find_iterator = this->class_unit_types.find(unit_class);
	if (find_iterator != this->class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_class_unit_type(unit_class);
	}

	return nullptr;
}

CUpgrade *civilization::get_class_upgrade(const upgrade_class *upgrade_class) const
{
	if (upgrade_class == nullptr) {
		return nullptr;
	}

	auto find_iterator = this->class_upgrades.find(upgrade_class);
	if (find_iterator != this->class_upgrades.end()) {
		return find_iterator->second;
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_class_upgrade(upgrade_class);
	}

	return nullptr;
}

}
