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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
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

#include "stratagus.h"

#include "unit/unit_type.h"

#include "actions.h"
#include "editor.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "player/player.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "util/assert_util.h"

/**
**  Find the building restriction that gives me this unit built on top
**  Allows you to define how the restriction is effecting the build
**
**  @param unit    the unit that is "OnTop"
**  @param parent  the parent unit if known. (guess otherwise)
**
**  @return        the BuildingRestrictionDetails
*/
const CBuildRestrictionOnTop *OnTopDetails(const wyrmgus::unit_type &type, const wyrmgus::unit_type *parent)
{
	for (const std::unique_ptr<CBuildRestriction> &b : type.BuildingRules) {
		const CBuildRestrictionOnTop *ontopb = dynamic_cast<CBuildRestrictionOnTop *>(b.get());

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

		CBuildRestrictionAnd *andb = dynamic_cast<CBuildRestrictionAnd *>(b.get());

		if (andb) {
			for (const auto &sub_b : andb->and_list) {
				ontopb = dynamic_cast<CBuildRestrictionOnTop *>(sub_b.get());
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
bool CBuildRestrictionAnd::Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const
{
	for (const auto &b : this->and_list) {
		if (!b->Check(builder, type, pos, ontoptarget, z)) {
			return false;
		}
	}
	return true;
}

/**
**  Check Or Restriction
*/
bool CBuildRestrictionOr::Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const
{
	for (const auto &b : this->or_list) {
		if (b->Check(builder, type, pos, ontoptarget, z)) {
			return true;
		}
	}
	return false;
}

/**
**  Init Distance Restriction
*/
void CBuildRestrictionDistance::Init()
{
	this->RestrictType = wyrmgus::unit_type::try_get(this->RestrictTypeName);
	this->restrict_class = wyrmgus::unit_class::try_get(this->restrict_class_name);
}

/**
**  Check Distance Restriction
*/
bool CBuildRestrictionDistance::Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&, int z) const
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
		pos2.x = std::min<int>(pos.x + type.get_tile_width() + this->Distance, CMap::get()->Info->MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.get_tile_height() + this->Distance, CMap::get()->Info->MapHeights[z]);
		distance = this->Distance;
	} else if (this->DistanceType == DistanceTypeType::LessThan || this->DistanceType == DistanceTypeType::GreaterThanEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance - 1, 0);
		pos1.y = std::max<int>(pos.y - this->Distance - 1, 0);
		pos2.x = std::min<int>(pos.x + type.get_tile_width() + this->Distance + 1, CMap::get()->Info->MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.get_tile_height() + this->Distance + 1, CMap::get()->Info->MapHeights[z]);
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
			 (!this->RestrictTypeOwner.compare("allied") && (player == table[i]->Player || player->is_allied_with(*table[i]->Player))) ||
			 //Wyrmgus start
//			 (!this->RestrictTypeOwner.compare("enemy") && player->is_enemy_of(*table[i]->Player)))) {
			 (!this->RestrictTypeOwner.compare("enemy") && player->is_enemy_of(*table[i])))) {
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
	this->RestrictType = wyrmgus::unit_type::get(this->RestrictTypeName);
}

/**
**  Check HasUnit Restriction
*/
bool CBuildRestrictionHasUnit::Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&, const int z) const
{
	Q_UNUSED(type)
	Q_UNUSED(pos)
	Q_UNUSED(z)

	Vec2i pos1(0, 0);
	Vec2i pos2(0, 0);
	CPlayer* player = builder != nullptr ? builder->Player : CPlayer::GetThisPlayer();
	int count = 0;
	if (this->RestrictTypeOwner.size() == 0 || !this->RestrictTypeOwner.compare("self")) {
		count = player->GetUnitTotalCount(*this->RestrictType);
	} else if (!this->RestrictTypeOwner.compare("allied")) {
		count = player->GetUnitTotalCount(*this->RestrictType);
		for (int i = 0; i < NumPlayers; i++) {
			if (player->is_allied_with(*CPlayer::Players[i])) {
				count += CPlayer::Players[i]->GetUnitTotalCount(*this->RestrictType);
			}
		}
	} else if (!this->RestrictTypeOwner.compare("enemy")) {
		for (int i = 0; i < NumPlayers; i++) {
			if (player->is_enemy_of(*CPlayer::Players[i])) {
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
	this->RestrictType = wyrmgus::unit_type::get(this->RestrictTypeName);
}

/**
**  Check Surrounded By Restriction
*/
bool CBuildRestrictionSurroundedBy::Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&, int z) const
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
		pos2.x = std::min<int>(pos.x + type.get_tile_width() + this->Distance, CMap::get()->Info->MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.get_tile_height() + this->Distance, CMap::get()->Info->MapHeights[z]);
		distance = this->Distance;
	}
	else if (this->DistanceType == DistanceTypeType::LessThan || this->DistanceType == DistanceTypeType::GreaterThanEqual) {
		pos1.x = std::max<int>(pos.x - this->Distance - 1, 0);
		pos1.y = std::max<int>(pos.y - this->Distance - 1, 0);
		pos2.x = std::min<int>(pos.x + type.get_tile_width() + this->Distance + 1, CMap::get()->Info->MapWidths[z]);
		pos2.y = std::min<int>(pos.y + type.get_tile_height() + this->Distance + 1, CMap::get()->Info->MapHeights[z]);
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
				(!this->RestrictTypeOwner.compare("allied") && (builder->Player == table[i]->Player || builder->Player->is_allied_with(*table[i]->Player))) ||
				//Wyrmgus start
//				(!this->RestrictTypeOwner.compare("enemy") && builder->Player->is_enemy_of(*table[i]->Player)))) {
				(!this->RestrictTypeOwner.compare("enemy") && builder->Player->is_enemy_of(*table[i])))) {
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
	this->Parent = wyrmgus::unit_type::get(this->ParentName);
}

/**
**  Check AddOn Restriction
*/
bool CBuildRestrictionAddOn::Check(const CUnit *, const wyrmgus::unit_type &, const Vec2i &pos, CUnit *&, int z) const
{
	Vec2i pos1 = pos - this->Offset;

	if (CMap::get()->Info->IsPointOnMap(pos1, z) == false) {
		return false;
	}
	functor f(Parent, pos1);
	//Wyrmgus start
	return (CMap::get()->Field(pos1, z)->UnitCache.find(f) != nullptr);
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
	explicit AliveConstructedAndSameTypeAs(const wyrmgus::unit_type &unitType) : type(&unitType) {}
	bool operator()(const CUnit *unit) const
	{
		return unit->IsAlive() && unit->Type == type && unit->CurrentAction() != UnitAction::Built;
	}
private:
	const wyrmgus::unit_type *type;
};

void CBuildRestrictionOnTop::Init()
{
	this->Parent = wyrmgus::unit_type::get(this->ParentName);
}

bool CBuildRestrictionOnTop::Check(const CUnit *builder, const wyrmgus::unit_type &, const Vec2i &pos, CUnit *&ontoptarget, int z) const
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	ontoptarget = nullptr;

	CUnitCache &cache = CMap::get()->Field(pos, z)->UnitCache;

	CUnitCache::iterator it = std::find_if(cache.begin(), cache.end(), AliveConstructedAndSameTypeAs(*this->Parent));

	if (it != cache.end() && (*it)->tilePos == pos) {
		CUnit &found = **it;
		std::vector<CUnit *> table;
		Vec2i endPos(found.tilePos + found.Type->get_tile_size() - QSize(1, 1));
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

			if (found.Type->get_domain() == (*it2)->Type->get_domain()) {
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
	this->RestrictTerrainType = wyrmgus::terrain_type::get(this->RestrictTerrainTypeName);
}

/**
**  Check Terrain Restriction
*/
bool CBuildRestrictionTerrain::Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&, int z) const
{
	Q_UNUSED(builder)

	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	for (int x = pos.x; x < pos.x + type.get_tile_width(); ++x) {
		for (int y = pos.y; y < pos.y + type.get_tile_height(); ++y) {
			if (!CMap::get()->Info->IsPointOnMap(x, y, z)) {
				continue;
			}
			const Vec2i tile_pos(x, y);
			const wyrmgus::terrain_type *terrain = CMap::get()->GetTileTerrain(tile_pos, this->RestrictTerrainType->is_overlay(), z);
			if (this->RestrictTerrainType == terrain) {
				return true;
			}
		}
	}

	return false;
}
//Wyrmgus end

static bool is_tile_impassable(const QPoint &pos, const int z, const unit_type &unit_type)
{
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		return false;
	}

	const tile *tile = CMap::get()->Field(pos, z);

	if (tile->has_flag(tile_flag::rock) || tile->has_flag(tile_flag::tree)) {
		return true;
	}

	switch (unit_type.get_domain()) {
		case unit_domain::land:
			if (!unit_type.BoolFlag[SHOREBUILDING_INDEX].value && !tile->has_flag(tile_flag::land_allowed)) {
				return true;
			}
			break;
		case unit_domain::water:
			if (!tile->has_flag(tile_flag::water_allowed) && !tile->has_flag(tile_flag::coast_allowed)) {
				return true;
			}
			break;
		default:
			break;
	}

	return false;
}

bool check_tile_passable_blocks(const QPoint &pos, const int z, const unit_type &unit_type, bool &current_impassable, int &passable_block_count)
{
	const bool impassable = is_tile_impassable(pos, z, unit_type);

	if (impassable == current_impassable) {
		return true;
	}

	if (!impassable == false) {
		++passable_block_count;

		if (passable_block_count > 1) {
			return false;
		}
	}

	current_impassable = impassable;

	return true;
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
**  @return      OnTop, parent unit, builder on true or 1 if unit==nullptr, null false.
*/
CUnit *CanBuildHere(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int z, const bool no_bordering_impassable)
{
	//  Can't build outside the map
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		return nullptr;
	}

	if ((pos.x() + type.get_tile_width()) > CMap::get()->Info->MapWidths[z]) {
		return nullptr;
	}
	if ((pos.y() + type.get_tile_height()) > CMap::get()->Info->MapHeights[z]) {
		return nullptr;
	}
	
	if (no_bordering_impassable && !OnTopDetails(type, nullptr)) {
		//if a game is starting, only place buildings with a certain space from other buildings
		for (int x = pos.x() - 1; x < pos.x() + type.get_tile_width() + 1; ++x) {
			for (int y = pos.y() - 1; y < pos.y() + type.get_tile_height() + 1; ++y) {
				const QPoint tile_pos(x, y);

				if (!CMap::get()->Info->IsPointOnMap(tile_pos, z)) {
					continue;
				}

				const tile *tile = CMap::get()->Field(tile_pos, z);

				if (tile->has_flag(tile_flag::building)) {
					return nullptr;
				}
			}
		}

		//additionally, the building's adjacent tiles cannot contain more than one block of passable tiles which cannot be traversed between one another
		bool impassable = true;
		int passable_block_count = 0;

		for (int x = pos.x() - 1; x < pos.x() + type.get_tile_width() + 1; ++x) {
			const int y = pos.y() - 1;
			const QPoint tile_pos(x, y);
			if (!check_tile_passable_blocks(tile_pos, z, type, impassable, passable_block_count)) {
				return nullptr;
			}
		}

		for (int y = pos.y() - 1; y < pos.y() + type.get_tile_height() + 1; ++y) {
			const int x = pos.x() + type.get_tile_width();
			const QPoint tile_pos(x, y);
			if (!check_tile_passable_blocks(tile_pos, z, type, impassable, passable_block_count)) {
				return nullptr;
			}
		}

		for (int x = pos.x() + type.get_tile_width(); x >= pos.x() - 1; --x) {
			const int y = pos.y() + type.get_tile_height();
			const QPoint tile_pos(x, y);
			if (!check_tile_passable_blocks(tile_pos, z, type, impassable, passable_block_count)) {
				return nullptr;
			}
		}

		for (int y = pos.y() + type.get_tile_height(); y >= pos.y() - 1; --y) {
			const int x = pos.x() - 1;
			const QPoint tile_pos(x, y);
			if (!check_tile_passable_blocks(tile_pos, z, type, impassable, passable_block_count)) {
				return nullptr;
			}
		}
	}

	// Must be checked before oil!
	if (type.BoolFlag[SHOREBUILDING_INDEX].value) {
		const int width = type.get_tile_width();
		int h = type.get_tile_height();
		bool success = false;

		// Need at least one coast tile
		//Wyrmgus start
//		unsigned int index = CMap::get()->get_pos_index(pos);
		unsigned int index = CMap::get()->get_pos_index(pos, z);
		//Wyrmgus end
		do {
			//Wyrmgus start
//			const wyrmgus::tile *mf = CMap::get()->Field(index);
			const wyrmgus::tile *mf = CMap::get()->Field(index, z);
			//Wyrmgus end
			int w = width;
			do {
				if (mf->CoastOnMap()) {
					success = true;
				}
				//Wyrmgus start
				if (GameCycle == 0 && mf->WaterOnMap() && CMap::get()->TileBordersFlag(pos, z, tile_flag::water_allowed, true)) { // if the game hasn't started, it is possible that coast map fields haven't been applied yet, so we have to check if the tile is a water tile with an adjacent non-water tile (which is what a coastal tile is)
					success = true;
				}
				//Wyrmgus end
				++mf;
			} while (!success && --w);
			//Wyrmgus start
//			index += CMap::get()->Info->MapWidth;
			index += CMap::get()->Info->MapWidths[z];
			//Wyrmgus end
		} while (!success && --h);
		if (!success) {
			return nullptr;
		}
	}

	// Check special rules for AI players
	if (unit && unit->Player->AiEnabled) {
		bool aiChecked = true;
		if (!type.AiBuildingRules.empty()) {
			for (const auto &rule : type.AiBuildingRules) {
				CUnit *ontoptarget = nullptr;
				// All checks processed, did we really have success
				if (rule->Check(unit, type, pos, ontoptarget, z)) {
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

	if (!type.BuildingRules.empty()) {
		for (const auto &rule : type.BuildingRules) {
			CUnit *ontoptarget = nullptr;
			// All checks processed, did we really have success
			if (rule->Check(unit, type, pos, ontoptarget, z)) {
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
	
	if (unit && z != unit->Player->StartMapLayer && CMap::get()->MapLayers[z]->world != CMap::get()->MapLayers[unit->Player->StartMapLayer]->world) {
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
bool CanBuildOn(const QPoint &pos, const tile_flag mask, const int z, const CPlayer *player, const wyrmgus::unit_type *unit_type)
{
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		return false;
	}

	const wyrmgus::tile *tile = CMap::get()->Field(pos, z);

	if (tile->CheckMask(mask)) {
		return false;
	}

	if (player != nullptr && tile->get_owner() != nullptr && tile->get_owner() != player) {
		return false;
	}

	const wyrmgus::terrain_type *built_terrain = unit_type->TerrainType;

	//cannot build anything other than pathways on trade routes
	if (tile->is_on_trade_route() && (built_terrain == nullptr || !built_terrain->is_pathway())) {
		return false;
	}

	const wyrmgus::terrain_type *overlay_terrain = tile->get_overlay_terrain();

	//can only build a pathway on top of another pathway if the latter has a smaller movement bonus, or if the former is a railroad and the latter isn't
	if (built_terrain != nullptr && built_terrain->is_pathway() && overlay_terrain != nullptr && overlay_terrain->is_pathway() && built_terrain->get_movement_bonus() <= overlay_terrain->get_movement_bonus() && (!built_terrain->has_flag(tile_flag::railroad) || tile->has_flag(tile_flag::railroad))) {
		return false;
	}

	return true;
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
CUnit *CanBuildUnitType(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int real, const bool ignore_exploration, const int z, const bool no_bordering_impassable)
{
	// Terrain Flags don't matter if building on top of a unit.
	CUnit *ontop = CanBuildHere(unit, type, pos, z, no_bordering_impassable);
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
//	if (unit && unit->Player->get_type() == player_type::person) {
	if (unit) {
	//Wyrmgus end
		player = unit->Player;
	}
	tile_flag testmask = tile_flag::none;
	unsigned int index = pos.y() * CMap::get()->Info->MapWidths[z];
	for (int h = 0; h < type.get_tile_height(); ++h) {
		for (int w = type.get_tile_width(); w--;) {
			/* first part of if (!CanBuildOn(x + w, y + h, testmask)) */
			if (!CMap::get()->Info->IsPointOnMap(pos.x() + w, pos.y() + h, z)) {
				ontop = nullptr;
				break;
			}
			if (player && !real) {
				//testmask = MapFogFilterFlags(player, x + w, y + h, type.MovementMask);
				testmask = MapFogFilterFlags(*player,
											 index + pos.x() + w, type.MovementMask, z);
			} else {
				testmask = type.MovementMask;
			}
			/*second part of if (!CanBuildOn(x + w, y + h, testmask)) */
			const wyrmgus::tile *tile = CMap::get()->Field(index + pos.x() + w, z);
			if (tile->CheckMask(testmask)) {
				ontop = nullptr;
				break;
			}
			//Wyrmgus start
//			if (player && !mf.player_info->is_explored(*player)) {
			if (player && !ignore_exploration && !tile->player_info->IsTeamExplored(*player)) {
			//Wyrmgus end
				ontop = nullptr;
				break;
			}

			if (player != nullptr && tile->get_owner() != nullptr && tile->get_owner() != player) {
				ontop = nullptr;
				break;
			}

			//cannot build anything other than pathways on trade routes
			if (tile->is_on_trade_route() && (type.TerrainType == nullptr || !type.TerrainType->is_pathway())) {
				ontop = nullptr;
				break;
			}

			//can only build a pathway on top of another pathway if the latter has a smaller movement bonus
			if (type.TerrainType != nullptr && type.TerrainType->is_pathway() && tile->get_overlay_terrain() != nullptr && tile->get_overlay_terrain()->is_pathway() && type.TerrainType->get_movement_bonus() <= tile->get_overlay_terrain()->get_movement_bonus()) {
				ontop = nullptr;
				break;
			}
		}

		if (ontop == nullptr) {
			break;
		}

		index += CMap::get()->Info->MapWidths[z];
	}
	if (unit) {
		MarkUnitFieldFlags(*unit);
	}
	// We can build here: check distance to gold mine/oil patch!
	return ontop;
}
