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
#include "player.h"
#include "player_color.h"
#include "time/calendar.h"
#include "unit/unit_class.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

int CCivilization::GetCivilizationClassUnitType(const CCivilization *civilization, const UnitClass *unit_class)
{
	if (civilization == nullptr || unit_class == nullptr) {
		return -1;
	}
	
	if (civilization->ClassUnitTypes.find(unit_class) != civilization->ClassUnitTypes.end()) {
		return civilization->ClassUnitTypes.find(unit_class)->second;
	}
	
	if (civilization->ParentCivilization) {
		return CCivilization::GetCivilizationClassUnitType(civilization->ParentCivilization, unit_class);
	}
	
	return -1;
}

int CCivilization::GetCivilizationClassUpgrade(const CCivilization *civilization, const int class_id)
{
	if (civilization == nullptr || class_id == -1) {
		return -1;
	}
	
	if (civilization->ClassUpgrades.find(class_id) != civilization->ClassUpgrades.end()) {
		return civilization->ClassUpgrades.find(class_id)->second;
	}
	
	if (civilization->ParentCivilization) {
		return CCivilization::GetCivilizationClassUpgrade(civilization->ParentCivilization, class_id);
	}
	
	return -1;
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
**	@brief	Get the civilization's upgrade
**
**	@return	The civilization's upgrade
*/
const CUpgrade *CCivilization::GetUpgrade() const
{
	if (!this->Upgrade.empty()) {
		return CUpgrade::Get(this->Upgrade);
	} else {
		return nullptr;
	}
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
	ClassDB::bind_method(D_METHOD("get_ident"), &CCivilization::GetIdent);
	ClassDB::bind_method(D_METHOD("get_name"), &CCivilization::GetName);
	ClassDB::bind_method(D_METHOD("get_interface"), &CCivilization::GetInterface);
	ClassDB::bind_method(D_METHOD("is_hidden"), &CCivilization::IsHidden);
	ClassDB::bind_method(D_METHOD("get_default_player_color"), &CCivilization::GetDefaultPlayerColor);
}
