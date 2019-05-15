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
/**@name faction.cpp - The faction source file. */
//
//      (c) Copyright 2019 by Andrettin
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

#include "faction.h"

#include "ai/ai.h"
#include "ai/ai_building_template.h"
#include "ai/force_template.h"
#include "civilization.h"
#include "economy/resource.h"
#include "grand_strategy.h"
#include "luacallback.h"
#include "player.h"
#include "player_color.h"
#include "ui/icon.h"
#include "unit/unit_class.h"
#include "upgrade/upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CFaction::~CFaction()
{
	for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = this->ForceTemplates.begin(); iterator != this->ForceTemplates.end(); ++iterator) {
		for (size_t i = 0; i < iterator->second.size(); ++i) {
			delete iterator->second[i];
		}
	}
	
	for (size_t i = 0; i < this->AiBuildingTemplates.size(); ++i) {
		delete this->AiBuildingTemplates[i];
	}

	if (this->Conditions) {
		delete Conditions;
	}
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CFaction::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "civilization") {
		this->Civilization = CCivilization::Get(value);
	} else if (key == "type") {
		value = FindAndReplaceString(value, "_", "-");
		const int faction_type = GetFactionTypeIdByName(value);
		if (faction_type != -1) {
			this->Type = faction_type;
		} else {
			fprintf(stderr, "Faction type \"%s\" doesn't exist.", value.c_str());
		}
	} else if (key == "primary_color") {
		CPlayerColor *player_color = CPlayerColor::Get(value);
		if (player_color != nullptr) {
			this->PrimaryColors.push_back(player_color);
		}
	} else if (key == "secondary_color") {
		CPlayerColor *player_color = CPlayerColor::Get(value);
		if (player_color != nullptr) {
			this->SecondaryColor = player_color;
		}
	} else if (key == "icon") {
		this->Icon = CIcon::Get(value);
	} else if (key == "faction_upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		this->FactionUpgrade = value;
	} else if (key == "develops_from") {
		CFaction *other_faction = CFaction::Get(value);
		if (other_faction != nullptr) {
			this->DevelopsFrom.push_back(other_faction);
			other_faction->DevelopsTo.push_back(this);
		}
	} else if (key == "develops_to") {
		CFaction *other_faction = CFaction::Get(value);
		if (other_faction != nullptr) {
			this->DevelopsTo.push_back(other_faction);
			other_faction->DevelopsFrom.push_back(this);
		}
	} else if (key == "default_tier") {
		value = FindAndReplaceString(value, "_", "-");
		const int faction_tier = GetFactionTierIdByName(value);
		if (faction_tier != -1) {
			this->DefaultTier = faction_tier;
		} else {
			fprintf(stderr, "Invalid faction tier: \"%s\".\n", value.c_str());
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CFaction::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "historical_resources") {
		CDate date;
		std::map<const CResource *, int> resource_quantities;
			
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			std::string value = property.Value;
			
			if (key == "date") {
				value = FindAndReplaceString(value, "_", "-");
				date = CDate::FromString(value);
			} else {
				const CResource *resource = CResource::Get(key);
				
				if (resource != nullptr) {
					resource_quantities[resource] = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid historical resources property: \"%s\".\n", key.c_str());
				}
			}
		}
		
		for (const auto &element : resource_quantities) {
			this->HistoricalResources[std::pair<CDate, int>(date, element.first->GetIndex())] = element.second;
		}
	} else if (section->Tag == "historical_diplomacy_state") {
		CDate date;
		const CFaction *other_faction = nullptr;
		int diplomacy_state = -1;
			
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			std::string value = property.Value;
			
			if (key == "date") {
				value = FindAndReplaceString(value, "_", "-");
				date = CDate::FromString(value);
			} else if (key == "faction") {
				other_faction = CFaction::Get(value);
			} else if (key == "diplomacy_state") {
				value = FindAndReplaceString(value, "_", "-");
				diplomacy_state = GetDiplomacyStateIdByName(value);
				if (diplomacy_state == -1) {
					fprintf(stderr, "Invalid diplomacy state: \"%s\".\n", value.c_str());
				}
			} else {
				fprintf(stderr, "Invalid historical resources property: \"%s\".\n", key.c_str());
			}
		}
		
		if (other_faction == nullptr) {
			fprintf(stderr, "Historical diplomacy state has no faction.\n");
			return true;
		}
		
		if (diplomacy_state == -1) {
			fprintf(stderr, "Historical diplomacy state has no diplomacy state.\n");
			return true;
		}
		
		this->HistoricalDiplomacyStates[std::pair<CDate, const CFaction *>(date, other_faction)] = diplomacy_state;
	} else if (section->Tag == "historical_upgrade") {
		CDate date;
		const CUpgrade *upgrade = nullptr;
		bool has_upgrade = true;
			
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			std::string value = property.Value;
			
			if (key == "date") {
				value = FindAndReplaceString(value, "_", "-");
				date = CDate::FromString(value);
			} else if (key == "upgrade") {
				upgrade = CUpgrade::Get(value);
			} else if (key == "has_upgrade") {
				has_upgrade = StringToBool(value);
			} else {
				fprintf(stderr, "Invalid historical upgrade property: \"%s\".\n", key.c_str());
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

int CFaction::GetFactionIndex(const std::string &faction_ident)
{
	if (faction_ident.empty()) {
		return -1;
	}
	
	const CFaction *faction = CFaction::Get(faction_ident);
	
	if (faction != nullptr) {
		return faction->GetIndex();
	} else {
		return -1;
	}
}

const CUnitType *CFaction::GetFactionClassUnitType(const CFaction *faction, const UnitClass *unit_class)
{
	if (faction == nullptr || unit_class == nullptr) {
		return nullptr;
	}
	
	if (faction->ClassUnitTypes.find(unit_class) != faction->ClassUnitTypes.end()) {
		return faction->ClassUnitTypes.find(unit_class)->second;
	}
	
	if (faction->ParentFaction != nullptr) {
		return CFaction::GetFactionClassUnitType(faction->ParentFaction, unit_class);
	}
	
	return CCivilization::GetCivilizationClassUnitType(faction->Civilization, unit_class);
}

int CFaction::GetFactionClassUpgrade(const CFaction *faction, const int class_id)
{
	if (faction == nullptr || class_id == -1) {
		return -1;
	}
	
	if (faction->ClassUpgrades.find(class_id) != faction->ClassUpgrades.end()) {
		return faction->ClassUpgrades.find(class_id)->second;
	}
		
	if (faction->ParentFaction != nullptr) {
		return CFaction::GetFactionClassUpgrade(faction->ParentFaction, class_id);
	}
	
	return CCivilization::GetCivilizationClassUpgrade(faction->Civilization, class_id);
}

std::vector<CFiller> CFaction::GetFactionUIFillers(const CFaction *faction)
{
	if (faction == nullptr) {
		return std::vector<CFiller>();
	}
	
	if (faction->UIFillers.size() > 0) {
		return faction->UIFillers;
	}
		
	if (faction->ParentFaction != nullptr) {
		return CFaction::GetFactionUIFillers(faction->ParentFaction);
	}
	
	return CCivilization::GetCivilizationUIFillers(faction->Civilization);
}

int CFaction::GetUpgradePriority(const CUpgrade *upgrade) const
{
	if (!upgrade) {
		fprintf(stderr, "Error in CFaction::GetUpgradePriority: the upgrade is null.\n");
	}
	
	if (this->UpgradePriorities.find(upgrade) != this->UpgradePriorities.end()) {
		return this->UpgradePriorities.find(upgrade)->second;
	}
	
	if (this->Civilization == nullptr) {
		fprintf(stderr, "Error in CFaction::GetUpgradePriority: the faction has no civilization.\n");
	}
	
	return this->Civilization->GetUpgradePriority(upgrade);
}

int CFaction::GetForceTypeWeight(int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in CFaction::GetForceTypeWeight: the force_type is -1.\n");
	}
	
	if (this->ForceTypeWeights.find(force_type) != this->ForceTypeWeights.end()) {
		return this->ForceTypeWeights.find(force_type)->second;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetForceTypeWeight(force_type);
	}
	
	if (this->Civilization == nullptr) {
		fprintf(stderr, "Error in CFaction::GetForceTypeWeight: the faction has no civilization.\n");
	}
	
	return this->Civilization->GetForceTypeWeight(force_type);
}

/**
**	@brief	Get the faction's currency
**
**	@return	The faction's currency
*/
Currency *CFaction::GetCurrency() const
{
	if (this->Currency) {
		return this->Currency;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetCurrency();
	}
	
	if (this->Civilization != nullptr) {
		return this->Civilization->GetCurrency();
	}
	
	return nullptr;
}

std::vector<CForceTemplate *> CFaction::GetForceTemplates(int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in CFaction::GetForceTemplates: the force_type is -1.\n");
	}
	
	if (this->ForceTemplates.find(force_type) != this->ForceTemplates.end()) {
		return this->ForceTemplates.find(force_type)->second;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetForceTemplates(force_type);
	}
	
	if (this->Civilization == nullptr) {
		fprintf(stderr, "Error in CFaction::GetForceTemplates: the faction has no civilization.\n");
	}
	
	return this->Civilization->GetForceTemplates(force_type);
}

std::vector<CAiBuildingTemplate *> CFaction::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetAiBuildingTemplates();
	}
	
	if (this->Civilization == nullptr) {
		fprintf(stderr, "Error in CFaction::GetAiBuildingTemplates: the faction has no civilization.\n");
	}
	
	return this->Civilization->GetAiBuildingTemplates();
}

const std::vector<std::string> &CFaction::GetShipNames() const
{
	if (this->ShipNames.size() > 0) {
		return this->ShipNames;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetShipNames();
	}
	
	return this->Civilization->GetShipNames();
}

void CFaction::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_primary_color"), &CFaction::GetPrimaryColor);
	ClassDB::bind_method(D_METHOD("get_secondary_color"), &CFaction::GetSecondaryColor);
}
