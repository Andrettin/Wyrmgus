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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "civilization_group.h"
#include "civilization_supergroup.h"
#include "database/defines.h"
#include "government_type.h"
#include "player.h"
#include "time/calendar.h"
#include "ui/button.h"
#include "ui/cursor.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"
#include "video.h"

namespace stratagus {

civilization::~civilization()
{
	for (const auto &kv_pair : this->ForceTemplates) {
		for (size_t i = 0; i < kv_pair.second.size(); ++i) {
			delete kv_pair.second[i];
		}
	}
	
	for (size_t i = 0; i < this->AiBuildingTemplates.size(); ++i) {
		delete this->AiBuildingTemplates[i];
	}
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
				throw std::runtime_error("Button action \"" + key + "\" doesn't exist.");
			}
		});
	} else if (tag == "unit_sounds") {
		database::process_sml_data(this->UnitSounds, scope);
	} else if (tag == "title_names") {
		scope.for_each_child([&](const sml_data &child_scope) {
			this->process_title_name_scope(child_scope);
		});

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			government_type government_type = string_to_government_type(key);
			this->title_names[government_type][faction_tier::none] = value;
		});
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
				throw std::runtime_error("Filler graphic file is empty.");
			}
			filler.G = CGraphic::New(filler_file);

			const QPoint pos = child_scope.get_child("pos").to_point();
			filler.X = pos.x();
			filler.Y = pos.y();
			this->ui_fillers.push_back(std::move(filler));
		});
	} else if (tag == "force_type_weights") {
		this->ForceTypeWeights.clear();

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const ForceType force_type = GetForceTypeIdByName(key);
			this->ForceTypeWeights[force_type] = std::stoi(value);
		});
	} else if (tag == "force_templates") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			CForceTemplate *force = new CForceTemplate;

			child_scope.for_each_element([&](const sml_property &property) {
				if (property.get_key() == "force_type") {
					force->ForceType = GetForceTypeIdByName(property.get_value());
				} else if (property.get_key() == "priority") {
					force->Priority = std::stoi(property.get_value());
				} else if (property.get_key() == "weight") {
					force->Weight = std::stoi(property.get_value());
				} else {
					throw std::runtime_error("Invalid force template property: " + property.get_key() + ".");
				}
			}, [&](const sml_data &grandchild_scope) {
				if (grandchild_scope.get_tag() == "units") {
					grandchild_scope.for_each_property([&](const sml_property &property) {
						const std::string &key = property.get_key();
						const std::string &value = property.get_value();

						const unit_class *unit_class = unit_class::get(key);
						const int unit_quantity = std::stoi(value);
						force->add_unit(unit_class, unit_quantity);
					});
				} else {
					throw std::runtime_error("Invalid force template property: " + grandchild_scope.get_tag() + ".");
				}
			});

			this->ForceTemplates[force->ForceType].push_back(force);
		});

		for (auto &kv_pair : this->ForceTemplates) {
			std::sort(kv_pair.second.begin(), kv_pair.second.end(), [](CForceTemplate *a, CForceTemplate *b) {
				return a->Priority > b->Priority;
			});
		}
	} else if (tag == "ai_building_templates") {
		scope.for_each_child([&](const sml_data &child_scope) {
			CAiBuildingTemplate *building_template = new CAiBuildingTemplate;

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
					throw std::runtime_error("Invalid AI building template property: " + child_scope.get_tag() + ".");
				}
			});

			this->AiBuildingTemplates.push_back(building_template);
		});

		std::sort(this->AiBuildingTemplates.begin(), this->AiBuildingTemplates.end(), [](CAiBuildingTemplate *a, CAiBuildingTemplate *b) {
			return a->get_priority() > b->get_priority();
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
	if (this->get_group() != nullptr) {
		if (!this->get_group()->is_initialized()) {
			this->get_group()->initialize();
		}

		if (this->get_species() == nullptr) {
			this->set_species(this->get_group()->get_species());
		}

		this->get_group()->add_names_from(this);
		this->get_supergroup()->add_names_from(this);
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

		//inherit button icons from the parent civilization, for button actions which none are specified
		for (std::map<ButtonCmd, IconConfig>::iterator iterator = PlayerRaces.ButtonIcons[parent_civilization_id].begin(); iterator != PlayerRaces.ButtonIcons[parent_civilization_id].end(); ++iterator) {
			if (PlayerRaces.ButtonIcons[this->ID].find(iterator->first) == PlayerRaces.ButtonIcons[this->ID].end()) {
				PlayerRaces.ButtonIcons[this->ID][iterator->first] = iterator->second;
			}
		}

		//inherit historical upgrades from the parent civilization, if no historical data is given for that upgrade for this civilization
		for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = parent_civilization->HistoricalUpgrades.begin(); iterator != parent_civilization->HistoricalUpgrades.end(); ++iterator) {
			if (this->HistoricalUpgrades.find(iterator->first) == this->HistoricalUpgrades.end()) {
				this->HistoricalUpgrades[iterator->first] = iterator->second;
			}
		}

		//unit sounds
		if (this->UnitSounds.Selected.Name.empty()) {
			this->UnitSounds.Selected = parent_civilization->UnitSounds.Selected;
		}
		if (this->UnitSounds.Acknowledgement.Name.empty()) {
			this->UnitSounds.Acknowledgement = parent_civilization->UnitSounds.Acknowledgement;
		}
		if (this->UnitSounds.Attack.Name.empty()) {
			this->UnitSounds.Attack = parent_civilization->UnitSounds.Attack;
		}
		if (this->UnitSounds.Idle.Name.empty()) {
			this->UnitSounds.Idle = parent_civilization->UnitSounds.Idle;
		}
		if (this->UnitSounds.Hit.Name.empty()) {
			this->UnitSounds.Hit = parent_civilization->UnitSounds.Hit;
		}
		if (this->UnitSounds.Miss.Name.empty()) {
			this->UnitSounds.Miss = parent_civilization->UnitSounds.Miss;
		}
		if (this->UnitSounds.FireMissile.Name.empty()) {
			this->UnitSounds.FireMissile = parent_civilization->UnitSounds.FireMissile;
		}
		if (this->UnitSounds.Step.Name.empty()) {
			this->UnitSounds.Step = parent_civilization->UnitSounds.Step;
		}
		if (this->UnitSounds.StepDirt.Name.empty()) {
			this->UnitSounds.StepDirt = parent_civilization->UnitSounds.StepDirt;
		}
		if (this->UnitSounds.StepGrass.Name.empty()) {
			this->UnitSounds.StepGrass = parent_civilization->UnitSounds.StepGrass;
		}
		if (this->UnitSounds.StepGravel.Name.empty()) {
			this->UnitSounds.StepGravel = parent_civilization->UnitSounds.StepGravel;
		}
		if (this->UnitSounds.StepMud.Name.empty()) {
			this->UnitSounds.StepMud = parent_civilization->UnitSounds.StepMud;
		}
		if (this->UnitSounds.StepStone.Name.empty()) {
			this->UnitSounds.StepStone = parent_civilization->UnitSounds.StepStone;
		}
		if (this->UnitSounds.Used.Name.empty()) {
			this->UnitSounds.Used = parent_civilization->UnitSounds.Used;
		}
		if (this->UnitSounds.Build.Name.empty()) {
			this->UnitSounds.Build = parent_civilization->UnitSounds.Build;
		}
		if (this->UnitSounds.Ready.Name.empty()) {
			this->UnitSounds.Ready = parent_civilization->UnitSounds.Ready;
		}
		if (this->UnitSounds.Repair.Name.empty()) {
			this->UnitSounds.Repair = parent_civilization->UnitSounds.Repair;
		}
		for (unsigned int j = 0; j < MaxCosts; ++j) {
			if (this->UnitSounds.Harvest[j].Name.empty()) {
				this->UnitSounds.Harvest[j] = parent_civilization->UnitSounds.Harvest[j];
			}
		}
		if (this->UnitSounds.Help.Name.empty()) {
			this->UnitSounds.Help = parent_civilization->UnitSounds.Help;
		}
		if (this->UnitSounds.HelpTown.Name.empty()) {
			this->UnitSounds.HelpTown = parent_civilization->UnitSounds.HelpTown;
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

	data_entry::initialize();
}

void civilization::check() const
{
	if (this != defines::get()->get_neutral_civilization()) {
		if (this->get_species() == nullptr) {
			throw std::runtime_error("Civilization \"" + this->get_identifier() + "\" has no species.");
		}

		if (this->get_group() == nullptr) {
			throw std::runtime_error("Civilization \"" + this->get_identifier() + "\" has no civilization group.");
		}
	}

	data_entry::check();
}

civilization_supergroup *civilization::get_supergroup() const
{
	if (this->get_group() != nullptr) {
		return this->get_group()->get_supergroup();
	}

	return nullptr;
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

int civilization::GetForceTypeWeight(const ForceType force_type) const
{
	if (force_type == ForceType::None) {
		fprintf(stderr, "Error in civilization::GetForceTypeWeight: the force_type is -1.\n");
	}
	
	if (this->ForceTypeWeights.find(force_type) != this->ForceTypeWeights.end()) {
		return this->ForceTypeWeights.find(force_type)->second;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetForceTypeWeight(force_type);
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
				case faction_tier::county:
					return "County";
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

void civilization::process_title_name_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const government_type government_type = string_to_government_type(tag);

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const faction_tier tier = string_to_faction_tier(key);
		this->title_names[government_type][tier] = value;
	});
}

std::string_view civilization::get_character_title_name(const int title_type, const int faction_type, stratagus::government_type government_type, const faction_tier tier, const gender gender) const
{
	auto find_iterator = this->character_title_names.find(title_type);
	if (find_iterator != this->character_title_names.end()) {
		auto secondary_find_iterator = find_iterator->second.find(faction_type);
		if (secondary_find_iterator == find_iterator->second.end()) {
			secondary_find_iterator = find_iterator->second.find(FactionTypeNoFactionType);
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
		case CharacterTitleHeadOfState:
			switch (faction_type) {
				case FactionTypeTribe:
					if (gender == gender::female) {
						return "Chieftess";
					} else {
						return "Chieftain";
					}
				case FactionTypePolity:
					switch (government_type) {
						case government_type::monarchy:
							switch (tier) {
								case faction_tier::barony:
									if (gender == gender::female) {
										return "Baroness";
									} else {
										return "Baron";
									}
								case faction_tier::county:
									if (gender == gender::female) {
										return "Countess";
									} else {
										return "Count";
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
		case CharacterTitleHeadOfGovernment:
			return "Prime Minister";
		case CharacterTitleEducationMinister:
			//return "Education Minister"; //education minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Master Educator";
		case CharacterTitleFinanceMinister:
			//return "Finance Minister"; //finance minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Treasurer";
		case CharacterTitleForeignMinister:
			//return "Foreign Minister"; //foreign minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Chancellor";
		case CharacterTitleIntelligenceMinister:
			//return "Intelligence Minister"; //intelligence minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Spymaster";
		case CharacterTitleInteriorMinister:
			//return "Interior Minister"; //interior minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "High Constable";
		case CharacterTitleJusticeMinister:
			//return "Justice Minister"; //justice minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Master of Laws";
		case CharacterTitleWarMinister:
			//return "War Minister"; //war minister sounds too modern, considering the technology tree we have up to now only goes to the medieval era
			return "Marshal";
		case CharacterTitleGovernor:
			return "Governor";
		case CharacterTitleMayor:
			return "Mayor";
		default:
			break;
	}

	return string::empty_str;
}

void civilization::process_character_title_name_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const int title_type = GetCharacterTitleIdByName(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		this->process_character_title_name_scope(title_type, child_scope);
	});
}

void civilization::process_character_title_name_scope(const int title_type, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const int faction_type = GetFactionTypeIdByName(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		this->process_character_title_name_scope(title_type, faction_type, child_scope);
	});
}

void civilization::process_character_title_name_scope(const int title_type, const int faction_type, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const government_type government_type = string_to_government_type(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		this->process_character_title_name_scope(title_type, faction_type, government_type, child_scope);
	});
}

void civilization::process_character_title_name_scope(const int title_type, const int faction_type, const government_type government_type, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const faction_tier faction_tier = string_to_faction_tier(tag);

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = string_to_gender(key);
		this->character_title_names[title_type][faction_type][government_type][faction_tier][gender] = value;
	});
}

std::vector<CForceTemplate *> civilization::GetForceTemplates(const ForceType force_type) const
{
	if (force_type == ForceType::None) {
		fprintf(stderr, "Error in civilization::GetForceTemplates: the force_type is -1.\n");
	}
	
	if (this->ForceTemplates.find(force_type) != this->ForceTemplates.end()) {
		return this->ForceTemplates.find(force_type)->second;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetForceTemplates(force_type);
	}
	
	return std::vector<CForceTemplate *>();
}

std::vector<CAiBuildingTemplate *> civilization::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetAiBuildingTemplates();
	}
	
	return std::vector<CAiBuildingTemplate *>();
}

const std::map<gender, std::vector<std::string>> &civilization::get_personal_names() const
{
	if (!civilization_base::get_personal_names().empty()) {
		return civilization_base::get_personal_names();
	}

	if (this->get_group() != nullptr && !this->get_group()->get_personal_names().empty()) {
		return this->get_group()->get_personal_names();
	}

	if (this->get_supergroup() != nullptr && !this->get_supergroup()->get_personal_names().empty()) {
		return this->get_supergroup()->get_personal_names();
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_personal_names();
	}
	
	return civilization_base::get_personal_names();
}

const std::vector<std::string> &civilization::get_personal_names(const gender gender) const
{
	const std::vector<std::string> &personal_names = civilization_base::get_personal_names(gender);
	if (!personal_names.empty()) {
		return personal_names;
	}

	if (this->get_group() != nullptr && !this->get_group()->get_personal_names(gender).empty()) {
		return this->get_group()->get_personal_names(gender);
	}

	if (this->get_supergroup() != nullptr && !this->get_supergroup()->get_personal_names(gender).empty()) {
		return this->get_supergroup()->get_personal_names(gender);
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_personal_names(gender);
	}
	
	return personal_names;
}

const std::vector<std::string> &civilization::get_surnames() const
{
	if (!civilization_base::get_surnames().empty()) {
		return civilization_base::get_surnames();
	}

	if (this->get_group() != nullptr && !this->get_group()->get_surnames().empty()) {
		return this->get_group()->get_surnames();
	}

	if (this->get_supergroup() != nullptr && !this->get_supergroup()->get_surnames().empty()) {
		return this->get_supergroup()->get_surnames();
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_surnames();
	}

	return civilization_base::get_surnames();
}

const std::vector<std::string> &civilization::get_unit_class_names(const unit_class *unit_class) const
{
	const std::vector<std::string> &unit_class_names = civilization_base::get_unit_class_names(unit_class);
	if (!unit_class_names.empty()) {
		return unit_class_names;
	}
	
	if (this->get_group() != nullptr && !this->get_group()->get_unit_class_names(unit_class).empty()) {
		return this->get_group()->get_unit_class_names(unit_class);
	}

	if (this->get_supergroup() != nullptr && !this->get_supergroup()->get_unit_class_names(unit_class).empty()) {
		return this->get_supergroup()->get_unit_class_names(unit_class);
	}

	if (this->parent_civilization != nullptr) {
		return this->parent_civilization->get_unit_class_names(unit_class);
	}
	
	return unit_class_names;
}

const std::vector<std::string> &civilization::get_ship_names() const
{
	if (!civilization_base::get_ship_names().empty()) {
		return civilization_base::get_ship_names();
	}
	
	if (this->get_group() != nullptr && !this->get_group()->get_ship_names().empty()) {
		return this->get_group()->get_ship_names();
	}

	if (this->get_supergroup() != nullptr && !this->get_supergroup()->get_ship_names().empty()) {
		return this->get_supergroup()->get_ship_names();
	}

	if (this->parent_civilization) {
		return this->parent_civilization->get_ship_names();
	}
	
	return civilization_base::get_ship_names();
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

QVariantList civilization::get_acquired_upgrades_qstring_list() const
{
	return container::to_qvariant_list(this->get_acquired_upgrades());
}

void civilization::remove_acquired_upgrade(CUpgrade *upgrade)
{
	vector::remove(this->acquired_upgrades, upgrade);
}

}
