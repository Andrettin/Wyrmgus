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
//      (c) Copyright 2016-2018 by Andrettin
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

#include "map.h" // for the map templates, which are cleaned here
#include "tileset.h" // for the terrain types, which are cleaned here

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CRegion *> Regions;
std::vector<CProvince *> Provinces;
std::vector<CWorldMapTerrainType *> WorldMapTerrainTypes;
std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CleanProvinces()
{
	for (size_t i = 0; i < Sites.size(); ++i) {
		delete Sites[i];
	}
	Sites.clear();
	
	for (size_t i = 0; i < TerrainFeatures.size(); ++i) {
		delete TerrainFeatures[i];
	}
	TerrainFeatures.clear();
	
	for (size_t i = 0; i < WorldMapTerrainTypes.size(); ++i) {
		delete WorldMapTerrainTypes[i];
	}
	WorldMapTerrainTypes.clear();
	
	for (size_t j = 0; j < Regions.size(); ++j) {
		delete Regions[j];
	}
}

CRegion *GetRegion(std::string region_ident)
{
	for (size_t i = 0; i < Regions.size(); ++i) {
		if (region_ident == Regions[i]->Ident) {
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

std::string GetEraNameById(int era)
{
	if (era == EraDevonian) {
		return "devonian";
	} else if (era == EraCarboniferous) {
		return "carboniferous";
	} else if (era == EraPermian) {
		return "permian";
	} else if (era == EraTriassic) {
		return "triassic";
	} else if (era == EraJurassic) {
		return "jurassic";
	} else if (era == EraCretaceous) {
		return "cretaceous";
	} else if (era == EraPaleocene) {
		return "paleocene";
	} else if (era == EraEocene) {
		return "eocene";
	} else if (era == EraOligocene) {
		return "oligocene";
	} else if (era == EraMiocene) {
		return "miocene";
	} else if (era == EraPliocene) {
		return "pliocene";
	} else if (era == EraPleistocene) {
		return "pleistocene";
	} else if (era == EraHolocene) {
		return "holocene";
	}

	return "";
}

int GetEraIdByName(std::string era)
{
	if (era == "devonian") {
		return EraDevonian;
	} else if (era == "carboniferous") {
		return EraCarboniferous;
	} else if (era == "permian") {
		return EraPermian;
	} else if (era == "triassic") {
		return EraTriassic;
	} else if (era == "jurassic") {
		return EraJurassic;
	} else if (era == "cretaceous") {
		return EraCretaceous;
	} else if (era == "paleocene") {
		return EraPaleocene;
	} else if (era == "eocene") {
		return EraEocene;
	} else if (era == "oligocene") {
		return EraOligocene;
	} else if (era == "miocene") {
		return EraMiocene;
	} else if (era == "pliocene") {
		return EraPliocene;
	} else if (era == "pleistocene") {
		return EraPleistocene;
	} else if (era == "holocene") {
		return EraHolocene;
	}

	return -1;
}

//@}
