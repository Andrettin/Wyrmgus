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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon,
//      Russell Smith and Andrettin
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

#include "action/action_build.h"

#include "action/action_built.h"
#include "ai.h"
//Wyrmgus start
#include "ai/ai_local.h"
//Wyrmgus end
#include "animation.h"
//Wyrmgus start
#include "character.h"
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder.h"
#include "player/player.h"
#include "script.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "video/video.h"

extern void AiReduceMadeInBuilt(PlayerAi &pai, const wyrmgus::unit_type &type, const landmass *landmass, const wyrmgus::site *settlement);

enum {
	State_Start = 0,
	State_MoveToLocationMax = 10, // Range from prev
	State_NearOfLocation = 11, // Range to next
	State_StartBuilding_Failed = 20,
	State_BuildFromInside = 21,
	State_BuildFromOutside = 22
};

std::unique_ptr<COrder> COrder::NewActionBuild(const CUnit &builder, const Vec2i &pos, const wyrmgus::unit_type &building, int z, const wyrmgus::site *settlement)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	auto order = std::make_unique<COrder_Build>();

	order->goalPos = pos;
	//Wyrmgus start
	order->MapLayer = z;
	order->settlement = settlement;
	//Wyrmgus end
	if (building.BoolFlag[BUILDEROUTSIDE_INDEX].value) {
		order->Range = builder.Type->RepairRange;
	} else {
		// If building inside, but be next to stop
		if (building.BoolFlag[SHOREBUILDING_INDEX].value && builder.Type->get_domain() == unit_domain::land) {
			// Peon won't dive :-)
			order->Range = 1;
		}
	}
	order->Type = &building;
	return order;
}

COrder_Build::COrder_Build() : COrder(UnitAction::Build)
{
	goalPos.x = -1;
	goalPos.y = -1;
}

COrder_Build::~COrder_Build()
{
}

void COrder_Build::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)
	
	file.printf("{\"action-build\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	if (this->settlement != nullptr) {
		file.printf(" \"settlement\", \"%s\",", this->settlement->get_identifier().c_str());
	}
	//Wyrmgus end

	if (this->get_building_unit() != nullptr) {
		file.printf(" \"building\", \"%s\",", UnitReference(this->get_building_unit()).c_str());
	}
	file.printf(" \"type\", \"%s\",", this->Type->Ident.c_str());
	file.printf(" \"state\", %d", this->State);
	file.printf("}");
}

bool COrder_Build::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
	
	if (!strcmp(value, "building")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->BuildingUnit = CclGetUnitFromRef(l)->acquire_ref();
		lua_pop(l, 1);
	} else if (!strcmp(value, "range")) {
		++j;
		this->Range = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "state")) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "tile")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->goalPos.x , &this->goalPos.y);
		lua_pop(l, 1);
	} else if (!strcmp(value, "type")) {
		++j;
		this->Type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
	//Wyrmgus start
	} else if (!strcmp(value, "map-layer")) {
		++j;
		this->MapLayer = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "settlement")) {
		++j;
		this->settlement = wyrmgus::site::get(LuaToString(l, -1, j + 1));
	//Wyrmgus end
	} else {
		return false;
	}
	return true;
}

bool COrder_Build::IsValid() const
{
	return true;
}

PixelPos COrder_Build::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	//Wyrmgus start
	if (this->MapLayer != UI.CurrentMapLayer->ID) {
		return lastScreenPos;
	}
	//Wyrmgus end

	PixelPos targetPos = vp.TilePosToScreen_Center(this->goalPos);
	targetPos += PixelPos(this->GetUnitType().get_tile_size() - QSize(1, 1)) * wyrmgus::defines::get()->get_scaled_tile_size() / 2;

	const int w = (this->GetUnitType().get_box_width() * preferences::get()->get_scale_factor()).to_int();
	const int h = (this->GetUnitType().get_box_height() * preferences::get()->get_scale_factor()).to_int();
	DrawSelection(ColorGray, targetPos.x - w / 2, targetPos.y - h / 2, targetPos.x + w / 2, targetPos.y + h / 2, render_commands);

	if (preferences::get()->are_pathlines_enabled()) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorGreen, targetPos, (3 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}

	return targetPos;
}

void COrder_Build::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(this->Type->BoolFlag[BUILDEROUTSIDE_INDEX].value ? 1 : 0);
	input.SetMaxRange(this->Range);

	const Vec2i tileSize(this->Type->get_tile_size());
	input.SetGoal(this->goalPos, tileSize, this->MapLayer);
}

/** Called when unit is killed.
**  warn the AI module.
*/
void COrder_Build::AiUnitKilled(CUnit &unit)
{
	DebugPrint("%d: %d(%s) killed, with order %s!\n" _C_
			   unit.Player->get_index() _C_ UnitNumber(unit) _C_
			   unit.Type->Ident.c_str() _C_ this->Type->Ident.c_str());
	if (this->BuildingUnit == nullptr) {
		//Wyrmgus start
//		AiReduceMadeInBuilt(*unit.Player->Ai, *this->Type);
		AiReduceMadeInBuilt(*unit.Player->Ai, *this->Type, CMap::get()->get_tile_landmass(this->goalPos, this->MapLayer), this->settlement);
		//Wyrmgus end
	}
}



/**
**  Move to build location
**
**  @param unit  Unit to move
*/
bool COrder_Build::MoveToLocation(CUnit &unit)
{
	// First entry
	if (this->State == 0) {
		unit.pathFinderData->output.Cycles = 0; //moving counter
		this->State = 1;
	}
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE: {
			//Wyrmgus start
			if (unit.MapLayer->Field(unit.tilePos)->has_flag(tile_flag::bridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->get_domain() == unit_domain::land) {
				std::vector<CUnit *> table;
				Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitAction::Still) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->goalPos, FlushCommands, this->MapLayer);
						}
						return false;
					}
				}
			}
			//Wyrmgus end
			// Some tries to reach the goal
			if (this->State++ < State_MoveToLocationMax) {
				// To keep the load low, retry each 1/4 second.
				// NOTE: we can already inform the AI about this problem?
				unit.Wait = CYCLES_PER_SECOND / 4;
				return false;
			}

			//Wyrmgus start
//			unit.Player->Notify(NotifyYellow, unit.tilePos, "%s", _("You cannot reach building place"));
			unit.Player->Notify(NotifyYellow, unit.tilePos, unit.MapLayer->ID, _("%s cannot reach building place"), unit.GetMessageName().c_str());
			//Wyrmgus end
			if (unit.Player->AiEnabled) {
				//Wyrmgus start
//				AiCanNotReach(unit, this->GetUnitType());
				AiCanNotReach(unit, this->GetUnitType(), CMap::get()->get_tile_landmass(this->goalPos, this->MapLayer), this->settlement);
				//Wyrmgus end
			}
			return true;
		}
		case PF_REACHED:
			this->State = State_NearOfLocation;
			return false;

		default:
			// Moving...
			return false;
	}
}

static bool CheckLimit(const CUnit &unit, const wyrmgus::unit_type &type, const landmass *landmass, const wyrmgus::site *settlement)
{
	const CPlayer &player = *unit.Player;
	bool isOk = true;

	// Check if enough resources for the building.
	if (player.CheckUnitType(type)) {
		// FIXME: Better tell what is missing?
		player.Notify(NotifyYellow, unit.tilePos,
					  //Wyrmgus start
					  unit.MapLayer->ID,
//					  _("Not enough resources to build %s"), type.Name.c_str());
					  _("Not enough resources to build %s"), type.GetDefaultName(&player).c_str());
					  //Wyrmgus end
		isOk = false;
	}

	// Check if hiting any limits for the building.
	if (player.CheckLimits(type) < 0) {
		player.Notify(NotifyYellow, unit.tilePos,
					  //Wyrmgus start
					  unit.MapLayer->ID,
//					  _("Can't build more units %s"), type.Name.c_str());
					  _("Can't build more units %s"), type.GetDefaultName(&player).c_str());
					  //Wyrmgus end
		isOk = false;
	}
	
	if (isOk == false && player.AiEnabled) {
		AiCanNotBuild(unit, type, landmass, settlement);
	}
	return isOk;
}


class AlreadyBuildingFinder
{
public:
	AlreadyBuildingFinder(const CUnit &unit, const wyrmgus::unit_type &t) :
		worker(&unit), type(&t) {}
	bool operator()(const CUnit *const unit) const
	{
		return (!unit->Destroyed && unit->Type == type
				&& (worker->Player == unit->Player || worker->is_allied_with(*unit)));
	}
	CUnit *Find(const wyrmgus::tile *const mf) const
	{
		return mf->UnitCache.find(*this);
	}
private:
	const CUnit *worker;
	const wyrmgus::unit_type *type;
};

/**
**  Check if the unit can build
**
**  @param unit  Unit to check
**
**  @return OnTop or null
*/
CUnit *COrder_Build::CheckCanBuild(CUnit &unit) const
{
	const Vec2i pos = this->goalPos;
	const wyrmgus::unit_type &type = this->GetUnitType();

	// Check if the building could be built there.

	//Wyrmgus start
//	CUnit *ontop = CanBuildUnitType(&unit, type, pos, 1);
	bool ignore_exploration = unit.Player->AiEnabled;
	CUnit *ontop = CanBuildUnitType(&unit, type, pos, 1, ignore_exploration, this->MapLayer);
	//Wyrmgus end

	if (ontop != nullptr) {
		return ontop;
	}
	return nullptr;
}

/**
**  Replaces this build command with repair.
**
**  @param unit     Builder who got this build order
**  @param building Building to repair
*/
void COrder_Build::HelpBuild(CUnit &unit, CUnit &building)
{
//Wyrmgus start
//#if 0
//Wyrmgus end
	/*
	 * FIXME: rb - CheckAlreadyBuilding should be somehow
	 * enabled/disable via game lua scripting
	 */
	if (unit.CurrentOrder() == this) {
		  DebugPrint("%d: Worker [%d] is helping build: %s [%d]\n"
		  //Wyrmgus start
//		  _C_ unit.Player->get_index() _C_ unit.Slot
//		  _C_ building->Type->Name.c_str()
//		  _C_ building->Slot);
		  _C_ unit.Player->get_index() _C_ UnitNumber(unit)
		  _C_ building.Type->GetDefaultName(unit.Player).c_str()
		  _C_ UnitNumber(building));
		  //Wyrmgus end

		//insert a repair order right after the current order
		unit.Orders.insert(unit.Orders.begin() + 1, COrder::NewActionRepair(building));
		return;
	}
//Wyrmgus start
//#endif
//Wyrmgus end
}

bool COrder_Build::StartBuilding(CUnit &unit, CUnit &ontop)
{
	const wyrmgus::unit_type &type = this->GetUnitType();

	unit.Player->SubUnitType(type);

	CUnit *build = MakeUnit(type, unit.Player);
	
	//Wyrmgus start
	build->Name.clear(); // under construction buildings should have no proper name
	//Wyrmgus end
	
	// If unable to make unit, stop, and report message
	if (build == nullptr) {
		// FIXME: Should we retry this?
		unit.Player->Notify(NotifyYellow, unit.tilePos,
							//Wyrmgus start
							unit.MapLayer->ID,
//							_("Unable to create building %s"), type.Name.c_str());
							_("Unable to create building %s"), type.GetDefaultName(unit.Player).c_str());
							//Wyrmgus end
		if (unit.Player->AiEnabled) {
			AiCanNotBuild(unit, type, CMap::get()->get_tile_landmass(this->goalPos, this->MapLayer), this->settlement);
		}
		return false;
	}
	build->UnderConstruction = 1;
	build->CurrentSightRange = 0;
	build->MapLayer = CMap::get()->MapLayers[this->MapLayer].get();

	// Building on top of something, may remove what is beneath it
	if (&ontop != &unit) {
		//Wyrmgus start
//		const CBuildRestrictionOnTop *b = OnTopDetails(*build, ontop.Type));
		const CBuildRestrictionOnTop *b = OnTopDetails(*build->Type, ontop.Type);
		//Wyrmgus end
		assert_throw(b != nullptr);
		if (b->ReplaceOnBuild) {
			build->ReplaceOnTop(ontop);
		}
	}

	// Must set action before placing, otherwise it will incorrectly mark radar
	build->Orders[0] = COrder::NewActionBuilt(unit, *build);

	UpdateUnitSightRange(*build);
	// Must place after previous for map flags
	//Wyrmgus start
//	build->Place(this->goalPos);
	build->Place(this->goalPos, this->MapLayer);
	//Wyrmgus end

	//Wyrmgus start
	build->Player->NumBuildingsUnderConstruction++;
	build->Player->ChangeUnitTypeUnderConstructionCount(&type, 1);
	//Wyrmgus end

	// HACK: the building is not ready yet
	build->Player->DecreaseCountsForUnit(build);

	// We need somebody to work on it.
	if (!type.BoolFlag[BUILDEROUTSIDE_INDEX].value) {
		UnitShowAnimation(unit, unit.get_animation_set()->Still.get());
		unit.Remove(build);
		this->State = State_BuildFromInside;
		if (unit.Selected) {
			SelectedUnitChanged();
		}
	} else {
		this->State = State_BuildFromOutside;
		this->BuildingUnit = build->acquire_ref();
		//Wyrmgus start
//		unit.Direction = DirectionToHeading(build->tilePos - unit.tilePos);
//		UnitUpdateHeading(unit);
		const Vec2i dir = PixelSize(PixelSize(build->tilePos) * wyrmgus::defines::get()->get_tile_size()) + build->get_half_tile_pixel_size() - PixelSize(PixelSize(unit.tilePos) * wyrmgus::defines::get()->get_tile_size()) - unit.get_half_tile_pixel_size();
		UnitHeadingFromDeltaXY(unit, dir);
		//Wyrmgus end
	}

	return true;
}

void COrder_Build::ConvertUnitType(const CUnit &unit, wyrmgus::unit_type &newType)
{
	Q_UNUSED(unit)
	
	this->Type = &newType;
}

static void AnimateActionBuild(CUnit &unit)
{
	const wyrmgus::animation_set *animations = unit.get_animation_set();

	if (animations == nullptr) {
		return ;
	}
	if (animations->Build) {
		UnitShowAnimation(unit, animations->Build.get());
	} else if (animations->Repair) {
		UnitShowAnimation(unit, animations->Repair.get());
	}
}


/**
**  Build the building
**
**  @param unit  worker which build.
*/
bool COrder_Build::BuildFromOutside(CUnit &unit) const
{
	AnimateActionBuild(unit);

	if (this->get_building_unit() == nullptr) {
		return false;
	}

	if (this->get_building_unit()->CurrentAction() == UnitAction::Built) {
		COrder_Built &target_order = *static_cast<COrder_Built *>(this->get_building_unit()->CurrentOrder());
		CUnit &goal = *this->get_building_unit();

		target_order.ProgressHp(goal, 100);
	}
	if (unit.Anim.Unbreakable) {
		return false;
	}
	return this->get_building_unit()->CurrentAction() != UnitAction::Built;
}

void COrder_Build::Execute(CUnit &unit)
{
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
	if (this->State <= State_MoveToLocationMax) {
		if (this->MoveToLocation(unit)) {
			this->Finished = true;
			return;
		}
	}

	const wyrmgus::unit_type &type = this->GetUnitType();

	if (State_NearOfLocation <= this->State && this->State < State_StartBuilding_Failed) {
		//Wyrmgus start
		/*
		if (CheckLimit(unit, type) == false) {
			this->Finished = true;
			return ;
		}
		*/
		//Wyrmgus end
		CUnit *ontop = this->CheckCanBuild(unit);

		if (ontop != nullptr) {
			//Wyrmgus start
			if (CheckLimit(unit, type, CMap::get()->get_tile_landmass(this->goalPos, this->MapLayer), this->settlement) == false) {
				this->Finished = true;
				return;
			}
			//Wyrmgus end
			this->StartBuilding(unit, *ontop);
		} else { /* can't be built */
			// Check if already building
			const Vec2i pos = this->goalPos;

			CUnit *building = AlreadyBuildingFinder(unit, type).Find(CMap::get()->Field(pos, this->MapLayer));

			if (building != nullptr) {
				this->HelpBuild(unit, *building);
				this->Finished = true;
				return;
			}

			// failed, retry later
			this->State++;
			// To keep the load low, retry each 10 cycles
			// NOTE: we can already inform the AI about this problem?
			unit.Wait = 10;
		}
	}
	if (this->State == State_StartBuilding_Failed) {
		unit.Player->Notify(NotifyYellow, unit.tilePos,
							//Wyrmgus start
							unit.MapLayer->ID,
//							_("You cannot build %s here"), type.Name.c_str());
							_("You cannot build a %s here"), type.GetDefaultName(unit.Player).c_str());
							//Wyrmgus end
		if (unit.Player->AiEnabled) {
			//Wyrmgus start
//			AiCanNotBuild(unit, type);
			AiCanNotBuild(unit, type, CMap::get()->get_tile_landmass(this->goalPos, this->MapLayer), this->settlement);
			//Wyrmgus end
		}
		this->Finished = true;
		return ;
	}
	if (this->State == State_BuildFromOutside) {
		//Wyrmgus start
//		if (this->BuildFromOutside(unit)) {
//			this->Finished = true;
//		}
		if (this->get_building_unit() != nullptr) {
			this->HelpBuild(unit, *this->get_building_unit());
		}
		this->Finished = true;
		//Wyrmgus end
	}
}

CUnit *COrder_Build::get_building_unit() const
{
	if (this->BuildingUnit == nullptr) {
		return nullptr;
	}

	return this->BuildingUnit->get();
}
