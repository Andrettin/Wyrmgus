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
/**@name pathfinder.cpp - The path finder routines. */
//
//      I use breadth-first.
//
//      (c) Copyright 1998-2007 by Lutz Sammer and Russell Smith
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

#include "pathfinder.h"

#include "actions.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/size_util.h"

//astar.cpp

/// Init the a* data structures
//Wyrmgus start
//extern void InitAStar(int mapWidth, int mapHeight);
extern void InitAStar();
//Wyrmgus end

/// free the a* data structures
extern void FreeAStar();

/// Find and a* path for a unit
//Wyrmgus start
/*
extern int AStarFindPath(const Vec2i &startPos, const Vec2i &goalPos, int gw, int gh,
						 int tilesizex, int tilesizey, int minrange,
						 //Wyrmgus start
//						 int maxrange, char *path, int pathlen, const CUnit &unit);
						 int maxrange, char *path, int pathlen, const CUnit &unit, int max_length, int z);
						 //Wyrmgus end
*/
//Wyrmgus end

void TerrainTraversal::SetSize(unsigned int width, unsigned int height)
{
	m_values.resize((width + 2) * (height + 2));
	m_extented_width = width + 2;
	m_height = height;
}

void TerrainTraversal::SetDiagonalAllowed(const bool allowed)
{
	allow_diagonal = allowed;
}

void TerrainTraversal::Init()
{
	const unsigned int height = m_height;
	const unsigned int width = m_extented_width - 2;
	const unsigned int width_ext = m_extented_width;

	memset(&m_values[0], '\xFF', width_ext * sizeof(dataType));
	for (unsigned i = 1; i < 1 + height; ++i) {
		m_values[i * width_ext] = -1;
		memset(&m_values[i * width_ext + 1], '\0', width * sizeof(dataType));
		m_values[i * width_ext + width + 1] = -1;
	}
	memset(&m_values[(height + 1) * width_ext], '\xFF', width_ext * sizeof(dataType));
}

void TerrainTraversal::PushPos(const Vec2i &pos)
{
	if (IsVisited(pos) == false) {
		m_queue.push(PosNode(pos, pos));
		Set(pos, 1);
	}
}

void TerrainTraversal::push_pos_if_passable(const QPoint &pos, const int z, const tile_flag passability_mask)
{
	if (!CMap::get()->Info.IsPointOnMap(pos, z) || !CanMoveToMask(pos, passability_mask, z)) {
		return;
	}

	this->PushPos(pos);
}

void TerrainTraversal::PushNeighbor(const Vec2i &pos)
{
	static constexpr std::array<Vec2i, 8> offsets = { Vec2i(0, -1), Vec2i(-1, 0), Vec2i(1, 0), Vec2i(0, 1), Vec2i(-1, -1), Vec2i(1, -1), Vec2i(-1, 1), Vec2i(1, 1) };

	int offsets_size = allow_diagonal ? 8 : 4;
	for (int i = 0; i != offsets_size; ++i) {
		const Vec2i newPos = pos + offsets[i];

		if (IsVisited(newPos) == false) {
			m_queue.push(PosNode(newPos, pos));
			Set(newPos, Get(pos) + 1);
		}
	}
}

void TerrainTraversal::push_pos_rect(const QRect &rect)
{
	const QPoint top_left = rect.topLeft();
	const QPoint bottom_right = rect.bottomRight();

	for (int y = top_left.y(); y <= bottom_right.y(); ++y) {
		for (int x = top_left.x(); x <= bottom_right.x(); ++x) {
			this->PushPos(QPoint(x, y));
		}
	}
}

void TerrainTraversal::push_pos_rect_if_passable(const QRect &rect, const int z, const tile_flag passability_mask)
{
	const QPoint top_left = rect.topLeft();
	const QPoint bottom_right = rect.bottomRight();

	for (int y = top_left.y(); y <= bottom_right.y(); ++y) {
		for (int x = top_left.x(); x <= bottom_right.x(); ++x) {
			this->push_pos_if_passable(QPoint(x, y), z, passability_mask);
		}
	}
}

void TerrainTraversal::push_pos_rect_borders(const QRect &rect)
{
	const QPoint top_left = rect.topLeft();
	const QPoint bottom_right = rect.bottomRight();

	for (int x = top_left.x(); x <= bottom_right.x(); ++x) {
		this->PushPos(QPoint(x, top_left.y()));
		this->PushPos(QPoint(x, bottom_right.y()));
	}

	for (int y = top_left.y(); y <= bottom_right.y(); ++y) {
		this->PushPos(QPoint(top_left.x(), y));
		this->PushPos(QPoint(bottom_right.x(), y));
	}
}

void TerrainTraversal::push_pos_rect_borders_if_passable(const QRect &rect, const int z, const tile_flag passability_mask)
{
	const QPoint top_left = rect.topLeft();
	const QPoint bottom_right = rect.bottomRight();

	for (int x = top_left.x(); x <= bottom_right.x(); ++x) {
		this->push_pos_if_passable(QPoint(x, top_left.y()), z, passability_mask);
		this->push_pos_if_passable(QPoint(x, bottom_right.y()), z, passability_mask);
	}

	for (int y = top_left.y(); y <= bottom_right.y(); ++y) {
		this->push_pos_if_passable(QPoint(top_left.x(), y), z, passability_mask);
		this->push_pos_if_passable(QPoint(bottom_right.x(), y), z, passability_mask);
	}
}

void TerrainTraversal::PushUnitPosAndNeighbor(const CUnit &unit)
{
	const CUnit *start_unit = unit.GetFirstContainer();

	//Wyrmgus start
	if (start_unit == nullptr) {
		fprintf(stderr, "TerrainTraversal::PushUnitPosAndNeighbor() error: startUnit is null.\n");
	} else if (start_unit->Type == nullptr) {
		fprintf(stderr, "TerrainTraversal::PushUnitPosAndNeighbor() error: startUnit's \"%s\" (ID %d) (%d, %d) type is null.\n", start_unit->Name.c_str(), UnitNumber(*start_unit), start_unit->tilePos.x, start_unit->tilePos.y);
	}
	//Wyrmgus end

	const QPoint offset(1, 1);
	const QPoint start = start_unit->tilePos - offset;
	const QPoint end = start_unit->get_bottom_right_tile_pos() + offset;

	const QRect rect(start, end);
	this->push_pos_rect(rect);
}

void TerrainTraversal::push_unit_pos_and_neighbor_if_passable(const CUnit &unit, const tile_flag passability_mask)
{
	const CUnit *start_unit = unit.GetFirstContainer();

	if (start_unit == nullptr) {
		fprintf(stderr, "TerrainTraversal::push_unit_pos_and_passable_neighbor() error: startUnit is null.\n");
	} else if (start_unit->Type == nullptr) {
		fprintf(stderr, "TerrainTraversal::push_unit_pos_and_passable_neighbor() error: startUnit's \"%s\" (ID %d) (%d, %d) type is null.\n", start_unit->Name.c_str(), UnitNumber(*start_unit), start_unit->tilePos.x, start_unit->tilePos.y);
	}

	const QPoint offset(1, 1);
	const QPoint start = start_unit->tilePos - offset;
	const QPoint end = start_unit->get_bottom_right_tile_pos() + offset;

	const QRect rect(start, end);
	this->push_pos_rect_if_passable(rect, start_unit->MapLayer->ID, passability_mask);
}

bool TerrainTraversal::IsVisited(const Vec2i &pos) const
{
	return Get(pos) != 0;
}

bool TerrainTraversal::IsReached(const Vec2i &pos) const
{
	return Get(pos) != 0 && Get(pos) != -1;
}

bool TerrainTraversal::IsInvalid(const Vec2i &pos) const
{
	return Get(pos) != -1;
}

TerrainTraversal::dataType TerrainTraversal::Get(const Vec2i &pos) const
{
	return m_values[m_extented_width + 1 + pos.y * m_extented_width + pos.x];
}

void TerrainTraversal::Set(const Vec2i &pos, TerrainTraversal::dataType value)
{
	m_values[m_extented_width + 1 + pos.y * m_extented_width + pos.x] = value;
}

/**
**  Init the pathfinder
*/
void InitPathfinder()
{
	//Wyrmgus start
//	InitAStar(Map.Info.MapWidth, Map.Info.MapHeight);
	InitAStar();
	//Wyrmgus end
}

/**
**  Free the pathfinder
*/
void FreePathfinder()
{
	FreeAStar();
}

/*----------------------------------------------------------------------------
--  PATH-FINDER USE
----------------------------------------------------------------------------*/

/**
**  Can the unit 'src' reach the place goalPos.
**
**  @param src       Unit for the path.
**  @param goalPos   Map tile position.
**  @param w         Width of Goal
**  @param h         Height of Goal
**  @param minrange  min range to the tile
**  @param range     Range to the tile.
**
**  @return          Distance to place.
*/
//Wyrmgus start
//int PlaceReachable(const CUnit &src, const Vec2i &goalPos, int w, int h, int minrange, int range)
int PlaceReachable(const CUnit &src, const Vec2i &goalPos, int w, int h, int minrange, int range, int max_length, int z, bool from_outside_container)
//Wyrmgus end
{
	//Wyrmgus start
	if (src.MapLayer->ID != z) {
		return 0;
	}
	//Wyrmgus end
	
	int i = PF_FAILED;
	if (!src.Container || !from_outside_container) {
		i = AStarFindPath(src.tilePos, goalPos, w, h,
						  src.Type->get_tile_width(), src.Type->get_tile_height(),
						  minrange, range, nullptr, src, max_length, z);
	} else {
		const Vec2i offset(1, 1);
		const Vec2i extra_tile_size(src.Container->Type->get_tile_size() - QSize(1, 1));
		const Vec2i start_pos = src.Container->tilePos - offset;
		const Vec2i end_pos = src.Container->tilePos + extra_tile_size + offset;
		const Vec2i pos_diff = end_pos - start_pos;

		int temp_i = PF_FAILED;
		for (Vec2i it = start_pos; it.y <= end_pos.y; it.y += pos_diff.y) {
			for (it.x = start_pos.x; it.x <= end_pos.x; it.x += pos_diff.x) {
				if (!CMap::get()->Info.IsPointOnMap(it, src.Container->MapLayer)) {
					continue;
				}

				if (!CanMoveToMask(it, src.Type->MovementMask, z)) {
					//ignore tiles to which the unit cannot be dropped from its container
					continue;
				}

				temp_i = AStarFindPath(it, goalPos, w, h,
						  src.Type->get_tile_width(), src.Type->get_tile_height(),
						  minrange, range, nullptr, src, max_length, z);
						  
				if (temp_i > i && i < PF_REACHED) {
					i = temp_i;
				}
			}
		}
	}

	switch (i) {
		case PF_FAILED:
		case PF_UNREACHABLE:
			i = 0;
			break;
		case PF_REACHED:
			/* since most of this function usage check return value as bool
			 * then reached state should be track as true value */
			i = 1;
			break;
		case PF_WAIT:
			Assert(0);
			i = 0;
			break;
		case PF_MOVE:
			break;
		default:
			break;
	}
	return i;
}

/**
**  Can the unit 'src' reach the unit 'dst'.
**
**  @param src    Unit for the path.
**  @param dst    Unit to be reached.
**  @param range  Range to unit.
**
**  @return       Distance to place.
*/
//Wyrmgus start
//int UnitReachable(const CUnit &src, const CUnit &dst, int range)
int UnitReachable(const CUnit &src, const CUnit &dst, int range, int max_length, bool from_outside_container)
//Wyrmgus end
{
	//  Find a path to the goal.
	if (src.Type->BoolFlag[BUILDING_INDEX].value) {
		return 0;
	}
	const int depth = PlaceReachable(src, dst.tilePos,
									 //Wyrmgus start
//									 dst.Type->get_tile_width(), dst.Type->get_tile_height(), 0, range);
									 dst.Type->get_tile_width(), dst.Type->get_tile_height(), 0, range, max_length, dst.MapLayer->ID, from_outside_container);
									 //Wyrmgus end
	if (depth <= 0) {
		return 0;
	}
	return depth;
}

/*----------------------------------------------------------------------------
--  REAL PATH-FINDER
----------------------------------------------------------------------------*/

//Wyrmgus start
//PathFinderInput::PathFinderInput() : unit(nullptr), minRange(0), maxRange(0),
PathFinderInput::PathFinderInput() : unit(nullptr), minRange(0), maxRange(0), MapLayer(0),
//Wyrmgus end
	isRecalculatePathNeeded(true)
{
	unitSize.x = 0;
	unitSize.y = 0;
	goalPos.x = -1;
	goalPos.y = -1;
	goalSize.x = 0;
	goalSize.y = 0;
}

const Vec2i &PathFinderInput::GetUnitPos() const { return unit->tilePos; }
const int PathFinderInput::GetUnitMapLayer() const { return unit->MapLayer->ID; }
Vec2i PathFinderInput::GetUnitSize() const
{
	return unit->Type->get_tile_size();
}

void PathFinderInput::SetUnit(CUnit &_unit)
{
	unit = &_unit;

	isRecalculatePathNeeded = true;
}

void PathFinderInput::SetGoal(const Vec2i &pos, const Vec2i &size, int z)
{
	Assert(CMap::get()->Info.IsPointOnMap(pos, z));
	Assert(unit);
	Assert(unit->IsAliveOnMap());
	Vec2i newPos = pos;
	// Large units may have a goal that goes outside the map, fix it here
	if (newPos.x + unit->Type->get_tile_width() - 1 >= CMap::get()->Info.MapWidths[z]) {
		newPos.x = CMap::get()->Info.MapWidths[z] - unit->Type->get_tile_width();
	}
	if (newPos.y + unit->Type->get_tile_height() - 1 >= CMap::get()->Info.MapHeights[z]) {
		newPos.y = CMap::get()->Info.MapHeights[z] - unit->Type->get_tile_height();
	}
	//Wyrmgus end
	//Wyrmgus start
//	if (goalPos != newPos || goalSize != size) {
	if (goalPos != newPos || goalSize != size || MapLayer != z) {
	//Wyrmgus end
		isRecalculatePathNeeded = true;
	}
	goalPos = newPos;
	goalSize = size;
	//Wyrmgus start
	MapLayer = z;
	//Wyrmgus end
}

void PathFinderInput::SetMinRange(int range)
{
	if (minRange != range) {
		minRange = range;
		isRecalculatePathNeeded = true;
	}
}

void PathFinderInput::SetMaxRange(int range)
{
	if (maxRange != range) {
		maxRange = range;
		isRecalculatePathNeeded = true;
	}
}

void PathFinderInput::PathRacalculated()
{
	unitSize = unit->Type->get_tile_size();

	isRecalculatePathNeeded = false;
}

/**
**  Find new path.
**
**  The destination could be a unit or a field.
**  Range gives how far we must reach the goal.
**
**  @note  The destination could become negative coordinates!
**
**  @param unit  Path for this unit.
**
**  @return      >0 remaining path length, 0 wait for path, -1
**               reached goal, -2 can't reach the goal.
*/
static int NewPath(PathFinderInput &input, PathFinderOutput &output)
{
	int i = AStarFindPath(input.GetUnitPos(),
						  input.GetGoalPos(),
						  input.GetGoalSize().x, input.GetGoalSize().y,
						  input.GetUnitSize().x, input.GetUnitSize().y,
						  input.GetMinRange(), input.GetMaxRange(),
						  &output.Path,
						  //Wyrmgus start
//						  *input.GetUnit());
						  *input.GetUnit(), 0, input.GetGoalMapLayer());
						  //Wyrmgus end
	input.PathRacalculated();
	if (i == PF_FAILED) {
		i = PF_UNREACHABLE;
	}

	// Update path if it was requested. Otherwise we may only want
	// to know if there exists a path.
	output.Length = std::min<int>(i, PathFinderOutput::MAX_PATH_LENGTH);
	if (output.Length == 0) {
		++output.Length;
	}

	return i;
}

/**
**  Returns the next element of a path.
**
**  @param unit  Unit that wants the path element.
**  @param pxd   Pointer for the x direction.
**  @param pyd   Pointer for the y direction.
**
**  @return >0 remaining path length, 0 wait for path, -1
**  reached goal, -2 can't reach the goal.
*/
int NextPathElement(CUnit &unit, int &pxd, int &pyd)
{
	PathFinderInput &input = unit.pathFinderData->input;
	PathFinderOutput &output = unit.pathFinderData->output;

	unit.CurrentOrder()->UpdatePathFinderData(input);
	// Attempt to use path cache
	// FIXME: If there is a goal, it may have moved, ruining the cache
	pxd = 0;
	pyd = 0;

	// Goal has moved, need to recalculate path or no cached path
	if (output.Length <= 0 || input.IsRecalculateNeeded()) {
		const int result = NewPath(input, output);

		if (result == PF_UNREACHABLE) {
			output.Length = 0;
			return result;
		}
		if (result == PF_REACHED) {
			return result;
		}
	}

	pxd = Heading2X[(int)output.Path[(int)output.Length - 1]];
	pyd = Heading2Y[(int)output.Path[(int)output.Length - 1]];
	const Vec2i dir(pxd, pyd);
	int result = output.Length;
	output.Length--;
	if (!UnitCanBeAt(unit, unit.tilePos + dir, unit.MapLayer->ID)) {
		// If obstructing unit is moving, wait for a bit.
		if (output.Fast) {
			output.Fast--;
			AstarDebugPrint("WAIT at %d\n" _C_ output.Fast);
			result = PF_WAIT;
		} else {
			output.Fast = 10;
			AstarDebugPrint("SET WAIT to 10\n");
			result = PF_WAIT;
		}
		if (output.Fast == 0 && result != 0) {
			AstarDebugPrint("WAIT expired\n");
			result = NewPath(input, output);
			if (result > 0) {
				pxd = Heading2X[(int)output.Path[(int)output.Length - 1]];
				pyd = Heading2Y[(int)output.Path[(int)output.Length - 1]];
				if (!UnitCanBeAt(unit, unit.tilePos + dir, unit.MapLayer->ID)) {
					// There may be unit in the way, Astar may allow you to walk onto it.
					result = PF_UNREACHABLE;
					pxd = 0;
					pyd = 0;
				} else {
					result = output.Length;
					output.Length--;
				}
			}
		}
	}
	if (result != PF_WAIT) {
		output.Fast = 0;
	}
	return result;
}
