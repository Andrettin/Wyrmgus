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
/**@name civilization.cpp - The civilization source file. */
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

#include "player.h"
#include "time/calendar.h"
#include "ui/button_action.h"
#include "unit/unittype.h"
#include "video.h"

namespace stratagus {

civilization::~civilization()
{
	for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = this->ForceTemplates.begin(); iterator != this->ForceTemplates.end(); ++iterator) {
		for (size_t i = 0; i < iterator->second.size(); ++i) {
			delete iterator->second[i];
		}
	}
	
	for (size_t i = 0; i < this->AiBuildingTemplates.size(); ++i) {
		delete this->AiBuildingTemplates[i];
	}
}

void civilization::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

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
	} else if (tag == "ui_fillers") {
		PlayerRaces.civilization_ui_fillers[this->ID].clear();

		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			CFiller filler = CFiller();
			const std::string filler_file = child_scope.get_property_value("file");
			if (filler_file.empty()) {
				throw std::runtime_error("Filler graphic file is empty.");
			}
			filler.G = CGraphic::New(filler_file);

			const QPoint pos = child_scope.get_child("pos").to_point();
			filler.X = pos.x();
			filler.Y = pos.y();
			PlayerRaces.civilization_ui_fillers[this->ID].push_back(std::move(filler));
		});
	} else if (tag == "force_type_weights") {
		this->ForceTypeWeights.clear();

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const int force_type = GetForceTypeIdByName(key);
			this->ForceTypeWeights[force_type] = std::stoi(value);
		});
	} else if (tag == "force_templates") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			CForceTemplate *force = new CForceTemplate;

			child_scope.for_each_element([&](const sml_property &property) {
				if (property.get_key() == "force_type") {
					force->ForceType = GetForceTypeIdByName(property.get_value());
					this->ForceTemplates[force->ForceType].push_back(force);
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

						const int unit_class = GetOrAddUnitTypeClassIndexByName(key);
						const int unit_quantity = std::stoi(value);
						force->Units.push_back(std::pair<int, int>(unit_class, unit_quantity));
					});
				} else {
					throw std::runtime_error("Invalid force template property: " + grandchild_scope.get_tag() + ".");
				}
			});
		});

		for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = this->ForceTemplates.begin(); iterator != this->ForceTemplates.end(); ++iterator) {
			std::sort(iterator->second.begin(), iterator->second.end(), [](CForceTemplate *a, CForceTemplate *b) {
				return a->Priority > b->Priority;
			});
		}
	}
}

void civilization::initialize()
{
	if (this->parent_civilization != nullptr) {
		if (!this->parent_civilization->is_initialized()) {
			this->parent_civilization->initialize();
		}

		const stratagus::civilization *parent_civilization = this->parent_civilization;
		const int parent_civilization_id = parent_civilization->ID;

		if (this->get_interface().empty()) {
			this->interface = parent_civilization->interface;
		}

		if (PlayerRaces.civilization_upgrades[this->ID].empty() && !PlayerRaces.civilization_upgrades[parent_civilization_id].empty()) { //if the civilization has no civilization upgrade, inherit that of its parent civilization
			PlayerRaces.civilization_upgrades[this->ID] = PlayerRaces.civilization_upgrades[parent_civilization_id];
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
		button_definition += "\tPopup = \"popup-commands\",\n";
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
		button_definition += "\tPopup = \"popup-commands\",\n";
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
		button_definition += "\tPopup = \"popup-commands\",\n";
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
		button_definition += "\tPopup = \"popup-commands\",\n";
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
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"t\",\n";
		button_definition += "\tHint = _(\"S~!tand Ground\"),\n";
		button_definition += "\tForUnit = {\"" + this->get_identifier() + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}

	data_entry::initialize();
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

int civilization::GetForceTypeWeight(int force_type) const
{
	if (force_type == -1) {
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

CCalendar *civilization::GetCalendar() const
{
	if (this->Calendar) {
		return this->Calendar;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetCalendar();
	}
	
	return CCalendar::BaseCalendar;
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

std::vector<CForceTemplate *> civilization::GetForceTemplates(int force_type) const
{
	if (force_type == -1) {
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

std::map<int, std::vector<std::string>> &civilization::GetPersonalNames()
{
	if (this->PersonalNames.size() > 0) {
		return this->PersonalNames;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetPersonalNames();
	}
	
	return this->PersonalNames;
}

std::vector<std::string> &civilization::GetUnitClassNames(int class_id)
{
	if (this->UnitClassNames[class_id].size() > 0) {
		return this->UnitClassNames[class_id];
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetUnitClassNames(class_id);
	}
	
	return this->UnitClassNames[class_id];
}

std::vector<std::string> &civilization::GetShipNames()
{
	if (this->ShipNames.size() > 0) {
		return this->ShipNames;
	}
	
	if (this->parent_civilization) {
		return this->parent_civilization->GetShipNames();
	}
	
	return this->ShipNames;
}

}