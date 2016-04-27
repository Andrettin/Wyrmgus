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
/**@name province.cpp - The provinces. */
//
//      (c) Copyright 2016 by Andrettin
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

#include "province.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CWorld *> Worlds;
std::vector<CRegion *> Regions;
std::vector<CProvince *> Provinces;
std::vector<WorldMapTerrainType *>  WorldMapTerrainTypes;
std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CleanWorlds()
{
	for (size_t i = 0; i < WorldMapTerrainTypes.size(); ++i) {
		delete WorldMapTerrainTypes[i];
	}
	WorldMapTerrainTypes.clear();
	
	for (size_t i = 0; i < Worlds.size(); ++i) {
		for (std::map<std::pair<int,int>, WorldMapTile *>::iterator iterator = Worlds[i]->Tiles.begin(); iterator != Worlds[i]->Tiles.end(); ++iterator) {
			delete iterator->second;
		}
		
		for (size_t j = 0; j < Worlds[i]->Provinces.size(); ++j) {
			delete Worlds[i]->Provinces[j];
		}
		Worlds[i]->Provinces.clear();
		
		delete Worlds[i];
	}
	Worlds.clear();
	
	for (size_t j = 0; j < Regions.size(); ++j) {
		delete Regions[j];
	}
}

CWorld *GetWorld(std::string world_name)
{
	for (size_t i = 0; i < Worlds.size(); ++i) {
		if (world_name == Worlds[i]->Name) {
			return Worlds[i];
		}
	}
	return NULL;
}

CRegion *GetRegion(std::string region_name)
{
	for (size_t i = 0; i < Regions.size(); ++i) {
		if (region_name == Regions[i]->Name) {
			return Regions[i];
		}
	}
	return NULL;
}

CProvince *GetProvince(std::string province_name)
{
	for (size_t i = 0; i < Provinces.size(); ++i) {
		if (province_name == Provinces[i]->Name) {
			return Provinces[i];
		}
	}
	return NULL;
}

/**
**  Get the ID of a world map terrain type
*/
int GetWorldMapTerrainTypeId(std::string terrain_type_name)
{
	if (terrain_type_name.empty()) {
		return -1;
	}
	
	if (WorldMapTerrainTypeStringToIndex.find(terrain_type_name) != WorldMapTerrainTypeStringToIndex.end()) {
		return WorldMapTerrainTypeStringToIndex[terrain_type_name];
	}
	
	return -1;
}

std::string GetPathwayNameById(int pathway)
{
	if (pathway == PathwayTrail) {
		return "trail";
	} else if (pathway == PathwayRoad) {
		return "road";
	} else if (pathway == PathwayRailroad) {
		return "railroad";
	}

	return "";
}

int GetPathwayIdByName(std::string pathway)
{
	if (pathway == "trail") {
		return PathwayTrail;
	} else if (pathway == "road") {
		return PathwayRoad;
	} else if (pathway == "railroad") {
		return PathwayRailroad;
	}

	return -1;
}

int GetPathwayTransportLevel(int pathway)
{
	if (pathway == PathwayTrail) {
		return 1;
	} else if (pathway == PathwayRoad) {
		return 2;
	} else if (pathway == PathwayRailroad) {
		return 3;
	}

	return 0;
}

int GetTransportLevelMaximumCapacity(int transport_level)
{
	if (transport_level == 1) {
		return 200;
	} else if (transport_level == 2) {
		return 400;
	}

	return 0;
}

//@}
