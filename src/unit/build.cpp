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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "unit/unit_type.h"

#include "actions.h"
#include "editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "player.h"
#include "quest.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "world.h"

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
//Wyrmgus start
//CBuildRestrictionOnTop *OnTopDetails(const CUnit &unit, const CUnitType *parent)
CBuildRestrictionOnTop *OnTopDetails(const CUnitType &type, const CUnitType *parent)
//Wyrmgus end
{
	//Wyrmgus start
//	for (std::vector<CBuildRestriction *>::const_iterator i = unit.Type->BuildingRules.begin();
	for (std::vector<CBuildRestriction *>::const_iterator i = type.BuildingRules.begin();
	//Wyrmgus end
		//Wyrmgus start
//		 i != unit.Type->BuildingRules.end(); ++i) {
		i != type.BuildingRules.end(); ++i) {
		//Wyrmgus end
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
	return nullptr;
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

//Wyrmgus start
/**
**  Check Or Restriction
*/
bool CBuildRestrictionOr::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const
{
	for (std::vector<CBuildRestriction *>::const_iterator i = _or_list.begin(); i != _or_list.end(); ++i) {
		if ((*i)->Check(builder, type, pos, ontoptarget, z)) {
			return true;
		}
	}
	return false;
}
//Wyrmgus end

/**
**  Init Distance Restriction
*/
void CBuildRestrictionDistance::Init()
{
	this->RestrictType = CUnitType::try_get(this->RestrictTypeName);
	this->restrict_class = stratagus::unit_class::try_get(this->restrict_class_name);
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
	CPlayer* player = builder != nullptr ? builder->Player : CPlayer::GetThisPlayer();

	if (this->DistanceType == DistanceTypeType::LessThanEqual
		|| this->DistanceType == DistanceTypeType::GreaterThan
		|| this->DistanceType == DistanceTypeType::Equal
		|| this->DistanceType == DistanceTypeType::NotEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance, 0);
		pos1.y = std::max<int>(pos.y - this->Distance, 0);
		pos2.x = std::min<int>(pos.x + type.TileSize.x + this->Distance, CMap::Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileSize.y + this->Distance, CMap::Map.Info.MapHeights[z]);
		distance = this->Distance;
	} else if (this->DistanceType == DistanceTypeType::LessThan || this->DistanceType == DistanceTypeType::GreaterThanEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance - 1, 0);
		pos1.y = std::max<int>(pos.y - this->Distance - 1, 0);
		pos2.x = std::min<int>(pos.x + type.TileSize.x + this->Distance + 1, CMap::Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileSize.y + this->Distance + 1, CMap::Map.Info.MapHeights[z]);
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
			(this->RestrictType == table[i]->Type || (this->restrict_class != nullptr && this->restrict_class == table[i]->Type->get_unit_class()) || (!this->RestrictType && this->restrict_class == nullptr && this->RestrictTypeOwner.size() > 0)) &&
			// RestrictTypeOwner is not set or unit belongs to a suitable player
			(this->RestrictTypeOwner.size() == 0 ||
			 (!this->RestrictTypeOwner.compare("self") && player == table[i]->Player) ||
			 (!this->RestrictTypeOwner.compare("allied") && (player == table[i]->Player || player->IsAllied(*table[i]->Player))) ||
			 //Wyrmgus start
//			 (!this->RestrictTypeOwner.compare("enemy") && player->IsEnemy(*table[i]->Player)))) {
			 (!this->RestrictTypeOwner.compare("enemy") && player->IsEnemy(*table[i])))) {
			 //Wyrmgus end

			switch (this->DistanceType) {
				case DistanceTypeType::GreaterThan:
				case DistanceTypeType::GreaterThanEqual:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) <= distance) {
						return Diagonal ? false : !(pos.x != table[i]->tilePos.x || pos.y != table[i]->tilePos.y);
					}
					break;
				case DistanceTypeType::LessThan:
				case DistanceTypeType::LessThanEqual:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) <= distance) {
						return Diagonal || pos.x == table[i]->tilePos.x || pos.y == table[i]->tilePos.y;
					}
					break;
				case DistanceTypeType::Equal:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) == distance) {
						return Diagonal || pos.x == table[i]->tilePos.x || pos.y == table[i]->tilePos.y;
					}
					break;
				case DistanceTypeType::NotEqual:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) == distance) {
						return Diagonal ? false : !(pos.x != table[i]->tilePos.x || pos.y != table[i]->tilePos.y);
					}
					break;
			}
		}
	}
	return (this->DistanceType == DistanceTypeType::GreaterThan ||
			this->DistanceType == DistanceTypeType::GreaterThanEqual ||
			this->DistanceType == DistanceTypeType::NotEqual);
}

void CBuildRestrictionHasUnit::Init()
{
	this->RestrictType = CUnitType::get(this->RestrictTypeName);
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
	CPlayer* player = builder != nullptr ? builder->Player : CPlayer::GetThisPlayer();
	int count = 0;
	if (this->RestrictTypeOwner.size() == 0 || !this->RestrictTypeOwner.compare("self")) {
		count = player->GetUnitTotalCount(*this->RestrictType);
	} else if (!this->RestrictTypeOwner.compare("allied")) {
		count = player->GetUnitTotalCount(*this->RestrictType);
		for (int i = 0; i < NumPlayers; i++) {
			if (player->IsAllied(*CPlayer::Players[i])) {
				count += CPlayer::Players[i]->GetUnitTotalCount(*this->RestrictType);
			}
		}
	} else if (!this->RestrictTypeOwner.compare("enemy")) {
		for (int i = 0; i < NumPlayers; i++) {
			if (player->IsEnemy(*CPlayer::Players[i])) {
				count += CPlayer::Players[i]->GetUnitTotalCount(*this->RestrictType);
			}
		}
	} else if (!this->RestrictTypeOwner.compare("any")) {
		for (int i = 0; i < NumPlayers; i++) {
			count += CPlayer::Players[i]->GetUnitTotalCount(*this->RestrictType);
		}
	}
	switch (this->CountType)
	{
	case DistanceTypeType::LessThan: return count < this->Count;
	case DistanceTypeType::LessThanEqual: return count <= this->Count;
	case DistanceTypeType::Equal: return count == this->Count;
	case DistanceTypeType::NotEqual: return count != this->Count;
	case DistanceTypeType::GreaterThanEqual: return count >= this->Count;
	case DistanceTypeType::GreaterThan: return count > this->Count;
	default: return false;
	}
}

void CBuildRestrictionSurroundedBy::Init()
{
	this->RestrictType = CUnitType::get(this->RestrictTypeName);
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

	if (this->DistanceType == DistanceTypeType::LessThanEqual
		|| this->DistanceType == DistanceTypeType::GreaterThan
		|| this->DistanceType == DistanceTypeType::Equal
		|| this->DistanceType == DistanceTypeType::NotEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance, 0);
		pos1.y = std::max<int>(pos.y - this->Distance, 0);
		pos2.x = std::min<int>(pos.x + type.TileSize.x + this->Distance, CMap::Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileSize.y + this->Distance, CMap::Map.Info.MapHeights[z]);
		distance = this->Distance;
	}
	else if (this->DistanceType == DistanceTypeType::LessThan || this->DistanceType == DistanceTypeType::GreaterThanEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance - 1, 0);
		pos1.y = std::max<int>(pos.y - this->Distance - 1, 0);
		pos2.x = std::min<int>(pos.x + type.TileSize.x + this->Distance + 1, CMap::Map.Info.MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.TileSize.y + this->Distance + 1, CMap::Map.Info.MapHeights[z]);
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
				//Wyrmgus start
//				(!this->RestrictTypeOwner.compare("enemy") && builder->Player->IsEnemy(*table[i]->Player)))) {
				(!this->RestrictTypeOwner.compare("enemy") && builder->Player->IsEnemy(*table[i])))) {
				//Wyrmgus end

			switch (this->DistanceType) {
			case DistanceTypeType::GreaterThan:
			case DistanceTypeType::GreaterThanEqual:
				break;
			case DistanceTypeType::LessThan:
			case DistanceTypeType::LessThanEqual:
				if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) <= distance) {
					count++;
				}
				break;
			case DistanceTypeType::Equal:
				if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) == distance) {
					count++;
				}
				break;
			case DistanceTypeType::NotEqual:
				if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) == distance) {
					count++;
				}
				break;
			}
		}
	}

	switch (this->CountType)
	{
	case DistanceTypeType::LessThan: return count < this->Count;
	case DistanceTypeType::LessThanEqual: return count <= this->Count;
	case DistanceTypeType::Equal: return count == this->Count;
	case DistanceTypeType::NotEqual: return count != this->Count;
	case DistanceTypeType::GreaterThanEqual: return count >= this->Count;
	case DistanceTypeType::GreaterThan: return count > this->Count;
	default: return false;
	}
}

inline bool CBuildRestrictionAddOn::functor::operator()(const CUnit *const unit) const
{
	return (unit->Type == Parent && unit->tilePos == this->pos);
}

void CBuildRestrictionAddOn::Init()
{
	this->Parent = CUnitType::get(this->ParentName);
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

	if (CMap::Map.Info.IsPointOnMap(pos1, z) == false) {
		return false;
	}
	functor f(Parent, pos1);
	//Wyrmgus start
	return (CMap::Map.Field(pos1, z)->UnitCache.find(f) != nullptr);
	//Wyrmgus end
}

/**
**  Check OnTop Restriction
*/
inline bool CBuildRestrictionOnTop::functor::operator()(CUnit *const unit)
{
	if (unit->tilePos == pos
		&& !unit->Destroyed && unit->Orders[0]->Action != UnitAction::Die) {
		if (unit->Type == this->Parent && unit->Orders[0]->Action != UnitAction::Built) {
			// Available to build on
			ontop = unit;
		} else {
			// Something else is built on this already
			ontop = nullptr;
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
		return unit->IsAlive() && unit->Type == type && unit->CurrentAction() != UnitAction::Built;
	}
private:
	const CUnitType *type;
};

void CBuildRestrictionOnTop::Init()
{
	this->Parent = CUnitType::get(this->ParentName);
}

bool CBuildRestrictionOnTop::Check(const CUnit *builder, const CUnitType &, const Vec2i &pos, CUnit *&ontoptarget, int z) const
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	ontoptarget = nullptr;

	CUnitCache &cache = CMap::Map.Field(pos, z)->UnitCache;

	CUnitCache::iterator it = std::find_if(cache.begin(), cache.end(), AliveConstructedAndSameTypeAs(*this->Parent));

	if (it != cache.end() && (*it)->tilePos == pos) {
		CUnit &found = **it;
		std::vector<CUnit *> table;
		Vec2i endPos(found.tilePos + found.Type->TileSize - 1);
		Select(found.tilePos, endPos, table, found.MapLayer->ID);
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

//Wyrmgus start
void CBuildRestrictionTerrain::Init()
{
	this->RestrictTerrainType = stratagus::terrain_type::get(this->RestrictTerrainTypeName);
}

/**
**  Check Terrain Restriction
*/
bool CBuildRestrictionTerrain::Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&, int z) const
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	for (int x = pos.x; x < pos.x + type.TileSize.x; ++x) {
		for (int y = pos.y; y < pos.y + type.TileSize.y; ++y) {
			if (!CMap::Map.Info.IsPointOnMap(x, y, z)) {
				continue;
			}
			Vec2i tile_pos(x, y);
			stratagus::terrain_type *terrain = CMap::Map.GetTileTerrain(tile_pos, this->RestrictTerrainType->Overlay, z);
			if (this->RestrictTerrainType == terrain) {
				return true;
			}
		}
	}

	return false;
}
//Wyrmgus end

/**
**  Can build unit here.
**  Hall too near to goldmine.
**  Refinery or shipyard too near to oil patch.
**
**  @param unit  Unit doing the building
**  @param type  unit-type to be checked.
**  @param pos   Map position.
**
**  @return      OnTop, parent unit, builder on true or 1 if unit==nullptr, null false.
*/
//Wyrmgus start
//CUnit *CanBuildHere(const CUnit *unit, const CUnitType &type, const Vec2i &pos)
CUnit *CanBuildHere(const CUnit *unit, const CUnitType &type, const Vec2i &pos, int z, bool no_bordering_building)
//Wyrmgus end
{
	//  Can't build outside the map
	if (!CMap::Map.Info.IsPointOnMap(pos, z)) {
		return nullptr;
	}

	if (pos.x + type.TileSize.x > CMap::Map.Info.MapWidths[z]) {
		return nullptr;
	}
	if (pos.y + type.TileSize.y > CMap::Map.Info.MapHeights[z]) {
		return nullptr;
	}
	
	//Wyrmgus start
	if (no_bordering_building && !OnTopDetails(type, nullptr)) { // if a game is starting, only place buildings with a certain space from other buildings
		for (int x = pos.x - 1; x < pos.x + type.TileSize.x + 1; ++x) {
			for (int y = pos.y - 1; y < pos.y + type.TileSize.y + 1; ++y) {
				if (CMap::Map.Info.IsPointOnMap(x, y, z) && (CMap::Map.Field(x, y, z)->Flags & MapFieldBuilding)) {
					return nullptr;
				}
			}
		}
	}
	
	if (unit) {
		for (int x = pos.x; x < pos.x + type.TileSize.x; ++x) {
			for (int y = pos.y; y < pos.y + type.TileSize.y; ++y) {
				if (CMap::Map.Info.IsPointOnMap(x, y, z) && CMap::Map.Field(x, y, z)->get_owner() != nullptr && CMap::Map.Field(x, y, z)->get_owner() != unit->Player) {
					return nullptr;
				}
			}
		}
	}
	//Wyrmgus end

	// Must be checked before oil!
	if (type.BoolFlag[SHOREBUILDING_INDEX].value) {
		const int width = type.TileSize.x;
		int h = type.TileSize.y;
		bool success = false;

		// Need at least one coast tile
		//Wyrmgus start
//		unsigned int index = CMap::Map.getIndex(pos);
		unsigned int index = CMap::Map.getIndex(pos, z);
		//Wyrmgus end
		do {
			//Wyrmgus start
//			const CMapField *mf = CMap::Map.Field(index);
			const CMapField *mf = CMap::Map.Field(index, z);
			//Wyrmgus end
			int w = width;
			do {
				if (mf->CoastOnMap()) {
					success = true;
				}
				//Wyrmgus start
				if (GameCycle == 0 && mf->WaterOnMap() && CMap::Map.TileBordersFlag(pos, z, MapFieldWaterAllowed, true)) { // if the game hasn't started, it is possible that coast map fields haven't been applied yet, so we have to check if the tile is a water tile with an adjacent non-water tile (which is what a coastal tile is)
					success = true;
				}
				//Wyrmgus end
				++mf;
			} while (!success && --w);
			//Wyrmgus start
//			index += CMap::Map.Info.MapWidth;
			index += CMap::Map.Info.MapWidths[z];
			//Wyrmgus end
		} while (!success && --h);
		if (!success) {
			return nullptr;
		}
	}

	// Check special rules for AI players
	if (unit && unit->Player->AiEnabled) {
		bool aiChecked = true;
		size_t count = type.AiBuildingRules.size();
		if (count > 0) {
			//Wyrmgus start
//			CUnit *ontoptarget = nullptr;
			//Wyrmgus end
			for (unsigned int i = 0; i < count; ++i) {
				CBuildRestriction *rule = type.AiBuildingRules[i];
				//Wyrmgus start
				CUnit *ontoptarget = nullptr;
				//Wyrmgus end
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
			return nullptr;
		}
	}

	size_t count = type.BuildingRules.size();
	if (count > 0) {
		for (unsigned int i = 0; i < count; ++i) {
			CBuildRestriction *rule = type.BuildingRules[i];
			CUnit *ontoptarget = nullptr;
			// All checks processed, did we really have success
			//Wyrmgus start
//			if (rule->Check(unit, type, pos, ontoptarget)) {
			if (rule->Check(unit, type, pos, ontoptarget, z)) {
			//Wyrmgus end
				// We passed a full ruleset return
				if (unit == nullptr) {
					return ontoptarget ? ontoptarget : (CUnit *)1;
				} else {
					return ontoptarget ? ontoptarget : const_cast<CUnit *>(unit);
				}
			}
		}
		return nullptr;
	}
	
	if (unit && z != unit->Player->StartMapLayer && (CMap::Map.MapLayers[z]->plane != CMap::Map.MapLayers[unit->Player->StartMapLayer]->plane || CMap::Map.MapLayers[z]->world != CMap::Map.MapLayers[unit->Player->StartMapLayer]->world)) {
		return nullptr;
	}

	return (unit == nullptr) ? (CUnit *)1 : const_cast<CUnit *>(unit);
}

/**
**  Can build on this point.
**
**  @param pos   tile map position.
**  @param  mask terrain mask
**
**  @return true if we can build on this point.
*/
bool CanBuildOn(const Vec2i &pos, int mask, int z)
{
	return (CMap::Map.Info.IsPointOnMap(pos, z) && !CMap::Map.Field(pos, z)->CheckMask(mask));
}

/**
**  Can build unit-type at this point.
**
**  @param unit  Worker that want to build the building or null.
**  @param type  Building unit-type.
**  @param pos   tile map position.
**  @param real  Really build, or just placement
**
**  @return      OnTop, parent unit, builder on true, null false.
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
	if (ontop == nullptr) {
		return nullptr;
	}
	if (ontop != (CUnit *)1 && ontop != unit) {
		return ontop;
	}

	//  Remove unit that is building!
	if (unit) {
		UnmarkUnitFieldFlags(*unit);
	}

	CPlayer *player = nullptr;

	//Wyrmgus start
//	if (unit && unit->Player->Type == PlayerPerson) {
	if (unit) {
	//Wyrmgus end
		player = unit->Player;
	}
	int testmask;
	//Wyrmgus start
//	unsigned int index = pos.y * CMap::Map.Info.MapWidth;
	unsigned int index = pos.y * CMap::Map.Info.MapWidths[z];
	//Wyrmgus end
	for (int h = 0; h < type.TileSize.y; ++h) {
		for (int w = type.TileSize.x; w--;) {
			/* first part of if (!CanBuildOn(x + w, y + h, testmask)) */
			if (!CMap::Map.Info.IsPointOnMap(pos.x + w, pos.y + h, z)) {
				h = type.TileSize.y;
				ontop = nullptr;
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
//			const CMapField &mf = *CMap::Map.Field(index + pos.x + w);
			const CMapField &mf = *CMap::Map.Field(index + pos.x + w, z);
			//Wyrmgus end
			if (mf.CheckMask(testmask)) {
				h = type.TileSize.y;
				ontop = nullptr;
				break;
			}
			//Wyrmgus start
//			if (player && !mf.playerInfo.IsExplored(*player)) {
			if (player && !ignore_exploration && !mf.playerInfo.IsTeamExplored(*player)) {
			//Wyrmgus end
				h = type.TileSize.y;
				ontop = nullptr;
				break;
			}
		}
		//Wyrmgus start
//		index += CMap::Map.Info.MapWidth;
		index += CMap::Map.Info.MapWidths[z];
		//Wyrmgus end
	}
	if (unit) {
		MarkUnitFieldFlags(*unit);
	}
	// We can build here: check distance to gold mine/oil patch!
	return ontop;
}
