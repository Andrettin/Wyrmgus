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
/**@name unit_find.h - The unit find header file. */
//
//      (c) Copyright 1998-2018 by Lutz Sammer, Jimmy Salmon, Joris Dauphin
//		and Andrettin
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

#ifndef __UNIT_FIND_H__
#define __UNIT_FIND_H__

//@{

#include "map/map.h"
#include "map/map_layer.h"
#include "pathfinder.h"
#include "unit/unit.h"
#include "unit/unittype.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CPlayer;
class CUnit;

//
//  Some predicates
//

class CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class NoFilter : public CUnitFilter
{
public:
	bool operator()(const CUnit *) const;
};

class HasSameTypeAs : public CUnitFilter
{
public:
	explicit HasSameTypeAs(const CUnitType &_type) : type(&_type) {}
	bool operator()(const CUnit *unit) const;
private:
	const CUnitType *type;
};

class HasSamePlayerAs : public CUnitFilter
{
public:
	explicit HasSamePlayerAs(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const;
private:
	const CPlayer *player;
};

class HasNotSamePlayerAs : public CUnitFilter
{
public:
	explicit HasNotSamePlayerAs(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const;
private:
	const CPlayer *player;
};

class IsAlliedWith : public CUnitFilter
{
public:
	explicit IsAlliedWith(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const;
private:
	const CPlayer *player;
};

class IsEnemyWith : public CUnitFilter
{
public:
	explicit IsEnemyWith(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const;
private:
	const CPlayer *player;
};

class HasSamePlayerAndTypeAs : public CUnitFilter
{
public:
	explicit HasSamePlayerAndTypeAs(const CUnit &unit);
	HasSamePlayerAndTypeAs(const CPlayer &_player, const CUnitType &_type) :
		player(&_player), type(&_type)
	{}

	bool operator()(const CUnit *unit) const;

private:
	const CPlayer *player;
	const CUnitType *type;
};

class IsNotTheSameUnitAs : public CUnitFilter
{
public:
	explicit IsNotTheSameUnitAs(const CUnit &unit) : forbidden(&unit) {}
	bool operator()(const CUnit *unit) const;
private:
	const CUnit *forbidden;
};

class IsBuildingType : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class IsNotBuildingType : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class IsOrganicType : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class IsBuiltUnit : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class IsAggresiveUnit : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class OutOfMinRange : public CUnitFilter
{
public:
	explicit OutOfMinRange(const int range, const Vec2i pos, int z) : range(range), pos(pos), z(z) {}
	bool operator()(const CUnit *unit) const;
private:
	int range;
	Vec2i pos;
	int z;
};

template <typename Pred>
class NotPredicate : public CUnitFilter
{
public:
	explicit NotPredicate(Pred _pred) : pred(_pred) {}
	bool operator()(const CUnit *unit) const { return pred(unit) == false; }
private:
	Pred pred;
};

template <typename Pred>
NotPredicate<Pred> MakeNotPredicate(Pred pred) { return NotPredicate<Pred>(pred); }

template <typename Pred1, typename Pred2>
class AndPredicate : public CUnitFilter
{
public:
	AndPredicate(Pred1 _pred1, Pred2 _pred2) : pred1(_pred1), pred2(_pred2) {}
	bool operator()(const CUnit *unit) const { return pred1(unit) && pred2(unit); }
private:
	Pred1 pred1;
	Pred2 pred2;
};

template <typename Pred1, typename Pred2>
AndPredicate<Pred1, Pred2> MakeAndPredicate(Pred1 pred1, Pred2 pred2) { return AndPredicate<Pred1, Pred2>(pred1, pred2); }

//Wyrmgus start
template <typename Pred1, typename Pred2>
class OrPredicate : public CUnitFilter
{
public:
	OrPredicate(Pred1 _pred1, Pred2 _pred2) : pred1(_pred1), pred2(_pred2) {}
	bool operator()(const CUnit *unit) const { return pred1(unit) || pred2(unit); }
private:
	Pred1 pred1;
	Pred2 pred2;
};

template <typename Pred1, typename Pred2>
OrPredicate<Pred1, Pred2> MakeOrPredicate(Pred1 pred1, Pred2 pred2) { return OrPredicate<Pred1, Pred2>(pred1, pred2); }
//Wyrmgus end

//unit_find
class CUnitTypeFinder
{
public:
	explicit CUnitTypeFinder(const UnitTypeType t) : unitTypeType(t) {}
	bool operator()(const CUnit *const unit) const;

private:
	const UnitTypeType unitTypeType;
};


class UnitFinder
{
public:
	UnitFinder(const CPlayer &player, const std::vector<CUnit *> &units, int maxDist, int movemask, CUnit **unitP, int z) :
		player(player), units(units), maxDist(maxDist), movemask(movemask), unitP(unitP), z(z) {}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	CUnit *FindUnitAtPos(const Vec2i &pos) const;
private:
	const CPlayer &player;
	const std::vector<CUnit *> &units;
	int maxDist;
	int movemask;
	CUnit **unitP;
	int z;
};

//Wyrmgus start
//void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units);
//void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units);
//void SelectAroundUnit(const CUnit &unit, int range, std::vector<CUnit *> &around);
void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, int z, bool circle = false);
void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, int z, bool circle = false);
void SelectAroundUnit(const CUnit &unit, int range, std::vector<CUnit *> &around, bool circle = false);
//Wyrmgus end

template <typename Pred>
//Wyrmgus start
//void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, Pred pred)
void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, int z, Pred pred, bool circle = false)
//Wyrmgus end
{
	Assert(Map.Info.IsPointOnMap(ltPos, z));
	Assert(Map.Info.IsPointOnMap(rbPos, z));
	Assert(units.empty());
	
	//Wyrmgus start
	double middle_x;
	double middle_y;
	double radius;
	if (circle) {
		middle_x = (rbPos.x + ltPos.x) / 2;
		middle_y = (rbPos.y + ltPos.y) / 2;
		radius = ((middle_x - ltPos.x) + (middle_y - ltPos.y)) / 2;
	}
	//Wyrmgus end

	for (Vec2i posIt = ltPos; posIt.y != rbPos.y + 1; ++posIt.y) {
		for (posIt.x = ltPos.x; posIt.x != rbPos.x + 1; ++posIt.x) {
			//Wyrmgus start
			if (circle) {
				double rel_x = posIt.x - middle_x;
				double rel_y = posIt.y - middle_y;
				double my = radius * radius - rel_x * rel_x;
				if ((rel_y * rel_y) > my) {
					continue;
				}
			}
			//Wyrmgus end

			const CMapField &mf = *Map.Field(posIt, z);
			const CUnitCache &cache = mf.UnitCache;

			for (size_t i = 0; i != cache.size(); ++i) {
				CUnit &unit = *cache[i];

				if (unit.CacheLock == 0 && pred(&unit)) {
					unit.CacheLock = 1;
					units.push_back(&unit);
				}
			}
		}
	}
	for (size_t i = 0; i != units.size(); ++i) {
		units[i]->CacheLock = 0;
	}
}

template <typename Pred>
//Wyrmgus start
//void SelectAroundUnit(const CUnit &unit, int range, std::vector<CUnit *> &around, Pred pred)
void SelectAroundUnit(const CUnit &unit, int range, std::vector<CUnit *> &around, Pred pred, bool circle = false)
//Wyrmgus end
{
	const Vec2i offset(range, range);
	//Wyrmgus start
//	const Vec2i typeSize(unit.Type->TileSize.x - 1, unit.Type->TileSize.y - 1);
	const CUnit *firstContainer = unit.GetFirstContainer();
	const Vec2i typeSize(firstContainer->Type->TileSize - 1);
	//Wyrmgus end

	Select(unit.tilePos - offset,
		   unit.tilePos + typeSize + offset, around,
		   //Wyrmgus start
		   unit.MapLayer->ID,
//		   MakeAndPredicate(IsNotTheSameUnitAs(unit), pred));
		   MakeAndPredicate(IsNotTheSameUnitAs(unit), pred), circle);
		   //Wyrmgus end
}

template <typename Pred>
//Wyrmgus start
//void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, Pred pred)
void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, int z, Pred pred, bool circle = false)
//Wyrmgus end
{
	Vec2i minPos = ltPos;
	Vec2i maxPos = rbPos;

	//Wyrmgus start
//	Map.FixSelectionArea(minPos, maxPos);
//	SelectFixed(minPos, maxPos, units, pred);
	Map.FixSelectionArea(minPos, maxPos, z);
	SelectFixed(minPos, maxPos, units, z, pred, circle);
	//Wyrmgus end
}

template <typename Pred>
CUnit *FindUnit_IfFixed(const Vec2i &ltPos, const Vec2i &rbPos, int z, Pred pred)
{
	Assert(Map.Info.IsPointOnMap(ltPos, z));
	Assert(Map.Info.IsPointOnMap(rbPos, z));

	for (Vec2i posIt = ltPos; posIt.y != rbPos.y + 1; ++posIt.y) {
		for (posIt.x = ltPos.x; posIt.x != rbPos.x + 1; ++posIt.x) {
			const CMapField &mf = *Map.Field(posIt, z);
			const CUnitCache &cache = mf.UnitCache;

			CUnitCache::const_iterator it = std::find_if(cache.begin(), cache.end(), pred);
			if (it != cache.end()) {
				return *it;
			}
		}
	}
	return nullptr;
}

template <typename Pred>
CUnit *FindUnit_If(const Vec2i &ltPos, const Vec2i &rbPos, int z, Pred pred)
{
	Vec2i minPos = ltPos;
	Vec2i maxPos = rbPos;

	Map.FixSelectionArea(minPos, maxPos, z);
	return FindUnit_IfFixed(minPos, maxPos, z, pred);
}

/// Find resource
extern CUnit *UnitFindResource(const CUnit &unit, const CUnit &startUnit, int range,
								//Wyrmgus Start
//							   int resource, bool check_usage = false, const CUnit *deposit = nullptr);
							   int resource, bool check_usage = false, const CUnit *deposit = nullptr, bool only_harvestable = true, bool ignore_exploration = false, bool only_unsettled_area = false, bool include_luxury = false, bool only_same = false, bool check_reachable = true, bool from_outside_container = false);
								//Wyrmgus end

/// Find nearest deposit
extern CUnit *FindDeposit(const CUnit &unit, int range, int resource);
//Wyrmgus start
/// Find nearest home market
extern CUnit *FindHomeMarket(const CUnit &unit, int range);
//Wyrmgus end
/// Find the next idle worker
extern CUnit *FindIdleWorker(const CPlayer &player, const CUnit *last);

/// Find the neareast piece of terrain with specific flags.
//Wyrmgus start
//extern bool FindTerrainType(int movemask, int resmask, int range,
extern bool FindTerrainType(int movemask, int resource, int range,
//Wyrmgus end
							//Wyrmgus start
//							const CPlayer &player, const Vec2i &startPos, Vec2i *pos);
							const CPlayer &player, const Vec2i &startPos, Vec2i *pos, int z, int landmass = 0);
							//Wyrmgus end

extern void FindUnitsByType(const CUnitType &type, std::vector<CUnit *> &units, bool everybody = false);

/// Find all units of this type of the player
extern void FindPlayerUnitsByType(const CPlayer &player, const CUnitType &type, std::vector<CUnit *> &units, bool ai_active = false);
/// Return any unit on that map tile
//Wyrmgus start
//extern CUnit *UnitOnMapTile(const Vec2i &pos, unsigned int type);// = -1);
extern CUnit *UnitOnMapTile(const Vec2i &pos, unsigned int type, int z);// = -1);
//Wyrmgus end
/// Return possible attack target on that map area
extern CUnit *TargetOnMap(const CUnit &unit, const Vec2i &pos1, const Vec2i &pos2, int z);

/// Return resource, if on map tile
//Wyrmgus start
//extern CUnit *ResourceOnMap(const Vec2i &pos, int resource, bool mine_on_top = true);
extern CUnit *ResourceOnMap(const Vec2i &pos, int resource, int z, bool only_harvestable = true, bool only_same = true);
//Wyrmgus end
/// Return resource deposit, if on map tile
//Wyrmgus start
//extern CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource);
extern CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource, int z);
//Wyrmgus end

/// Check map for obstacles in a line between 2 tiles
//Wyrmgus start
//extern bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, unsigned short flags, int *distance = nullptr);
extern bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, unsigned long flags, int z, int max_difference = 0, int *distance = nullptr, int player = -1);
//Wyrmgus end
/// Find best enemy in numeric range to attack
//Wyrmgus start
//extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range, CUnitFilter pred);
//extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range);
extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range, CUnitFilter pred, bool circle = false, bool include_neutral = false);
extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range, bool circle = false, bool include_neutral = false);
//Wyrmgus end
/// Find best enemy in attack range to attack
extern CUnit *AttackUnitsInRange(const CUnit &unit, CUnitFilter pred);
extern CUnit *AttackUnitsInRange(const CUnit &unit);
/// Find best enemy in reaction range to attack
//Wyrmgus start
//extern CUnit *AttackUnitsInReactRange(const CUnit &unit, CUnitFilter pred);
//extern CUnit *AttackUnitsInReactRange(const CUnit &unit);
extern CUnit *AttackUnitsInReactRange(const CUnit &unit, CUnitFilter pred, bool include_neutral = false);
extern CUnit *AttackUnitsInReactRange(const CUnit &unit, bool include_neutral = false);
extern bool CheckPathwayConnection(const CUnit &src_unit, const CUnit &dst_unit, unsigned int flags);
//Wyrmgus end



//@}

#endif // !__UNIT_FIND_H__
