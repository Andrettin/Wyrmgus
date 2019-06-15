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
//      (c) Copyright 2018-2019 by Andrettin
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

#include "civilization.h"

#include "ai/ai_building_template.h"
#include "ai/force_template.h"
#include "language/language.h"
#include "player.h"
#include "player_color.h"
#include "species/species.h"
#include "time/calendar.h"
#include "ui/button_action.h"
#include "unit/unit_class.h"
#include "upgrade/upgrade.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

const CUnitType *CCivilization::GetCivilizationClassUnitType(const CCivilization *civilization, const UnitClass *unit_class)
{
	if (civilization == nullptr || unit_class == nullptr) {
		return nullptr;
	}
	
	if (civilization->ClassUnitTypes.find(unit_class) != civilization->ClassUnitTypes.end()) {
		return civilization->ClassUnitTypes.find(unit_class)->second;
	}
	
	if (civilization->ParentCivilization) {
		return CCivilization::GetCivilizationClassUnitType(civilization->ParentCivilization, unit_class);
	}
	
	return nullptr;
}

const CUpgrade *CCivilization::GetCivilizationClassUpgrade(const CCivilization *civilization, const UpgradeClass *upgrade_class)
{
	if (civilization == nullptr || upgrade_class == nullptr) {
		return nullptr;
	}
	
	if (civilization->ClassUpgrades.find(upgrade_class) != civilization->ClassUpgrades.end()) {
		return civilization->ClassUpgrades.find(upgrade_class)->second;
	}
	
	if (civilization->ParentCivilization) {
		return CCivilization::GetCivilizationClassUpgrade(civilization->ParentCivilization, upgrade_class);
	}
	
	return nullptr;
}

std::vector<CFiller> CCivilization::GetCivilizationUIFillers(const CCivilization *civilization)
{
	if (civilization == nullptr) {
		return std::vector<CFiller>();
	}
	
	if (civilization->UIFillers.size() > 0) {
		return civilization->UIFillers;
	}
	
	if (civilization->ParentCivilization) {
		return CCivilization::GetCivilizationUIFillers(civilization->ParentCivilization);
	}
	
	return std::vector<CFiller>();
}

/**
**	@brief	Destructor
*/
CCivilization::~CCivilization()
{
	for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = this->ForceTemplates.begin(); iterator != this->ForceTemplates.end(); ++iterator) {
		for (CForceTemplate *force_template : iterator->second) {
			delete force_template;
		}
	}
	
	for (CAiBuildingTemplate *ai_building_template : this->AiBuildingTemplates) {
		delete ai_building_template;
	}
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CCivilization::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "historical_upgrade") {
		CDate date;
		const CUpgrade *upgrade = nullptr;
		bool has_upgrade = true;
			
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
				continue;
			}
			
			String key = property.Key;
			String value = property.Value;
			
			if (key == "date") {
				value = value.replace("_", "-");
				date = CDate::FromString(value.utf8().get_data());
			} else if (key == "upgrade") {
				upgrade = CUpgrade::Get(value.utf8().get_data());
			} else if (key == "has_upgrade") {
				has_upgrade = StringToBool(value);
			} else {
				fprintf(stderr, "Invalid historical upgrade property: \"%s\".\n", key.utf8().get_data());
			}
		}
		
		if (upgrade == nullptr) {
			fprintf(stderr, "Historical upgrade has no upgrade.\n");
			return true;
		}
		
		this->HistoricalUpgrades[upgrade->Ident][date] = has_upgrade;
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the civilization
*/
void CCivilization::Initialize()
{
	if (this->ParentCivilization != nullptr) {
		if (!this->ParentCivilization->IsInitialized()) {
			fprintf(stderr, "Civilization \"%s\" is inheriting features from a non-initialized parent (\"%s\").\n", this->Ident.c_str(), this->ParentCivilization->Ident.c_str());
		}
		
		//inherit button icons from the parent civilization, for button actions which none are specified
		for (std::map<int, IconConfig>::const_iterator iterator = this->ParentCivilization->ButtonIcons.begin(); iterator != this->ParentCivilization->ButtonIcons.end(); ++iterator) {
			if (this->ButtonIcons.find(iterator->first) == this->ButtonIcons.end()) {
				this->ButtonIcons[iterator->first] = iterator->second;
			}
		}
		
		//inherit historical upgrades from the parent civilization, if no historical data is given for that upgrade for this civilization
		for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = this->ParentCivilization->HistoricalUpgrades.begin(); iterator != this->ParentCivilization->HistoricalUpgrades.end(); ++iterator) {
			if (this->HistoricalUpgrades.find(iterator->first) == this->HistoricalUpgrades.end()) {
				this->HistoricalUpgrades[iterator->first] = iterator->second;
			}
		}
		
		//unit sounds
		if (this->UnitSounds.Selected.Name.empty()) {
			this->UnitSounds.Selected = this->ParentCivilization->UnitSounds.Selected;
		}
		if (this->UnitSounds.Acknowledgement.Name.empty()) {
			this->UnitSounds.Acknowledgement = this->ParentCivilization->UnitSounds.Acknowledgement;
		}
		if (this->UnitSounds.Attack.Name.empty()) {
			this->UnitSounds.Attack = this->ParentCivilization->UnitSounds.Attack;
		}
		if (this->UnitSounds.Idle.Name.empty()) {
			this->UnitSounds.Idle = this->ParentCivilization->UnitSounds.Idle;
		}
		if (this->UnitSounds.Hit.Name.empty()) {
			this->UnitSounds.Hit = this->ParentCivilization->UnitSounds.Hit;
		}
		if (this->UnitSounds.Miss.Name.empty()) {
			this->UnitSounds.Miss = this->ParentCivilization->UnitSounds.Miss;
		}
		if (this->UnitSounds.FireMissile.Name.empty()) {
			this->UnitSounds.FireMissile = this->ParentCivilization->UnitSounds.FireMissile;
		}
		if (this->UnitSounds.Step.Name.empty()) {
			this->UnitSounds.Step = this->ParentCivilization->UnitSounds.Step;
		}
		for (const auto &element : this->ParentCivilization->UnitSounds.TerrainTypeStep) {
			if (this->UnitSounds.TerrainTypeStep.find(element.first) == this->UnitSounds.TerrainTypeStep.end()) {
				this->UnitSounds.TerrainTypeStep[element.first] = element.second;
			}
		}
		if (this->UnitSounds.Used.Name.empty()) {
			this->UnitSounds.Used = this->ParentCivilization->UnitSounds.Used;
		}
		if (this->UnitSounds.Build.Name.empty()) {
			this->UnitSounds.Build = this->ParentCivilization->UnitSounds.Build;
		}
		if (this->UnitSounds.Ready.Name.empty()) {
			this->UnitSounds.Ready = this->ParentCivilization->UnitSounds.Ready;
		}
		if (this->UnitSounds.Repair.Name.empty()) {
			this->UnitSounds.Repair = this->ParentCivilization->UnitSounds.Repair;
		}
		for (unsigned int j = 0; j < MaxCosts; ++j) {
			if (this->UnitSounds.Harvest[j].Name.empty()) {
				this->UnitSounds.Harvest[j] = this->ParentCivilization->UnitSounds.Harvest[j];
			}
		}
		if (this->UnitSounds.Help.Name.empty()) {
			this->UnitSounds.Help = this->ParentCivilization->UnitSounds.Help;
		}
		if (this->UnitSounds.HelpTown.Name.empty()) {
			this->UnitSounds.HelpTown = this->ParentCivilization->UnitSounds.HelpTown;
		}
	}
	
	if (this->ButtonIcons.find(ButtonMove) != this->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 1,\n";
		button_definition += "\tAction = \"move\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"m\",\n";
		button_definition += "\tHint = _(\"~!Move\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(this->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (this->ButtonIcons.find(ButtonStop) != this->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 2,\n";
		button_definition += "\tAction = \"stop\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"s\",\n";
		button_definition += "\tHint = _(\"~!Stop\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(this->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (this->ButtonIcons.find(ButtonAttack) != this->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 3,\n";
		button_definition += "\tAction = \"attack\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"a\",\n";
		button_definition += "\tHint = _(\"~!Attack\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(this->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (this->ButtonIcons.find(ButtonPatrol) != this->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 4,\n";
		button_definition += "\tAction = \"patrol\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"p\",\n";
		button_definition += "\tHint = _(\"~!Patrol\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(this->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (this->ButtonIcons.find(ButtonStandGround) != this->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 5,\n";
		button_definition += "\tAction = \"stand-ground\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"t\",\n";
		button_definition += "\tHint = _(\"S~!tand Ground\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(this->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (this->GetIcon() == nullptr) {
		fprintf(stderr, "Civilization \"%s\" has no icon.\n", this->Ident.c_str());
	}
	
	if (this->GetDefaultPrimaryPlayerColor() == nullptr) {
		fprintf(stderr, "Civilization \"%s\" has no primary player color.\n", this->Ident.c_str());
	}
	
	if (this->GetDefaultSecondaryPlayerColor() == nullptr) {
		fprintf(stderr, "Civilization \"%s\" has no secondary player color.\n", this->Ident.c_str());
	}
	
	this->Initialized = true;
}

/**
**	@brief	Get the civilization's upgrade
**
**	@return	The civilization's upgrade
*/
const CUpgrade *CCivilization::GetUpgrade() const
{
	if (!this->Upgrade.empty()) {
		return CUpgrade::Get(this->Upgrade);
	}
	
	if (this->ParentCivilization != nullptr) {
		return this->ParentCivilization->GetUpgrade();
	}
	
	return nullptr;
}
	
int CCivilization::GetUpgradePriority(const CUpgrade *upgrade) const
{
	if (!upgrade) {
		fprintf(stderr, "Error in CCivilization::GetUpgradePriority: the upgrade is null.\n");
	}
	
	std::map<const CUpgrade *, int>::const_iterator find_iterator = this->UpgradePriorities.find(upgrade);
	if (find_iterator != this->UpgradePriorities.end()) {
		return find_iterator->second;
	}
	
	return 100;
}

int CCivilization::GetForceTypeWeight(const int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in CCivilization::GetForceTypeWeight: the force_type is -1.\n");
	}
	
	std::map<int, int>::const_iterator find_iterator = this->ForceTypeWeights.find(force_type);
	if (find_iterator != this->ForceTypeWeights.end()) {
		return find_iterator->second;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetForceTypeWeight(force_type);
	}
	
	return 1;
}

/**
**	@brief	Get the calendar for the civilization
**
**	@return	The civilization's calendar
*/
CCalendar *CCivilization::GetCalendar() const
{
	if (this->Calendar) {
		return this->Calendar;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetCalendar();
	}
	
	return CCalendar::BaseCalendar;
}

std::vector<CForceTemplate *> CCivilization::GetForceTemplates(const int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in CCivilization::GetForceTemplates: the force_type is -1.\n");
	}
	
	std::map<int, std::vector<CForceTemplate *>>::const_iterator find_iterator = this->ForceTemplates.find(force_type);
	if (find_iterator != this->ForceTemplates.end()) {
		return find_iterator->second;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetForceTemplates(force_type);
	}
	
	return std::vector<CForceTemplate *>();
}

void CCivilization::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_parent_civilization", "civilization_ident"), +[](CCivilization *civilization, const String &civilization_ident){
		civilization->ParentCivilization = CCivilization::Get(civilization_ident);
	});
	ClassDB::bind_method(D_METHOD("get_parent_civilization"), +[](const CCivilization *civilization){ return civilization->ParentCivilization; });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parent_civilization"), "set_parent_civilization", "get_parent_civilization");
	
	ClassDB::bind_method(D_METHOD("set_species", "species_ident"), +[](CCivilization *civilization, const String &species_ident){
		civilization->Species = CSpecies::Get(species_ident);
	});
	ClassDB::bind_method(D_METHOD("get_species"), +[](const CCivilization *civilization){ return civilization->GetSpecies(); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "species"), "set_species", "get_species");
	
	ClassDB::bind_method(D_METHOD("set_language", "language_ident"), +[](CCivilization *civilization, const String &language_ident){
		civilization->Language = CLanguage::Get(language_ident);
	});
	ClassDB::bind_method(D_METHOD("get_language"), +[](const CCivilization *civilization){ return civilization->GetLanguage(); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "language"), "set_language", "get_language");
	
	ClassDB::bind_method(D_METHOD("set_upgrade", "upgrade_ident"), +[](CCivilization *civilization, const String &upgrade_ident){
		civilization->Upgrade = upgrade_ident.utf8().get_data();
	});
	ClassDB::bind_method(D_METHOD("get_upgrade"), +[](const CCivilization *civilization){ return const_cast<CUpgrade *>(civilization->GetUpgrade()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "upgrade"), "set_upgrade", "get_upgrade");
	
	ClassDB::bind_method(D_METHOD("get_interface"), &CCivilization::GetInterface);
	
	ClassDB::bind_method(D_METHOD("set_default_primary_player_color", "ident"), +[](CCivilization *civilization, const String &ident){
		civilization->DefaultPrimaryPlayerColor = CPlayerColor::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_default_primary_player_color"), +[](const CCivilization *civilization){ return const_cast<CPlayerColor *>(civilization->GetDefaultPrimaryPlayerColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_primary_player_color"), "set_default_primary_player_color", "get_default_primary_player_color");
	
	ClassDB::bind_method(D_METHOD("set_default_secondary_player_color", "ident"), +[](CCivilization *civilization, const String &ident){
		civilization->DefaultSecondaryPlayerColor = CPlayerColor::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_default_secondary_player_color"), +[](const CCivilization *civilization){ return const_cast<CPlayerColor *>(civilization->GetDefaultSecondaryPlayerColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_secondary_player_color"), "set_default_secondary_player_color", "get_default_secondary_player_color");
	
	ClassDB::bind_method(D_METHOD("get_victory_background_file"), &CCivilization::GetVictoryBackgroundFile);
	ClassDB::bind_method(D_METHOD("get_defeat_background_file"), &CCivilization::GetDefeatBackgroundFile);
	
	ClassDB::bind_method(D_METHOD("set_playable", "playable"), +[](CCivilization *civilization, const bool playable){ civilization->Playable = playable; });
	ClassDB::bind_method(D_METHOD("is_playable"), &CCivilization::IsPlayable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playable"), "set_playable", "is_playable");
	
	ClassDB::bind_method(D_METHOD("add_to_develops_from", "civilization_ident"), +[](CCivilization *civilization, const String &civilization_ident){
		CCivilization *develops_from = CCivilization::Get(civilization_ident);
		civilization->DevelopsFrom.push_back(develops_from);
		develops_from->DevelopsTo.push_back(civilization);
	});
	ClassDB::bind_method(D_METHOD("remove_from_develops_from", "civilization_ident"), +[](CCivilization *civilization, const String &civilization_ident){
		CCivilization *develops_from = CCivilization::Get(civilization_ident);
		civilization->DevelopsFrom.erase(std::remove(civilization->DevelopsFrom.begin(), civilization->DevelopsFrom.end(), develops_from), civilization->DevelopsFrom.end());
		develops_from->DevelopsTo.erase(std::remove(develops_from->DevelopsTo.begin(), develops_from->DevelopsTo.end(), civilization), develops_from->DevelopsTo.end());
	});
	ClassDB::bind_method(D_METHOD("get_develops_from"), +[](const CCivilization *civilization){ return ContainerToGodotArray(civilization->DevelopsFrom); });
	
	ClassDB::bind_method(D_METHOD("add_to_develops_to", "civilization_ident"), +[](CCivilization *civilization, const String &civilization_ident){
		CCivilization *develops_to = CCivilization::Get(civilization_ident);
		civilization->DevelopsTo.push_back(develops_to);
		develops_to->DevelopsFrom.push_back(civilization);
	});
	ClassDB::bind_method(D_METHOD("remove_from_develops_to", "civilization_ident"), +[](CCivilization *civilization, const String &civilization_ident){
		CCivilization *develops_to = CCivilization::Get(civilization_ident);
		civilization->DevelopsTo.erase(std::remove(civilization->DevelopsTo.begin(), civilization->DevelopsTo.end(), develops_to), civilization->DevelopsTo.end());
		develops_to->DevelopsFrom.erase(std::remove(develops_to->DevelopsFrom.begin(), develops_to->DevelopsFrom.end(), civilization), develops_to->DevelopsFrom.end());
	});
	ClassDB::bind_method(D_METHOD("get_develops_to"), +[](const CCivilization *civilization){ return ContainerToGodotArray(civilization->DevelopsTo); });
}
