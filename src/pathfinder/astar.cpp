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
//      (c) Copyright 1999-2022 by Lutz Sammer, Fabrice Rossi, Russell Smith,
//                                 Francois Beerten, Jimmy Salmon and Andrettin
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

#include "pathfinder/pathfinder.h"

#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "settings.h"
#include "time/time_of_day.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_domain_blocker_finder.h"
#include "unit/unit_find.h"
#include "util/assert_util.h"
#include "util/enum_util.h"
#include "util/log_util.h"
#include "util/number_util.h"
#include "util/point_util.h"
#include "util/util.h"
#include "util/vector_util.h"

struct Node {
	int CostFromStart = 0;  /// Real costs to reach this point
	short int CostToGoal = 0;     /// Estimated cost to goal
	char InGoal = 0;        /// is this point in the goal
	char Direction = 0;     /// Direction for trace back
};

//Wyrmgus start
//std::array<int, 9> Heading2O;//heading to offset
std::array<std::vector<int>, 9> Heading2O;//heading to offset
//Wyrmgus end
static constexpr std::array<std::array<int, 3>, 3> XY2Heading = { { {7, 6, 5}, {0, 0, 4}, {1, 2, 3} } };

/// cost matrix
//Wyrmgus start
//static std::vector<Node> AStarMatrix;
static std::vector<std::vector<Node>> AStarMatrix;
//Wyrmgus end

/// a list of close nodes, helps to speed up the matrix cleaning
//Wyrmgus start
//static std::vector<int> CloseSet;
//static size_t Threshold = 0;
static std::vector<std::vector<int>> CloseSet;
static std::vector<size_t> Threshold;
//Wyrmgus end
static constexpr int MAX_CLOSE_SET_RATIO = 4;
static constexpr int MAX_OPEN_SET_RATIO = 8; // 10,16 to small

/// see pathfinder.h
int AStarFixedUnitCrossingCost;// = MaxMapWidth * MaxMapHeight;
int AStarMovingUnitCrossingCost = 5;
bool AStarKnowUnseenTerrain = false;
int AStarUnknownTerrainCost = 2;

//Wyrmgus start
//static int AStarMapWidth;
//static int AStarMapHeight;
static std::vector<int> AStarMapWidth;
static std::vector<int> AStarMapHeight;
//Wyrmgus end

static int AStarGoalX;
static int AStarGoalY;

static int AStarGoalZ;

struct Open final
{
	explicit Open(const Vec2i &pos, const short int costs, const unsigned int offset)
		: pos(pos), costs(costs), offset(offset)
	{
		this->distance = number::fast_abs(this->pos.x - AStarGoalX) + number::fast_abs(this->pos.y - AStarGoalY);
	}

	bool operator <(const Open &rhs) const
	{
		if (this->costs != rhs.costs) {
			return this->costs < rhs.costs;
		}

		const int cost_to_goal = AStarMatrix[AStarGoalZ][this->offset].CostToGoal;
		const int rhs_cost_to_goal = AStarMatrix[AStarGoalZ][rhs.offset].CostToGoal;

		if (cost_to_goal != rhs_cost_to_goal) {
			return cost_to_goal < rhs_cost_to_goal;
		}

		if (this->distance != rhs.distance) {
			return this->distance < rhs.distance;
		}

		return this->offset < rhs.offset;
	}

	const Vec2i pos = Vec2i(0, 0);
	const short int costs = 0; //complete costs to goal
	const unsigned int offset = 0; //offset into matrix
private:
	int distance = 0;
};

/// heuristic cost function for a*
static int AStarCosts(const Vec2i &pos, const Vec2i &goalPos)
{
	const Vec2i diff = pos - goalPos;
	return std::max<int>(number::fast_abs(diff.x), number::fast_abs(diff.y));
}

/**
**  The Open set is handled by a stored array
**  the end of the array holds the item with the smallest cost.
*/

/// The set of Open nodes
static std::vector<std::set<Open>> OpenSet;

//Wyrmgus start
//static std::vector<int> CostMoveToCache;
static std::vector<std::map<int, int>> CostMoveToCache;
//Wyrmgus end
static constexpr int CacheNotSet = -5;

/**
**  Init A* data structures
*/
//Wyrmgus start
//void InitAStar(int mapWidth, int mapHeight)
void InitAStar()
//Wyrmgus end
{
	for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
		// Should only be called once
		assert_throw(AStarMatrix.size() <= z);
	
		AStarMapWidth.push_back(CMap::get()->Info->MapWidths[z]);
		AStarMapHeight.push_back(CMap::get()->Info->MapHeights[z]);
		
		AStarMatrix.push_back(std::vector<Node>(AStarMapWidth[z] * AStarMapHeight[z]));

		const size_t threshold = static_cast<size_t>(AStarMapWidth[z] * AStarMapHeight[z] / MAX_CLOSE_SET_RATIO);
		Threshold.push_back(threshold);
		CloseSet.emplace_back();
		CloseSet.back().reserve(threshold);

		const size_t open_set_max_size = AStarMapWidth[z] * AStarMapHeight[z] / MAX_OPEN_SET_RATIO;
		OpenSet.emplace_back();

		CostMoveToCache.emplace_back();

		for (int i = 0; i < 9; ++i) {
			Heading2O[i].push_back(Heading2Y[i] * AStarMapWidth[z]);
		}
	}
}

/**
**  Free A* data structure
*/
void FreeAStar()
{
	AStarMapWidth.clear();
	AStarMapHeight.clear();
	AStarMatrix.clear();
	Threshold.clear();
	CloseSet.clear();
	OpenSet.clear();
	CostMoveToCache.clear();
	
	for (int i = 0; i < 9; ++i) {
		Heading2O[i].clear();
	}
}

/**
**  Prepare pathfinder.
*/
//Wyrmgus start
//static void AStarPrepare()
static void AStarPrepare(int z)
//Wyrmgus end
{
	//Wyrmgus start
//	std::fill(AStarMatrix.begin(), AStarMatrix.end(), Node());
	std::fill(AStarMatrix[z].begin(), AStarMatrix[z].end(), Node());
	//Wyrmgus end
}

/**
**  Clean up A*
*/
//Wyrmgus start
//static void AStarCleanUp()
static void AStarCleanUp(int z)
//Wyrmgus end
{
	//Wyrmgus start
//	if (CloseSet.size() >= Threshold) {
	if (CloseSet[z].size() >= Threshold[z]) {
	//Wyrmgus end
		//Wyrmgus start
//		AStarPrepare();
		AStarPrepare(z);
		//Wyrmgus end
	} else {
		for (const int close : CloseSet[z]) {
			AStarMatrix[z][close].CostFromStart = 0;
			AStarMatrix[z][close].InGoal = 0;
		}
	}
}

//Wyrmgus start
//static void CostMoveToCacheCleanUp()
static void CostMoveToCacheCleanUp(const int z)
//Wyrmgus end
{
	CostMoveToCache[z].clear();
}

static int get_cost_move_to_cache(const int index, const int z)
{
	const std::map<int, int> &cache = CostMoveToCache[z];

	const auto find_iterator = cache.find(index);
	if (find_iterator != cache.end()) {
		return find_iterator->second;
	}

	return CacheNotSet;
}

/**
**  Add a new node to the open set (and update the heap structure)
**
**  @return  0 or PF_FAILED
*/
//Wyrmgus start
//static int AStarAddNode(const Vec2i &pos, const int o, const int costs)
static int AStarAddNode(const Vec2i &pos, const int o, const int costs, const int z)
//Wyrmgus end
{
	// fill our new node
	OpenSet[z].emplace(pos, costs, o);

	return 0;
}

/**
**  Change the cost associated to an open node.
**  Can be further optimised knowing that the new cost MUST BE LOWER
**  than the old one.
*/
static void AStarReplaceNode(const Open *node_ptr, const int z)
{
	Open node = std::move(OpenSet[z].extract(*node_ptr).value());

	// Re-add the node with the new cost
	AStarAddNode(node.pos, node.offset, node.costs, z);
}

/**
**  Check if a node is already in the open set.
**
**  @return  The pointer to the node if found, or null otherwise.
*/
static const Open *AStarFindNode(const int eo, const int z)
{
	for (const Open &open_node : OpenSet[z]) {
		if (open_node.offset == eo) {
			return &open_node;
		}
	}

	return nullptr;
}

/**
**  Add a node to the closed set
*/
//Wyrmgus start
//static void AStarAddToClose(int node)
static void AStarAddToClose(int node, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	if (CloseSet.size() < Threshold) {
	if (CloseSet[z].size() < Threshold[z]) {
	//Wyrmgus end
		//Wyrmgus start
//		CloseSet.push_back(node);
		CloseSet[z].push_back(node);
		//Wyrmgus end
	}
}

//Wyrmgus start
//#define GetIndex(x, y) (x) + (y) * AStarMapWidth
#define GetIndex(x, y, z) (x) + (y) * AStarMapWidth[(z)]
//Wyrmgus end

/* build-in costmoveto code */
static int CostMoveToCallBack_Default(unsigned int index, const CUnit &unit, int z)
{
#ifdef DEBUG
	{
		Vec2i pos;
		pos.y = index / CMap::get()->Info->MapWidths[z];
		pos.x = index - pos.y * CMap::get()->Info->MapWidths[z];
		assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));
	}
#endif
	int cost = 0;
	const tile_flag mask = unit.Type->MovementMask;
	const unit_domain_blocker_finder unit_finder(unit.Type->get_domain());

	// verify each tile of the unit.
	int h = unit.Type->get_tile_height();
	const int w = unit.Type->get_tile_width();
	do {
		const wyrmgus::tile *mf = CMap::get()->Field(index, z);
		int i = w;
		do {
			const tile_flag tile_flags = mf->get_flags();
			const tile_flag flags = tile_flags & mask;
			
			if (flags != tile_flag::none && (AStarKnowUnseenTerrain || mf->player_info->IsTeamExplored(*unit.Player))) {
				if ((flags & ~(tile_flag::land_unit | tile_flag::air_unit | tile_flag::sea_unit)) != tile_flag::none) {
					//we can't cross fixed units and other unpassable things
					return -1;
				}

				CUnit *goal = mf->UnitCache.find(unit_finder);
				if (!goal) {
					//shouldn't happen, mask says there is something on this tile
					throw std::runtime_error("Error in CostMoveToCallBack_Default: tile " + point::to_string(CMap::get()->get_index_pos(index, z)) + " says there is something impassable in its mask (" + std::to_string(enumeration::to_underlying(flags)) + "), but no appropriate unit could be found on the tile for domain \"" + unit_domain_to_string(unit.Type->get_domain()) + "\" (unit cache size: " + std::to_string(mf->UnitCache.size()) + ").");
				}

				//Wyrmgus start
//				if (goal->Moving)  {
				if (&unit != goal && goal->Moving)  {
				//Wyrmgus end
					// moving unit are crossable
					cost += AStarMovingUnitCrossingCost;
				} else {
					// for non moving unit Always Fail unless goal is unit, or unit can attack the target
					if (&unit != goal) {
						//Wyrmgus start
						/*
//						if (goal->Player->is_enemy_of(unit) && unit.IsAgressive() && unit.Type->can_target(goal)
						//Wyrmgus end
							&& goal->Variable[UNHOLYARMOR_INDEX].Value == 0 && goal->IsVisibleAsGoal(*unit.Player)) {
								cost += 2 * AStarMovingUnitCrossingCost;
						} else {
						*/
						// FIXME: Need support for moving a fixed unit to add cost
							return -1;
						//Wyrmgus start
//						}
						//Wyrmgus end
						//cost += AStarFixedUnitCrossingCost;
					}
				}
			}

			// Add cost of crossing unknown tiles if required
			if (!AStarKnowUnseenTerrain && !mf->player_info->IsTeamExplored(*unit.Player)) {
				// Tend against unknown tiles.
				cost += AStarUnknownTerrainCost;
			}
			
			//Wyrmgus start
			if (
				(mf->has_flag(tile_flag::desert))
				&& mf->get_owner() != unit.Player
				&& unit.Type->BoolFlag[ORGANIC_INDEX].value
				&& unit.get_center_tile_time_of_day() != nullptr
				&& unit.get_center_tile_time_of_day()->is_day()
				&& unit.Variable[DEHYDRATIONIMMUNITY_INDEX].Value <= 0
			) {
				cost += 32; //increase the cost of moving through deserts for units affected by dehydration, as we want the pathfinding to try to avoid that
			}
			//Wyrmgus end
			
			//add tile movement cost
			switch (unit.Type->get_domain()) {
				case unit_domain::air:
				case unit_domain::air_low:
				case unit_domain::space:
					cost += DefaultTileMovementCost;
					break;
				default:
					cost += mf->get_movement_cost();

					if (unit.Variable[RAIL_SPEED_BONUS_INDEX].Value != 0 && !mf->has_flag(tile_flag::railroad)) {
						//add rail speed bonus to the cost for non-railroad tiles, as it is an implicit penalty for them
						cost += unit.Variable[RAIL_SPEED_BONUS_INDEX].Value;
					}
					break;
			}

			++mf;
		} while (--i);

		//Wyrmgus start
//		index += AStarMapWidth;
		index += AStarMapWidth[z];
		//Wyrmgus end
	} while (--h);

	return cost;
}


/**
**  Compute the cost of crossing tile (x,y)
**
**  @param x     X tile where to move.
**  @param y     Y tile where to move.
**  @param data  user data.
**
**  @return      -1 -> impossible to cross.
**                0 -> no induced cost, except move
**               >0 -> costly tile
*/
static inline int CostMoveTo(unsigned int index, const CUnit &unit, int z)
{
	//Wyrmgus start
	if (!&unit) {
		log::log_error("Error in CostMoveTo(unsigned int index, const CUnit &unit): Unit is null.");
		return -1;
	}
	//Wyrmgus end

	int c = get_cost_move_to_cache(index, z);

	if (c != CacheNotSet) {
		return c;
	}

	c = CostMoveToCallBack_Default(index, unit, z);
	CostMoveToCache[z][index] = c;

	return c;
}

class AStarGoalMarker final
{
public:
	explicit AStarGoalMarker(const CUnit &unit, bool &goal_reachable)
		: unit(unit), goal_reachable(goal_reachable)
	{
	}

	void operator()(int offset, int z) const
	{
		if (CostMoveTo(offset, unit, z) >= 0) {
			AStarMatrix[z][offset].InGoal = 1;
			goal_reachable = true;
		}
		//Wyrmgus start
//		AStarAddToClose(offset);
		AStarAddToClose(offset, z);
		//Wyrmgus end
	}
private:
	const CUnit &unit;
	bool &goal_reachable;
};


template <typename T>
class MinMaxRangeVisitor
{
public:
	explicit MinMaxRangeVisitor(const T &func) : func(func), minrange(0), maxrange(0) {}

	//Wyrmgus start
//	void SetGoal(Vec2i goalTopLeft, Vec2i goalBottomRight)
	void SetGoal(Vec2i goalTopLeft, Vec2i goalBottomRight, int z)
	//Wyrmgus end
	{
		this->goalTopLeft = goalTopLeft;
		this->goalBottomRight = goalBottomRight;
		//Wyrmgus start
		this->z = z;
		//Wyrmgus end
	}

	void SetRange(int minrange, int maxrange)
	{
		this->minrange = minrange;
		this->maxrange = maxrange;
	}

	void SetUnitSize(const Vec2i &tileSize)
	{
		this->unitExtraTileSize.x = tileSize.x - 1;
		this->unitExtraTileSize.y = tileSize.y - 1;
	}

	void Visit() const
	{
		TopHemicycle();
		TopHemicycleNoMinRange();
		Center();
		BottomHemicycleNoMinRange();
		BottomHemicycle();
	}

private:
	int GetMaxOffsetX(int dy, int range) const
	{
		return isqrt(square(range + 1) - square(dy) - 1);
	}

	// Distance are computed between bottom of unit and top of goal
	void TopHemicycle() const
	{
		const int miny = std::max(0, goalTopLeft.y - maxrange - unitExtraTileSize.y);
		const int maxy = std::min(goalTopLeft.y - minrange - unitExtraTileSize.y, goalTopLeft.y - 1 - unitExtraTileSize.y);
		for (int y = miny; y <= maxy; ++y) {
			const int offsetx = GetMaxOffsetX(y - goalTopLeft.y, maxrange);
			const int minx = std::max(0, goalTopLeft.x - offsetx - unitExtraTileSize.x);
			//Wyrmgus start
//			const int maxx = std::min(CMap::get()->Info->MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			const int maxx = std::min(CMap::get()->Info->MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			//Wyrmgus end
			Vec2i mpos(minx, y);
			//Wyrmgus start
//			const unsigned int offset = mpos.y * CMap::get()->Info->MapWidth;
			const unsigned int offset = mpos.y * CMap::get()->Info->MapWidths[z];
			//Wyrmgus end

			for (mpos.x = minx; mpos.x <= maxx; ++mpos.x) {
				//Wyrmgus start
//				func(offset + mpos.x);
				func(offset + mpos.x, z);
				//Wyrmgus end
			}
		}
	}

	void HemiCycleRing(int y, int offsetminx, int offsetmaxx) const
	{
		const int minx = std::max(0, goalTopLeft.x - offsetmaxx - unitExtraTileSize.x);
		//Wyrmgus start
//		const int maxx = std::min(CMap::get()->Info->MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetmaxx);
		const int maxx = std::min(CMap::get()->Info->MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetmaxx);
		//Wyrmgus end
		Vec2i mpos(minx, y);
		//Wyrmgus start
//		const unsigned int offset = mpos.y * CMap::get()->Info->MapWidth;
		const unsigned int offset = mpos.y * CMap::get()->Info->MapWidths[z];
		//Wyrmgus end

		for (mpos.x = minx; mpos.x <= goalTopLeft.x - offsetminx - unitExtraTileSize.x; ++mpos.x) {
			//Wyrmgus start
//			func(offset + mpos.x);
			func(offset + mpos.x, z);
			//Wyrmgus end
		}
		for (mpos.x = goalBottomRight.x + offsetminx; mpos.x <= maxx; ++mpos.x) {
			//Wyrmgus start
//			func(offset + mpos.x);
			func(offset + mpos.x, z);
			//Wyrmgus end
		}
	}

	void TopHemicycleNoMinRange() const
	{
		const int miny = std::max(0, goalTopLeft.y - (minrange - 1) - unitExtraTileSize.y);
		const int maxy = goalTopLeft.y - 1 - unitExtraTileSize.y;
		for (int y = miny; y <= maxy; ++y) {
			const int offsetmaxx = GetMaxOffsetX(y - goalTopLeft.y, maxrange);
			const int offsetminx = GetMaxOffsetX(y - goalTopLeft.y, minrange - 1) + 1;

			HemiCycleRing(y, offsetminx, offsetmaxx);
		}
	}

	void Center() const
	{
		const int miny = std::max(0, goalTopLeft.y - unitExtraTileSize.y);
		//Wyrmgus start
//		const int maxy = std::min<int>(CMap::get()->Info->MapHeight - 1 - unitExtraTileSize.y, goalBottomRight.y);
		const int maxy = std::min<int>(CMap::get()->Info->MapHeights[z] - 1 - unitExtraTileSize.y, goalBottomRight.y);
		//Wyrmgus end
		const int minx = std::max(0, goalTopLeft.x - maxrange - unitExtraTileSize.x);
		//Wyrmgus start
//		const int maxx = std::min<int>(CMap::get()->Info->MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + maxrange);
		const int maxx = std::min<int>(CMap::get()->Info->MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + maxrange);
		//Wyrmgus end

		if (minrange == 0) {
			for (int y = miny; y <= maxy; ++y) {
				Vec2i mpos(minx, y);
				//Wyrmgus start
//				const unsigned int offset = mpos.y * CMap::get()->Info->MapWidth;
				const unsigned int offset = mpos.y * CMap::get()->Info->MapWidths[z];
				//Wyrmgus end

				for (mpos.x = minx; mpos.x <= maxx; ++mpos.x) {
					//Wyrmgus start
//					func(offset + mpos.x);
					func(offset + mpos.x, z);
					//Wyrmgus end
				}
			}
		} else {
			for (int y = miny; y <= maxy; ++y) {
				Vec2i mpos(minx, y);
				//Wyrmgus start
//				const unsigned int offset = mpos.y * CMap::get()->Info->MapWidth;
				const unsigned int offset = mpos.y * CMap::get()->Info->MapWidths[z];
				//Wyrmgus end

				for (mpos.x = minx; mpos.x <= goalTopLeft.x - minrange - unitExtraTileSize.x; ++mpos.x) {
					//Wyrmgus start
//					func(offset + mpos.x);
					func(offset + mpos.x, z);
					//Wyrmgus end
				}
				for (mpos.x = goalBottomRight.x + minrange; mpos.x <= maxx; ++mpos.x) {
					//Wyrmgus start
//					func(offset + mpos.x);
					func(offset + mpos.x, z);
					//Wyrmgus end
				}
			}
		}
	}

	void BottomHemicycleNoMinRange() const
	{
		const int miny = goalBottomRight.y + 1;
		//Wyrmgus start
//		const int maxy = std::min(CMap::get()->Info->MapHeight - 1 - unitExtraTileSize.y, goalBottomRight.y + (minrange - 1));
		const int maxy = std::min(CMap::get()->Info->MapHeights[z] - 1 - unitExtraTileSize.y, goalBottomRight.y + (minrange - 1));
		//Wyrmgus end

		for (int y = miny; y <= maxy; ++y) {
			const int offsetmaxx = GetMaxOffsetX(y - goalBottomRight.y, maxrange);
			const int offsetminx = GetMaxOffsetX(y - goalBottomRight.y, minrange - 1) + 1;

			HemiCycleRing(y, offsetminx, offsetmaxx);
		}
	}

	void BottomHemicycle() const
	{
		const int miny = std::max(goalBottomRight.y + minrange, goalBottomRight.y + 1);
		//Wyrmgus start
//		const int maxy = std::min(CMap::get()->Info->MapHeight - 1 - unitExtraTileSize.y, goalBottomRight.y + maxrange);
		const int maxy = std::min(CMap::get()->Info->MapHeights[z] - 1 - unitExtraTileSize.y, goalBottomRight.y + maxrange);
		//Wyrmgus end
		for (int y = miny; y <= maxy; ++y) {
			const int offsetx = GetMaxOffsetX(y - goalBottomRight.y, maxrange);
			const int minx = std::max(0, goalTopLeft.x - offsetx - unitExtraTileSize.x);
			//Wyrmgus start
//			const int maxx = std::min(CMap::get()->Info->MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			const int maxx = std::min(CMap::get()->Info->MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			//WYrmgus end
			Vec2i mpos(minx, y);
			//Wyrmgus start
//			const unsigned int offset = mpos.y * CMap::get()->Info->MapWidth;
			const unsigned int offset = mpos.y * CMap::get()->Info->MapWidths[z];
			//Wyrmgus end

			for (mpos.x = minx; mpos.x <= maxx; ++mpos.x) {
				//Wyrmgus start
//				func(offset + mpos.x);
				func(offset + mpos.x, z);
				//Wyrmgus end
			}
		}
	}

private:
	T func;
	Vec2i goalTopLeft;
	Vec2i goalBottomRight;
	Vec2i unitExtraTileSize;
	int minrange;
	int maxrange;
	//Wyrmgus start
	int z;
	//Wyrmgus end
};



/**
**  MarkAStarGoal
*/
static int AStarMarkGoal(const Vec2i &goal, int gw, int gh,
						 //Wyrmgus start
//						 int tilesizex, int tilesizey, int minrange, int maxrange, const CUnit &unit)
						 int tilesizex, int tilesizey, int minrange, int maxrange, const CUnit &unit, int z)
						 //Wyrmgus end
{
	if (minrange == 0 && maxrange == 0 && gw == 0 && gh == 0) {
		//Wyrmgus start
//		if (goal.x + tilesizex > AStarMapWidth || goal.y + tilesizey > AStarMapHeight) {
		if (goal.x + tilesizex - 1 > AStarMapWidth[z] || goal.y + tilesizey - 1 > AStarMapHeight[z]) {
		//Wyrmgus end
			return 0;
		}

		unsigned int offset = GetIndex(goal.x, goal.y, z);
		if (CostMoveTo(offset, unit, z) >= 0) {
			AStarMatrix[z][offset].InGoal = 1;
			return 1;
		} else {
			return 0;
		}
	}

	bool goal_reachable = false;

	gw = std::max(gw, 1);
	gh = std::max(gh, 1);

	AStarGoalMarker aStarGoalMarker(unit, goal_reachable);
	MinMaxRangeVisitor<AStarGoalMarker> visitor(aStarGoalMarker);

	const Vec2i goalBottomRigth(goal.x + gw - 1, goal.y + gh - 1);
	//Wyrmgus start
//	visitor.SetGoal(goal, goalBottomRigth);
	visitor.SetGoal(goal, goalBottomRigth, z);
	//Wyrmgus end
	visitor.SetRange(minrange, maxrange);
	const Vec2i tileSize(tilesizex, tilesizey);
	visitor.SetUnitSize(tileSize);

	const Vec2i extratilesize(tilesizex - 1, tilesizey - 1);

	visitor.Visit();

	return goal_reachable;
}

/**
**  Save the path
**
**  @return  The length of the path
*/
//Wyrmgus start
//static int AStarSavePath(const Vec2i &startPos, const Vec2i &endPos, std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path)
static int AStarSavePath(const Vec2i &startPos, const Vec2i &endPos, std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, int z)
//Wyrmgus end
{
	int fullPathLength;
	int pathPos;
	int direction;

	// Figure out the full path length
	fullPathLength = 0;
	Vec2i curr = endPos;
	//Wyrmgus start
//	int currO = curr.y * AStarMapWidth;
	int currO = curr.y * AStarMapWidth[z];
	//Wyrmgus end
	while (curr != startPos) {
		//Wyrmgus start
//		direction = AStarMatrix[currO + curr.x].Direction;
		direction = AStarMatrix[z][currO + curr.x].Direction;
		//Wyrmgus end
		curr.x -= Heading2X[direction];
		curr.y -= Heading2Y[direction];
		//Wyrmgus start
//		currO -= Heading2O[direction];
		currO -= Heading2O[direction][z];
		//Wyrmgus end
		fullPathLength++;
	}

	// Save as much of the path as we can
	if (path != nullptr) {
		const int path_len = std::min<int>(fullPathLength, path->size());
		pathPos = fullPathLength;
		curr = endPos;
		//Wyrmgus start
//		currO = curr.y * AStarMapWidth;
		currO = curr.y * AStarMapWidth[z];
		//Wyrmgus end
		while (curr != startPos) {
			//Wyrmgus start
//			direction = AStarMatrix[currO + curr.x].Direction;
			direction = AStarMatrix[z][currO + curr.x].Direction;
			//Wyrmgus end
			curr.x -= Heading2X[direction];
			curr.y -= Heading2Y[direction];
			//Wyrmgus start
//			currO -= Heading2O[direction];
			currO -= Heading2O[direction][z];
			//Wyrmgus end
			--pathPos;
			if (pathPos < path_len) {
				(*path)[path_len - pathPos - 1] = direction;
			}
		}
	}

	return fullPathLength;
}

/**
**  Optimization to find a simple path
**  Check if we're at the goal or if it's 1 tile away
*/
static int AStarFindSimplePath(const Vec2i &startPos, const Vec2i &goal, int gw, int gh,
							   int, int, int minrange, int maxrange,
							   //Wyrmgus start
//							   std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, const CUnit &unit)
							   std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, const CUnit &unit, int z)
							   //Wyrmgus end
{
	// At exact destination point already
	if (goal == startPos && minrange == 0) {
		return PF_REACHED;
	}

	// Don't allow unit inside destination area
	if (goal.x <= startPos.x && startPos.x <= goal.x + gw - 1
		&& goal.y <= startPos.y && startPos.y <= goal.y + gh - 1) {
		return PF_FAILED;
	}

	const Vec2i diff = goal - startPos;
	const int distance = isqrt(square(diff.x) + square(diff.y));

	// Within range of destination
	if (minrange <= distance && distance <= maxrange) {
		return PF_REACHED;
	}

	//Wyrmgus start
//	if (number::fast_abs(diff.x) <= 1 && number::fast_abs(diff.y) <= 1) {
	if (minrange <= distance && number::fast_abs(diff.x) <= 1 && number::fast_abs(diff.y) <= 1) {
	//Wyrmgus end
		// Move to adjacent cell
		if (CostMoveTo(GetIndex(goal.x, goal.y, z), unit, z) == -1) {
			return PF_UNREACHABLE;
		}

		if (path != nullptr) {
			(*path)[0] = XY2Heading[diff.x + 1][diff.y + 1];
		}

		return 1;
	}

	return PF_FAILED;
}

/**
**  Find path.
**
**  @return  _move_return_ or the path length
*/
int AStarFindPath(const Vec2i &startPos, const Vec2i &goalPos, const int gw, const int gh,
				  const int tilesizex, const int tilesizey, const int minrange, const int maxrange,
				  //Wyrmgus start
//				  std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, const CUnit &unit)
                  std::array<char, PathFinderOutput::MAX_PATH_LENGTH> *path, const CUnit &unit, const int max_length, const int z)
				  //Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(startPos, z));
	
	//Wyrmgus start
	if (unit.MapLayer->ID != z) {
		return PF_UNREACHABLE;
	}
	//Wyrmgus end

	AStarGoalX = goalPos.x;
	AStarGoalY = goalPos.y;

	AStarGoalZ = z;

	//  Check for simple cases first
	int ret = AStarFindSimplePath(startPos, goalPos, gw, gh, tilesizex, tilesizey,
								  //Wyrmgus start
//								  minrange, maxrange, path, unit);
								  minrange, maxrange, path, unit, z);
								  //Wyrmgus end
	if (ret != PF_FAILED) {
		return ret;
	}

	//  Initialize
	AStarCleanUp(z);
	CostMoveToCacheCleanUp(z);

	OpenSet[z].clear();
	CloseSet[z].clear();

	//Wyrmgus start
//	if (!AStarMarkGoal(goalPos, gw, gh, tilesizex, tilesizey, minrange, maxrange, unit)) {
	if (!AStarMarkGoal(goalPos, gw, gh, tilesizex, tilesizey, minrange, maxrange, unit, z)) {
	//Wyrmgus end
		// goal is not reachable
		ret = PF_UNREACHABLE;
		return ret;
	}

	//Wyrmgus start
//	int eo = startPos.y * AStarMapWidth + startPos.x;
	int eo = startPos.y * AStarMapWidth[z] + startPos.x;
	//Wyrmgus end
	// it is quite important to start from 1 rather than 0, because we use
	// 0 as a way to represent nodes that we have not visited yet.
	//Wyrmgus start
//	AStarMatrix[eo].CostFromStart = 1;
	AStarMatrix[z][eo].CostFromStart = 1;
	//Wyrmgus end
	// 8 to say we are came from nowhere.
	//Wyrmgus start
//	AStarMatrix[eo].Direction = 8;
	AStarMatrix[z][eo].Direction = 8;
	//Wyrmgus end

	// place start point in open, it that failed, try another pathfinder
	int costToGoal = AStarCosts(startPos, goalPos);
	//Wyrmgus start
//	AStarMatrix[eo].CostToGoal = costToGoal;
//	if (AStarAddNode(startPos, eo, 1 + costToGoal) == PF_FAILED) {
	AStarMatrix[z][eo].CostToGoal = costToGoal;
	if (AStarAddNode(startPos, eo, 1 + costToGoal, z) == PF_FAILED) {
	//Wyrmgus end
		ret = PF_FAILED;
		return ret;
	}

	AStarAddToClose((*OpenSet[z].begin()).offset, z);

	if (AStarMatrix[z][eo].InGoal) {
		ret = PF_REACHED;
		return ret;
	}

	Vec2i endPos;
	
	//Wyrmgus start
	int length = 0;
	//Wyrmgus end

	//  Begin search
	while (true) {
		//Wyrmgus start
		if (max_length != 0 && length > max_length) {
			ret = PF_FAILED;
			return ret;
		}
		//Wyrmgus end
		
		// Find the best node of from the open set
		const Open shortest = std::move(OpenSet[z].extract(OpenSet[z].begin()).value());
		const int x = shortest.pos.x;
		const int y = shortest.pos.y;
		const int o = shortest.offset;

		// If we have reached the goal, then exit.
		if (AStarMatrix[z][o].InGoal == 1) {
			endPos.x = x;
			endPos.y = y;
			break;
		}

#if 0
		// If we have looked too long, then exit.
		if (!counter--) {
			// FIXME: Select a "good" point from the open set.
			// Nearest point to goal.
			AstarDebugPrint("way too long\n");
			ret = PF_FAILED;
			ProfileEnd("AStarFindPath");
			return ret;
		}
#endif

		// Generate successors of this node.

		// Node that this node was generated from.
		const int px = x - Heading2X[(int)AStarMatrix[z][o].Direction];
		const int py = y - Heading2Y[(int)AStarMatrix[z][o].Direction];

		for (int i = 0; i < 8; ++i) {
			endPos.x = x + Heading2X[i];
			endPos.y = y + Heading2Y[i];

			// Don't check the tile we came from, it's not going to be better
			// Should reduce load on A*

			if (endPos.x == px && endPos.y == py) {
				continue;
			}

			// Outside the map or can't be entered.
			if (endPos.x < 0 || endPos.x + tilesizex - 1 >= AStarMapWidth[z]
				//Wyrmgus start
//				|| endPos.y < 0 || endPos.y + tilesizey - 1 >= AStarMapHeight) {
				|| endPos.y < 0 || endPos.y + tilesizey - 1 >= AStarMapHeight[z]
				|| !CMap::get()->Info->IsPointOnMap(endPos, z)) {
				//Wyrmgus end
				continue;
			}

			//eo = GetIndex(ex, ey);
			eo = endPos.x + (o - x) + Heading2O[i][z];

			// if the point is "move to"-able and
			// if we have not reached this point before,
			// or if we have a better path to it, we add it to open set
			int new_cost = CostMoveTo(eo, unit, z);
			if (new_cost == -1) {
				// uncrossable tile
				continue;
			}

			// Add a cost for walking to make paths more realistic for the user.
			new_cost++;
			//Wyrmgus start
//			new_cost += AStarMatrix[o].CostFromStart;
//			if (AStarMatrix[eo].CostFromStart == 0) {
			new_cost += AStarMatrix[z][o].CostFromStart;
			if (AStarMatrix[z][eo].CostFromStart == 0) {
			//Wyrmgus end
				// we are sure the current node has not been already visited
				AStarMatrix[z][eo].CostFromStart = new_cost;
				AStarMatrix[z][eo].Direction = i;
				costToGoal = AStarCosts(endPos, goalPos);
				//Wyrmgus start
//				AStarMatrix[eo].CostToGoal = costToGoal;
//				if (AStarAddNode(endPos, eo, AStarMatrix[eo].CostFromStart + costToGoal) == PF_FAILED) {
				AStarMatrix[z][eo].CostToGoal = costToGoal;
				if (AStarAddNode(endPos, eo, AStarMatrix[z][eo].CostFromStart + costToGoal, z) == PF_FAILED) {
				//Wyrmgus end
					ret = PF_FAILED;
					return ret;
				}
				// we add the point to the close set
				AStarAddToClose(eo, z);
			} else if (new_cost < AStarMatrix[z][eo].CostFromStart) {
				// Already visited node, but we have here a better path
				// I know, it's redundant (but simpler like this)
				AStarMatrix[z][eo].CostFromStart = new_cost;
				AStarMatrix[z][eo].Direction = i;

				// this point might be already in the OpenSet
				const Open *j = AStarFindNode(eo, z);
				if (j == nullptr) {
					costToGoal = AStarCosts(endPos, goalPos);
					//Wyrmgus start
//					AStarMatrix[eo].CostToGoal = costToGoal;
//					if (AStarAddNode(endPos, eo, AStarMatrix[eo].CostFromStart + costToGoal) == PF_FAILED) {
					AStarMatrix[z][eo].CostToGoal = costToGoal;
					if (AStarAddNode(endPos, eo, AStarMatrix[z][eo].CostFromStart + costToGoal, z) == PF_FAILED) {
					//Wyrmgus end
						ret = PF_FAILED;
						return ret;
					}
				} else {
					costToGoal = AStarCosts(endPos, goalPos);
					//Wyrmgus start
//					AStarMatrix[eo].CostToGoal = costToGoal;
//					AStarReplaceNode(j);
					AStarMatrix[z][eo].CostToGoal = costToGoal;
					AStarReplaceNode(j, z);
					//Wyrmgus end
				}
				// we don't have to add this point to the close set
			}
		}

		if (OpenSet[z].size() <= 0) { // no new nodes generated
			ret = PF_UNREACHABLE;
			return ret;
		}
		
		++length;
	}

	//Wyrmgus start
//	const int path_length = AStarSavePath(startPos, endPos, path);
	const int path_length = AStarSavePath(startPos, endPos, path, z);
	//Wyrmgus end

	ret = path_length;

	return ret;
}

struct StatsNode {
	int Direction = 0;
	int InGoal = 0;
	int CostFromStart = 0;
	int Costs = 0;
	int CostToGoal = 0;
};

/*----------------------------------------------------------------------------
--  Configurable costs
----------------------------------------------------------------------------*/

// AStarFixedUnitCrossingCost
void SetAStarFixedUnitCrossingCost(int cost)
{
	if (cost <= 3) {
		log::log_error("AStarFixedUnitCrossingCost must be greater than 3.");
	}
}
int GetAStarFixedUnitCrossingCost()
{
	return AStarFixedUnitCrossingCost;
}

// AStarMovingUnitCrossingCost
void SetAStarMovingUnitCrossingCost(int cost)
{
	if (cost <= 3) {
		log::log_error("AStarMovingUnitCrossingCost must be greater than 3.");
	}
}
int GetAStarMovingUnitCrossingCost()
{
	return AStarMovingUnitCrossingCost;
}

// AStarUnknownTerrainCost
void SetAStarUnknownTerrainCost(int cost)
{
	if (cost < 0) {
		log::log_error("AStarUnknownTerrainCost must be non-negative.");
		return;
	}
	AStarUnknownTerrainCost = cost;
}

int GetAStarUnknownTerrainCost()
{
	return AStarUnknownTerrainCost;
}
