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
/**@name world.h - The world header file. */
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

#ifndef __WORLD_H__
#define __WORLD_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CPlane;
class CProvince;
class CSeasonSchedule;
class CSpecies;
class CTerrainFeature;
class CTimeOfDaySchedule;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CWorld : public DetailedDataElement, public DataType<CWorld>
{
	DATA_TYPE(CWorld, DetailedDataElement)
	
public:
	static constexpr const char *ClassIdentifier = "world";

private:
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(Plane);
		REGISTER_PROPERTY(SeasonSchedule);
		REGISTER_PROPERTY(TimeOfDaySchedule);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();

public:
	static CWorld *Add(const std::string &ident);
	static void Clear();
	
	Property<CPlane *> Plane = nullptr;
	Property<CTimeOfDaySchedule *> TimeOfDaySchedule = nullptr;			/// this world's time of day schedule
	Property<CSeasonSchedule *> SeasonSchedule = nullptr;				/// this world's season schedule
	std::vector<CProvince *> Provinces;									/// Provinces in this world
	std::vector<CTerrainFeature *> TerrainFeatures;						/// Terrain features in this world
	std::vector<CSpecies *> Species;									/// Species in this world

protected:
	static void _bind_methods();
};

#endif
