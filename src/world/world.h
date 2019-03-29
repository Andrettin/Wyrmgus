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

#include <map>

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

class CWorld : public CDataType
{
	GDCLASS(CWorld, CDataType)

public:
	static CWorld *GetWorld(const std::string &ident, const bool should_find = true);
	static CWorld *GetOrAddWorld(const std::string &ident);
	static void ClearWorlds();
	
	static std::vector<CWorld *> Worlds;								/// Worlds
	static std::map<std::string, CWorld *> WorldsByIdent;

	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;

	std::string Name;
	std::string Description;
	std::string Background;
	std::string Quote;
	CPlane *Plane = nullptr;
	CTimeOfDaySchedule *TimeOfDaySchedule = nullptr;					/// this world's time of day schedule
	CSeasonSchedule *SeasonSchedule = nullptr;							/// this world's season schedule
	std::vector<CProvince *> Provinces;									/// Provinces in this world
	std::vector<CTerrainFeature *> TerrainFeatures;						/// Terrain features in this world
	std::vector<CSpecies *> Species;									/// Species in this world

protected:
	static inline void _bind_methods() {}
};

#endif
