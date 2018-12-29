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
/**@name unit_save.cpp - Save unit. */
//
//      (c) Copyright 1998-2008 by Lutz Sammer and Jimmy Salmon
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

#include <sstream>
#include <iomanip>

#include "stratagus.h"

#include "unit/unit.h"

#include "actions.h"
#include "animation.h"
#include "character.h"
#include "construct.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "pathfinder.h"
#include "player.h"
#include "spells.h"
#include "unit/unittype.h"

#include <stdio.h>

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Generate a unit reference, a printable unique string for unit.
*/
std::string UnitReference(const CUnit &unit)
{
	std::ostringstream ss;
	ss << "U" << std::setfill('0') << std::setw(4) << std::uppercase
	   << std::hex << UnitNumber(unit);
	return ss.str();
}

/**
**  Generate a unit reference, a printable unique string for unit.
*/
std::string UnitReference(const CUnitPtr &unit)
{
	Assert(unit != nullptr);

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

	file.printf("\"player\", %d,\n  ", unit.Player->Index);

	if (unit.MapLayer) {
		file.printf("\"map-layer\", %d, ", unit.MapLayer->ID);
	}
	file.printf("\"tile\", {%d, %d}, ", unit.tilePos.x, unit.tilePos.y);
	file.printf("\"seen-tile\", {%d, %d}, ", unit.Seen.tilePos.x, unit.Seen.tilePos.y);

	file.printf("\"refs\", %d, ", unit.Refs);
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
	file.printf("\"stats\", %d,\n  ", unit.Player->Index);
#endif
	file.printf("\"pixel\", {%d, %d}, ", unit.IX, unit.IY);
	file.printf("\"seen-pixel\", {%d, %d}, ", unit.Seen.IX, unit.Seen.IY);
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
		file.printf("\"trait\", \"%s\", ", unit.Trait->Ident.c_str());
	}
	file.printf("\"personal-name\", \"%s\", ", unit.Name.c_str());
	if (!unit.ExtraName.empty()) {
		file.printf("\"extra-name\", \"%s\", ", unit.ExtraName.c_str());
	}
	if (!unit.FamilyName.empty()) {
		file.printf("\"family-name\", \"%s\", ", unit.FamilyName.c_str());
	}
	if (unit.Settlement) {
		file.printf("\"settlement\", \"%s\", ", unit.Settlement->Ident.c_str());
	}
	if (unit.Prefix != nullptr) {
		file.printf("\"prefix\", \"%s\", ", unit.Prefix->Ident.c_str());
	}
	if (unit.Suffix != nullptr) {
		file.printf("\"suffix\", \"%s\", ", unit.Suffix->Ident.c_str());
	}
	if (unit.Spell != nullptr) {
		file.printf("\"spell\", \"%s\", ", unit.Spell->Ident.c_str());
	}
	if (unit.Work != nullptr) {
		file.printf("\"work\", \"%s\", ", unit.Work->Ident.c_str());
	}
	if (unit.Elixir != nullptr) {
		file.printf("\"elixir\", \"%s\", ", unit.Elixir->Ident.c_str());
	}
	if (unit.Unique) {
		file.printf("\"unique\", \"%s\", ", unit.Unique->Ident.c_str());
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
		file.printf(" \"rescued-from\", %d,", unit.RescuedFrom->Index);
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
					unit.Container->Type->TileSize.x,
					unit.Container->Type->TileSize.y);
	}
	file.printf(" \"seen-by-player\", \"");
	for (int i = 0; i < PlayerMax; ++i) {
		file.printf("%c", (unit.Seen.ByPlayer & (1 << i)) ? 'X' : '_');
	}
	file.printf("\",\n ");
	file.printf(" \"seen-destroyed\", \"");
	for (int i = 0; i < PlayerMax; ++i) {
		file.printf("%c", (unit.Seen.Destroyed & (1 << i)) ? 'X' : '_');
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
	file.printf("\"step-count\", %d,\n  ", unit.StepCount);

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
					DefaultResourceNames[unit.CurrentResource].c_str());
	}
	
	//Wyrmgus start
	if (unit.GivesResource) {
		file.printf("\"gives-resource\", \"%s\",\n  ",
					DefaultResourceNames[unit.GivesResource].c_str());
	}
	//Wyrmgus end

	unit.pathFinderData->input.Save(file);
	unit.pathFinderData->output.Save(file);

	file.printf("\"wait\", %d, ", unit.Wait);
	CAnimations::SaveUnitAnim(file, unit);
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

	if (unit.NextWorker) {
		if (unit.NextWorker->Destroyed) {
			/* this unit is destroyed so it's not in the global unit
			 * array - this means it won't be saved!!! */
			printf("FIXME: storing destroyed Worker - loading will fail.\n");
		}
		file.printf(" \"next-worker\", \"%s\",", UnitReference(*unit.NextWorker).c_str());
	}

	if (unit.Resource.Workers != nullptr) {
		file.printf(" \"resource-active\", %d,", unit.Resource.Active);
		file.printf(" \"resource-assigned\", %d,", unit.Resource.Assigned);
		file.printf(" \"resource-workers\", \"%s\",", UnitReference(*unit.Resource.Workers).c_str());
	} else {
		Assert(unit.Resource.Active == 0);
		Assert(unit.Resource.Assigned == 0);
	}
	file.printf(" \"units-boarded-count\", %d,", unit.BoardCount);

	//Wyrmgus start
//	if (unit.UnitInside) {
	if (unit.UnitInside && !(unit.Character && unit.HasInventory())) { // don't save items for persistent heroes
	//Wyrmgus end
		file.printf("\n  \"units-contained\", {");
		CUnit *uins = unit.UnitInside->PrevContained;
		for (int i = unit.InsideCount; i; --i, uins = uins->PrevContained) {
			file.printf("\"%s\"", UnitReference(*uins).c_str());
			if (i > 1) {
				file.printf(", ");
			}
		}
		file.printf("},\n  ");
	}
	file.printf("\"orders\", {\n");
	Assert(unit.Orders.empty() == false);
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
	if (unit.CriticalOrder) {
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
	if (unit.AutoCastSpell) {
		for (size_t i = 0; i < CSpell::Spells.size(); ++i) {
			if (unit.AutoCastSpell[i]) {
				file.printf(",\n  \"auto-cast\", \"%s\"", CSpell::Spells[i]->Ident.c_str());
			}
		}
	}
	if (unit.SpellCoolDownTimers) {
		file.printf(",\n  \"spell-cooldown\", {");
		for (size_t i = 0; i < CSpell::Spells.size(); ++i) {
			if (i) {
				file.printf(" ,");
			}
			file.printf("%d", unit.SpellCoolDownTimers[i]);
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
	for (std::map<CUnitType *, int>::const_iterator iterator = unit.Type->Stats[unit.Player->Index].UnitStock.begin(); iterator != unit.Type->Stats[unit.Player->Index].UnitStock.end(); ++iterator) {
		CUnitType *unit_type = iterator->first;
		
		if (unit.GetUnitStock(unit_type) != 0) {
			file.printf(",\n  \"unit-stock\", \"%s\", %d", unit_type->Ident.c_str(), unit.GetUnitStock(unit_type));
		}
		
		if (unit.GetUnitStockReplenishmentTimer(unit_type) != 0) {
			file.printf(",\n  \"unit-stock-replenishment-timer\", \"%s\", %d", unit_type->Ident.c_str(), unit.GetUnitStockReplenishmentTimer(unit_type));
		}
	}
	for (std::map<int, int>::const_iterator iterator = unit.IndividualUpgrades.begin(); iterator != unit.IndividualUpgrades.end(); ++iterator) {
		int upgrade_id = iterator->first;
		CUpgrade *upgrade = AllUpgrades[upgrade_id];
		
		if (unit.GetIndividualUpgrade(upgrade)) {
			file.printf(",\n  \"individual-upgrade\", \"%s\", %d", upgrade->Ident.c_str(), unit.GetIndividualUpgrade(upgrade));
		}
	}
	if (unit.RallyPointPos.x != -1 && unit.RallyPointPos.y != -1) {
		file.printf(",\n  \"rally-point\", %d, %d", unit.RallyPointPos.x, unit.RallyPointPos.y);
		file.printf(",\n  \"rally-point-map-layer\", %d, ", unit.RallyPointMapLayer);
	}
	if (unit.Character != nullptr && unit.CurrentAction() != UnitActionDie && !unit.Destroyed) {
		if (!unit.Character->Custom) {
			file.printf(",\n  \"character\", \"%s\"", unit.Character->Ident.c_str());
		} else {
			file.printf(",\n  \"custom-hero\", \"%s\"", unit.Character->Ident.c_str());
		}
	}
	//Wyrmgus end

	file.printf("})\n");
}

//@}
