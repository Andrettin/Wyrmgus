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
/**@name pathfinder.h - The path finder headerfile. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer, Russell Smith
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

#if defined(DEBUG_ASTAR)
#define AstarDebugPrint(x) DebugPrint(x)
#else
#define AstarDebugPrint(x)
#endif

#include "util/assert_util.h"
#include "vec2i.h"

class CUnit;
class CFile;
struct lua_State;

namespace wyrmgus {
	enum class tile_flag : uint32_t;
}

/**
**  Result codes of the pathfinder.
**
**  @todo
**    Another idea is SINT_MAX as reached, SINT_MIN as unreachable
**    stop others how far to goal.
*/
enum _move_return_ {
	PF_FAILED = -3,       /// This Pathfinder failed, try another
	PF_UNREACHABLE = -2,  /// Unreachable stop
	PF_REACHED = -1,      /// Reached goal stop
	PF_WAIT = 0,          /// Wait, no time or blocked
	PF_MOVE = 1           /// On the way moving
};

class PathFinderInput
{
public:
	PathFinderInput();
	CUnit *GetUnit() const { return unit; }
	const Vec2i &GetUnitPos() const;
	//Wyrmgus start
	const int GetUnitMapLayer() const;
	//Wyrmgus end
	Vec2i GetUnitSize() const;
	const Vec2i &GetGoalPos() const { return goalPos; }
	//Wyrmgus start
	const int GetGoalMapLayer() const { return MapLayer; }
	//Wyrmgus end
	const Vec2i &GetGoalSize() const { return goalSize; }
	int GetMinRange() const { return minRange; }
	int GetMaxRange() const { return maxRange; }
	bool IsRecalculateNeeded() const { return isRecalculatePathNeeded; }

	void SetUnit(CUnit &_unit);
	//Wyrmgus start
//	void SetGoal(const Vec2i &pos, const Vec2i &size);
	void SetGoal(const Vec2i &pos, const Vec2i &size, int z);
	//Wyrmgus end
	void SetMinRange(int range);
	void SetMaxRange(int range);

	void PathRacalculated();

	void Save(CFile &file) const;
	void Load(lua_State *l);

private:
	CUnit *unit;
	Vec2i unitSize;
	Vec2i goalPos;
	Vec2i goalSize;
	int minRange;
	int maxRange;
	//Wyrmgus start
	int MapLayer;
	//Wyrmgus end
	bool isRecalculatePathNeeded;
};

class PathFinderOutput final
{
public:
	enum {MAX_PATH_LENGTH = 28}; /// max length of precalculated path
public:
	void Save(CFile &file) const;
	void Load(lua_State *l);
public:
	unsigned short int Cycles = 0;  /// how much Cycles we move.
	char Fast = 0;                  /// Flag fast move (one step)
	char Length = 0;                /// stored path length
	std::array<char, MAX_PATH_LENGTH> Path{}; /// directions of stored path
};

class PathFinderData final
{
public:
	PathFinderInput input;
	PathFinderOutput output;
};

//  Terrain traversal stuff.

enum class VisitResult {
	Finished,
	DeadEnd,
	Ok,
	Cancel
};

class TerrainTraversal
{
public:
	using dataType = short int;

	void SetSize(unsigned int width, unsigned int height);
	void Init();

	void PushPos(const Vec2i &pos);
	void push_pos_if_passable(const QPoint &pos, const int z, const tile_flag passability_mask);
	void PushNeighbor(const Vec2i &pos);
	void push_pos_rect(const QRect &rect);
	void push_pos_rect_if_passable(const QRect &rect, const int z, const tile_flag passability_mask);
	void push_pos_rect_borders(const QRect &rect);
	void push_pos_rect_borders_if_passable(const QRect &rect, const int z, const tile_flag passability_mask);
	void PushUnitPosAndNeighbor(const CUnit &unit);
	void push_unit_pos_and_neighbor_if_passable(const CUnit &unit, const tile_flag passability_mask);

	template <typename T>
	bool Run(T &context);

	bool IsVisited(const Vec2i &pos) const;
	bool IsReached(const Vec2i &pos) const;
	bool IsInvalid(const Vec2i &pos) const;

	// Accept pos to be at one inside the real map
	dataType Get(const Vec2i &pos) const;

private:
	void Set(const Vec2i &pos, dataType value);

	struct PosNode {
		PosNode(const Vec2i &pos, const Vec2i &from) : pos(pos), from(from) {}
		Vec2i pos;
		Vec2i from;
	};

private:
	std::vector<dataType> m_values;
	std::queue<PosNode> m_queue;
	unsigned int m_extented_width;
	unsigned int m_height;
};

template <typename T>
bool TerrainTraversal::Run(T &context)
{
	for (; m_queue.empty() == false; m_queue.pop()) {
		const PosNode &posNode = m_queue.front();

		switch (context.Visit(*this, posNode.pos, posNode.from)) {
			case VisitResult::Finished: return true;
			case VisitResult::DeadEnd: Set(posNode.pos, -1); break;
			case VisitResult::Ok: PushNeighbor(posNode.pos); break;
			case VisitResult::Cancel: return false;
		}
		assert_throw(IsVisited(posNode.pos));
	}
	return false;
}

/// cost associated to move on a tile occupied by a fixed unit
extern int AStarFixedUnitCrossingCost;
/// cost associated to move on a tile occupied by a moving unit
extern int AStarMovingUnitCrossingCost;
/// Whether to have knowledge of terrain that we haven't visited yet
extern bool AStarKnowUnseenTerrain;
/// Cost of using a square we haven't seen before.
extern int AStarUnknownTerrainCost;

//
//  Convert heading into direction.
//  N NE  E SE  S SW  W NW
constexpr std::array<int, 9> Heading2X = { 0, +1, +1, +1, 0, -1, -1, -1, 0 };
constexpr std::array<int, 9> Heading2Y = { -1, -1, 0, +1, +1, +1, 0, -1, 0 };

/// Init the pathfinder
extern void InitPathfinder();
/// Free the pathfinder
extern void FreePathfinder();

/// Returns the next element of the path
extern int NextPathElement(CUnit &unit, int &xdp, int &ydp);
/// Return distance to unit.
//Wyrmgus start
//extern int UnitReachable(const CUnit &unit, const CUnit &dst, int range);
extern int UnitReachable(const CUnit &unit, const CUnit &dst, int range, int max_length = 0, bool from_outside_container = false);
//Wyrmgus end
/// Can the unit 'src' reach the place x,y
extern int PlaceReachable(const CUnit &src, const Vec2i &pos, int w, int h,
						  //Wyrmgus start
//						  int minrange, int maxrange);
						  int minrange, int maxrange, int max_length, int z, bool from_outside_container = false);
						  //Wyrmgus end

// in astar.cpp

extern void SetAStarFixedUnitCrossingCost(int cost);
extern int GetAStarFixedUnitCrossingCost();

extern void SetAStarMovingUnitCrossingCost(int cost);
extern int GetAStarMovingUnitCrossingCost();

extern void SetAStarUnknownTerrainCost(int cost);
extern int GetAStarUnknownTerrainCost();

//Wyrmgus start
/// Find and a* path for a unit
extern int AStarFindPath(const Vec2i &startPos, const Vec2i &goalPos, int gw, int gh,
						 int tilesizex, int tilesizey, int minrange,
						 //Wyrmgus start
//						 int maxrange, std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, const CUnit &unit);
						 int maxrange, std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, const CUnit &unit, int max_length, int z);
						 //Wyrmgus end
//Wyrmgus end

extern void PathfinderCclRegister();
