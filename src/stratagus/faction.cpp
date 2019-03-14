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

#include "ai.h"
#include "civilization.h"
#include "faction.h"
#include "luacallback.h"
#include "player.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CFaction *> CFaction::Factions;

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

	this->UIFillers.clear();
}

int CFaction::GetFactionIndexByName(const std::string &faction_ident)
{
	if (faction_ident.empty()) {
		return -1;
	}
	
	if (FactionStringToIndex.find(faction_ident) != FactionStringToIndex.end()) {
		return FactionStringToIndex[faction_ident];
	} else {
		return -1;
	}
}

CFaction *CFaction::GetFaction(const std::string &faction_ident)
{
	if (faction_ident.empty()) {
		return nullptr;
	}
	
	if (FactionStringToIndex.find(faction_ident) != FactionStringToIndex.end()) {
		return CFaction::Factions[FactionStringToIndex[faction_ident]];
	} else {
		return nullptr;
	}
}

int CFaction::GetFactionClassUnitType(int faction, int class_id)
{
	if (faction == -1 || class_id == -1) {
		return -1;
	}
	
	if (CFaction::Factions[faction]->ClassUnitTypes.find(class_id) != CFaction::Factions[faction]->ClassUnitTypes.end()) {
		return CFaction::Factions[faction]->ClassUnitTypes[class_id];
	}
	
	if (CFaction::Factions[faction]->ParentFaction != -1) {
		return CFaction::GetFactionClassUnitType(CFaction::Factions[faction]->ParentFaction, class_id);
	}
	
	return PlayerRaces.GetCivilizationClassUnitType(CFaction::Factions[faction]->Civilization->ID, class_id);
}

int CFaction::GetFactionClassUpgrade(int faction, int class_id)
{
	if (faction == -1 || class_id == -1) {
		return -1;
	}
	
	if (CFaction::Factions[faction]->ClassUpgrades.find(class_id) != CFaction::Factions[faction]->ClassUpgrades.end()) {
		return CFaction::Factions[faction]->ClassUpgrades[class_id];
	}
		
	if (CFaction::Factions[faction]->ParentFaction != -1) {
		return CFaction::GetFactionClassUpgrade(CFaction::Factions[faction]->ParentFaction, class_id);
	}
	
	return PlayerRaces.GetCivilizationClassUpgrade(CFaction::Factions[faction]->Civilization->ID, class_id);
}

std::vector<CFiller> CFaction::GetFactionUIFillers(int faction)
{
	if (faction == -1) {
		return std::vector<CFiller>();
	}
	
	if (CFaction::Factions[faction]->UIFillers.size() > 0) {
		return CFaction::Factions[faction]->UIFillers;
	}
		
	if (CFaction::Factions[faction]->ParentFaction != -1) {
		return CFaction::GetFactionUIFillers(CFaction::Factions[faction]->ParentFaction);
	}
	
	return PlayerRaces.GetCivilizationUIFillers(CFaction::Factions[faction]->Civilization->ID);
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
	
	if (this->ParentFaction != -1) {
		return CFaction::Factions[this->ParentFaction]->GetForceTypeWeight(force_type);
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
CCurrency *CFaction::GetCurrency() const
{
	if (this->Currency) {
		return this->Currency;
	}
	
	if (this->ParentFaction != -1) {
		return CFaction::Factions[this->ParentFaction]->GetCurrency();
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
	
	if (this->ParentFaction != -1) {
		return CFaction::Factions[this->ParentFaction]->GetForceTemplates(force_type);
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
	
	if (this->ParentFaction != -1) {
		return CFaction::Factions[this->ParentFaction]->GetAiBuildingTemplates();
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
	
	if (this->ParentFaction != -1) {
		return CFaction::Factions[this->ParentFaction]->GetShipNames();
	}
	
	return this->Civilization->GetShipNames();
}

void CFaction::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_primary_color"), &CFaction::GetPrimaryColor);
}
