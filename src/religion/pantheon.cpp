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
/**@name pantheon.cpp - The pantheon source file. */
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

#include "religion/pantheon.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CPantheon *> CPantheon::Pantheons;
std::map<std::string, CPantheon *> CPantheon::PantheonsByIdent;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a pantheon
**
**	@param	ident		The pantheon's string identifier
**	@param	should_find	Whether it is an error if the pantheon could not be found; this is true by default
**
**	@return	The pantheon if found, or null otherwise
*/
CPantheon *CPantheon::GetPantheon(const std::string &ident, const bool should_find)
{
	std::map<std::string, CPantheon *>::const_iterator find_iterator = PantheonsByIdent.find(ident);
	
	if (find_iterator != PantheonsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid pantheon: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a pantheon
**
**	@param	ident	The pantheon's string identifier
**
**	@return	The pantheon if found, or a newly-created one otherwise
*/
CPantheon *CPantheon::GetOrAddPantheon(const std::string &ident)
{
	CPantheon *pantheon = GetPantheon(ident, false);
	
	if (!pantheon) {
		pantheon = new CPantheon;
		pantheon->Ident = ident;
		Pantheons.push_back(pantheon);
		PantheonsByIdent[ident] = pantheon;
	}
	
	return pantheon;
}

/**
**	@brief	Remove the existing pantheons
*/
void CPantheon::ClearPantheons()
{
	for (size_t i = 0; i < Pantheons.size(); ++i) {
		delete Pantheons[i];
	}
	Pantheons.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CPantheon::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "background") {
			this->Background = value;
		} else if (key == "quote") {
			this->Quote = value;
		} else {
			fprintf(stderr, "Invalid pantheon property: \"%s\".\n", key.c_str());
		}
	}
}
