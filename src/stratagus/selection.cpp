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
/**@name selection.cpp - The units' selection. */
//
//      (c) Copyright 1999-2021 by Patrice Fortier, Lutz Sammer,
//      Jimmy Salmon and Andrettin
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

//Wyrmgus start
#include "ai/ai_local.h" // for AiHelpers
//Wyrmgus end
#include "commands.h"
#include "database/preferences.h"
//Wyrmgus start
#include "game/game.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "network.h"
#include "player/player.h"
#include "script.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/size_util.h"

unsigned int MaxSelectable;								/// Maximum number of selected units

std::vector<CUnit *> Selected;							/// All selected units
static std::vector<CUnit *> TeamSelected[PlayerMax];	/// teams currently selected units

static std::vector<CUnit *> _Selected;					/// save of Selected
static std::vector<CUnit *> _TeamSelected[PlayerMax];	/// save of TeamSelected
static CUnit *_TrackUnit = nullptr;						/// save of the unit being tracked (if any)

static unsigned GroupId;								/// Unique group # for automatic groups

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

bool IsOnlySelected(const CUnit &unit)
{
	return !Selected.empty() && Selected[0] == &unit;
}

/**
**  Save selection to restore after.
*/
void SaveSelection()
{
	for (int i = 0; i < PlayerMax; ++i) {
		_TeamSelected[i] = TeamSelected[i];
	}
	_Selected = Selected;
	_TrackUnit = UI.SelectedViewport->Unit;
}

/**
**  Restore selection.
*/
void RestoreSelection()
{
	UnSelectAll();
	for (int i = 0; i < PlayerMax; ++i) {
		TeamSelected[i] = _TeamSelected[i];
		for (size_t j = 0; j != _TeamSelected[i].size(); ++j) {
			TeamSelected[i][j]->TeamSelected |= (1 << i);
		}
	}
	Selected = _Selected;
	for (size_t i = 0; i != _Selected.size(); ++i) {
		Selected[i]->Selected = 1;
	}
	UI.SelectedViewport->Unit = _TrackUnit;
}

/**
**  Unselect all the units in the current selection
*/
void UnSelectAll()
{
	while (!++GroupId) { // Advance group id, but keep non zero
	}

	for (size_t i = 0; i != Selected.size(); ++i) {
		Selected[i]->Selected = 0;
	}
	Selected.clear();
	UI.SelectedViewport->Unit = nullptr;
}

/**
**  Handle a suicide unit click
**
**  @param unit  suicide unit.
**  @todo remove static (bug with save/load...)
*/
static void HandleSuicideClick(CUnit &unit)
{
	static int NumClicks = 0;

	assert_throw(unit.Type->ClicksToExplode > 0);
	if (GameObserve || GameEstablishing) {
		return;
	}
	if (IsOnlySelected(unit)) {
		NumClicks++;
	} else {
		NumClicks = 1;
	}
	if (NumClicks == unit.Type->ClicksToExplode) {
		SendCommandDismiss(unit);
		NumClicks = 0;
	}
}

/**
**  Replace a group of selected units by an other group of units.
**
**  @param units  Array of units to be selected.
**  @param count  Number of units in array to be selected.
*/
static void ChangeSelectedUnits(CUnit * const *units, unsigned int count)
{
	assert_throw(count <= MaxSelectable);

	if (count == 1 && units[0]->Type->ClicksToExplode &&
		!units[0]->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value) {
		HandleSuicideClick(*units[0]);
		if (!units[0]->IsAlive()) {
			NetworkSendSelection(units, count);
			return;
		}
	}

	//Wyrmgus start
	bool suitable_selectee = false;
	for (unsigned int i = 0; i < count; ++i) {
		CUnit &unit = *units[i];
		if (!unit.Removed && !unit.TeamSelected && !unit.Type->BoolFlag[ISNOTSELECTABLE_INDEX].value && unit.IsAlive()) {
			suitable_selectee = true;
		}
	}
	if (!suitable_selectee) {
		return;
	}
	//Wyrmgus end
	UnSelectAll();
	NetworkSendSelection(units, count);
	for (unsigned int i = 0; i < count; ++i) {
		CUnit &unit = *units[i];
		if (!unit.Removed && !unit.TeamSelected && !unit.Type->BoolFlag[ISNOTSELECTABLE_INDEX].value && unit.IsAlive()) {
			Selected.push_back(&unit);
			unit.Selected = 1;
			if (count > 1) {
				unit.LastGroup = GroupId;
			}
		}
	}
}

/**
**  Change A Unit Selection from my Team
**
**  @param player  The Player who is selecting the units
**  @param units   The Units to add/remove
*/
void ChangeTeamSelectedUnits(CPlayer &player, const std::vector<CUnit *> &units)
{
	// UnSelectAllTeam(player);
	while (!TeamSelected[player.get_index()].empty()) {
		CUnit *unit = TeamSelected[player.get_index()].back();
		TeamSelected[player.get_index()].pop_back();
		unit->TeamSelected &= ~(1 << player.get_index());
	}
	// Add to selection
	for (size_t i = 0; i != units.size(); ++i) {
		CUnit &unit = *units[i];
		assert_throw(!unit.Removed);
		if (!unit.Type->BoolFlag[ISNOTSELECTABLE_INDEX].value && unit.IsAlive()) {
			TeamSelected[player.get_index()].push_back(&unit);
			unit.TeamSelected |= 1 << player.get_index();
		}
	}
	assert_throw(TeamSelected[player.get_index()].size() <= MaxSelectable);
}

/**
**  Add a unit to the other selected units.
**
**  @param unit  Pointer to unit to add.
**
**  @return      true if added to selection, false otherwise
**               (if Selected.size() == MaxSelectable or
**                unit is already selected or unselectable)
*/
int SelectUnit(CUnit &unit)
{
	if (unit.Type->BoolFlag[REVEALER_INDEX].value) { // Revealers cannot be selected
		DebugPrint("Selecting revealer?\n");
		return 0;
	}

	if (unit.Removed) { // Removed cannot be selected
		DebugPrint("Selecting removed?\n");
		return 0;
	}

	if (Selected.size() == MaxSelectable) {
		return 0;
	}

	if (unit.Selected) {
		return 0;
	}

	if (unit.Type->BoolFlag[ISNOTSELECTABLE_INDEX].value && GameRunning) {
		return 0;
	}

	//Wyrmgus start
	if (!unit.IsAlive()) { // don't select dead units
		return 0;
	}
	//Wyrmgus end
	
	Selected.push_back(&unit);
	unit.Selected = 1;
	if (Selected.size() > 1) {
		Selected[0]->LastGroup = unit.LastGroup = GroupId;
	}
	return 1;
}

/**
**  Select a single unit, unselecting the previous ones
**
**  @param unit  Pointer to unit to be selected.
*/
void SelectSingleUnit(CUnit &unit)
{
	CUnit *unitPtr = &unit;

	ChangeSelectedUnits(&unitPtr, 1);
}

/**
**  Unselect unit
**
**  @param unit  Pointer to unit to be unselected.
*/
void UnSelectUnit(CUnit &unit)
{
	if (unit.TeamSelected) {
		for (int i = 0; i < PlayerMax; ++i) {
			if (unit.TeamSelected & (1 << i)) {
				size_t j;
				for (j = 0; TeamSelected[i][j] != &unit; ++i) {
					// Empty
				}
				assert_throw(j < TeamSelected[i].size());

				std::swap(TeamSelected[i][j], TeamSelected[i].back());
				TeamSelected[i].pop_back();
				unit.TeamSelected &= ~(1 << i);
			}
		}
	}
	//Wyrmgus start
//	if (!unit.Selected) {
	if (!unit.Selected || SaveGameLoading) { //loaded characters are unselected if their unit type is different than that of the unit, but the Selected vector hasn't been loaded yet
	//Wyrmgus end
		return;
	}
	std::vector<CUnit *>::iterator it = std::find(Selected.begin(), Selected.end(), &unit);
	assert_throw(it != Selected.end());

	*it = Selected.back();
	Selected.pop_back();

	if (Selected.size() > 1) { // Assign new group to remaining units
		while (!++GroupId) { // Advance group id, but keep non zero
		}
		for (size_t i = 0; i != Selected.size(); ++i) {
			Selected[i]->LastGroup = GroupId;
		}
	}
	unit.Selected = 0;

	//Turn track unit mode off
	UI.SelectedViewport->Unit = nullptr;
}

//Wyrmgus start
bool UnitCanBeSelectedWith(const CUnit &first_unit, const CUnit &second_unit)
{
	if (first_unit.Type->BoolFlag[BUILDING_INDEX].value != second_unit.Type->BoolFlag[BUILDING_INDEX].value) {
		return false;
	}

	if (first_unit.Type->BoolFlag[BUILDING_INDEX].value) {
		if (second_unit.Type != first_unit.Type) {
			return false;
		}
	}
	
	return true;
}
//Wyrmgus end

/**
**  Toggle the selection of a unit in a group of selected units
**
**  @param unit  Pointer to unit to be toggled.
**  @return      0 if unselected, 1 otherwise
*/
int ToggleSelectUnit(CUnit &unit)
{
	if (unit.Selected) {
		UnSelectUnit(unit);
		return 0;
	}
	//Wyrmgus start
	if (Selected.size() && !UnitCanBeSelectedWith(*Selected[0], unit)) {
		return 0;
	}
	//Wyrmgus end
	SelectUnit(unit);
	return 1;
}

/**
**  Select units from a particular type and belonging to the local player.
**
**  The base is included in the selection and defines
**  the type of the other units to be selected.
**
**  @param base  Select all units of same type.
**
**  @return      Number of units found, 0 means selection unchanged
**
**  FIXME: 0 can't happen. Maybe when scripting will use it?
**
**  FIXME: should always select the nearest 9 units to the base!
*/
int SelectUnitsByType(CUnit &base, bool only_visible)
{
	const wyrmgus::unit_type &type = *base.Type;
	const CViewport *vp = UI.MouseViewport;

	assert_throw(UI.MouseViewport != nullptr);

	if (type.ClicksToExplode) {
		HandleSuicideClick(base);
	}

	// if unit is a cadaver or hidden (not on map)
	// no unit can be selected.
	if (base.Removed || base.IsAlive() == false) {
		return 0;
	}

	if (type.BoolFlag[ISNOTSELECTABLE_INDEX].value && GameRunning) {
		return 0;
	}
	if (base.TeamSelected) { // Somebody else onteam has this unit
		return 0;
	}
	
	//Wyrmgus start
	if (!base.IsAlive()) { // dead units cannot be selected
		return 0;
	}
	//Wyrmgus end

	UnSelectAll();
	Selected.push_back(&base);
	base.Selected = 1;

	// if unit isn't belonging to the player or allied player, or is a static unit
	// (like a building), only 1 unit can be selected at the same time.
	if (!CanSelectMultipleUnits(*base.Player) || !type.BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
		return Selected.size();
	}

	//
	// Search for other visible units of the same type
	//
	std::vector<CUnit *> table;
	// select all visible units.
	// StephanR: should be (MapX,MapY,MapX+MapWidth-1,MapY+MapHeight-1) ???
	/* FIXME: this should probably be cleaner implemented if SelectUnitsByType()
	 * took parameters of the selection rectangle as arguments */
	const Vec2i offset(1, 1);
	//Wyrmgus start
//	const Vec2i minPos = vp->MapPos - offset;
	Vec2i minPos = vp->MapPos - offset;
	//Wyrmgus end
	const Vec2i vpSize(vp->MapWidth, vp->MapHeight);
	//Wyrmgus start
//	const Vec2i maxPos = vp->MapPos + vpSize + offset;
	Vec2i maxPos = vp->MapPos + vpSize + offset;
	//Wyrmgus end
	
	//Wyrmgus start
	if (!only_visible) {
		minPos.x = 0;
		minPos.y = 0;
		maxPos.x = UI.CurrentMapLayer->get_width() - 1;
		maxPos.y = UI.CurrentMapLayer->get_height() - 1;
	}
	//Wyrmgus end
	//Wyrmgus start
//	Select(minPos, maxPos, table, HasSameTypeAs(type));
	Select(minPos, maxPos, table, UI.CurrentMapLayer->ID, HasSameTypeAs(type));
	//Wyrmgus end

	// FIXME: peon/peasant with gold/wood & co are considered from
	//   different type... idem for tankers
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];
		if (!CanSelectMultipleUnits(*unit.Player)) {
			continue;
		}
		if (unit.IsUnusable()) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (&unit == &base) {  // no need to have the same unit twice :)
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		Selected.push_back(&unit);
		unit.Selected = 1;
		if (Selected.size() == MaxSelectable) {
			break;
		}
	}
	if (Selected.size() > 1) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			Selected[i]->LastGroup = GroupId;
		}
	}
	NetworkSendSelection(&Selected[0], Selected.size());
	return Selected.size();
}

/**
**  Toggle units from a particular type and belonging to the local player.
**
**  The base is included in the selection and defines
**  the type of the other units to be selected.
**
**  @param base  Toggle all units of same type.
**
**  @return      Number of units found, 0 means selection unchanged
**
**  FIXME: toggle not written
**  FIXME: should always select the nearest 9 units to the base!
*/
int ToggleUnitsByType(CUnit &base)
{
	const wyrmgus::unit_type &type = *base.Type;

	// if unit is a cadaver or hidden (not on map)
	// no unit can be selected.
	if (base.Removed || base.IsAlive() == false) {
		return 0;
	}
	// if unit isn't belonging to the player, or is a static unit
	// (like a building), only 1 unit can be selected at the same time.
	if (!CanSelectMultipleUnits(*base.Player) || !type.BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
		return 0;
	}

	//Wyrmgus start
	if (Selected.size() && !UnitCanBeSelectedWith(*Selected[0], base)) {
		return 0;
	}
	//Wyrmgus end
	
	if (!SelectUnit(base)) { // Add base to selection
		return 0;
	}

	//  Search for other visible units of the same type
	
	// select all visible units.
	// StephanR: should be (MapX,MapY,MapX+MapWidth-1,MapY+MapHeight-1) ???
	// FIXME: this should probably be cleaner implemented if SelectUnitsByType()
	// took parameters of the selection rectangle as arguments */
	const CViewport *vp = UI.MouseViewport;
	const Vec2i offset(1, 1);
	const Vec2i minPos = vp->MapPos - offset;
	const Vec2i vpSize(vp->MapWidth, vp->MapHeight);
	const Vec2i maxPos = vp->MapPos + vpSize + offset;
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(minPos, maxPos, table, HasSameTypeAs(type));
	Select(minPos, maxPos, table, UI.CurrentMapLayer->ID, HasSameTypeAs(type));
	//Wyrmgus end

	// FIXME: peon/peasant with gold/wood & co are considered from
	// different type... idem for tankers
	for (size_t i = 0; i < table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!CanSelectMultipleUnits(*unit.Player)) {
			continue;
		}
		if (unit.IsUnusable()) { // guess SelectUnits doesn't check this
			continue;
		}
		if (&unit == &base) { // no need to have the same unit twice
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		//Wyrmgus start
		if (Selected.size() && !UnitCanBeSelectedWith(*Selected[0], unit)) {
			continue;
		}
		//Wyrmgus end
		if (!SelectUnit(unit)) { // add unit to selection
			return Selected.size();
		}
	}

	NetworkSendSelection(&Selected[0], Selected.size());
	return Selected.size();
}

/**
**  Change selected units to units from group group_number
**  Doesn't change the selection if the group has no unit.
**
**  @param group_number  number of the group to be selected.
**
**  @return              number of units in the group.
*/
int SelectGroup(int group_number, GroupSelectionMode mode)
{
	const std::vector<CUnit *> &units = GetUnitsOfGroup(group_number);

	if (units.empty()) {
		return 0;
	}
	if (mode == GroupSelectionMode::SELECT_ALL || !IsGroupTainted(group_number)) {
		ChangeSelectedUnits(&units[0], units.size());
		return Selected.size();
	}
	std::vector<CUnit *> table;

	for (size_t i = 0; i != units.size(); ++i) {
		const wyrmgus::unit_type *type = units[i]->Type;
		if (type && type->CanSelect(mode)) {
			table.push_back(units[i]);
		}
	}
	if (table.empty() == false) {
		ChangeSelectedUnits(&table[0], table.size());
		return Selected.size();
	}
	return 0;
}

/**
**  Add units from group of a particular unit to selection.
**
**  @param unit  unit belonging to the group to be selected.
**
**  @return      0 if the unit doesn't belong to a group,
**               or the number of units in the group.
*/
int AddGroupFromUnitToSelection(const CUnit &group_unit)
{
	const unsigned int group = group_unit.LastGroup;

	if (!group) { // belongs to no group
		return 0;
	}
	
	//Wyrmgus start
	if (Selected.size() && !UnitCanBeSelectedWith(*Selected[0], group_unit)) {
		return 0;
	}
	//Wyrmgus end

	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		if (unit->LastGroup == group && !unit->Removed) {
			SelectUnit(*unit);
			if (Selected.size() == MaxSelectable) {
				return Selected.size();
			}
		}
	}

	return Selected.size();
}

/**
**  Select units from group of a particular unit.
**  Doesn't change the selection if the group has no unit,
**  or the unit doesn't belong to any group.
**
**  @param unit  unit belonging to the group to be selected.
**
**  @return      0 if the unit doesn't belong to a group,
**               or the number of units in the group.
*/
int SelectGroupFromUnit(const CUnit &unit)
{
	if (!unit.LastGroup) { // belongs to no group
		return 0;
	}

	UnSelectAll();
	return AddGroupFromUnitToSelection(unit);
}

/**
**  Select the units selectable by rectangle in a local table.
**  Act like a filter: The source table is modified.
**  Return the original table if no unit is found.
**
**  @param table      Input/Output table of units.
**
**  return true if at least a unit is found;
*/
//Wyrmgus start
//static bool SelectOrganicUnitsInTable(std::vector<CUnit *> &table)
static bool SelectOrganicUnitsInTable(std::vector<CUnit *> &table, bool added_table)
//Wyrmgus end
{
	unsigned int n = 0;
	
	//Wyrmgus start
	//check if has non-building
	bool hasNonBuilding = false;
	
	if (added_table == false) {
		for (size_t i = 0; i != table.size(); ++i) {
			CUnit &unit = *table[i];
			
			if (!unit.Type->BoolFlag[BUILDING_INDEX].value) {
				hasNonBuilding = true;
			}
		}
	}
	//Wyrmgus end

	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!CanSelectMultipleUnits(*unit.Player) || !unit.Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
			continue;
		}
		if (unit.IsUnusable()) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		//Wyrmgus start
		//only select buildings if another building of the same type is already selected
		if (added_table == false && unit.Type->BoolFlag[BUILDING_INDEX].value && ((i != 0 && !UnitCanBeSelectedWith(*table[0], unit)) || hasNonBuilding)) {
			continue;
		}
		//Wyrmgus end
		table[n++] = &unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	if (n != 0) {
		table.resize(n);
		return true;
	}
	return false;
}

/**
**  Selects units from the table whose sprite is at least partially
**  covered by the rectangle. The rectangle is determined by coordinates
**  of its upper left and lower right corner expressed in screen map
**  coordinate system.
**
**  @param corner_topleft      coord of upper left corner of the rectangle
**  @param corner_bottomright  coord of lower right corner of the rectangle
**  @param table               table of units
**
**  @return           number of units found
*/
static void SelectSpritesInsideRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright,
										 std::vector<CUnit *> &table)
{
	int n = 0;
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];
		const wyrmgus::unit_type &type = *unit.Type;
		QPoint sprite_pos = unit.get_scaled_map_pixel_pos_center();
		sprite_pos += type.get_offset() * scale_factor - (size::to_point(type.get_box_size() * scale_factor) + QPoint(type.BoxOffsetX, type.BoxOffsetY) * scale_factor) / 2;

		if (sprite_pos.x() + (type.get_box_width() * scale_factor).to_int() + (type.BoxOffsetX * scale_factor).to_int() < corner_topleft.x
			|| sprite_pos.x() > corner_bottomright.x
			|| sprite_pos.y() + (type.get_box_height() * scale_factor).to_int() + (type.BoxOffsetY * scale_factor).to_int() < corner_topleft.y
			|| sprite_pos.y() > corner_bottomright.y) {
			continue;
		}
		table[n++] = &unit;
	}
	table.resize(n);
}

/**
**  Select units in a rectangle.
**  Proceed in order in none found:
**    @li select local player mobile units
**    @li select one local player static unit (random)
**    @li select one neutral unit (critter, mine...)
**    @li select one enemy unit (random)
**
**  @param corner_topleft,  start of selection rectangle
**  @param corner_bottomright end of selection rectangle
**
**  @return     the number of units found.
*/
int SelectUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright)
{
	const Vec2i t0 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_topleft);
	const Vec2i t1 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_bottomright);
	const Vec2i range(2, 2);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(t0 - range, t1 + range, table);
	Select(t0 - range, t1 + range, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	SelectSpritesInsideRectangle(corner_topleft, corner_bottomright, table);

	// 1) search for the player units selectable with rectangle
	//Wyrmgus start
//	if (SelectOrganicUnitsInTable(table)) {
	if (SelectOrganicUnitsInTable(table, false)) {
	//Wyrmgus end
		const int size = static_cast<int>(table.size());
		ChangeSelectedUnits(&table[0], size);
		return size;
	}

	// 2) If no unit found, try a player's unit not selectable by rectangle
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!CanSelectMultipleUnits(*unit.Player)) {
			continue;
		}
		// FIXME: Can we get this?
		if (!unit.Removed && unit.IsAlive()) {
			SelectSingleUnit(unit);
			return 1;
		}
	}

	// 3) If no unit found, try a resource or a neutral critter
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];
		// Unit visible FIXME: write function UnitSelectable
		if (!unit.IsVisibleInViewport(*UI.SelectedViewport)) {
			continue;
		}
		const wyrmgus::unit_type &type = *unit.Type;
		// Buildings are visible but not selectable
		if (type.BoolFlag[BUILDING_INDEX].value && !unit.IsVisibleOnMap(*CPlayer::GetThisPlayer())) {
			continue;
		}
		if ((type.get_given_resource() != nullptr && !unit.Removed)) { // no built resources.
			SelectSingleUnit(unit);
			return 1;
		}
	}

	// 4) If no unit found, select an enemy unit (first found)
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];
		// Unit visible FIXME: write function UnitSelectable
		if (!unit.IsVisibleInViewport(*UI.SelectedViewport)) {
			continue;
		}
		// Buildings are visible but not selectable
		if (unit.Type->BoolFlag[BUILDING_INDEX].value && !unit.IsVisibleOnMap(*CPlayer::GetThisPlayer())) {
			continue;
		}
		if (unit.IsAliveOnMap()) {
			SelectSingleUnit(unit);
			return 1;
		}
	}
	return 0;
}

//Wyrmgus start
/**
**  Select entire army.
**
**
**  @return     the number of units found.
*/
int SelectArmy()
{
	const Vec2i minPos(0, 0);
	const Vec2i maxPos(UI.CurrentMapLayer->get_width() - 1, UI.CurrentMapLayer->get_height() - 1);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(minPos, maxPos, table);
	Select(minPos, maxPos, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	unsigned int n = 0;
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!CanSelectMultipleUnits(*unit.Player) || !unit.Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
			continue;
		}
		if (unit.IsUnusable()) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit.Type->get_domain() == unit_domain::water || unit.Type->get_domain() == unit_domain::air || unit.Type->get_domain() == unit_domain::space) {
			continue;
		}
		if (unit.TeamSelected) { // Somebody else on team has this unit
			continue;
		}
		if (unit.Type->BoolFlag[BUILDING_INDEX].value) { //this selection mode is not for buildings
			continue;
		}
		if (unit.Type->BoolFlag[HARVESTER_INDEX].value) { //this selection mode is not for workers
			continue;
		}
		table[n++] = &unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	if (n) {
		ChangeSelectedUnits(&table[0], n);
	}
	return n;
}
//Wyrmgus end

/**
**  Add the units in the rectangle to the current selection
**
**  @param corner_topleft,  start of selection rectangle
**  @param corner_bottomright end of selection rectangle
**
**  @return    the _total_ number of units selected.
*/
int AddSelectedUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright)
{
	// Check if the original selected unit (if it's alone) is ours,
	// and can be selectable by rectangle.
	// In this case, do nothing.
	if (Selected.size() == 1
		&& (!CanSelectMultipleUnits(*Selected[0]->Player)
			|| !Selected[0]->Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value)) {
		return Selected.empty();
	}
	// If there is no selected unit yet, do a simple selection.
	if (Selected.empty()) {
		return SelectUnitsInRectangle(corner_topleft, corner_bottomright);
	}
	const Vec2i tilePos0 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_topleft);
	const Vec2i tilePos1 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_bottomright);
	const Vec2i range(2, 2);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(tilePos0 - range, tilePos1 + range, table);
	Select(tilePos0 - range, tilePos1 + range, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	SelectSpritesInsideRectangle(corner_topleft, corner_bottomright, table);
	// If no unit in rectangle area... do nothing
	if (table.empty()) {
		return Selected.size();
	}

	// Now we should only have mobile (organic) units belonging to us,
	// so if there's no such units in the rectangle, do nothing.
	//Wyrmgus start
//	if (SelectOrganicUnitsInTable(table) == false) {
	if (SelectOrganicUnitsInTable(table, true) == false) {
	//Wyrmgus end
		return Selected.size();
	}

	for (size_t i = 0; i < table.size() && Selected.size() < MaxSelectable; ++i) {
		//Wyrmgus start
		if (Selected.size() && !UnitCanBeSelectedWith(*Selected[0], *table[i])) {
			continue;
		}
		//Wyrmgus end
		SelectUnit(*table[i]);
	}
	return Selected.size();
}

/**
**  Select own ground units in a rectangle.
**
**  @param corner_topleft,  start of selection rectangle
**  @param corner_bottomright end of selection rectangle
**
**  @return     the number of units found.
*/
int SelectGroundUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright)
{
	const Vec2i t0 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_topleft);
	const Vec2i t1 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_bottomright);
	const Vec2i range(2, 2);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(t0 - range, t1 + range, table);
	Select(t0 - range, t1 + range, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	SelectSpritesInsideRectangle(corner_topleft, corner_bottomright, table);

	unsigned int n = 0;
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!CanSelectMultipleUnits(*unit.Player) || !unit.Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
			continue;
		}
		if (unit.IsUnusable()) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit.Type->get_domain() == unit_domain::air || unit.Type->get_domain() == unit_domain::space) {
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		//Wyrmgus start
		if (unit.Type->BoolFlag[BUILDING_INDEX].value) { //this selection mode is not for buildings
			continue;
		}
		//Wyrmgus end
		table[n++] = &unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	if (n) {
		ChangeSelectedUnits(&table[0], n);
	}
	return n;
}

/**
**  Select own air units in a rectangle.
**
**  @param corner_topleft,  start of selection rectangle
**  @param corner_bottomright end of selection rectangle
**
**  @return     the number of units found.
*/
int SelectAirUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright)
{
	const Vec2i t0 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_topleft);
	const Vec2i t1 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_bottomright);
	const Vec2i range(2, 2);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(t0 - range, t1 + range, table);
	Select(t0 - range, t1 + range, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	SelectSpritesInsideRectangle(corner_topleft, corner_bottomright, table);
	unsigned int n = 0;
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];
		if (!CanSelectMultipleUnits(*unit.Player) || !unit.Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
			continue;
		}
		if (unit.IsUnusable()) { // guess SelectUnits doesn't check this
			continue;
		}
		if (unit.Type->get_domain() != unit_domain::air) {
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		//Wyrmgus start
		if (unit.Type->BoolFlag[BUILDING_INDEX].value) { //this selection mode is not for buildings
			continue;
		}
		//Wyrmgus end
		table[n++] = &unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	if (n) {
		ChangeSelectedUnits(&table[0], n);
	}
	return n;
}


/**
**  Add the ground units in the rectangle to the current selection
**
**  @param corner_topleft,     start of selection rectangle
**  @param corner_bottomright  end of selection rectangle
**
**  @return     the number of units found.
*/
int AddSelectedGroundUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright)
{
	// Check if the original selected unit (if it's alone) is ours,
	// and can be selectable by rectangle.
	// In this case, do nothing.
	if (Selected.size() == 1
		&& (!CanSelectMultipleUnits(*Selected[0]->Player)
			|| !Selected[0]->Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value)) {
		return Selected.size();
	}

	// If there is no selected unit yet, do a simple selection.
	//Wyrmgus start
//	if (Selected.empty()) {
	if (Selected.empty() || (Selected.size() && Selected[0]->Type->BoolFlag[BUILDING_INDEX].value)) {
	//Wyrmgus end
		return SelectGroundUnitsInRectangle(corner_topleft, corner_bottomright);
	}

	const Vec2i t0 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_topleft);
	const Vec2i t1 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_bottomright);
	const Vec2i range(2, 2);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(t0 - range, t1 + range, table);
	Select(t0 - range, t1 + range, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	SelectSpritesInsideRectangle(corner_topleft, corner_bottomright, table);

	unsigned int n = 0;
	for (size_t i = 0; i < table.size(); ++i) {
		CUnit &unit = *table[i];

		if (!CanSelectMultipleUnits(*unit.Player) ||
			!unit.Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
			continue;
		}
		if (unit.IsUnusable()) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit.Type->get_domain() == unit_domain::air || unit.Type->get_domain() == unit_domain::space) {
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		//Wyrmgus start
		if (unit.Type->BoolFlag[BUILDING_INDEX].value) { //this selection mode is not for buildings
			continue;
		}
		//Wyrmgus end
		table[n++] = &unit;
		if (n == MaxSelectable) {
			break;
		}
	}

	// Add the units to selected.
	for (unsigned int i = 0; i < n && Selected.size() < MaxSelectable; ++i) {
		SelectUnit(*table[i]);
	}
	return Selected.size();
}

/**
**  Add the air units in the rectangle to the current selection
**
**  @param corner_topleft,     start of selection rectangle
**  @param corner_bottomright  end of selection rectangle
**
**  @return     the number of units found.
*/
int AddSelectedAirUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright)
{
	// Check if the original selected unit (if it's alone) is ours,
	// and can be selectable by rectangle.
	// In this case, do nothing.
	if (Selected.size() == 1
		&& (!CanSelectMultipleUnits(*Selected[0]->Player)
			|| !Selected[0]->Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value)) {
		return Selected.size();
	}

	// If there is no selected unit yet, do a simple selection.
	//Wyrmgus start
//	if (Selected.empty()) {
	if (Selected.empty() || (Selected.size() && Selected[0]->Type->BoolFlag[BUILDING_INDEX].value)) {
	//Wyrmgus end
		return SelectAirUnitsInRectangle(corner_topleft, corner_bottomright);
	}

	const Vec2i t0 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_topleft);
	const Vec2i t1 = CMap::get()->scaled_map_pixel_pos_to_tile_pos(corner_bottomright);
	const Vec2i range(2, 2);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(t0 - range, t1 + range, table);
	Select(t0 - range, t1 + range, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	SelectSpritesInsideRectangle(corner_topleft, corner_bottomright, table);
	unsigned int n = 0;
	for (size_t i = 0; i < table.size(); ++i) {
		CUnit &unit = *table[i];
		if (!CanSelectMultipleUnits(*unit.Player) ||
			!unit.Type->BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value) {
			continue;
		}
		if (unit.IsUnusable()) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit.Type->get_domain() != unit_domain::air) {
			continue;
		}
		if (unit.TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		//Wyrmgus start
		if (unit.Type->BoolFlag[BUILDING_INDEX].value) { //this selection mode is not for buildings
			continue;
		}
		//Wyrmgus end
		table[n++] = &unit;
		if (n == MaxSelectable) {
			break;
		}
	}

	// Add the units to selected.
	for (unsigned int i = 0; i < n && Selected.size() < MaxSelectable; ++i) {
		SelectUnit(*table[i]);
	}
	return Selected.size();
}


/**
**  Save current selection state.
**
**  @param file  Output file.
*/
void SaveSelections(CFile &file)
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: selection\n\n");

	file.printf("SetGroupId(%d)\n", GroupId);
	file.printf("Selection(%lu, {", (long unsigned int)Selected.size()); // TODO: remove
	for (size_t i = 0; i != Selected.size(); ++i) {
		file.printf("\"%s\", ", UnitReference(Selected[i]).c_str());
	}
	file.printf("})\n");
}

/**
**  Clean up the selection module.
*/
void CleanSelections()
{
	GroupId = 0;
	Selected.clear();
	_Selected.clear();

	for (int i = 0; i < PlayerMax; ++i) {
		TeamSelected[i].clear();
		_TeamSelected[i].clear();
	}
}

// ----------------------------------------------------------------------------

/**
**  Set the current group id. (Needed for load/save)
**
**  @param l  Lua state.
*/
static int CclSetGroupId(lua_State *l)
{
	int old;

	LuaCheckArgs(l, 1);
	old = GroupId;
	GroupId = LuaToNumber(l, 1);

	lua_pushnumber(l, old);
	return 1;
}

/**
**  Define the current selection.
**
**  @param l  Lua state.
*/
static int CclSelection(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	//NumSelected = LuaToNumber(l, 1);
	const int args = lua_rawlen(l, 2);
	for (int j = 0; j < args; ++j) {
		const char *str = LuaToString(l, 2, j + 1);
		Selected.push_back(&wyrmgus::unit_manager::get()->GetSlotUnit(strtol(str + 1, nullptr, 16)));
	}
	return 0;
}

/**
**  Register CCL features for selections.
*/
void SelectionCclRegister()
{
	lua_register(Lua, "SetGroupId", CclSetGroupId);
	lua_register(Lua, "Selection", CclSelection);
}
