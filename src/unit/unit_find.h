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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon, Joris Dauphin
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

#pragma once

#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "pathfinder.h"
#include "unit/unit.h"
#include "unit/unit_cache.h"
#include "unit/unit_type.h"
#include "util/fractional_int.h"

class CPlayer;
class CUnit;
enum class UnitTypeType;

namespace wyrmgus {
	class resource;
	enum class tile_flag : uint32_t;
}

//  Some predicates

class CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const
	{
		Q_UNUSED(unit)

		return true;
	}
};

class NoFilter : public CUnitFilter
{
public:
	bool operator()(const CUnit *) const
	{
		return true;
	}
};

class HasSameTypeAs : public CUnitFilter
{
public:
	explicit HasSameTypeAs(const wyrmgus::unit_type &_type) : type(&_type) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->Type == type;
	}	
private:
	const wyrmgus::unit_type *type;
};

class HasSamePlayerAs : public CUnitFilter
{
public:
	explicit HasSamePlayerAs(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->Player == player;
	}
private:
	const CPlayer *player;
};

class HasNotSamePlayerAs : public CUnitFilter
{
public:
	explicit HasNotSamePlayerAs(const CPlayer &_player) : player(&_player) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->Player != player;
	}
private:
	const CPlayer *player;
};

class IsAlliedWith final : public CUnitFilter
{
public:
	explicit IsAlliedWith(const CPlayer &_player) : player(&_player)
	{
	}

	bool operator()(const CUnit *unit) const
	{
		return unit->is_allied_with(*player);
	}

private:
	const CPlayer *player = nullptr;
};

class IsEnemyWithPlayer final : public CUnitFilter
{
public:
	explicit IsEnemyWithPlayer(const CPlayer &_player) : player(&_player)
	{
	}

	bool operator()(const CUnit *unit) const
	{
		return unit->is_enemy_of(*player);
	}

private:
	const CPlayer *player = nullptr;
};

class IsEnemyWithUnit final : public CUnitFilter
{
public:
	explicit IsEnemyWithUnit(const CUnit *unit) : unit(unit)
	{
	}

	bool operator()(const CUnit *unit) const
	{
		return unit->is_enemy_of(*this->unit);
	}
private:
	const CUnit *unit = nullptr;
};

class HasSamePlayerAndTypeAs : public CUnitFilter
{
public:
	explicit HasSamePlayerAndTypeAs(const CUnit &unit) :
		player(unit.Player), type(unit.Type)
	{}
	
	HasSamePlayerAndTypeAs(const CPlayer &_player, const wyrmgus::unit_type &_type) :
		player(&_player), type(&_type)
	{}

	bool operator()(const CUnit *unit) const
	{
		return (unit->Player == player && unit->Type == type);
	}

private:
	const CPlayer *player;
	const wyrmgus::unit_type *type;
};

class IsNotTheSameUnitAs : public CUnitFilter
{
public:
	explicit IsNotTheSameUnitAs(const CUnit &unit) : forbidden(&unit) {}
	bool operator()(const CUnit *unit) const
	{
		return unit != forbidden;
	}
private:
	const CUnit *forbidden;
};

class IsBuildingType : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const
	{
		return unit->Type->BoolFlag[BUILDING_INDEX].value;
	}
};

class IsNotBuildingType : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const
	{
		return !unit->Type->BoolFlag[BUILDING_INDEX].value;
	}
};

class IsOrganicType : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const
	{
		return unit->Type->BoolFlag[ORGANIC_INDEX].value;
	}
};

class IsBuiltUnit : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const;
};

class IsAggresiveUnit : public CUnitFilter
{
public:
	bool operator()(const CUnit *unit) const
	{
		return unit->IsAgressive();
	}
};

class OutOfMinRange : public CUnitFilter
{
public:
	explicit OutOfMinRange(const int range, const Vec2i pos, int z) : range(range), pos(pos), z(z) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->MapDistanceTo(pos, z) >= range;
	}
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
class CUnitTypeFinder final
{
public:
	explicit CUnitTypeFinder(const UnitTypeType t) : unit_type_type_set({ t })
	{
	}

	explicit CUnitTypeFinder(const std::set<UnitTypeType> &set) : unit_type_type_set(set)
	{
	}

	bool operator()(const CUnit *const unit) const
	{
		if (!unit) {
			fprintf(stderr, "CUnitTypeFinder Error: Unit is null.\n");
			return false;
		}

		if (!unit->Type) {
			fprintf(stderr, "CUnitTypeFinder Error: Unit's type is null.\n");
			return false;
		}

		const wyrmgus::unit_type &type = *unit->Type;
		if (type.BoolFlag[VANISHES_INDEX].value || (!unit_type_type_set.empty() && !unit_type_type_set.contains(type.UnitType))) {
			return false;
		}
		return true;
	}

	bool operator()(const std::shared_ptr<wyrmgus::unit_ref> &unit_ref);

private:
	const std::set<UnitTypeType> unit_type_type_set;
};

class UnitFinder final
{
public:
	explicit UnitFinder(const CPlayer &player, const std::vector<CUnit *> &units, const int maxDist, const tile_flag movemask, CUnit **unitP, const int z)
		: player(player), units(units), maxDist(maxDist), movemask(movemask), unitP(unitP), z(z) 
	{
	}

	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);

private:
	CUnit *FindUnitAtPos(const Vec2i &pos) const;

private:
	const CPlayer &player;
	const std::vector<CUnit *> &units;
	int maxDist = 0;
	tile_flag movemask;
	CUnit **unitP = nullptr;
	int z = 0;
};

template <bool circle, typename Pred>
//Wyrmgus start
//inline void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, Pred pred)
inline void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, const int z, Pred pred)
//Wyrmgus end
{
	Assert(CMap::get()->Info->IsPointOnMap(ltPos, z));
	Assert(CMap::get()->Info->IsPointOnMap(rbPos, z));
	Assert(units.empty());
	
	decimillesimal_int middle_x;
	decimillesimal_int middle_y;
	decimillesimal_int radius;

	if constexpr (circle) {
		middle_x = decimillesimal_int(rbPos.x + ltPos.x) / 2;
		middle_y = decimillesimal_int(rbPos.y + ltPos.y) / 2;
		radius = ((middle_x - ltPos.x) + (middle_y - ltPos.y)) / 2;
	}

	for (Vec2i posIt = ltPos; posIt.y != rbPos.y + 1; ++posIt.y) {
		for (posIt.x = ltPos.x; posIt.x != rbPos.x + 1; ++posIt.x) {
			if constexpr (circle) {
				const decimillesimal_int rel_x = posIt.x - middle_x;
				const decimillesimal_int rel_y = posIt.y - middle_y;
				const decimillesimal_int my = radius * radius - rel_x * rel_x;
				if ((rel_y * rel_y) > my) {
					continue;
				}
			}

			const CUnitCache &cache = CMap::get()->get_tile_unit_cache(posIt, z);

			for (CUnit *unit : cache) {
				if (unit->CacheLock == 0 && pred(unit)) {
					unit->CacheLock = 1;
					units.push_back(unit);
				}
			}
		}
	}
	for (size_t i = 0; i != units.size(); ++i) {
		units[i]->CacheLock = 0;
	}
}

template <bool circle = false, typename Pred>
inline void SelectAroundUnit(const CUnit &unit, const int range, std::vector<CUnit *> &around, Pred pred)
{
	const Vec2i offset(range, range);
	const CUnit *firstContainer = unit.GetFirstContainer();
	const Vec2i typeSize(firstContainer->Type->get_tile_size() - QSize(1, 1));

	Select<circle>(unit.tilePos - offset,
		   unit.tilePos + typeSize + offset, around,
		   //Wyrmgus start
		   unit.MapLayer->ID,
		   //Wyrmgus end
		   MakeAndPredicate(IsNotTheSameUnitAs(unit), pred));
}

template <bool circle = false, typename Pred>
//Wyrmgus start
//inline void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, Pred pred)
inline void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, const int z, Pred pred)
//Wyrmgus end
{
	Vec2i minPos = ltPos;
	Vec2i maxPos = rbPos;

	//Wyrmgus start
//	CMap::get()->FixSelectionArea(minPos, maxPos);
//	SelectFixed(minPos, maxPos, units, pred);
	CMap::get()->FixSelectionArea(minPos, maxPos, z);
	SelectFixed<circle>(minPos, maxPos, units, z, pred);
	//Wyrmgus end
}

template <bool circle = false>
inline void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, const int z)
{
	//Wyrmgus start
//	Select<circle>(ltPos, rbPos, units, NoFilter());
	Select<circle>(ltPos, rbPos, units, z, NoFilter());
	//Wyrmgus end
}

inline void Select(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, const int z, const bool circle)
{
	if (circle) {
		Select<true>(ltPos, rbPos, units, z, NoFilter());
	} else {
		Select<false>(ltPos, rbPos, units, z, NoFilter());
	}
}

template <bool circle = false>
inline void SelectFixed(const Vec2i &ltPos, const Vec2i &rbPos, std::vector<CUnit *> &units, const int z)
{
	//Wyrmgus start
//	Select<circle>(ltPos, rbPos, units, NoFilter());
	Select<circle>(ltPos, rbPos, units, z, NoFilter());
	//Wyrmgus end
}

template <bool circle = false>
inline void SelectAroundUnit(const CUnit &unit, const int range, std::vector<CUnit *> &around)
{
	SelectAroundUnit<circle>(unit, range, around, NoFilter());
}

template <typename Pred>
CUnit *FindUnit_IfFixed(const Vec2i &ltPos, const Vec2i &rbPos, int z, Pred pred)
{
	Assert(CMap::get()->Info->IsPointOnMap(ltPos, z));
	Assert(CMap::get()->Info->IsPointOnMap(rbPos, z));

	for (Vec2i posIt = ltPos; posIt.y != rbPos.y + 1; ++posIt.y) {
		for (posIt.x = ltPos.x; posIt.x != rbPos.x + 1; ++posIt.x) {
			const CUnitCache &cache = CMap::get()->get_tile_unit_cache(posIt, z);

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

	CMap::get()->FixSelectionArea(minPos, maxPos, z);
	return FindUnit_IfFixed(minPos, maxPos, z, pred);
}

/// Find resource
extern CUnit *UnitFindResource(const CUnit &unit, const CUnit &start_unit, int range,
								//Wyrmgus Start
//							   const resource *resource, bool check_usage = false, const CUnit *deposit = nullptr);
							   const resource *resource, bool check_usage = false, const CUnit *deposit = nullptr, bool only_harvestable = true, bool ignore_exploration = false, bool only_unsettled_area = false, bool include_luxury = false, bool only_same = false);
								//Wyrmgus end

/// Find nearest deposit
extern CUnit *FindDeposit(const CUnit &unit, const int range, const resource *resource);
//Wyrmgus start
/// Find nearest home market
extern CUnit *FindHomeMarket(const CUnit &unit, int range);
//Wyrmgus end
/// Find the next idle worker
extern CUnit *FindIdleWorker(const CPlayer &player, const CUnit *last);

/// Find the neareast piece of terrain with specific flags.
extern bool FindTerrainType(const tile_flag movemask, const wyrmgus::resource *resource, int range,
							const CPlayer &player, const Vec2i &startPos, Vec2i *pos, int z, const landmass *landmass = nullptr);

extern void FindUnitsByType(const wyrmgus::unit_type &type, std::vector<CUnit *> &units, bool everybody = false);

/// Find all units of this type of the player
extern void FindPlayerUnitsByType(const CPlayer &player, const wyrmgus::unit_type &type, std::vector<CUnit *> &units, bool ai_active_only = false);

/// Return any unit on that map tile
extern CUnit *UnitOnMapTile(const Vec2i &pos, const std::set<UnitTypeType> &type_set, int z);// = -1);
/// Return possible attack target on that map area
extern CUnit *TargetOnMap(const CUnit &unit, const Vec2i &pos1, const Vec2i &pos2, int z);

/// Return resource, if on map tile
//Wyrmgus start
//extern CUnit *ResourceOnMap(const Vec2i &pos, const resource *resource, bool mine_on_top = true);
extern CUnit *ResourceOnMap(const Vec2i &pos, const resource *resource, const int z, const bool only_harvestable = true, const bool only_same = true);
//Wyrmgus end
/// Return resource deposit, if on map tile
//Wyrmgus start
//extern CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource);
extern CUnit *ResourceDepositOnMap(const Vec2i &pos, int resource, int z);
//Wyrmgus end

/// Check map for obstacles in a line between 2 tiles
//Wyrmgus start
//extern bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, const tile_flag flags, int *distance = nullptr);
extern bool CheckObstaclesBetweenTiles(const Vec2i &unitPos, const Vec2i &goalPos, const tile_flag flags, const int z, int max_difference = 0, int *distance = nullptr);
//Wyrmgus end
/// Find best enemy in numeric range to attack
//Wyrmgus start
//extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range, CUnitFilter pred);
//extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range);
template <bool circle = false>
extern CUnit *AttackUnitsInDistance(const CUnit &unit, int range, CUnitFilter pred, bool include_neutral = false);

extern template CUnit *AttackUnitsInDistance<false>(const CUnit &unit, int range, CUnitFilter pred, bool include_neutral);
extern template CUnit *AttackUnitsInDistance<true>(const CUnit &unit, int range, CUnitFilter pred, bool include_neutral);

template <bool circle>
extern CUnit *AttackUnitsInDistance(const CUnit &unit, const int range, const bool include_neutral = false);

extern template CUnit *AttackUnitsInDistance<false>(const CUnit &unit, const int range, const bool include_neutral);
extern template CUnit *AttackUnitsInDistance<true>(const CUnit &unit, const int range, const bool include_neutral);
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

extern bool CheckPathwayConnection(const CUnit &src_unit, const CUnit &dst_unit, const tile_flag flags);
//Wyrmgus end
