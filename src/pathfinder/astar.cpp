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
/**@name astar.cpp - The a* path finder routines. */
//
//      (c) Copyright 1999-2008 by Lutz Sammer, Fabrice Rossi, Russell Smith,
//                                 Francois Beerten, Jimmy Salmon.
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "settings.h"
#include "time/time_of_day.h"
#include "unit/unit.h"
#include "unit/unit_find.h"

#include "pathfinder/pathfinder.h"

#include <stdio.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

struct AStarNode {
	int CostFromStart;  /// Real costs to reach this point
	short int CostToGoal;     /// Estimated cost to goal
	char InGoal;        /// is this point in the goal
	char Direction;     /// Direction for trace back
};

struct Open {
	Vec2i pos;
	short int Costs; /// complete costs to goal
	//Wyrmgus start
//	unsigned short int O;     /// Offset into matrix
	unsigned int O;     /// Offset into matrix
	//Wyrmgus end
};

//for 32 bit signed int
inline int MyAbs(int x) { return (x ^ (x >> 31)) - (x >> 31); }

/// heuristic cost function for a*
static inline int AStarCosts(const Vec2i &pos, const Vec2i &goalPos)
{
	const Vec2i diff = pos - goalPos;
	return std::max<int>(MyAbs(diff.x), MyAbs(diff.y));
}

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//  Convert heading into direction.
//                      //  N NE  E SE  S SW  W NW
const int Heading2X[9] = {  0, +1, +1, +1, 0, -1, -1, -1, 0 };
const int Heading2Y[9] = { -1, -1, 0, +1, +1, +1, 0, -1, 0 };
//Wyrmgus start
//int Heading2O[9];//heading to offset
std::vector<int> Heading2O[9];//heading to offset
//Wyrmgus end
const int XY2Heading[3][3] = { {7, 6, 5}, {0, 0, 4}, {1, 2, 3}};

/// cost matrix
//Wyrmgus start
//static AStarNode *AStarMatrix;
static std::vector<AStarNode *> AStarMatrix;
//Wyrmgus end

/// a list of close nodes, helps to speed up the matrix cleaning
//Wyrmgus start
//static int *CloseSet;
//static int CloseSetSize;
//static int Threshold;
//static int OpenSetMaxSize;
//static int AStarMatrixSize;
static std::vector<int *> CloseSet;
static std::vector<int> CloseSetSize;
static std::vector<int> Threshold;
static std::vector<int> OpenSetMaxSize;
static std::vector<int> AStarMatrixSize;
//Wyrmgus end
constexpr int MAX_CLOSE_SET_RATIO = 4;
constexpr int MAX_OPEN_SET_RATIO = 8; // 10,16 to small

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

/**
**  The Open set is handled by a stored array
**  the end of the array holds the item with the smallest cost.
*/

/// The set of Open nodes
//Wyrmgus start
//static Open *OpenSet;
static std::vector<Open *> OpenSet;
//Wyrmgus end
/// The size of the open node set
//Wyrmgus start
//static int OpenSetSize;
static std::vector<int> OpenSetSize;
//Wyrmgus end

//Wyrmgus start
//static int *CostMoveToCache;
static std::vector<int *> CostMoveToCache;
//Wyrmgus end
static const int CacheNotSet = -5;

/*----------------------------------------------------------------------------
--  Profile
----------------------------------------------------------------------------*/

#ifdef ASTAR_PROFILE

#include <map>
#ifdef _WIN32
#include <windows.h>
#else

union LARGE_INTEGER {
	uint64_t QuadPart;
	uint32_t DoublePart[2];
};
inline int QueryPerformanceCounter(LARGE_INTEGER *ptr)
{
	unsigned int lo, hi;
	__asm__ __volatile__(       // serialize
		"xorl %%eax,%%eax \n        cpuid"
		::: "%rax", "%rbx", "%rcx", "%rdx");
	/* We cannot use "=A", since this would use %rax on x86_64 */
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	ptr->DoublePart[0] = lo;
	ptr->DoublePart[1] = hi;
	return 1;
};

inline int QueryPerformanceFrequency(LARGE_INTEGER *ptr)
{
	ptr->QuadPart = 1000;
	return 1;
}

#endif

#undef max
#undef min
static std::map<const char *const, LARGE_INTEGER> functionTimerMap;
struct ProfileData {
	unsigned long Calls;
	unsigned long TotalTime;
};
static std::map<const char *const, ProfileData> functionProfiles;

inline void ProfileInit()
{
	functionTimerMap.clear();
	functionProfiles.clear();
}

inline void ProfileBegin(const char *const function)
{
	LARGE_INTEGER counter;
	if (!QueryPerformanceCounter(&counter)) {
		return;
	}
	functionTimerMap[function] = counter;
}

inline void ProfileEnd(const char *const function)
{
	LARGE_INTEGER counter;
	if (!QueryPerformanceCounter(&counter)) {
		return;
	}
	unsigned long time = (unsigned long)(counter.QuadPart - functionTimerMap[function].QuadPart);
	ProfileData *data = &functionProfiles[function];
	data->Calls++;
	data->TotalTime += time;
}

static bool compProfileData(const ProfileData *lhs, const ProfileData *rhs)
{
	return (lhs->TotalTime > rhs->TotalTime);
}


inline void ProfilePrint()
{
	LARGE_INTEGER frequency;
	if (!QueryPerformanceFrequency(&frequency)) {
		return;
	}
	std::vector<ProfileData *> prof;
	for (std::map<const char *const, ProfileData>::iterator i = functionProfiles.begin();
		 i != functionProfiles.end(); ++i) {
		ProfileData *data = &i->second;
		prof.insert(std::upper_bound(prof.begin(), prof.end(), data, compProfileData), data);
	}

	FILE *fd = fopen("profile.txt", "wb");
	fprintf(fd, "    total\t    calls\t      per\tname\n");

	for (std::vector<ProfileData *>::iterator i = prof.begin(); i != prof.end(); ++i) {
		ProfileData *data = (*i);
		fprintf(fd, "%9.3f\t%9lu\t%9.3f\t",
				(double)data->TotalTime / frequency.QuadPart * 1000.0,
				data->Calls,
				(double)data->TotalTime / frequency.QuadPart * 1000.0 / data->Calls);
		for (std::map<const char *const, ProfileData>::iterator j =
				 functionProfiles.begin(); j != functionProfiles.end(); ++j) {
			ProfileData *data2 = &j->second;
			if (data == data2) {
				fprintf(fd, "%s\n", j->first);
			}
		}

	}

	fclose(fd);
}

#else
#define ProfileInit()
#define ProfileBegin(f)
#define ProfileEnd(f)
#define ProfilePrint()
#endif

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Init A* data structures
*/
//Wyrmgus start
//void InitAStar(int mapWidth, int mapHeight)
void InitAStar()
//Wyrmgus end
{
	//Wyrmgus start
	/*
	// Should only be called once
	Assert(!AStarMatrix);

	AStarMapWidth = mapWidth;
	AStarMapHeight = mapHeight;

	AStarMatrixSize = sizeof(AStarNode) * AStarMapWidth * AStarMapHeight;
	AStarMatrix = new AStarNode[AStarMapWidth * AStarMapHeight];
	memset(AStarMatrix, 0, AStarMatrixSize);

	Threshold = AStarMapWidth * AStarMapHeight / MAX_CLOSE_SET_RATIO;
	CloseSet = new int[Threshold];

	OpenSetMaxSize = AStarMapWidth * AStarMapHeight / MAX_OPEN_SET_RATIO;
	OpenSet = new Open[OpenSetMaxSize];

	CostMoveToCache = new int[AStarMapWidth * AStarMapHeight];

	for (int i = 0; i < 9; ++i) {
		Heading2O[i] = Heading2Y[i] * AStarMapWidth;
	}
	*/

	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		// Should only be called once
		Assert(AStarMatrix.size() <= z);
	
		AStarMapWidth.push_back(CMap::Map.Info.MapWidths[z]);
		AStarMapHeight.push_back(CMap::Map.Info.MapHeights[z]);
		
		AStarMatrixSize.push_back(sizeof(AStarNode) * AStarMapWidth[z] * AStarMapHeight[z]);
		AStarMatrix.push_back(new AStarNode[AStarMapWidth[z] * AStarMapHeight[z]]);
		memset(AStarMatrix[z], 0, AStarMatrixSize[z]);

		Threshold.push_back(AStarMapWidth[z] * AStarMapHeight[z] / MAX_CLOSE_SET_RATIO);
		CloseSet.push_back(new int[Threshold[z]]);
		CloseSetSize.push_back(0);

		OpenSetMaxSize.push_back(AStarMapWidth[z] * AStarMapHeight[z] / MAX_OPEN_SET_RATIO);
		OpenSet.push_back(new Open[OpenSetMaxSize[z]]);
		OpenSetSize.push_back(0);

		CostMoveToCache.push_back(new int[AStarMapWidth[z] * AStarMapHeight[z]]);

		for (int i = 0; i < 9; ++i) {
			Heading2O[i].push_back(Heading2Y[i] * AStarMapWidth[z]);
		}
	}
	//Wyrmgus end

	ProfileInit();
}

/**
**  Free A* data structure
*/
void FreeAStar()
{
	//Wyrmgus start
	AStarMapWidth.clear();
	AStarMapHeight.clear();
	//Wyrmgus end
	//Wyrmgus start
	/*
	delete[] AStarMatrix;
	AStarMatrix = nullptr;
	delete[] CloseSet;
	CloseSet = nullptr;
	CloseSetSize = 0;
	delete[] OpenSet;
	OpenSet = nullptr;
	OpenSetSize = 0;
	delete[] CostMoveToCache;
	CostMoveToCache = nullptr;
	*/
	for (size_t z = 0; z < AStarMatrix.size(); ++z) {
		delete[] AStarMatrix[z];
		AStarMatrix[z] = nullptr;
	}
	AStarMatrix.clear();
	AStarMatrixSize.clear();
	Threshold.clear();
	for (size_t z = 0; z < CloseSet.size(); ++z) {
		delete[] CloseSet[z];
		CloseSet[z] = nullptr;
	}
	CloseSet.clear();
	CloseSetSize.clear();
	for (size_t z = 0; z < OpenSet.size(); ++z) {
		delete[] OpenSet[z];
		OpenSet[z] = nullptr;
	}
	OpenSet.clear();
	OpenSetSize.clear();
	OpenSetMaxSize.clear();
	for (size_t z = 0; z < CostMoveToCache.size(); ++z) {
		delete[] CostMoveToCache[z];
		CostMoveToCache[z] = nullptr;
	}
	CostMoveToCache.clear();
	
	for (int i = 0; i < 9; ++i) {
		Heading2O[i].clear();
	}
	//Wyrmgus end

	ProfilePrint();
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
//	memset(AStarMatrix, 0, AStarMatrixSize);
	memset(AStarMatrix[z], 0, AStarMatrixSize[z]);
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
	ProfileBegin("AStarCleanUp");

	//Wyrmgus start
//	if (CloseSetSize >= Threshold) {
	if (CloseSetSize[z] >= Threshold[z]) {
	//Wyrmgus end
		//Wyrmgus start
//		AStarPrepare();
		AStarPrepare(z);
		//Wyrmgus end
	} else {
		for (int i = 0; i < CloseSetSize[z]; ++i) {
			//Wyrmgus start
//			AStarMatrix[CloseSet[i]].CostFromStart = 0;
//			AStarMatrix[CloseSet[i]].InGoal = 0;
			AStarMatrix[z][CloseSet[z][i]].CostFromStart = 0;
			AStarMatrix[z][CloseSet[z][i]].InGoal = 0;
			//Wyrmgus end
		}
	}
	ProfileEnd("AStarCleanUp");
}

//Wyrmgus start
//static void CostMoveToCacheCleanUp()
static void CostMoveToCacheCleanUp(int z)
//Wyrmgus end
{
	ProfileBegin("CostMoveToCacheCleanUp");
	//Wyrmgus start
//	int AStarMapMax =  AStarMapWidth * AStarMapHeight;
	int AStarMapMax =  AStarMapWidth[z] * AStarMapHeight[z];
	//Wyrmgus end
#if 1
	//Wyrmgus start
//	int *ptr = CostMoveToCache;
	int *ptr = CostMoveToCache[z];
	//Wyrmgus end
#ifdef __x86_64__
	union {
		intptr_t d;
		int i[2];
	} conv;
	conv.i[0] = CacheNotSet;
	conv.i[1] = CacheNotSet;

	if (((uintptr_t)ptr) & 4) {
		*ptr++ = CacheNotSet;
		--AStarMapMax;
	}
#endif
	while (AStarMapMax > 3) {
#ifdef __x86_64__
		*((intptr_t *)ptr) = conv.d;
		*((intptr_t *)(ptr + 2)) = conv.d;
		ptr += 4;
#else
		*ptr++ = CacheNotSet;
		*ptr++ = CacheNotSet;
		*ptr++ = CacheNotSet;
		*ptr++ = CacheNotSet;
#endif
		AStarMapMax -= 4;
	};
	while (AStarMapMax) {
		*ptr++ = CacheNotSet;
		--AStarMapMax;
	}
#else
	for (int i = 0; i < AStarMapMax; ++i) {
		//Wyrmgus start
//		CostMoveToCache[i] = CacheNotSet;
		CostMoveToCache[z][i] = CacheNotSet;
		//Wyrmgus end
	}
#endif
	ProfileEnd("CostMoveToCacheCleanUp");
}

/**
**  Find the best node in the current open node set
**  Returns the position of this node in the open node set
*/
//Wyrmgus start
//#define AStarFindMinimum() (OpenSetSize - 1)
#define AStarFindMinimum(z) (OpenSetSize[(z)] - 1)
//Wyrmgus end

/**
**  Remove the minimum from the open node set
*/
//Wyrmgus start
//static void AStarRemoveMinimum(int pos)
static void AStarRemoveMinimum(int pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(pos == OpenSetSize - 1);
	Assert(pos == OpenSetSize[z] - 1);
	//Wyrmgus end

	//Wyrmgus start
//	OpenSetSize--;
	OpenSetSize[z]--;
	//Wyrmgus end
}

/**
**  Add a new node to the open set (and update the heap structure)
**
**  @return  0 or PF_FAILED
*/
//Wyrmgus start
//static inline int AStarAddNode(const Vec2i &pos, int o, int costs)
static inline int AStarAddNode(const Vec2i &pos, int o, int costs, int z)
//Wyrmgus end
{
	ProfileBegin("AStarAddNode");

	//Wyrmgus start
//	int bigi = 0, smalli = OpenSetSize;
	int bigi = 0, smalli = OpenSetSize[z];
	//Wyrmgus end
	int midcost;
	int midi;
	int midCostToGoal;
	int midDist;
	const Open *open;

	//Wyrmgus start
//	if (OpenSetSize + 1 >= OpenSetMaxSize) {
	if (OpenSetSize[z] + 1 >= OpenSetMaxSize[z]) {
	//Wyrmgus end
		fprintf(stderr, "A* internal error: raise Open Set Max Size "
				//Wyrmgus start
//				"(current value %d)\n", OpenSetMaxSize);
				"(current value %d)\n", OpenSetMaxSize[z]);
				//Wyrmgus end
		ProfileEnd("AStarAddNode");
		return PF_FAILED;
	}

	//Wyrmgus start
//	const int costToGoal = AStarMatrix[o].CostToGoal;
	const int costToGoal = AStarMatrix[z][o].CostToGoal;
	//Wyrmgus end
	const int dist = MyAbs(pos.x - AStarGoalX) + MyAbs(pos.y - AStarGoalY);

	// find where we should insert this node.
	// binary search where to insert the new node
	while (bigi < smalli) {
		midi = (smalli + bigi) >> 1;
		//Wyrmgus start
//		open = &OpenSet[midi];
		open = &OpenSet[z][midi];
		//Wyrmgus end
		midcost = open->Costs;
		//Wyrmgus start
//		midCostToGoal = AStarMatrix[open->O].CostToGoal;
		midCostToGoal = AStarMatrix[z][open->O].CostToGoal;
		//Wyrmgus end
		midDist = MyAbs(open->pos.x - AStarGoalX) + MyAbs(open->pos.y - AStarGoalY);
		if (costs > midcost || (costs == midcost
								&& (costToGoal > midCostToGoal || (costToGoal == midCostToGoal
																   && dist > midDist)))) {
			smalli = midi;
		} else if (costs < midcost || (costs == midcost
									   && (costToGoal < midCostToGoal || (costToGoal == midCostToGoal
											   && dist < midDist)))) {
			if (bigi == midi) {
				bigi++;
			} else {
				bigi = midi;
			}
		} else {
			bigi = midi;
			smalli = midi;
		}
	}

	//Wyrmgus start
//	if (OpenSetSize > bigi) {
	if (OpenSetSize[z] > bigi) {
	//Wyrmgus end
		// free a the slot for our node
		//Wyrmgus start
//		memmove(&OpenSet[bigi + 1], &OpenSet[bigi], (OpenSetSize - bigi) * sizeof(Open));
		memmove(&OpenSet[z][bigi + 1], &OpenSet[z][bigi], (OpenSetSize[z] - bigi) * sizeof(Open));
		//Wyrmgus end
	}

	// fill our new node
	//Wyrmgus start
//	OpenSet[bigi].pos = pos;
//	OpenSet[bigi].O = o;
//	OpenSet[bigi].Costs = costs;
//	++OpenSetSize;
	OpenSet[z][bigi].pos = pos;
	OpenSet[z][bigi].O = o;
	OpenSet[z][bigi].Costs = costs;
	++OpenSetSize[z];
	//Wyrmgus end

	ProfileEnd("AStarAddNode");

	return 0;
}

/**
**  Change the cost associated to an open node.
**  Can be further optimised knowing that the new cost MUST BE LOWER
**  than the old one.
*/
//Wyrmgus start
//static void AStarReplaceNode(int pos)
static void AStarReplaceNode(int pos, int z)
//Wyrmgus end
{
	ProfileBegin("AStarReplaceNode");

	Open node;

	// Remove the outdated node
	//Wyrmgus start
//	node = OpenSet[pos];
//	OpenSetSize--;
//	memmove(&OpenSet[pos], &OpenSet[pos+1], sizeof(Open) * (OpenSetSize-pos));
	node = OpenSet[z][pos];
	OpenSetSize[z]--;
	memmove(&OpenSet[z][pos], &OpenSet[z][pos+1], sizeof(Open) * (OpenSetSize[z]-pos));
	//Wyrmgus end

	// Re-add the node with the new cost
	//Wyrmgus start
//	AStarAddNode(node.pos, node.O, node.Costs);
	AStarAddNode(node.pos, node.O, node.Costs, z);
	//Wyrmgus end
	ProfileEnd("AStarReplaceNode");
}


/**
**  Check if a node is already in the open set.
**
**  @return  -1 if not found and the position of the node in the table if found.
*/
//Wyrmgus start
//static int AStarFindNode(int eo)
static int AStarFindNode(int eo, int z)
//Wyrmgus end
{
	ProfileBegin("AStarFindNode");

	//Wyrmgus start
//	for (int i = 0; i < OpenSetSize; ++i) {
	for (int i = 0; i < OpenSetSize[z]; ++i) {
	//Wyrmgus end
		//Wyrmgus start
//		if (OpenSet[i].O == eo) {
		if (OpenSet[z][i].O == eo) {
		//Wyrmgus end
			ProfileEnd("AStarFindNode");
			return i;
		}
	}
	ProfileEnd("AStarFindNode");
	return -1;
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
//	if (CloseSetSize < Threshold) {
	if (CloseSetSize[z] < Threshold[z]) {
	//Wyrmgus end
		//Wyrmgus start
//		CloseSet[CloseSetSize++] = node;
		CloseSet[z][CloseSetSize[z]++] = node;
		//Wyrmgus end
	}
}

static int GetIndex(const int x, const int y, const int z)
{
	return x + y * AStarMapWidth[z];
}

/* build-in costmoveto code */
static int CostMoveToCallBack_Default(unsigned int index, const CUnit &unit, int z)
{
#ifdef DEBUG
	{
		Vec2i pos;
		pos.y = index / CMap::Map.Info.MapWidths[z];
		pos.x = index - pos.y * CMap::Map.Info.MapWidths[z];
		Assert(CMap::Map.Info.IsPointOnMap(pos, z));
	}
#endif
	int cost = 0;
	const int mask = unit.Type->MovementMask;
	const CUnitTypeFinder unit_finder((UnitTypeType)unit.Type->UnitType);

	// verify each tile of the unit.
	int h = unit.Type->TileSize.y;
	const int w = unit.Type->TileSize.x;
	do {
		const CMapField *mf = CMap::Map.Field(index, z);
		int i = w;
		do {
			//Wyrmgus start
//			const int flag = mf->Flags & mask;
			//for purposes of this check, don't count MapFieldWaterAllowed and MapFieldCoastAllowed if there is a bridge present
			unsigned long check_flags = mf->Flags;
			if (check_flags & MapFieldBridge) {
				check_flags &= ~(MapFieldWaterAllowed | MapFieldCoastAllowed);
			}
			const unsigned long flag = check_flags & mask;
			//Wyrmgus end
			
			if (flag && (AStarKnowUnseenTerrain || mf->playerInfo.IsTeamExplored(*unit.Player))) {
				if (flag & ~(MapFieldLandUnit | MapFieldAirUnit | MapFieldSeaUnit)) {
					// we can't cross fixed units and other unpassable things
					return -1;
				}
				CUnit *goal = mf->UnitCache.find(unit_finder);
				if (!goal) {
					// Shouldn't happen, mask says there is something on this tile
					Assert(0);
					return -1;
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
//						if (goal->Player->IsEnemy(unit) && unit.IsAgressive() && CanTarget(*unit.Type, *goal->Type)
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
			if (!AStarKnowUnseenTerrain && !mf->playerInfo.IsTeamExplored(*unit.Player)) {
				// Tend against unknown tiles.
				cost += AStarUnknownTerrainCost;
			}
			
			//Wyrmgus start
			if (
				(mf->Flags & MapFieldDesert)
				&& mf->Owner != unit.Player->GetIndex()
				&& unit.Type->BoolFlag[ORGANIC_INDEX].value
				&& unit.MapLayer->GetTimeOfDay() != nullptr
				&& unit.MapLayer->GetTimeOfDay()->IsDay()
				&& unit.Variable[DEHYDRATIONIMMUNITY_INDEX].Value <= 0
			) {
				cost += 32; //increase the cost of moving through deserts for units affected by dehydration, as we want the pathfinding to try to avoid that
			}
			//Wyrmgus end
			
			// Add tile movement cost
			if (unit.Type->UnitType == UnitTypeFly || unit.Type->UnitType == UnitTypeFlyLow) {
				cost += DefaultTileMovementCost;
			} else {
				cost += mf->getCost();
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
//Wyrmgus start
//static inline int CostMoveTo(unsigned int index, const CUnit &unit)
static inline int CostMoveTo(unsigned int index, const CUnit &unit, int z)
//Wyrmgus end
{
	//Wyrmgus start
	if (!&unit) {
		fprintf(stderr, "Error in CostMoveTo(unsigned int index, const CUnit &unit): Unit is null.\n");
		return -1;
	}
	//Wyrmgus end
	//Wyrmgus start
//	int *c = &CostMoveToCache[index];
	int *c = &CostMoveToCache[z][index];
	//Wyrmgus end
	if (*c != CacheNotSet) {
		return *c;
	}
	//Wyrmgus start
//	*c = CostMoveToCallBack_Default(index, unit);
	*c = CostMoveToCallBack_Default(index, unit, z);
	//Wyrmgus end
	return *c;
}

class AStarGoalMarker
{
public:
	AStarGoalMarker(const CUnit &unit, bool *goal_reachable) :
		unit(unit), goal_reachable(goal_reachable)
	{}

	//Wyrmgus start
//	void operator()(int offset) const
	void operator()(int offset, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		if (CostMoveTo(offset, unit) >= 0) {
		if (CostMoveTo(offset, unit, z) >= 0) {
		//Wyrmgus end
			//Wyrmgus start
//			AStarMatrix[offset].InGoal = 1;
			AStarMatrix[z][offset].InGoal = 1;
			//Wyrmgus end
			*goal_reachable = true;
		}
		//Wyrmgus start
//		AStarAddToClose(offset);
		AStarAddToClose(offset, z);
		//Wyrmgus end
	}
private:
	const CUnit &unit;
	bool *goal_reachable;
};


template <typename T>
class MinMaxRangeVisitor
{
public:
	explicit MinMaxRangeVisitor(const T &func) : func(func) {}

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
//			const int maxx = std::min(CMap::Map.Info.MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			const int maxx = std::min(CMap::Map.Info.MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			//Wyrmgus end
			Vec2i mpos(minx, y);
			//Wyrmgus start
//			const unsigned int offset = mpos.y * CMap::Map.Info.MapWidth;
			const unsigned int offset = mpos.y * CMap::Map.Info.MapWidths[z];
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
//		const int maxx = std::min(CMap::Map.Info.MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetmaxx);
		const int maxx = std::min(CMap::Map.Info.MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetmaxx);
		//Wyrmgus end
		Vec2i mpos(minx, y);
		//Wyrmgus start
//		const unsigned int offset = mpos.y * CMap::Map.Info.MapWidth;
		const unsigned int offset = mpos.y * CMap::Map.Info.MapWidths[z];
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
//		const int maxy = std::min<int>(CMap::Map.Info.MapHeight - 1 - unitExtraTileSize.y, goalBottomRight.y);
		const int maxy = std::min<int>(CMap::Map.Info.MapHeights[z] - 1 - unitExtraTileSize.y, goalBottomRight.y);
		//Wyrmgus end
		const int minx = std::max(0, goalTopLeft.x - maxrange - unitExtraTileSize.x);
		//Wyrmgus start
//		const int maxx = std::min<int>(CMap::Map.Info.MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + maxrange);
		const int maxx = std::min<int>(CMap::Map.Info.MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + maxrange);
		//Wyrmgus end

		if (minrange == 0) {
			for (int y = miny; y <= maxy; ++y) {
				Vec2i mpos(minx, y);
				//Wyrmgus start
//				const unsigned int offset = mpos.y * CMap::Map.Info.MapWidth;
				const unsigned int offset = mpos.y * CMap::Map.Info.MapWidths[z];
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
//				const unsigned int offset = mpos.y * CMap::Map.Info.MapWidth;
				const unsigned int offset = mpos.y * CMap::Map.Info.MapWidths[z];
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
//		const int maxy = std::min(CMap::Map.Info.MapHeight - 1 - unitExtraTileSize.y, goalBottomRight.y + (minrange - 1));
		const int maxy = std::min(CMap::Map.Info.MapHeights[z] - 1 - unitExtraTileSize.y, goalBottomRight.y + (minrange - 1));
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
//		const int maxy = std::min(CMap::Map.Info.MapHeight - 1 - unitExtraTileSize.y, goalBottomRight.y + maxrange);
		const int maxy = std::min(CMap::Map.Info.MapHeights[z] - 1 - unitExtraTileSize.y, goalBottomRight.y + maxrange);
		//Wyrmgus end
		for (int y = miny; y <= maxy; ++y) {
			const int offsetx = GetMaxOffsetX(y - goalBottomRight.y, maxrange);
			const int minx = std::max(0, goalTopLeft.x - offsetx - unitExtraTileSize.x);
			//Wyrmgus start
//			const int maxx = std::min(CMap::Map.Info.MapWidth - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			const int maxx = std::min(CMap::Map.Info.MapWidths[z] - 1 - unitExtraTileSize.x, goalBottomRight.x + offsetx);
			//WYrmgus end
			Vec2i mpos(minx, y);
			//Wyrmgus start
//			const unsigned int offset = mpos.y * CMap::Map.Info.MapWidth;
			const unsigned int offset = mpos.y * CMap::Map.Info.MapWidths[z];
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
	int minrange = 0;
	int maxrange = 0;
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
	ProfileBegin("AStarMarkGoal");

	if (minrange == 0 && maxrange == 0 && gw == 0 && gh == 0) {
		//Wyrmgus start
//		if (goal.x + tilesizex > AStarMapWidth || goal.y + tilesizey > AStarMapHeight) {
		if (goal.x + tilesizex - 1 > AStarMapWidth[z] || goal.y + tilesizey - 1 > AStarMapHeight[z]) {
		//Wyrmgus end
			ProfileEnd("AStarMarkGoal");
			return 0;
		}
		//Wyrmgus start
//		unsigned int offset = GetIndex(goal.x, goal.y);
//		if (CostMoveTo(offset, unit) >= 0) {
		unsigned int offset = GetIndex(goal.x, goal.y, z);
		if (CostMoveTo(offset, unit, z) >= 0) {
		//Wyrmgus end
			//Wyrmgus start
//			AStarMatrix[offset].InGoal = 1;
			AStarMatrix[z][offset].InGoal = 1;
			//Wyrmgus end
			ProfileEnd("AStarMarkGoal");
			return 1;
		} else {
			ProfileEnd("AStarMarkGoal");
			return 0;
		}
	}

	bool goal_reachable = false;

	gw = std::max(gw, 1);
	gh = std::max(gh, 1);

	AStarGoalMarker aStarGoalMarker(unit, &goal_reachable);
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

	ProfileEnd("AStarMarkGoal");
	return goal_reachable;
}

/**
**  Save the path
**
**  @return  The length of the path
*/
//Wyrmgus start
//static int AStarSavePath(const Vec2i &startPos, const Vec2i &endPos, char *path, int pathLen)
static int AStarSavePath(const Vec2i &startPos, const Vec2i &endPos, char *path, int pathLen, int z)
//Wyrmgus end
{
	ProfileBegin("AStarSavePath");

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
	if (path) {
		pathLen = std::min<int>(fullPathLength, pathLen);
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
			if (pathPos < pathLen) {
				path[pathLen - pathPos - 1] = direction;
			}
		}
	}

	ProfileEnd("AStarSavePath");
	return fullPathLength;
}

/**
**  Optimization to find a simple path
**  Check if we're at the goal or if it's 1 tile away
*/
static int AStarFindSimplePath(const Vec2i &startPos, const Vec2i &goal, int gw, int gh,
							   int, int, int minrange, int maxrange,
							   //Wyrmgus start
//							   char *path, const CUnit &unit)
							   char *path, const CUnit &unit, int z, bool allow_diagonal)
							   //Wyrmgus end
{
	ProfileBegin("AStarFindSimplePath");
	// At exact destination point already
	if (goal == startPos && minrange == 0) {
		ProfileEnd("AStarFindSimplePath");
		return PF_REACHED;
	}

	// Don't allow unit inside destination area
	if (goal.x <= startPos.x && startPos.x <= goal.x + gw - 1
		&& goal.y <= startPos.y && startPos.y <= goal.y + gh - 1) {
		//Wyrmgus start
		ProfileEnd("AStarFindSimplePath"); // seems like this should be here
		//Wyrmgus end
		return PF_FAILED;
	}

	const Vec2i diff = goal - startPos;
	const int distance = isqrt(square(diff.x) + square(diff.y));

	// Within range of destination
	if (minrange <= distance && distance <= maxrange) {
		ProfileEnd("AStarFindSimplePath");
		return PF_REACHED;
	}

	//Wyrmgus start
//	if (MyAbs(diff.x) <= 1 && MyAbs(diff.y) <= 1) {
	if (minrange <= distance && MyAbs(diff.x) <= 1 && MyAbs(diff.y) <= 1 && (allow_diagonal || diff.x == 0 || diff.y == 0)) {
	//Wyrmgus end
		// Move to adjacent cell
		//Wyrmgus start
//		if (CostMoveTo(GetIndex(goal.x, goal.y), unit) == -1) {
		if (CostMoveTo(GetIndex(goal.x, goal.y, z), unit, z) == -1) {
		//Wyrmgus end
			ProfileEnd("AStarFindSimplePath");
			return PF_UNREACHABLE;
		}

		if (path) {
			path[0] = XY2Heading[diff.x + 1][diff.y + 1];
		}
		ProfileEnd("AStarFindSimplePath");
		return 1;
	}

	ProfileEnd("AStarFindSimplePath");
	return PF_FAILED;
}

/**
**  Find path.
*/
int AStarFindPath(const Vec2i &startPos, const Vec2i &goalPos, int gw, int gh,
				  int tilesizex, int tilesizey, int minrange, int maxrange,
				  //Wyrmgus start
//				  char *path, int pathlen, const CUnit &unit)
				  char *path, int pathlen, const CUnit &unit, int max_length, int z, bool allow_diagonal)
				  //Wyrmgus end
{
	Assert(CMap::Map.Info.IsPointOnMap(startPos, z));
	
	//Wyrmgus start
	if (unit.MapLayer->ID != z) {
		return PF_UNREACHABLE;
	}
	
	allow_diagonal = allow_diagonal && !unit.Type->BoolFlag[RAIL_INDEX].value; //rail units cannot move diagonally
	//Wyrmgus end

	ProfileBegin("AStarFindPath");

	AStarGoalX = goalPos.x;
	AStarGoalY = goalPos.y;

	//  Check for simple cases first
	int ret = AStarFindSimplePath(startPos, goalPos, gw, gh, tilesizex, tilesizey,
								  //Wyrmgus start
//								  minrange, maxrange, path, unit);
								  minrange, maxrange, path, unit, z, allow_diagonal);
								  //Wyrmgus end
	if (ret != PF_FAILED) {
		ProfileEnd("AStarFindPath");
		return ret;
	}

	//  Initialize
	//Wyrmgus start
//	AStarCleanUp();
//	CostMoveToCacheCleanUp();
	AStarCleanUp(z);
	CostMoveToCacheCleanUp(z);
	//Wyrmgus end

	//Wyrmgus start
//	OpenSetSize = 0;
//	CloseSetSize = 0;
	OpenSetSize[z] = 0;
	CloseSetSize[z] = 0;
	//Wyrmgus end

	//Wyrmgus start
//	if (!AStarMarkGoal(goalPos, gw, gh, tilesizex, tilesizey, minrange, maxrange, unit)) {
	if (!AStarMarkGoal(goalPos, gw, gh, tilesizex, tilesizey, minrange, maxrange, unit, z)) {
	//Wyrmgus end
		// goal is not reachable
		ret = PF_UNREACHABLE;
		ProfileEnd("AStarFindPath");
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
		ProfileEnd("AStarFindPath");
		return ret;
	}
	//Wyrmgus start
//	AStarAddToClose(OpenSet[0].O);
//	if (AStarMatrix[eo].InGoal) {
	AStarAddToClose(OpenSet[z][0].O, z);
	if (AStarMatrix[z][eo].InGoal) {
	//Wyrmgus end
		ret = PF_REACHED;
		ProfileEnd("AStarFindPath");
		return ret;
	}
	Vec2i endPos;
	
	//Wyrmgus start
	int length = 0;
	//Wyrmgus end

	//  Begin search
	while (1) {
		//Wyrmgus start
		if (max_length != 0 && length > max_length) {
			ret = PF_FAILED;
			ProfileEnd("AStarFindPath");
			return ret;
		}
		//Wyrmgus end
		
		// Find the best node of from the open set
		//Wyrmgus start
//		const int shortest = AStarFindMinimum();
//		const int x = OpenSet[shortest].pos.x;
//		const int y = OpenSet[shortest].pos.y;
//		const int o = OpenSet[shortest].O;
		const int shortest = AStarFindMinimum(z);
		const int x = OpenSet[z][shortest].pos.x;
		const int y = OpenSet[z][shortest].pos.y;
		const int o = OpenSet[z][shortest].O;
		//Wyrmgus end

		//Wyrmgus start
//		AStarRemoveMinimum(shortest);
		AStarRemoveMinimum(shortest, z);
		//Wyrmgus end

		// If we have reached the goal, then exit.
		//Wyrmgus start
//		if (AStarMatrix[o].InGoal == 1) {
		if (AStarMatrix[z][o].InGoal == 1) {
		//Wyrmgus end
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
		//Wyrmgus start
//		const int px = x - Heading2X[(int)AStarMatrix[o].Direction];
//		const int py = y - Heading2Y[(int)AStarMatrix[o].Direction];
		const int px = x - Heading2X[(int)AStarMatrix[z][o].Direction];
		const int py = y - Heading2Y[(int)AStarMatrix[z][o].Direction];
		//Wyrmgus end

		for (int i = 0; i < 8; ++i) {
			//Wyrmgus start
			if (!allow_diagonal && Heading2X[i] != 0 && Heading2Y[i] != 0) { //can't move diagonally
				continue;
			}
			//Wyrmgus end
			
			endPos.x = x + Heading2X[i];
			endPos.y = y + Heading2Y[i];

			// Don't check the tile we came from, it's not going to be better
			// Should reduce load on A*

			if (endPos.x == px && endPos.y == py) {
				continue;
			}

			// Outside the map or can't be entered.
			//Wyrmgus start
//			if (endPos.x < 0 || endPos.x + tilesizex - 1 >= AStarMapWidth
			if (endPos.x < 0 || endPos.x + tilesizex - 1 >= AStarMapWidth[z]
			//Wyrmgus end
				//Wyrmgus start
//				|| endPos.y < 0 || endPos.y + tilesizey - 1 >= AStarMapHeight) {
				|| endPos.y < 0 || endPos.y + tilesizey - 1 >= AStarMapHeight[z]
				|| !CMap::Map.Info.IsPointOnMap(endPos, z)) {
				//Wyrmgus end
				continue;
			}

			//eo = GetIndex(ex, ey);
			//Wyrmgus start
//			eo = endPos.x + (o - x) + Heading2O[i];
			eo = endPos.x + (o - x) + Heading2O[i][z];
			//Wyrmgus end

			// if the point is "move to"-able and
			// if we have not reached this point before,
			// or if we have a better path to it, we add it to open set
			//Wyrmgus start
//			int new_cost = CostMoveTo(eo, unit);
			int new_cost = CostMoveTo(eo, unit, z);
			//Wyrmgus end
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
				//Wyrmgus start
//				AStarMatrix[eo].CostFromStart = new_cost;
//				AStarMatrix[eo].Direction = i;
				AStarMatrix[z][eo].CostFromStart = new_cost;
				AStarMatrix[z][eo].Direction = i;
				//Wyrmgus end
				costToGoal = AStarCosts(endPos, goalPos);
				//Wyrmgus start
//				AStarMatrix[eo].CostToGoal = costToGoal;
//				if (AStarAddNode(endPos, eo, AStarMatrix[eo].CostFromStart + costToGoal) == PF_FAILED) {
				AStarMatrix[z][eo].CostToGoal = costToGoal;
				if (AStarAddNode(endPos, eo, AStarMatrix[z][eo].CostFromStart + costToGoal, z) == PF_FAILED) {
				//Wyrmgus end
					ret = PF_FAILED;
					ProfileEnd("AStarFindPath");
					return ret;
				}
				// we add the point to the close set
				//Wyrmgus start
//				AStarAddToClose(eo);
				AStarAddToClose(eo, z);
				//Wyrmgus end
			//Wyrmgus start
//			} else if (new_cost < AStarMatrix[eo].CostFromStart) {
			} else if (new_cost < AStarMatrix[z][eo].CostFromStart) {
			//Wyrmgus end
				// Already visited node, but we have here a better path
				// I know, it's redundant (but simpler like this)
				//Wyrmgus start
//				AStarMatrix[eo].CostFromStart = new_cost;
//				AStarMatrix[eo].Direction = i;
				AStarMatrix[z][eo].CostFromStart = new_cost;
				AStarMatrix[z][eo].Direction = i;
				//Wyrmgus end
				// this point might be already in the OpenSet
				//Wyrmgus start
//				const int j = AStarFindNode(eo);
				const int j = AStarFindNode(eo, z);
				//Wyrmgus end
				if (j == -1) {
					costToGoal = AStarCosts(endPos, goalPos);
					//Wyrmgus start
//					AStarMatrix[eo].CostToGoal = costToGoal;
//					if (AStarAddNode(endPos, eo, AStarMatrix[eo].CostFromStart + costToGoal) == PF_FAILED) {
					AStarMatrix[z][eo].CostToGoal = costToGoal;
					if (AStarAddNode(endPos, eo, AStarMatrix[z][eo].CostFromStart + costToGoal, z) == PF_FAILED) {
					//Wyrmgus end
						ret = PF_FAILED;
						ProfileEnd("AStarFindPath");
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
		//Wyrmgus start
//		if (OpenSetSize <= 0) { // no new nodes generated
		if (OpenSetSize[z] <= 0) { // no new nodes generated
		//Wyrmgus end
			ret = PF_UNREACHABLE;
			ProfileEnd("AStarFindPath");
			return ret;
		}
		
		//Wyrmgus start
		length += 1;
		//Wyrmgus end
	}

	//Wyrmgus start
//	const int path_length = AStarSavePath(startPos, endPos, path, pathlen);
	const int path_length = AStarSavePath(startPos, endPos, path, pathlen, z);
	//Wyrmgus end

	ret = path_length;

	ProfileEnd("AStarFindPath");
	return ret;
}

struct StatsNode {
	int Direction = 0;
	int InGoal = 0;
	int CostFromStart = 0;
	int Costs = 0;
	int CostToGoal = 0;
};

//Wyrmgus start
//StatsNode *AStarGetStats()
StatsNode *AStarGetStats(int z)
//Wyrmgus end
{
	//Wyrmgus start
//	StatsNode *stats = new StatsNode[AStarMapWidth * AStarMapHeight];
	StatsNode *stats = new StatsNode[AStarMapWidth[z] * AStarMapHeight[z]];
	//Wyrmgus end
	StatsNode *s = stats;
	//Wyrmgus start
//	AStarNode *m = AStarMatrix;
	AStarNode *m = AStarMatrix[z];
	//Wyrmgus end

	//Wyrmgus start
//	for (int j = 0; j < AStarMapHeight; ++j) {
	for (int j = 0; j < AStarMapHeight[z]; ++j) {
	//Wyrmgus end
		//Wyrmgus start
//		for (int i = 0; i < AStarMapWidth; ++i) {
		for (int i = 0; i < AStarMapWidth[z]; ++i) {
		//Wyrmgus end
			s->Direction = m->Direction;
			s->InGoal = m->InGoal;
			s->CostFromStart = m->CostFromStart;
			s->CostToGoal = m->CostToGoal;
			++s;
			++m;
		}
	}

	//Wyrmgus start
//	for (int i = 0; i < OpenSetSize; ++i) {
	for (int i = 0; i < OpenSetSize[z]; ++i) {
	//Wyrmgus end
		//Wyrmgus start
//		stats[OpenSet[i].O].Costs = OpenSet[i].Costs;
		stats[OpenSet[z][i].O].Costs = OpenSet[z][i].Costs;
		//Wyrmgus end
	}
	return stats;
}

void AStarFreeStats(StatsNode *stats)
{
	delete[] stats;
}

/*----------------------------------------------------------------------------
--  Configurable costs
----------------------------------------------------------------------------*/

// AStarFixedUnitCrossingCost
void SetAStarFixedUnitCrossingCost(int cost)
{
	if (cost <= 3) {
		fprintf(stderr, "AStarFixedUnitCrossingCost must be greater than 3\n");
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
		fprintf(stderr, "AStarMovingUnitCrossingCost must be greater than 3\n");
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
		fprintf(stderr, "AStarUnknownTerrainCost must be non-negative\n");
		return;
	}
	AStarUnknownTerrainCost = cost;
}
int GetAStarUnknownTerrainCost()
{
	return AStarUnknownTerrainCost;
}
