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
/**@name build.cpp - The units. */
//
//      (c) Copyright 1998-2015 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "unittype.h"

#include "actions.h"
#include "map.h"
#include "player.h"
#include "unit.h"
#include "unit_find.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Find the building restriction that gives me this unit built on top
**  Allows you to define how the restriction is effecting the build
**
**  @param unit    the unit that is "OnTop"
**  @param parent  the parent unit if known. (guess otherwise)
**
**  @return        the BuildingRestrictionDetails
*/
CBuildRestrictionOnTop *OnTopDetails(const CUnit &unit, const CUnitType *parent)
{

	for (std::vector<CBuildRestriction *>::const_iterator i = unit.Type->BuildingRules.begin();
		 i != unit.Type->BuildingRules.end(); ++i) {
		CBuildRestrictionOnTop *ontopb = dynamic_cast<CBuildRestrictionOnTop *>(*i);

		if (ontopb) {
			if (!parent) {
				// Guess this is right
				return ontopb;
			}
			if (parent == ontopb->Parent) {
				return ontopb;
			}
			continue;
		}
		CBuildRestrictionAnd *andb = dynamic_cast<CBuildRestrictionAnd *>(*i);

		if (andb) {
			for (std::vector<CBuildRestriction *>::iterator j = andb->_or_list.begin(); j != andb->_or_list.end(); ++j) {
				CBuildRestrictionOnTop *ontopb = dynamic_cast<CBuildRestrictionOnTop *>(*j);
				if (ontopb) {
					if (!parent) {
						// Guess this is right
						return ontopb;
					}
					if (parent == ontopb->Parent) {
						return ontopb;
					}
				}
			}
		}
	}
	return NULL;
}

/**
**  Check And Restriction
*/
//Wyrmgus start
//bool CBuildRestrictionAnd::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const
bool CBuildRestrictionAnd::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const
//Wyrmgus end
{
	for (std::vector<CBuildRestriction *>::const_iterator i = _or_list.begin(); i != _or_list.end(); ++i) {
		//Wyrmgus start
//		if (!(*i)->Check(builder, type, pos, ontoptarget)) {
		if (!(*i)->Check(builder, type, pos, ontoptarget, z)) {
		//Wyrmgus end
			return false;
		}
	}
	return true;
}

/**
**  Check Distance Restriction
*/
//Wyrmgus start
//bool CBuildRestrictionDistance::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&) const
bool CBuildRestrictionDistance::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&, int z) const
//Wyrmgus end
{
	Vec2i pos1(0, 0);
	Vec2i pos2(0, 0);
	int distance = 0;
	CPlayer* player = builder != NULL ? builder->Player : ThisPlayer;

	if (this->DistanceType == LessThanEqual
		|| this->DistanceType == GreaterThan
		|| this->DistanceType == Equal
		|| this->DistanceType == NotEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance, 0);
		pos1.y = std::max<int>(pos.y - this->Distance, 0);
		//Wyrmgus start
//		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance, Map.Info.MapWidth);
//		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance, Map.Info.MapHeight);
		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance, Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance, Map.Info.MapHeights[z]);
		//Wyrmgus end
		distance = this->Distance;
	} else if (this->DistanceType == LessThan || this->DistanceType == GreaterThanEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance - 1, 0);
		pos1.y = std::max<int>(pos.y - this->Distance - 1, 0);
		//Wyrmgus start
//		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance + 1, Map.Info.MapWidth);
//		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance + 1, Map.Info.MapHeight);
		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance + 1, Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance + 1, Map.Info.MapHeights[z]);
		//Wyrmgus end
		distance = this->Distance - 1;
	}
	std::vector<CUnit *> table;
	//Wyrmgus start
//	Select(pos1, pos2, table);
	Select(pos1, pos2, table, z);
	//Wyrmgus end

	for (size_t i = 0; i != table.size(); ++i) {
		if ((builder != table[i] || this->CheckBuilder) &&
			// unit has RestrictType or no RestrictType was set, but a RestrictTypeOwner
			(this->RestrictType == table[i]->Type || (!this->RestrictType && this->RestrictTypeOwner.size() > 0)) &&
			// RestrictTypeOwner is not set or unit belongs to a suitable player
			(this->RestrictTypeOwner.size() == 0 ||
			 (!this->RestrictTypeOwner.compare("self") && player == table[i]->Player) ||
			 (!this->RestrictTypeOwner.compare("allied") && (player == table[i]->Player || player->IsAllied(*table[i]->Player))) ||
			 (!this->RestrictTypeOwner.compare("enemy") && player->IsEnemy(*table[i]->Player)))) {

			switch (this->DistanceType) {
				case GreaterThan :
				case GreaterThanEqual :
					//Wyrmgus start
//					if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) <= distance) {
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) <= distance) {
					//Wyrmgus end
						return Diagonal ? false : !(pos.x != table[i]->tilePos.x || pos.y != table[i]->tilePos.y);
					}
					break;
				case LessThan :
				case LessThanEqual :
					//Wyrmgus start
//					if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) <= distance) {
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) <= distance) {
					//Wyrmgus end
						return Diagonal || pos.x == table[i]->tilePos.x || pos.y == table[i]->tilePos.y;
					}
					break;
				case Equal :
					//Wyrmgus start
//					if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) == distance) {
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) == distance) {
					//Wyrmgus end
						return Diagonal || pos.x == table[i]->tilePos.x || pos.y == table[i]->tilePos.y;
					}
					break;
				case NotEqual :
					//Wyrmgus start
//					if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) == distance) {
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) == distance) {
					//Wyrmgus end
						return Diagonal ? false : !(pos.x != table[i]->tilePos.x || pos.y != table[i]->tilePos.y);
					}
					break;
			}
		}
	}
	return (this->DistanceType == GreaterThan ||
			this->DistanceType == GreaterThanEqual ||
			this->DistanceType == NotEqual);
}

/**
**  Check HasUnit Restriction
*/
//Wyrmgus start
//bool CBuildRestrictionHasUnit::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&) const
bool CBuildRestrictionHasUnit::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&, int z) const
//Wyrmgus end
{
	Vec2i pos1(0, 0);
	Vec2i pos2(0, 0);
	CPlayer* player = builder != NULL ? builder->Player : ThisPlayer;
	int count = 0;
	if (this->RestrictTypeOwner.size() == 0 || !this->RestrictTypeOwner.compare("self")) {
		count = player->GetUnitTotalCount(*this->RestrictType);
	} else if (!this->RestrictTypeOwner.compare("allied")) {
		count = player->GetUnitTotalCount(*this->RestrictType);
		for (int i = 0; i < NumPlayers; i++) {
			if (player->IsAllied(Players[i])) {
				count += Players[i].GetUnitTotalCount(*this->RestrictType);
			}
		}
	} else if (!this->RestrictTypeOwner.compare("enemy")) {
		for (int i = 0; i < NumPlayers; i++) {
			if (player->IsEnemy(Players[i])) {
				count += Players[i].GetUnitTotalCount(*this->RestrictType);
			}
		}
	} else if (!this->RestrictTypeOwner.compare("any")) {
		for (int i = 0; i < NumPlayers; i++) {
			count += Players[i].GetUnitTotalCount(*this->RestrictType);
		}
	}
	switch (this->CountType)
	{
	case LessThan: return count < this->Count;
	case LessThanEqual: return count <= this->Count;
	case Equal: return count == this->Count;
	case NotEqual: return count != this->Count;
	case GreaterThanEqual: return count >= this->Count;
	case GreaterThan: return count > this->Count;
	default: return false;
	}
}

/**
**  Check Surrounded By Restriction
*/
//Wyrmgus start
//bool CBuildRestrictionSurroundedBy::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&) const
bool CBuildRestrictionSurroundedBy::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&, int z) const
//Wyrmgus end
{
	Vec2i pos1(0, 0);
	Vec2i pos2(0, 0);
	int distance = 0;
	int count = 0;

	if (this->DistanceType == LessThanEqual
		|| this->DistanceType == GreaterThan
		|| this->DistanceType == Equal
		|| this->DistanceType == NotEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance, 0);
		pos1.y = std::max<int>(pos.y - this->Distance, 0);
		//Wyrmgus start
//		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance, Map.Info.MapWidth);
//		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance, Map.Info.MapHeight);
		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance, Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance, Map.Info.MapHeights[z]);
		//Wyrmgus end
		distance = this->Distance;
	}
	else if (this->DistanceType == LessThan || this->DistanceType == GreaterThanEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance - 1, 0);
		pos1.y = std::max<int>(pos.y - this->Distance - 1, 0);
		//Wyrmgus start
//		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance + 1, Map.Info.MapWidth);
//		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance + 1, Map.Info.MapHeight);
		pos2.x = std::min<int>(pos.x + type.TileWidth + this->Distance + 1, Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileHeight + this->Distance + 1, Map.Info.MapHeights[z]);
		//Wyrmgus end
		distance = this->Distance - 1;
	}
	std::vector<CUnit *> table;
	//Wyrmgus start
//	Select(pos1, pos2, table);
	Select(pos1, pos2, table, z);
	//Wyrmgus end

	for (size_t i = 0; i != table.size(); ++i) {
		if ((builder != table[i] || this->CheckBuilder) &&
			// unit has RestrictType or no RestrictType was set, but a RestrictTypeOwner
			(this->RestrictType == table[i]->Type || (!this->RestrictType && this->RestrictTypeOwner.size() > 0)) &&
			// RestrictTypeOwner is not set or unit belongs to a suitable player
			(this->RestrictTypeOwner.size() == 0 ||
				(!this->RestrictTypeOwner.compare("self") && builder->Player == table[i]->Player) ||
				(!this->RestrictTypeOwner.compare("allied") && (builder->Player == table[i]->Player || builder->Player->IsAllied(*table[i]->Player))) ||
				(!this->RestrictTypeOwner.compare("enemy") && builder->Player->IsEnemy(*table[i]->Player)))) {

			switch (this->DistanceType) {
			case GreaterThan:
			case GreaterThanEqual:
				break;
			case LessThan:
			case LessThanEqual:
				//Wyrmgus start
//				if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) <= distance) {
				if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) <= distance) {
				//Wyrmgus end
					count++;
				}
				break;
			case Equal:
				//Wyrmgus start
//				if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) == distance) {
				if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) == distance) {
				//Wyrmgus end
					count++;
				}
				break;
			case NotEqual:
				//Wyrmgus start
//				if (MapDistanceBetweenTypes(type, pos, *table[i]->Type, table[i]->tilePos) == distance) {
				if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer) == distance) {
				//Wyrmgus end
					count++;
				}
				break;
			}
		}
	}

	switch (this->CountType)
	{
	case LessThan: return count < this->Count;
	case LessThanEqual: return count <= this->Count;
	case Equal: return count == this->Count;
	case NotEqual: return count != this->Count;
	case GreaterThanEqual: return count >= this->Count;
	case GreaterThan: return count > this->Count;
	default: return false;
	}
}

inline bool CBuildRestrictionAddOn::functor::operator()(const CUnit *const unit) const
{
	return (unit->Type == Parent && unit->tilePos == this->pos);
}

/**
**  Check AddOn Restriction
*/
//Wyrmgus start
//bool CBuildRestrictionAddOn::Check(const CUnit *, const CUnitType &, const Vec2i &pos, CUnit *&) const
bool CBuildRestrictionAddOn::Check(const CUnit *, const CUnitType &, const Vec2i &pos, CUnit *&, int z) const
//Wyrmgus end
{
	Vec2i pos1 = pos - this->Offset;

	//Wyrmgus start
//	if (Map.Info.IsPointOnMap(pos1) == false) {
	if (Map.Info.IsPointOnMap(pos1, z) == false) {
	//Wyrmgus end
		return false;
	}
	functor f(Parent, pos1);
	//Wyrmgus start
//	return (Map.Field(pos1)->UnitCache.find(f) != NULL);
	return (Map.Field(pos1, z)->UnitCache.find(f) != NULL);
	//Wyrmgus end
}

/**
**  Check OnTop Restriction
*/
inline bool CBuildRestrictionOnTop::functor::operator()(CUnit *const unit)
{
	if (unit->tilePos == pos
		&& !unit->Destroyed && unit->Orders[0]->Action != UnitActionDie) {
		if (unit->Type == this->Parent && unit->Orders[0]->Action != UnitActionBuilt) {
			// Available to build on
			ontop = unit;
		} else {
			// Something else is built on this already
			ontop = NULL;
			return false;
		}
	}
	return true;
}

class AliveConstructedAndSameTypeAs
{
public:
	explicit AliveConstructedAndSameTypeAs(const CUnitType &unitType) : type(&unitType) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsAlive() && unit->Type == type && unit->CurrentAction() != UnitActionBuilt;
	}
private:
	const CUnitType *type;
};

//Wyrmgus start
//bool CBuildRestrictionOnTop::Check(const CUnit *builder, const CUnitType &, const Vec2i &pos, CUnit *&ontoptarget) const
bool CBuildRestrictionOnTop::Check(const CUnit *builder, const CUnitType &, const Vec2i &pos, CUnit *&ontoptarget, int z) const
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
	Assert(Map.Info.IsPointOnMap(pos, z));
	//Wyrmgus end

	ontoptarget = NULL;
	//Wyrmgus start
//	CUnitCache &cache = Map.Field(pos)->UnitCache;
	CUnitCache &cache = Map.Field(pos, z)->UnitCache;
	//Wyrmgus end

	CUnitCache::iterator it = std::find_if(cache.begin(), cache.end(), AliveConstructedAndSameTypeAs(*this->Parent));

	if (it != cache.end() && (*it)->tilePos == pos) {
		CUnit &found = **it;
		std::vector<CUnit *> table;
		Vec2i endPos(found.tilePos.x + found.Type->TileWidth - 1, found.tilePos.y + found.Type->TileHeight - 1);
		//Wyrmgus start
//		Select(found.tilePos, endPos, table);
		Select(found.tilePos, endPos, table, found.MapLayer);
		//Wyrmgus end
		for (std::vector<CUnit *>::iterator it2 = table.begin(); it2 != table.end(); ++it2) {
			if (*it == *it2) {
				continue;
			}
			if (builder == *it2) {
				continue;
			}
			//Wyrmgus start
			// allow to build if a decoration is present under the deposit
			if ((*it2)->Type->BoolFlag[DECORATION_INDEX].value) {
				continue;
			}
			//Wyrmgus end
			if (found.Type->UnitType == (*it2)->Type->UnitType) {
				return false;
			}
		}
		ontoptarget = *it;
		return true;
	}
	return false;
}


/**
**  Can build unit here.
**  Hall too near to goldmine.
**  Refinery or shipyard too near to oil patch.
**
**  @param unit  Unit doing the building
**  @param type  unit-type to be checked.
**  @param pos   Map position.
**
**  @return      OnTop, parent unit, builder on true or 1 if unit==NULL, NULL false.
*/
//Wyrmgus start
//CUnit *CanBuildHere(const CUnit *unit, const CUnitType &type, const Vec2i &pos)
CUnit *CanBuildHere(const CUnit *unit, const CUnitType &type, const Vec2i &pos, int z)
//Wyrmgus end
{
	//  Can't build outside the map
	//Wyrmgus start
//	if (pos.x + type.TileWidth > Map.Info.MapWidth) {
	if (pos.x + type.TileWidth > Map.Info.MapWidths[z]) {
	//Wyrmgus end
		return NULL;
	}
	//Wyrmgus start
//	if (pos.y + type.TileHeight > Map.Info.MapHeight) {
	if (pos.y + type.TileHeight > Map.Info.MapHeights[z]) {
	//Wyrmgus end
		return NULL;
	}

	// Must be checked before oil!
	if (type.BoolFlag[SHOREBUILDING_INDEX].value) {
		const int width = type.TileWidth;
		int h = type.TileHeight;
		bool success = false;

		// Need at least one coast tile
		//Wyrmgus start
//		unsigned int index = Map.getIndex(pos);
		unsigned int index = Map.getIndex(pos, z);
		//Wyrmgus end
		do {
			//Wyrmgus start
//			const CMapField *mf = Map.Field(index);
			const CMapField *mf = Map.Field(index, z);
			//Wyrmgus end
			int w = width;
			do {
				if (mf->CoastOnMap()) {
					success = true;
				}
				++mf;
			} while (!success && --w);
			//Wyrmgus start
//			index += Map.Info.MapWidth;
			index += Map.Info.MapWidths[z];
			//Wyrmgus end
		} while (!success && --h);
		if (!success) {
			return NULL;
		}
	}

	// Check special rules for AI players
	if (unit && unit->Player->AiEnabled) {
		bool aiChecked = true;
		size_t count = type.AiBuildingRules.size();
		if (count > 0) {
			CUnit *ontoptarget = NULL;
			for (unsigned int i = 0; i < count; ++i) {
				CBuildRestriction *rule = type.AiBuildingRules[i];
				// All checks processed, did we really have success
				//Wyrmgus start
//				if (rule->Check(unit, type, pos, ontoptarget)) {
				if (rule->Check(unit, type, pos, ontoptarget, z)) {
				//Wyrmgus end
					// We passed a full ruleset
					aiChecked = true;
					break;
				} else {
					aiChecked = false;
				}
			}
		}
		if (aiChecked == false) {
			return NULL;
		}
	}

	size_t count = type.BuildingRules.size();
	if (count > 0) {
		for (unsigned int i = 0; i < count; ++i) {
			CBuildRestriction *rule = type.BuildingRules[i];
			CUnit *ontoptarget = NULL;
			// All checks processed, did we really have success
			//Wyrmgus start
//			if (rule->Check(unit, type, pos, ontoptarget)) {
			if (rule->Check(unit, type, pos, ontoptarget, z)) {
			//Wyrmgus end
				// We passed a full ruleset return
				if (unit == NULL) {
					return ontoptarget ? ontoptarget : (CUnit *)1;
				} else {
					return ontoptarget ? ontoptarget : const_cast<CUnit *>(unit);
				}
			}
		}
		return NULL;
	}

	return (unit == NULL) ? (CUnit *)1 : const_cast<CUnit *>(unit);
}

/**
**  Can build on this point.
**
**  @param pos   tile map position.
**  @param  mask terrain mask
**
**  @return true if we can build on this point.
*/
//Wyrmgus start
//bool CanBuildOn(const Vec2i &pos, int mask)
bool CanBuildOn(const Vec2i &pos, int mask, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return (Map.Info.IsPointOnMap(pos) && !Map.Field(pos)->CheckMask(mask));
	return (Map.Info.IsPointOnMap(pos, z) && !Map.Field(pos, z)->CheckMask(mask));
	//Wyrmgus end
}

/**
**  Can build unit-type at this point.
**
**  @param unit  Worker that want to build the building or NULL.
**  @param type  Building unit-type.
**  @param pos   tile map position.
**  @param real  Really build, or just placement
**
**  @return      OnTop, parent unit, builder on true, NULL false.
**
*/
//Wyrmgus start
//CUnit *CanBuildUnitType(const CUnit *unit, const CUnitType &type, const Vec2i &pos, int real)
CUnit *CanBuildUnitType(const CUnit *unit, const CUnitType &type, const Vec2i &pos, int real, bool ignore_exploration, int z)
//Wyrmgus end
{
	// Terrain Flags don't matter if building on top of a unit.
	//Wyrmgus start
//	CUnit *ontop = CanBuildHere(unit, type, pos);
	CUnit *ontop = CanBuildHere(unit, type, pos, z);
	//Wyrmgus end
	if (ontop == NULL) {
		return NULL;
	}
	if (ontop != (CUnit *)1 && ontop != unit) {
		return ontop;
	}

	//  Remove unit that is building!
	if (unit) {
		UnmarkUnitFieldFlags(*unit);
	}

	CPlayer *player = NULL;

	//Wyrmgus start
//	if (unit && unit->Player->Type == PlayerPerson) {
	if (unit) {
	//Wyrmgus end
		player = unit->Player;
	}
	int testmask;
	//Wyrmgus start
//	unsigned int index = pos.y * Map.Info.MapWidth;
	unsigned int index = pos.y * Map.Info.MapWidths[z];
	//Wyrmgus end
	for (int h = 0; h < type.TileHeight; ++h) {
		for (int w = type.TileWidth; w--;) {
			/* first part of if (!CanBuildOn(x + w, y + h, testmask)) */
			//Wyrmgus start
//			if (!Map.Info.IsPointOnMap(pos.x + w, pos.y + h)) {
			if (!Map.Info.IsPointOnMap(pos.x + w, pos.y + h, z)) {
			//Wyrmgus end
				h = type.TileHeight;
				ontop = NULL;
				break;
			}
			if (player && !real) {
				//testmask = MapFogFilterFlags(player, x + w, y + h, type.MovementMask);
				testmask = MapFogFilterFlags(*player,
											 //Wyrmgus start
//											 index + pos.x + w, type.MovementMask);
											 index + pos.x + w, type.MovementMask, z);
											 //Wyrmgus end
			} else {
				testmask = type.MovementMask;
			}
			/*secound part of if (!CanBuildOn(x + w, y + h, testmask)) */
			//Wyrmgus start
//			const CMapField &mf = *Map.Field(index + pos.x + w);
			const CMapField &mf = *Map.Field(index + pos.x + w, z);
			//Wyrmgus end
			if (mf.CheckMask(testmask)) {
				h = type.TileHeight;
				ontop = NULL;
				break;
			}
			//Wyrmgus start
//			if (player && !mf.playerInfo.IsExplored(*player)) {
			if (player && !ignore_exploration && !mf.playerInfo.IsTeamExplored(*player)) {
			//Wyrmgus end
				h = type.TileHeight;
				ontop = NULL;
				break;
			}
		}
		//Wyrmgus start
//		index += Map.Info.MapWidth;
		index += Map.Info.MapWidths[z];
		//Wyrmgus end
	}
	if (unit) {
		MarkUnitFieldFlags(*unit);
	}
	// We can build here: check distance to gold mine/oil patch!
	return ontop;
}

//@}
