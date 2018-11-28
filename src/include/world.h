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
/**@name world.h - The world headerfile. */
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

#ifndef __WORLD_H__
#define __WORLD_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CPlane;
class CProvince;
class CSpecies;
class CTerrainFeature;

class CWorld
{
public:
	CWorld() :
		ID(-1), Plane(NULL), HoursPerDay(DefaultHoursPerDay), DaysPerYear(DefaultDaysPerYear)
	{
	}
	
	static CWorld *GetWorld(std::string world_ident);
	static CWorld *GetOrAddWorld(std::string world_ident);
	static void ClearWorlds();
	
	static std::vector<CWorld *> Worlds;								/// Worlds

	int ID;																/// ID of this world
	int HoursPerDay;													/// How many hours does a day in this world contain
	int DaysPerYear;													/// How many days does a year in this world contain
	std::string Ident;
	std::string Name;
	std::string Description;
	std::string Background;
	std::string Quote;
	CPlane *Plane;
	std::vector<CProvince *> Provinces;									/// Provinces in this world
	std::vector<CTerrainFeature *> TerrainFeatures;						/// Terrain features in this world
	std::vector<CSpecies *> Species;									/// Species in this world
};

//@}

#endif // !__WORLD_H__
