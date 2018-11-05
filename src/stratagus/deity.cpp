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
/**@name deity.cpp - The deities. */
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

#include "deity.h"

#include "config.h"
#include "deity_domain.h"
#include "player.h"
#include "province.h"
#include "religion.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CDeity *> CDeity::Deities;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CDeity *CDeity::GetDeity(std::string deity_ident)
{
	for (size_t i = 0; i < Deities.size(); ++i) {
		if (deity_ident == Deities[i]->Ident) {
			return Deities[i];
		}
	}
	
	return NULL;
}

void CDeity::Clean()
{
	for (size_t i = 0; i < Deities.size(); ++i) {
		delete Deities[i];
	}
	Deities.clear();
}

void CDeity::ProcessConfigData(CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "pantheon") {
			this->Pantheon = value;
		} else if (key == "upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			this->UpgradeIdent = value;
		} else if (key == "gender") {
			this->Gender = GetGenderIdByName(value);
		} else if (key == "major") {
			this->Major = StringToBool(value);
		} else if (key == "civilization") {
			value = FindAndReplaceString(value, "_", "-");
			int civilization = PlayerRaces.GetRaceIndexByName(value.c_str());
			if (civilization != -1) {
				this->Civilizations.push_back(civilization);
			} else {
				fprintf(stderr, "Civilization \"%s\" doesn't exist.", value.c_str());
			}
		} else if (key == "religion") {
			value = FindAndReplaceString(value, "_", "-");
			CReligion *religion = CReligion::GetReligion(value.c_str());
			if (religion) {
				this->Religions.push_back(religion);
			} else {
				fprintf(stderr, "Religion \"%s\" doesn't exist.", value.c_str());
			}
		} else if (key == "domain") {
			value = FindAndReplaceString(value, "_", "-");
			CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(value.c_str());
			if (deity_domain) {
				this->Domains.push_back(deity_domain);
			} else {
				fprintf(stderr, "Deity domain \"%s\" doesn't exist.", value.c_str());
			}
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "background") {
			this->Background = value;
		} else if (key == "quote") {
			this->Quote = value;
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = NULL;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (key == "home_plane") {
			value = FindAndReplaceString(value, "_", "-");
			CPlane *plane = GetPlane(value);
			if (plane) {
				this->HomePlane = plane;
			} else {
				fprintf(stderr, "Plane \"%s\" doesn't exist.", value.c_str());
			}
		}
	}
}

//@}
