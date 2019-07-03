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
/**@name province.h - The province header file. */
//
//      (c) Copyright 2016-2019 by Andrettin
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

#ifndef __PROVINCE_H__
#define __PROVINCE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

#include <core/math/vector2.h>

#include <tuple>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CFaction;
class CSite;
class CUpgrade;
class CWorld;
class Province;
class WorldMapTile;
struct lua_State;

/*----------------------------------------------------------------------------
--  Enumerations
----------------------------------------------------------------------------*/

/**
**  Indexes into era array.
*/
enum Eras {
	EraDevonian,
	EraCarboniferous,
	EraPermian,
	EraTriassic,
	EraJurassic,
	EraCretaceous,
	EraPaleocene,
	EraEocene,
	EraOligocene,
	EraMiocene,
	EraPliocene,
	EraPleistocene,
	EraHolocene,
	
	MaxEras
};

/*----------------------------------------------------------------------------
--  Definitions
----------------------------------------------------------------------------*/

class CWorldMapTerrainType
{
public:
	std::string Name;
	std::string Tag;				/// used to locate graphic files
	bool HasTransitions = false;
	bool Water = false;
	int ID = -1;
	int BaseTile = -1;
	int Variations = 0;				/// quantity of variations
};

class CRegion
{
public:
	std::string Ident;
	std::string Name;
	int ID = -1;														/// ID of this region
	std::vector<Province *> Provinces;									/// Provinces which belong to this region
	std::vector<CSite *> Sites;											/// Sites which belong to this region
	std::map<int, int> HistoricalPopulation;							/// Historical population, mapped to the year
};

class Province : public DetailedDataElement, public DataType<Province>
{
	GDCLASS(Province, DetailedDataElement)

public:
	static constexpr const char *ClassIdentifier = "province";
	
	bool IsWater() const
	{
		return this->Water;
	}
	
public:
	const CWorld *World = nullptr;										/// the world to which the province belongs
private:
	bool Water = false;													/// whether the province is a water province or not
public:
	bool Coastal = false;												/// whether the province is a coastal province or not
	std::map<int, String> CulturalNames;								/// names for the province for each different culture/civilization
	std::vector<CSite *> Sites;											/// sites which belong to this province
	std::vector<CFaction *> FactionClaims;								/// factions which have a claim to this province
	std::vector<CRegion *> Regions;										/// regions to which this province belongs
	std::map<int, CFaction *> HistoricalOwners;							/// historical owners of the province, mapped to the year
	std::map<int, CFaction *> HistoricalClaims;							/// historical claims over the province, mapped to the year
	std::map<int, int> HistoricalCultures;								/// historical cultures which were predominant in the province, mapped to the year
	std::map<int, int> HistoricalPopulation;							/// historical population, mapped to the year
	std::map<int, std::map<int, bool>> HistoricalSettlementBuildings;	/// historical settlement buildings, mapped to building unit type id and year
	std::map<CUpgrade *, std::map<int, bool>> HistoricalModifiers;		/// historical province modifiers, mapped to the modifier's upgrade and year
	std::map<std::tuple<int, int>, CCharacter *> HistoricalGovernors;	/// historical governors of the province, mapped to the beginning and end of the term
	
	friend int CclDefineProvince(lua_State *l);

protected:
	static void _bind_methods();
};

class WorldMapTile
{
public:
	int Terrain = -1;							/// Tile terrain (i.e. plains)
	int Resource = -1;							/// The tile's resource, if any
	bool Capital = false;						/// Whether the tile is its province's capital
	Vector2i Position = Vector2i(-1, -1);				/// Position of the tile
	CWorld *World = nullptr;
	std::map<std::pair<int,int>, std::vector<std::string>> CulturalTerrainNames;	/// Names for the tile (if it has a certain terrain) for each culture/civilization
	std::map<std::pair<int,const CFaction *>, std::vector<std::string>> FactionCulturalTerrainNames;	/// Names for the tile (if it has a certain terrain) for each faction
	std::map<std::pair<int,int>, std::vector<std::string>> CulturalResourceNames;	/// Names for the tile (if it has a certain resource) for each culture/civilization
	std::map<std::pair<int,const CFaction *>, std::vector<std::string>> FactionCulturalResourceNames;	/// Names for the tile (if it has a certain resource) for each faction
	std::map<int, std::vector<std::string>> CulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::map<const CFaction *, std::vector<std::string>> FactionCulturalSettlementNames;	/// Names for the tile's settlement for each faction
	std::vector<CFaction *> FactionClaims;								/// Factions which have a claim to this tile
	std::map<int, CFaction *> HistoricalOwners;							/// Historical owners of the tile, mapped to the year
	std::map<int, CFaction *> HistoricalClaims;							/// Historical claims over the tile, mapped to the year
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CRegion *> Regions;
extern std::vector<CWorldMapTerrainType *>  WorldMapTerrainTypes;
extern std::map<std::string, int> WorldMapTerrainTypeStringToIndex;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanTerrainFeatures();
extern CRegion *GetRegion(const std::string &region_name);
extern int GetWorldMapTerrainTypeId(const std::string &terrain_type_name);
extern std::string GetEraNameById(int era);
extern int GetEraIdByName(const std::string &era);
extern void ProvinceCclRegister();

#endif
