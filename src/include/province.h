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
/**@name province.h - The province headerfile. */
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

#ifndef __PROVINCE_H__
#define __PROVINCE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <map>

#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnitType;
class CUpgrade;
class CFaction;

class CProvince
{
public:
	CProvince() :
		ID(-1),
		Water(false), Coastal(false), SettlementLocation(-1, -1)
	{
	}
	
	std::string Name;
	std::string SettlementName;
	std::string World;
	std::string Map;
	std::string SettlementTerrain;
	int ID;																/// ID of this province
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	Vec2i SettlementLocation;											/// In which tile the province's settlement is located
	std::string CulturalNames[MAX_RACES];								/// Names for the province for each different culture/civilization
	std::map<CFaction *, std::string> FactionCulturalNames;				/// Names for the province for each different faction
	std::string CulturalSettlementNames[MAX_RACES];						/// Names for the province's settlement for each different culture/civilization
	std::map<CFaction *, std::string> FactionCulturalSettlementNames;	/// Names for the province's settlement for each different faction
	std::vector<CFaction *> FactionClaims;								/// Factions which have a claim to this province
	std::vector<Vec2i> Tiles;
};


/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CProvince *> Provinces;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanProvinces();
extern CProvince *GetProvince(std::string province_name);
extern void ProvinceCclRegister();

//@}

#endif // !__PROVINCE_H__
