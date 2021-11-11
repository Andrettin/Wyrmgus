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
/**@name action_resource.cpp - The generic resource action. */
//
//      (c) Copyright 2001-2021 by Crestez Leonard, Jimmy Salmon and Andrettin
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

#include "action/action_resource.h"

#include "ai.h"
#include "ai/ai_local.h"
#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/resource.h"
#include "economy/resource_storage_type.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder.h"
#include "player/civilization.h"
#include "player/player.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "script.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound/sound.h"
#include "translate.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "video/video.h"

static constexpr int SUB_START_RESOURCE = 0;
static constexpr int SUB_MOVE_TO_RESOURCE = 5;
static constexpr int SUB_UNREACHABLE_RESOURCE = 31;
static constexpr int SUB_START_GATHERING = 55;
static constexpr int SUB_GATHER_RESOURCE = 60;
static constexpr int SUB_STOP_GATHERING = 65;
static constexpr int SUB_MOVE_TO_DEPOT = 70;
static constexpr int SUB_UNREACHABLE_DEPOT = 100;
static constexpr int SUB_RETURN_RESOURCE = 120;

class NearReachableTerrainFinder final
{
public:
	explicit NearReachableTerrainFinder(const CPlayer &player, const int maxDist, const tile_flag movemask, const int resource, Vec2i *resPos, const int z) :
		player(player), maxDist(maxDist), movemask(movemask), resource(resource), resPos(resPos), z(z) {}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CPlayer &player;
	int maxDist;
	tile_flag movemask;
	//Wyrmgus start
//	int resmask;
	int resource;
	int z;
	//Wyrmgus end
	Vec2i *resPos;
};

VisitResult NearReachableTerrainFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	//Wyrmgus start
//	if (!player.AiEnabled && !Map.Field(pos)->player_info->IsExplored(player)) {
	if (!CMap::get()->Field(pos, z)->player_info->IsTeamExplored(player)) {
	//Wyrmgus end
		return VisitResult::DeadEnd;
	}

	//Wyrmgus start
	if (CMap::get()->Field(pos, z)->get_owner() != nullptr && CMap::get()->Field(pos, z)->get_owner() != &player && !CMap::get()->Field(pos, z)->get_owner()->has_neutral_faction_type() && !player.has_neutral_faction_type()) {
		return VisitResult::DeadEnd;
	}
	//Wyrmgus end

	// Look if found what was required.
	//Wyrmgus start
//	if (CanMoveToMask(pos, movemask)) {
	if (CanMoveToMask(pos, movemask, z)) {
	//Wyrmgus end
		if (resPos) {
			*resPos = from;
		}
		return VisitResult::Finished;
	}
	//Wyrmgus start
//	if (CMap::get()->Field(pos)->CheckMask(resmask)) { // reachable
	if (CMap::get()->Field(pos, z)->get_resource() == wyrmgus::resource::get_all()[resource]) { // reachable
	//Wyrmgus end
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult::Ok;
		} else {
			return VisitResult::DeadEnd;
		}
	} else { // unreachable
		return VisitResult::DeadEnd;
	}
}

//Wyrmgus start
//static bool FindNearestReachableTerrainType(const tile_flag movemask, int resmask, int range,
static bool FindNearestReachableTerrainType(const tile_flag movemask, const int resource, const int range,
//Wyrmgus end
											//Wyrmgus start
//											const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos)
											const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos, const int z)
											//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::get()->Info->MapWidth, CMap::get()->Info->MapHeight);
	terrainTraversal.SetSize(CMap::get()->Info->MapWidths[z], CMap::get()->Info->MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	//Wyrmgus start
//	assert_throw(CMap::get()->Field(startPos)->CheckMask(resmask));
	assert_throw(CMap::get()->Field(startPos, z)->get_resource() == wyrmgus::resource::get_all()[resource]);
	//Wyrmgus end
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	NearReachableTerrainFinder nearReachableTerrainFinder(player, range, movemask, resmask, terrainPos);
	NearReachableTerrainFinder nearReachableTerrainFinder(player, range, movemask, resource, terrainPos, z);
	//Wyrmgus end

	return terrainTraversal.Run(nearReachableTerrainFinder);
}

//Wyrmgus start
//std::unique_ptr<COrder> COrder::NewActionResource(CUnit &harvester, const Vec2i &pos)
std::unique_ptr<COrder> COrder::NewActionResource(CUnit &harvester, const Vec2i &pos, int z)
//Wyrmgus end
{
	auto order = std::make_unique<COrder_Resource>(harvester);
	Vec2i ressourceLoc;

	if (CMap::get()->Info->IsPointOnMap(pos, z) && CMap::get()->Field(pos, z)->get_resource() != nullptr) {
		order->CurrentResource = CMap::get()->Field(pos, z)->get_resource()->get_index();
		//  Find the closest resource tile next to a tile where the unit can move
		if (!FindNearestReachableTerrainType(harvester.Type->MovementMask, CMap::get()->Field(pos, z)->get_resource()->get_index(), 20, *harvester.Player, pos, &ressourceLoc, z)) {
			DebugPrint("FIXME: Give up???\n");
			ressourceLoc = pos;
		}
	}
	order->goalPos = ressourceLoc;
	order->MapLayer = z;
	//Wyrmgus end
		
	return order;
}

std::unique_ptr<COrder> COrder::NewActionResource(CUnit &harvester, CUnit &mine)
{
	auto order = std::make_unique<COrder_Resource>(harvester);

	order->set_goal(&mine);
	order->Resource.Mine = mine.acquire_ref();
	order->Resource.Pos = mine.tilePos + mine.GetHalfTileSize();
	order->Resource.MapLayer = mine.MapLayer->ID;
	order->CurrentResource = mine.GivesResource;
	return order;
}

std::unique_ptr<COrder> COrder::NewActionReturnGoods(CUnit &harvester, CUnit *depot)
{
	auto order = std::make_unique<COrder_Resource>(harvester);

	// Destination could be killed. NETWORK!
	if (depot && depot->Destroyed) {
		depot = nullptr;
	}
	order->CurrentResource = harvester.CurrentResource;
	order->DoneHarvesting = true;

	if (depot == nullptr) {
		depot = FindDeposit(harvester, 1000, harvester.get_current_resource());
	}
	if (depot) {
		order->Depot = depot->acquire_ref();
		order->UnitGotoGoal(harvester, depot, SUB_MOVE_TO_DEPOT);
	} else {
		order->State = SUB_UNREACHABLE_DEPOT;
		order->goalPos = harvester.tilePos;
		order->MapLayer = harvester.MapLayer->ID;
	}
	return order;
}

const resource *COrder_Resource::get_current_resource() const
{
	if (this->CurrentResource != 0) {
		return resource::get_all()[this->CurrentResource];
	}

	return nullptr;
}

Vec2i COrder_Resource::GetHarvestLocation() const
{
	if (this->Resource.get_mine() != nullptr) {
		assert_throw(this->Resource.get_mine()->Type != nullptr);
		return this->Resource.get_mine()->tilePos;
	} else {
		return this->Resource.Pos;
	}
}

//Wyrmgus start
int COrder_Resource::GetHarvestMapLayer() const
{
	//Wyrmgus start
//	if (this->Resource.get_mine() != nullptr) {
	if (this->Resource.get_mine() != nullptr && this->Resource.get_mine()->Type != nullptr) {
	//Wyrmgus end
		return this->Resource.get_mine()->MapLayer->ID;
	} else {
		return this->Resource.MapLayer;
	}
}
//Wyrmgus end

bool COrder_Resource::IsGatheringStarted() const
{
	return this->State > SUB_START_GATHERING;
}

bool COrder_Resource::IsGatheringFinished() const
{
	return this->State >= SUB_STOP_GATHERING;
}

bool COrder_Resource::IsGatheringWaiting() const
{
	return this->State == SUB_START_GATHERING && this->get_worker()->Wait != 0;
}

COrder_Resource::COrder_Resource(CUnit &harvester) : COrder(UnitAction::Resource), worker(harvester.acquire_ref())
{
}

COrder_Resource::~COrder_Resource()
{
	CUnit *mine = this->Resource.get_mine();

	if (mine && mine->IsAlive()) {
		this->get_worker()->DeAssignWorkerFromMine(*mine);
	}

	CUnit *goal = this->get_goal();
	if (goal) {
		// If mining decrease the active count on the resource.
		if (this->State == SUB_GATHER_RESOURCE) {

			goal->Resource.Active--;
			assert_throw(goal->Resource.Active >= 0);
		}
	}
}

void COrder_Resource::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-resource\",");
	if (this->Finished) {
		file.printf(" \"finished\",");
	}
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	assert_throw(this->get_worker() != nullptr && this->get_worker()->IsAlive());
	file.printf(" \"worker\", \"%s\",", UnitReference(this->get_worker()).c_str());
	file.printf(" \"current-res\", %d,", this->CurrentResource);

	file.printf(" \"res-pos\", {%d, %d},", this->Resource.Pos.x, this->Resource.Pos.y);
	//Wyrmgus start
	file.printf(" \"res-map-layer\", %d,", this->Resource.MapLayer);
	//Wyrmgus end
	//Wyrmgus start
//	if (this->Resource.get_mine() != nullptr) {
	if (this->Resource.get_mine() != nullptr && this->Resource.get_mine()->Type != nullptr) {
	//Wyrmgus end
		file.printf(" \"res-mine\", \"%s\",", UnitReference(this->Resource.get_mine()).c_str());
	}
	if (this->Depot != nullptr) {
		file.printf(" \"res-depot\", \"%s\",", UnitReference(this->get_depot()).c_str());
	}
	if (this->DoneHarvesting) {
		file.printf(" \"done-harvesting\",");
	}
	file.printf(" \"timetoharvest\", %d,", this->TimeToHarvest);
	file.printf(" \"state\", %d", this->State);
	file.printf("}");
}

bool COrder_Resource::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp(value, "current-res")) {
		++j;
		this->CurrentResource = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "done-harvesting")) {
		this->DoneHarvesting = true;
	} else if (!strcmp(value, "res-depot")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Depot = CclGetUnitFromRef(l)->acquire_ref();
		lua_pop(l, 1);
	} else if (!strcmp(value, "res-mine")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Resource.Mine = CclGetUnitFromRef(l)->acquire_ref();
		lua_pop(l, 1);
	} else if (!strcmp(value, "res-pos")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->Resource.Pos.x , &this->Resource.Pos.y);
		lua_pop(l, 1);
	//Wyrmgus start
	} else if (!strcmp(value, "res-map-layer")) {
		++j;
		this->Resource.MapLayer = LuaToNumber(l, -1, j + 1);
	//Wyrmgus end
	} else if (!strcmp(value, "state")) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "timetoharvest")) {
		++j;
		this->TimeToHarvest = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "worker")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->worker = CclGetUnitFromRef(l)->acquire_ref();
		lua_pop(l, 1);
	} else if (!strcmp(value, "tile")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->goalPos.x , &this->goalPos.y);
		lua_pop(l, 1);
	//Wyrmgus start
	} else if (!strcmp(value, "map-layer")) {
		++j;
		this->MapLayer = LuaToNumber(l, -1, j + 1);
	//Wyrmgus end
	} else {
		return false;
	}
	return true;
}

bool COrder_Resource::IsValid() const
{
	return true;
}

PixelPos COrder_Resource::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	PixelPos targetPos;

	if (this->has_goal()) {
		//Wyrmgus start
		if (this->get_goal()->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.scaled_map_to_screen_pixel_pos(this->get_goal()->get_scaled_map_pixel_pos_center());
	} else {
		//Wyrmgus start
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}

	if (preferences::get()->are_pathlines_enabled()) {
		Video.FillCircleClip(ColorYellow, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		Video.DrawLineClip(ColorYellow, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorYellow, targetPos, (3 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}

	return targetPos;
}

void COrder_Resource::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(1);

	Vec2i tileSize;
	if (this->has_goal()) {
		CUnit *goal = this->get_goal();
		tileSize = goal->get_tile_size();
		input.SetGoal(goal->tilePos, tileSize, goal->MapLayer->ID);
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}

bool COrder_Resource::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /* damage*/)
{
	Q_UNUSED(attacker)

	if (this->IsGatheringFinished()) {
		// Normal return to depot
		return true;
	}
	if (this->IsGatheringStarted()  && unit.ResourcesHeld > 0) {
		// escape to Depot with what you have
		this->DoneHarvesting = true;
		return true;
	}
	return false;
}



/**
**  Move unit to terrain.
**
**  @return      1 if reached, -1 if unreacheable, 0 if on the way.
*/
int COrder_Resource::MoveToResource_Terrain(CUnit &unit)
{
	Vec2i pos = this->goalPos;
	//Wyrmgus start
	int z = this->MapLayer;
	//Wyrmgus end

	// Wood gone, look somewhere else.
	if ((CMap::get()->Info->IsPointOnMap(pos, z) == false || CMap::get()->Field(pos, z)->get_resource() != resource::get_all()[this->CurrentResource])
		&& (!unit.get_pixel_offset().x()) && (!unit.get_pixel_offset().y())) {
		//Wyrmgus start
//		if (!FindTerrainType(unit.Type->MovementMask, tile_flag::tree, 16, *unit.Player, this->goalPos, &pos)) {
		if (!FindTerrainType(unit.Type->MovementMask, resource::get_all()[this->CurrentResource], 16, *unit.Player, this->goalPos, &pos, this->MapLayer)) {
		//Wyrmgus end
			// no wood in range
			return -1;
		} else {
			this->goalPos = pos;
		}
	}
	switch (DoActionMove(unit)) {
		case PF_UNREACHABLE:
			unit.Wait = 10;
			if (unit.Player->AiEnabled) {
				this->Range++;
				if (this->Range >= 5) {
					this->Range = 0;
					AiCanNotMove(unit);
				}
			}

			//Wyrmgus start
//			if (FindTerrainType(unit.Type->MovementMask, tile_flag::tree, 9999, *unit.Player, unit.tilePos, &pos)) {
			if (FindTerrainType(unit.Type->MovementMask, wyrmgus::resource::get_all()[this->CurrentResource], 9999, *unit.Player, unit.tilePos, &pos, z)) {
			//Wyrmgus end
				this->goalPos = pos;
				DebugPrint("Found a better place to harvest %d,%d\n" _C_ pos.x _C_ pos.y);
				// FIXME: can't this overflow? It really shouldn't, since
				// x and y are really supossed to be reachable, checked thorugh a flood fill.
				// I don't know, sometimes stuff happens.
				return 0;
			}
			return -1;
		case PF_REACHED:
			return 1;
		case PF_WAIT:
			if (unit.Player->AiEnabled) {
				this->Range++;
				if (this->Range >= 5) {
					this->Range = 0;
					AiCanNotMove(unit);
				}
			}
		default:
			return 0;
	}
}

/**
**  Move unit to unit resource.
**
**  @return      1 if reached, -1 if unreacheable, 0 if on the way.
*/
int COrder_Resource::MoveToResource_Unit(CUnit &unit)
{
	const CUnit *goal = this->get_goal();
	assert_throw(goal != nullptr);

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			return -1;
		case PF_REACHED:
			break;
		case PF_WAIT:
			if (unit.Player->AiEnabled) {
				this->Range++;
				if (this->Range >= 5) {
					this->Range = 0;
					AiCanNotMove(unit);
				}
			}
		default:
			// Goal gone or something.
			if (unit.Anim.Unbreakable || goal->IsVisibleAsGoal(*unit.Player)) {
				return 0;
			}
			break;
	}
	return 1;
}

/**
**  Move unit to resource.
**
**  @param unit  Pointer to unit.
**
**  @return      1 if reached, -1 if unreacheable, 0 if on the way.
*/
int COrder_Resource::MoveToResource(CUnit &unit)
{
	if (CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
		return MoveToResource_Terrain(unit);
	} else {
		return MoveToResource_Unit(unit);
	}
}

void COrder_Resource::UnitGotoGoal(CUnit &unit, CUnit *const goal, int state)
{
	if (this->get_goal() != goal) {
		this->set_goal(goal);
		if (goal) {
			this->goalPos.x = this->goalPos.y = -1;
		}
	}
	this->State = state;
	if (state == SUB_MOVE_TO_DEPOT || state == SUB_MOVE_TO_RESOURCE) {
		unit.pathFinderData->output.Cycles = 0; //moving counter
	}
}

/**
**  Start harvesting the resource.
**
**  @param unit  Pointer to unit.
**
**  @return      TRUE if ready, otherwise FALSE.
*/
int COrder_Resource::StartGathering(CUnit &unit)
{
	CUnit *goal = nullptr;
	const resource_info *res_info = unit.Type->get_resource_info(this->get_current_resource());
	assert_throw(!unit.get_pixel_offset().x());
	assert_throw(!unit.get_pixel_offset().y());

	//Wyrmgus start
	const resource *input_resource = wyrmgus::resource::get_all()[this->CurrentResource]->get_input_resource();
	if (input_resource != nullptr && unit.Player->get_resource(input_resource, resource_storage_type::both) == 0) { //if the resource requires an input, but there's none in store, don't gather
		const std::string &input_name = input_resource->get_identifier();
		const std::string &input_action_name = input_resource->get_action_name();
		unit.Player->Notify(_("Not enough %s... %s more %s."), _(input_name.c_str()), _(input_action_name.c_str()), _(input_name.c_str())); //added extra space to look better
		if (unit.Player == CPlayer::GetThisPlayer() && unit.Player->get_civilization() != nullptr) {
			const wyrmgus::sound *sound = unit.Player->get_civilization()->get_not_enough_resource_sound(input_resource);
			if (sound != nullptr) {
				PlayGameSound(sound, MaxSampleVolume);
			}
		}
		this->Finished = true;
		return 0;
	}
	//Wyrmgus end
		
	//Wyrmgus start
//	if (resinfo.TerrainHarvester) {
	if (CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		// This shouldn't happened?
#if 0
		if (!Map.IsTerrainResourceOnMap(unit.Orders->goalPos, this->CurrentResource)) {
			DebugPrint("Wood gone, just like that?\n");
			return 0;
		}
#endif
		UnitHeadingFromDeltaXY(unit, this->goalPos - unit.tilePos);
		if (res_info->WaitAtResource) {
			this->TimeToHarvest = std::max<int>(1, res_info->WaitAtResource * CPlayer::base_speed_factor / std::max(1, unit.Player->get_resource_harvest_speed(resource::get_all()[this->CurrentResource])));
		} else {
			this->TimeToHarvest = 1;
		}
		this->DoneHarvesting = 0;
		if (this->CurrentResource != unit.CurrentResource) {
			DropResource(unit);
			unit.CurrentResource = this->CurrentResource;
		}
		return 1;
	}

	goal = this->get_goal();

	// Target is dead, stop getting resources.
	//Wyrmgus start
//	if (!goal || goal->IsVisibleAsGoal(*unit.Player) == false) {
	//the goal could also have changed its given resource
	if (!goal || !goal->IsVisibleAsGoal(*unit.Player) || goal->GivesResource != this->CurrentResource) {
	//Wyrmgus end
		// Find an alternative, but don't look too far.
		this->goalPos.x = -1;
		this->goalPos.y = -1;
		//Wyrmgus start
//		if ((goal = UnitFindResource(unit, unit, 15, this->get_current_resource(), unit.Player->AiEnabled))) {
		if ((goal = UnitFindResource(unit, unit, 15, this->get_current_resource(), true, nullptr, true, false, false, false, true))) {
		//Wyrmgus end
			this->State = SUB_START_RESOURCE;
			this->set_goal(goal);
		} else {
			this->clear_goal();
			this->Finished = true;
		}
		return 0;
	}

	// FIXME: 0 can happen, if to near placed by map designer.
	assert_log(unit.MapDistanceTo(*goal) <= 1);

	// Update the heading of a harvesting unit to looks straight at the resource.
	//Wyrmgus start
//	UnitHeadingFromDeltaXY(unit, goal->tilePos - unit.tilePos + goal->Type->GetHalfTileSize());
	UnitHeadingFromDeltaXY(unit, PixelSize(PixelSize(goal->tilePos) * wyrmgus::defines::get()->get_tile_size()) - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) + goal->get_half_tile_pixel_size() - unit.get_half_tile_pixel_size());
	//Wyrmgus end

	// If resource is still under construction, wait!
	if ((goal->Type->MaxOnBoard && goal->Resource.Active >= goal->Type->MaxOnBoard)
		|| goal->CurrentAction() == UnitAction::Built) {
		// FIXME: Determine somehow when the resource will be free to use
		// FIXME: Could we somehow find another resource? Think minerals
		// FIXME: We should add a flag for that, and a limited range.
		// However the CPU usage is really low (no pathfinding stuff).
		unit.Wait = 10;
		return 0;
	}

	// Place unit inside the resource
	//Wyrmgus start
//	if (!resinfo.HarvestFromOutside) {
	if (!goal->Type->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
	//Wyrmgus end
		//Wyrmgus start
//		if (goal->Variable[MAXHARVESTERS_INDEX].Value == 0 || goal->Variable[MAXHARVESTERS_INDEX].Value > goal->InsideCount) {
		if (goal->Variable[MAXHARVESTERS_INDEX].Value == 0 || goal->Variable[MAXHARVESTERS_INDEX].Value > goal->Resource.Active) {
		//Wyrmgus end
			this->clear_goal();
			int selected = unit.Selected;
			unit.Remove(goal);
			if (selected && !Preference.DeselectInMine) {
				unit.Removed = 0;
				SelectUnit(unit);
				SelectionChanged();
				unit.Removed = 1;
			}
		//Wyrmgus start
//		} else if (goal->Variable[MAXHARVESTERS_INDEX].Value <= goal->InsideCount) {
		} else if (goal->Variable[MAXHARVESTERS_INDEX].Value <= goal->Resource.Active) {
		//Wyrmgus end
			//Resource is full, wait
			unit.Wait = 10;
			return 0;
		}
	}

	if (this->CurrentResource != unit.CurrentResource) {
		DropResource(unit);
		unit.CurrentResource = this->CurrentResource;
	}

	// Activate the resource
	goal->Resource.Active++;

	if (res_info->WaitAtResource) {
		//Wyrmgus start
//		this->TimeToHarvest = std::max<int>(1, resinfo.WaitAtResource * CPlayer::base_speed_factor / unit.Player->SpeedResourcesHarvest[resinfo.ResourceId]);
		int wait_at_resource = res_info->WaitAtResource;
		if (!goal->Type->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
			wait_at_resource = res_info->WaitAtResource * 100 / std::max(1, unit.get_resource_step(this->get_current_resource()));
		}
		this->TimeToHarvest = std::max<int>(1, wait_at_resource * CPlayer::base_speed_factor / std::max(1, unit.Player->get_resource_harvest_speed(res_info->get_resource()) + goal->Variable[TIMEEFFICIENCYBONUS_INDEX].Value));
		//Wyrmgus end
	} else {
		this->TimeToHarvest = 1;
	}
	this->DoneHarvesting = 0;
	return 1;
}

/**
**  Animate a unit that is harvesting
**
**  @param unit  Unit to animate
*/
static void AnimateActionHarvest(CUnit &unit)
{
	const CAnimation *animation = unit.get_animation_set()->get_harvest_animation(resource::get_all()[unit.CurrentResource]);
	assert_throw(animation != nullptr);
	UnitShowAnimation(unit, animation);
}

/**
**  Find something else to do when the resource is exhausted.
**  This is called from GatherResource when the resource is empty.
**
**  @param unit    pointer to harvester unit.
**  @param source  pointer to resource unit.
*/
void COrder_Resource::LoseResource(CUnit &unit, CUnit &source)
{
	CUnit *depot;
	const resource_info *res_info = unit.Type->get_resource_info(this->get_current_resource());

	//Wyrmgus start
	const wyrmgus::unit_type &source_type = *source.Type;
	
//	assert_throw((unit.Container == &source && !resinfo.HarvestFromOutside)
//		   || (!unit.Container && resinfo.HarvestFromOutside));
	assert_throw((unit.Container == &source && !source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value)
		   || (!unit.Container && source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value));
	//Wyrmgus end

	//Wyrmgus start
//	if (resinfo.HarvestFromOutside) {
	if (source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
	//Wyrmgus end
		this->clear_goal();
		--source.Resource.Active;
	}

	// Continue to harvest if we aren't fully loaded
	//Wyrmgus start
//	if (resinfo.HarvestFromOutside && unit.ResourcesHeld < resinfo.ResourceCapacity) {
	if (source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value && unit.ResourcesHeld < res_info->ResourceCapacity) {
	//Wyrmgus end
		//Wyrmgus start
//		CUnit *goal = UnitFindResource(unit, unit, 15, this->get_current_resource(), 1);
		CUnit *goal = UnitFindResource(unit, unit, 15, this->get_current_resource(), 1, nullptr, true, false, false, false, true);
		//Wyrmgus end

		if (goal) {
			this->goalPos.x = -1;
			this->goalPos.y = -1;
			this->State = SUB_START_RESOURCE;
			this->set_goal(goal);
			return;
		}
	}

	// If we are fully loaded first search for a depot.
	if (unit.ResourcesHeld && (depot = FindDeposit(unit, 1000, unit.get_current_resource()))) {
		if (unit.Container) {
			DropOutNearest(unit, depot->tilePos + depot->GetHalfTileSize(), &source);
		}
		// Remember where it mined, so it can look around for another resource.
		//
		//FIXME!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//unit.CurrentOrder()->Arg1.ResourcePos = (unit.X << 16) | unit.Y;
		this->DoneHarvesting = true;
		UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
		DebugPrint("%d: Worker %d report: Resource is exhausted, Going to depot\n"
				   _C_ unit.Player->get_index() _C_ UnitNumber(unit));
		return;
	}
	// No depot found, or harvester empty
	// Dump the unit outside and look for something to do.
	if (unit.Container) {
		//Wyrmgus start
//		assert_throw(!resinfo.HarvestFromOutside);
		assert_throw(!source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value);
		//Wyrmgus end
		DropOutOnSide(unit, LookingW, &source);
	}
	this->goalPos.x = -1;
	this->goalPos.y = -1;
	//use depot as goal
	//Wyrmgus start
//	depot = UnitFindResource(unit, unit, 15, this->get_current_resource(), unit.Player->AiEnabled);
	depot = UnitFindResource(unit, unit, 15, this->get_current_resource(), true, nullptr, true, false, false, false, true);
	//Wyrmgus end
	if (depot) {
		DebugPrint("%d: Worker %d report: Resource is exhausted, Found another resource.\n"
				   _C_ unit.Player->get_index() _C_ UnitNumber(unit));
		this->State = SUB_START_RESOURCE;
		this->set_goal(depot);
	} else {
		DebugPrint("%d: Worker %d report: Resource is exhausted, Just sits around confused.\n"
				   _C_ unit.Player->get_index() _C_ UnitNumber(unit));
		this->Finished = true;
	}
}



/**
**  Gather the resource
**
**  @param unit  Pointer to unit.
**
**  @return      non-zero if ready, otherwise zero.
*/
int COrder_Resource::GatherResource(CUnit &unit)
{
	CUnit *source = nullptr;
	const resource_info *res_info = unit.Type->get_resource_info(this->get_current_resource());
	int addload;

	//Wyrmgus start
	bool harvest_from_outside = (this->get_goal() && this->get_goal()->Type->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value);
//	if (resinfo.HarvestFromOutside || resinfo.TerrainHarvester) {
	if (harvest_from_outside || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		AnimateActionHarvest(unit);
	} else {
		unit.Anim.CurrAnim = nullptr;
	}

	this->TimeToHarvest--;

	if (this->DoneHarvesting) {
		//Wyrmgus start
//		assert_throw(resinfo.HarvestFromOutside || resinfo.TerrainHarvester);
		assert_throw(harvest_from_outside || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer));
		//Wyrmgus end
		return !unit.Anim.Unbreakable;
	}

	// Target gone?
	//Wyrmgus start
//	if (resinfo.TerrainHarvester && !CMap::get()->Field(this->goalPos)->IsTerrainResourceOnMap(this->CurrentResource)) {
	if (CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer) && CMap::get()->Field(this->goalPos, this->MapLayer)->get_resource() != wyrmgus::resource::get_all()[this->CurrentResource]) {
	//Wyrmgus end
		if (!unit.Anim.Unbreakable) {
			// Action now breakable, move to resource again.
			this->State = SUB_MOVE_TO_RESOURCE;
			// Give it some reasonable look while searching.
			// FIXME: which frame?
			unit.Frame = 0;
		}
		return 0;
		// No wood? Freeze!!!
	}

	while (!this->DoneHarvesting && this->TimeToHarvest < 0) {
		//FIXME: rb - how should it look for WaitAtResource == 0
		if (res_info->WaitAtResource) {
			// Wyrmgus start
//			this->TimeToHarvest += std::max<int>(1, resinfo.WaitAtResource * CPlayer::base_speed_factor / unit.Player->SpeedResourcesHarvest[resinfo.ResourceId]);
			int wait_at_resource = res_info->WaitAtResource;
			int resource_harvest_speed = unit.Player->get_resource_harvest_speed(res_info->get_resource());
			if (!CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer) && !harvest_from_outside) {
				wait_at_resource = res_info->WaitAtResource * 100 / std::max(1, unit.get_resource_step(this->get_current_resource()));
			}
			if (this->get_goal()) {
				resource_harvest_speed += this->get_goal()->Variable[TIMEEFFICIENCYBONUS_INDEX].Value;
			}
			this->TimeToHarvest += std::max<int>(1, wait_at_resource * CPlayer::base_speed_factor / std::max(1, resource_harvest_speed));
			//Wyrmgus end
		} else {
			this->TimeToHarvest += 1;
		}

		// Calculate how much we can load.
		if (unit.get_resource_step(this->get_current_resource()) && (harvest_from_outside || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer))) {
			addload = unit.get_resource_step(this->get_current_resource());
		} else {
			//Wyrmgus start
//			addload = resinfo.ResourceCapacity;
			if (this->CurrentResource == TradeCost) { // the load added when trading depends on the price difference between the two players
				addload = unit.Player->ConvergePricesWith(*unit.Container->Player, res_info->ResourceCapacity);
				addload = std::max(10, addload);

				this->trade_partner = unit.Container->Player;
			} else {
				addload = std::min(100, res_info->ResourceCapacity);
			}
			//Wyrmgus end
		}
		// Make sure we don't bite more than we can chew.
		if (unit.ResourcesHeld + addload > res_info->ResourceCapacity) {
			addload = res_info->ResourceCapacity - unit.ResourcesHeld;
		}

		//Wyrmgus start
//		if (resinfo.TerrainHarvester) {
		if (CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
			//Wyrmgus start
			wyrmgus::tile &mf = *CMap::get()->Field(this->goalPos, this->MapLayer);
			if (addload > mf.get_value()) {
				addload = mf.get_value();
			}
			mf.change_value(-addload);
			//Wyrmgus end
			//Wyrmgus start
//			unit.ResourcesHeld += addload;
			unit.ChangeResourcesHeld(addload);
			//Wyrmgus end
			
			//Wyrmgus start
//			if (addload && unit.ResourcesHeld == resinfo.ResourceCapacity) {
			if (mf.get_value() <= 0) {
			//Wyrmgus end
				//Wyrmgus start
//				CMap::get()->ClearWoodTile(this->goalPos);
				CMap::get()->ClearOverlayTile(this->goalPos, this->MapLayer);
				//Wyrmgus end
			}
		} else {
			//Wyrmgus start
//			if (resinfo.HarvestFromOutside) {
			if (harvest_from_outside) {
			//Wyrmgus end
				source = this->get_goal();
			} else {
				source = unit.Container;
			}

			assert_throw(source != nullptr);
			assert_throw(source->ResourcesHeld <= 655350);
			//Wyrmgus start
			UpdateUnitVariables(*source); //update resource source's variables
			//Wyrmgus end
			bool is_visible = source->IsVisibleAsGoal(*unit.Player);
			// Target is not dead, getting resources.
			if (is_visible) {
				// Don't load more that there is.
				addload = std::min(source->ResourcesHeld, addload);
				//Wyrmgus start
//				unit.ResourcesHeld += addload;
//				source->ResourcesHeld -= addload;
				const resource *input_resource = wyrmgus::resource::get_all()[this->CurrentResource]->get_input_resource();
				if (input_resource != nullptr) {
					addload = std::min(unit.Player->get_resource(input_resource, resource_storage_type::both), addload);
					
					if (!addload) {
						const char *input_name = input_resource->get_identifier().c_str();
						const char *input_action_name = input_resource->get_action_name().c_str();
						unit.Player->Notify(_("Not enough %s... %s more %s."), _(input_name), _(input_action_name), _(input_name));
						if (unit.Player == CPlayer::GetThisPlayer() && unit.Player->get_civilization() != nullptr) {
							const wyrmgus::sound *sound = unit.Player->get_civilization()->get_not_enough_resource_sound(input_resource);
							if (sound != nullptr) {
								PlayGameSound(sound, MaxSampleVolume);
							}
						}

						if (unit.Container) {
							DropOutOnSide(unit, LookingW, source);
						}
						this->Finished = true;
						return 0;
					}
					
					unit.Player->change_resource(input_resource, -addload, true);
				}
				unit.ChangeResourcesHeld(addload);
				if (!source->Type->BoolFlag[INEXHAUSTIBLE_INDEX].value) {
					source->ChangeResourcesHeld(-addload);
				}
				//Wyrmgus end
			}

			// End of resource: destroy the resource.
			// FIXME: implement depleted resources.
			if ((!is_visible) || (source->ResourcesHeld == 0)) {
				if (unit.Anim.Unbreakable) {
					return 0;
				}
				DebugPrint("%d: Worker %d report: Resource is destroyed\n" _C_ unit.Player->get_index() _C_ UnitNumber(unit));
				const bool dead = source->IsAlive() == false;

				// Improved version of DropOutAll that makes workers go to the depot.
				LoseResource(unit, *source);
				for (const std::shared_ptr<wyrmgus::unit_ref> &uins_ref : source->Resource.Workers) {
					CUnit *uins = uins_ref->get();
					if (uins != &unit && uins->CurrentOrder()->Action == UnitAction::Resource) {
						COrder_Resource &order = *static_cast<COrder_Resource *>(uins->CurrentOrder());
						if (!uins->Anim.Unbreakable && order.State == SUB_GATHER_RESOURCE) {
							order.LoseResource(*uins, *source);
						}
					}
				}

				// Don't destroy the resource twice.
				// This only happens when it's empty.
				if (!dead) {
					if (Preference.MineNotifications
						&& unit.Player == CPlayer::GetThisPlayer()
						&& source->Variable[GIVERESOURCE_INDEX].Max > (wyrmgus::resource::get_all()[this->CurrentResource]->get_default_income() * 10)) {
							unit.Player->Notify(NotifyYellow, source->tilePos, source->MapLayer->ID, _("Our %s has been depleted!"), source->Type->get_name().c_str());
					}
					LetUnitDie(*source);
					// FIXME: make the workers inside look for a new resource.
				}
				source = nullptr;
				return 0;
			}
		}
		//Wyrmgus start
//		if (resinfo.TerrainHarvester) {
		if (CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
			if (unit.ResourcesHeld == res_info->ResourceCapacity) {
				// Mark as complete.
				this->DoneHarvesting = true;
			}
			return 0;
		} else {
			//Wyrmgus start
//			if (resinfo.HarvestFromOutside) {
			if (harvest_from_outside) {
			//Wyrmgus end
				if ((unit.ResourcesHeld == res_info->ResourceCapacity) || (source == nullptr)) {
					// Mark as complete.
					this->DoneHarvesting = true;
				}
				return 0;
			} else {
				return unit.ResourcesHeld == res_info->ResourceCapacity && source;
			}
		}
	}
	return 0;
}

int GetNumWaitingWorkers(const CUnit &mine)
{
	int ret = 0;

	for (const std::shared_ptr<wyrmgus::unit_ref> &worker_ref : mine.Resource.Workers) {
		const CUnit *worker = worker_ref->get();
		assert_throw(worker->CurrentAction() == UnitAction::Resource);
		COrder_Resource &order = *static_cast<COrder_Resource *>(worker->CurrentOrder());

		if (order.IsGatheringWaiting()) {
			ret++;
		}
	}

	return ret;
}

/**
**  Stop gathering from the resource, go home.
**
**  @param unit  Pointer to unit.
**
**  @return      True if ready, or false otherwise.
*/
bool COrder_Resource::StopGathering(CUnit &unit)
{
	CUnit *source = nullptr;

	//Wyrmgus start
//	if (!resinfo.TerrainHarvester) {
	if (!CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		//Wyrmgus start
//		if (resinfo.HarvestFromOutside) {
		if (this->get_goal() != nullptr && this->get_goal()->Type->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
		//Wyrmgus end
			if (this->get_goal()->IsAlive()) {
				source = this->get_goal();
			}

			this->clear_goal();
		} else {
			source = unit.Container;
		}

		if (source != nullptr) {
			source->Resource.Active--;
			assert_log(source->Resource.Active >= 0);
			//Store resource position.
			this->Resource.Mine = source->acquire_ref();

			if (Preference.MineNotifications && unit.Player == CPlayer::GetThisPlayer()
				&& source->IsAlive()
				&& !source->MineLow
				&& source->Variable[GIVERESOURCE_INDEX].Max > 0
				&& source->ResourcesHeld * 100 / source->Variable[GIVERESOURCE_INDEX].Max <= 10
				&& source->Variable[GIVERESOURCE_INDEX].Max > (wyrmgus::resource::get_all()[this->CurrentResource]->get_default_income() * 10)) {
				//Wyrmgus start
//				unit.Player->Notify(NotifyYellow, source->tilePos, _("%s is running low!"), source->Type->Name.c_str());
				unit.Player->Notify(NotifyYellow, source->tilePos, source->MapLayer->ID, _("Our %s is nearing depletion!"), source->Type->get_name().c_str());
				//Wyrmgus end
				source->MineLow = 1;
			}

			if (source->Type->MaxOnBoard) {
				int count = 0;
				CUnit *next = nullptr;
				for (const std::shared_ptr<wyrmgus::unit_ref> &worker_ref : source->Resource.Workers) {
					CUnit *worker = worker_ref->get();
					assert_throw(worker->CurrentAction() == UnitAction::Resource);
					COrder_Resource &order = *static_cast<COrder_Resource *>(worker->CurrentOrder());
					if (worker != &unit && order.IsGatheringWaiting()) {
						count++;
						if (next) {
							if (next->Wait > worker->Wait) {
								next = worker;
							}
						} else {
							next = worker;
						}
					}
				}
				if (next != nullptr) {
					if (!unit.Player->AiEnabled) {
						DebugPrint("%d: Worker %d report: Unfreez resource gathering of %d <Wait %d> on %d [Assigned: %d Waiting %d].\n"
							_C_ unit.Player->get_index() _C_ UnitNumber(unit)
							_C_ UnitNumber(*next) _C_ next->Wait
							_C_ UnitNumber(*source) _C_ source->Resource.Workers.size()
							_C_ count);
					}
					next->Wait = 0;
					//source->Data.Resource.Waiting = count - 1;
					//assert_throw(source->Data.Resource.Assigned >= source->Data.Resource.Waiting);
					//StartGathering(next);
				}
			}
		}
	} else {
		// Store resource position.
		this->Resource.Pos = unit.tilePos;
		this->Resource.MapLayer = unit.MapLayer->ID;
		assert_throw(this->Resource.Mine == nullptr);
	}

#ifdef DEBUG
	if (!unit.ResourcesHeld) {
		DebugPrint("Unit %d is empty???\n" _C_ UnitNumber(unit));
	}
#endif

	// Find and send to resource deposit.
	CUnit *depot = FindDeposit(unit, 1000, unit.get_current_resource());
	if (!depot || !unit.ResourcesHeld || this->Finished) {
		//Wyrmgus start
//		if (!(resinfo.HarvestFromOutside || resinfo.TerrainHarvester)) {
		if (!((source && source->Type->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer))) {
		//Wyrmgus end
			assert_throw(unit.Container != nullptr);
			DropOutOnSide(unit, LookingW, source);
		}
		CUnit *mine = this->Resource.get_mine();

		if (mine) {
			unit.DeAssignWorkerFromMine(*mine);
			this->Resource.Mine.reset();
		}

		DebugPrint("%d: Worker %d report: Can't find a resource [%d] deposit.\n"
				   _C_ unit.Player->get_index() _C_ UnitNumber(unit) _C_ unit.CurrentResource);
		this->Finished = true;
		return false;
	} else {
		//Wyrmgus start
//		if (!(resinfo.HarvestFromOutside || resinfo.TerrainHarvester)) {
		if (!((source && source->Type->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) || CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer))) {
		//Wyrmgus end
			assert_throw(unit.Container != nullptr);
			DropOutNearest(unit, depot->tilePos + depot->GetHalfTileSize(), source);
		}
		UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
	}

	if (IsOnlySelected(unit)) {
		SelectedUnitChanged();
	}

	return true;
}

/**
**  Move to resource depot
**
**  @param unit  Pointer to unit.
**
**  @return      TRUE if reached, otherwise FALSE.
*/
int COrder_Resource::MoveToDepot(CUnit &unit)
{
	const resource_info *res_info = unit.Type->get_resource_info(this->get_current_resource());

	assert_throw(this->get_goal() != nullptr);

	CUnit &goal = *this->get_goal();
	CPlayer &player = *unit.Player;

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			return -1;
		case PF_REACHED:
			break;
		case PF_WAIT:
			if (unit.Player->AiEnabled) {
				this->Range++;
				if (this->Range >= 5) {
					this->Range = 0;
					AiCanNotMove(unit);
				}
			}
		default:
			if (unit.Anim.Unbreakable || goal.IsVisibleAsGoal(player)) {
				return 0;
			}
			break;
	}

	//
	// Target is dead, stop getting resources.
	//
	if (!goal.IsVisibleAsGoal(player)) {
		DebugPrint("%d: Worker %d report: Destroyed depot\n" _C_ player.get_index() _C_ UnitNumber(unit));

		unit.CurrentOrder()->clear_goal();

		CUnit *depot = FindDeposit(unit, 1000, unit.get_current_resource());

		if (depot) {
			UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
			DebugPrint("%d: Worker %d report: Going to new deposit.\n" _C_ player.get_index() _C_ UnitNumber(unit));
		} else {
			DebugPrint("%d: Worker %d report: Can't find a new resource deposit.\n"
					   _C_ player.get_index() _C_ UnitNumber(unit));

			// FIXME: perhaps we should choose an alternative
			this->Finished = true;
		}
		return 0;
	}

	// If resource depot is still under construction, wait!
	if (goal.CurrentAction() == UnitAction::Built) {
		unit.Wait = 10;
		return 0;
	}

	this->clear_goal();
	unit.Wait = res_info->WaitAtDepot;

	// Place unit inside the depot
	if (unit.Wait) {
		int selected = unit.Selected;
		unit.Remove(&goal);
		if (selected && !Preference.DeselectInMine) {
			unit.Removed = 0;
			SelectUnit(unit);
			SelectionChanged();
			unit.Removed = 1;
		}
		unit.Anim.CurrAnim = nullptr;
	}

	// Update resource.
	const resource *final_resource = resource::get_all()[this->CurrentResource]->get_final_resource();
	const int processed_resource_change = unit.ResourcesHeld * player.get_income(resource::get_all()[this->CurrentResource]) / 100;
	int final_resource_change = processed_resource_change * resource::get_all()[this->CurrentResource]->get_final_resource_conversion_rate() / 100;
	
	if (player.AiEnabled) {
		if (GameSettings.Difficulty == DifficultyEasy) {
			final_resource_change /= 2;
		} else if (GameSettings.Difficulty == DifficultyHard) {
			final_resource_change *= 3;
			final_resource_change /= 4;
		} else if (GameSettings.Difficulty == DifficultyBrutal) {
			final_resource_change *= 2;
		}
	}
	
	player.change_resource(final_resource, final_resource_change, true);
	player.change_resource_total(final_resource, final_resource_change);
	player.pay_overlord_tax(final_resource, final_resource_change);
	
	//give XP to the worker according to how much was gathered, based on their base price in relation to gold
	int xp_gained = unit.ResourcesHeld;
	xp_gained /= 20;
	unit.ChangeExperience(xp_gained);

	if (this->CurrentResource == TradeCost && this->trade_partner != nullptr) {
		if (this->trade_partner != unit.Player && !this->trade_partner->is_neutral_player()) {
			player.add_recent_trade_partner(this->trade_partner);
		}
	}
	
	//update quests
	player.on_resource_gathered(final_resource, final_resource_change);
	if (final_resource->get_index() != this->CurrentResource) {
		player.on_resource_gathered(wyrmgus::resource::get_all()[this->CurrentResource], processed_resource_change);
	}
	
	//Wyrmgus start
//	unit.ResourcesHeld = 0;
	unit.SetResourcesHeld(0);
	//Wyrmgus end
	unit.CurrentResource = 0;

	if (unit.Wait) {
		//Wyrmgus start
//		unit.Wait /= std::max(1, unit.Player->SpeedResourcesReturn[resinfo->ResourceId] / CPlayer::base_speed_factor);
		unit.Wait /= std::max(1, (unit.Player->get_resource_return_speed(res_info->get_resource()) + goal.Variable[TIMEEFFICIENCYBONUS_INDEX].Value) / CPlayer::base_speed_factor);
		//Wyrmgus end
		if (unit.Wait) {
			unit.Wait--;
		}
	}
	return 1;
}

/**
**  Wait in depot, for the resources stored.
**
**  @param unit  Pointer to unit.
**
**  @return      TRUE if ready, otherwise FALSE.
*/
bool COrder_Resource::WaitInDepot(CUnit &unit)
{
	const resource_info *res_info = unit.Type->get_resource_info(this->get_current_resource());
	const CUnit *depot = ResourceDepositOnMap(unit.tilePos, res_info->get_resource()->get_index(), unit.MapLayer->ID);

	//assert_throw(depot);

	// Range hardcoded. don't stray too far though
	//Wyrmgus start
//	if (resinfo.TerrainHarvester) {
	if (this->Resource.get_mine() == nullptr || this->Resource.get_mine()->Type == nullptr) {
	//Wyrmgus end
		Vec2i pos = this->Resource.Pos;
		int z = this->Resource.MapLayer = unit.MapLayer->ID;

		//Wyrmgus start
//		if (FindTerrainType(unit.Type->MovementMask, tile_flag::tree, 10, *unit.Player, pos, &pos)) {
		if (FindTerrainType(unit.Type->MovementMask, wyrmgus::resource::get_all()[this->CurrentResource], 10, *unit.Player, pos, &pos, z)) {
		//Wyrmgus end
			if (depot) {
				DropOutNearest(unit, pos, depot);
			}
			this->goalPos = pos;
			//Wyrmgus start
			this->MapLayer = z;
			//Wyrmgus end
		} else {
			if (depot) {
				DropOutOnSide(unit, LookingW, depot);
			}
			this->Finished = true;
			return false;
		}
	} else {
		static constexpr unsigned int too_many_workers = 15;
		CUnit *mine = this->Resource.get_mine();
		static constexpr int range = 15;
		CUnit *newdepot = nullptr;
		CUnit *goal = nullptr;
		const bool longWay = unit.pathFinderData->output.Cycles > 500;

		//Wyrmgus start
//		if (unit.Player->AiEnabled && AiPlayer && AiPlayer->BuildDepots) {
		if (depot && unit.Player->AiEnabled && AiPlayer && AiPlayer->BuildDepots) { //check if the depot is valid
		//Wyrmgus end
			// If the depot is overused, we need first to try to switch into another depot
			// Use depot's ref counter for that
			//Wyrmgus start
//			if (longWay || !mine || (depot->get_ref_count() > too_many_workers)) {
			if (longWay || !mine || mine->Type == nullptr || (depot->get_ref_count() > too_many_workers)) {
			//Wyrmgus end
				newdepot = AiGetSuitableDepot(unit, *depot, &goal);
				if (newdepot == nullptr && longWay && unit.Player->NumTownHalls > 0) {
					// We need a new depot
					AiNewDepotRequest(unit);
				}
			}
		}

		// If goal is not null, then we got it in AiGetSuitableDepot
		if (!goal) {
			if (mine != nullptr && mine->IsAlive() && mine->GivesResource == this->CurrentResource) {
				goal = mine;
			} else {
				const CUnit *start_unit = nullptr;
				if (newdepot != nullptr) {
					start_unit = newdepot;
				} else if (depot != nullptr) {
					start_unit = depot;
				}

				//Wyrmgus start
	//			goal = UnitFindResource(unit, newdepot ? *newdepot : (mine ? *mine : unit), mine ? range : 1000,
	//									this->CurrentResource, unit.Player->AiEnabled, newdepot ? newdepot : depot);
				goal = UnitFindResource(unit, start_unit ? *start_unit : unit, 1000, this->get_current_resource(), true, newdepot ? newdepot : depot, true, false, false, false, true);
				//Wyrmgus end
			}
		}

		if (goal) {
			if (depot) {
				DropOutNearest(unit, goal->tilePos + goal->GetHalfTileSize(), depot);
			}

			if (goal != mine) {
				if (mine) {
					unit.DeAssignWorkerFromMine(*mine);
				}
				unit.AssignWorkerToMine(*goal);
				this->Resource.Mine = goal->acquire_ref();
			}
			this->set_goal(goal);
			this->goalPos.x = this->goalPos.y = -1;
		} else {
#ifdef DEBUG
			const Vec2i &pos = mine ? mine->tilePos : unit.tilePos;
			DebugPrint("%d: Worker %d report: [%d,%d] Resource gone near [%d,%d] in range %d. Sit and play dumb.\n"
					   _C_ unit.Player->get_index() _C_ UnitNumber(unit)
					   _C_ unit.tilePos.x _C_ unit.tilePos.y
					   _C_ pos.x _C_ pos.y _C_ range);
#endif // DEBUG
			if (depot) {
				DropOutOnSide(unit, LookingW, depot);
			}
			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
				this->Resource.Mine.reset();
			}
			this->Finished = true;
			return false;
		}
	}
	return true;
}

void COrder_Resource::DropResource(CUnit &unit)
{
	if (unit.CurrentResource) {
		//Wyrmgus start
//		if (!resinfo.TerrainHarvester) {
		if (!CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
			CUnit *mine = this->Resource.get_mine();
			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
			}
		}
		//fast clean both resource data: pos and mine
		this->Resource.Mine.reset();
		unit.CurrentResource = 0;
		//Wyrmgus start
//		unit.ResourcesHeld = 0;
		unit.SetResourcesHeld(0);
		//Wyrmgus end
	}
}

/**
**  Give up on gathering.
**
**  @param unit  Pointer to unit.
*/
void COrder_Resource::ResourceGiveUp(CUnit &unit)
{
	DebugPrint("%d: Worker %d report: Gave up on resource gathering.\n" _C_ unit.Player->get_index() _C_ UnitNumber(unit));
	if (this->has_goal()) {
		DropResource(unit);
		this->clear_goal();
	}
	this->Finished = true;
}

/**
**  Try to find another resource before give up
**
**  return false if failed, true otherwise.
*/

bool COrder_Resource::FindAnotherResource(CUnit &unit)
{
	if (this->CurrentResource) {
		const resource_info *res_info = unit.Type->get_resource_info(this->get_current_resource());
		if (res_info != nullptr) {
			//Wyrmgus start
	//		if (!resinfo.TerrainHarvester) {
			if (!CMap::get()->Info->IsPointOnMap(this->goalPos, this->MapLayer)) {
			//Wyrmgus end
				//Wyrmgus start
//				CUnit *newGoal = UnitFindResource(unit, this->Resource.get_mine() ? *this->Resource.get_mine() : unit, 8, this->CurrentResource, 1);
				CUnit *newGoal = UnitFindResource(unit, (this->Resource.get_mine() && this->Resource.get_mine()->Type) ? *this->Resource.get_mine() : unit, 8, this->get_current_resource(), 1, nullptr, true, false, false, false, true);
				//Wyrmgus end

				if (newGoal) {
					CUnit *mine = this->Resource.get_mine();
					if (mine) {
						unit.DeAssignWorkerFromMine(*mine);
					}
					unit.AssignWorkerToMine(*newGoal);
					this->Resource.Mine = newGoal->acquire_ref();
					this->goalPos.x = -1;
					this->goalPos.y = -1;
					this->State = SUB_MOVE_TO_RESOURCE;
					this->set_goal(newGoal);
					return true;
				}
			} else {
				Vec2i resPos;
				//Wyrmgus start
//				if (FindTerrainType(unit.Type->MovementMask, tile_flag::tree, 8, *unit.Player, unit.tilePos, &resPos)) {
				if (FindTerrainType(unit.Type->MovementMask, wyrmgus::resource::get_all()[this->CurrentResource], 8, *unit.Player, unit.tilePos, &resPos, unit.MapLayer->ID)) {
				//Wyrmgus end
					this->goalPos = resPos;
					this->MapLayer = unit.MapLayer->ID;
					this->State = SUB_MOVE_TO_RESOURCE;
					DebugPrint("Found a better place to harvest %d,%d\n" _C_ resPos.x _C_ resPos.y);
					return true;
				}
			}
		}
	}
	return false;
}

/**
**  Initialize
**
**  return false if action is canceled, true otherwise.
*/
bool COrder_Resource::ActionResourceInit(CUnit &unit)
{
	assert_throw(this->State == SUB_START_RESOURCE);

	this->Range = 0;
	CUnit *goal = this->get_goal();
	CUnit *mine = this->Resource.get_mine();

	if (mine) {
		unit.DeAssignWorkerFromMine(*mine);
		this->Resource.Mine.reset();
	}
	if (goal != nullptr) {
		if (!goal->IsAlive()) {
			return false;
		}

		if (goal->CurrentAction() != UnitAction::Built) {
			unit.AssignWorkerToMine(*goal);
			this->Resource.Mine = goal->acquire_ref();
		}
	}

	UnitGotoGoal(unit, goal, SUB_MOVE_TO_RESOURCE);
	return true;
}

/**
**  Control the unit action: getting a resource.
**
**  This the generic function for oil, gold, ...
**
**  @param unit  Pointer to unit.
*/
void COrder_Resource::Execute(CUnit &unit)
{
	// can be different by Cloning (trained unit)...
	this->worker = unit.acquire_ref();

	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Still.get());
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}

	// Let's start mining.
	if (this->State == SUB_START_RESOURCE) {
		if (ActionResourceInit(unit) == false) {
			ResourceGiveUp(unit);
			return;
		}
	}

	// Move to the resource location.
	if (SUB_MOVE_TO_RESOURCE <= this->State && this->State < SUB_UNREACHABLE_RESOURCE) {
		const int ret = MoveToResource(unit);

		switch (ret) {
			case -1: { // Can't Reach
				this->State++;
				unit.Wait = 5;
				return;
			}
			case 1: { // Reached
				this->State = SUB_START_GATHERING;
				break;
			}
			case 0: // Move along.
				return;
			default: {
				assert_throw(false);
				break;
			}
		}
	}

	// Resource seems to be unreachable
	if (this->State == SUB_UNREACHABLE_RESOURCE) {
		if (this->FindAnotherResource(unit) == false) {
			ResourceGiveUp(unit);
			return;
		}
	}

	// Start gathering the resource
	if (this->State == SUB_START_GATHERING) {
		if (StartGathering(unit)) {
			this->State = SUB_GATHER_RESOURCE;
		} else {
			return;
		}
	}

	// Gather the resource.
	if (this->State == SUB_GATHER_RESOURCE) {
		if (GatherResource(unit)) {
			this->State = SUB_STOP_GATHERING;
		} else {
			return;
		}
	}

	// Stop gathering the resource.
	if (this->State == SUB_STOP_GATHERING) {
		if (StopGathering(unit)) {
			this->State = SUB_MOVE_TO_DEPOT;
			unit.pathFinderData->output.Cycles = 0; //moving counter
		} else {
			return;
		}
	}

	// Move back home.
	if (SUB_MOVE_TO_DEPOT <= this->State && this->State < SUB_UNREACHABLE_DEPOT) {
		const int ret = MoveToDepot(unit);

		switch (ret) {
			case -1: { // Can't Reach
				this->State++;
				unit.Wait = 5;
				return;
			}
			case 1: { // Reached
				this->State = SUB_RETURN_RESOURCE;
				return;
			}
			case 0: // Move along.
				return;
			default: {
				assert_throw(false);
				return;
			}
		}
	}

	// Depot seems to be unreachable
	if (this->State == SUB_UNREACHABLE_DEPOT) {
		ResourceGiveUp(unit);
		return;
	}

	// Unload resources at the depot.
	if (this->State == SUB_RETURN_RESOURCE) {
		if (WaitInDepot(unit)) {
			this->State = SUB_START_RESOURCE;

			// It's posible, though very rare that the unit's goal blows up
			// this cycle, but after this unit. Thus, next frame the unit
			// will start mining a destroyed site. If, on the otherhand we
			// are already in SUB_MOVE_TO_RESOURCE then we can handle it.
			// So, we pass through SUB_START_RESOURCE the very instant it
			// goes out of the depot.
			//HandleActionResource(order, unit);
		}
	}
}

CUnit *COrder_Resource::get_worker() const
{
	if (this->worker == nullptr) {
		return nullptr;
	}

	return this->worker->get();
}

CUnit *COrder_Resource::get_depot() const
{
	if (this->Depot == nullptr) {
		return nullptr;
	}

	return this->Depot->get();
}

CUnit *COrder_Resource::Resource::get_mine() const
{
	if (this->Mine == nullptr) {
		return nullptr;
	}

	return this->Mine->get();
}
