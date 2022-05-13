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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "economy/resource_finder.h"

#include "actions.h"
#include "economy/resource.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder/pathfinder.h"
#include "player/player.h"
#include "unit/unit_find.h"

namespace wyrmgus {

struct find_resource_cost final
{
public:
	int calculate_modified_distance(int distance, const resource *resource, const CPlayer *worker_player) const
	{
		//apply modifiers to distance

		//apply conversion rate modifier
		distance *= 100;
		distance /= resource->get_final_resource_conversion_rate();

		//alter the distance score by the conversion rate, so that the unit will prefer resources with better conversion rates, but without going for ones that are too far away
		int price_modifier = worker_player->get_resource_price(resource->get_final_resource()) * resource->get_final_resource_conversion_rate() / 100;
		if (resource->get_input_resource() != nullptr) {
			price_modifier -= worker_player->get_resource_price(resource->get_input_resource());
		}
		price_modifier = std::max(price_modifier, 1);

		distance *= 100;
		distance /= price_modifier;

		return distance;
	}

	void set_from(const tile *tile, const QPoint &tile_pos, const int z, const CUnit *depot, const CUnit *worker)
	{
		const resource *resource = tile->get_resource();

		this->distance = depot ? depot->MapDistanceTo(tile_pos, z) : 0;

		this->distance = this->calculate_modified_distance(this->distance, resource, worker->Player);

		this->assigned = 0;
		this->waiting = 0;
	}

	void set_from(const CUnit *mine, const CUnit *depot, const CUnit *worker, const bool check_usage)
	{
		const resource *resource = mine->get_given_resource();

		this->distance = depot ? mine->MapDistanceTo(*depot) : 0;

		this->distance = this->calculate_modified_distance(this->distance, resource, worker->Player);

		if (!mine->Type->BoolFlag[CANHARVEST_INDEX].value) {
			// if it is a deposit rather than a readily-harvestable resource, multiply the distance score
			this->distance *= 8;
		}

		if (check_usage) {
			this->assigned = static_cast<int>(mine->Resource.Workers.size()) - mine->Type->MaxOnBoard;
			this->waiting = GetNumWaitingWorkers(*mine);
		} else {
			this->assigned = 0;
			this->waiting = 0;
		}
	}

	bool operator < (const find_resource_cost &rhs) const
	{
		if (this->waiting != rhs.waiting) {
			return this->waiting < rhs.waiting;
		} else if (this->distance != rhs.distance) {
			return this->distance < rhs.distance;
		} else {
			return this->assigned < rhs.assigned;
		}
	}

	void set_to_max()
	{ 
		assigned = waiting = distance = UINT_MAX; 
	}

	bool is_min() const
	{
		return this->assigned == 0 && this->waiting == 0 && this->distance == 0;
	}

private:
	unsigned int assigned = 0;
	unsigned int waiting = 0;
	unsigned int distance = 0;
};

struct find_resource_context final
{
	explicit find_resource_context(const resource_finder *finder, find_resource_result &result)
		: result(result), worker(finder->get_worker()), movemask(finder->get_worker()->Type->MovementMask), max_range(finder->get_range()), resource(finder->get_resource()), depot(finder->get_depot()), ignore_exploration(finder->ignores_exploration()), check_usage(finder->checks_usage()), only_harvestable(finder->includes_only_harvestable()), include_luxury_resources(finder->includes_luxury_resources()), only_same_resource(finder->allows_only_same_resource())
	{
		result = find_resource_result();
		this->best_cost.set_to_max();
	}

	VisitResult Visit(TerrainTraversal &terrain_traversal, const Vec2i &pos, const Vec2i &from)
	{
		Q_UNUSED(from)

		const CUnit *worker = this->worker;

		const tile *tile = worker->MapLayer->Field(pos);

		if (!tile->player_info->IsTeamExplored(*worker->Player) && !this->ignore_exploration) {
			return VisitResult::DeadEnd;
		}

		const CPlayer *tile_owner = tile->get_owner();

		if (tile_owner != nullptr && tile_owner != worker->Player && !tile_owner->has_neutral_faction_type() && !worker->Player->has_neutral_faction_type()) {
			if (this->resource->get_index() != TradeCost || tile_owner->is_enemy_of(*worker->Player)) {
				return VisitResult::DeadEnd;
			}
		}

		const int z = worker->MapLayer->ID;

		if (this->is_valid_resource_tile(tile)) {
			find_resource_cost cost;

			cost.set_from(tile, pos, z, this->depot, worker);

			if (cost < this->best_cost) {
				this->result.resource_pos = pos;
				this->result.resource_unit = nullptr;

				if (cost.is_min()) {
					return VisitResult::Finished;
				}

				this->best_cost = cost;
			}
		}

		CUnit *mine = tile->UnitCache.find([this](const CUnit *unit) {
			return this->is_valid_resource_unit(unit);
		});

		if (
			mine != nullptr && mine != result.resource_unit
			&& (mine->Type->BoolFlag[CANHARVEST_INDEX].value || tile_owner == nullptr || tile_owner == worker->Player) //this is needed to prevent neutral factions from trying to build mines in others' territory
		) {
			find_resource_cost cost;

			cost.set_from(mine, this->depot, worker, this->check_usage);

			if (cost < this->best_cost) {
				this->result.resource_unit = mine;
				this->result.resource_pos = QPoint(-1, -1);

				if (cost.is_min()) {
					return VisitResult::Finished;
				}

				this->best_cost = cost;
			}
		}

		if (CanMoveToMask(pos, this->movemask, z)) { // reachable
			if (terrain_traversal.Get(pos) < this->max_range) {
				return VisitResult::Ok;
			} else {
				return VisitResult::DeadEnd;
			}
		} else {
			//unreachable
			return VisitResult::DeadEnd;
		}
	}

	bool is_valid_resource(const wyrmgus::resource *resource) const
	{
		if (resource == nullptr) {
			return false;
		}

		return resource == this->resource
			|| (!this->only_same_resource && resource->get_index() != TradeCost && resource->get_final_resource() == this->resource)
			|| (this->include_luxury_resources && resource->is_luxury());
	}

	bool is_valid_resource_tile(const tile *tile) const
	{
		const wyrmgus::resource *tile_resource = tile->get_resource();

		return this->is_valid_resource(tile_resource) && tile->get_value() > 0 && this->worker->can_harvest(tile_resource);
	}

	bool is_valid_resource_unit(const CUnit *unit) const
	{
		return this->is_valid_resource(unit->get_given_resource())
			&& unit->ResourcesHeld > 0
			&& this->worker->can_harvest(unit, this->only_harvestable)
			&& !unit->IsUnusable(false);
	}

	find_resource_result &result;
	const CUnit *worker = nullptr;
	const tile_flag movemask = tile_flag::none;
	int max_range = 0;
	const wyrmgus::resource *resource = nullptr;
	const CUnit *depot = nullptr;
	const bool ignore_exploration = false;
	const bool check_usage = false;
	const bool only_harvestable = false;
	const bool include_luxury_resources = false;
	const bool only_same_resource = false;
	find_resource_cost best_cost;
};

find_resource_result resource_finder::find()
{
	if (this->depot == nullptr) { // Find the nearest depot
		this->depot = FindDepositNearLoc(*this->worker->Player, this->start_unit->tilePos, this->range, this->resource, this->start_unit->MapLayer->ID);
	}

	TerrainTraversal terrainTraversal;

	terrainTraversal.SetSize(this->start_unit->MapLayer->get_width(), this->start_unit->MapLayer->get_height());
	terrainTraversal.Init();

	if (this->worker != this->start_unit || this->start_unit->Container != nullptr) {
		terrainTraversal.push_unit_pos_and_neighbor_if_passable(*this->start_unit, this->worker->Type->MovementMask);
	} else {
		terrainTraversal.PushUnitPosAndNeighbor(*this->start_unit);
	}

	find_resource_result result;

	find_resource_context context(this, result);

	terrainTraversal.Run(context);

	return result;
}

}
