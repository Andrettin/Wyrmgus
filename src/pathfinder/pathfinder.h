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
/**@name pathfinder.h - The path finder header file. */
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
//

#ifndef __PATH_FINDER_H__
#define __PATH_FINDER_H__

#if defined(DEBUG_ASTAR)
#define AstarDebugPrint(x) DebugPrint(x)
#else
#define AstarDebugPrint(x)
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#include <core/math/vector2.h>

#include <queue>

class CUnit;
class CFile;
struct lua_State;

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
	CUnit *GetUnit() const { return unit; }
	const Vector2i &GetUnitPos() const;
	//Wyrmgus start
	const int GetUnitMapLayer() const;
	//Wyrmgus end
	Vector2i GetUnitSize() const;
	const Vector2i &GetGoalPos() const { return goalPos; }
	//Wyrmgus start
	const int GetGoalMapLayer() const { return MapLayer; }
	//Wyrmgus end
	const Vector2i &GetGoalSize() const { return goalSize; }
	int GetMinRange() const { return minRange; }
	int GetMaxRange() const { return maxRange; }
	bool IsRecalculateNeeded() const { return isRecalculatePathNeeded; }

	void SetUnit(CUnit &_unit);
	//Wyrmgus start
//	void SetGoal(const Vector2i &pos, const Vector2i &size);
	void SetGoal(const Vector2i &pos, const Vector2i &size, int z);
	//Wyrmgus end
	void SetMinRange(int range);
	void SetMaxRange(int range);

	void PathRacalculated();

	void Save(CFile &file) const;
	void Load(lua_State *l);

private:
	CUnit *unit = nullptr;
	Vector2i unitSize = Vector2i(0, 0);
	Vector2i goalPos = Vector2i(-1, -1);
	Vector2i goalSize = Vector2i(0, 0);
	int minRange = 0;
	int maxRange = 0;
	//Wyrmgus start
	int MapLayer = 0;
	//Wyrmgus end
	bool isRecalculatePathNeeded = true;
};

class PathFinderOutput
{
public:
	enum {MAX_PATH_LENGTH = 28}; /// max length of precalculated path
public:
	PathFinderOutput();
	void Save(CFile &file) const;
	void Load(lua_State *l);
public:
	unsigned short int Cycles;  /// how much Cycles we move.
	char Fast;                  /// Flag fast move (one step)
	char Length;                /// stored path length
	char Path[MAX_PATH_LENGTH]; /// directions of stored path
};

class PathFinderData
{
public:
	PathFinderInput input;
	PathFinderOutput output;
};


//
//  Terrain traversal stuff.
//

enum VisitResult {
	VisitResult_Finished,
	VisitResult_DeadEnd,
	VisitResult_Ok,
	VisitResult_Cancel
};

class TerrainTraversal
{
public:
	typedef short int dataType;
public:
	void SetSize(unsigned int width, unsigned int height);
	void SetDiagonalAllowed(const bool allowed);
	void Init();

	void PushPos(const Vector2i &pos);
	void PushNeighbor(const Vector2i &pos);
	void PushUnitPosAndNeighbor(const CUnit &unit);

	template <typename T>
	bool Run(T &context);

	bool IsVisited(const Vector2i &pos) const;
	bool IsReached(const Vector2i &pos) const;
	bool IsInvalid(const Vector2i &pos) const;

	// Accept pos to be at one inside the real map
	dataType Get(const Vector2i &pos) const;

private:
	void Set(const Vector2i &pos, dataType value);

	struct PosNode {
		PosNode(const Vector2i &pos, const Vector2i &from) : pos(pos), from(from) {}
		Vector2i pos;
		Vector2i from;
	};

private:
	std::vector<dataType> m_values;
	std::queue<PosNode> m_queue;
	unsigned int m_extented_width;
	unsigned int m_height;
	bool allow_diagonal = true;
};

template <typename T>
bool TerrainTraversal::Run(T &context)
{
	for (; m_queue.empty() == false; m_queue.pop()) {
		const PosNode &posNode = m_queue.front();

		switch (context.Visit(*this, posNode.pos, posNode.from)) {
			case VisitResult_Finished: return true;
			case VisitResult_DeadEnd: Set(posNode.pos, -1); break;
			case VisitResult_Ok: PushNeighbor(posNode.pos); break;
			case VisitResult_Cancel: return false;
		}
		Assert(IsVisited(posNode.pos));
	}
	return false;
}


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

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
extern const int Heading2X[9];
extern const int Heading2Y[9];
extern const int XY2Heading[3][3];

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Init the pathfinder
extern void InitPathfinder();
/// Free the pathfinder
extern void FreePathfinder();

/// Returns the next element of the path
extern int NextPathElement(CUnit &unit, int *xdp, int *ydp);
/// Return distance to unit.
//Wyrmgus start
//extern int UnitReachable(const CUnit &unit, const CUnit &dst, int range);
extern int UnitReachable(const CUnit &unit, const CUnit &dst, int range, int max_length = 0, bool from_outside_container = false);
//Wyrmgus end
/// Can the unit 'src' reach the place x,y
extern int PlaceReachable(const CUnit &src, const Vector2i &pos, int w, int h,
						  //Wyrmgus start
//						  int minrange, int maxrange);
						  int minrange, int maxrange, int max_length, int z, bool from_outside_container = false);
						  //Wyrmgus end

//
// in astar.cpp
//

extern void SetAStarFixedUnitCrossingCost(int cost);
extern int GetAStarFixedUnitCrossingCost();

extern void SetAStarMovingUnitCrossingCost(int cost);
extern int GetAStarMovingUnitCrossingCost();

extern void SetAStarUnknownTerrainCost(int cost);
extern int GetAStarUnknownTerrainCost();

//Wyrmgus start
/// Find and a* path for a unit
extern int AStarFindPath(const Vector2i &startPos, const Vector2i &goalPos, int gw, int gh,
						 int tilesizex, int tilesizey, int minrange,
						 //Wyrmgus start
//						 int maxrange, char *path, int pathlen, const CUnit &unit);
						 int maxrange, char *path, int pathlen, const CUnit &unit, int max_length, int z, bool allow_diagonal = true);
						 //Wyrmgus end
//Wyrmgus end

extern void PathfinderCclRegister();

#endif
