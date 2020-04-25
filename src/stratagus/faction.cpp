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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "faction.h"

#include "civilization.h"
#include "luacallback.h"

namespace stratagus {

faction::~faction()
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

void faction::check() const
{
	if (this->civilization == nullptr) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no civilization.");
	}
}

int faction::GetUpgradePriority(const CUpgrade *upgrade) const
{
	if (!upgrade) {
		fprintf(stderr, "Error in faction::GetUpgradePriority: the upgrade is null.\n");
	}
	
	if (this->UpgradePriorities.find(upgrade) != this->UpgradePriorities.end()) {
		return this->UpgradePriorities.find(upgrade)->second;
	}
	
	return this->civilization->GetUpgradePriority(upgrade);
}

int faction::GetForceTypeWeight(int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in faction::GetForceTypeWeight: the force_type is -1.\n");
	}
	
	if (this->ForceTypeWeights.find(force_type) != this->ForceTypeWeights.end()) {
		return this->ForceTypeWeights.find(force_type)->second;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetForceTypeWeight(force_type);
	}
	
	return this->civilization->GetForceTypeWeight(force_type);
}

/**
**	@brief	Get the faction's currency
**
**	@return	The faction's currency
*/
CCurrency *faction::GetCurrency() const
{
	if (this->Currency != nullptr) {
		return this->Currency;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetCurrency();
	}
	
	return this->civilization->GetCurrency();
}

std::vector<CForceTemplate *> faction::GetForceTemplates(int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in faction::GetForceTemplates: the force_type is -1.\n");
	}
	
	if (this->ForceTemplates.find(force_type) != this->ForceTemplates.end()) {
		return this->ForceTemplates.find(force_type)->second;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetForceTemplates(force_type);
	}
	
	return this->civilization->GetForceTemplates(force_type);
}

std::vector<CAiBuildingTemplate *> faction::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetAiBuildingTemplates();
	}
	
	return this->civilization->GetAiBuildingTemplates();
}

const std::vector<std::string> &faction::get_ship_names() const
{
	if (this->ship_names.size() > 0) {
		return this->ship_names;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->get_ship_names();
	}
	
	return this->civilization->get_ship_names();
}

CUnitType *faction::get_class_unit_type(const unit_class *unit_class) const
{
	if (unit_class == nullptr) {
		return nullptr;
	}

	auto find_iterator = this->class_unit_types.find(unit_class);
	if (find_iterator != this->class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->get_class_unit_type(unit_class);
	}

	return this->civilization->get_class_unit_type(unit_class);
}

}
