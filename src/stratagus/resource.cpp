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
/**@name resource.cpp - The resource source file. */
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

#include "resource.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CResource *> CResource::Resources;
std::map<std::string, CResource *> CResource::ResourcesByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a resource
**
**	@param	ident		The resource's string identifier
**	@param	should_find	Whether it is an error if the resource couldn't be found
**
**	@return	The resource if found, or null otherwise
*/
CResource *CResource::GetResource(const std::string &ident, const bool should_find)
{
	std::map<std::string, CResource *>::const_iterator find_iterator = ResourcesByIdent.find(ident);
	
	if (find_iterator != ResourcesByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid resource: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a resource
**
**	@param	ident	The resource's string identifier
**
**	@return	The resource if found, otherwise a new resource is created and returned
*/
CResource *CResource::GetOrAddResource(const std::string &ident)
{
	CResource *resource = GetResource(ident, false);
	
	if (!resource) {
		resource = new CResource;
		resource->Ident = ident;
		Resources.push_back(resource);
		ResourcesByIdent[ident] = resource;
	}
	
	return resource;
}

/**
**	@brief	Remove the existing resources
*/
void CResource::ClearResources()
{
	for (size_t i = 0; i < Resources.size(); ++i) {
		delete Resources[i];
	}
	Resources.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CResource::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid resource property: \"%s\".\n", key.c_str());
		}
	}
}
