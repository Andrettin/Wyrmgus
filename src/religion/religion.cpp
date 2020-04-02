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
/**@name religion.cpp - The religion source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "religion/religion.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CReligion *> CReligion::Religions;
std::map<std::string, CReligion *> CReligion::ReligionsByIdent;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a religion
**
**	@param	ident		The religion's string identifier
**	@param	should_find	Whether it is an error if the religion could not be found; this is true by default
**
**	@return	The religion if found, or null otherwise
*/
CReligion *CReligion::GetReligion(const std::string &ident, const bool should_find)
{
	if (ReligionsByIdent.find(ident) != ReligionsByIdent.end()) {
		return ReligionsByIdent.find(ident)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid religion: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a religion
**
**	@param	ident	The religion's string identifier
**
**	@return	The religion if found, or a newly-created one otherwise
*/
CReligion *CReligion::GetOrAddReligion(const std::string &ident)
{
	CReligion *religion = GetReligion(ident, false);
	
	if (!religion) {
		religion = new CReligion;
		religion->Ident = ident;
		Religions.push_back(religion);
		ReligionsByIdent[ident] = religion;
	}
	
	return religion;
}

/**
**	@brief	Remove the existing religions
*/
void CReligion::ClearReligions()
{
	for (size_t i = 0; i < Religions.size(); ++i) {
		delete Religions[i];
	}
	Religions.clear();
}
