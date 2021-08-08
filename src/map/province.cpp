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
//      (c) Copyright 2016-2021 by Andrettin
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

#include "stratagus.h"

#include "province.h"

std::vector<CProvince *> Provinces;
std::vector<CWorldMapTerrainType *> WorldMapTerrainTypes;
std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

void CleanProvinces()
{
	for (size_t i = 0; i < WorldMapTerrainTypes.size(); ++i) {
		delete WorldMapTerrainTypes[i];
	}
	WorldMapTerrainTypes.clear();
}

CProvince *GetProvince(const std::string &province_name)
{
	for (size_t i = 0; i < Provinces.size(); ++i) {
		if (province_name == Provinces[i]->Name) {
			return Provinces[i];
		}
	}
	return nullptr;
}

int GetWorldMapTerrainTypeId(const std::string &terrain_type_name)
{
	if (terrain_type_name.empty()) {
		return -1;
	}
	
	if (WorldMapTerrainTypeStringToIndex.find(terrain_type_name) != WorldMapTerrainTypeStringToIndex.end()) {
		return WorldMapTerrainTypeStringToIndex[terrain_type_name];
	}
	
	return -1;
}
