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
//      (c) Copyright 2016-2022 by Andrettin
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

#pragma once

#include "vec2i.h"

class CGraphic;
class CProvince;
class CUpgrade;
class WorldMapTile;

namespace wyrmgus {
	class character;
	class faction;
	class region;
	class site;
	class unit_type;
	class world;
}

class CWorldMapTerrainType
{
public:
	CWorldMapTerrainType() :
		HasTransitions(false), Water(false), ID(-1), BaseTile(-1), Variations(0)
	{
	}

	std::string Name;
	std::string Tag;				/// used to locate graphic files
	bool HasTransitions;
	bool Water;
	int ID;
	int BaseTile;
	int Variations;					/// quantity of variations
};

class CProvince
{
public:
	CProvince() :
		ID(-1),
		Water(false), Coastal(false)
	{
	}
	
	std::string Name;
	wyrmgus::world *world = nullptr;
	int ID;																/// ID of this province
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	std::map<int, std::string> CulturalNames;							/// Names for the province for each different culture/civilization
	std::map<wyrmgus::faction *, std::string> FactionCulturalNames;				/// Names for the province for each different faction
	std::vector<wyrmgus::faction *> FactionClaims;								/// Factions which have a claim to this province
	std::vector<wyrmgus::region *> Regions;										/// Regions to which this province belongs
	std::map<int, wyrmgus::faction *> HistoricalOwners;							/// Historical owners of the province, mapped to the year
	std::map<int, wyrmgus::faction *> HistoricalClaims;							/// Historical claims over the province, mapped to the year
	std::map<int, int> HistoricalCultures;								/// Historical cultures which were predominant in the province, mapped to the year
	std::map<int, int> HistoricalPopulation;							/// Historical population, mapped to the year
	std::map<int, std::map<int, bool>> HistoricalSettlementBuildings;	/// Historical settlement buildings, mapped to building unit type id and year
	std::map<CUpgrade *, std::map<int, bool>> HistoricalModifiers;		/// Historical province modifiers, mapped to the modifier's upgrade and year
	std::map<std::tuple<int, int>, wyrmgus::character *> HistoricalGovernors;	/// Historical governors of the province, mapped to the beginning and end of the term
};

class WorldMapTile
{
public:
	WorldMapTile() :
		Terrain(-1), Resource(-1),
		Capital(false),
		Position(-1, -1)
	{
	}

	int Terrain;								/// Tile terrain (i.e. plains)
	int Resource;								/// The tile's resource, if any
	bool Capital;								/// Whether the tile is its province's capital
	Vec2i Position;								/// Position of the tile
	wyrmgus::world *world = nullptr;
	std::map<std::pair<int,int>, std::vector<std::string>> CulturalTerrainNames;			/// Names for the tile (if it has a certain terrain) for each culture/civilization
	std::map<std::pair<int, wyrmgus::faction *>, std::vector<std::string>> FactionCulturalTerrainNames;	/// Names for the tile (if it has a certain terrain) for each faction
	std::map<std::pair<int,int>, std::vector<std::string>> CulturalResourceNames;		/// Names for the tile (if it has a certain resource) for each culture/civilization
	std::map<std::pair<int, wyrmgus::faction *>, std::vector<std::string>> FactionCulturalResourceNames;	/// Names for the tile (if it has a certain resource) for each faction
	std::map<int, std::vector<std::string>> CulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::map<wyrmgus::faction *, std::vector<std::string>> FactionCulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::vector<wyrmgus::faction *> FactionClaims;								/// Factions which have a claim to this tile
	std::map<int, wyrmgus::faction *> HistoricalOwners;							/// Historical owners of the tile, mapped to the year
	std::map<int, wyrmgus::faction *> HistoricalClaims;							/// Historical claims over the tile, mapped to the year
};

extern std::vector<CProvince *> Provinces;
extern std::vector<CWorldMapTerrainType *>  WorldMapTerrainTypes;
extern std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

extern void CleanProvinces();
extern CProvince *GetProvince(const std::string &province_name);
extern int GetWorldMapTerrainTypeId(const std::string &terrain_type_name);
extern void ProvinceCclRegister();
