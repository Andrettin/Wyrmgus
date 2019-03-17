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
//      (c) Copyright 2018-2019 by Andrettin
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
class CScheduledSeason;
class CScheduledTimeOfDay;
class CSeason;
class CSeasonSchedule;
class CTimeOfDay;
class CTimeOfDaySchedule;
class CUnit;
class CWorld;

class CMapLayer
{
public:
	CMapLayer(const int width, const int height);
	~CMapLayer();
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	index	The index of the map field
	**
	**	@return	The map field
	*/
	CMapField *Field(const unsigned int index) const;
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	x	The x coordinate of the map field
	**	@param	y	The y coordinate of the map field
	**
	**	@return	The map field
	*/
	CMapField *Field(const int x, const int y) const
	{
		return this->Field(x + y * this->Width);
	}
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	pos	The coordinates of the map field
	**
	**	@return	The map field
	*/
	CMapField *Field(const Vec2i &pos) const
	{
		return this->Field(pos.x, pos.y);
	}
	
	Vec2i GetPosFromIndex(unsigned int index) const
	{
		Vec2i pos;
		pos.x = index % this->Width;
		pos.y = index / this->Width;
		return pos;
	}
	
	int GetWidth() const
	{
		return this->Width;
	}
	
	int GetHeight() const
	{
		return this->Height;
	}
	
	void DoPerCycleLoop();
	void DoPerHourLoop();
	void RegenerateForest();
	//regenerate a forest tile	
	void RegenerateForestTile(const Vec2i &pos);
private:
	void DecrementRemainingTimeOfDayHours();
	void IncrementTimeOfDay();
public:
	void SetTimeOfDayByHours(const unsigned long long hours);
	void SetTimeOfDay(CScheduledTimeOfDay *time_of_day);
	CTimeOfDay *GetTimeOfDay() const;
private:
	void DecrementRemainingSeasonHours();
	void IncrementSeason();
public:
	void SetSeasonByHours(const unsigned long long hours);
	void SetSeason(CScheduledSeason *season);
	CSeason *GetSeason() const;
	
	int ID = -1;
private:
	CMapField *Fields = nullptr;				/// fields on the map layer
	int Width = 0;								/// the width in tiles of the map layer
	int Height = 0;								/// the height in tiles of the map layer
public:
	CScheduledTimeOfDay *TimeOfDay = nullptr;	/// the time of day for the map layer
	CTimeOfDaySchedule *TimeOfDaySchedule = nullptr;	/// the time of day schedule for the map layer
	int RemainingTimeOfDayHours = 0;			/// the quantity of hours remaining for the current time of day to end
	CScheduledSeason *Season = nullptr;			/// the current season for the map layer
	CSeasonSchedule *SeasonSchedule = nullptr;	/// the season schedule for the map layer
	int RemainingSeasonHours = 0;				/// the quantity of hours remaining for the current season to end
	bool Overland = false;						/// whether the map layer is an overland map
	CPlane *Plane = nullptr;					/// the plane pointer (if any) for the map layer
	CWorld *World = nullptr;					/// the world pointer (if any) for the map layer
	int SurfaceLayer = 0;						/// the surface layer for the map layer
	std::vector<CUnit *> LayerConnectors;		/// connectors in the map layer which lead to other map layers
	PixelSize PixelTileSize = PixelSize(32, 32);	/// the pixel tile size for the map layer
	std::vector<std::tuple<Vec2i, Vec2i, CMapTemplate *>> SubtemplateAreas;
	std::vector<Vec2i> DestroyedForestTiles;	/// destroyed forest tiles; this list is used for forest regeneration
};

#endif
