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
#include "faction_type.h"
#include "grand_strategy.h"
#include "luacallback.h"
#include "map/site.h"
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
	if (key == "default_tier") {
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
				value = FindAndReplaceString(value, "_", "-");
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
	} else if (section->Tag == "historical_capitals") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			key = FindAndReplaceString(key, "_", "-");
			CDate date = CDate::FromString(key);
			
			std::string value = property.Value;
			const CSite *site = CSite::Get(value);
			
			this->HistoricalCapitals.push_back(std::pair<CDate, std::string>(date, site->Ident));
		}
	} else if (section->Tag == "historical_tiers") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			key = FindAndReplaceString(key, "_", "-");
			CDate date = CDate::FromString(key);
			
			std::string value = property.Value;
			value = FindAndReplaceString(value, "_", "-");
			const int faction_tier = GetFactionTierIdByName(value);
			if (faction_tier == -1) {
				fprintf(stderr, "Invalid faction tier: \"%s\".\n", value.c_str());
				continue;
			}
			
			this->HistoricalTiers[date.Year] = faction_tier;
		}
	} else if (section->Tag == "historical_government_types") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			key = FindAndReplaceString(key, "_", "-");
			CDate date = CDate::FromString(key);
			
			std::string value = property.Value;
			value = FindAndReplaceString(value, "_", "-");
			const int government_type = GetGovernmentTypeIdByName(value);
			if (government_type == -1) {
				fprintf(stderr, "Invalid government type: \"%s\".\n", value.c_str());
				continue;
			}
			
			this->HistoricalGovernmentTypes[date.Year] = government_type;
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the faction
*/
void CFaction::Initialize()
{
	if (this->GetType()->IsTribal()) {
		this->DefiniteArticle = true;
	}
	
	if (this->GetParentFaction() != nullptr) {
		if (this->FactionUpgrade.empty()) { //if the faction has no faction upgrade, inherit that of its parent faction
			this->FactionUpgrade = this->GetParentFaction()->FactionUpgrade;
		}
		
		//inherit button icons from parent faction, for button actions which none are specified
		for (std::map<int, IconConfig>::const_iterator iterator = this->GetParentFaction()->ButtonIcons.begin(); iterator != this->GetParentFaction()->ButtonIcons.end(); ++iterator) {
			if (this->ButtonIcons.find(iterator->first) == this->ButtonIcons.end()) {
				this->ButtonIcons[iterator->first] = iterator->second;
			}
		}
		
		for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = this->GetParentFaction()->HistoricalUpgrades.begin(); iterator != this->GetParentFaction()->HistoricalUpgrades.end(); ++iterator) {
			if (this->HistoricalUpgrades.find(iterator->first) == this->HistoricalUpgrades.end()) {
				this->HistoricalUpgrades[iterator->first] = iterator->second;
			}
		}
	}
	
	if (!this->GetCivilization()->IsHidden() && !this->IsHidden()) {
		if (this->GetIcon() == nullptr) {
			fprintf(stderr, "Faction \"%s\" has no icon.\n", this->Ident.c_str());
		}
		
		if (this->GetPrimaryColor() == nullptr) {
			fprintf(stderr, "Faction \"%s\" has no primary color.\n", this->Ident.c_str());
		}
		
		if (this->GetSecondaryColor() == nullptr) {
			fprintf(stderr, "Faction \"%s\" has no secondary color.\n", this->Ident.c_str());
		}
	}
	
	this->Initialized = true;
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

const CUpgrade *CFaction::GetFactionClassUpgrade(const CFaction *faction, const UpgradeClass *upgrade_class)
{
	if (faction == nullptr || upgrade_class == nullptr) {
		return nullptr;
	}
	
	if (faction->ClassUpgrades.find(upgrade_class) != faction->ClassUpgrades.end()) {
		return faction->ClassUpgrades.find(upgrade_class)->second;
	}
		
	if (faction->ParentFaction != nullptr) {
		return CFaction::GetFactionClassUpgrade(faction->ParentFaction, upgrade_class);
	}
	
	return CCivilization::GetCivilizationClassUpgrade(faction->Civilization, upgrade_class);
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
**	@brief	Get the faction's primary color
**
**	@return	The faction's primary color
*/
const CPlayerColor *CFaction::GetPrimaryColor() const
{
	if (this->PrimaryColor != nullptr) {
		return this->PrimaryColor;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetPrimaryColor();
	}
	
	return this->Civilization->GetDefaultPrimaryPlayerColor();
}

/**
**	@brief	Get the faction's secondary color
**
**	@return	The faction's secondary color
*/
const CPlayerColor *CFaction::GetSecondaryColor() const
{
	if (this->SecondaryColor != nullptr) {
		return this->SecondaryColor;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetSecondaryColor();
	}
	
	return this->Civilization->GetDefaultSecondaryPlayerColor();
}

/**
**	@brief	Get the faction's icon
**
**	@return	The faction's icon
*/
CIcon *CFaction::GetIcon() const
{
	if (this->Icon != nullptr) {
		return this->Icon;
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetIcon();
	}
	
	return this->Civilization->GetIcon();
}

/**
**	@brief	Get the faction's upgrade
**
**	@return	The faction's upgrade
*/
const CUpgrade *CFaction::GetUpgrade() const
{
	if (!this->FactionUpgrade.empty()) {
		return CUpgrade::Get(this->FactionUpgrade);
	}
	
	if (this->ParentFaction != nullptr) {
		return this->ParentFaction->GetUpgrade();
	}
	
	return nullptr;
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
	ClassDB::bind_method(D_METHOD("set_parent_faction", "ident"), +[](CFaction *faction, const String &ident){ faction->ParentFaction = CFaction::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_parent_faction"), +[](const CFaction *faction){ return const_cast<CFaction *>(faction->GetParentFaction()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parent_faction"), "set_parent_faction", "get_parent_faction");

	ClassDB::bind_method(D_METHOD("set_type", "ident"), +[](CFaction *faction, const String &ident){ faction->Type = FactionType::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_type"), +[](const CFaction *faction){ return const_cast<FactionType *>(faction->GetType()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "type"), "set_type", "get_type");

	ClassDB::bind_method(D_METHOD("set_civilization", "ident"), +[](CFaction *faction, const String &ident){ faction->Civilization = CCivilization::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_civilization"), &CFaction::GetCivilization);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "civilization"), "set_civilization", "get_civilization");

	ClassDB::bind_method(D_METHOD("set_primary_color", "ident"), +[](CFaction *faction, const String &ident){
		const CPlayerColor *player_color = CPlayerColor::Get(ident);
		faction->PrimaryColor = player_color;
	});
	ClassDB::bind_method(D_METHOD("get_primary_color"), +[](const CFaction *faction){ return const_cast<CPlayerColor *>(faction->GetPrimaryColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "primary_color"), "set_primary_color", "get_primary_color");
	
	ClassDB::bind_method(D_METHOD("set_secondary_color", "ident"), +[](CFaction *faction, const String &ident){
		CPlayerColor *player_color = CPlayerColor::Get(ident);
		if (player_color != nullptr) {
			faction->SecondaryColor = player_color;
		}
	});
	ClassDB::bind_method(D_METHOD("get_secondary_color"), +[](const CFaction *faction){ return const_cast<CPlayerColor *>(faction->GetSecondaryColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "secondary_color"), "set_secondary_color", "get_secondary_color");
	
	ClassDB::bind_method(D_METHOD("set_faction_upgrade", "ident"), +[](CFaction *faction, const String &ident){
		std::string faction_upgrade_ident = ident.utf8().get_data();
		faction_upgrade_ident = FindAndReplaceString(faction_upgrade_ident, "_", "-");
		faction->FactionUpgrade = faction_upgrade_ident;
	});
	ClassDB::bind_method(D_METHOD("get_faction_upgrade"), +[](const CFaction *faction){ return const_cast<CUpgrade *>(faction->GetUpgrade()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "faction_upgrade"), "set_faction_upgrade", "get_faction_upgrade");
	
	ClassDB::bind_method(D_METHOD("set_adjective", "ident"), +[](CFaction *faction, const String &adjective){ faction->Adjective = adjective; });
	ClassDB::bind_method(D_METHOD("get_adjective"), &CFaction::GetAdjective);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "adjective"), "set_adjective", "get_adjective");
	
	ClassDB::bind_method(D_METHOD("set_playable", "playable"), +[](CFaction *faction, const bool playable){ faction->Playable = playable; });
	ClassDB::bind_method(D_METHOD("is_playable"), &CFaction::IsPlayable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playable"), "set_playable", "is_playable");

	ClassDB::bind_method(D_METHOD("add_to_develops_from", "ident"), +[](CFaction *faction, const String &ident){
		CFaction *other_faction = CFaction::Get(ident);
		if (other_faction != nullptr) {
			faction->DevelopsFrom.push_back(other_faction);
			other_faction->DevelopsTo.push_back(faction);
		}
	});
	ClassDB::bind_method(D_METHOD("remove_from_develops_from", "ident"), +[](CFaction *faction, const String &ident){
		CFaction *other_faction = CFaction::Get(ident);
		if (other_faction != nullptr) {
			faction->DevelopsFrom.erase(std::remove(faction->DevelopsFrom.begin(), faction->DevelopsFrom.end(), other_faction), faction->DevelopsFrom.end());
			other_faction->DevelopsTo.erase(std::remove(other_faction->DevelopsTo.begin(), other_faction->DevelopsTo.end(), faction), other_faction->DevelopsTo.end());
		}
	});
	ClassDB::bind_method(D_METHOD("get_develops_from"), +[](const CFaction *faction){ return VectorToGodotArray(faction->DevelopsFrom); });

	ClassDB::bind_method(D_METHOD("add_to_develops_to", "ident"), +[](CFaction *faction, const String &ident){
		CFaction *other_faction = CFaction::Get(ident);
		if (other_faction != nullptr) {
			faction->DevelopsTo.push_back(other_faction);
			other_faction->DevelopsFrom.push_back(faction);
		}
	});
	ClassDB::bind_method(D_METHOD("remove_from_develops_to", "ident"), +[](CFaction *faction, const String &ident){
		CFaction *other_faction = CFaction::Get(ident);
		if (other_faction != nullptr) {
			faction->DevelopsTo.erase(std::remove(faction->DevelopsTo.begin(), faction->DevelopsTo.end(), other_faction), faction->DevelopsTo.end());
			other_faction->DevelopsFrom.erase(std::remove(other_faction->DevelopsFrom.begin(), other_faction->DevelopsFrom.end(), faction), other_faction->DevelopsFrom.end());
		}
	});
	ClassDB::bind_method(D_METHOD("get_develops_to"), +[](const CFaction *faction){ return VectorToGodotArray(faction->DevelopsTo); });
}
