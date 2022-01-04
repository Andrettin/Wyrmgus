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

#include <iomanip>

#include "stratagus.h"

#include "unit/unit.h"

#include "actions.h"
#include "animation.h"
#include "character.h"
#include "economy/resource.h"
#include "iolib.h"
#include "item/unique_item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "pathfinder.h"
#include "player/player.h"
#include "spell/spell.h"
#include "spell/status_effect.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"

/**
**  Generate a unit reference, a printable unique string for unit.
*/
std::string UnitReference(const CUnit *unit)
{
	assert_throw(unit != nullptr);

	std::ostringstream ss;
	ss << "U" << std::setfill('0') << std::setw(4) << std::uppercase
	   << std::hex << UnitNumber(*unit);
	return ss.str();
}

void PathFinderInput::Save(CFile &file) const
{
	file.printf("\"pathfinder-input\", {");

	if (this->isRecalculatePathNeeded) {
		file.printf("\"invalid\"");
	} else {
		file.printf("\"unit-size\", {%d, %d}, ", this->unitSize.x, this->unitSize.y);
		file.printf("\"goalpos\", {%d, %d}, ", this->goalPos.x, this->goalPos.y);
		file.printf("\"goal-size\", {%d, %d}, ", this->goalSize.x, this->goalSize.y);
		file.printf("\"minrange\", %d, ", this->minRange);
		file.printf("\"maxrange\", %d", this->maxRange);
	}
	file.printf("},\n  ");
}

void PathFinderOutput::Save(CFile &file) const
{
	file.printf("\"pathfinder-output\", {");

	if (this->Fast) {
		file.printf("\"fast\", ");
	}
	if (this->Length > 0) {
		file.printf("\"path\", {");
		for (int i = 0; i < this->Length; ++i) {
			file.printf("%d, ", this->Path[i]);
		}
		file.printf("},");
	}
	file.printf("\"cycles\", %d", this->Cycles);

	file.printf("},\n  ");
}


/**
**  Save the state of a unit to file.
**
**  @param unit  Unit pointer to be saved.
**  @param file  Output file.
*/
void SaveUnit(const CUnit &unit, CFile &file)
{
	file.printf("\nUnit(%d, {", UnitNumber(unit));

	// 'type and 'player must be first, needed to create the unit slot
	file.printf("\"type\", \"%s\", ", unit.Type->Ident.c_str());
	if (unit.Seen.Type) {
		file.printf("\"seen-type\", \"%s\", ", unit.Seen.Type->Ident.c_str());
	}

	file.printf("\"player\", %d,\n  ", unit.Player->get_index());

	if (unit.MapLayer) {
		file.printf("\"map-layer\", %d, ", unit.MapLayer->ID);
	}
	file.printf("\"tile\", {%d, %d}, ", unit.tilePos.x, unit.tilePos.y);
	file.printf("\"seen-tile\", {%d, %d}, ", unit.Seen.tilePos.x, unit.Seen.tilePos.y);

#if 0
	// latimerius: why is this so complex?
	// JOHNS: An unit can be owned by a new player and have still the old stats
	for (i = 0; i < PlayerMax; ++i) {
		if (&unit.Type->Stats[i] == unit.Stats) {
			file.printf("\"stats\", %d,\n  ", i);
			break;
		}
	}
	// latimerius: what's the point of storing a pointer value anyway?
	if (i == PlayerMax) {
		file.printf("\"stats\", \"S%08X\",\n  ", (int)unit.Stats);
	}
#else
	file.printf("\"stats\", %d,\n  ", unit.Player->get_index());
#endif
	file.printf("\"pixel\", {%d, %d}, ", unit.get_pixel_offset().x(), unit.get_pixel_offset().y());
	file.printf("\"seen-pixel\", {%d, %d}, ", unit.Seen.pixel_offset.x(), unit.Seen.pixel_offset.y());
	file.printf("\"frame\", %d, ", unit.Frame);
	if (unit.Seen.Frame != UnitNotSeen) {
		file.printf("\"seen\", %d, ", unit.Seen.Frame);
	} else {
		file.printf("\"not-seen\", ");
	}
	file.printf("\"direction\", %d,\n  ", unit.Direction);
	file.printf("\"damage-type\", %d,", unit.DamagedType);
	file.printf("\"attacked\", %lu,\n ", unit.Attacked);
	//Wyrmgus start
	if (unit.Trait != nullptr) {
		file.printf("\"trait\", \"%s\", ", unit.Trait->get_identifier().c_str());
	}
	file.printf("\"personal-name\", \"%s\", ", unit.Name.c_str());
	if (!unit.ExtraName.empty()) {
		file.printf("\"extra-name\", \"%s\", ", unit.ExtraName.c_str());
	}
	if (!unit.get_surname().empty()) {
		file.printf("\"surname\", \"%s\", ", unit.get_surname().c_str());
	}
	if (unit.get_site() != nullptr) {
		file.printf("\"site\", \"%s\", ", unit.get_site()->get_identifier().c_str());
	}
	if (unit.settlement != nullptr) {
		file.printf("\"settlement\", \"%s\", ", unit.settlement->get_identifier().c_str());
	}
	if (unit.Prefix != nullptr) {
		file.printf("\"prefix\", \"%s\", ", unit.Prefix->get_identifier().c_str());
	}
	if (unit.Suffix != nullptr) {
		file.printf("\"suffix\", \"%s\", ", unit.Suffix->get_identifier().c_str());
	}
	if (unit.Spell != nullptr) {
		file.printf("\"spell\", \"%s\", ", unit.Spell->get_identifier().c_str());
	}
	if (unit.Work != nullptr) {
		file.printf("\"work\", \"%s\", ", unit.Work->get_identifier().c_str());
	}
	if (unit.Elixir != nullptr) {
		file.printf("\"elixir\", \"%s\", ", unit.Elixir->get_identifier().c_str());
	}
	if (unit.get_unique() != nullptr) {
		file.printf("\"unique\", \"%s\", ", unit.get_unique()->get_identifier().c_str());
	}
	if (unit.Bound) {
		file.printf("\"bound\", true, ");
	}
	if (!unit.Identified) {
		file.printf("\"identified\", false, ");
	}
	if (unit.Type->BoolFlag[ITEM_INDEX].value && unit.Container != nullptr && unit.Container->IsItemEquipped(&unit)) {
		file.printf("\"equipped\", true, ");
	}
	if (unit.Container != nullptr && std::find(unit.Container->SoldUnits.begin(), unit.Container->SoldUnits.end(), &unit) != unit.Container->SoldUnits.end()) {
		file.printf("\"sold-unit\", true, ");
	}
	if (unit.ConnectingDestination != nullptr) {
		file.printf("\"connecting-destination\", %d, ", UnitNumber(*unit.ConnectingDestination));
	}
	//Wyrmgus end
	file.printf(" \"current-sight-range\", %d,", unit.CurrentSightRange);
	if (unit.Burning) {
		file.printf(" \"burning\",");
	}
	if (unit.Destroyed) {
		file.printf(" \"destroyed\",");
	}
	if (unit.Removed) {
		file.printf(" \"removed\",");
	}
	if (unit.Selected) {
		file.printf(" \"selected\",");
	}
	if (unit.Summoned) {
		file.printf(" \"summoned\",");
	}
	if (unit.Waiting) {
		file.printf(" \"waiting\",");
	}
	if (unit.MineLow) {
		file.printf(" \"mine-low\",");
	}
	if (unit.RescuedFrom) {
		file.printf(" \"rescued-from\", %d,", unit.RescuedFrom->get_index());
	}
	
	//Wyrmgus start
	// have variables before the host-info, since the latter maps ethereal vision sight
	for (size_t i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
		if (unit.Variable[i] != unit.Type->DefaultStat.Variables[i]) {
			file.printf("\"%s\", {Value = %d, Max = %d, Increase = %d, Enable = %s},\n  ",
						UnitTypeVar.VariableNameLookup[i], unit.Variable[i].Value, unit.Variable[i].Max,
						unit.Variable[i].Increase, unit.Variable[i].Enable ? "true" : "false");
		}
	}
	//Wyrmgus end
	
	// n0b0dy: How is this useful?
	// mr-russ: You can't always load units in order, it saved the information
	// so you can load a unit whose Container hasn't been loaded yet.
	// SEE unit loading code.
	if (unit.Container && unit.Removed) {
		file.printf(" \"host-info\", {%d, %d, %d, %d, %d}, ",
					unit.Container->MapLayer->ID,
					unit.Container->tilePos.x, unit.Container->tilePos.y,
					unit.Container->Type->get_tile_width(),
					unit.Container->Type->get_tile_height());
	}
	file.printf(" \"seen-by-player\", \"");
	for (int i = 0; i < PlayerMax; ++i) {
		file.printf("%c", unit.is_seen_by_player(i) ? 'X' : '_');
	}
	file.printf("\",\n ");
	file.printf(" \"seen-destroyed\", \"");
	for (int i = 0; i < PlayerMax; ++i) {
		file.printf("%c", unit.is_seen_destroyed_by_player(i) ? 'X' : '_');
	}
	file.printf("\",\n ");
	if (unit.UnderConstruction) {
		file.printf(" \"under-construction\",");
	}
	if (unit.Seen.UnderConstruction) {
		file.printf(" \"seen-under-construction\",");
	}
	file.printf(" \"seen-state\", %d, ", unit.Seen.State);
	if (unit.Active) {
		file.printf(" \"active\",");
	}
	file.printf("\"ttl\", %lu,\n  ", unit.TTL);
	file.printf("\"threshold\", %d,\n  ", unit.Threshold);
	file.printf("\"step-count\", %d,\n  ", unit.get_step_count());

	//Wyrmgus start
//	for (size_t i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
//		if (unit.Variable[i] != unit.Type->DefaultStat.Variables[i]) {
//			file.printf("\"%s\", {Value = %d, Max = %d, Increase = %d, Enable = %s},\n  ",
//						UnitTypeVar.VariableNameLookup[i], unit.Variable[i].Value, unit.Variable[i].Max,
//						unit.Variable[i].Increase, unit.Variable[i].Enable ? "true" : "false");
//		}
//	}
	//Wyrmgus end

	file.printf("\"group-id\", %d,\n  ", unit.GroupId);
	file.printf("\"last-group\", %d,\n  ", unit.LastGroup);

	file.printf("\"resources-held\", %d,\n  ", unit.ResourcesHeld);
	if (unit.CurrentResource) {
		file.printf("\"current-resource\", \"%s\",\n  ",
					unit.get_current_resource()->get_identifier().c_str());
	}
	
	//Wyrmgus start
	if (unit.GivesResource) {
		file.printf("\"gives-resource\", \"%s\",\n  ",
					unit.get_given_resource()->get_identifier().c_str());
	}
	//Wyrmgus end

	unit.pathFinderData->input.Save(file);
	unit.pathFinderData->output.Save(file);

	file.printf("\"wait\", %d, ", unit.Wait);
	wyrmgus::animation_set::SaveUnitAnim(file, unit);
	file.printf(",\n  \"blink\", %d,", unit.Blink);
	if (unit.Moving) {
		file.printf(" \"moving\",");
	}
	if (unit.ReCast) {
		file.printf(" \"re-cast\",");
	}
	if (unit.Boarded) {
		file.printf(" \"boarded\",");
	}
	if (unit.AutoRepair) {
		file.printf(" \"auto-repair\",");
	}

	if (!unit.Resource.Workers.empty()) {
		file.printf(" \"resource-active\", %d,", unit.Resource.Active);
		file.printf("\n  \"resource-workers\", {");
		for (size_t i = 0; i < unit.Resource.Workers.size(); ++i) {
			const std::shared_ptr<wyrmgus::unit_ref> &worker_ref = unit.Resource.Workers[i];
			const CUnit *worker = worker_ref->get();

			if (worker->Destroyed) {
				/* this unit is destroyed so it's not in the global unit
				 * array - this means it won't be saved!!! */
				printf("FIXME: storing destroyed Worker - loading will fail.\n");
			}

			if (i > 0) {
				file.printf(", ");
			}
			file.printf("\"%s\"", UnitReference(worker).c_str());
		}
		file.printf("},\n  ");
	} else {
		assert_log(unit.Resource.Active == 0);
	}
	file.printf(" \"units-boarded-count\", %d,", unit.BoardCount);

	//Wyrmgus start
//	if (unit.has_units_inside()) {
	if (unit.has_units_inside() && !(unit.get_character() != nullptr && unit.HasInventory())) { //don't save items for persistent heroes
	//Wyrmgus end
		file.printf("\n  \"units-contained\", {");
		bool first = true;
		for (const CUnit *uins : unit.get_units_inside()) {
			file.printf("\"%s\"", UnitReference(uins).c_str());
			if (first) {
				first = false;
			} else {
				file.printf(", ");
			}
		}
		file.printf("},\n  ");
	}
	file.printf("\"orders\", {\n");
	assert_throw(unit.Orders.empty() == false);
	unit.Orders[0]->Save(file, unit);
	for (size_t i = 1; i != unit.Orders.size(); ++i) {
		file.printf(",\n ");
		unit.Orders[i]->Save(file, unit);
	}
	file.printf("}");
	if (unit.SavedOrder) {
		file.printf(",\n  \"saved-order\", ");
		unit.SavedOrder->Save(file, unit);
	}
	if (unit.CriticalOrder != nullptr) {
		file.printf(",\n  \"critical-order\", ");
		unit.CriticalOrder->Save(file, unit);
	}
	if (unit.NewOrder) {
		file.printf(",\n  \"new-order\", ");
		unit.NewOrder->Save(file, unit);
	}

	if (unit.Goal) {
		file.printf(",\n  \"goal\", %d", UnitNumber(*unit.Goal));
	}
	for (const wyrmgus::spell *spell : unit.get_autocast_spells()) {
		file.printf(",\n  \"auto-cast\", \"%s\"", spell->get_identifier().c_str());
	}

	if (!unit.get_spell_cooldown_timers().empty()) {
		file.printf(",\n  \"spell-cooldown\", {");
		bool first = true;
		for (const auto &kv_pair : unit.get_spell_cooldown_timers()) {
			if (first) {
				first = false;
			} else {
				file.printf(" ,");
			}
			file.printf("\"%s\", %d", kv_pair.first->get_identifier().c_str(), kv_pair.second);
		}
		file.printf("}");
	}

	if (!unit.get_status_effect_timers().empty()) {
		file.printf(",\n  \"status-effects\", {");
		bool first = true;
		for (const auto &[status_effect, cycles] : unit.get_status_effect_timers()) {
			if (first) {
				first = false;
			} else {
				file.printf(" ,");
			}
			file.printf("\"%s\", %d", status_effect_to_string(status_effect).c_str(), cycles);
		}
		file.printf("}");
	}

	//Wyrmgus start
	file.printf(",\n  \"variation\", %d", unit.Variation);
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (unit.LayerVariation[i] != -1) {
			file.printf(",\n  \"layer-variation\", \"%s\", %d", GetImageLayerNameById(i).c_str(), unit.LayerVariation[i]);
		}
	}

	for (const auto &kv_pair : unit.Type->Stats[unit.Player->get_index()].get_unit_stocks()) {
		const unit_type *unit_type = kv_pair.first;

		if (unit.GetUnitStock(unit_type) != 0) {
			file.printf(",\n  \"unit-stock\", \"%s\", %d", unit_type->Ident.c_str(), unit.GetUnitStock(unit_type));
		}

		if (unit.GetUnitStockReplenishmentTimer(unit_type) != 0) {
			file.printf(",\n  \"unit-stock-replenishment-timer\", \"%s\", %d", unit_type->Ident.c_str(), unit.GetUnitStockReplenishmentTimer(unit_type));
		}
	}

	for (std::map<int, int>::const_iterator iterator = unit.IndividualUpgrades.begin(); iterator != unit.IndividualUpgrades.end(); ++iterator) {
		int upgrade_id = iterator->first;
		CUpgrade *upgrade = CUpgrade::get_all()[upgrade_id];
		
		if (unit.GetIndividualUpgrade(upgrade)) {
			file.printf(",\n  \"individual-upgrade\", \"%s\", %d", upgrade->get_identifier().c_str(), unit.GetIndividualUpgrade(upgrade));
		}
	}
	if (unit.get_rally_point_pos().x() != -1 && unit.get_rally_point_pos().y() != -1 && unit.get_rally_point_map_layer() != nullptr) {
		file.printf(",\n  \"rally_point\", %d, %d", unit.get_rally_point_pos().x(), unit.get_rally_point_pos().y());
		file.printf(",\n  \"rally_point_map_layer\", %d, ", unit.get_rally_point_map_layer()->ID);
	}
	if (unit.get_character() != nullptr && unit.CurrentAction() != UnitAction::Die && !unit.Destroyed) {
		if (!unit.get_character()->is_custom()) {
			file.printf(",\n  \"character\", \"%s\"", unit.get_character()->get_identifier().c_str());
		} else {
			file.printf(",\n  \"custom-hero\", \"%s\"", unit.get_character()->get_identifier().c_str());
		}
	}
	//Wyrmgus end

	if (unit.Player->get_last_created_unit() == &unit) {
		file.printf(",\n  \"last_created_player_unit\"");
	}

	file.printf("})\n");
}
