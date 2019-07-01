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

#include "vec2i.h"

#pragma warning(push, 0)
#include <core/math/vector2.h>
#include <core/object.h>
#pragma warning(pop)

#include <tuple>
#include <vector>

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
class CTerrainType;
class CTimeOfDay;
class CTimeOfDaySchedule;
class CUnit;
class CWorld;
struct lua_State;

class CMapLayer : public Object
{
	GDCLASS(CMapLayer, Object)
	
public:
	CMapLayer(const int index = -1, const Vector2i &size = Vector2i(0, 0), CPlane *plane = nullptr, CWorld *world = nullptr, const int surface_layer = 0);
	~CMapLayer();
	
	int GetIndex() const
	{
		return this->Index;
	}
	
	const Vector2i &GetSize() const
	{
		return this->Size;
	}
	
	int GetWidth() const
	{
		return this->Size.width;
	}
	
	int GetHeight() const
	{
		return this->Size.height;
	}
	
	const CTerrainType *GetTileTerrainType(const Vector2i &pos, const bool overlay) const;
	
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
		return this->Field(x + y * this->GetWidth());
	}
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	pos	The coordinates of the map field
	**
	**	@return	The map field
	*/
	CMapField *Field(const Vector2i &pos) const
	{
		return this->Field(pos.x, pos.y);
	}
	
	Vector2i GetPosFromIndex(const unsigned int index) const
	{
		Vector2i pos;
		pos.x = index % this->GetWidth();
		pos.y = index / this->GetWidth();
		return pos;
	}
	
	int GetTileIndex(const int x, const int y) const
	{
		return x + y * this->GetWidth();
	}
	
	int GetTileIndex(const Vector2i &tile_pos) const
	{
		return this->GetTileIndex(tile_pos.x, tile_pos.y);
	}
	
	CPlane *GetPlane() const
	{
		return this->Plane;
	}
	
	CWorld *GetWorld() const
	{
		return this->World;
	}
	
	int GetSurfaceLayer() const
	{
		return this->SurfaceLayer;
	}
	
	void DoPerHourLoop();
	void RegenerateForest();
	/// regenerate a forest tile	
	void RegenerateForestTile(const Vector2i &pos);
	
	/// Returns true if there is a wall on the map tile field
	bool WallOnMap(const Vector2i &pos) const;
	
	bool TileBlockHasTree(const Vector2i &min_pos, const Vector2i &max_pos) const;
	
private:
	void DecrementRemainingTimeOfDayHours();
	void IncrementTimeOfDay();
public:
	void SetTimeOfDayByHours(const unsigned long long hours);
	void SetTimeOfDay(const CScheduledTimeOfDay *time_of_day);
	const CTimeOfDay *GetTimeOfDay() const;
private:
	void DecrementRemainingSeasonHours();
	void IncrementSeason();
public:
	void SetSeasonByHours(const unsigned long long hours);
	void SetSeason(const CScheduledSeason *season);
	const CSeason *GetSeason() const;
	
	bool IsUnderground() const
	{
		return this->SurfaceLayer > 0;
	}
	
	void AddLayerConnector(CUnit *connector)
	{
		this->LayerConnectors.push_back(connector);
	}
	
	const std::vector<CUnit *> &GetLayerConnectors() const
	{
		return this->LayerConnectors;
	}
	
private:
	int Index = -1;
	CMapField *Fields = nullptr;				/// fields on the map layer
	Vector2i Size = Vector2i(0, 0);				/// the size in tiles of the map layer
	const CScheduledTimeOfDay *TimeOfDay = nullptr;	/// the time of day for the map layer
	const CTimeOfDaySchedule *TimeOfDaySchedule = nullptr;	/// the time of day schedule for the map layer
	int RemainingTimeOfDayHours = 0;			/// the quantity of hours remaining for the current time of day to end
	const CScheduledSeason *Season = nullptr;			/// the current season for the map layer
	const CSeasonSchedule *SeasonSchedule = nullptr;	/// the season schedule for the map layer
	int RemainingSeasonHours = 0;				/// the quantity of hours remaining for the current season to end
	CPlane *Plane = nullptr;					/// the plane pointer (if any) for the map layer
	CWorld *World = nullptr;					/// the world pointer (if any) for the map layer
	int SurfaceLayer = 0;						/// the surface layer for the map layer
	std::vector<CUnit *> LayerConnectors;		/// connectors in the map layer which lead to other map layers
public:
	PixelSize PixelTileSize = PixelSize(32, 32);	/// the pixel tile size for the map layer
	std::vector<std::tuple<Vector2i, Vector2i, CMapTemplate *>> SubtemplateAreas;
	std::vector<Vector2i> DestroyedForestTiles;	/// destroyed forest tiles; this list is used for forest regeneration
	
	friend class CMap;
	friend class CMapTemplate;
	friend int CclStratagusMap(lua_State *l);
	friend void SetTimeOfDay(const std::string &time_of_day_ident, int z);
	friend void SetTimeOfDaySchedule(const std::string &time_of_day_schedule_ident, const unsigned z);
	friend void SetSeason(const std::string &season_ident, int z);
	friend void SetSeasonSchedule(const std::string &season_schedule_ident, int z);
	
protected:
	static void _bind_methods();
};

#endif
