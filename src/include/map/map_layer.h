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
/**@name map_layer.h - The map layer header file. */
//
//      (c) Copyright 2018 by Andrettin
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

#ifndef __MAP_LAYER_H__
#define __MAP_LAYER_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <tuple>
#include <vector>

#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CMapField;
class CMapTemplate;
class CPlane;
class CUnit;
class CWorld;

class CMapLayer
{
public:
	CMapLayer() :
		ID(-1), HoursPerDay(DefaultHoursPerDay), TimeOfDay(0), SurfaceLayer(0),
		Overland(false),
		PixelTileSize(32, 32),
		Fields(NULL), Plane(NULL), World(NULL)
	{
	}

	~CMapLayer();
	void IncrementTimeOfDay();
	void SetTimeOfDay(const int time_of_day);
	int GetCyclesPerTimeOfDay() const;
	
	int ID;
	CMapField *Fields;						/// fields on the map layer
	int HoursPerDay;						/// how many hours does a day take in this map layer	
	int TimeOfDay;							/// the time of day for the map layer
	bool Overland;							/// whether the map layer is an overland map
	CPlane *Plane;							/// the plane pointer (if any) for the map layer
	CWorld *World;							/// the world pointer (if any) for the map layer
	int SurfaceLayer;						/// the surface layer for the map layer
	std::vector<CUnit *> LayerConnectors;	/// connectors in the map layer which lead to other map layers
	PixelSize PixelTileSize;				/// the pixel tile size for the map layer
	std::vector<std::tuple<Vec2i, Vec2i, CMapTemplate *>> SubtemplateAreas;
};

//@}

#endif // !__MAP_LAYER_H__
