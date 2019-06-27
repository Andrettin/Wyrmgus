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
//      (c) Copyright 2001-2019 by Crestez Leonard, Jimmy Salmon and Andrettin
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

#include "action/action_resource.h"

#include "ai/ai.h"
#include "ai/ai_local.h"
#include "animation/animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "economy/resource.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder/pathfinder.h"
#include "player.h"
#include "quest/quest.h"
#include "script.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound/sound.h"
#include "translate.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

constexpr int SUB_START_RESOURCE = 0;
constexpr int SUB_MOVE_TO_RESOURCE = 5;
constexpr int SUB_UNREACHABLE_RESOURCE = 31;
constexpr int SUB_START_GATHERING = 55;
constexpr int SUB_GATHER_RESOURCE = 60;
constexpr int SUB_STOP_GATHERING = 65;
constexpr int SUB_MOVE_TO_DEPOT = 70;
constexpr int SUB_UNREACHABLE_DEPOT = 100;
constexpr int SUB_RETURN_RESOURCE = 120;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

class NearReachableTerrainFinder
{
public:
	NearReachableTerrainFinder(const CPlayer &player, const int maxDist, const int movemask, const int resource, Vec2i *resPos, const int z) :
		player(player), maxDist(maxDist), movemask(movemask), resource(resource), resPos(resPos), z(z) {}
	VisitResult Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from);
private:
	const CPlayer &player;
	int maxDist;
	int movemask;
	//Wyrmgus start
//	int resmask;
	int resource;
	int z;
	//Wyrmgus end
	Vec2i *resPos;
};

VisitResult NearReachableTerrainFinder::Visit(TerrainTraversal &terrainTraversal, const Vec2i &pos, const Vec2i &from)
{
	if (!CMap::Map.Field(pos, z)->playerInfo.IsTeamExplored(player)) {
		return VisitResult_DeadEnd;
	}

	//Wyrmgus start
	if (CMap::Map.Field(pos, z)->Owner != -1 && CMap::Map.Field(pos, z)->Owner != player.GetIndex() && !CPlayer::Players[CMap::Map.Field(pos, z)->Owner]->HasNeutralFactionType() && !player.HasNeutralFactionType()) {
		return VisitResult_DeadEnd;
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
		return VisitResult_Finished;
	}
	//Wyrmgus start
//	if (CMap::Map.Field(pos)->CheckMask(resmask)) { // reachable
	if (CMap::Map.Field(pos, z)->GetResource() == resource) { // reachable
	//Wyrmgus end
		if (terrainTraversal.Get(pos) <= maxDist) {
			return VisitResult_Ok;
		} else {
			return VisitResult_DeadEnd;
		}
	} else { // unreachable
		return VisitResult_DeadEnd;
	}
}

//Wyrmgus start
//static bool FindNearestReachableTerrainType(int movemask, int resmask, int range,
static bool FindNearestReachableTerrainType(int movemask, int resource, int range,
//Wyrmgus end
											//Wyrmgus start
//											const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos)
											const CPlayer &player, const Vec2i &startPos, Vec2i *terrainPos, int z)
											//Wyrmgus end
{
	TerrainTraversal terrainTraversal;

	//Wyrmgus start
//	terrainTraversal.SetSize(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
	terrainTraversal.SetSize(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
	//Wyrmgus end
	terrainTraversal.Init();

	//Wyrmgus start
//	Assert(CMap::Map.Field(startPos)->CheckMask(resmask));
	Assert(CMap::Map.Field(startPos, z)->GetResource() == resource);
	//Wyrmgus end
	terrainTraversal.PushPos(startPos);

	//Wyrmgus start
//	NearReachableTerrainFinder nearReachableTerrainFinder(player, range, movemask, resmask, terrainPos);
	NearReachableTerrainFinder nearReachableTerrainFinder(player, range, movemask, resource, terrainPos, z);
	//Wyrmgus end

	return terrainTraversal.Run(nearReachableTerrainFinder);
}



//Wyrmgus start
///* static */ COrder *COrder::NewActionResource(CUnit &harvester, const Vec2i &pos)
/* static */ COrder *COrder::NewActionResource(CUnit &harvester, const Vec2i &pos, int z)
//Wyrmgus end
{
	COrder_Resource *order = new COrder_Resource(harvester);
	Vec2i ressourceLoc;

	if (CMap::Map.Info.IsPointOnMap(pos, z) && CMap::Map.Field(pos, z)->GetResource() != -1) {
		order->CurrentResource = CMap::Map.Field(pos, z)->GetResource();
		//  Find the closest resource tile next to a tile where the unit can move
		if (!FindNearestReachableTerrainType(harvester.GetType()->MovementMask, CMap::Map.Field(pos, z)->GetResource(), 20, *harvester.GetPlayer(), pos, &ressourceLoc, z)) {
			DebugPrint("FIXME: Give up???\n");
			ressourceLoc = pos;
		}
	}
	order->goalPos = ressourceLoc;
	order->MapLayer = z;
		
	return order;
}

/* static */ COrder *COrder::NewActionResource(CUnit &harvester, CUnit &mine)
{
	COrder_Resource *order = new COrder_Resource(harvester);

	order->SetGoal(&mine);
	order->Resource.Mine = &mine;
	order->Resource.Pos = mine.GetTilePos() + mine.GetHalfTileSize();
	order->Resource.MapLayer = mine.GetMapLayer()->GetIndex();
	order->CurrentResource = mine.GivesResource;
	return order;
}

/* static */ COrder *COrder::NewActionReturnGoods(CUnit &harvester, CUnit *depot)
{
	COrder_Resource *order = new COrder_Resource(harvester);

	// Destination could be killed. NETWORK!
	if (depot && depot->Destroyed) {
		depot = nullptr;
	}
	order->CurrentResource = harvester.GetCurrentResource();
	order->DoneHarvesting = true;

	if (depot == nullptr) {
		depot = FindDeposit(harvester, 1000, harvester.GetCurrentResource());
	}
	if (depot) {
		order->Depot = depot;
		order->UnitGotoGoal(harvester, depot, SUB_MOVE_TO_DEPOT);
	} else {
		order->State = SUB_UNREACHABLE_DEPOT;
		order->goalPos = harvester.GetTilePos();
		order->MapLayer = harvester.GetMapLayer()->GetIndex();
	}
	return order;
}


Vec2i COrder_Resource::GetHarvestLocation() const
{
	//Wyrmgus start
//	if (this->Resource.Mine != nullptr) {
	if (this->Resource.Mine != nullptr && this->Resource.Mine->GetType() != nullptr) {
	//Wyrmgus end
		return this->Resource.Mine->GetTilePos();
	} else {
		return this->Resource.Pos;
	}
}

//Wyrmgus start
int COrder_Resource::GetHarvestMapLayer() const
{
	//Wyrmgus start
//	if (this->Resource.Mine != nullptr) {
	if (this->Resource.Mine != nullptr && this->Resource.Mine->GetType() != nullptr) {
	//Wyrmgus end
		return this->Resource.Mine->GetMapLayer()->GetIndex();
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
	return this->State == SUB_START_GATHERING && this->worker->Wait != 0;
}

COrder_Resource::~COrder_Resource()
{
	CUnit *mine = this->Resource.Mine;

	if (mine && mine->IsAlive()) {
		worker->DeAssignWorkerFromMine(*mine);
	}

	CUnit *goal = this->GetGoal();
	if (goal) {
		// If mining decrease the active count on the resource.
		if (this->State == SUB_GATHER_RESOURCE) {

			goal->Resource.Active--;
			Assert(goal->Resource.Active >= 0);
		}
	}
}

/* virtual */ void COrder_Resource::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-resource\",");
	if (this->Finished) {
		file.printf(" \"finished\",");
	}
	if (this->HasGoal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->GetGoal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	Assert(this->worker != nullptr && worker->IsAlive());
	file.printf(" \"worker\", \"%s\",", UnitReference(worker).c_str());
	file.printf(" \"current-res\", %d,", this->CurrentResource);

	file.printf(" \"res-pos\", {%d, %d},", this->Resource.Pos.x, this->Resource.Pos.y);
	//Wyrmgus start
	file.printf(" \"res-map-layer\", %d,", this->Resource.MapLayer);
	//Wyrmgus end
	//Wyrmgus start
//	if (this->Resource.Mine != nullptr) {
	if (this->Resource.Mine != nullptr && this->Resource.Mine->GetType() != nullptr) {
	//Wyrmgus end
		file.printf(" \"res-mine\", \"%s\",", UnitReference(this->Resource.Mine).c_str());
	}
	if (this->Depot != nullptr) {
		file.printf(" \"res-depot\", \"%s\",", UnitReference(this->Depot).c_str());
	}
	if (this->DoneHarvesting) {
		file.printf(" \"done-harvesting\",");
	}
	file.printf(" \"timetoharvest\", %d,", this->TimeToHarvest);
	file.printf(" \"state\", %d", this->State);
	file.printf("}");
}

/* virtual */ bool COrder_Resource::ParseSpecificData(lua_State *l, int &j, const char *value, CUnit &unit)
{
	if (!strcmp(value, "current-res")) {
		++j;
		this->CurrentResource = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "done-harvesting")) {
		this->DoneHarvesting = true;
	} else if (!strcmp(value, "res-depot")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Depot = CclGetUnitFromRef(l);
		lua_pop(l, 1);
	} else if (!strcmp(value, "res-mine")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->Resource.Mine = CclGetUnitFromRef(l);
		lua_pop(l, 1);
		//Wyrmgus start
		if (this->Resource.Mine->GetType() == nullptr) {
			this->Resource.Mine = nullptr;
		}
		//Wyrmgus end
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
		this->worker = CclGetUnitFromRef(l);
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

/* virtual */ bool COrder_Resource::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Resource::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
{
	PixelPos targetPos;

	if (this->HasGoal()) {
		//Wyrmgus start
		if (this->GetGoal()->GetMapLayer() != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.MapToScreenPixelPos(this->GetGoal()->GetMapPixelPosCenter());
	} else {
		//Wyrmgus start
		if (this->MapLayer != UI.CurrentMapLayer->GetIndex()) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	//Wyrmgus start
//	Video.FillCircleClip(ColorYellow, lastScreenPos, 2);
//	Video.DrawLineClip(ColorYellow, lastScreenPos, targetPos);
//	Video.FillCircleClip(ColorYellow, targetPos, 3);
	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorYellow, lastScreenPos, 2);
		Video.DrawLineClip(ColorYellow, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorYellow, targetPos, 3);
	}
	//Wyrmgus end
	return targetPos;
}

/* virtual */ void COrder_Resource::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(1);

	Vec2i tileSize;
	if (this->HasGoal()) {
		CUnit *goal = this->GetGoal();
		tileSize = goal->GetTileSize();
		input.SetGoal(goal->GetTilePos(), tileSize, goal->GetMapLayer()->GetIndex());
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}


/* virtual */ bool COrder_Resource::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /* damage*/)
{
	if (this->IsGatheringFinished()) {
		// Normal return to depot
		return true;
	}
	if (this->IsGatheringStarted()  && unit.GetResourcesHeld() > 0) {
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
	if ((CMap::Map.Info.IsPointOnMap(pos, z) == false || CMap::Map.Field(pos, z)->IsTerrainResourceOnMap(CurrentResource) == false)
		&& (!unit.GetPixelOffset().x) && (!unit.GetPixelOffset().y)) {
		if (!FindTerrainType(unit.GetType()->MovementMask, this->CurrentResource, 16, *unit.GetPlayer(), this->goalPos, &pos, this->MapLayer)) {
			// no wood in range
			return -1;
		} else {
			this->goalPos = pos;
		}
	}
	switch (DoActionMove(unit)) {
		case PF_UNREACHABLE:
			//Wyrmgus start
			//if is unreachable and is on a raft, see if the raft can move closer
			if ((unit.GetMapLayer()->Field(unit.GetTilePos())->GetFlags() & MapFieldBridge) && !unit.GetType()->BoolFlag[BRIDGE_INDEX].value && unit.GetType()->UnitType == UnitTypeLand) {
				std::vector<CUnit *> table;
				Select(unit.GetTilePos(), unit.GetTilePos(), table, unit.GetMapLayer()->GetIndex());
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->GetType()->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitActionStill) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->GetTilePos() : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->GetMapLayer()->GetIndex() : this->MapLayer);
						}
						return 0;
					}
				}
			}
			//Wyrmgus end
			unit.Wait = 10;
			if (unit.GetPlayer()->AiEnabled) {
				this->Range++;
				if (this->Range >= 5) {
					this->Range = 0;
					AiCanNotMove(unit);
				}
			}
			if (FindTerrainType(unit.GetType()->MovementMask, this->CurrentResource, 9999, *unit.GetPlayer(), unit.GetTilePos(), &pos, z)) {
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
			if (unit.GetPlayer()->AiEnabled) {
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
	const CUnit *goal = this->GetGoal();
	Assert(goal);

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			//Wyrmgus start
			//if is unreachable and is on a raft, see if the raft can move closer
			if ((unit.GetMapLayer()->Field(unit.GetTilePos())->GetFlags() & MapFieldBridge) && !unit.GetType()->BoolFlag[BRIDGE_INDEX].value && unit.GetType()->UnitType == UnitTypeLand) {
				std::vector<CUnit *> table;
				Select(unit.GetTilePos(), unit.GetTilePos(), table, unit.GetMapLayer()->GetIndex());
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->GetType()->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitActionStill) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->GetTilePos() : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->GetMapLayer()->GetIndex() : this->MapLayer);
						}
						return 0;
					}
				}
			}
			//Wyrmgus end
			return -1;
		case PF_REACHED:
			break;
		case PF_WAIT:
			if (unit.GetPlayer()->AiEnabled) {
				this->Range++;
				if (this->Range >= 5) {
					this->Range = 0;
					AiCanNotMove(unit);
				}
			}
		default:
			// Goal gone or something.
			if (unit.Anim.Unbreakable || goal->IsVisibleAsGoal(*unit.GetPlayer())) {
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
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];

	//Wyrmgus start
//	if (resinfo.TerrainHarvester) {
	if (CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		return MoveToResource_Terrain(unit);
	} else {
		return MoveToResource_Unit(unit);
	}
}

void COrder_Resource::UnitGotoGoal(CUnit &unit, CUnit *const goal, int state)
{
	if (this->GetGoal() != goal) {
		this->SetGoal(goal);
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
	CUnit *goal;
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];
	Assert(!unit.GetPixelOffset().x);
	Assert(!unit.GetPixelOffset().y);

	//Wyrmgus start
	const int input_resource = CResource::GetAll()[this->CurrentResource]->InputResource;
	if (input_resource && (unit.GetPlayer()->Resources[input_resource] + unit.GetPlayer()->StoredResources[input_resource]) == 0) { //if the resource requires an input, but there's none in store, don't gather
		const char *input_name = DefaultResourceNames[input_resource].c_str();
		const char *input_actionName = CResource::GetAll()[input_resource]->ActionName.c_str();
		unit.GetPlayer()->Notify(_("Not enough %s... %s more %s."), _(input_name), _(input_actionName), _(input_name)); //added extra space to look better
		if (unit.GetPlayer() == CPlayer::GetThisPlayer() && GameSounds.NotEnoughRes[unit.GetPlayer()->Race][input_resource].Sound) {
			PlayGameSound(GameSounds.NotEnoughRes[unit.GetPlayer()->Race][input_resource].Sound, MaxSampleVolume);
		}
		this->Finished = true;
		return 0;
	}
	//Wyrmgus end
		
	//Wyrmgus start
//	if (resinfo.TerrainHarvester) {
	if (CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		// This shouldn't happened?
#if 0
		if (!CMap::Map.IsTerrainResourceOnMap(unit.Orders->goalPos, this->CurrentResource)) {
			DebugPrint("Wood gone, just like that?\n");
			return 0;
		}
#endif
		UnitHeadingFromDeltaXY(unit, this->goalPos - unit.GetTilePos());
		if (resinfo.WaitAtResource) {
			this->TimeToHarvest = std::max<int>(1, resinfo.WaitAtResource * SPEEDUP_FACTOR / unit.GetPlayer()->SpeedResourcesHarvest[resinfo.ResourceId]);
		} else {
			this->TimeToHarvest = 1;
		}
		this->DoneHarvesting = 0;
		if (this->CurrentResource != unit.GetCurrentResource()) {
			DropResource(unit);
			unit.SetCurrentResource(this->CurrentResource);
		}
		return 1;
	}

	goal = this->GetGoal();

	// Target is dead, stop getting resources.
	//Wyrmgus start
//	if (!goal || goal->IsVisibleAsGoal(*unit.GetPlayer()) == false) {
	//the goal could also have changed its given resource
	if (!goal || goal->IsVisibleAsGoal(*unit.GetPlayer()) == false || goal->GivesResource != this->CurrentResource) {
	//Wyrmgus end
		// Find an alternative, but don't look too far.
		this->goalPos.x = -1;
		this->goalPos.y = -1;
		//Wyrmgus start
//		if ((goal = UnitFindResource(unit, unit, 15, this->CurrentResource, unit.GetPlayer()->AiEnabled))) {
		if ((goal = UnitFindResource(unit, unit, 15, this->CurrentResource, true, nullptr, true, false, false, false, true))) {
		//Wyrmgus end
			this->State = SUB_START_RESOURCE;
			this->SetGoal(goal);
		} else {
			this->ClearGoal();
			this->Finished = true;
		}
		return 0;
	}

	// FIXME: 0 can happen, if to near placed by map designer.
	Assert(unit.MapDistanceTo(*goal) <= 1);

	// Update the heading of a harvesting unit to looks straight at the resource.
	//Wyrmgus start
//	UnitHeadingFromDeltaXY(unit, goal->GetTilePos() - unit.GetTilePos() + goal->GetType()->GetHalfTileSize());
	UnitHeadingFromDeltaXY(unit, PixelSize(PixelSize(goal->GetTilePos()) * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex())) - PixelSize(PixelSize(unit.GetTilePos()) * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex())) + goal->GetHalfTilePixelSize() - unit.GetHalfTilePixelSize());
	//Wyrmgus end

	// If resource is still under construction, wait!
	if ((goal->GetType()->MaxOnBoard && goal->Resource.Active >= goal->GetType()->MaxOnBoard)
		|| goal->CurrentAction() == UnitActionBuilt) {
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
	if (!goal->GetType()->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
	//Wyrmgus end
		//Wyrmgus start
//		if (goal->Variable[MAXHARVESTERS_INDEX].Value == 0 || goal->Variable[MAXHARVESTERS_INDEX].Value > goal->InsideCount) {
		if (goal->Variable[MAXHARVESTERS_INDEX].Value == 0 || goal->Variable[MAXHARVESTERS_INDEX].Value > goal->Resource.Active) {
		//Wyrmgus end
			this->ClearGoal();
			const bool selected = unit.IsSelected();
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

	if (this->CurrentResource != unit.GetCurrentResource()) {
		DropResource(unit);
		unit.SetCurrentResource(this->CurrentResource);
	}

	// Activate the resource
	goal->Resource.Active++;

	if (resinfo.WaitAtResource) {
		//Wyrmgus start
//		this->TimeToHarvest = std::max<int>(1, resinfo.WaitAtResource * SPEEDUP_FACTOR / unit.GetPlayer()->SpeedResourcesHarvest[resinfo.ResourceId]);
		int wait_at_resource = resinfo.WaitAtResource;
		if (!goal->GetType()->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
			wait_at_resource = resinfo.WaitAtResource * 100 / unit.GetResourceStep(this->CurrentResource);
		}
		this->TimeToHarvest = std::max<int>(1, wait_at_resource * SPEEDUP_FACTOR / (unit.GetPlayer()->SpeedResourcesHarvest[resinfo.ResourceId] + goal->Variable[TIMEEFFICIENCYBONUS_INDEX].Value));
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
	//Wyrmgus start
//	Assert(unit.GetType()->Animations->Harvest[unit.GetCurrentResource()]);
//	UnitShowAnimation(unit, unit.GetType()->Animations->Harvest[unit.GetCurrentResource()]);
	Assert(unit.GetAnimations()->Harvest[unit.GetCurrentResource()]);
	UnitShowAnimation(unit, unit.GetAnimations()->Harvest[unit.GetCurrentResource()]);
	//Wyrmgus end
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
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];

	//Wyrmgus start
	const CUnitType &source_type = *source.GetType();
	
//	Assert((unit.Container == &source && !resinfo.HarvestFromOutside)
//		   || (!unit.Container && resinfo.HarvestFromOutside));
	Assert((unit.Container == &source && !source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value)
		   || (!unit.Container && source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value));
	//Wyrmgus end

	//Wyrmgus start
//	if (resinfo.HarvestFromOutside) {
	if (source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
	//Wyrmgus end
		this->ClearGoal();
		--source.Resource.Active;
	}

	// Continue to harvest if we aren't fully loaded
	//Wyrmgus start
//	if (resinfo.HarvestFromOutside && unit.GetResourcesHeld() < resinfo.ResourceCapacity) {
	if (source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value && unit.GetResourcesHeld() < resinfo.ResourceCapacity) {
	//Wyrmgus end
		//Wyrmgus start
//		CUnit *goal = UnitFindResource(unit, unit, 15, this->CurrentResource, 1);
		CUnit *goal = UnitFindResource(unit, unit, 15, this->CurrentResource, 1, nullptr, true, false, false, false, true);
		//Wyrmgus end

		if (goal) {
			this->goalPos.x = -1;
			this->goalPos.y = -1;
			this->State = SUB_START_RESOURCE;
			this->SetGoal(goal);
			return;
		}
	}

	// If we are fully loaded first search for a depot.
	if (unit.GetResourcesHeld() && (depot = FindDeposit(unit, 1000, unit.GetCurrentResource()))) {
		if (unit.Container) {
			DropOutNearest(unit, depot->GetTilePos() + depot->GetHalfTileSize(), &source);
		}
		// Remember where it mined, so it can look around for another resource.
		//
		//FIXME!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//unit.CurrentOrder()->Arg1.ResourcePos = (unit.X << 16) | unit.Y;
		this->DoneHarvesting = true;
		UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
		DebugPrint("%d: Worker %d report: Resource is exhausted, Going to depot\n"
				   _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit));
		return;
	}
	// No depot found, or harvester empty
	// Dump the unit outside and look for something to do.
	if (unit.Container) {
		//Wyrmgus start
//		Assert(!resinfo.HarvestFromOutside);
		Assert(!source_type.BoolFlag[HARVESTFROMOUTSIDE_INDEX].value);
		//Wyrmgus end
		DropOutOnSide(unit, LookingW, &source);
	}
	this->goalPos.x = -1;
	this->goalPos.y = -1;
	//use depot as goal
	//Wyrmgus start
//	depot = UnitFindResource(unit, unit, 15, this->CurrentResource, unit.GetPlayer()->AiEnabled);
	depot = UnitFindResource(unit, unit, 15, this->CurrentResource, true, nullptr, true, false, false, false, true);
	//Wyrmgus end
	if (depot) {
		DebugPrint("%d: Worker %d report: Resource is exhausted, Found another resource.\n"
				   _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit));
		this->State = SUB_START_RESOURCE;
		this->SetGoal(depot);
	} else {
		DebugPrint("%d: Worker %d report: Resource is exhausted, Just sits around confused.\n"
				   _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit));
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
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];
	int addload;

	//Wyrmgus start
	bool harvest_from_outside = (this->GetGoal() && this->GetGoal()->GetType()->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value);
//	if (resinfo.HarvestFromOutside || resinfo.TerrainHarvester) {
	if (harvest_from_outside || CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		AnimateActionHarvest(unit);
	} else {
		unit.Anim.CurrAnim = nullptr;
	}

	this->TimeToHarvest--;

	if (this->DoneHarvesting) {
		//Wyrmgus start
//		Assert(resinfo.HarvestFromOutside || resinfo.TerrainHarvester);
		Assert(harvest_from_outside || CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer));
		//Wyrmgus end
		return !unit.Anim.Unbreakable;
	}

	// Target gone?
	//Wyrmgus start
//	if (resinfo.TerrainHarvester && !CMap::Map.Field(this->goalPos)->IsTerrainResourceOnMap(this->CurrentResource)) {
	if (CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer) && !CMap::Map.Field(this->goalPos, this->MapLayer)->IsTerrainResourceOnMap(this->CurrentResource)) {
	//Wyrmgus end
		if (!unit.Anim.Unbreakable) {
			// Action now breakable, move to resource again.
			this->State = SUB_MOVE_TO_RESOURCE;
			// Give it some reasonable look while searching.
			// FIXME: which frame?
			unit.SetFrame(0);
		}
		return 0;
		// No wood? Freeze!!!
	}

	while (!this->DoneHarvesting && this->TimeToHarvest < 0) {
		//FIXME: rb - how should it look for WaitAtResource == 0
		if (resinfo.WaitAtResource) {
			// Wyrmgus start
//			this->TimeToHarvest += std::max<int>(1, resinfo.WaitAtResource * SPEEDUP_FACTOR / unit.GetPlayer()->SpeedResourcesHarvest[resinfo.ResourceId]);
			int wait_at_resource = resinfo.WaitAtResource;
			int resource_harvest_speed = unit.GetPlayer()->SpeedResourcesHarvest[resinfo.ResourceId];
			if (!CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer) && !harvest_from_outside) {
				wait_at_resource = resinfo.WaitAtResource * 100 / unit.GetResourceStep(this->CurrentResource);
			}
			if (this->GetGoal()) {
				resource_harvest_speed += this->GetGoal()->Variable[TIMEEFFICIENCYBONUS_INDEX].Value;
			}
			this->TimeToHarvest += std::max<int>(1, wait_at_resource * SPEEDUP_FACTOR / resource_harvest_speed);
			//Wyrmgus end
		} else {
			this->TimeToHarvest += 1;
		}

		// Calculate how much we can load.
		if (unit.GetResourceStep(this->CurrentResource) && (harvest_from_outside || CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer))) {
			addload = unit.GetResourceStep(this->CurrentResource);
		} else {
			//Wyrmgus start
//			addload = resinfo.ResourceCapacity;
			if (this->CurrentResource == TradeCost) { // the load added when trading depends on the price difference between the two players
				addload = unit.GetPlayer()->ConvergePricesWith(*unit.Container->GetPlayer(), resinfo.ResourceCapacity);
				addload = std::max(10, addload);
			} else {
				addload = std::min(100, resinfo.ResourceCapacity);
			}
			//Wyrmgus end
		}
		// Make sure we don't bite more than we can chew.
		if (unit.GetResourcesHeld() + addload > resinfo.ResourceCapacity) {
			addload = resinfo.ResourceCapacity - unit.GetResourcesHeld();
		}

		//Wyrmgus start
//		if (resinfo.TerrainHarvester) {
		if (CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
			//Wyrmgus start
			CMapField &mf = *CMap::Map.Field(this->goalPos, this->MapLayer);
			if (addload > mf.Value) {
				addload = mf.Value;
			}
			mf.Value -= addload;
			//Wyrmgus end

			unit.ChangeResourcesHeld(addload);
			
			//Wyrmgus start
//			if (addload && unit.GetResourcesHeld() == resinfo.ResourceCapacity) {
			if (mf.Value <= 0) {
			//Wyrmgus end
				//Wyrmgus start
//				CMap::Map.ClearWoodTile(this->goalPos);
				CMap::Map.ClearOverlayTile(this->goalPos, this->MapLayer);
				//Wyrmgus end
			}
		} else {
			//Wyrmgus start
//			if (resinfo.HarvestFromOutside) {
			if (harvest_from_outside) {
			//Wyrmgus end
				source = this->GetGoal();
			} else {
				source = unit.Container;
			}

			Assert(source);
			Assert(source->GetResourcesHeld() <= 655350);
			//Wyrmgus start
			UpdateUnitVariables(*source); //update resource source's variables
			//Wyrmgus end
			bool is_visible = source->IsVisibleAsGoal(*unit.GetPlayer());
			// Target is not dead, getting resources.
			if (is_visible) {
				// Don't load more that there is.
				addload = std::min(source->GetResourcesHeld(), addload);
				//Wyrmgus start
//				unit.ResourcesHeld += addload;
//				source->ResourcesHeld -= addload;
				const int input_resource = CResource::GetAll()[this->CurrentResource]->InputResource;
				if (input_resource) {
					addload = std::min(unit.GetPlayer()->Resources[input_resource] + unit.GetPlayer()->StoredResources[input_resource], addload);
					
					if (!addload) {
						const char *input_name = DefaultResourceNames[input_resource].c_str();
						const char *input_actionName = CResource::GetAll()[input_resource]->ActionName.c_str();
						unit.GetPlayer()->Notify(_("Not enough %s... %s more %s."), _(input_name), _(input_actionName), _(input_name)); //added extra space to look better
						if (unit.GetPlayer() == CPlayer::GetThisPlayer() && GameSounds.NotEnoughRes[unit.GetPlayer()->Race][input_resource].Sound) {
							PlayGameSound(GameSounds.NotEnoughRes[unit.GetPlayer()->Race][input_resource].Sound, MaxSampleVolume);
						}

						if (unit.Container) {
							DropOutOnSide(unit, LookingW, source);
						}
						this->Finished = true;
						return 0;
					}
					
					unit.GetPlayer()->ChangeResource(input_resource, -addload, true);
				}
				unit.ChangeResourcesHeld(addload);
				if (!source->GetType()->BoolFlag[INEXHAUSTIBLE_INDEX].value) {
					source->ChangeResourcesHeld(-addload);
				}
				//Wyrmgus end
			}

			// End of resource: destroy the resource.
			// FIXME: implement depleted resources.
			if ((!is_visible) || (source->GetResourcesHeld() == 0)) {
				if (unit.Anim.Unbreakable) {
					return 0;
				}
				DebugPrint("%d: Worker %d report: Resource is destroyed\n" _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit));
				bool dead = source->IsAlive() == false;

				// Improved version of DropOutAll that makes workers go to the depot.
				LoseResource(unit, *source);
				for (CUnit *uins = source->Resource.Workers;
					 uins; uins = uins->NextWorker) {
					if (uins != &unit && uins->CurrentOrder()->Action == UnitActionResource) {
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
						&& unit.GetPlayer()->GetIndex() == CPlayer::GetThisPlayer()->GetIndex()
						&& source->Variable[GIVERESOURCE_INDEX].Max > (CResource::GetAll()[this->CurrentResource]->DefaultIncome * 10)) {
							unit.GetPlayer()->Notify(NotifyYellow, source->GetTilePos(), source->GetMapLayer()->GetIndex(), _("Our %s has been depleted!"), source->GetType()->GetName().utf8().get_data());
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
		if (CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
			if (unit.GetResourcesHeld() == resinfo.ResourceCapacity) {
				// Mark as complete.
				this->DoneHarvesting = true;
			}
			return 0;
		} else {
			//Wyrmgus start
//			if (resinfo.HarvestFromOutside) {
			if (harvest_from_outside) {
			//Wyrmgus end
				if ((unit.GetResourcesHeld() == resinfo.ResourceCapacity) || (source == nullptr)) {
					// Mark as complete.
					this->DoneHarvesting = true;
				}
				return 0;
			} else {
				return unit.GetResourcesHeld() == resinfo.ResourceCapacity && source;
			}
		}
	}
	return 0;
}

int GetNumWaitingWorkers(const CUnit &mine)
{
	int ret = 0;
	CUnit *worker = mine.Resource.Workers;

	for (int i = 0; nullptr != worker; worker = worker->NextWorker, ++i) {
		Assert(worker->CurrentAction() == UnitActionResource);
		COrder_Resource &order = *static_cast<COrder_Resource *>(worker->CurrentOrder());

		if (order.IsGatheringWaiting()) {
			ret++;
		}
		Assert(i <= mine.Resource.Assigned);
	}
	return ret;
}

/**
**  Stop gathering from the resource, go home.
**
**  @param unit  Poiner to unit.
**
**  @return      TRUE if ready, otherwise FALSE.
*/
int COrder_Resource::StopGathering(CUnit &unit)
{
	CUnit *source = 0;
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];

	//Wyrmgus start
//	if (!resinfo.TerrainHarvester) {
	if (!CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
	//Wyrmgus end
		//Wyrmgus start
//		if (resinfo.HarvestFromOutside) {
		if (this->GetGoal() && this->GetGoal()->GetType()->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) {
		//Wyrmgus end
			source = this->GetGoal();
			this->ClearGoal();
		} else {
			source = unit.Container;
		}
		source->Resource.Active--;
		Assert(source->Resource.Active >= 0);
		//Store resource position.
		this->Resource.Mine = source;
		
		if (Preference.MineNotifications && unit.GetPlayer()->GetIndex() == CPlayer::GetThisPlayer()->GetIndex()
			&& source->IsAlive()
			&& !source->MineLow
			&& (source->GetResourcesHeld() * 100 / source->Variable[GIVERESOURCE_INDEX].Max) <= 10
			&& source->Variable[GIVERESOURCE_INDEX].Max > (CResource::GetAll()[this->CurrentResource]->DefaultIncome * 10)) {
				unit.GetPlayer()->Notify(NotifyYellow, source->GetTilePos(), source->GetMapLayer()->GetIndex(), _("Our %s is nearing depletion!"), source->GetType()->GetName().utf8().get_data());
				source->MineLow = 1;
		}

		if (source->GetType()->MaxOnBoard) {
			int count = 0;
			CUnit *worker = source->Resource.Workers;
			CUnit *next = nullptr;
			for (; nullptr != worker; worker = worker->NextWorker) {
				Assert(worker->CurrentAction() == UnitActionResource);
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
			if (next) {
				if (!unit.GetPlayer()->AiEnabled) {
					DebugPrint("%d: Worker %d report: Unfreez resource gathering of %d <Wait %d> on %d [Assigned: %d Waiting %d].\n"
							   _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit)
							   _C_ UnitNumber(*next) _C_ next->Wait
							   _C_ UnitNumber(*source) _C_ source->Resource.Assigned
							   _C_ count);
				}
				next->Wait = 0;
				//source->Data.Resource.Waiting = count - 1;
				//Assert(source->Data.Resource.Assigned >= source->Data.Resource.Waiting);
				//StartGathering(next);
			}
		}
	} else {
		// Store resource position.
		this->Resource.Pos = unit.GetTilePos();
		this->Resource.MapLayer = unit.GetMapLayer()->GetIndex();
		Assert(this->Resource.Mine == nullptr);
	}

#ifdef DEBUG
	if (!unit.GetResourcesHeld()) {
		DebugPrint("Unit %d is empty???\n" _C_ UnitNumber(unit));
	}
#endif

	// Find and send to resource deposit.
	CUnit *depot = FindDeposit(unit, 1000, unit.GetCurrentResource());
	if (!depot || !unit.GetResourcesHeld() || this->Finished) {
		//Wyrmgus start
//		if (!(resinfo.HarvestFromOutside || resinfo.TerrainHarvester)) {
		if (!((source && source->GetType()->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) || CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer))) {
		//Wyrmgus end
			Assert(unit.Container);
			DropOutOnSide(unit, LookingW, source);
		}
		CUnit *mine = this->Resource.Mine;

		if (mine) {
			unit.DeAssignWorkerFromMine(*mine);
			this->Resource.Mine = nullptr;
		}

		DebugPrint("%d: Worker %d report: Can't find a resource [%d] deposit.\n"
				   _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit) _C_ unit.GetCurrentResource());
		this->Finished = true;
		return 0;
	} else {
		//Wyrmgus start
//		if (!(resinfo.HarvestFromOutside || resinfo.TerrainHarvester)) {
		if (!((source && source->GetType()->BoolFlag[HARVESTFROMOUTSIDE_INDEX].value) || CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer))) {
		//Wyrmgus end
			Assert(unit.Container);
			DropOutNearest(unit, depot->GetTilePos() + depot->GetHalfTileSize(), source);
		}
		UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
	}
	if (IsOnlySelected(unit)) {
		SelectedUnitChanged();
	}
#if 1
	return 1;
#endif
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
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];
	CUnit &goal = *this->GetGoal();
	CPlayer &player = *unit.GetPlayer();
	Assert(&goal);

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			//Wyrmgus start
			//if is unreachable and is on a raft, see if the raft can move closer
			if ((unit.GetMapLayer()->Field(unit.GetTilePos())->GetFlags() & MapFieldBridge) && !unit.GetType()->BoolFlag[BRIDGE_INDEX].value && unit.GetType()->UnitType == UnitTypeLand) {
				std::vector<CUnit *> table;
				Select(unit.GetTilePos(), unit.GetTilePos(), table, unit.GetMapLayer()->GetIndex());
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->GetType()->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitActionStill) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->GetTilePos() : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->GetMapLayer()->GetIndex() : this->MapLayer);
						}
						return 0;
					}
				}
			}
			//Wyrmgus end
			return -1;
		case PF_REACHED:
			break;
		case PF_WAIT:
			if (unit.GetPlayer()->AiEnabled) {
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
		DebugPrint("%d: Worker %d report: Destroyed depot\n" _C_ player.GetIndex() _C_ UnitNumber(unit));

		unit.CurrentOrder()->ClearGoal();

		CUnit *depot = FindDeposit(unit, 1000, unit.GetCurrentResource());

		if (depot) {
			UnitGotoGoal(unit, depot, SUB_MOVE_TO_DEPOT);
			DebugPrint("%d: Worker %d report: Going to new deposit.\n" _C_ player.GetIndex() _C_ UnitNumber(unit));
		} else {
			DebugPrint("%d: Worker %d report: Can't find a new resource deposit.\n"
					   _C_ player.GetIndex() _C_ UnitNumber(unit));

			// FIXME: perhaps we should choose an alternative
			this->Finished = true;
		}
		return 0;
	}

	// If resource depot is still under construction, wait!
	if (goal.CurrentAction() == UnitActionBuilt) {
		unit.Wait = 10;
		return 0;
	}

	this->ClearGoal();
	unit.Wait = resinfo.WaitAtDepot;

	// Place unit inside the depot
	if (unit.Wait) {
		const bool selected = unit.IsSelected();
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
	const int rindex = CResource::GetAll()[this->CurrentResource]->FinalResource;
	int resource_change = unit.GetResourcesHeld() * CResource::GetAll()[this->CurrentResource]->FinalResourceConversionRate / 100;
	int processed_resource_change = (resource_change * player.Incomes[this->CurrentResource]) / 100;
	
	if (player.AiEnabled) {
		if (GameSettings.Difficulty == DifficultyEasy) {
			processed_resource_change /= 2;
		} else if (GameSettings.Difficulty == DifficultyHard) {
			processed_resource_change *= 3;
			processed_resource_change /= 4;
		} else if (GameSettings.Difficulty == DifficultyBrutal) {
			processed_resource_change *= 2;
		}
	}
	
	if (!player.Overlord) {
		player.ChangeResource(rindex, processed_resource_change, true);
		player.TotalResources[rindex] += processed_resource_change;
	} else { // if the player has an overlord, give 10% of the resources gathered to them
		player.ChangeResource(rindex, processed_resource_change * 90 / 100, true);
		player.TotalResources[rindex] += processed_resource_change * 90 / 100;
		player.Overlord->ChangeResource(rindex, processed_resource_change / 10, true);
		player.Overlord->TotalResources[rindex] += processed_resource_change / 10;
	}
	
	//give XP to the worker according to how much was gathered, based on their base price in relation to gold
	int xp_gained = unit.GetResourcesHeld();
	xp_gained /= 20;
	unit.ChangeExperience(xp_gained);
	
	//update quests
	for (size_t i = 0; i < player.QuestObjectives.size(); ++i) {
		if (player.QuestObjectives[i]->ObjectiveType == GatherResourceObjectiveType) {
			if (player.QuestObjectives[i]->Resource == rindex) {
				player.QuestObjectives[i]->Counter = std::min(player.QuestObjectives[i]->Counter + processed_resource_change, player.QuestObjectives[i]->Quantity);
			} else if (player.QuestObjectives[i]->Resource == this->CurrentResource) {
				player.QuestObjectives[i]->Counter = std::min(player.QuestObjectives[i]->Counter + unit.GetResourcesHeld(), player.QuestObjectives[i]->Quantity);
			}
		}
	}
	
	unit.SetResourcesHeld(0);
	unit.SetCurrentResource(0);

	if (unit.Wait) {
		//Wyrmgus start
//		unit.Wait /= std::max(1, unit.GetPlayer()->SpeedResourcesReturn[resinfo.ResourceId] / SPEEDUP_FACTOR);
		unit.Wait /= std::max(1, (unit.GetPlayer()->SpeedResourcesReturn[resinfo.ResourceId] + goal.Variable[TIMEEFFICIENCYBONUS_INDEX].Value) / SPEEDUP_FACTOR);
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
	const ResourceInfo &resinfo = *unit.GetType()->ResInfo[this->CurrentResource];
	const CUnit *depot = ResourceDepositOnMap(unit.GetTilePos(), resinfo.ResourceId, unit.GetMapLayer()->GetIndex());

	//Assert(depot);

	// Range hardcoded. don't stray too far though
	//Wyrmgus start
//	if (resinfo.TerrainHarvester) {
	if (!this->Resource.Mine || this->Resource.Mine->GetType() == nullptr) {
	//Wyrmgus end
		Vec2i pos = this->Resource.Pos;
		int z = this->Resource.MapLayer = unit.GetMapLayer()->GetIndex();

		if (FindTerrainType(unit.GetType()->MovementMask, this->CurrentResource, 10, *unit.GetPlayer(), pos, &pos, z)) {
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
		const unsigned int tooManyWorkers = 15;
		CUnit *mine = this->Resource.Mine;
		const int range = 15;
		CUnit *newdepot = nullptr;
		CUnit *goal = nullptr;
		const bool longWay = unit.pathFinderData->output.Cycles > 500;

		//Wyrmgus start
//		if (unit.GetPlayer()->AiEnabled && AiPlayer && AiPlayer->BuildDepots) {
		if (depot && unit.GetPlayer()->AiEnabled && AiPlayer && AiPlayer->BuildDepots) { //check if the depot is valid
		//Wyrmgus end
			// If the depot is overused, we need first to try to switch into another depot
			// Use depot's ref counter for that
			//Wyrmgus start
//			if (longWay || !mine || (depot->Refs > tooManyWorkers)) {
			if (longWay || !mine || mine->GetType() == nullptr || (depot->Refs > tooManyWorkers)) {
			//Wyrmgus end
				newdepot = AiGetSuitableDepot(unit, *depot, &goal);
				if (newdepot == nullptr && longWay && unit.GetPlayer()->NumTownHalls > 0) {
					// We need a new depot
					AiNewDepotRequest(unit);
				}
			}
		}

		// If goal is not null, then we got it in AiGetSuitableDepot
		if (!goal) {
			//Wyrmgus start
//			goal = UnitFindResource(unit, newdepot ? *newdepot : (mine ? *mine : unit), mine ? range : 1000,
//									this->CurrentResource, unit.GetPlayer()->AiEnabled, newdepot ? newdepot : depot);
			goal = UnitFindResource(unit, newdepot ? *newdepot : ((mine && mine->GetType()) ? *mine : unit), (mine && mine->GetType()) ? range : 1000,
									this->CurrentResource, true, newdepot ? newdepot : depot, true, false, false, false, true, true, true);
			//Wyrmgus end
		}

		if (goal) {
			if (depot) {
				DropOutNearest(unit, goal->GetTilePos() + goal->GetHalfTileSize(), depot);
			}

			if (goal != mine) {
				if (mine) {
					unit.DeAssignWorkerFromMine(*mine);
				}
				unit.AssignWorkerToMine(*goal);
				this->Resource.Mine = goal;
			}
			this->SetGoal(goal);
			this->goalPos.x = this->goalPos.y = -1;
		} else {
#ifdef DEBUG
			const Vec2i &pos = mine ? mine->GetTilePos() : unit.GetTilePos();
			DebugPrint("%d: Worker %d report: [%d,%d] Resource gone near [%d,%d] in range %d. Sit and play dumb.\n"
					   _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit)
					   _C_ unit.GetTilePos().x _C_ unit.GetTilePos().y
					   _C_ pos.x _C_ pos.y _C_ range);
#endif
			if (depot) {
				DropOutOnSide(unit, LookingW, depot);
			}
			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
				this->Resource.Mine = nullptr;
			}
			this->Finished = true;
			return false;
		}
	}
	return true;
}

void COrder_Resource::DropResource(CUnit &unit)
{
	if (unit.GetCurrentResource()) {
		const ResourceInfo &resinfo = *unit.GetType()->ResInfo[unit.GetCurrentResource()];

		//Wyrmgus start
//		if (!resinfo.TerrainHarvester) {
		if (!CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
		//Wyrmgus end
			CUnit *mine = this->Resource.Mine;
			if (mine) {
				unit.DeAssignWorkerFromMine(*mine);
			}
		}
		//fast clean both resource data: pos and mine
		this->Resource.Mine = nullptr;
		unit.SetCurrentResource(0);
		unit.SetResourcesHeld(0);
	}
}

/**
**  Give up on gathering.
**
**  @param unit  Pointer to unit.
*/
void COrder_Resource::ResourceGiveUp(CUnit &unit)
{
	DebugPrint("%d: Worker %d report: Gave up on resource gathering.\n" _C_ unit.GetPlayer()->GetIndex() _C_ UnitNumber(unit));
	if (this->HasGoal()) {
		DropResource(unit);
		this->ClearGoal();
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
		const ResourceInfo *resinfo = unit.GetType()->ResInfo[this->CurrentResource];
		if (resinfo) {
			//Wyrmgus start
	//		if (!resinfo.TerrainHarvester) {
			if (!CMap::Map.Info.IsPointOnMap(this->goalPos, this->MapLayer)) {
			//Wyrmgus end
				//Wyrmgus start
//				CUnit *newGoal = UnitFindResource(unit, this->Resource.Mine ? *this->Resource.Mine : unit, 8, this->CurrentResource, 1);
				CUnit *newGoal = UnitFindResource(unit, (this->Resource.Mine && this->Resource.Mine->GetType()) ? *this->Resource.Mine : unit, 8, this->CurrentResource, 1, nullptr, true, false, false, false, true);
				//Wyrmgus end

				if (newGoal) {
					CUnit *mine = this->Resource.Mine;
					if (mine) {
						unit.DeAssignWorkerFromMine(*mine);
					}
					unit.AssignWorkerToMine(*newGoal);
					this->Resource.Mine = newGoal;
					this->goalPos.x = -1;
					this->goalPos.y = -1;
					this->State = SUB_MOVE_TO_RESOURCE;
					this->SetGoal(newGoal);
					return true;
				}
			} else {
				Vec2i resPos;
				if (FindTerrainType(unit.GetType()->MovementMask, this->CurrentResource, 8, *unit.GetPlayer(), unit.GetTilePos(), &resPos, unit.GetMapLayer()->GetIndex())) {
					this->goalPos = resPos;
					this->MapLayer = unit.GetMapLayer()->GetIndex();
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
	Assert(this->State == SUB_START_RESOURCE);

	this->Range = 0;
	CUnit *const goal = this->GetGoal();
	CUnit *mine = this->Resource.Mine;

	if (mine) {
		unit.DeAssignWorkerFromMine(*mine);
		this->Resource.Mine = nullptr;
	}
	if (goal && goal->IsAlive() == false) {
		return false;
	}
	if (goal && goal->CurrentAction() != UnitActionBuilt) {
		unit.AssignWorkerToMine(*goal);
		this->Resource.Mine = goal;
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
	this->worker = &unit;

	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		//Wyrmgus start
//		UnitShowAnimation(unit, unit.GetType()->Animations->Still);
		UnitShowAnimation(unit, unit.GetAnimations()->Still);
		//Wyrmgus end
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
				Assert(0);
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
				Assert(0);
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
