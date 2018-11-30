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
//      (c) Copyright 2018 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "civilization.h"

#include "calendar/calendar.h"
#include "player.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CCivilization *> CCivilization::Civilizations;
std::map<std::string, CCivilization *> CCivilization::CivilizationsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a civilization
**
**	@param	ident		The civilization's string identifier
**	@param	should_find	Whether it is an error if the civilization could not be found; this is true by default
**
**	@return	The civilization if found, or null otherwise
*/
CCivilization *CCivilization::GetCivilization(const std::string &ident, const bool should_find)
{
	if (CivilizationsByIdent.find(ident) != CivilizationsByIdent.end()) {
		return CivilizationsByIdent.find(ident)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid civilization: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a civilization
**
**	@param	ident	The civilization's string identifier
**
**	@return	The civilization if found, or a newly-created one otherwise
*/
CCivilization *CCivilization::GetOrAddCivilization(const std::string &ident)
{
	CCivilization *civilization = GetCivilization(ident, false);
	
	if (!civilization) {
		civilization = new CCivilization;
		civilization->Ident = ident;
		civilization->ID = Civilizations.size();
		Civilizations.push_back(civilization);
		CivilizationsByIdent[ident] = civilization;
		
		PlayerRaces.Name[civilization->ID] = ident;
		PlayerRaces.Playable[civilization->ID] = true; //civilizations are playable by default
	}
	
	return civilization;
}

/**
**	@brief	Remove the existing civilizations
*/
void CCivilization::ClearCivilizations()
{
	for (size_t i = 0; i < Civilizations.size(); ++i) {
		delete Civilizations[i];
	}
	Civilizations.clear();
}

/**
**	@brief	Destructor
*/
CCivilization::~CCivilization()
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

int CCivilization::GetUpgradePriority(const CUpgrade *upgrade) const
{
	if (!upgrade) {
		fprintf(stderr, "Error in CCivilization::GetUpgradePriority: the upgrade is null.\n");
	}
	
	if (this->UpgradePriorities.find(upgrade) != this->UpgradePriorities.end()) {
		return this->UpgradePriorities.find(upgrade)->second;
	}
	
	return 100;
}

int CCivilization::GetForceTypeWeight(int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in CCivilization::GetForceTypeWeight: the force_type is -1.\n");
	}
	
	if (this->ForceTypeWeights.find(force_type) != this->ForceTypeWeights.end()) {
		return this->ForceTypeWeights.find(force_type)->second;
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

std::vector<CForceTemplate *> CCivilization::GetForceTemplates(int force_type) const
{
	if (force_type == -1) {
		fprintf(stderr, "Error in CCivilization::GetForceTemplates: the force_type is -1.\n");
	}
	
	if (this->ForceTemplates.find(force_type) != this->ForceTemplates.end()) {
		return this->ForceTemplates.find(force_type)->second;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetForceTemplates(force_type);
	}
	
	return std::vector<CForceTemplate *>();
}

std::vector<CAiBuildingTemplate *> CCivilization::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetAiBuildingTemplates();
	}
	
	return std::vector<CAiBuildingTemplate *>();
}

std::map<int, std::vector<std::string>> &CCivilization::GetPersonalNames()
{
	if (this->PersonalNames.size() > 0) {
		return this->PersonalNames;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetPersonalNames();
	}
	
	return this->PersonalNames;
}

std::vector<std::string> &CCivilization::GetUnitClassNames(int class_id)
{
	if (this->UnitClassNames[class_id].size() > 0) {
		return this->UnitClassNames[class_id];
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetUnitClassNames(class_id);
	}
	
	return this->UnitClassNames[class_id];
}

std::vector<std::string> &CCivilization::GetShipNames()
{
	if (this->ShipNames.size() > 0) {
		return this->ShipNames;
	}
	
	if (this->ParentCivilization) {
		return this->ParentCivilization->GetShipNames();
	}
	
	return this->ShipNames;
}

//@}
