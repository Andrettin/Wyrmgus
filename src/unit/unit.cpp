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
/**@name unit.cpp - The unit source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "unit/unit.h"

#include "action/action_attack.h"
//Wyrmgus start
#include "action/action_resource.h"
#include "action/action_upgradeto.h"
//Wyrmgus end
#include "action/actions.h"
#include "ai/ai.h"
//Wyrmgus start
#include "ai/ai_local.h" //for using AiHelpers
//Wyrmgus end
#include "animation/animation.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "construct.h"
#include "dependency/dependency.h"
#include "faction.h"
#include "game/game.h"
#include "editor/editor.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "item/item.h"
#include "item/item_class.h"
#include "item/item_slot.h"
#include "item/unique_item.h"
#include "language/language.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "missile/burning_building_frame.h"
#include "missile/missile.h"
#include "missile/missile_type.h"
#include "network/network.h"
#include "pathfinder/pathfinder.h"
#include "player.h"
#include "quest/achievement.h"
//Wyrmgus start
#include "quest/quest.h"
//Wyrmgus end
#include "religion/deity.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "sound/unit_sound.h"
#include "species/gender.h"
#include "species/species.h"
#include "spell/spells.h"
#include "time/time_of_day.h"
#include "translate.h"
#include "ui/button_action.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "unit/variable.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "util.h"
#include "video/palette_image.h"
#include "video/video.h"
#include "world/plane.h"
#include "wyrmgus.h"

#include <algorithm>
#include <math.h>

/*----------------------------------------------------------------------------
-- Documentation
----------------------------------------------------------------------------*/

/**
**  @class CUnit unit.h
**
**  \#include "unit/unit.h"
**
**  Everything belonging to a unit. FIXME: rearrange for less memory.
**
**  This class contains all information about a unit in game.
**  A unit could be anything: a man, a vehicle, a ship, or a building.
**  Currently only a tile, a unit, or a missile could be placed on the map.
**
**  The unit structure members:
**
**  CUnit::Refs
**
**  The reference counter of the unit. If the pointer to the unit
**  is stored the counter must be incremented and if this reference
**  is destroyed the counter must be decremented. Alternative it
**  would be possible to implement a garbage collector for this.
**
**  CUnit::Slot
**
**  This is the unique slot number. It is not possible that two
**  units have the same slot number at the same time. The slot
**  numbers are reused.
**  This field could be accessed by the macro UnitNumber(Unit *).
**
**  CUnit::UnitSlot
**
**  This is the index into #Units[], where the unit pointer is
**  stored.  #Units[] is a table of all units currently active in
**  game. This pointer is only needed to speed up, the remove of
**  the unit pointer from #Units[], it didn't must be searched in
**  the table.
**
**  CUnit::PlayerSlot
**
**  The index into Player::Units[], where the unit pointer is
**  stored. Player::Units[] is a table of all units currently
**  belonging to a player. This pointer is only needed to speed
**  up, the remove of the unit pointer from Player::Units[].
**
**  CUnit::Container
**
**  Pointer to the unit containing it, or null if the unit is
**  free. This points to the transporter for units on board, or to
**  the building for peasants inside(when they are mining).
**
**  CUnit::UnitInside
**
**  Pointer to the last unit added inside. Order doesn't really
**  matter. All units inside are kept in a circular linked list.
**  This is null if there are no units inside. Multiple levels
**  of inclusion are allowed, though not very useful right now
**
**  CUnit::NextContained, CUnit::PrevContained
**
**  The next and previous element in the curent container. Bogus
**  values allowed for units not contained.
**
**  CUnit::InsideCount
**
**  The number of units inside the container.
**
**  CUnit::BoardCount
**
**  The number of units transported inside the container. This
**  does not include for instance stuff like harvesters returning
**  cargo.
**
**  CUnit::tilePos
**
**  The tile map coordinates of the unit.
**  0,0 is the upper left on the map.
**
**  CUnit::Type
**
**  Pointer to the unit-type (::UnitType). The unit-type contains
**  all information that all units of the same type shares.
**  (Animations, Name, Stats, ...)
**
**  CUnit::SeenType
**  Pointer to the unit-type that this unit was, when last seen.
**  Currently only used by buildings.
**
**  CUnit::Player
**
**  Pointer to the owner of this unit (::Player). An unit could
**  only be owned by one player.
**
**  CUnit::Stats
**
**  Pointer to the current status (::UnitStats) of a unit. The
**  units of the same player and the same type could share the same
**  stats. The status contains all values which could be different
**  for each player. This f.e. the upgradeable abilities of an
**  unit.  (CUnit::Stats::SightRange, CUnit::Stats::Armor,
**  CUnit::Stats::HitPoints, ...)
**
**  CUnit::CurrentSightRange
**
**  Current sight range of a unit, this changes when a unit enters
**  a transporter or building or exits one of these.
**
**  CUnit::Colors
**
**  Player colors of the unit. Contains the hardware dependent
**  pixel values for the player colors (palette index #208-#211).
**  Setup from the global palette. This is a pointer.
**  @note Index #208-#211 are various SHADES of the team color
**  (#208 is brightest shade, #211 is darkest shade) .... these
**  numbers are NOT red=#208, blue=#209, etc
**
**  CUnit::PixelOffset
**
**  Coordinate displacement in pixels or coordinates inside a tile.
**  Currently only !=0, if the unit is moving from one tile to
**  another (0-32 and for ships/flyers 0-64).
**
**  CUnit::Frame
**
**  Current graphic image of the animation sequence. The high bit
**  (128) is used to flip this image horizontal (x direction).
**  This also limits the number of different frames/image to 126.
**
**  CUnit::SeenFrame
**
**  Graphic image (see CUnit::Frame) what the player on this
**  computer has last seen. If UnitNotSeen the player haven't seen
**  this unit yet.
**
**  CUnit::Direction
**
**  Contains the binary angle (0-255) in which the direction the
**  unit looks. 0, 32, 64, 128, 160, 192, 224, 256 corresponds to
**  0?, 45?, 90?, 135?, 180?, 225?, 270?, 315?, 360? or north,
**  north-east, east, south-east, south, south-west, west,
**  north-west, north. Currently only 8 directions are used, this
**  is more for the future.
**
**  CUnit::Attacked
**
**  Last cycle the unit was attacked. 0 means never.
**
**  CUnit::Burning
**
**  If Burning is non-zero, the unit is burning.
**
**  CUnit::VisCount[PlayerMax]
**
**              Used to keep track of visible units on the map, it counts the
**              Number of seen tiles for each player. This is only modified
**              in UnitsMarkSeen and UnitsUnmarkSeen, from fow.
**              We keep track of visilibty for each player, and combine with
**              Shared vision ONLY when querying and such.
**
**  CUnit::SeenByPlayer
**
**              This is a bitmask of 1 and 0 values. SeenByPlayer & (1<<p) is 0
**              If p never saw the unit and 1 if it did. This is important for
**              keeping track of dead units under fog. We only keep track of units
**              that are visible under fog with this.
**
**  CUnit::Destroyed
**
** @todo docu.
**  If you need more information, please send me an email or write it self.
**
**  CUnit::Removed
**
**  This flag means the unit is not active on map. This flag
**  have workers set if they are inside a building, units that are
**  on board of a transporter.
**
**  CUnit::Selected
**
**  Unit is selected. (So you can give it orders)
**
**  CUnit::UnderConstruction
**  Set when a building is under construction, and still using the
**  generic building animation.
**
**  CUnit::SeenUnderConstruction
**  Last seen state of construction.  Used to draw correct building
**  frame. See CUnit::UnderConstruction for more information.
**
**  CUnit::SeenState
**  The Seen State of the building.
**  01 The building in being built when last seen.
**  10 The building was been upgraded when last seen.
**
**  CUnit::Boarded
**
**  This is 1 if the unit is on board a transporter.
**
**
**  CUnit::XP
**
**  Number of XP of the unit.
**
**  CUnit::Kills
**
**  How many units have been killed by the unit.
**
**  CUnit::GroupId
**
**  Number of the group to that the unit belongs. This is the main
**  group showed on map, a unit can belong to many groups.
**
**  CUnit::LastGroup
**
**  Automatic group number, to reselect the same units. When the
**  user selects more than one unit all units is given the next
**  same number. (Used for ALT-CLICK)
**
**  CUnit::Value
**
**  This values hold the amount of resources in a resource or in
**  in a harvester.
**  @todo continue documentation
**
**  CUnit::Wait
**
**  The unit is forced too wait for that many cycles. Be careful,
**  setting this to 0 will lock the unit.
**
**  CUnit::State
**
**  Animation state, currently position in the animation script.
**  0 if an animation has just started, it should only be changed
**  inside of actions.
**
**  CUnit::Reset
**
**  @todo continue documentation
**
**  CUnit::Blink
**
**
**  CUnit::Moving
**
**
**  CUnit::RescuedFrom
**
**  Pointer to the original owner of a unit. It will be null if
**  the unit was not rescued.
**
**  CUnit::Orders
**
**  Contains all orders of the unit. Slot 0 is always used.
**
**  CUnit::SavedOrder
**
**  This order is executed, if the current order is finished.
**  This is used for attacking units, to return to the old
**  place or for patrolling units to return to patrol after
**  killing some enemies. Any new order given to the unit,
**  clears this saved order.
**
**  CUnit::NewOrder
**
**  This field is only used by buildings and this order is
**  assigned to any by this building new trained unit.
**  This is can be used to set the exit or gathering point of a
**  building.
**
**  CUnit::Goal
**
**  Generic goal pointer. Used by teleporters to point to circle of power.
**
**
** @todo continue documentation
**
*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

bool EnableTrainingQueue;                 /// Config: training queues enabled
bool EnableBuildingCapture = false;               /// Config: capture buildings enabled
bool RevealAttacker;                      /// Config: reveal attacker enabled
int ResourcesMultiBuildersMultiplier = 0; /// Config: spend resources for building with multiple workers

static unsigned long HelpMeLastCycle;     /// Last cycle HelpMe sound played
static int HelpMeLastX;                   /// Last X coordinate HelpMe sound played
static int HelpMeLastY;                   /// Last Y coordinate HelpMe sound played

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

static void RemoveUnitFromContainer(CUnit &unit);

extern int ExtraDeathIndex(const char *death);

/**
**  Increase a unit's reference count.
*/
void CUnit::RefsIncrease()
{
	Assert(Refs && !Destroyed);
	if (!SaveGameLoading) {
		++Refs;
	}
}

/**
**  Decrease a unit's reference count.
*/
void CUnit::RefsDecrease()
{
	Assert(Refs);
	if (!SaveGameLoading) {
		if (Destroyed) {
			if (!--Refs) {
				Release();
			}
		} else {
			--Refs;
			Assert(Refs);
		}
	}
}

void CUnit::Init()
{
	this->Refs = 0;
	this->ReleaseCycle = 0;
	this->PlayerSlot = static_cast<size_t>(-1);
	this->InsideCount = 0;
	this->BoardCount = 0;
	this->UnitInside = nullptr;
	this->Container = nullptr;
	this->NextContained = nullptr;
	this->PrevContained = nullptr;
	this->NextWorker = nullptr;

	this->Resource.Workers = nullptr;
	this->Resource.Assigned = 0;
	this->Resource.Active = 0;
	
	//Wyrmgus start
	this->EquippedItems.clear();
	this->SoldUnits.clear();
	//Wyrmgus end

	this->TilePos.x = 0;
	this->TilePos.y = 0;
	//Wyrmgus start
	this->RallyPointPos.x = -1;
	this->RallyPointPos.y = -1;
	this->MapLayer = nullptr;
	this->RallyPointMapLayer = nullptr;
	//Wyrmgus end
	this->Offset = 0;
	this->Type = nullptr;
	this->Player = nullptr;
	this->Stats = nullptr;
	//Wyrmgus start
	this->Character = nullptr;
	this->Settlement = nullptr;
	this->Trait = nullptr;
	this->Prefix = nullptr;
	this->Suffix = nullptr;
	this->Spell = nullptr;
	this->Work = nullptr;
	this->Elixir = nullptr;
	this->Unique = nullptr;
	this->Bound = false;
	this->Identified = true;
	this->ConnectingDestination = nullptr;
	//Wyrmgus end
	this->CurrentSightRange = 0;

	pathFinderData = new PathFinderData;
	pathFinderData->input.SetUnit(*this);

	Colors = nullptr;
	//Wyrmgus start
	this->Name.clear();
	this->ExtraName.clear();
	this->FamilyName.clear();
	this->Variation = nullptr;
	memset(this->LayerVariation, -1, sizeof(this->LayerVariation));
	//Wyrmgus end
	this->PixelOffset = Vector2i(0, 0);
	this->Frame = 0;
	this->Direction = 0;
	this->DamagedType = ANIMATIONS_DEATHTYPES;
	this->Attacked = 0;
	this->Burning = 0;
	this->Destroyed = 0;
	this->Removed = 0;
	this->Selected = false;
	this->TeamSelected = 0;
	this->UnderConstruction = false;
	this->Active = 0;
	this->Boarded = 0;
	this->RescuedFrom = nullptr;
	memset(this->VisCount, 0, sizeof(this->VisCount));
	memset(&this->Seen, 0, sizeof(this->Seen));
	this->Variable = nullptr;
	this->TTL = 0;
	this->Threshold = 0;
	this->GroupId = 0;
	this->LastGroup = 0;
	this->ResourcesHeld = 0;
	this->Wait = 0;
	this->Blink = 0;
	this->Moving = 0;
	this->ReCast = 0;
	this->CacheLock = 0;
	this->Summoned = 0;
	this->Waiting = 0;
	this->MineLow = 0;
	memset(&this->Anim, 0, sizeof(this->Anim));
	memset(&this->WaitBackup, 0, sizeof(this->WaitBackup));
	this->GivesResource = 0;
	this->CurrentResource = 0;
	this->StepCount = 0;
	this->Orders.clear();
	delete this->SavedOrder;
	this->SavedOrder = nullptr;
	delete this->NewOrder;
	this->NewOrder = nullptr;
	delete this->CriticalOrder;
	this->CriticalOrder = nullptr;
	this->AutoCastSpells.clear();
	this->SpellCoolDownTimers.clear();
	this->AutoRepair = 0;
	this->Goal = nullptr;
	this->IndividualUpgrades.clear();
}

/**
**  Release an unit.
**
**  The unit is only released, if all references are dropped.
*/
void CUnit::Release(bool final)
{
	if (Type == nullptr) {
		DebugPrint("unit already free\n");
		return;
	}
	//Wyrmgus start
	if (Orders.size() != 1) {
		fprintf(stderr, "Unit to be released has more than 1 order; Unit Type: \"%s\", Orders: %d, First Order Type: %d.\n", this->Type->Ident.c_str(), (int)Orders.size(), this->CurrentAction());
	}
	//Wyrmgus end
	Assert(Orders.size() == 1);
	// Must be removed before here
	Assert(Removed);

	// First release, remove from lists/tables.
	if (!Destroyed) {
		DebugPrint("%d: First release %d\n" _C_ Player->GetIndex() _C_ UnitNumber(*this));

		// Are more references remaining?
		Destroyed = 1; // mark as destroyed

		if (Container && !final) {
			if (Boarded) {
				Container->BoardCount--;
			}
			MapUnmarkUnitSight(*this);
			RemoveUnitFromContainer(*this);
		}

		while (Resource.Workers) {
			Resource.Workers->DeAssignWorkerFromMine(*this);
		}

		if (--Refs > 0) {
			return;
		}
	}

	Assert(!Refs);

	//
	// No more references remaining, but the network could have an order
	// on the way. We must wait a little time before we could free the
	// memory.
	//

	Type = nullptr;
	//Wyrmgus start
	Character = nullptr;
	if (this->Settlement && this->Settlement->SiteUnit == this) {
		this->Settlement->SiteUnit = nullptr;
	}
	Settlement = nullptr;
	Trait = nullptr;
	Prefix = nullptr;
	Suffix = nullptr;
	Spell = nullptr;
	Work = nullptr;
	Elixir = nullptr;
	Unique = nullptr;
	Bound = false;
	Identified = true;
	ConnectingDestination = nullptr;
	
	this->EquippedItems.clear();
	this->SoldUnits.clear();
	//Wyrmgus end

	delete pathFinderData;
	this->AutoCastSpells.clear();
	this->SpellCoolDownTimers.clear();
	delete[] Variable;
	for (std::vector<COrder *>::iterator order = Orders.begin(); order != Orders.end(); ++order) {
		delete *order;
	}
	Orders.clear();

	// Remove the unit from the global units table.
	UnitManager.ReleaseUnit(this);
}

void CUnit::SetType(const CUnitType *new_type)
{
	if (new_type == this->Type) {
		return;
	}
	
	const PaletteImage *old_image = this->GetImage();
	
	this->Type = new_type;
	
	const PaletteImage *new_image = this->GetImage();
	
	if (old_image != new_image) {
		this->emit_signal("image_changed", new_image);
	}
}

void CUnit::SetCurrentResource(const unsigned char resource_index)
{
	if (resource_index == this->GetCurrentResource()) {
		return;
	}
	
	const PaletteImage *old_image = this->GetImage();
	
	this->CurrentResource = resource_index;
	
	//emit a signal if the image has changed
	const PaletteImage *new_image = this->GetImage();
	if (old_image != new_image) {
		this->emit_signal("image_changed", new_image);
	}
}

void CUnit::SetResourcesHeld(const int quantity)
{
	if (quantity == this->GetResourcesHeld()) {
		return;
	}
	
	const bool resource_loaded_changed = (this->GetResourcesHeld() > 0) != (quantity > 0);
	
	const PaletteImage *old_image = nullptr;
	if (resource_loaded_changed) {
		old_image = this->GetImage();
	}
	
	this->ResourcesHeld = quantity;
	
	if (resource_loaded_changed) {
		//emit a signal if the image has changed
		const PaletteImage *new_image = this->GetImage();
		if (old_image != new_image) {
			this->emit_signal("image_changed", new_image);
		}
	}
	
	const UnitTypeVariation *variation = this->GetVariation();
	if (
		variation
		&& (
			(variation->ResourceMin && this->GetResourcesHeld() < variation->ResourceMin)
			|| (variation->ResourceMax && this->GetResourcesHeld() > variation->ResourceMax)
		)
	) {
		this->ChooseVariation();
	}
}

void CUnit::ChangeResourcesHeld(const int quantity)
{
	this->SetResourcesHeld(this->GetResourcesHeld() + quantity);
}

void CUnit::ReplaceOnTop(CUnit &replaced_unit)
{
	if (replaced_unit.Unique != nullptr) {
		this->SetUnique(replaced_unit.Unique);
	} else {
		if (replaced_unit.Prefix != nullptr) {
			this->SetPrefix(replaced_unit.Prefix);
		}
		if (replaced_unit.Suffix != nullptr) {
			this->SetSuffix(replaced_unit.Suffix);
		}
		if (replaced_unit.Spell != nullptr) {
			this->SetSpell(replaced_unit.Spell);
		}
	}
	if (replaced_unit.Settlement != nullptr) {
		this->Settlement = replaced_unit.Settlement;
		if (this->Type->BoolFlag[TOWNHALL_INDEX].value) {
			this->Settlement->SiteUnit = this;
			CMap::Map.SiteUnits.erase(std::remove(CMap::Map.SiteUnits.begin(), CMap::Map.SiteUnits.end(), &replaced_unit), CMap::Map.SiteUnits.end());
			CMap::Map.SiteUnits.push_back(this);
		}
	}
	
	this->SetResourcesHeld(replaced_unit.GetResourcesHeld()); // We capture the value of what is beneath.
	this->Variable[GIVERESOURCE_INDEX].Value = replaced_unit.Variable[GIVERESOURCE_INDEX].Value;
	this->Variable[GIVERESOURCE_INDEX].Max = replaced_unit.Variable[GIVERESOURCE_INDEX].Max;
	this->Variable[GIVERESOURCE_INDEX].Enable = replaced_unit.Variable[GIVERESOURCE_INDEX].Enable;
	
	replaced_unit.Remove(nullptr); // Destroy building beneath
	UnitLost(replaced_unit);
	UnitClearOrders(replaced_unit);
	replaced_unit.Release();
}

//Wyrmgus start
void CUnit::ChangeExperience(int amount, int around_range)
{
	std::vector<CUnit *> table;
	if (around_range > 0) {
		SelectAroundUnit(*this, around_range, table, MakeAndPredicate(HasSamePlayerAs(*this->Player), IsNotBuildingType()));
	}
	
	amount /= 1 + table.size();

	if (this->Type->BoolFlag[ORGANIC_INDEX].value) {
		this->Variable[XP_INDEX].Max += amount;
		this->Variable[XP_INDEX].Value = this->Variable[XP_INDEX].Max;
		this->XPChanged();
	}

	if (around_range > 0) {
		for (size_t i = 0; i != table.size(); ++i) {
			if (table[i]->GetType()->BoolFlag[ORGANIC_INDEX].value) {
				table[i]->Variable[XP_INDEX].Max += amount;
				table[i]->Variable[XP_INDEX].Value = table[i]->Variable[XP_INDEX].Max;
				table[i]->XPChanged();
			}
		}
	}
}

void CUnit::IncreaseLevel(int level_quantity, bool automatic_learning)
{
	while (level_quantity > 0) {
		this->Variable[LEVEL_INDEX].Value += 1;
		if (this->Type->Stats[this->Player->GetIndex()].Variables[LEVEL_INDEX].Value < this->Variable[LEVEL_INDEX].Value) {
			if (GetAvailableLevelUpUpgrades(true) == 0 || (this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->GetIndex()].Variables[LEVEL_INDEX].Value) > 1) {
				this->Variable[POINTS_INDEX].Max += 5 * (this->Variable[LEVEL_INDEX].Value + 1);
				this->Variable[POINTS_INDEX].Value += 5 * (this->Variable[LEVEL_INDEX].Value + 1);
			}
			
			this->Variable[LEVELUP_INDEX].Value += 1;
			this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
			// if there are no level-up upgrades available for the unit, increase its HP instead
			if (this->GetAvailableLevelUpUpgrades() < this->Variable[LEVELUP_INDEX].Value) {
				this->Variable[HP_INDEX].Max += 10;
				this->Variable[LEVELUP_INDEX].Value -= 1;
				this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
			}
		}
		this->Variable[HP_INDEX].Value = this->GetModifiedVariable(HP_INDEX, VariableMax);
		level_quantity -= 1;
	}
	
	UpdateXPRequired();
	
	bool upgrade_found = true;
	while (this->Variable[LEVELUP_INDEX].Value > 0 && upgrade_found && automatic_learning) {
		upgrade_found = false;

		if (((int) AiHelpers.ExperienceUpgrades.size()) > this->Type->GetIndex()) {
			std::vector<CUnitType *> potential_upgrades;
			
			if ((this->Player->AiEnabled || this->Character == nullptr) && this->Type->BoolFlag[HARVESTER_INDEX].value && this->GetCurrentResource() && AiHelpers.ExperienceUpgrades[this->Type->GetIndex()].size() > 1) {
				//if is a harvester who is currently gathering, try to upgrade to a unit type which is best for harvesting the current resource
				unsigned int best_gathering_rate = 0;
				for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[this->Type->GetIndex()].size(); ++i) {
					CUnitType *experience_upgrade_type = AiHelpers.ExperienceUpgrades[this->Type->GetIndex()][i];
					if (CheckDependencies(experience_upgrade_type, this, true)) {
						if (this->Character == nullptr || std::find(this->Character->ForbiddenUpgrades.begin(), this->Character->ForbiddenUpgrades.end(), experience_upgrade_type) == this->Character->ForbiddenUpgrades.end()) {
							if (!experience_upgrade_type->ResInfo[this->GetCurrentResource()]) {
								continue;
							}
							unsigned int gathering_rate = experience_upgrade_type->GetResourceStep(this->GetCurrentResource(), this->Player->GetIndex());
							if (gathering_rate >= best_gathering_rate) {
								if (gathering_rate > best_gathering_rate) {
									best_gathering_rate = gathering_rate;
									potential_upgrades.clear();
								}
								potential_upgrades.push_back(experience_upgrade_type);
							}
						}
					}
				}
			} else if (this->Player->AiEnabled || (this->Character == nullptr && AiHelpers.ExperienceUpgrades[this->Type->GetIndex()].size() == 1)) {
				for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[this->Type->GetIndex()].size(); ++i) {
					if (CheckDependencies(AiHelpers.ExperienceUpgrades[this->Type->GetIndex()][i], this, true)) {
						if (this->Character == nullptr || std::find(this->Character->ForbiddenUpgrades.begin(), this->Character->ForbiddenUpgrades.end(), AiHelpers.ExperienceUpgrades[this->Type->GetIndex()][i]) == this->Character->ForbiddenUpgrades.end()) {
							potential_upgrades.push_back(AiHelpers.ExperienceUpgrades[this->Type->GetIndex()][i]);
						}
					}
				}
			}
			
			if (potential_upgrades.size() > 0) {
				this->Variable[LEVELUP_INDEX].Value -= 1;
				this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
				CUnitType *chosen_unit_type = potential_upgrades[SyncRand(potential_upgrades.size())];
				if (this->Player == CPlayer::GetThisPlayer()) {
					this->Player->Notify(NotifyGreen, this->GetTilePos(), this->GetMapLayer()->GetIndex(), _("%s has upgraded to %s!"), this->GetMessageName().c_str(), chosen_unit_type->GetName().utf8().get_data());
				}
				TransformUnitIntoType(*this, *chosen_unit_type);
				upgrade_found = true;
			}
		}
			
		if ((this->Player->AiEnabled || this->Character == nullptr) && this->Variable[LEVELUP_INDEX].Value) {
			if (((int) AiHelpers.LearnableAbilities.size()) > this->Type->GetIndex()) {
				std::vector<const CUpgrade *> potential_abilities;
				for (size_t i = 0; i != AiHelpers.LearnableAbilities[this->Type->GetIndex()].size(); ++i) {
					if (this->CanLearnAbility(AiHelpers.LearnableAbilities[this->Type->GetIndex()][i])) {
						potential_abilities.push_back(AiHelpers.LearnableAbilities[this->Type->GetIndex()][i]);
					}
				}
				if (potential_abilities.size() > 0) {
					if (potential_abilities.size() == 1 || this->Player->AiEnabled) { //if can only acquire one particular ability, get it automatically
						const CUpgrade *chosen_ability = potential_abilities[SyncRand(potential_abilities.size())];
						AbilityAcquire(*this, chosen_ability);
						upgrade_found = true;
						if (this->Player == CPlayer::GetThisPlayer()) {
							this->Player->Notify(NotifyGreen, this->GetTilePos(), this->GetMapLayer()->GetIndex(), _("%s has acquired the %s ability!"), this->GetMessageName().c_str(), chosen_ability->GetName().utf8().get_data());
						}
					}
				}
			}
		}
	}
	
	this->Variable[LEVELUP_INDEX].Enable = 1;
	
	this->Player->UpdateLevelUpUnits();
}

void CUnit::Retrain()
{
	//lose all abilities (the AbilityLost function also returns the level-ups to the unit)
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		if (this->GetIndividualUpgrade(upgrade)) {
			if (upgrade->IsAbility() && std::find(this->Type->StartingAbilities.begin(), this->Type->StartingAbilities.end(), upgrade) == this->Type->StartingAbilities.end()) {
				AbilityLost(*this, upgrade, true);
			} else if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21) && this->Character && this->Character->Custom) { //allow changing the deity for custom heroes
				IndividualUpgradeLost(*this, upgrade, true);
			}
		}
	}
	
	std::string unit_name = GetMessageName();
	
	//now, revert the unit's type to the level 1 one
	while (this->Type->Stats[this->Player->GetIndex()].Variables[LEVEL_INDEX].Value > 1) {
		bool found_previous_unit_type = false;
		for (CUnitType *unit_type : CUnitType::GetAll()) {
			if (this->Character != nullptr && std::find(this->Character->ForbiddenUpgrades.begin(), this->Character->ForbiddenUpgrades.end(), unit_type) != this->Character->ForbiddenUpgrades.end()) {
				continue;
			}
			if (((int) AiHelpers.ExperienceUpgrades.size()) > unit_type->GetIndex()) {
				for (size_t j = 0; j != AiHelpers.ExperienceUpgrades[unit_type->GetIndex()].size(); ++j) {
					if (AiHelpers.ExperienceUpgrades[unit_type->GetIndex()][j] == this->Type) {
						this->Variable[LEVELUP_INDEX].Value += 1;
						this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
						this->Variable[LEVELUP_INDEX].Enable = 1;
						TransformUnitIntoType(*this, *unit_type);
						if (!IsNetworkGame() && Character != nullptr) {	//save the unit-type experience upgrade for persistent characters
							if (this->Character->UnitType != unit_type) {
								if (Player->AiEnabled == false) {
									this->Character->UnitType = unit_type;
									SaveHero(Character);
									CAchievement::CheckAchievements();
								}
							}
						}
						found_previous_unit_type = true;
						break;
					}
				}
			}
			if (found_previous_unit_type) {
				break;
			}
		}
		if (!found_previous_unit_type) {
			break;
		}
	}
	
	if (this->Player == CPlayer::GetThisPlayer()) {
		this->Player->Notify(NotifyGreen, this->GetTilePos(), this->GetMapLayer()->GetIndex(), _("%s's level-up choices have been reset."), unit_name.c_str());
	}
}

void CUnit::HealingItemAutoUse()
{
	if (!HasInventory()) {
		return;
	}
	
	CUnit *uins = this->UnitInside;
	
	for (int i = 0; i < this->InsideCount; ++i, uins = uins->NextContained) {
		if (!uins->GetType()->BoolFlag[ITEM_INDEX].value || uins->Elixir) {
			continue;
		}
		
		if (uins->GetType()->ItemClass->IsConsumable() == false) {
			continue;
		}
		
		if (uins->Variable[HITPOINTHEALING_INDEX].Value > 0) {
			if (
				uins->Variable[HITPOINTHEALING_INDEX].Value <= (this->GetModifiedVariable(HP_INDEX, VariableMax) - this->Variable[HP_INDEX].Value)
				|| (this->Variable[HP_INDEX].Value * 100 / this->GetModifiedVariable(HP_INDEX, VariableMax)) <= 20 // use a healing item if has less than 20% health
			) {
				if (!this->CriticalOrder) {
					this->CriticalOrder = COrder::NewActionUse(*uins);
				}
				break;
			}
		}
	}
}

void CUnit::SetCharacter(const std::string &character_ident, bool custom_hero)
{
	if (this->CurrentAction() == UnitActionDie) {
		return;
	}
	
	if (this->Character != nullptr) {
		this->Player->Heroes.erase(std::remove(this->Player->Heroes.begin(), this->Player->Heroes.end(), this), this->Player->Heroes.end());
		
		this->Variable[HERO_INDEX].Max = this->Variable[HERO_INDEX].Value = this->Variable[HERO_INDEX].Enable = 0;
	}
	
	CCharacter *character = nullptr;
	if (!custom_hero) {
		character = CCharacter::Get(character_ident);
	} else {
		character = GetCustomHero(character_ident);
	}
	
	if (character) {
		this->Character = character;
	} else {
		fprintf(stderr, "Character \"%s\" doesn't exist.\n", character_ident.c_str());
		return;
	}
		
	int old_mana_percent = 0;
	if (this->Variable[MANA_INDEX].Max > 0) {
		old_mana_percent = this->Variable[MANA_INDEX].Value * 100 / this->Variable[MANA_INDEX].Max;
	}
	
	this->Name = this->Character->GetName().utf8().get_data();
	this->ExtraName = this->Character->GetExtraName().utf8().get_data();
	this->FamilyName = this->Character->GetFamilyName().utf8().get_data();
	
	if (this->Character->UnitType != nullptr) {
		if (this->Character->UnitType != this->Type) { //set type to that of the character
			TransformUnitIntoType(*this, *this->Character->UnitType);
		}
		
		memcpy(Variable, this->Character->UnitType->Stats[this->Player->GetIndex()].Variables, UnitTypeVar.GetNumberVariable() * sizeof(*Variable));
	} else {
		fprintf(stderr, "Character \"%s\" has no unit type.\n", character_ident.c_str());
		return;
	}
	
	this->IndividualUpgrades.clear(); //reset the individual upgrades and then apply the character's
	this->Trait = nullptr;
	
	if (this->Type->GetCivilization() != nullptr && this->Type->GetCivilization()->GetUpgrade() != nullptr) {
		const CUpgrade *civilization_upgrade = this->Type->GetCivilization()->GetUpgrade();
		if (civilization_upgrade) {
			this->SetIndividualUpgrade(civilization_upgrade, 1);
		}
	}
	if (this->Type->GetCivilization() != nullptr && this->Type->GetFaction() != nullptr) {
		const CUpgrade *faction_upgrade = this->Type->GetFaction()->GetUpgrade();
		if (faction_upgrade != nullptr) {
			this->SetIndividualUpgrade(faction_upgrade, 1);
		}
	}

	if (this->Character->Trait != nullptr) { //set trait
		TraitAcquire(*this, this->Character->Trait);
	} else if (Editor.Running == EditorNotRunning && this->Type->Traits.size() > 0) {
		TraitAcquire(*this, this->Type->Traits[SyncRand(this->Type->Traits.size())]);
	}
	
	if (this->Character->Deity != nullptr && this->Character->Deity->CharacterUpgrade != nullptr) {
		IndividualUpgradeAcquire(*this, this->Character->Deity->CharacterUpgrade);
	}
	
	//load worshipped deities
	for (const CDeity *deity : this->Character->Deities) {
		const CUpgrade *deity_upgrade = deity->DeityUpgrade;
		if (deity_upgrade) {
			IndividualUpgradeAcquire(*this, deity_upgrade);
		}
	}
	
	for (const CUpgrade *ability_upgrade : this->Type->StartingAbilities) {
		if (CheckDependencies(ability_upgrade, this)) {
			IndividualUpgradeAcquire(*this, ability_upgrade);
		}
	}
	
	this->Variable[LEVEL_INDEX].Max = 100000; // because the code above sets the max level to the unit type stats' Level variable (which is the same as its value)
	if (this->Variable[LEVEL_INDEX].Value < this->Character->GetLevel()) {
		this->IncreaseLevel(this->Character->GetLevel() - this->Variable[LEVEL_INDEX].Value, false);
	}
	
	this->Variable[XP_INDEX].Enable = 1;
	this->Variable[XP_INDEX].Value = this->Variable[XPREQUIRED_INDEX].Value * this->Character->ExperiencePercent / 100;
	this->Variable[XP_INDEX].Max = this->Variable[XP_INDEX].Value;
	
	if (this->Variable[MANA_INDEX].Max > 0) {
		this->Variable[MANA_INDEX].Value = this->Variable[MANA_INDEX].Max * old_mana_percent / 100;
	}
			
	//load learned abilities
	std::vector<const CUpgrade *> abilities_to_remove;
	for (const CUpgrade *ability_upgrade : this->Character->Abilities) {
		if (this->CanLearnAbility(ability_upgrade)) {
			AbilityAcquire(*this, ability_upgrade, false);
		} else { //can't learn the ability? something changed in the game's code, remove it from persistent data and allow the hero to repick the ability
			abilities_to_remove.push_back(ability_upgrade);
		}
	}
	
	for (const CUpgrade *ability_upgrade : abilities_to_remove) {
		this->Character->Abilities.erase(std::remove(this->Character->Abilities.begin(), this->Character->Abilities.end(), ability_upgrade), this->Character->Abilities.end());
		SaveHero(this->Character);
	}
	
	//load read works
	for (const CUpgrade *work_upgrade : this->Character->ReadWorks) {
		this->ReadWork(work_upgrade, false);
	}
	
	//load consumed elixirs
	for (const CUpgrade *elixir_upgrade : this->Character->ConsumedElixirs) {
		this->ConsumeElixir(elixir_upgrade, false);
	}
	
	//load items
	for (const CPersistentItem *persistent_item : this->Character->Items) {
		CUnit *item = MakeUnitAndPlace(this->GetTilePos(), *persistent_item->Type, CPlayer::Players[PlayerNumNeutral], this->GetMapLayer()->GetIndex());
		if (persistent_item->Prefix != nullptr) {
			item->SetPrefix(persistent_item->Prefix);
		}
		if (persistent_item->Suffix != nullptr) {
			item->SetSuffix(persistent_item->Suffix);
		}
		if (persistent_item->Spell != nullptr) {
			item->SetSpell(persistent_item->Spell);
		}
		if (persistent_item->Work != nullptr) {
			item->SetWork(persistent_item->Work);
		}
		if (persistent_item->Elixir != nullptr) {
			item->SetElixir(persistent_item->Elixir);
		}
		item->Unique = persistent_item->Unique;
		if (!persistent_item->Name.empty()) {
			item->Name = persistent_item->Name.utf8().get_data();
		}
		item->Bound = persistent_item->Bound;
		item->Identified = persistent_item->Identified;
		item->Remove(this);
		if (this->Character->IsItemEquipped(persistent_item)) {
			this->EquipItem(item, false);
		}
	}
	
	if (this->Character != nullptr) {
		this->Player->Heroes.push_back(this);
	}

	this->Variable[HERO_INDEX].Max = this->Variable[HERO_INDEX].Value = this->Variable[HERO_INDEX].Enable = 1;
	
	this->ChooseVariation(); //choose a new variation now
	for (int i = 0; i < MaxImageLayers; ++i) {
		ChooseVariation(nullptr, i);
	}
	this->UpdateButtonIcons();
	this->UpdateXPRequired();
}

bool CUnit::CheckTerrainForVariation(const UnitTypeVariation *variation) const
{
	//if the variation has one or more terrain set as a precondition, then all tiles underneath the unit must match at least one of those terrains
	if (variation->Terrains.size() > 0) {
		if (!CMap::Map.Info.IsPointOnMap(this->GetTilePos(), this->MapLayer)) {
			return false;
		}
		bool terrain_check = true;
		for (int x = 0; x < this->Type->TileSize.x; ++x) {
			for (int y = 0; y < this->Type->TileSize.y; ++y) {
				if (CMap::Map.Info.IsPointOnMap(this->GetTilePos() + Vec2i(x, y), this->MapLayer)) {
					if (std::find(variation->Terrains.begin(), variation->Terrains.end(), CMap::Map.GetTileTopTerrain(this->GetTilePos() + Vec2i(x, y), false, this->GetMapLayer()->GetIndex(), true)) == variation->Terrains.end()) {
						terrain_check = false;
						break;
					}
				}
			}
			if (!terrain_check) {
				break;
			}
		}
		if (!terrain_check) {
			return false;
		}
	}
	
	//if the variation has one or more terrains set as a forbidden precondition, then no tiles underneath the unit may match one of those terrains
	if (variation->TerrainsForbidden.size() > 0) {
		if (!CMap::Map.Info.IsPointOnMap(this->GetTilePos(), this->MapLayer)) {
			return false;
		}
		bool terrain_check = true;
		for (int x = 0; x < this->Type->TileSize.x; ++x) {
			for (int y = 0; y < this->Type->TileSize.y; ++y) {
				if (CMap::Map.Info.IsPointOnMap(this->GetTilePos() + Vec2i(x, y), this->MapLayer)) {
					if (std::find(variation->TerrainsForbidden.begin(), variation->TerrainsForbidden.end(), CMap::Map.GetTileTopTerrain(this->GetTilePos() + Vec2i(x, y), false, this->GetMapLayer()->GetIndex(), true)) == variation->TerrainsForbidden.end()) {
						terrain_check = false;
						break;
					}
				}
			}
			if (!terrain_check) {
				break;
			}
		}
		if (terrain_check) {
			return false;
		}
	}
	
	return true;
}

bool CUnit::CheckSeasonForVariation(const UnitTypeVariation *variation) const
{
	if (
		!variation->Seasons.empty()
		&& (!this->MapLayer || std::find(variation->Seasons.begin(), variation->Seasons.end(), this->GetMapLayer()->GetSeason()) == variation->Seasons.end())
	) {
		return false;
	}
	
	if (
		!variation->ForbiddenSeasons.empty()
		&& this->MapLayer
		&& std::find(variation->ForbiddenSeasons.begin(), variation->ForbiddenSeasons.end(), this->GetMapLayer()->GetSeason()) != variation->ForbiddenSeasons.end()
	) {
		return false;
	}
	
	return true;
}

void CUnit::ChooseVariation(const CUnitType *new_type, int image_layer)
{
	String old_variation_ident;
	const CHairColor *hair_color = nullptr;
	if (image_layer == -1) {
		if (this->Character != nullptr && this->Character->GetHairColor() != nullptr) {
			hair_color = this->Character->GetHairColor();
		} else if (this->GetVariation() != nullptr) {
			hair_color = this->GetVariation()->GetHairColor();
		}
		
		if (this->GetVariation() != nullptr) {
			old_variation_ident = this->GetVariation()->GetIdent();
		}
	} else {
		if (image_layer == HairImageLayer && this->Character != nullptr && this->Character->GetHairColor() != nullptr) {
			hair_color = this->Character->GetHairColor();
		} else if (this->GetLayerVariation(image_layer) != nullptr) {
			hair_color = this->GetLayerVariation(image_layer)->GetHairColor();
		}
		
		if (this->GetLayerVariation(image_layer) != nullptr) {
			old_variation_ident = this->GetLayerVariation(image_layer)->GetIdent();
		}
	}
	
	std::vector<UnitTypeVariation *> type_variations;
	const std::vector<UnitTypeVariation *> &variation_list = image_layer == -1 ? (new_type != nullptr ? new_type->Variations : this->Type->Variations) : (new_type != nullptr ? new_type->LayerVariations[image_layer] : this->Type->LayerVariations[image_layer]);
	
	bool found_similar_ident = false;
	bool found_same_hair_color = false;
	for (UnitTypeVariation *variation : variation_list) {
		if (!CheckDependencies(variation, this)) {
			continue;
		}
		
		if (variation->ResourceMin && this->GetResourcesHeld() < variation->ResourceMin) {
			continue;
		}
		if (variation->ResourceMax && this->GetResourcesHeld() > variation->ResourceMax) {
			continue;
		}
		
		if (!this->CheckSeasonForVariation(variation)) {
			continue;
		}
		
		if (!this->CheckTerrainForVariation(variation)) {
			continue;
		}
		
		bool upgrades_check = true;
		bool requires_weapon = false;
		bool found_weapon = false;
		bool requires_shield = false;
		bool found_shield = false;
		for (const CUpgrade *required_upgrade : variation->UpgradesRequired) {
			if (required_upgrade->GetItemSlot() != nullptr && required_upgrade->GetItemSlot()->IsWeapon()) {
				requires_weapon = true;
				if (UpgradeIdentAllowed(*this->Player, required_upgrade->Ident.c_str()) == 'R' || this->GetIndividualUpgrade(required_upgrade)) {
					found_weapon = true;
				}
			} else if (required_upgrade->GetItemSlot() != nullptr && required_upgrade->GetItemSlot()->IsShield()) {
				requires_shield = true;
				if (UpgradeIdentAllowed(*this->Player, required_upgrade->Ident.c_str()) == 'R' || this->GetIndividualUpgrade(required_upgrade)) {
					found_shield = true;
				}
			} else if (UpgradeIdentAllowed(*this->Player, required_upgrade->Ident.c_str()) != 'R' && this->GetIndividualUpgrade(required_upgrade) == false) {
				upgrades_check = false;
				break;
			}
		}
		
		if (upgrades_check) {
			for (const CUpgrade *forbidden_upgrade : variation->UpgradesForbidden) {
				if (UpgradeIdentAllowed(*this->Player, forbidden_upgrade->Ident.c_str()) == 'R' || this->GetIndividualUpgrade(forbidden_upgrade)) {
					upgrades_check = false;
					break;
				}
			}
		}
		
		for (const ItemClass *item_class : variation->ItemClassesNotEquipped) {
			if (this->IsItemClassEquipped(item_class)) {
				upgrades_check = false;
				break;
			}
		}
		for (size_t j = 0; j < variation->ItemsNotEquipped.size(); ++j) {
			if (this->IsItemTypeEquipped(variation->ItemsNotEquipped[j])) {
				upgrades_check = false;
				break;
			}
		}
		if (upgrades_check == false) {
			continue;
		}
		for (const ItemClass *item_class : variation->ItemClassesEquipped) {
			if (item_class->GetSlot()->IsWeapon()) {
				requires_weapon = true;
				if (this->IsItemClassEquipped(item_class)) {
					found_weapon = true;
				}
			} else if (item_class->GetSlot()->IsShield()) {
				requires_shield = true;
				if (this->IsItemClassEquipped(item_class)) {
					found_shield = true;
				}
			}
		}
		for (size_t j = 0; j < variation->ItemsEquipped.size(); ++j) {
			if (variation->ItemsEquipped[j]->ItemClass->GetSlot()->IsWeapon()) {
				requires_weapon = true;
				if (this->IsItemTypeEquipped(variation->ItemsEquipped[j])) {
					found_weapon = true;
				}
			} else if (variation->ItemsEquipped[j]->ItemClass->GetSlot()->IsShield()) {
				requires_shield = true;
				if (this->IsItemTypeEquipped(variation->ItemsEquipped[j])) {
					found_shield = true;
				}
			}
		}
		if ((requires_weapon && !found_weapon) || (requires_shield && !found_shield)) {
			continue;
		}
		
		if (hair_color != nullptr) {
			//give priority to variations with the same hair color as the current one
			if (hair_color == variation->GetHairColor()) {
				if (!found_same_hair_color) {
					found_same_hair_color = true;
					type_variations.clear();
					found_similar_ident = false;
				}
			} else {
				if (found_same_hair_color) {
					continue;
				}
			}
		}
		
		if (!old_variation_ident.empty() && (variation->GetIdent().find(old_variation_ident) != -1 || old_variation_ident.find(variation->GetIdent()) != -1)) { // if the old variation's ident is included in that of a new viable variation (or vice-versa), give priority to the new variation over others
			if (!found_similar_ident) {
				found_similar_ident = true;
				type_variations.clear();
			}
		} else {
			if (found_similar_ident) {
				continue;
			}
		}
	
		for (int j = 0; j < variation->Weight; ++j) {
			type_variations.push_back(variation);
		}
	}
	if (type_variations.size() > 0) {
		this->SetVariation(type_variations[SyncRand(type_variations.size())], new_type, image_layer);
	} else {
		this->SetVariation(nullptr, new_type, image_layer);
	}
}

void CUnit::SetVariation(const UnitTypeVariation *new_variation, const CUnitType *new_type, int image_layer)
{
	if (image_layer == -1) {
		const PaletteImage *old_image = this->GetImage();
		
		if (
			(this->GetVariation() && this->GetVariation()->Animations)
			|| (new_variation && new_variation->Animations)
		) { //if the old (if any) or the new variation has specific animations, set the unit's frame to its type's still frame
			this->SetFrame(this->Type->StillFrame);
		}
		
		this->Variation = new_variation;
		
		//emit a signal if the image has changed
		const PaletteImage *new_image = this->GetImage();
		if (old_image != new_image) {
			this->emit_signal("image_changed", new_image);
		}
	} else {
		this->LayerVariation[image_layer] = new_variation ? new_variation->GetIndex() : -1;
	}
}

const UnitTypeVariation *CUnit::GetLayerVariation(const unsigned int image_layer) const
{
	if (this->LayerVariation[image_layer] >= 0 && this->LayerVariation[image_layer] < (int) this->Type->LayerVariations[image_layer].size()) {
		return this->Type->LayerVariations[image_layer][this->LayerVariation[image_layer]];
	}
	
	return nullptr;
}

void CUnit::UpdateButtonIcons()
{
	this->ChooseButtonIcon(ButtonAttack);
	this->ChooseButtonIcon(ButtonStop);
	this->ChooseButtonIcon(ButtonMove);
	this->ChooseButtonIcon(ButtonStandGround);
	this->ChooseButtonIcon(ButtonPatrol);
	if (this->Type->BoolFlag[HARVESTER_INDEX].value) {
		this->ChooseButtonIcon(ButtonReturn);
	}
}

void CUnit::ChooseButtonIcon(const int button_action)
{
	//get button icons from the equipment
	const CUnit *button_unit = nullptr;
	for (std::map<const ItemSlot *, std::vector<CUnit *>>::iterator iterator = this->EquippedItems.begin(); iterator != this->EquippedItems.end(); ++iterator) {
		const ItemSlot *item_slot = iterator->first;
		
		if (button_action == ButtonAttack || button_action == ButtonStandGround) {
			if (!item_slot->IsArrows() && !item_slot->IsWeapon()) {
				continue;
			}
		} else if (button_action == ButtonStop) {
			if (!item_slot->IsShield()) {
				continue;
			}
		} else if (button_action == ButtonMove) {
			if (!item_slot->IsBoots()) {
				continue;
			}
		} else {
			continue;
		}
			
		for (const CUnit *equipment_unit : iterator->second) {
			if (button_action == ButtonAttack || button_action == ButtonStandGround) {
				if (item_slot->IsWeapon() && equipment_unit->GetType()->ItemClass->AllowsArrows()) { //use the arrow icon for attack/stand ground buttons instead if the weapon allows arrows
					continue;
				}
			} else if (button_action == ButtonStop) {
				if (!equipment_unit->GetType()->ItemClass->IsShield()) {
					continue;
				}
			}
			
			if (button_action == ButtonStandGround) {
				if (equipment_unit->GetType()->ButtonIcons.find(button_action) == equipment_unit->GetType()->ButtonIcons.end()) {
					continue;
				}
			} else {
				if (equipment_unit->GetIcon() == nullptr) {
					continue;
				}
			}
			
			button_unit = equipment_unit;
			break;
		}
		
		if (button_unit != nullptr) {
			break;
		}
	}
	if (button_unit != nullptr) {
		if (button_action == ButtonStandGround) {
			this->ButtonIcons[button_action] = button_unit->GetType()->ButtonIcons.find(button_action)->second.Icon;
		} else {
			this->ButtonIcons[button_action] = button_unit->GetIcon();
		}
		return;
	}
	
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && variation->ButtonIcons.find(button_action) != variation->ButtonIcons.end()) {
		this->ButtonIcons[button_action] = variation->ButtonIcons.find(button_action)->second.Icon;
		return;
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const UnitTypeVariation *layer_variation = this->GetLayerVariation(i);
		if (layer_variation && layer_variation->ButtonIcons.find(button_action) != layer_variation->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = layer_variation->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
	}

	for (int i = (CUpgradeModifier::UpgradeModifiers.size() - 1); i >= 0; --i) {
		const CUpgradeModifier *modifier = CUpgradeModifier::UpgradeModifiers[i];
		const CUpgrade *upgrade = CUpgrade::Get(modifier->UpgradeId);
		if (this->Player->Allow.Upgrades[upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType()) && upgrade->GetItemSlot() != nullptr) {
			if (
				(
					(button_action == ButtonAttack && ((upgrade->GetItemSlot()->IsWeapon() && upgrade->GetItem()->ItemClass->AllowsArrows() == false) || upgrade->GetItemSlot()->IsArrows()))
					|| (button_action == ButtonStop && upgrade->GetItemSlot()->IsShield())
					|| (button_action == ButtonMove && upgrade->GetItemSlot()->IsBoots())
				)
				&& upgrade->GetItem()->GetIcon() != nullptr
			) {
				this->ButtonIcons[button_action] = upgrade->GetItem()->GetIcon();
				return;
			} else if (button_action == ButtonStandGround && (upgrade->GetItemSlot()->IsWeapon() || upgrade->GetItemSlot()->IsArrows()) && upgrade->GetItem()->ButtonIcons.find(button_action) != upgrade->GetItem()->ButtonIcons.end()) {
				this->ButtonIcons[button_action] = upgrade->GetItem()->ButtonIcons.find(button_action)->second.Icon;
				return;
			}
		}
	}
	
	//get button icons from the default equipment
	const CUnitType *button_unit_type = nullptr;
	for (std::map<const ItemSlot *, CUnitType *>::const_iterator iterator = this->Type->DefaultEquipment.begin(); iterator != this->Type->DefaultEquipment.end(); ++iterator) {
		const ItemSlot *item_slot = iterator->first;
		const CUnitType *equipment_unit_type = iterator->second;
		
		if (button_action == ButtonAttack || button_action == ButtonStandGround) {
			if (!item_slot->IsArrows() && !item_slot->IsWeapon()) {
				continue;
			}
			if (item_slot->IsWeapon() && equipment_unit_type->ItemClass->AllowsArrows()) { //use the arrow icon for attack/stand ground buttons instead if the weapon allows arrows
				continue;
			}
		} else if (button_action == ButtonStop) {
			if (!item_slot->IsShield() || !equipment_unit_type->ItemClass->IsShield()) {
				continue;
			}
		} else if (button_action == ButtonMove) {
			if (!item_slot->IsBoots()) {
				continue;
			}
		} else {
			continue;
		}
		
		if (button_action == ButtonStandGround) {
			if (equipment_unit_type->ButtonIcons.find(button_action) == equipment_unit_type->ButtonIcons.end()) {
				continue;
			}
		} else {
			if (equipment_unit_type->GetIcon() == nullptr) {
				continue;
			}
		}
		
		button_unit_type = equipment_unit_type;
		
		break;
	}
	if (button_unit_type != nullptr) {
		if (button_action == ButtonStandGround) {
			this->ButtonIcons[button_action] = button_unit_type->ButtonIcons.find(button_action)->second.Icon;
		} else {
			this->ButtonIcons[button_action] = button_unit_type->GetIcon();
		}
		return;
	}
	
	if (this->Type->ButtonIcons.find(button_action) != this->Type->ButtonIcons.end()) {
		this->ButtonIcons[button_action] = this->Type->ButtonIcons.find(button_action)->second.Icon;
		return;
	}
	
	if (this->Type->GetCivilization() != nullptr) {
		CCivilization *civilization = this->Type->GetCivilization();
		const CFaction *faction = this->Type->GetFaction();
		
		if (faction == nullptr && this->Player->Race == civilization->GetIndex()) {
			faction = this->Player->GetFaction();
		}
		
		if (faction != nullptr && faction->ButtonIcons.find(button_action) != faction->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = faction->ButtonIcons.find(button_action)->second.Icon;
			return;
		} else if (civilization->ButtonIcons.find(button_action) != civilization->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = civilization->ButtonIcons[button_action].Icon;
			return;
		}
	}
	
	if (this->ButtonIcons.find(button_action) != this->ButtonIcons.end()) { //if no proper button icon found, make sure any old button icon set for this button action isn't used either
		this->ButtonIcons.erase(button_action);
	}
}

void CUnit::EquipItem(CUnit *item, const bool affect_character)
{
	const ItemClass *item_class = item->Type->ItemClass;
	const ItemSlot *item_slot = item_class->GetSlot();
	
	if (item_slot == nullptr) {
		fprintf(stderr, "Trying to equip item of type \"%s\", which has no item slot.\n", item->GetTypeName().c_str());
		return;
	}
	
	if (this->GetItemSlotQuantity(item_slot) > 0 && this->EquippedItems[item_slot].size() == this->GetItemSlotQuantity(item_slot)) {
		this->DeequipItem(this->EquippedItems[item_slot][this->EquippedItems[item_slot].size() - 1]);
	}
	
	if (this->EquippedItems[item_slot].size() == 0) {
		if (item_slot->IsWeapon()) {
			// remove the upgrade modifiers from weapon technologies or from abilities which require the base weapon class but aren't compatible with this weapon's class; and apply upgrade modifiers from abilities which require this weapon's class
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (
					(modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsWeapon() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType()))
					|| (
						modifier_upgrade->IsAbility()
						&& this->GetIndividualUpgrade(modifier_upgrade)
						&& modifier_upgrade->WeaponClasses.size() > 0
						&& std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) != modifier_upgrade->WeaponClasses.end()
						&& std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) == modifier_upgrade->WeaponClasses.end()
					)
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							RemoveIndividualUpgradeModifier(*this, modifier);
						}
					} else {
						RemoveIndividualUpgradeModifier(*this, modifier);
					}
				} else if (
					modifier_upgrade->IsAbility() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) == modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) != modifier_upgrade->WeaponClasses.end()
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							ApplyIndividualUpgradeModifier(*this, modifier);
						}
					} else {
						ApplyIndividualUpgradeModifier(*this, modifier);
					}
				}
			}
		} else if (item_slot->IsShield()) {
			// remove the upgrade modifiers from shield technologies
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsShield() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) {
					RemoveIndividualUpgradeModifier(*this, modifier);
				}
			}
		} else if (item_slot->IsBoots()) {
			// remove the upgrade modifiers from boots technologies
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsBoots() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) {
					RemoveIndividualUpgradeModifier(*this, modifier);
				}
			}
		} else if (item_slot->IsArrows()) {
			// remove the upgrade modifiers from arrows technologies
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsArrows() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) {
					RemoveIndividualUpgradeModifier(*this, modifier);
				}
			}
		}
	}
	
	if (item->Unique && item->Unique->Set && this->EquippingItemCompletesSet(item)) {
		for (const CUpgradeModifier *modifier : item->Unique->Set->UpgradeModifiers) {
			ApplyIndividualUpgradeModifier(*this, modifier);
		}
	}

	if (!IsNetworkGame() && this->Character && this->Player->AiEnabled == false && affect_character) {
		if (this->Character->GetItem(item) != nullptr) {
			if (!this->Character->IsItemEquipped(this->Character->GetItem(item))) {
				this->Character->EquippedItems[item_slot].push_back(this->Character->GetItem(item));
				SaveHero(this->Character);
			} else {
				fprintf(stderr, "Item is not equipped by character \"%s\"'s unit, but is equipped by the character itself.\n", Character->Ident.c_str());
			}
		} else {
			fprintf(stderr, "Item is present in the inventory of the character \"%s\"'s unit, but not in the character's inventory itself.\n", Character->Ident.c_str());
		}
	}
	this->EquippedItems[item_slot].push_back(item);
	
	//change variation, if the current one has become forbidden
	const UnitTypeVariation *variation = this->GetVariation();
	if (
		variation
		&& (
			std::find(variation->ItemClassesNotEquipped.begin(), variation->ItemClassesNotEquipped.end(), item->Type->ItemClass) != variation->ItemClassesNotEquipped.end()
			|| std::find(variation->ItemsNotEquipped.begin(), variation->ItemsNotEquipped.end(), item->Type) != variation->ItemsNotEquipped.end()
		)
	) {
		this->ChooseVariation(); //choose a new variation now
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const UnitTypeVariation *layer_variation = this->GetLayerVariation(i);
		if (
			layer_variation
			&& (
				std::find(layer_variation->ItemClassesNotEquipped.begin(), layer_variation->ItemClassesNotEquipped.end(), item->Type->ItemClass) != layer_variation->ItemClassesNotEquipped.end()
				|| std::find(layer_variation->ItemsNotEquipped.begin(), layer_variation->ItemsNotEquipped.end(), item->Type) != layer_variation->ItemsNotEquipped.end()
			)
		) {
			this->ChooseVariation(nullptr, i);
		}
	}
	
	if (item_slot->IsWeapon() || item_slot->IsArrows()) {
		this->ChooseButtonIcon(ButtonAttack);
		this->ChooseButtonIcon(ButtonStandGround);
	} else if (item_slot->IsShield()) {
		this->ChooseButtonIcon(ButtonStop);
	} else if (item_slot->IsBoots()) {
		this->ChooseButtonIcon(ButtonMove);
	}
	this->ChooseButtonIcon(ButtonPatrol);
	
	//add item bonuses
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (
			i == BASICDAMAGE_INDEX || i == PIERCINGDAMAGE_INDEX || i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX
			|| i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX || i == ACIDDAMAGE_INDEX
			|| i == ARMOR_INDEX || i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX
			|| i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX || i == ACIDRESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX || i == SPEED_INDEX || i == CHARGEBONUS_INDEX || i == BACKSTAB_INDEX
			|| i == ATTACKRANGE_INDEX
		) {
			Variable[i].Value += item->Variable[i].Value;
			Variable[i].Max += item->Variable[i].Max;
		} else if (i == HITPOINTBONUS_INDEX) {
			Variable[HP_INDEX].Value += item->Variable[i].Value;
			Variable[HP_INDEX].Max += item->Variable[i].Max;
			Variable[HP_INDEX].Increase += item->Variable[i].Increase;
		} else if (i == SIGHTRANGE_INDEX || i == DAYSIGHTRANGEBONUS_INDEX || i == NIGHTSIGHTRANGEBONUS_INDEX) {
			if (!SaveGameLoading) {
				MapUnmarkUnitSight(*this);
			}
			Variable[i].Value += item->Variable[i].Value;
			Variable[i].Max += item->Variable[i].Max;
			if (!SaveGameLoading) {
				if (i == SIGHTRANGE_INDEX) {
					CurrentSightRange = Variable[i].Value;
				}
				UpdateUnitSightRange(*this);
				MapMarkUnitSight(*this);
			}
		}
	}
}

void CUnit::DeequipItem(CUnit *item, const bool affect_character)
{
	//remove item bonuses
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (
			i == BASICDAMAGE_INDEX || i == PIERCINGDAMAGE_INDEX || i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX
			|| i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX || i == ACIDDAMAGE_INDEX
			|| i == ARMOR_INDEX || i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX
			|| i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX || i == ACIDRESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX || i == SPEED_INDEX || i == CHARGEBONUS_INDEX || i == BACKSTAB_INDEX
			|| i == ATTACKRANGE_INDEX
		) {
			Variable[i].Value -= item->Variable[i].Value;
			Variable[i].Max -= item->Variable[i].Max;
		} else if (i == HITPOINTBONUS_INDEX) {
			Variable[HP_INDEX].Value -= item->Variable[i].Value;
			Variable[HP_INDEX].Max -= item->Variable[i].Max;
			Variable[HP_INDEX].Increase -= item->Variable[i].Increase;
		} else if (i == SIGHTRANGE_INDEX || i == DAYSIGHTRANGEBONUS_INDEX || i == NIGHTSIGHTRANGEBONUS_INDEX) {
			MapUnmarkUnitSight(*this);
			Variable[i].Value -= item->Variable[i].Value;
			Variable[i].Max -= item->Variable[i].Max;
			if (i == SIGHTRANGE_INDEX) {
				CurrentSightRange = Variable[i].Value;
			}
			UpdateUnitSightRange(*this);
			MapMarkUnitSight(*this);
		}
	}
	
	if (item->Unique && item->Unique->Set && this->DeequippingItemBreaksSet(item)) {
		for (const CUpgradeModifier *modifier : item->Unique->Set->UpgradeModifiers) {
			RemoveIndividualUpgradeModifier(*this, modifier);
		}
	}

	const ItemClass *item_class = item->Type->ItemClass;
	const ItemSlot *item_slot = item_class->GetSlot();
	
	if (item_slot == nullptr) {
		fprintf(stderr, "Trying to de-equip item of type \"%s\", which has no item slot.\n", item->GetTypeName().c_str());
		return;
	}
	
	if (!IsNetworkGame() && Character && this->Player->AiEnabled == false && affect_character) {
		if (this->Character->GetItem(item) != nullptr) {
			if (this->Character->IsItemEquipped(this->Character->GetItem(item))) {
				this->Character->EquippedItems[item_slot].erase(std::remove(this->Character->EquippedItems[item_slot].begin(), this->Character->EquippedItems[item_slot].end(), this->Character->GetItem(item)), this->Character->EquippedItems[item_slot].end());
				
				if (this->Character->EquippedItems[item_slot].empty()) {
					this->Character->EquippedItems.erase(item_slot);
				}
				
				SaveHero(this->Character);
			} else {
				fprintf(stderr, "Item is equipped by character \"%s\"'s unit, but not by the character itself.\n", Character->Ident.c_str());
			}
		} else {
			fprintf(stderr, "Item is present in the inventory of the character \"%s\"'s unit, but not in the character's inventory itself.\n", Character->Ident.c_str());
		}
	}
	
	this->EquippedItems[item_slot].erase(std::remove(this->EquippedItems[item_slot].begin(), this->EquippedItems[item_slot].end(), item), this->EquippedItems[item_slot].end());
	
	if (this->EquippedItems[item_slot].empty()) {
		this->EquippedItems.erase(item_slot);
		
		if (item_slot->IsWeapon()) {
			// restore the upgrade modifiers from weapon technologies, and apply ability effects that are weapon class-specific accordingly
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (
					(modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsWeapon() && Player->Allow.Upgrades[modifier->UpgradeId] == 'R' && modifier->AppliesToUnitType(this->GetType()))
					|| (modifier_upgrade->IsAbility() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) != modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) == modifier_upgrade->WeaponClasses.end())
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							ApplyIndividualUpgradeModifier(*this, modifier);
						}
					} else {
						ApplyIndividualUpgradeModifier(*this, modifier);
					}
				} else if (
					modifier_upgrade->IsAbility() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->Type->WeaponClasses[0]) == modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item_class) != modifier_upgrade->WeaponClasses.end()
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							RemoveIndividualUpgradeModifier(*this, modifier);
						}
					} else {
						RemoveIndividualUpgradeModifier(*this, modifier);
					}
				}
			}
		} else if (item_slot->IsShield()) {
			// restore the upgrade modifiers from shield technologies
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsShield() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) {
					ApplyIndividualUpgradeModifier(*this, modifier);
				}
			}
		} else if (item_slot->IsBoots()) {
			// restore the upgrade modifiers from boots technologies
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsBoots() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) {
					ApplyIndividualUpgradeModifier(*this, modifier);
				}
			}
		} else if (item_slot->IsArrows()) {
			// restore the upgrade modifiers from arrows technologies
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (modifier_upgrade->GetItemSlot() != nullptr && modifier_upgrade->GetItemSlot()->IsArrows() && Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) {
					ApplyIndividualUpgradeModifier(*this, modifier);
				}
			}
		}
	}
	
	//change variation, if the current one has become forbidden
	const UnitTypeVariation *variation = this->GetVariation();
	if (
		variation
		&& (
			std::find(variation->ItemClassesEquipped.begin(), variation->ItemClassesEquipped.end(), item->Type->ItemClass) != variation->ItemClassesEquipped.end() 
			|| std::find(variation->ItemsEquipped.begin(), variation->ItemsEquipped.end(), item->Type) != variation->ItemsEquipped.end()
		)
	) {
		this->ChooseVariation(); //choose a new variation now
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const UnitTypeVariation *layer_variation = this->GetLayerVariation(i);

		if (
			layer_variation
			&& (
				std::find(layer_variation->ItemClassesEquipped.begin(), layer_variation->ItemClassesEquipped.end(), item->Type->ItemClass) != layer_variation->ItemClassesEquipped.end()
				|| std::find(layer_variation->ItemsEquipped.begin(), layer_variation->ItemsEquipped.end(), item->Type) != layer_variation->ItemsEquipped.end()
			)
		) {
			this->ChooseVariation(nullptr, i);
		}
	}
	
	if (item_slot->IsWeapon() || item_slot->IsArrows()) {
		this->ChooseButtonIcon(ButtonAttack);
		this->ChooseButtonIcon(ButtonStandGround);
	} else if (item_slot->IsShield()) {
		this->ChooseButtonIcon(ButtonStop);
	} else if (item_slot->IsBoots()) {
		this->ChooseButtonIcon(ButtonMove);
	}
	this->ChooseButtonIcon(ButtonPatrol);
}

/**
**	@brief	Read a literary work.
**
**	@param	work				The literary work.
**	@param	affect_character	Whether the reading of the work should be saved persistently for the unit's character, if any.
*/
void CUnit::ReadWork(const CUpgrade *work, const bool affect_character)
{
	IndividualUpgradeAcquire(*this, work);
	
	if (!IsNetworkGame() && this->Character && this->Player->AiEnabled == false && affect_character) {
		if (std::find(this->Character->ReadWorks.begin(), this->Character->ReadWorks.end(), work) == this->Character->ReadWorks.end()) {
			this->Character->ReadWorks.push_back(work);
			SaveHero(Character);
		}
	}
}

/**
**	@brief	Consume an elixir.
**
**	@param	elixir				The elixir.
**	@param	affect_character	Whether the consuming of the elixir should be saved persistently for the unit's character, if any.
*/
void CUnit::ConsumeElixir(const CUpgrade *elixir, const bool affect_character)
{
	IndividualUpgradeAcquire(*this, elixir);
	
	if (!IsNetworkGame() && this->Character && this->Player->AiEnabled == false && affect_character) {
		if (std::find(this->Character->ConsumedElixirs.begin(), this->Character->ConsumedElixirs.end(), elixir) == this->Character->ConsumedElixirs.end()) {
			this->Character->ConsumedElixirs.push_back(elixir);
			SaveHero(Character);
		}
	}
}

void CUnit::ApplyAura(int aura_index)
{
	if (aura_index == LEADERSHIPAURA_INDEX) {
		if (!this->IsInCombat()) {
			return;
		}
	}
	
	this->ApplyAuraEffect(aura_index);
			
	//apply aura to all appropriate nearby units
	int aura_range = AuraRange - (this->Type->TileSize.x - 1);
	std::vector<CUnit *> table;
	SelectAroundUnit(*this, aura_range, table, MakeOrPredicate(HasSamePlayerAs(*this->Player), IsAlliedWith(*this->Player)), true);
	for (size_t i = 0; i != table.size(); ++i) {
		table[i]->ApplyAuraEffect(aura_index);
	}
	
	table.clear();
	SelectAroundUnit(*this, aura_range, table, MakeOrPredicate(MakeOrPredicate(HasSamePlayerAs(*this->Player), IsAlliedWith(*this->Player)), HasSamePlayerAs(*CPlayer::Players[PlayerNumNeutral])), true);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->UnitInside) {
			CUnit *uins = table[i]->UnitInside;
			for (int j = 0; j < table[i]->InsideCount; ++j, uins = uins->NextContained) {
				if (uins->GetPlayer() == this->GetPlayer() || uins->IsAllied(*this->GetPlayer())) {
					uins->ApplyAuraEffect(aura_index);
				}
			}
		}
	}
}

void CUnit::ApplyAuraEffect(int aura_index)
{
	int effect_index = -1;
	if (aura_index == LEADERSHIPAURA_INDEX) {
		if (this->Type->BoolFlag[BUILDING_INDEX].value) {
			return;
		}
		effect_index = LEADERSHIP_INDEX;
	} else if (aura_index == REGENERATIONAURA_INDEX) {
		if (!this->Type->BoolFlag[ORGANIC_INDEX].value || this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableMax)) {
			return;
		}
		effect_index = REGENERATION_INDEX;
	} else if (aura_index == HYDRATINGAURA_INDEX) {
		if (!this->Type->BoolFlag[ORGANIC_INDEX].value) {
			return;
		}
		effect_index = HYDRATING_INDEX;
		this->Variable[DEHYDRATION_INDEX].Max = 0;
		this->Variable[DEHYDRATION_INDEX].Value = 0;
	}
	
	if (effect_index == -1) {
		return;
	}
	
	this->Variable[effect_index].Enable = 1;
	this->Variable[effect_index].Max = std::max(CYCLES_PER_SECOND + 1, this->Variable[effect_index].Max);
	this->Variable[effect_index].Value = std::max(CYCLES_PER_SECOND + 1, this->Variable[effect_index].Value);
}

void CUnit::SetPrefix(const CUpgrade *prefix)
{
	if (this->Prefix != nullptr) {
		for (size_t z = 0; z < Prefix->UpgradeModifiers.size(); ++z) {
			RemoveIndividualUpgradeModifier(*this, Prefix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Prefix->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Prefix->GetMagicLevel();
	}
	if (!IsNetworkGame() && Container && Container->Character && Container->GetPlayer()->AiEnabled == false && Container->Character->GetItem(this) != nullptr && Container->Character->GetItem(this)->Prefix != prefix) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(this)->Prefix = prefix;
		SaveHero(Container->Character);
	}
	this->Prefix = prefix;
	if (this->Prefix != nullptr) {
		for (size_t z = 0; z < Prefix->UpgradeModifiers.size(); ++z) {
			ApplyIndividualUpgradeModifier(*this, Prefix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value += this->Prefix->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max += this->Prefix->GetMagicLevel();
	}
	
	this->UpdateItemName();
}

void CUnit::SetSuffix(const CUpgrade *suffix)
{
	if (this->Suffix != nullptr) {
		for (size_t z = 0; z < Suffix->UpgradeModifiers.size(); ++z) {
			RemoveIndividualUpgradeModifier(*this, Suffix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Suffix->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Suffix->GetMagicLevel();
	}
	if (!IsNetworkGame() && Container && Container->Character && Container->GetPlayer()->AiEnabled == false && Container->Character->GetItem(this) != nullptr && Container->Character->GetItem(this)->Suffix != suffix) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(this)->Suffix = suffix;
		SaveHero(Container->Character);
	}
	this->Suffix = suffix;
	if (Suffix != nullptr) {
		for (size_t z = 0; z < Suffix->UpgradeModifiers.size(); ++z) {
			ApplyIndividualUpgradeModifier(*this, Suffix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value += this->Suffix->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max += this->Suffix->GetMagicLevel();
	}
	
	this->UpdateItemName();
}

void CUnit::SetSpell(const CSpell *spell)
{
	if (!IsNetworkGame() && Container && Container->Character && Container->GetPlayer()->AiEnabled == false && Container->Character->GetItem(this) != nullptr && Container->Character->GetItem(this)->Spell != spell) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(this)->Spell = spell;
		SaveHero(Container->Character);
	}
	this->Spell = spell;
	
	this->UpdateItemName();
}

void CUnit::SetWork(const CUpgrade *work)
{
	if (this->Work != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Work->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Work->GetMagicLevel();
	}
	
	if (!IsNetworkGame() && Container && Container->Character && Container->GetPlayer()->AiEnabled == false && Container->Character->GetItem(this) != nullptr && Container->Character->GetItem(this)->Work != work) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(this)->Work = work;
		SaveHero(Container->Character);
	}
	
	this->Work = work;
	
	if (this->Work != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value += this->Work->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max += this->Work->GetMagicLevel();
	}
	
	this->UpdateItemName();
}

void CUnit::SetElixir(const CUpgrade *elixir)
{
	if (this->Elixir != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Elixir->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Elixir->GetMagicLevel();
	}
	
	if (!IsNetworkGame() && Container && Container->Character && Container->GetPlayer()->AiEnabled == false && Container->Character->GetItem(this) != nullptr && Container->Character->GetItem(this)->Elixir != elixir) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(this)->Elixir = elixir;
		SaveHero(Container->Character);
	}
	
	this->Elixir = elixir;
	
	if (this->Elixir != nullptr) {
		this->Variable[MAGICLEVEL_INDEX].Value += this->Elixir->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max += this->Elixir->GetMagicLevel();
	}
	
	this->UpdateItemName();
}

void CUnit::SetUnique(UniqueItem *unique)
{
	if (this->Unique && this->Unique->Set) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Unique->Set->GetMagicLevel();
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Unique->Set->GetMagicLevel();
	}
		
	if (unique != nullptr) {
		this->SetPrefix(unique->Prefix);
		this->SetSuffix(unique->Suffix);
		this->SetSpell(unique->Spell);
		this->SetWork(unique->Work);
		this->SetElixir(unique->Elixir);
		if (unique->ResourcesHeld != 0) {
			this->SetResourcesHeld(unique->ResourcesHeld);
			this->Variable[GIVERESOURCE_INDEX].Value = unique->ResourcesHeld;
			this->Variable[GIVERESOURCE_INDEX].Max = unique->ResourcesHeld;
			this->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
		if (unique->Set) {
			this->Variable[MAGICLEVEL_INDEX].Value += unique->Set->GetMagicLevel();
			this->Variable[MAGICLEVEL_INDEX].Max += unique->Set->GetMagicLevel();
		}
		this->Name = unique->GetName().utf8().get_data();
		this->Unique = unique;
	} else {
		this->Name.clear();
		this->Unique = nullptr;
		this->SetPrefix(nullptr);
		this->SetSuffix(nullptr);
		this->SetSpell(nullptr);
		this->SetWork(nullptr);
		this->SetElixir(nullptr);
	}
}

void CUnit::Identify()
{
	if (!IsNetworkGame() && Container && Container->Character && Container->GetPlayer()->AiEnabled == false && Container->Character->GetItem(this) != nullptr && Container->Character->GetItem(this)->Identified != true) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(this)->Identified = true;
		SaveHero(Container->Character);
	}
	
	this->Identified = true;
	
	if (this->Container != nullptr && this->Container->GetPlayer() == CPlayer::GetThisPlayer()) {
		this->Container->GetPlayer()->Notify(NotifyGreen, this->Container->GetTilePos(), this->Container->GetMapLayer()->GetIndex(), _("%s has identified the %s!"), this->Container->GetMessageName().c_str(), this->GetMessageName().c_str());
	}
}

void CUnit::CheckIdentification()
{
	if (!HasInventory()) {
		return;
	}
	
	CUnit *uins = this->UnitInside;
	
	for (int i = 0; i < this->InsideCount; ++i, uins = uins->NextContained) {
		if (!uins->GetType()->BoolFlag[ITEM_INDEX].value) {
			continue;
		}
		
		if (!uins->Identified && this->Variable[KNOWLEDGEMAGIC_INDEX].Value >= uins->Variable[MAGICLEVEL_INDEX].Value) {
			uins->Identify();
		}
	}
}

void CUnit::CheckKnowledgeChange(int variable, int change) // this happens after the variable has already been changed
{
	if (!change) {
		return;
	}
	
	if (variable == KNOWLEDGEMAGIC_INDEX) {
		int mana_change = (this->Variable[variable].Value / 5) - ((this->Variable[variable].Value - change) / 5); // +1 max mana for every 5 levels in Knowledge (Magic)
		this->Variable[MANA_INDEX].Max += mana_change;
		if (mana_change < 0) {
			this->Variable[MANA_INDEX].Value += mana_change;
		}
		
		this->CheckIdentification();
	} else if (variable == KNOWLEDGEWARFARE_INDEX) {
		int hp_change = (this->Variable[variable].Value / 5) - ((this->Variable[variable].Value - change) / 5); // +1 max HP for every 5 levels in Knowledge (Warfare)
		this->Variable[HP_INDEX].Max += hp_change;
		this->Variable[HP_INDEX].Value += hp_change;
	} else if (variable == KNOWLEDGEMINING_INDEX) {
		int stat_change = (this->Variable[variable].Value / 25) - ((this->Variable[variable].Value - change) / 25); // +1 mining gathering bonus for every 25 levels in Knowledge (Mining)
		this->Variable[COPPERGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[COPPERGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[SILVERGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[SILVERGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[GOLDGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[GOLDGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[IRONGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[IRONGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[MITHRILGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[MITHRILGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[COALGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[COALGATHERINGBONUS_INDEX].Value += stat_change;
		this->Variable[GEMSGATHERINGBONUS_INDEX].Max += stat_change;
		this->Variable[GEMSGATHERINGBONUS_INDEX].Value += stat_change;
	}
}

void CUnit::UpdateItemName()
{
	if (this->Unique) {
		this->Name = this->Unique->GetName().utf8().get_data();
		return;
	}
	
	this->Name.clear();
	if (this->Prefix == nullptr && this->Spell == nullptr && this->Work == nullptr && this->Suffix == nullptr) { //elixirs use the name of their unit type
		return;
	}
	
	if (this->Prefix != nullptr) {
		this->Name += _(this->Prefix->GetName().utf8().get_data());
		this->Name += " ";
	}
	if (this->Work != nullptr) {
		this->Name += _(this->Work->GetName().utf8().get_data());
	} else {
		this->Name += this->GetTypeName();
	}
	if (this->Suffix != nullptr) {
		this->Name += " ";
		this->Name += _(this->Suffix->GetName().utf8().get_data());
	} else if (this->Spell != nullptr) {
		this->Name += " ";
		this->Name += _("of");
		this->Name += " ";
		this->Name += _(this->Spell->GetName().utf8().get_data());
	}
}

void CUnit::GenerateDrop()
{
	bool base_based_mission = false;
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->NumTownHalls > 0 || CPlayer::Players[p]->LostTownHallTimer) {
			base_based_mission = true;
		}
	}
	
	if (this->Type->BoolFlag[ORGANIC_INDEX].value && !this->Character && !this->Type->BoolFlag[FAUNA_INDEX].value && base_based_mission) { //if the unit is organic and isn't a character (and isn't fauna) and this is a base-based mission, don't generate a drop
		return;
	}
	
	Vec2i drop_pos = this->GetTilePos();
	drop_pos.x += SyncRand(this->Type->TileSize.x);
	drop_pos.y += SyncRand(this->Type->TileSize.y);
	CUnit *droppedUnit = nullptr;
	CUnitType *chosen_drop = nullptr;
	std::vector<CUnitType *> potential_drops;
	for (size_t i = 0; i < this->Type->Drops.size(); ++i) {
		if (CheckDependencies(this->Type->Drops[i], this)) {
			potential_drops.push_back(this->Type->Drops[i]);
		}
	}
	if (this->Player->AiEnabled) {
		for (size_t i = 0; i < this->Type->AiDrops.size(); ++i) {
			if (CheckDependencies(this->Type->AiDrops[i], this)) {
				potential_drops.push_back(this->Type->AiDrops[i]);
			}
		}
		for (std::map<std::string, std::vector<CUnitType *>>::const_iterator iterator = this->Type->ModAiDrops.begin(); iterator != this->Type->ModAiDrops.end(); ++iterator) {
			for (size_t i = 0; i < iterator->second.size(); ++i) {
				if (CheckDependencies(iterator->second[i], this)) {
					potential_drops.push_back(iterator->second[i]);
				}
			}
		}
	}
	if (potential_drops.size() > 0) {
		chosen_drop = potential_drops[SyncRand(potential_drops.size())];
	}
		
	if (chosen_drop != nullptr) {
		CBuildRestrictionOnTop *ontop_b = OnTopDetails(*this->Type, nullptr);
		if (((chosen_drop->BoolFlag[ITEM_INDEX].value || chosen_drop->BoolFlag[POWERUP_INDEX].value) && (this->GetMapLayer()->Field(drop_pos)->GetFlags() & MapFieldItem)) || (ontop_b && ontop_b->ReplaceOnDie)) { //if the dropped unit is an item (and there's already another item there), or if this building is an ontop one (meaning another will appear under it after it is destroyed), search for another spot
			Vec2i resPos;
			FindNearestDrop(*chosen_drop, drop_pos, resPos, LookingW, this->GetMapLayer()->GetIndex());
			if (!CMap::Map.Info.IsPointOnMap(resPos, this->GetMapLayer()->GetIndex())) {
				return;
			}
			droppedUnit = MakeUnitAndPlace(resPos, *chosen_drop, CPlayer::Players[PlayerNumNeutral], this->GetMapLayer()->GetIndex());
		} else {
			droppedUnit = MakeUnitAndPlace(drop_pos, *chosen_drop, CPlayer::Players[PlayerNumNeutral], this->GetMapLayer()->GetIndex());
		}
			
		if (droppedUnit != nullptr) {
			if (droppedUnit->GetType()->BoolFlag[FAUNA_INDEX].value) {
				droppedUnit->GenerateName();
			}
			
			droppedUnit->GenerateSpecialProperties(this, this->Player);
			
			if (droppedUnit->GetType()->BoolFlag[ITEM_INDEX].value && !droppedUnit->Unique) { //save the initial cycle items were placed in the ground to destroy them if they have been there for too long
				int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
				if (droppedUnit->Prefix != nullptr || droppedUnit->Suffix != nullptr || droppedUnit->Spell != nullptr || droppedUnit->Work != nullptr || droppedUnit->Elixir != nullptr) {
					ttl_cycles *= 4;
				}
				droppedUnit->TTL = GameCycle + ttl_cycles;
			}
		}
	}
}

void CUnit::GenerateSpecialProperties(const CUnit *dropper, const CPlayer *dropper_player, const bool allow_unique, const bool sold_item, const bool always_magic)
{
	int magic_affix_chance = 10; //10% chance of the unit having a magic prefix or suffix
	int unique_chance = 5; //0.5% chance of the unit being unique
	if (dropper != nullptr) {
		if (dropper->Character) { //if the dropper is a character, multiply the chances of the item being magic or unique by the character's level
			magic_affix_chance *= dropper->Character->GetLevel();
			unique_chance *= dropper->Character->GetLevel();
		} else if (dropper->Type->BoolFlag[BUILDING_INDEX].value) { //if the dropper is a building, multiply the chances of the drop being magic or unique by a factor according to whether the building itself is magic/unique
			int chance_multiplier = 2;
			if (dropper->Unique) {
				chance_multiplier += 8;
			} else {
				if (dropper->Prefix != nullptr) {
					chance_multiplier += 1;
				}
				if (dropper->Suffix != nullptr) {
					chance_multiplier += 1;
				}
			}
			magic_affix_chance *= chance_multiplier;
			unique_chance *= chance_multiplier;
		}
	}
	
	if (sold_item) {
		magic_affix_chance /= 4;
		unique_chance /= 4;
	}

	if (SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GenerateWork(dropper, dropper_player);
	}
	if (SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GeneratePrefix(dropper, dropper_player);
	}
	if (SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GenerateSuffix(dropper, dropper_player);
	}
	if (this->Prefix == nullptr && this->Suffix == nullptr && this->Work == nullptr && this->Elixir == nullptr && SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GenerateSpell(dropper, dropper_player);
	}
	if (allow_unique && SyncRand(1000) >= (1000 - unique_chance)) {
		this->GenerateUnique(dropper, dropper_player);
	}
	
	if (this->Type->BoolFlag[ITEM_INDEX].value && (this->Prefix != nullptr || this->Suffix != nullptr)) {
		this->Identified = false;
	}
	
	if (
		this->Prefix == nullptr && this->Suffix == nullptr && this->Spell == nullptr && this->Work == nullptr && this->Elixir == nullptr
		&& ((this->Type->ItemClass != nullptr && this->Type->ItemClass->IsSpecialPropertyRequired()) || always_magic)
	) { //scrolls, books, jewelry and horns must always have a property
		this->GenerateSpecialProperties(dropper, dropper_player, allow_unique, sold_item, always_magic);
	}
}
			
void CUnit::GeneratePrefix(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const CUpgrade *> potential_prefixes;
	
	for (const CUpgrade *affix : this->Type->Affixes) {
		if ((this->Type->ItemClass == nullptr && affix->MagicPrefix) || (this->Type->ItemClass != nullptr && affix->ItemPrefix.find(this->Type->ItemClass) != affix->ItemPrefix.end())) {
			potential_prefixes.push_back(affix);
		}
	}
	
	if (dropper_player != nullptr) {
		for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
			if (this->Type->ItemClass == nullptr || upgrade->ItemPrefix.find(this->Type->ItemClass) == upgrade->ItemPrefix.end()) {
				continue;
			}
			if (dropper != nullptr) {
				if (!CheckDependencies(upgrade, dropper)) {
					continue;
				}
			} else {
				if (!CheckDependencies(upgrade, dropper_player)) {
					continue;
				}
			}
			
			potential_prefixes.push_back(upgrade);
		}
	}
	
	if (potential_prefixes.size() > 0) {
		this->SetPrefix(potential_prefixes[SyncRand(potential_prefixes.size())]);
	}
}

void CUnit::GenerateSuffix(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const CUpgrade *> potential_suffixes;
	
	for (const CUpgrade *affix : this->Type->Affixes) {
		if ((this->Type->ItemClass == nullptr && affix->MagicSuffix) || (this->Type->ItemClass != nullptr && affix->ItemSuffix.find(this->Type->ItemClass) != affix->ItemSuffix.end())) {
			if (this->Prefix == nullptr || !affix->IncompatibleAffixes[this->Prefix->GetIndex()]) { //don't allow a suffix incompatible with the prefix to appear
				potential_suffixes.push_back(affix);
			}
		}
	}
	
	if (dropper_player != nullptr) {
		for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
			if (this->Type->ItemClass == nullptr || upgrade->ItemSuffix.find(this->Type->ItemClass) == upgrade->ItemSuffix.end()) {
				continue;
			}
			if (dropper != nullptr) {
				if (!CheckDependencies(upgrade, dropper)) {
					continue;
				}
			} else {
				if (!CheckDependencies(upgrade, dropper_player)) {
					continue;
				}
			}
			
			if (this->Prefix != nullptr && upgrade->IncompatibleAffixes[this->Prefix->GetIndex()]) { //don't allow a suffix incompatible with the prefix to appear
				continue;
			}
			
			potential_suffixes.push_back(upgrade);
		}
	}
	
	if (potential_suffixes.size() > 0) {
		this->SetSuffix(potential_suffixes[SyncRand(potential_suffixes.size())]);
	}
}

void CUnit::GenerateSpell(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const CSpell *> potential_spells;
	if (dropper != nullptr) {
		for (const CSpell *spell : dropper->Type->DropSpells) {
			if (this->Type->ItemClass != nullptr && spell->ItemSpell.find(this->Type->ItemClass) != spell->ItemSpell.end()) {
				potential_spells.push_back(spell);
			}
		}
	}
	
	if (potential_spells.size() > 0) {
		this->SetSpell(potential_spells[SyncRand(potential_spells.size())]);
	}
}

void CUnit::GenerateWork(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<const CUpgrade *> potential_works;
	
	for (const CUpgrade *affix : this->Type->Affixes) {
		if (this->Type->ItemClass != nullptr && affix->Work == this->Type->ItemClass && !affix->UniqueOnly) {
			potential_works.push_back(affix);
		}
	}
	
	if (dropper_player != nullptr) {
		for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
			if (this->Type->ItemClass == nullptr || upgrade->Work != this->Type->ItemClass || upgrade->UniqueOnly) {
				continue;
			}
			
			if (dropper != nullptr) {
				if (!CheckDependencies(upgrade, dropper)) {
					continue;
				}
			} else {
				if (!CheckDependencies(upgrade, dropper_player)) {
					continue;
				}
			}
			
			potential_works.push_back(upgrade);
		}
	}
	
	if (potential_works.size() > 0) {
		this->SetWork(potential_works[SyncRand(potential_works.size())]);
	}
}

void CUnit::GenerateUnique(const CUnit *dropper, const CPlayer *dropper_player)
{
	std::vector<UniqueItem *> potential_uniques;
	for (UniqueItem *unique : UniqueItem::GetAll()) {
		if (this->Type != unique->Type) {
			continue;
		}
		
		if (unique->Prefix != nullptr) {
			//the dropper unit must be capable of generating this unique item's prefix to drop the item, or else the unit type must be capable of generating it on its own
			if (std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), unique->Prefix) == this->Type->Affixes.end()) {
				if (dropper_player == nullptr) {
					continue;
				}
				
				if (dropper != nullptr) {
					if (!CheckDependencies(unique->Prefix, dropper)) {
						continue;
					}
				} else {
					if (!CheckDependencies(unique->Prefix, dropper_player)) {
						continue;
					}
				}
			}
		}
		
		if (unique->Suffix != nullptr) {
			//the dropper unit must be capable of generating this unique item's suffix to drop the item, or else the unit type must be capable of generating it on its own
			if (std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), unique->Suffix) == this->Type->Affixes.end()) {
				if (dropper_player == nullptr) {
					continue;
				}
				
				if (dropper != nullptr) {
					if (!CheckDependencies(unique->Suffix, dropper)) {
						continue;
					}
				} else {
					if (!CheckDependencies(unique->Suffix, dropper_player)) {
						continue;
					}
				}
			}
		}
		
		if (unique->Set != nullptr) {
			//the dropper unit must be capable of generating this unique item's set to drop the item
			if (dropper_player == nullptr) {
				continue;
			}
			
			if (dropper != nullptr) {
				if (!CheckDependencies(unique->Set, dropper)) {
					continue;
				}
			} else {
				if (!CheckDependencies(unique->Set, dropper_player)) {
					continue;
				}
			}
		}
		
		if (unique->Spell != nullptr) {
			//the dropper unit must be capable of generating this unique item's spell to drop the item
			if (dropper == nullptr) {
				continue;
			}
			
			if (std::find(dropper->Type->DropSpells.begin(), dropper->Type->DropSpells.end(), unique->Spell) == dropper->Type->DropSpells.end()) {
				continue;
			}
		}
		
		if (unique->Work != nullptr) {
			//the dropper unit must be capable of generating this unique item's work to drop the item, or else the unit type must be capable of generating it on its own
			if (std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), unique->Work) == this->Type->Affixes.end()) {
				if (dropper_player == nullptr) {
					continue;
				}
				
				if (dropper != nullptr) {
					if (!CheckDependencies(unique->Work, dropper)) {
						continue;
					}
				} else {
					if (!CheckDependencies(unique->Work, dropper_player)) {
						continue;
					}
				}
			}
		}
		
		if (unique->Elixir != nullptr) {
			//the dropper unit must be capable of generating this unique item's elixir to drop the item, or else the unit type must be capable of generating it on its own
			if (std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), unique->Elixir) == this->Type->Affixes.end()) {
				if (dropper_player == nullptr) {
					continue;
				}
				
				if (dropper != nullptr) {
					if (!CheckDependencies(unique->Elixir, dropper)) {
						continue;
					}
				} else {
					if (!CheckDependencies(unique->Elixir, dropper_player)) {
						continue;
					}
				}
			}
		}
		
		if (!unique->CanDrop()) {
			continue;
		}
		
		potential_uniques.push_back(unique);
	}
	
	if (potential_uniques.size() > 0) {
		UniqueItem *chosen_unique = potential_uniques[SyncRand(potential_uniques.size())];
		this->SetUnique(chosen_unique);
	}
}

void CUnit::UpdateSoldUnits()
{
	if (!this->Type->BoolFlag[RECRUITHEROES_INDEX].value && this->Type->SoldUnits.empty() && this->SoldUnits.empty()) {
		return;
	}
	
	if (this->UnderConstruction == true || !CMap::Map.Info.IsPointOnMap(this->GetTilePos(), this->MapLayer) || Editor.Running != EditorNotRunning) {
		return;
	}
	
	for (CUnit *sold_unit : this->SoldUnits) {
		DestroyAllInside(*sold_unit);
		LetUnitDie(*sold_unit);
	}
	this->SoldUnits.clear();
	
	std::vector<const CUnitType *> potential_items;
	std::vector<CCharacter *> potential_heroes;
	if (this->Type->BoolFlag[RECRUITHEROES_INDEX].value && !IsNetworkGame()) { // allow heroes to be recruited at town halls
		const CCivilization *civilization = this->Type->GetCivilization();
		if (civilization != nullptr && civilization->GetIndex() != this->Player->Race && this->Player->Race != -1 && this->Player->GetFaction() != nullptr && this->Type == CFaction::GetFactionClassUnitType(this->Player->GetFaction(), this->Type->GetClass())) {
			civilization = CCivilization::Get(this->Player->Race);
		}
		
		if (CurrentQuest == nullptr) {
			for (CCharacter *character : CCharacter::GetAll()) {
				if (this->CanRecruitHero(character)) {
					int weight = 1;
					if (this->Settlement != nullptr && this->Settlement == character->GetHomeSite()) {
						weight = 10; //a character is ten times as likely to be picked in their home site than other characters
					}
					for (int i = 0; i < weight; ++i) {
						potential_heroes.push_back(character);
					}
				}
			}
		}
		if (this->Player == CPlayer::GetThisPlayer()) {
			for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
				if (
					(iterator->second->Civilization && iterator->second->Civilization == civilization || iterator->second->UnitType == CCivilization::GetCivilizationClassUnitType(civilization, iterator->second->UnitType->GetClass()))
					&& CheckDependencies(iterator->second->UnitType, this, true) && iterator->second->CanAppear()
				) {
					potential_heroes.push_back(iterator->second);
				}
			}
		}
	} else {
		for (const CUnitType *sold_unit_type : this->Type->SoldUnits) {
			if (CheckDependencies(sold_unit_type, this)) {
				potential_items.push_back(sold_unit_type);
			}
		}
	}
	
	if (potential_items.empty() && potential_heroes.empty()) {
		return;
	}
	
	int sold_unit_max = 4;
	if (!potential_items.empty()) {
		sold_unit_max = 15;
	}
	
	for (int i = 0; i < sold_unit_max; ++i) {
		CUnit *new_unit = nullptr;
		if (!potential_heroes.empty()) {
			CCharacter *chosen_hero = potential_heroes[SyncRand(potential_heroes.size())];
			new_unit = MakeUnitAndPlace(this->GetTilePos(), *chosen_hero->UnitType, CPlayer::Players[PlayerNumNeutral], this->GetMapLayer()->GetIndex());
			new_unit->SetCharacter(chosen_hero->Ident, chosen_hero->Custom);
			potential_heroes.erase(std::remove(potential_heroes.begin(), potential_heroes.end(), chosen_hero), potential_heroes.end());
		} else {
			const CUnitType *chosen_unit_type = potential_items[SyncRand(potential_items.size())];
			new_unit = MakeUnitAndPlace(this->GetTilePos(), *chosen_unit_type, CPlayer::Players[PlayerNumNeutral], this->GetMapLayer()->GetIndex());
			new_unit->GenerateSpecialProperties(this, this->Player, true, true);
			new_unit->Identified = true;
			if (new_unit->Unique && this->Player == CPlayer::GetThisPlayer()) { //send a notification if a unique item is being sold, we don't want the player to have to worry about missing it :)
				this->Player->Notify(NotifyGreen, this->GetTilePos(), this->GetMapLayer()->GetIndex(), "%s", _("Unique item available for sale"));
			}
		}
		new_unit->Remove(this);
		this->SoldUnits.push_back(new_unit);
		if (potential_heroes.empty() && potential_items.empty()) {
			break;
		}
	}
	if (IsOnlySelected(*this)) {
		UI.ButtonPanel.Update();
	}
}

void CUnit::SellUnit(CUnit *sold_unit, int player)
{
	this->SoldUnits.erase(std::remove(this->SoldUnits.begin(), this->SoldUnits.end(), sold_unit), this->SoldUnits.end());
	DropOutOnSide(*sold_unit, sold_unit->Direction, this);
	if (!sold_unit->GetType()->BoolFlag[ITEM_INDEX].value) {
		sold_unit->ChangeOwner(*CPlayer::Players[player]);
	}
	CPlayer::Players[player]->ChangeResource(CopperCost, -sold_unit->GetPrice(), true);
	if (CPlayer::Players[player]->AiEnabled && !sold_unit->GetType()->BoolFlag[ITEM_INDEX].value && !sold_unit->GetType()->BoolFlag[HARVESTER_INDEX].value) { //add the hero to an AI force, if the hero isn't a harvester
		CPlayer::Players[player]->Ai->Force.RemoveDeadUnit();
		CPlayer::Players[player]->Ai->Force.Assign(*sold_unit, -1, true);
	}
	if (sold_unit->Character) {
		CPlayer::Players[player]->HeroCooldownTimer = HeroCooldownCycles;
		sold_unit->Variable[MANA_INDEX].Value = 0; //start off with 0 mana
	}
	if (IsOnlySelected(*this)) {
		UI.ButtonPanel.Update();
	}
}

/**
**	@brief	Get whether this unit can recruit a given hero
**
**	@param	character	The character
**
**	@return	True if the hero can be recruited, or false otherwise
*/
bool CUnit::CanRecruitHero(const CCharacter *character) const
{
	if (!this->Player->CanRecruitHero(character)) {
		return false;
	}
	
	//if the player is not of the character's faction (and the character has a faction), then they must own the character's home site, and this unit must belong to that site
	if (
		!character->Factions.empty() && (this->Player->GetFaction() == nullptr || std::find(character->Factions.begin(), character->Factions.end(), this->Player->GetFaction()) == character->Factions.end())
		&& (character->GetHomeSite() == nullptr || character->GetHomeSite() != this->Settlement)
	) {
		return false;
	}
	
	return true;
}

/**
**	@brief	Produce a resource
**
**	@param	resource	Resource to be produced.
*/
void CUnit::ProduceResource(const int resource)
{
	if (resource == this->GivesResource) {
		return;
	}
	
	int old_resource = this->GivesResource;
	
	if (resource != 0) {
		this->GivesResource = resource;
		this->ResourcesHeld = 10000;
	} else {
		this->GivesResource = 0;
		this->ResourcesHeld = 0;
	}
	
	if (old_resource != 0) {
		if (this->Resource.Workers) {
			for (CUnit *uins = this->Resource.Workers; uins; uins = uins->NextWorker) {
				if (uins->Container == this) {
					uins->CurrentOrder()->Finished = true;
					DropOutOnSide(*uins, LookingW, this);
				}
			}
		}
		this->Resource.Active = 0;
	}
}

/**
**  Sells 100 of a resource for copper
**
**  @param resource  Resource to be sold.
*/
void CUnit::SellResource(const int resource, const int player)
{
	if ((CPlayer::Players[player]->Resources[resource] + CPlayer::Players[player]->StoredResources[resource]) < 100) {
		return;
	}

	CPlayer::Players[player]->ChangeResource(resource, -100, true);
	CPlayer::Players[player]->ChangeResource(CopperCost, this->Player->GetEffectiveResourceSellPrice(resource), true);
	
	this->Player->DecreaseResourcePrice(resource);
}

/**
**  Buy a resource for copper
**
**  @param resource  Resource to be bought.
*/
void CUnit::BuyResource(const int resource, const int player)
{
	if ((CPlayer::Players[player]->Resources[CopperCost] + CPlayer::Players[player]->StoredResources[CopperCost]) < this->Player->GetEffectiveResourceBuyPrice(resource)) {
		return;
	}

	CPlayer::Players[player]->ChangeResource(resource, 100, true);
	CPlayer::Players[player]->ChangeResource(CopperCost, -this->Player->GetEffectiveResourceBuyPrice(resource), true);
	
	this->Player->IncreaseResourcePrice(resource);
}

void CUnit::Scout()
{
	int scout_range = std::max(16, this->CurrentSightRange * 2);
			
	Vec2i target_pos = this->GetTilePos();

	target_pos.x += SyncRand(scout_range * 2 + 1) - scout_range;
	target_pos.y += SyncRand(scout_range * 2 + 1) - scout_range;

	// restrict to map
	CMap::Map.Clamp(target_pos, this->GetMapLayer()->GetIndex());

	// move if possible
	if (target_pos != this->GetTilePos()) {
		// if the tile the scout is moving to happens to have a layer connector, use it
		CUnitCache &unitcache = CMap::Map.Field(target_pos, this->GetMapLayer()->GetIndex())->UnitCache;
		for (CUnitCache::iterator it = unitcache.begin(); it != unitcache.end(); ++it) {
			CUnit *connector = *it;

			if (connector->ConnectingDestination == nullptr || !this->CanUseItem(connector)) {
				continue;
			}
			
			if (!UnitReachable(*this, *connector, 1, this->GetReactionRange() * 8)) {
				continue;
			}
			
			CommandUse(*this, *connector, FlushCommands);
			return;
		}
		
		UnmarkUnitFieldFlags(*this);
		if (UnitCanBeAt(*this, target_pos, this->GetMapLayer()->GetIndex())) {
			MarkUnitFieldFlags(*this);
			
			if (!PlaceReachable(*this, target_pos, 1, 1, 0, 1, this->GetReactionRange() * 8, this->GetMapLayer()->GetIndex())) {
				return;
			}
			
			CommandMove(*this, target_pos, FlushCommands, this->GetMapLayer()->GetIndex());
			return;
		}
		MarkUnitFieldFlags(*this);
	}
}
//Wyrmgus end

unsigned int CUnit::CurrentAction() const
{
	return (CurrentOrder()->Action);
}

void CUnit::ClearAction()
{
	Orders[0]->Finished = true;

	if (Selected) {
		SelectedUnitChanged();
	}
}


bool CUnit::IsIdle() const
{
	//Wyrmgus start
//	return Orders.size() == 1 && CurrentAction() == UnitActionStill;
	return Orders.size() == 1 && CurrentAction() == UnitActionStill && this->Variable[STUN_INDEX].Value == 0;
	//Wyrmgus end
}

bool CUnit::IsAlive() const
{
	return !Destroyed && CurrentAction() != UnitActionDie;
}

int CUnit::GetDrawLevel() const
{
	return ((this->Type->CorpseType && CurrentAction() == UnitActionDie) ?
		this->Type->CorpseType->GetDrawLevel() :
	((CurrentAction() == UnitActionDie) ? this->Type->GetDrawLevel() - 10 : this->Type->GetDrawLevel()));
}

/**
**  Initialize the unit slot with default values.
**
**  @param type    Unit-type
*/
void CUnit::Init(const CUnitType &type)
{
	//  Set refs to 1. This is the "I am alive ref", lost in ReleaseUnit.
	Refs = 1;

	//  Build all unit table
	UnitManager.Add(this);

	//  Initialise unit structure (must be zero filled!)
	Type = &type;

	Seen.Frame = UnitNotSeen; // Unit isn't yet seen

	this->Frame = type.StillFrame;

	if (UnitTypeVar.GetNumberVariable()) {
		Assert(!Variable);
		const unsigned int size = UnitTypeVar.GetNumberVariable();
		Variable = new CVariable[size];
		std::copy(type.MapDefaultStat.Variables, type.MapDefaultStat.Variables + size, Variable);
	} else {
		Variable = nullptr;
	}

	IndividualUpgrades.clear();

	// Set a heading for the unit if it Handles Directions
	// Don't set a building heading, as only 1 construction direction
	//   is allowed.
	if (type.NumDirections > 1 && type.BoolFlag[NORANDOMPLACING_INDEX].value == false && type.Sprite && !type.BoolFlag[BUILDING_INDEX].value) {
		Direction = (SyncRand() >> 8) & 0xFF; // random heading
		UnitUpdateHeading(*this);
	}

	this->AutoCastSpells = this->Type->AutoCastActiveSpells;

	Active = 1;
	Removed = 1;
	
	// Has StartingResources, Use those
	//Wyrmgus start
//	this->ResourcesHeld = type.StartingResources;
	if (type.GivesResource) {
		this->GivesResource = type.GivesResource;
		if (type.StartingResources.size() > 0) {
			this->ResourcesHeld = type.StartingResources[SyncRand(type.StartingResources.size())];
		} else {
			this->ResourcesHeld = 0;
		}
	}
	//Wyrmgus end

	Assert(Orders.empty());

	Orders.push_back(COrder::NewActionStill());

	Assert(NewOrder == nullptr);
	NewOrder = nullptr;
	Assert(SavedOrder == nullptr);
	SavedOrder = nullptr;
	Assert(CriticalOrder == nullptr);
	CriticalOrder = nullptr;
}

/**
**  Restore the saved order
**
**  @return      True if the saved order was restored
*/
bool CUnit::RestoreOrder()
{
	COrder *savedOrder = this->SavedOrder;

	if (savedOrder == nullptr) {
		return false;
	}

	if (savedOrder->IsValid() == false) {
		delete savedOrder;
		this->SavedOrder = nullptr;
		return false;
	}

	// Cannot delete this->Orders[0] since it is generally that order
	// which call this method.
	this->Orders[0]->Finished = true;

	//copy
	this->Orders.insert(this->Orders.begin() + 1, savedOrder);

	this->SavedOrder = nullptr;
	return true;
}

/**
**  Check if we can store this order
**
**  @return      True if the order could be saved
*/
bool CUnit::CanStoreOrder(COrder *order)
{
	Assert(order);

	if ((order && order->Finished == true) || order->IsValid() == false) {
		return false;
	}
	if (this->SavedOrder != nullptr) {
		return false;
	}
	return true;
}

/**
**  Assigns a unit to a player, adjusting buildings, food and totals
**
**  @param player  player which have the unit.
*/
void CUnit::AssignToPlayer(CPlayer &player)
{
	const CUnitType &type = *Type;

	// Build player unit table
	//Wyrmgus start
//	if (!type.BoolFlag[VANISHES_INDEX].value && CurrentAction() != UnitActionDie) {
	if (!type.BoolFlag[VANISHES_INDEX].value && CurrentAction() != UnitActionDie && !this->Destroyed) {
	//Wyrmgus end
		player.AddUnit(*this);
		if (!SaveGameLoading) {
			// If unit is dying, it's already been lost by all players
			// don't count again
			if (type.BoolFlag[BUILDING_INDEX].value) {
				// FIXME: support more races
				//Wyrmgus start
//				if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
				//Wyrmgus end
					player.TotalBuildings++;
				//Wyrmgus start
//				}
				//Wyrmgus end
			} else {
				player.TotalUnits++;
			}
		}
		
		player.IncreaseCountsForUnit(this);

		player.Demand += type.Stats[player.GetIndex()].Variables[DEMAND_INDEX].Value; // food needed
	}

	// Don't Add the building if it's dying, used to load a save game
	//Wyrmgus start
//	if (type.BoolFlag[BUILDING_INDEX].value && CurrentAction() != UnitActionDie) {
	if (type.BoolFlag[BUILDING_INDEX].value && CurrentAction() != UnitActionDie && !this->Destroyed && !type.BoolFlag[VANISHES_INDEX].value) {
	//Wyrmgus end
		//Wyrmgus start
//		if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
		//Wyrmgus end
			player.NumBuildings++;
			//Wyrmgus start
			if (CurrentAction() == UnitActionBuilt) {
				player.NumBuildingsUnderConstruction++;
				player.ChangeUnitTypeUnderConstructionCount(&type, 1);
			}
			//Wyrmgus end
		//Wyrmgus start
//		}
		//Wyrmgus end
	}
	Player = &player;
	Stats = &type.Stats[Player->GetIndex()];
	Colors = &player.UnitColors;
	if (!SaveGameLoading) {
		if (UnitTypeVar.GetNumberVariable()) {
			Assert(Variable);
			Assert(Stats->Variables);
			memcpy(Variable, Stats->Variables, UnitTypeVar.GetNumberVariable() * sizeof(*Variable));
		}
	}
	
	//Wyrmgus start
	if (!SaveGameLoading) {
		//assign a gender to the unit
		if (this->Variable[GENDER_INDEX].Value == 0 && this->Type->BoolFlag[ORGANIC_INDEX].value) { // Gender: 0 = Not Set; upper values equal the index of the gender plus 1
			CSpecies *species = nullptr;
			if (this->Type->GetSpecies() != nullptr) {
				species = this->Type->GetSpecies();
			} else if (this->Type->GetCivilization() != nullptr) {
				species = this->Type->GetCivilization()->GetSpecies();
			}
			if (species != nullptr) {
				const std::vector<const CGender *> &genders = species->GetGenders();
				if (!genders.empty()) {
					this->Variable[GENDER_INDEX].Value = genders[SyncRand(genders.size())]->GetIndex() + 1;
					this->Variable[GENDER_INDEX].Max = CGender::GetAll().size();
					this->Variable[GENDER_INDEX].Enable = 1;
				}
			}
		}
		
		//generate a personal name for the unit, if applicable
		if (this->Character == nullptr) {
			this->UpdatePersonalName();
		}
		
		this->UpdateSoldUnits();
		
		if (!type.BoolFlag[BUILDING_INDEX].value) {
			//for buildings this is already handled in the "built" action
			for (CPlayerQuestObjective *objective : player.QuestObjectives) {
				if (
					objective->ObjectiveType == BuildUnitsObjectiveType
					&& (
						std::find(objective->UnitTypes.begin(), objective->UnitTypes.end(), &type) != objective->UnitTypes.end()
						|| std::find(objective->UnitClasses.begin(), objective->UnitClasses.end(), type.GetClass()) != objective->UnitClasses.end()
					)
				) {
					if (!objective->Settlement || objective->Settlement == this->Settlement) {
						objective->Counter = std::min(objective->Counter + 1, objective->Quantity);
					}
				}
			}
		}
	}
	//Wyrmgus end
}

/**
**  Create a new unit.
**
**  @param type      Pointer to unit-type.
**  @param player    Pointer to owning player.
**
**  @return          Pointer to created unit.
*/
CUnit *MakeUnit(const CUnitType &type, CPlayer *player)
{
	CUnit *unit = UnitManager.AllocUnit();
	if (unit == nullptr) {
		return nullptr;
	}
	unit->Init(type);
	// Only Assign if a Player was specified
	if (player) {
		unit->AssignToPlayer(*player);

		//Wyrmgus start
		unit->ChooseVariation(nullptr);
		for (int i = 0; i < MaxImageLayers; ++i) {
			unit->ChooseVariation(nullptr, i);
		}
		unit->UpdateButtonIcons();
		unit->UpdateXPRequired();
		//Wyrmgus end
	}

	//Wyrmgus start
	// grant the unit the civilization/faction upgrades of its respective civilization/faction, so that it is able to pursue its upgrade line in experience upgrades even if it changes hands
	if (unit->GetType()->GetCivilization() != nullptr && unit->GetType()->GetCivilization()->GetUpgrade() != nullptr) {
		const CUpgrade *civilization_upgrade = unit->GetType()->GetCivilization()->GetUpgrade();
		if (civilization_upgrade) {
			unit->SetIndividualUpgrade(civilization_upgrade, 1);
		}
	}
	if (unit->GetType()->GetCivilization() != nullptr && unit->GetType()->GetFaction() != nullptr) {
		const CUpgrade *faction_upgrade = unit->GetType()->GetFaction()->GetUpgrade();
		if (faction_upgrade != nullptr) {
			unit->SetIndividualUpgrade(faction_upgrade, 1);
		}
	}

	// generate a trait for the unit, if any are available (only if the editor isn't running)
	if (Editor.Running == EditorNotRunning && unit->GetType()->Traits.size() > 0) {
		TraitAcquire(*unit, unit->GetType()->Traits[SyncRand(unit->GetType()->Traits.size())]);
	}
	
	for (size_t i = 0; i < unit->GetType()->StartingAbilities.size(); ++i) {
		if (CheckDependencies(unit->GetType()->StartingAbilities[i], unit)) {
			IndividualUpgradeAcquire(*unit, unit->GetType()->StartingAbilities[i]);
		}
	}
	
	if (unit->GetType()->Elixir) { //set the unit type's elixir, if any
		unit->SetElixir(unit->GetType()->Elixir);
	}
	
	unit->Variable[MANA_INDEX].Value = 0; //start off with 0 mana
	//Wyrmgus end
	
	if (unit->GetType()->OnInit) {
		unit->GetType()->OnInit->pushPreamble();
		unit->GetType()->OnInit->pushInteger(UnitNumber(*unit));
		unit->GetType()->OnInit->run();
	}

	//  fancy buildings: mirror buildings (but shadows not correct)
	if (type.BoolFlag[BUILDING_INDEX].value && FancyBuildings
		&& unit->GetType()->BoolFlag[NORANDOMPLACING_INDEX].value == false && (SyncRand() & 1) != 0) {
		unit->SetFrame(-unit->GetFrame() - 1);
	}
	
	return unit;
}

/**
**  (Un)Mark on vision table the Sight of the unit
**  (and units inside for transporter (recursively))
**
**  @param unit    Unit to (un)mark.
**  @param pos     coord of first container of unit.
**  @param width   Width of the first container of unit.
**  @param height  Height of the first container of unit.
**  @param f       Function to (un)mark for normal vision.
**  @param f2        Function to (un)mark for cloaking vision.
*/
static void MapMarkUnitSightRec(const CUnit &unit, const Vec2i &pos, int width, int height,
								//Wyrmgus start
//								MapMarkerFunc *f, MapMarkerFunc *f2)
								MapMarkerFunc *f, MapMarkerFunc *f2, MapMarkerFunc *f3)
								//Wyrmgus end
{
	Assert(f);
	//Wyrmgus start
	/*
	MapSight(*unit.GetPlayer(), pos, width, height,
			 unit.GetFirstContainer()->CurrentSightRange, f);

	if (unit.GetType() && unit.GetType()->BoolFlag[DETECTCLOAK_INDEX].value && f2) {
		MapSight(*unit.GetPlayer(), pos, width, height,
				 unit.GetFirstContainer()->CurrentSightRange, f2);
	}
	*/

	MapSight(*unit.GetPlayer(), pos, width, height,
			 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f, unit.GetMapLayer()->GetIndex());

	if (unit.GetType() && unit.GetType()->BoolFlag[DETECTCLOAK_INDEX].value && f2) {
		MapSight(*unit.GetPlayer(), pos, width, height,
				 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f2, unit.GetMapLayer()->GetIndex());
	}
	
	if (unit.Variable[ETHEREALVISION_INDEX].Value && f3) {
		MapSight(*unit.GetPlayer(), pos, width, height,
				 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f3, unit.GetMapLayer()->GetIndex());
	}
	//Wyrmgus end

	CUnit *unit_inside = unit.UnitInside;
	for (int i = unit.InsideCount; i--; unit_inside = unit_inside->NextContained) {
		//Wyrmgus start
//		MapMarkUnitSightRec(*unit_inside, pos, width, height, f, f2);
		MapMarkUnitSightRec(*unit_inside, pos, width, height, f, f2, f3);
		//Wyrmgus end
	}
}

/**
**	@brief	Return the topmost container for the unit
**
**	@param	unit	The unit for which to get the topmost container
**
**	@return	The unit's topmost container if present, or the unit itself otherwise; this function should never return null
*/
CUnit *CUnit::GetFirstContainer() const
{
	const CUnit *container = this;

	while (container->Container) {
		container = container->Container;
	}
	
	return const_cast<CUnit *>(container);
}

void CUnit::SetSelected(const bool selected)
{
	if (selected == this->IsSelected()) {
		return;
	}
	
	this->Selected = selected;
	
	Color selection_color;
	
	if (selected) {
		selection_color = IntColorToColor(this->GetSelectionColor());
	}
	
	this->emit_signal("selected_changed", selected, selection_color);
}

/**
**  Mark on vision table the Sight of the unit
**  (and units inside for transporter)
**
**  @param unit  unit to unmark its vision.
**  @see MapUnmarkUnitSight.
*/
void MapMarkUnitSight(CUnit &unit)
{
	CUnit *container = unit.GetFirstContainer();// First container of the unit.
	Assert(container->GetType());

	MapMarkUnitSightRec(unit, container->GetTilePos(), container->GetType()->TileSize.x, container->GetType()->TileSize.y,
						//Wyrmgus start
//						MapMarkTileSight, MapMarkTileDetectCloak);
						MapMarkTileSight, MapMarkTileDetectCloak, MapMarkTileDetectEthereal);
						//Wyrmgus end

	// Never mark radar, except if the top unit, and unit is usable
	if (&unit == container && !unit.IsUnusable()) {
		if (unit.Stats->Variables[RADAR_INDEX].Value) {
			MapMarkRadar(*unit.GetPlayer(), unit.GetTilePos(), unit.GetType()->TileSize.x,
						 unit.GetType()->TileSize.y, unit.Stats->Variables[RADAR_INDEX].Value, unit.GetMapLayer()->GetIndex());
		}
		if (unit.Stats->Variables[RADARJAMMER_INDEX].Value) {
			MapMarkRadarJammer(*unit.GetPlayer(), unit.GetTilePos(), unit.GetType()->TileSize.x,
							   unit.GetType()->TileSize.y, unit.Stats->Variables[RADARJAMMER_INDEX].Value, unit.GetMapLayer()->GetIndex());
		}
	}

	//Wyrmgus start
	if (unit.Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value) {
		MapMarkOwnership(*unit.GetPlayer(), unit.GetTilePos(), unit.GetType()->TileSize.x,
						   unit.GetType()->TileSize.y, unit.Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value, unit.GetMapLayer()->GetIndex());
	}
	//Wyrmgus end
}

/**
**  Unmark on vision table the Sight of the unit
**  (and units inside for transporter)
**
**  @param unit    unit to unmark its vision.
**  @see MapMarkUnitSight.
*/
void MapUnmarkUnitSight(CUnit &unit)
{
	Assert(unit.GetType());

	CUnit *container = unit.GetFirstContainer();
	Assert(container->GetType());
	MapMarkUnitSightRec(unit,
						container->GetTilePos(), container->GetType()->TileSize.x, container->GetType()->TileSize.y,
						//Wyrmgus start
//						MapUnmarkTileSight, MapUnmarkTileDetectCloak);
						MapUnmarkTileSight, MapUnmarkTileDetectCloak, MapUnmarkTileDetectEthereal);
						//Wyrmgus end

	// Never mark radar, except if the top unit?
	if (&unit == container && !unit.IsUnusable()) {
		if (unit.Stats->Variables[RADAR_INDEX].Value) {
			MapUnmarkRadar(*unit.GetPlayer(), unit.GetTilePos(), unit.GetType()->TileSize.x,
						   unit.GetType()->TileSize.y, unit.Stats->Variables[RADAR_INDEX].Value, unit.GetMapLayer()->GetIndex());
		}
		if (unit.Stats->Variables[RADARJAMMER_INDEX].Value) {
			MapUnmarkRadarJammer(*unit.GetPlayer(), unit.GetTilePos(), unit.GetType()->TileSize.x,
								 unit.GetType()->TileSize.y, unit.Stats->Variables[RADARJAMMER_INDEX].Value, unit.GetMapLayer()->GetIndex());
		}
		
	}
	
	//Wyrmgus start
	if (unit.Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value) {
		MapUnmarkOwnership(*unit.GetPlayer(), unit.GetTilePos(), unit.GetType()->TileSize.x,
							 unit.GetType()->TileSize.y, unit.Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value, unit.GetMapLayer()->GetIndex());
	}
	//Wyrmgus end
}

/**
**  Update the Unit Current sight range to good value and transported units inside.
**
**  @param unit  unit to update SightRange
**
**  @internal before using it, MapUnmarkUnitSight(unit)
**  and after MapMarkUnitSight(unit)
**  are often necessary.
**
**  FIXME @todo manage differently unit inside with option.
**  (no vision, min, host value, own value, bonus value, ...)
*/
void UpdateUnitSightRange(CUnit &unit)
{
//Wyrmgus start
/*
#if 0 // which is the better ? caller check ?
	if (SaveGameLoading) {
		return ;
	}
#else
	Assert(!SaveGameLoading);
#endif
*/
//Wyrmgus end
	// FIXME : these values must be configurable.
	//Wyrmgus start
	int unit_sight_range = unit.Variable[SIGHTRANGE_INDEX].Max;
	if (unit.GetMapLayer() != nullptr) {
		if (unit.GetMapLayer()->GetTimeOfDay() && unit.GetMapLayer()->GetTimeOfDay()->IsDay()) {
			unit_sight_range += unit.Variable[DAYSIGHTRANGEBONUS_INDEX].Value;
		} else if (unit.GetMapLayer()->GetTimeOfDay() && unit.GetMapLayer()->GetTimeOfDay()->IsNight()) {
			unit_sight_range += unit.Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value;
		}
	}
	unit_sight_range = std::max<int>(1, unit_sight_range);
	//Wyrmgus end
	if (unit.UnderConstruction) { // Units under construction have no sight range.
		unit.CurrentSightRange = 1;
	} else if (!unit.Container) { // proper value.
		//Wyrmgus start
//		unit.CurrentSightRange = unit.Stats->Variables[SIGHTRANGE_INDEX].Max;
		unit.CurrentSightRange = unit_sight_range;
		//Wyrmgus end
	} else { // value of it container.
		//Wyrmgus start
//		unit.CurrentSightRange = unit.Container->CurrentSightRange;
		//if a unit is inside a container, then use the sight of the unit or the container, whichever is greater
		if (unit_sight_range <= unit.Container->CurrentSightRange) {
			unit.CurrentSightRange = unit.Container->CurrentSightRange;
		} else {
			unit.CurrentSightRange = unit_sight_range;
		}
		//Wyrmgus end
	}

	CUnit *unit_inside = unit.UnitInside;
	for (int i = unit.InsideCount; i--; unit_inside = unit_inside->NextContained) {
		UpdateUnitSightRange(*unit_inside);
	}
}

/**
**  Mark the field with the FieldFlags.
**
**  @param unit  unit to mark.
*/
void MarkUnitFieldFlags(const CUnit &unit)
{
	const unsigned int flags = unit.GetType()->FieldFlags;
	int h = unit.GetType()->TileSize.y;          // Tile height of the unit.
	const int width = unit.GetType()->TileSize.x; // Tile width of the unit.
	unsigned int index = unit.Offset;

	//Wyrmgus start
//	if (unit.GetType()->BoolFlag[VANISHES_INDEX].value) {
	if (unit.GetType()->BoolFlag[VANISHES_INDEX].value || unit.CurrentAction() == UnitActionDie) {
	//Wyrmgus end
		return ;
	}
	do {
		CMapField *mf = unit.GetMapLayer()->Field(index);
		int w = width;
		do {
			mf->Flags |= flags;
			++mf;
		} while (--w);
		index += unit.GetMapLayer()->GetWidth();
	} while (--h);
}

class _UnmarkUnitFieldFlags
{
public:
	_UnmarkUnitFieldFlags(const CUnit &unit, CMapField *mf) : main(&unit), mf(mf)
	{}

	void operator()(CUnit *const unit) const
	{
		if (main != unit && unit->CurrentAction() != UnitActionDie) {
			mf->Flags |= unit->GetType()->FieldFlags;
		}
	}
private:
	const CUnit *const main;
	CMapField *mf;
};


/**
**  Mark the field with the FieldFlags.
**
**  @param unit  unit to mark.
*/
void UnmarkUnitFieldFlags(const CUnit &unit)
{
	const unsigned int flags = ~unit.GetType()->FieldFlags;
	const int width = unit.GetType()->TileSize.x;
	int h = unit.GetType()->TileSize.y;
	unsigned int index = unit.Offset;

	if (unit.GetType()->BoolFlag[VANISHES_INDEX].value) {
		return ;
	}
	do {
		CMapField *mf = unit.GetMapLayer()->Field(index);

		int w = width;
		do {
			mf->Flags &= flags;//clean flags
			_UnmarkUnitFieldFlags funct(unit, mf);

			mf->UnitCache.for_each(funct);
			++mf;
		} while (--w);
		index += unit.GetMapLayer()->GetWidth();
	} while (--h);
}

/**
**  Add unit to a container. It only updates linked list stuff.
**
**  @param host  Pointer to container.
*/
void CUnit::AddInContainer(CUnit &host)
{
	Assert(Container == nullptr);
	Container = &host;
	if (host.InsideCount == 0) {
		NextContained = PrevContained = this;
	} else {
		NextContained = host.UnitInside;
		PrevContained = host.UnitInside->PrevContained;
		host.UnitInside->PrevContained->NextContained = this;
		host.UnitInside->PrevContained = this;
	}
	host.UnitInside = this;
	host.InsideCount++;
	//Wyrmgus start
	if (!SaveGameLoading) { //if host has no range by itself, but the unit has range, and the unit can attack from a transporter, change the host's range to the unit's; but don't do this while loading, as it causes a crash (since one unit needs to be loaded before the other, and when this function is processed both won't already have their variables set)
		host.UpdateContainerAttackRange();
	}
	//Wyrmgus end
}

/**
**  Remove unit from a container. It only updates linked list stuff.
**
**  @param unit  Pointer to unit.
*/
static void RemoveUnitFromContainer(CUnit &unit)
{
	CUnit *host = unit.Container; // transporter which contain unit.
	Assert(unit.Container);
	Assert(unit.Container->InsideCount > 0);
	
	host->InsideCount--;
	unit.NextContained->PrevContained = unit.PrevContained;
	unit.PrevContained->NextContained = unit.NextContained;
	if (host->InsideCount == 0) {
		host->UnitInside = nullptr;
	} else {
		if (host->UnitInside == &unit) {
			host->UnitInside = unit.NextContained;
		}
	}
	unit.Container = nullptr;
	//Wyrmgus start
	//reset host attack range
	host->UpdateContainerAttackRange();
	//Wyrmgus end
}

//Wyrmgus start
void CUnit::UpdateContainerAttackRange()
{
	//reset attack range, if this unit is a transporter (or garrisonable building) from which units can attack
	if (this->Type->CanTransport() && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && !this->Type->CanAttack) {
		this->Variable[ATTACKRANGE_INDEX].Enable = 0;
		this->Variable[ATTACKRANGE_INDEX].Max = 0;
		this->Variable[ATTACKRANGE_INDEX].Value = 0;
		if (this->BoardCount > 0) {
			CUnit *boarded_unit = this->UnitInside;
			for (int i = 0; i < this->InsideCount; ++i, boarded_unit = boarded_unit->NextContained) {
				if (boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX) > this->Variable[ATTACKRANGE_INDEX].Value && boarded_unit->GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) { //if container has no range by itself, but the unit has range, and the unit can attack from a transporter, change the container's range to the unit's
					this->Variable[ATTACKRANGE_INDEX].Enable = 1;
					this->Variable[ATTACKRANGE_INDEX].Max = boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX);
					this->Variable[ATTACKRANGE_INDEX].Value = boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX);
				}
			}
		}
	}
}

void CUnit::UpdateXPRequired()
{
	if (!this->Type->BoolFlag[ORGANIC_INDEX].value) {
		return;
	}
	
	this->Variable[XPREQUIRED_INDEX].Value = this->Type->Stats[this->Player->GetIndex()].Variables[POINTS_INDEX].Value * 4 * this->Type->Stats[this->Player->GetIndex()].Variables[LEVEL_INDEX].Value;
	int extra_levels = this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->GetIndex()].Variables[LEVEL_INDEX].Value;
	for (int i = 1; i <= extra_levels; ++i) {
		this->Variable[XPREQUIRED_INDEX].Value += 50 * 4 * i;
	}
	this->Variable[XPREQUIRED_INDEX].Max = this->Variable[XPREQUIRED_INDEX].Value;
	this->Variable[XPREQUIRED_INDEX].Enable = 1;
	this->Variable[XP_INDEX].Enable = 1;
}

/**
**	@brief	Get the potential personal name words for this unit
**
**	@return	The potential personal name words
*/
std::vector<String> CUnit::GetPotentialNames() const
{
	std::vector<String> potential_names;
	
	const CGender *gender = nullptr;
	if (this->Variable[GENDER_INDEX].Value != 0) {
		gender = CGender::Get(this->Variable[GENDER_INDEX].Value - 1);
	}
	
	if (this->Type->PersonalNames.find(nullptr) != this->Type->PersonalNames.end()) {
		for (size_t i = 0; i < this->Type->PersonalNames.find(nullptr)->second.size(); ++i) {
			potential_names.push_back(this->Type->PersonalNames.find(nullptr)->second[i].c_str());
		}
	}
	if (gender != nullptr && this->Type->PersonalNames.find(gender) != this->Type->PersonalNames.end()) {
		for (size_t i = 0; i < this->Type->PersonalNames.find(gender)->second.size(); ++i) {
			potential_names.push_back(this->Type->PersonalNames.find(gender)->second[i].c_str());
		}
	}
	
	if (this->Type->BoolFlag[FAUNA_INDEX].value) {
		if (this->Type->GetSpecies() != nullptr) {
			const std::vector<CWord *> &specimen_name_words = this->Type->GetSpecies()->GetSpecimenNameWords(gender);
			for (const CWord *name_word : specimen_name_words) {
				if (!CheckDependencies(name_word, this)) {
					continue;
				}
				
				const int weight = name_word->GetSpecimenNameWeight(this->Type->GetSpecies(), gender);
				for (int i = 0; i < weight; ++i) {
					potential_names.push_back(name_word->GetAnglicizedName());
				}
			}
		}
	} else if (this->Type->GetCivilization() != nullptr) {
		const CCivilization *civilization = this->Type->GetCivilization();
		const CFaction *faction = nullptr;
		if (this->Player->GetFaction() != nullptr) {
			faction = this->Player->GetFaction();
			
			if (civilization != nullptr && civilization != faction->GetCivilization() && civilization->GetSpecies() == faction->GetCivilization()->GetSpecies() && this->Type == CFaction::GetFactionClassUnitType(faction, this->Type->GetClass())) {
				civilization = faction->GetCivilization();
			}
		}
		if (faction && faction->GetCivilization() != civilization) {
			faction = nullptr;
		}
		if (this->Type->GetFaction() != nullptr && !faction) {
			faction = this->Type->GetFaction();
		}
		
		const CLanguage *language = this->GetLanguage();
		
		if (language != nullptr) {
			if (this->Type->BoolFlag[ORGANIC_INDEX].value) {
				const std::vector<CWord *> &personal_name_words = language->GetPersonalNameWords(gender);
				for (const CWord *name_word : personal_name_words) {
					if (!CheckDependencies(name_word, this)) {
						continue;
					}
					
					const int weight = name_word->GetPersonalNameWeight(gender);
					for (int i = 0; i < weight; ++i) {
						potential_names.push_back(name_word->GetAnglicizedName());
					}
				}
				
				if (civilization->GetPersonalNames().find(nullptr) != civilization->GetPersonalNames().end()) {
					for (size_t i = 0; i < civilization->GetPersonalNames().find(nullptr)->second.size(); ++i) {
						potential_names.push_back(civilization->GetPersonalNames().find(nullptr)->second[i].c_str());
					}
				}
				if (gender != nullptr && civilization->GetPersonalNames().find(gender) != civilization->GetPersonalNames().end()) {
					for (size_t i = 0; i < civilization->GetPersonalNames().find(gender)->second.size(); ++i) {
						potential_names.push_back(civilization->GetPersonalNames().find(gender)->second[i].c_str());
					}
				}
			} else {
				if (this->Type->GetClass() != nullptr) {
					const std::vector<CWord *> &unit_class_name_words = language->GetUnitNameWords(this->Type->GetClass());
					for (const CWord *name_word : unit_class_name_words) {
						if (!CheckDependencies(name_word, this)) {
							continue;
						}
						
						const int weight = name_word->GetUnitNameWeight(this->Type->GetClass());
						for (int i = 0; i < weight; ++i) {
							potential_names.push_back(name_word->GetAnglicizedName());
						}
					}
					
					for (const std::string &name : civilization->GetUnitClassNames(this->Type->GetClass())) {
						potential_names.push_back(name.c_str());
					}
				}
				
				if (potential_names.size() < CWord::MinimumWordsForNameGeneration && this->Type->UnitType == UnitTypeNaval) { // if is a ship
					const std::vector<CWord *> &ship_name_words = language->GetShipNameWords();
					for (const CWord *name_word : ship_name_words) {
						if (!CheckDependencies(name_word, this)) {
							continue;
						}
						
						const int weight = name_word->GetShipNameWeight();
						for (int i = 0; i < weight; ++i) {
							potential_names.push_back(name_word->GetAnglicizedName());
						}
					}
					
					if (faction) {
						for (const std::string &name : faction->GetShipNames()) {
							potential_names.push_back(name.c_str());
						}
					}
					
					for (const std::string &name : civilization->GetShipNames()) {
						potential_names.push_back(name.c_str());
					}
				}
			}
		}
	}
	
	return potential_names;
}

/**
**	@brief	Generate a name for the unit
*/
void CUnit::GenerateName()
{
	std::vector<String> potential_names = this->GetPotentialNames();
	
	if (!potential_names.empty()) {
		const String chosen_name = potential_names[SyncRand(potential_names.size())];
		this->Name = chosen_name.utf8().get_data();
	}
}

/**
**	@brief	Get whether the unit's name is valid
**
**	@return	True if the name is valid, or false otherwise
*/
bool CUnit::IsNameValid() const
{
	std::vector<String> potential_names = this->GetPotentialNames();
	
	for (const String &name : potential_names) {
		if (String(this->Name.c_str()) == name) {
			return true;
		}
	}
	
	return false;
}

void CUnit::UpdatePersonalName(const bool update_settlement_name)
{
	if (this->Character != nullptr) {
		return;
	} else if (this->Type->BoolFlag[ITEM_INDEX].value || this->Unique || this->Prefix || this->Suffix) {
		this->UpdateItemName();
		return;
	}
	
	const CCivilization *civilization = this->Type->GetCivilization();
	
	const CFaction *faction = nullptr;
	if (this->Player->GetFaction() != nullptr) {
		faction = this->Player->GetFaction();
		
		if (civilization != nullptr && civilization != faction->GetCivilization() && civilization->GetSpecies() == faction->GetCivilization()->GetSpecies() && this->Type == CFaction::GetFactionClassUnitType(faction, this->Type->GetClass())) {
			civilization = faction->GetCivilization();
		}
	}
	
	CLanguage *language = nullptr;
	if (civilization) {
		language = civilization->GetLanguage();
	}
	
	if (this->Name.empty()) { //this is the first time the unit receives a name
		if (!this->Type->BoolFlag[FAUNA_INDEX].value && this->Trait != nullptr && !this->Trait->GetEpithets().empty() && SyncRand(4) == 0) { // 25% chance to give the unit an epithet based on their trait
			this->ExtraName = this->Trait->GetEpithets()[SyncRand(this->Trait->GetEpithets().size())].utf8().get_data();
		}
	}
	
	if (!this->IsNameValid()) {
		// first see if can translate the current personal name
		std::string new_personal_name = language != nullptr ? language->TranslateName(this->Name.c_str()).utf8().get_data() : this->Name;
		if (!new_personal_name.empty()) {
			this->Name = new_personal_name;
		} else {
			this->GenerateName();
		}
	}
	
	if (update_settlement_name && (this->Type->BoolFlag[TOWNHALL_INDEX].value || (this->Type->BoolFlag[BUILDING_INDEX].value && !this->Settlement))) {
		this->UpdateSettlement();
	}
}

void CUnit::UpdateExtraName()
{
	if (this->Character != nullptr || !this->Type->BoolFlag[ORGANIC_INDEX].value || this->Type->BoolFlag[FAUNA_INDEX].value) {
		return;
	}
	
	if (this->Trait == nullptr) {
		return;
	}
	
	this->ExtraName.clear();
	
	if (this->Trait->GetEpithets().size() > 0 && SyncRand(4) == 0) { // 25% chance to give the unit an epithet based on their trait
		this->ExtraName = this->Trait->GetEpithets()[SyncRand(this->Trait->GetEpithets().size())].utf8().get_data();
	}
}

void CUnit::UpdateSettlement()
{
	if (this->Removed || Editor.Running != EditorNotRunning) {
		return;
	}
	
	if (!this->Type->BoolFlag[BUILDING_INDEX].value || this->Type->TerrainType) {
		return;
	}
	
	if (this->Type->BoolFlag[TOWNHALL_INDEX].value || this->Type == SettlementSiteUnitType) {
		if (!this->Settlement) {
			const CCivilization *civilization = this->Type->GetCivilization();
			if (civilization != nullptr && this->Player->GetFaction() != nullptr && (CCivilization::Get(this->Player->Race) == civilization || this->Type == CFaction::GetFactionClassUnitType(this->Player->GetFaction(), this->Type->GetClass()))) {
				civilization = CCivilization::Get(this->Player->Race);
			}
			
			const CFaction *faction = this->Type->GetFaction();
			if (CCivilization::Get(this->Player->Race) == civilization && this->Type == CFaction::GetFactionClassUnitType(this->Player->GetFaction(), this->Type->GetClass())) {
				faction = this->Player->GetFaction();
			}

			std::vector<CSite *> potential_settlements;
			if (civilization) {
				for (CSite *site : civilization->Sites) {
					if (!site->SiteUnit) {
						potential_settlements.push_back(site);
					}
				}
			}
			
			if (potential_settlements.empty() && faction != nullptr) {
				for (CSite *site : faction->Sites) {
					if (!site->SiteUnit) {
						potential_settlements.push_back(site);
					}
				}
			}
			
			if (potential_settlements.empty()) {
				for (CSite *site : CSite::GetAll()) {
					if (!site->SiteUnit) {
						potential_settlements.push_back(site);
					}
				}
			}
			
			if (potential_settlements.size() > 0) {
				this->Settlement = potential_settlements[SyncRand(potential_settlements.size())];
				this->Settlement->SiteUnit = this;
				CMap::Map.SiteUnits.push_back(this);
			}
		}
		if (this->Settlement) {
			this->UpdateBuildingSettlementAssignment();
		}
	} else {
		if (this->Player->GetIndex() == PlayerNumNeutral) {
			return;
		}
		
		this->Settlement = this->Player->GetNearestSettlement(this->GetTilePos(), this->GetMapLayer()->GetIndex(), this->Type->TileSize);
	}
}

void CUnit::UpdateBuildingSettlementAssignment(CSite *old_settlement)
{
	if (Editor.Running != EditorNotRunning) {
		return;
	}
	
	if (this->Player->GetIndex() == PlayerNumNeutral) {
		return;
	}
		
	for (int p = 0; p < PlayerMax; ++p) {
		if (!CPlayer::Players[p]->HasNeutralFactionType() && this->Player->GetIndex() != p) {
			continue;
		}
		for (int i = 0; i < CPlayer::Players[p]->GetUnitCount(); ++i) {
			CUnit *settlement_unit = &CPlayer::Players[p]->GetUnit(i);
			if (!settlement_unit || !settlement_unit->IsAliveOnMap() || !settlement_unit->GetType()->BoolFlag[BUILDING_INDEX].value || settlement_unit->GetType()->BoolFlag[TOWNHALL_INDEX].value || settlement_unit->GetType() == SettlementSiteUnitType || this->MapLayer != settlement_unit->GetMapLayer()) {
				continue;
			}
			if (old_settlement && settlement_unit->Settlement != old_settlement) {
				continue;
			}
			settlement_unit->UpdateSettlement();
		}
	}
}

void CUnit::XPChanged()
{
	if (!this->Type->BoolFlag[ORGANIC_INDEX].value || this->Type->BoolFlag[BUILDING_INDEX].value) {
		return;
	}
	
	if (this->Variable[XPREQUIRED_INDEX].Value == 0) {
		return;
	}
	
	while (this->Variable[XP_INDEX].Value >= this->Variable[XPREQUIRED_INDEX].Value) {
		this->Variable[XP_INDEX].Max -= this->Variable[XPREQUIRED_INDEX].Max;
		this->Variable[XP_INDEX].Value -= this->Variable[XPREQUIRED_INDEX].Value;
		if (this->Player == CPlayer::GetThisPlayer()) {
			this->Player->Notify(NotifyGreen, this->GetTilePos(), this->GetMapLayer()->GetIndex(), _("%s has leveled up!"), GetMessageName().c_str());
		}
		this->IncreaseLevel(1);
	}
	
	if (!IsNetworkGame() && this->Character != nullptr && this->Player->AiEnabled == false) {
		this->Character->ExperiencePercent = (this->Variable[XP_INDEX].Value * 100) / this->Variable[XPREQUIRED_INDEX].Value;
		SaveHero(this->Character);
	}
}
//Wyrmgus end

/**
**  Affect Tile coord of a unit (with units inside) to tile (x, y).
**
**  @param unit  unit to move.
**  @param pos   map tile position.
**
**  @internal before use it, CMap::Map.Remove(unit), MapUnmarkUnitSight(unit)
**  and after CMap::Map.Insert(unit), MapMarkUnitSight(unit)
**  are often necessary. Check Flag also for Pathfinder.
*/
static void UnitInXY(CUnit &unit, const Vec2i &pos, const int z)
{
	const CMapLayer *old_map_layer = unit.GetMapLayer();
	
	CUnit *unit_inside = unit.UnitInside;

	unit.TilePos = pos;
	unit.Offset = CMap::Map.getIndex(pos, z);
	unit.MapLayer = CMap::Map.MapLayers[z];
	
	//Wyrmgus start
	if (!SaveGameLoading && unit.GetMapLayer() != old_map_layer) {
		UpdateUnitSightRange(unit);
	}
	//Wyrmgus end

	for (int i = unit.InsideCount; i--; unit_inside = unit_inside->NextContained) {
		UnitInXY(*unit_inside, pos, z);
	}
	
	if (!unit.Removed) {
		if (unit.MapLayer != old_map_layer) {
			unit.emit_signal("map_layer_changed", unit.GetMapLayer());
		}
		unit.emit_signal("tile_pos_changed", Vector2(unit.GetTilePos()));
	}
}

/**
**  Move a unit (with units inside) to tile (pos).
**  (Do stuff with vision, cachelist and pathfinding).
**
**  @param pos  map tile position.
**
*/
//Wyrmgus start
//void CUnit::MoveToXY(const Vec2i &pos)
void CUnit::MoveToXY(const Vec2i &pos, int z)
//Wyrmgus end
{
	MapUnmarkUnitSight(*this);
	CMap::Map.Remove(*this);
	UnmarkUnitFieldFlags(*this);

	//Wyrmgus start
//	Assert(UnitCanBeAt(*this, pos));
	Assert(UnitCanBeAt(*this, pos, z));
	//Wyrmgus end
	// Move the unit.
	//Wyrmgus start
//	UnitInXY(*this, pos);
	UnitInXY(*this, pos, z);
	//Wyrmgus end

	CMap::Map.Insert(*this);
	MarkUnitFieldFlags(*this);
	//  Recalculate the seen count.
	UnitCountSeen(*this);
	MapMarkUnitSight(*this);
	
	//Wyrmgus start
	// if there is a trap in the new tile, trigger it
	if ((this->Type->UnitType != UnitTypeFly && this->Type->UnitType != UnitTypeFlyLow) || !this->Type->BoolFlag[ORGANIC_INDEX].value) {
		const CUnitCache &cache = CMap::Map.Field(pos, z)->UnitCache;
		for (size_t i = 0; i != cache.size(); ++i) {
			if (!cache[i]) {
				fprintf(stderr, "Error in CUnit::MoveToXY (pos %d, %d): a unit in the tile's unit cache is null.\n", pos.x, pos.y);
			}
			CUnit &unit = *cache[i];
			if (unit.IsAliveOnMap() && unit.GetType()->BoolFlag[TRAP_INDEX].value) {
				FireMissile(unit, this, this->GetTilePos(), this->GetMapLayer()->GetIndex());
				LetUnitDie(unit);
			}
		}
	}
	//Wyrmgus end
}

/**
**  Place unit on map.
**
**  @param pos  map tile position.
*/
void CUnit::Place(const Vec2i &pos, int z)
{
	Assert(Removed);
	
	const CMapLayer *old_map_layer = this->MapLayer;

	if (Container) {
		MapUnmarkUnitSight(*this);
		RemoveUnitFromContainer(*this);
	}
	if (!SaveGameLoading) {
		UpdateUnitSightRange(*this);
	}
	Removed = 0;
	UnitInXY(*this, pos, z);
	// Pathfinding info.
	MarkUnitFieldFlags(*this);
	// Tha cache list.
	CMap::Map.Insert(*this);
	//  Calculate the seen count.
	UnitCountSeen(*this);
	// Vision
	MapMarkUnitSight(*this);

	// Correct directions for wall units
	if (this->Type->BoolFlag[WALL_INDEX].value && this->CurrentAction() != UnitActionBuilt) {
		CorrectWallDirections(*this);
		UnitUpdateHeading(*this);
		CorrectWallNeighBours(*this);
	}

	//Wyrmgus start
	if (this->IsAlive()) {
		if (this->Type->BoolFlag[BUILDING_INDEX].value) {
			this->UpdateSettlement(); // update the settlement name of a building when placing it
		}
		
		//remove pathways, destroyed walls and decoration units under buildings
		if (this->Type->BoolFlag[BUILDING_INDEX].value && !this->Type->TerrainType) {
			for (int x = this->GetTilePos().x; x < this->GetTilePos().x + this->Type->TileSize.x; ++x) {
				for (int y = this->GetTilePos().y; y < this->GetTilePos().y + this->Type->TileSize.y; ++y) {
					if (!CMap::Map.Info.IsPointOnMap(x, y, this->MapLayer)) {
						continue;
					}
					Vec2i building_tile_pos(x, y);
					CMapField &mf = *this->GetMapLayer()->Field(building_tile_pos);
					if ((mf.GetFlags() & MapFieldRoad) || (mf.GetFlags() & MapFieldRailroad) || (mf.GetFlags() & MapFieldWall)) {
						CMap::Map.RemoveTileOverlayTerrain(building_tile_pos, this->GetMapLayer()->GetIndex());
					}
					//remove decorations if a building has been built here
					std::vector<CUnit *> table;
					Select(building_tile_pos, building_tile_pos, table, this->GetMapLayer()->GetIndex());
					for (size_t i = 0; i != table.size(); ++i) {
						if (table[i] && table[i]->IsAlive() && table[i]->GetType()->UnitType == UnitTypeLand && table[i]->GetType()->BoolFlag[DECORATION_INDEX].value) {
							if (Editor.Running == EditorNotRunning) {
								LetUnitDie(*table[i]);			
							} else {
								EditorActionRemoveUnit(*table[i], false);
							}
						}
					}
				}
			}
		}
		
		const UnitTypeVariation *variation = this->GetVariation();
		if (variation) {
			// if a unit that is on the tile has a terrain-dependent or season-dependent variation that is not compatible with the new tile, or if this is the first position the unit is being placed in, repick the unit's variation
			if (!old_map_layer || !this->CheckTerrainForVariation(variation) || !this->CheckSeasonForVariation(variation)) {
				this->ChooseVariation();
			}
		}
	}
	//Wyrmgus end
	
	Wyrmgus::GetInstance()->emit_signal("unit_placed", this);
}

/**
**  Create new unit and place on map.
**
**  @param pos     map tile position.
**  @param type    Pointer to unit-type.
**  @param player  Pointer to owning player.
**
**  @return        Pointer to created unit.
*/
//Wyrmgus start
//CUnit *MakeUnitAndPlace(const Vec2i &pos, const CUnitType &type, CPlayer *player)
CUnit *MakeUnitAndPlace(const Vec2i &pos, const CUnitType &type, CPlayer *player, int z)
//Wyrmgus end
{
	CUnit *unit = MakeUnit(type, player);

	if (unit != nullptr) {
		unit->Place(pos, z);
	}
	return unit;
}

//Wyrmgus start
/**
**  Create a new unit and place it on the map, updating its player accordingly.
**
**  @param pos     map tile position.
**  @param type    Pointer to unit-type.
**  @param player  Pointer to owning player.
**
**  @return        Pointer to created unit.
*/
CUnit *CreateUnit(const Vec2i &pos, const CUnitType &type, CPlayer *player, int z, bool no_bordering_building)
{
	Vec2i res_pos;
	const int heading = SyncRand() % 256;
	FindNearestDrop(type, pos, res_pos, heading, z, no_bordering_building);
	if (!CMap::Map.Info.IsPointOnMap(res_pos, z)) {
		return nullptr;
	}
	
	CUnit *unit = MakeUnit(type, player);

	if (unit != nullptr) {
		if (type.BoolFlag[BUILDING_INDEX].value) {
			CBuildRestrictionOnTop *b = OnTopDetails(type, nullptr);
			if (b && b->ReplaceOnBuild) {
				CUnitCache &unitCache = CMap::Map.Field(res_pos, z)->UnitCache;
				CUnitCache::iterator it = std::find_if(unitCache.begin(), unitCache.end(), HasSameTypeAs(*b->Parent));

				if (it != unitCache.end()) {
					CUnit &replacedUnit = **it;
					unit->ReplaceOnTop(replacedUnit);
				}
			}
		}
		
		unit->Place(res_pos, z);
		UpdateForNewUnit(*unit, 0);
	}
	return unit;
}

CUnit *CreateResourceUnit(const Vec2i &pos, const CUnitType &type, int z, bool allow_unique)
{
	CUnit *unit = CreateUnit(pos, type, CPlayer::Players[PlayerNumNeutral], z, true);
	unit->GenerateSpecialProperties(nullptr, nullptr, allow_unique);
			
	// create metal rocks near metal resources
	CUnitType *metal_rock_type = nullptr;
	if (type.Ident == "unit-gold-deposit") {
		metal_rock_type = CUnitType::Get("unit-gold-rock");
	} else if (type.Ident == "unit-silver-deposit") {
		metal_rock_type = CUnitType::Get("unit-silver-rock");
	} else if (type.Ident == "unit-copper-deposit") {
		metal_rock_type = CUnitType::Get("unit-copper-rock");
	} else if (type.Ident == "unit-diamond-deposit") {
		metal_rock_type = CUnitType::Get("unit-diamond-rock");
	} else if (type.Ident == "unit-emerald-deposit") {
		metal_rock_type = CUnitType::Get("unit-emerald-rock");
	}
	if (metal_rock_type) {
		Vec2i metal_rock_offset((type.TileSize - 1) / 2);
		for (int i = 0; i < 9; ++i) {
			CUnit *metal_rock_unit = CreateUnit(unit->GetTilePos() + metal_rock_offset, *metal_rock_type, CPlayer::Players[PlayerNumNeutral], z);
		}
	}
			
	return unit;
}
//Wyrmgus end

/**
**  Find the nearest position at which unit can be placed.
**
**  @param type     Type of the dropped unit.
**  @param goalPos  Goal map tile position.
**  @param resPos   Holds the nearest point.
**  @param heading  preferense side to drop out of.
*/
//Wyrmgus start
//void FindNearestDrop(const CUnitType &type, const Vec2i &goalPos, Vec2i &resPos, int heading)
void FindNearestDrop(const CUnitType &type, const Vec2i &goalPos, Vec2i &resPos, int heading, int z, bool no_bordering_building, bool ignore_construction_requirements)
//Wyrmgus end
{
	int addx = 0;
	int addy = 0;
	Vec2i pos = goalPos;
	int directions_outside_the_map = 0;

	if (heading < LookingNE || heading > LookingNW) {
		goto starts;
	} else if (heading < LookingSE) {
		goto startw;
	} else if (heading < LookingSW) {
		goto startn;
	} else {
		goto starte;
	}

	for (;;) {
		directions_outside_the_map = 0;
startw:
		for (int i = addy; i--; ++pos.y) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, nullptr) && !ignore_construction_requirements))
				&& (!type.BoolFlag[BUILDING_INDEX].value || ignore_construction_requirements || CanBuildHere(nullptr, type, pos, z, no_bordering_building) != nullptr)
				&& CMap::Map.GetTileTopTerrain(pos, false, z) != nullptr
			) {
			//Wyrmgus end
				goto found;
			}
		}
		if (pos.y >= CMap::Map.MapLayers[z]->GetHeight()) {
			directions_outside_the_map++;
		}
		++addx;
starts:
		for (int i = addx; i--; ++pos.x) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, nullptr) && !ignore_construction_requirements))
				&& (!type.BoolFlag[BUILDING_INDEX].value || ignore_construction_requirements || CanBuildHere(nullptr, type, pos, z, no_bordering_building) != nullptr)
				&& CMap::Map.GetTileTopTerrain(pos, false, z) != nullptr
			) {
			//Wyrmgus end
				goto found;
			}
		}
		if (pos.x >= CMap::Map.MapLayers[z]->GetWidth()) {
			directions_outside_the_map++;
		}
		++addy;
starte:
		for (int i = addy; i--; --pos.y) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, nullptr) && !ignore_construction_requirements))
				&& (!type.BoolFlag[BUILDING_INDEX].value || ignore_construction_requirements || CanBuildHere(nullptr, type, pos, z, no_bordering_building) != nullptr)
				&& CMap::Map.GetTileTopTerrain(pos, false, z) != nullptr
			) {
			//Wyrmgus end
				goto found;
			}
		}
		if (pos.y < 0) {
			directions_outside_the_map++;
		}
		++addx;
startn:
		for (int i = addx; i--; --pos.x) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, nullptr) && !ignore_construction_requirements))
				&& (!type.BoolFlag[BUILDING_INDEX].value || ignore_construction_requirements || CanBuildHere(nullptr, type, pos, z, no_bordering_building) != nullptr)
				&& CMap::Map.GetTileTopTerrain(pos, false, z) != nullptr
			) {
			//Wyrmgus end
				goto found;
			}
		}
		if (pos.x < 0) {
			directions_outside_the_map++;
		}
		++addy;
		
		if (directions_outside_the_map == 4) {
			fprintf(stderr, "Could not find valid position on the map for unit type \"%s\".\n", type.Ident.c_str());
			resPos = Vec2i(-1, -1);
			return;
		}
	}

found:
	resPos = pos;
}

/**
**  Remove unit from map.
**
**  Update selection.
**  Update panels.
**  Update map.
**
**  @param host  Pointer to housing unit.
*/
void CUnit::Remove(CUnit *host)
{
	if (this->Removed) { // could happen!
		// If unit is removed (inside) and building is destroyed.
		DebugPrint("unit '%s(%d)' already removed\n" _C_ Type->Ident.c_str() _C_ UnitNumber(*this));
		return;
	}
	
	this->emit_signal("removed");
	
	CMap::Map.Remove(*this);
	MapUnmarkUnitSight(*this);
	UnmarkUnitFieldFlags(*this);
	if (host) {
		AddInContainer(*host);
		UpdateUnitSightRange(*this);
		UnitInXY(*this, host->GetTilePos(), host->GetMapLayer()->GetIndex());
		MapMarkUnitSight(*this);
	}

	Removed = 1;

	// Correct surrounding walls directions
	if (this->Type->BoolFlag[WALL_INDEX].value) {
		CorrectWallNeighBours(*this);
	}

	//  Remove unit from the current selection
	if (this->IsSelected()) {
		if (::Selected.size() == 1) { //  Remove building cursor
			CancelBuildingMode();
		}
		UnSelectUnit(*this);
		//Wyrmgus start
//		SelectionChanged();
		if (GameRunning) { // to avoid a crash when SelectionChanged() calls UI.ButtonPanel.Update()
			SelectionChanged();
		}
		//Wyrmgus end
	}
	// Remove unit from team selections
	if (!Selected && TeamSelected) {
		UnSelectUnit(*this);
	}

	// Unit is seen as under cursor
	if (UnitUnderCursor == this) {
		UnitUnderCursor = nullptr;
	}
}

/**
**  Update information for lost units.
**
**  @param unit  Pointer to unit.
**
**  @note Also called by ChangeUnitOwner
*/
void UnitLost(CUnit &unit)
{
	CPlayer &player = *unit.GetPlayer();

	Assert(&player);  // Next code didn't support no player!

	//  Call back to AI, for killed or lost units.
	if (Editor.Running == EditorNotRunning) {
		if (player.AiEnabled) {
			AiUnitKilled(unit);
		} else {
			//  Remove unit from its groups
			if (unit.GroupId) {
				RemoveUnitFromGroups(unit);
			}
		}
	}

	//  Remove the unit from the player's units table.

	const CUnitType &type = *unit.GetType();
	if (!type.BoolFlag[VANISHES_INDEX].value) {
		player.RemoveUnit(unit);

		if (type.BoolFlag[BUILDING_INDEX].value) {
			// FIXME: support more races
			//Wyrmgus start
//			if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
			//Wyrmgus end
				player.NumBuildings--;
				//Wyrmgus start
				if (unit.CurrentAction() == UnitActionBuilt) {
					player.NumBuildingsUnderConstruction--;
					player.ChangeUnitTypeUnderConstructionCount(&type, -1);
				}
				//Wyrmgus end
			//Wyrmgus start
//			}
			//Wyrmgus end
		}
		if (unit.CurrentAction() != UnitActionBuilt) {
			if (player.AiEnabled && player.Ai) {
				if (std::find(player.Ai->Scouts.begin(), player.Ai->Scouts.end(), &unit) != player.Ai->Scouts.end()) {
					if (player.Ai->Scouting) { //if an AI player's scout has been lost, unmark it as "scouting" so that the force can see if it now has a viable target
						player.Ai->Scouting = false;
					}
				}
				for (std::map<int, std::vector<CUnit *>>::iterator iterator = player.Ai->Transporters.begin(); iterator != player.Ai->Transporters.end(); ++iterator) {
					if (std::find(iterator->second.begin(), iterator->second.end(), &unit) != iterator->second.end()) {
						iterator->second.erase(std::remove(iterator->second.begin(), iterator->second.end(), &unit), iterator->second.end());
					}
				}
			}
			
			player.DecreaseCountsForUnit(&unit);
			
			if (unit.Variable[LEVELUP_INDEX].Value > 0) {
				player.UpdateLevelUpUnits(); //recalculate level-up units, since this unit no longer should be in that vector
			}
			//Wyrmgus end
		}
	}

	//  Handle unit demand. (Currently only food supported.)
	player.Demand -= type.Stats[player.GetIndex()].Variables[DEMAND_INDEX].Value;

	//  Update information.
	if (unit.CurrentAction() != UnitActionBuilt) {
		player.Supply -= unit.Variable[SUPPLY_INDEX].Value;
		// Decrease resource limit
		for (int i = 0; i < MaxCosts; ++i) {
			if (player.MaxResources[i] != -1 && type.Stats[player.GetIndex()].Storing[i]) {
				const int newMaxValue = player.MaxResources[i] - type.Stats[player.GetIndex()].Storing[i];

				player.MaxResources[i] = std::max(0, newMaxValue);
				player.SetResource(i, player.StoredResources[i], STORE_BUILDING);
			}
		}
		//  Handle income improvements, look if a player loses a building
		//  which have given him a better income, find the next best
		//  income.
		for (int i = 1; i < MaxCosts; ++i) {
			if (player.Incomes[i] && type.Stats[player.GetIndex()].ImproveIncomes[i] == player.Incomes[i]) {
				int m = CResource::GetAll()[i]->DefaultIncome;

				for (int j = 0; j < player.GetUnitCount(); ++j) {
					m = std::max(m, player.GetUnit(j).GetType()->Stats[player.GetIndex()].ImproveIncomes[i]);
				}
				player.Incomes[i] = m;
			}
		}
		
		if (type.Stats[player.GetIndex()].Variables[TRADECOST_INDEX].Enable) {
			int m = DEFAULT_TRADE_COST;

			for (int j = 0; j < player.GetUnitCount(); ++j) {
				if (player.GetUnit(j).GetType()->Stats[player.GetIndex()].Variables[TRADECOST_INDEX].Enable) {
					m = std::min(m, player.GetUnit(j).GetType()->Stats[player.GetIndex()].Variables[TRADECOST_INDEX].Value);
				}
			}
			player.TradeCost = m;
		}
		
		//Wyrmgus start
		if (type.BoolFlag[TOWNHALL_INDEX].value) {
			bool lost_town_hall = true;
			for (int j = 0; j < player.GetUnitCount(); ++j) {
				if (player.GetUnit(j).GetType()->BoolFlag[TOWNHALL_INDEX].value) {
					lost_town_hall = false;
				}
			}
			if (lost_town_hall && CPlayer::GetThisPlayer()->HasContactWith(player)) {
				player.LostTownHallTimer = GameCycle + (30 * CYCLES_PER_SECOND); //30 seconds until being revealed
				for (int j = 0; j < NumPlayers; ++j) {
					if (player.GetIndex() != j && CPlayer::Players[j]->Type != PlayerNobody) {
						CPlayer::Players[j]->Notify(_("%s has lost their last town hall, and will be revealed in thirty seconds!"), player.Name.c_str());
					} else {
						CPlayer::Players[j]->Notify("%s", _("You have lost your last town hall, and will be revealed in thirty seconds!"));
					}
				}
			}
		}
		//Wyrmgus end
	}

	//  Handle order cancels.
	unit.CurrentOrder()->Cancel(unit);

	DebugPrint("%d: Lost %s(%d)\n" _C_ player.GetIndex() _C_ type.Ident.c_str() _C_ UnitNumber(unit));

	// Destroy resource-platform, must re-make resource patch.
	//Wyrmgus start
//	CBuildRestrictionOnTop *b = OnTopDetails(unit, nullptr);
	CBuildRestrictionOnTop *b = OnTopDetails(*unit.GetType(), nullptr);
	//Wyrmgus end
	if (b != nullptr) {
		if (b->ReplaceOnDie && (!type.GivesResource || unit.GetResourcesHeld() != 0)) {
			CUnit *temp = MakeUnitAndPlace(unit.GetTilePos(), *b->Parent, CPlayer::Players[PlayerNumNeutral], unit.GetMapLayer()->GetIndex());
			if (temp == nullptr) {
				DebugPrint("Unable to allocate Unit");
			} else {
				//Wyrmgus start
//				temp->ResourcesHeld = unit.GetResourcesHeld();
//				temp->Variable[GIVERESOURCE_INDEX].Value = unit.Variable[GIVERESOURCE_INDEX].Value;
//				temp->Variable[GIVERESOURCE_INDEX].Max = unit.Variable[GIVERESOURCE_INDEX].Max;
//				temp->Variable[GIVERESOURCE_INDEX].Enable = unit.Variable[GIVERESOURCE_INDEX].Enable;
				//Wyrmgus end
				//Wyrmgus start
				if (unit.Unique != nullptr) {
					temp->SetUnique(unit.Unique);
				} else {
					if (unit.Prefix != nullptr) {
						temp->SetPrefix(unit.Prefix);
					}
					if (unit.Suffix != nullptr) {
						temp->SetSuffix(unit.Suffix);
					}
					if (unit.Spell != nullptr) {
						temp->SetSpell(unit.Spell);
					}
				}
				if (unit.Settlement != nullptr) {
					if (unit.GetType()->BoolFlag[TOWNHALL_INDEX].value) {
						temp->Settlement = unit.Settlement;
						temp->Settlement->SiteUnit = temp;
						CMap::Map.SiteUnits.erase(std::remove(CMap::Map.SiteUnits.begin(), CMap::Map.SiteUnits.end(), &unit), CMap::Map.SiteUnits.end());
						CMap::Map.SiteUnits.push_back(temp);
					}
				}
				if (type.GivesResource && unit.GetResourcesHeld() != 0) {
					temp->SetResourcesHeld(unit.GetResourcesHeld());
					temp->Variable[GIVERESOURCE_INDEX].Value = unit.Variable[GIVERESOURCE_INDEX].Value;
					temp->Variable[GIVERESOURCE_INDEX].Max = unit.Variable[GIVERESOURCE_INDEX].Max;
					temp->Variable[GIVERESOURCE_INDEX].Enable = unit.Variable[GIVERESOURCE_INDEX].Enable;
				}
				//Wyrmgus end
			}
		//Wyrmgus start
		} else if (unit.Settlement && unit.Settlement->SiteUnit == &unit) {
			unit.Settlement->SiteUnit = nullptr;
		//Wyrmgus end
		}
	}
}

/**
**  Removes all orders from a unit.
**
**  @param unit  The unit that will have all its orders cleared
*/
void UnitClearOrders(CUnit &unit)
{
	for (size_t i = 0; i != unit.Orders.size(); ++i) {
		delete unit.Orders[i];
	}
	unit.Orders.clear();
	unit.Orders.push_back(COrder::NewActionStill());
}

/**
**  Update for new unit. Food and income ...
**
**  @param unit     New unit pointer.
**  @param upgrade  True unit was upgraded.
*/
void UpdateForNewUnit(const CUnit &unit, int upgrade)
{
	const CUnitType &type = *unit.GetType();
	CPlayer &player = *unit.GetPlayer();

	// Handle unit supply and max resources.
	// Note an upgraded unit can't give more supply.
	if (!upgrade) {
		player.Supply += unit.Variable[SUPPLY_INDEX].Value;
		for (int i = 0; i < MaxCosts; ++i) {
			if (player.MaxResources[i] != -1 && type.Stats[player.GetIndex()].Storing[i]) {
				player.MaxResources[i] += type.Stats[player.GetIndex()].Storing[i];
			}
		}
	}

	// Update resources
	for (int u = 1; u < MaxCosts; ++u) {
		player.Incomes[u] = std::max(player.Incomes[u], type.Stats[player.GetIndex()].ImproveIncomes[u]);
	}
	
	if (type.Stats[player.GetIndex()].Variables[TRADECOST_INDEX].Enable) {
		player.TradeCost = std::min(player.TradeCost, type.Stats[player.GetIndex()].Variables[TRADECOST_INDEX].Value);
	}
	
	//Wyrmgus start
	if (player.LostTownHallTimer != 0 && type.BoolFlag[TOWNHALL_INDEX].value && CPlayer::GetThisPlayer()->HasContactWith(player)) {
		player.LostTownHallTimer = 0;
		player.Revealed = false;
		for (int j = 0; j < NumPlayers; ++j) {
			if (player.GetIndex() != j && CPlayer::Players[j]->Type != PlayerNobody) {
				CPlayer::Players[j]->Notify(_("%s has rebuilt a town hall, and will no longer be revealed!"), player.Name.c_str());
			} else {
				CPlayer::Players[j]->Notify("%s", _("You have rebuilt a town hall, and will no longer be revealed!"));
			}
		}
	}
	//Wyrmgus end
}

/**
**  Find nearest point of unit.
**
**  @param unit  Pointer to unit.
**  @param pos   tile map position.
**  @param dpos  Out: nearest point tile map position to (tx,ty).
*/
void NearestOfUnit(const CUnit &unit, const Vec2i &pos, Vec2i *dpos)
{
	const int x = unit.GetTilePos().x;
	const int y = unit.GetTilePos().y;

	*dpos = pos;
	dpos->x = std::clamp<short int>(dpos->x, x, x + unit.GetType()->TileSize.x - 1);
	dpos->y = std::clamp<short int>(dpos->y, y, y + unit.GetType()->TileSize.y - 1);
}

/**
**  Copy the unit look in Seen variables. This should be called when
**  buildings go under fog of war for ThisPlayer.
**
**  @param unit  The unit to work on
*/
static void UnitFillSeenValues(CUnit &unit)
{
	// Seen values are undefined for visible units.
	unit.Seen.TilePos = unit.GetTilePos();
	unit.Seen.IY = unit.GetPixelOffset().y;
	unit.Seen.IX = unit.GetPixelOffset().x;
	unit.Seen.Frame = unit.GetFrame();
	unit.Seen.Type = unit.GetType();
	unit.Seen.UnderConstruction = unit.UnderConstruction;

	unit.CurrentOrder()->FillSeenValues(unit);
}

// Wall unit positions
enum {
	W_NORTH = 0x10,
	W_WEST = 0x20,
	W_SOUTH = 0x40,
	W_EAST = 0x80
};

/**
**  Correct direction for placed wall.
**
**  @param unit    The wall unit.
*/
void CorrectWallDirections(CUnit &unit)
{
	Assert(unit.GetType()->BoolFlag[WALL_INDEX].value);
	Assert(unit.GetType()->NumDirections == 16);
	Assert(!unit.GetType()->Flip);

	if (!CMap::Map.Info.IsPointOnMap(unit.GetTilePos(), unit.GetMapLayer())) {
		return;
	}
	const struct {
		Vec2i offset;
		const int dirFlag;
	} configs[] = {{Vec2i(0, -1), W_NORTH}, {Vec2i(1, 0), W_EAST},
		{Vec2i(0, 1), W_SOUTH}, {Vec2i(-1, 0), W_WEST}
	};
	int flags = 0;

	for (int i = 0; i != sizeof(configs) / sizeof(*configs); ++i) {
		const Vec2i pos = unit.GetTilePos() + configs[i].offset;
		const int dirFlag = configs[i].dirFlag;

		if (CMap::Map.Info.IsPointOnMap(pos, unit.GetMapLayer()) == false) {
			flags |= dirFlag;
		} else {
			const CUnitCache &unitCache = CMap::Map.Field(pos, unit.GetMapLayer()->GetIndex())->UnitCache;
			const CUnit *neighbor = unitCache.find(HasSamePlayerAndTypeAs(unit));

			if (neighbor != nullptr) {
				flags |= dirFlag;
			}
		}
	}
	unit.Direction = flags;
}

/**
** Correct the surrounding walls.
**
** @param unit The wall unit.
*/
void CorrectWallNeighBours(CUnit &unit)
{
	Assert(unit.GetType()->BoolFlag[WALL_INDEX].value);

	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1)};

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		const Vec2i pos = unit.GetTilePos() + offset[i];

		if (CMap::Map.Info.IsPointOnMap(pos, unit.GetMapLayer()) == false) {
			continue;
		}
		CUnitCache &unitCache = unit.GetMapLayer()->Field(pos)->UnitCache;
		CUnit *neighbor = unitCache.find(HasSamePlayerAndTypeAs(unit));

		if (neighbor != nullptr) {
			CorrectWallDirections(*neighbor);
			UnitUpdateHeading(*neighbor);
		}
	}
}

/**
**  This function should get called when a unit goes under fog of war.
**
**  @param unit    The unit that goes under fog.
**  @param player  The player the unit goes out of fog for.
*/
void UnitGoesUnderFog(CUnit &unit, const CPlayer &player)
{
	if (unit.GetType()->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
		if (player.Type == PlayerPerson && !unit.Destroyed) {
			unit.RefsIncrease();
		}
		//
		// Icky yucky icky Seen.Destroyed trickery.
		// We track for each player if he's seen the unit as destroyed.
		// Remember, a unit is marked Destroyed when it's gone as in
		// completely gone, the corpses vanishes. In that case the unit
		// only survives since some players did NOT see the unit destroyed.
		// Keeping trackof that is hard, mostly due to complex shared vision
		// configurations.
		// A unit does NOT get a reference when it goes under fog if it's
		// Destroyed. Furthermore, it shouldn't lose a reference if it was
		// Seen destroyed. That only happened with complex shared vision, and
		// it's sort of the whole point of this tracking.
		//
		if (unit.Destroyed) {
			unit.Seen.Destroyed |= (1 << player.GetIndex());
		}
		if (&player == CPlayer::GetThisPlayer()) {
			UnitFillSeenValues(unit);
		}
	}
}

/**
**  This function should get called when a unit goes out of fog of war.
**
**  @param unit    The unit that goes out of fog.
**  @param player  The player the unit goes out of fog for.
**
**  @note For units that are visible under fog (mostly buildings)
**  we use reference counts, from the players that know about
**  the building. When an building goes under fog it gets a refs
**  increase, and when it shows up it gets a decrease. It must
**  not get an decrease the first time it's seen, so we have to
**  keep track of what player saw what units, with SeenByPlayer.
*/
void UnitGoesOutOfFog(CUnit &unit, const CPlayer &player)
{
	if (!unit.GetType()->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
		return;
	}
	if (unit.Seen.ByPlayer & (1 << (player.GetIndex()))) {
		if ((player.Type == PlayerPerson) && (!(unit.Seen.Destroyed & (1 << player.GetIndex())))) {
			unit.RefsDecrease();
		}
	} else {
		unit.Seen.ByPlayer |= (1 << (player.GetIndex()));
	}
}

/**
**  Recalculates a units visiblity count. This happens really often,
**  Like every time a unit moves. It's really fast though, since we
**  have per-tile counts.
**
**  @param unit  pointer to the unit to check if seen
*/
void UnitCountSeen(CUnit &unit)
{
	Assert(unit.GetType());

	// FIXME: optimize, only work on certain players?
	// This is for instance good for updating shared vision...

	//  Store old values in oldv[p]. This store if the player could see the
	//  unit before this calc.
	int oldv[PlayerMax];
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->Type != PlayerNobody) {
			oldv[p] = unit.IsVisible(*CPlayer::Players[p]);
		}
	}

	//  Calculate new VisCount values.
	const int height = unit.GetType()->TileSize.y;
	const int width = unit.GetType()->TileSize.x;

	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->Type != PlayerNobody) {
			int newv = 0;
			int y = height;
			unsigned int index = unit.Offset;
			do {
				CMapField *mf = unit.GetMapLayer()->Field(index);
				int x = width;
				do {
					if (unit.GetType()->BoolFlag[PERMANENTCLOAK_INDEX].value && unit.GetPlayer() != CPlayer::Players[p]) {
						if (mf->playerInfo.VisCloak[p]) {
							newv++;
						}
					//Wyrmgus start
					} else if (unit.GetType()->BoolFlag[ETHEREAL_INDEX].value && unit.GetPlayer() != CPlayer::Players[p]) {
						if (mf->playerInfo.VisEthereal[p]) {
							newv++;
						}
					//Wyrmgus end
					} else {
						if (mf->playerInfo.IsVisible(*CPlayer::Players[p])) {
							newv++;
						}
					}
					++mf;
				} while (--x);
				index += unit.GetMapLayer()->GetWidth();
			} while (--y);
			unit.VisCount[p] = newv;
		}
	}

	//
	// Now here comes the tricky part. We have to go in and out of fog
	// for players. Hopefully this works with shared vision just great.
	//
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->Type != PlayerNobody) {
			int newv = unit.IsVisible(*CPlayer::Players[p]);
			if (!oldv[p] && newv) {
				// Might have revealed a destroyed unit which caused it to
				// be released
				if (!unit.GetType()) {
					break;
				}
				UnitGoesOutOfFog(unit, *CPlayer::Players[p]);
			}
			if (oldv[p] && !newv) {
				UnitGoesUnderFog(unit, *CPlayer::Players[p]);
			}
		}
	}
}

/**
**  Returns true, if the unit is visible. It check the Viscount of
**  the player and everyone who shares vision with him.
**
**  @note This understands shared vision, and should be used all around.
**
**  @param player  The player to check.
*/
bool CUnit::IsVisible(const CPlayer &player) const
{
	if (VisCount[player.GetIndex()]) {
		return true;
	}
	for (int p = 0; p < PlayerMax; ++p) {
		if (p != player.GetIndex() && (player.IsBothSharedVision(*CPlayer::Players[p]) || CPlayer::Players[p]->Revealed)) {
			if (VisCount[p]) {
				return true;
			}
		}
	}
	return false;
}

/**
**  Returns true, if unit is shown on minimap.
**
**  @warning This is for ::ThisPlayer only.
**  @todo radar support
**
**  @return      True if visible, false otherwise.
*/
bool CUnit::IsVisibleOnMinimap() const
{
	if (this->GetMapLayer() != UI.CurrentMapLayer) {
		return false;
	}

	// Invisible units.
	if (IsInvisibile(*CPlayer::GetThisPlayer())) {
		return false;
	}
	if (IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap || IsVisibleOnRadar(*CPlayer::GetThisPlayer())) {
		return IsAliveOnMap();
	} else {
		return Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value && Seen.State != 3
			   && (Seen.ByPlayer & (1 << CPlayer::GetThisPlayer()->GetIndex()))
			   //Wyrmgus start
//			   && !(Seen.Destroyed & (1 << CPlayer::GetThisPlayer()->GetIndex()));
			   && !(Seen.Destroyed & (1 << CPlayer::GetThisPlayer()->GetIndex()))
			   && !Destroyed
			   && CMap::Map.Info.IsPointOnMap(this->GetTilePos(), this->MapLayer)
			   && this->GetMapLayer()->Field(this->GetTilePos())->playerInfo.IsTeamExplored(*CPlayer::GetThisPlayer());
			   //Wyrmgus end
	}
}

/**
**  Returns true, if unit is visible in viewport.
**
**  @warning This is only true for ::ThisPlayer
**
**  @param vp  Viewport pointer.
**
**  @return    True if visible, false otherwise.
*/
bool CUnit::IsVisibleInViewport(const CViewport &vp) const
{
	// Check if the graphic is inside the viewport.
	//Wyrmgus start
//	int x = tilePos.x * CMap::Map.GetMapLayerPixelTileSize(this->MapLayer).x + this->GetPixelOffset().x - (Type->Width - Type->TileSize.x * CMap::Map.GetMapLayerPixelTileSize(this->MapLayer).x) / 2 + this->Type->GetOffsetX();
//	int y = tilePos.y * CMap::Map.GetMapLayerPixelTileSize(this->MapLayer).y + this->GetPixelOffset().y - (Type->Height - Type->TileSize.y * CMap::Map.GetMapLayerPixelTileSize(this->MapLayer).y) / 2 + this->Type->GetOffsetY();

	int frame_width = this->GetType()->GetFrameSize().width;
	int frame_height = this->GetType()->GetFrameSize().height;
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && variation->GetFrameSize().width != 0 && variation->GetFrameSize().height != 0) {
		frame_width = variation->GetFrameSize().width;
		frame_height = variation->GetFrameSize().height;
	}

	int x = this->TilePos.x * CMap::Map.GetMapLayerPixelTileSize(this->GetMapLayer()->GetIndex()).x + this->GetPixelOffset().x - (frame_width - this->Type->TileSize.x * CMap::Map.GetMapLayerPixelTileSize(this->GetMapLayer()->GetIndex()).x) / 2 + this->Type->GetOffsetX();
	int y = this->TilePos.y * CMap::Map.GetMapLayerPixelTileSize(this->GetMapLayer()->GetIndex()).y + this->GetPixelOffset().y - (frame_height - this->Type->TileSize.y * CMap::Map.GetMapLayerPixelTileSize(this->GetMapLayer()->GetIndex()).y) / 2 + this->Type->GetOffsetY();
	//Wyrmgus end
	const PixelSize vpSize = vp.GetPixelSize();
	const PixelPos vpTopLeftMapPos = CMap::Map.TilePosToMapPixelPos_TopLeft(vp.MapPos, UI.CurrentMapLayer) + vp.Offset;
	const PixelPos vpBottomRightMapPos = vpTopLeftMapPos + vpSize;

	if (x + frame_width < vpTopLeftMapPos.x || x > vpBottomRightMapPos.x
		|| y + frame_height < vpTopLeftMapPos.y || y > vpBottomRightMapPos.y) {
		return false;
	}

	if (!CPlayer::GetThisPlayer()) {
		//FIXME: ARI: Added here for early game setup state by
		// MakeAndPlaceUnit() from LoadMap(). ThisPlayer not yet set,
		// so don't show anything until first real map-draw.
		DebugPrint("FIXME: ThisPlayer not set yet?!\n");
		return false;
	}

	// Those are never ever visible.
	if (IsInvisibile(*CPlayer::GetThisPlayer())) {
		return false;
	}

	if (IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap) {
		return !Destroyed;
	} else {
		// Unit has to be 'discovered'
		// Destroyed units ARE visible under fog of war, if we haven't seen them like that.
		if (!Destroyed || !(Seen.Destroyed & (1 << CPlayer::GetThisPlayer()->GetIndex()))) {
			return (Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value && (Seen.ByPlayer & (1 << CPlayer::GetThisPlayer()->GetIndex())));
		} else {
			return false;
		}
	}
}

/**
**  Change the unit's owner
**
**  @param newplayer  New owning player.
*/
//Wyrmgus start
//void CUnit::ChangeOwner(CPlayer &newplayer)
void CUnit::ChangeOwner(CPlayer &newplayer, bool show_change)
//Wyrmgus end
{
	CPlayer *oldplayer = Player;

	// This shouldn't happen
	if (oldplayer == &newplayer) {
		DebugPrint("Change the unit owner to the same player???\n");
		return;
	}

	// Can't change owner for dead units
	if (this->IsAlive() == false) {
		return;
	}

	// Rescue all units in buildings/transporters.
	//Wyrmgus start
//	CUnit *uins = UnitInside;
//	for (int i = InsideCount; i; --i, uins = uins->NextContained) {
//		uins->ChangeOwner(newplayer);
//	}

	//only rescue units inside if the player is actually a rescuable player (to avoid, for example, unintended worker owner changes when a depot changes hands)
	if (oldplayer->Type == PlayerRescueActive || oldplayer->Type == PlayerRescuePassive) {
		CUnit *uins = UnitInside;
		for (int i = InsideCount; i; --i, uins = uins->NextContained) {
			uins->ChangeOwner(newplayer);
		}
	}
	//Wyrmgus end

	//  Must change food/gold and other.
	UnitLost(*this);

	//  Now the new side!

	if (Type->BoolFlag[BUILDING_INDEX].value) {
		//Wyrmgus start
//		if (!Type->BoolFlag[WALL_INDEX].value) {
		//Wyrmgus end
			newplayer.TotalBuildings++;
		//Wyrmgus start
//		}
		//Wyrmgus end
	} else {
		newplayer.TotalUnits++;
	}

	MapUnmarkUnitSight(*this);
	newplayer.AddUnit(*this);
	Stats = &Type->Stats[newplayer.GetIndex()];

	//  Must change food/gold and other.
	//Wyrmgus start
//	if (Type->GivesResource) {
	if (this->GivesResource) {
	//Wyrmgus end
		DebugPrint("Resource transfer not supported\n");
	}
	newplayer.Demand += Type->Stats[newplayer.GetIndex()].Variables[DEMAND_INDEX].Value;
	newplayer.Supply += this->Variable[SUPPLY_INDEX].Value;
	// Increase resource limit
	for (int i = 0; i < MaxCosts; ++i) {
		if (newplayer.MaxResources[i] != -1 && Type->Stats[newplayer.GetIndex()].Storing[i]) {
			newplayer.MaxResources[i] += Type->Stats[newplayer.GetIndex()].Storing[i];
		}
	}
	//Wyrmgus start
//	if (Type->BoolFlag[BUILDING_INDEX].value && !Type->BoolFlag[WALL_INDEX].value) {
	if (Type->BoolFlag[BUILDING_INDEX].value) {
	//Wyrmgus end
		newplayer.NumBuildings++;
	}
	//Wyrmgus start
	if (CurrentAction() == UnitActionBuilt) {
		newplayer.NumBuildingsUnderConstruction++;
		newplayer.ChangeUnitTypeUnderConstructionCount(this->Type, 1);
	}
	//Wyrmgus end

	//apply upgrades of the new player, if the old one doesn't have that upgrade
	for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
		const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
		if (oldplayer->Allow.Upgrades[modifier_upgrade->GetIndex()] != 'R' && newplayer.Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())) { //if the old player doesn't have the modifier's upgrade (but the new one does), and the upgrade is applicable to the unit
			//Wyrmgus start
//			ApplyIndividualUpgradeModifier(*this, modifier);
			if ( // don't apply equipment-related upgrades if the unit has an item of that equipment type equipped
				(modifier_upgrade->GetItemSlot() == nullptr || this->EquippedItems.find(modifier_upgrade->GetItemSlot()) == this->EquippedItems.end() || this->EquippedItems.find(modifier_upgrade->GetItemSlot())->second.empty())
				&& !(newplayer.Race != -1 && modifier_upgrade == CCivilization::Get(newplayer.Race)->GetUpgrade())
				&& !(newplayer.Race != -1 && newplayer.GetFaction() != nullptr && modifier_upgrade == newplayer.GetFaction()->GetUpgrade())
			) {
				ApplyIndividualUpgradeModifier(*this, modifier);
			}
			//Wyrmgus end
		}
	}

	newplayer.IncreaseCountsForUnit(this);
	UpdateForNewUnit(*this, 1);
	
	UpdateUnitSightRange(*this);
	MapMarkUnitSight(*this);
	
	//not very elegant way to make sure the tile ownership is calculated correctly
	MapUnmarkUnitSight(*this);
	MapMarkUnitSight(*this);
	
	//Wyrmgus start
	if (newplayer.GetIndex() == CPlayer::GetThisPlayer()->GetIndex() && show_change) {
		this->Blink = 5;
		PlayGameSound(GameSounds.Rescue[newplayer.Race].Sound, MaxSampleVolume);
	}
	//Wyrmgus end
}

static bool IsMineAssignedBy(const CUnit &mine, const CUnit &worker)
{
	for (CUnit *it = mine.Resource.Workers; it; it = it->NextWorker) {
		if (it == &worker) {
			return true;
		}
	}
	return false;
}


void CUnit::AssignWorkerToMine(CUnit &mine)
{
	if (IsMineAssignedBy(mine, *this) == true) {
		return;
	}
	Assert(this->NextWorker == nullptr);

	CUnit *head = mine.Resource.Workers;
#if 0
	DebugPrint("%d: Worker [%d] is adding into %s [%d] on %d pos\n"
			   _C_ this->Player->GetIndex() _C_ this->Slot
			   _C_ mine.GetType()->Name.c_str()
			   _C_ mine.Slot
			   _C_ mine.Data.Resource.Assigned);
#endif
	this->RefsIncrease();
	this->NextWorker = head;
	mine.Resource.Workers = this;
	mine.Resource.Assigned++;
}

void CUnit::DeAssignWorkerFromMine(CUnit &mine)
{
	if (IsMineAssignedBy(mine, *this) == false) {
		return ;
	}
	CUnit *prev = nullptr, *worker = mine.Resource.Workers;
#if 0
	DebugPrint("%d: Worker [%d] is removing from %s [%d] left %d units assigned\n"
			   _C_ this->Player->GetIndex() _C_ this->Slot
			   _C_ mine.GetType()->Name.c_str()
			   _C_ mine.Slot
			   _C_ mine.CurrentOrder()->Data.Resource.Assigned);
#endif
	for (int i = 0; nullptr != worker; worker = worker->NextWorker, ++i) {
		if (worker == this) {
			CUnit *next = worker->NextWorker;
			worker->NextWorker = nullptr;
			if (prev) {
				prev->NextWorker = next;
			}
			if (worker == mine.Resource.Workers) {
				mine.Resource.Workers = next;
			}
			worker->RefsDecrease();
			break;
		}
		prev = worker;
		Assert(i <= mine.Resource.Assigned);
	}
	mine.Resource.Assigned--;
	Assert(mine.Resource.Assigned >= 0);
}


/**
**  Change the owner of all units of a player.
**
**  @param oldplayer    Old owning player.
**  @param newplayer    New owning player.
*/
static void ChangePlayerOwner(CPlayer &oldplayer, CPlayer &newplayer)
{
	if (&oldplayer == &newplayer) {
		return ;
	}

	for (int i = 0; i != oldplayer.GetUnitCount(); ++i) {
		CUnit &unit = oldplayer.GetUnit(i);

		unit.Blink = 5;
		unit.RescuedFrom = &oldplayer;
	}
	// ChangeOwner remove unit from the player: so change the array.
	while (oldplayer.GetUnitCount() != 0) {
		CUnit &unit = oldplayer.GetUnit(0);

		unit.ChangeOwner(newplayer);
	}
}

/**
**  Rescue units.
**
**  Look through all rescueable players, if they could be rescued.
*/
void RescueUnits()
{
	if (NoRescueCheck) {  // all possible units are rescued
		return;
	}
	NoRescueCheck = true;

	//  Look if player could be rescued.
	for (CPlayer *p : CPlayer::Players) {
		if (p->Type != PlayerRescuePassive && p->Type != PlayerRescueActive) {
			continue;
		}
		if (p->GetUnitCount() != 0) {
			NoRescueCheck = false;
			// NOTE: table is changed.
			std::vector<CUnit *> table;
			table.insert(table.begin(), p->UnitBegin(), p->UnitEnd());

			const size_t l = table.size();
			for (size_t j = 0; j != l; ++j) {
				CUnit &unit = *table[j];
				// Do not rescue removed units. Units inside something are
				// rescued by ChangeUnitOwner
				if (unit.Removed) {
					continue;
				}
				std::vector<CUnit *> around;

				SelectAroundUnit(unit, 1, around);
				//  Look if ally near the unit.
				for (size_t i = 0; i != around.size(); ++i) {
					//Wyrmgus start
//					if (around[i]->Type->CanAttack && unit.IsAllied(*around[i]) && around[i]->GetPlayer()->Type != PlayerRescuePassive && around[i]->GetPlayer()->Type != PlayerRescueActive) {
					if (around[i]->CanAttack() && unit.IsAllied(*around[i]) && around[i]->GetPlayer()->Type != PlayerRescuePassive && around[i]->GetPlayer()->Type != PlayerRescueActive) {
					//Wyrmgus end
						//  City center converts complete race
						//  NOTE: I use a trick here, centers could
						//        store gold. FIXME!!!
						//Wyrmgus start
//						if (unit.GetType()->CanStore[GoldCost]) {
						if (unit.GetType()->BoolFlag[TOWNHALL_INDEX].value) {
						//Wyrmgus end
							ChangePlayerOwner(*p, *around[i]->GetPlayer());
							break;
						}
						unit.RescuedFrom = unit.GetPlayer();
						//Wyrmgus start
//						unit.ChangeOwner(*around[i]->GetPlayer());
						unit.ChangeOwner(*around[i]->GetPlayer(), true);
//						unit.Blink = 5;
//						PlayGameSound(GameSounds.Rescue[unit.GetPlayer()->Race].Sound, MaxSampleVolume);
						//Wyrmgus end
						break;
					}
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------
--  Unit headings
----------------------------------------------------------------------------*/

/**
**  Fast arc tangent function.
**
**  @param val  atan argument
**
**  @return     atan(val)
*/
static int myatan(int val)
{
	static int init;
	static unsigned char atan_table[2608];

	if (val >= 2608) {
		return 63;
	}
	if (!init) {
		for (; init < 2608; ++init) {
			atan_table[init] =
				(unsigned char)(atan((double)init / 64) * (64 * 4 / 6.2831853));
		}
	}

	return atan_table[val];
}

/**
**  Convert direction to heading.
**
**  @param delta  Delta.
**
**  @return         Angle (0..255)
*/
int DirectionToHeading(const Vec2i &delta)
{
	//  Check which quadrant.
	if (delta.x > 0) {
		if (delta.y < 0) { // Quadrant 1?
			return myatan((delta.x * 64) / -delta.y);
		}
		// Quadrant 2?
		return myatan((delta.y * 64) / delta.x) + 64;
	}
	if (delta.y > 0) { // Quadrant 3?
		return myatan((delta.x * -64) / delta.y) + 64 * 2;
	}
	if (delta.x) { // Quadrant 4.
		return myatan((delta.y * -64) / -delta.x) + 64 * 3;
	}
	return 0;
}

/**
**  Convert direction to heading.
**
**  @param delta  Delta.
**
**  @return         Angle (0..255)
*/
int DirectionToHeading(const PixelDiff &delta)
{
	// code is identic for Vec2i and PixelDiff
	Vec2i delta2(delta.x, delta.y);
	return DirectionToHeading(delta2);
}

/**
**  Update sprite frame for new heading.
*/
void UnitUpdateHeading(CUnit &unit)
{
	//fix direction if it does not correspond to one of the defined directions
	int num_dir = std::max<int>(8, unit.GetType()->NumDirections);
	if (unit.Direction % (256 / num_dir) != 0) {
		unit.Direction -= unit.Direction % (256 / num_dir);
	}
	
	bool neg;
	if (unit.GetFrame() < 0) {
		unit.SetFrame(-unit.GetFrame() - 1);
		neg = true;
	} else {
		neg = false;
	}
	
	unit.SetFrame(unit.GetFrame() / (unit.GetType()->NumDirections / 2 + 1));
	unit.SetFrame(unit.GetFrame() * (unit.GetType()->NumDirections / 2 + 1));
	// Remove heading, keep animation frame

	const int nextdir = 256 / unit.GetType()->NumDirections;
	const int dir = ((unit.Direction + nextdir / 2) & 0xFF) / nextdir;
	if (dir <= LookingS / nextdir) { // north->east->south
		unit.ChangeFrame(dir);
	} else {
		unit.ChangeFrame(256 / nextdir - dir);
		unit.SetFrame(-unit.GetFrame() - 1);
	}
	if (neg && !unit.GetFrame() && unit.GetType()->BoolFlag[BUILDING_INDEX].value) {
		unit.SetFrame(-1);
	}
}

/**
**  Change unit heading/frame from delta direction x, y.
**
**  @param unit  Unit for new direction looking.
**  @param delta  map tile delta direction.
*/
void UnitHeadingFromDeltaXY(CUnit &unit, const Vec2i &delta)
{
	//Wyrmgus start
//	unit.Direction = DirectionToHeading(delta);
	int num_dir = std::max<int>(8, unit.GetType()->NumDirections);
	int heading = DirectionToHeading(delta) + ((256 / num_dir) / 2);
	if (heading % (256 / num_dir) != 0) {
		heading = heading - (heading % (256 / num_dir));
	}
	unit.Direction = heading;
	//Wyrmgus end
	UnitUpdateHeading(unit);
}

/*----------------------------------------------------------------------------
  -- Drop out units
  ----------------------------------------------------------------------------*/

/**
**  Place a unit on the map to the side of a unit.
**
**  @param unit       Unit to drop out.
**  @param heading    Direction in which the unit should appear.
**  @param container  Unit "containing" unit to drop (may be different of unit.Container).
*/
void DropOutOnSide(CUnit &unit, int heading, const CUnit *container)
{
	Vec2i pos;
	int addx = 0;
	int addy = 0;
	//Wyrmgus start
	int z;
	//Wyrmgus end

	if (container) {
		pos = container->GetTilePos();
		pos -= unit.GetType()->TileSize - 1;
		addx = container->GetType()->TileSize.x + unit.GetType()->TileSize.x - 1;
		addy = container->GetType()->TileSize.y + unit.GetType()->TileSize.y - 1;
		z = container->GetMapLayer()->GetIndex();

		if (heading < LookingNE || heading > LookingNW) {
			pos.x += addx - 1;
			--pos.y;
			goto startn;
		} else if (heading < LookingSE) {
			pos.x += addx;
			pos.y += addy - 1;
			goto starte;
		} else if (heading < LookingSW) {
			pos.y += addy;
			goto starts;
		} else {
			--pos.x;
			goto startw;
		}
	} else {
		pos = unit.GetTilePos();
		z = unit.GetMapLayer()->GetIndex();

		if (heading < LookingNE || heading > LookingNW) {
			goto starts;
		} else if (heading < LookingSE) {
			goto startw;
		} else if (heading < LookingSW) {
			goto startn;
		} else {
			goto starte;
		}
	}
	// FIXME: don't search outside of the map
	for (;;) {
startw:
		for (int i = addy; i--; ++pos.y) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				goto found;
			}
		}
		++addx;
starts:
		for (int i = addx; i--; ++pos.x) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				goto found;
			}
		}
		++addy;
starte:
		for (int i = addy; i--; --pos.y) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				goto found;
			}
		}
		++addx;
startn:
		for (int i = addx; i--; --pos.x) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				goto found;
			}
		}
		++addy;
	}

found:
	//Wyrmgus start
//	unit.Place(pos);
	unit.Place(pos, z);
	//Wyrmgus end
}

/**
**  Place a unit on the map nearest to goalPos.
**
**  @param unit  Unit to drop out.
**  @param goalPos Goal map tile position.
**  @param addx  Tile width of unit it's dropping out of.
**  @param addy  Tile height of unit it's dropping out of.
*/
void DropOutNearest(CUnit &unit, const Vec2i &goalPos, const CUnit *container)
{
	Vec2i pos;
	Vec2i bestPos;
	int bestd = 99999;
	int addx = 0;
	int addy = 0;
	//Wyrmgus start
	int z;
	//Wyrmgus end

	if (container) {
		Assert(unit.Removed);
		pos = container->GetTilePos();
		pos -= unit.GetType()->TileSize - 1;
		addx = container->GetType()->TileSize.x + unit.GetType()->TileSize.x - 1;
		addy = container->GetType()->TileSize.y + unit.GetType()->TileSize.y - 1;
		--pos.x;
		z = container->GetMapLayer()->GetIndex();
	} else {
		pos = unit.GetTilePos();
		z = unit.GetMapLayer()->GetIndex();
	}
	// FIXME: if we reach the map borders we can go fast up, left, ...

	for (;;) {
		for (int i = addy; i--; ++pos.y) { // go down
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				const int n = SquareDistance(goalPos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		++addx;
		for (int i = addx; i--; ++pos.x) { // go right
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				const int n = SquareDistance(goalPos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		++addy;
		for (int i = addy; i--; --pos.y) { // go up
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				const int n = SquareDistance(goalPos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		++addx;
		for (int i = addx; i--; --pos.x) { // go left
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z)) {
			//Wyrmgus end
				const int n = SquareDistance(goalPos, pos);

				if (n < bestd) {
					bestd = n;
					bestPos = pos;
				}
			}
		}
		if (bestd != 99999) {
			//Wyrmgus start
//			unit.Place(bestPos);
			unit.Place(bestPos, z);
			//Wyrmgus end
			return;
		}
		++addy;
	}
}

/**
**  Drop out all units inside unit.
**
**  @param source  All units inside source are dropped out.
*/
void DropOutAll(const CUnit &source)
{
	CUnit *unit = source.UnitInside;

	for (int i = source.InsideCount; i; --i, unit = unit->NextContained) {
		DropOutOnSide(*unit, LookingW, &source);
	}
	
	//Wyrmgus start
	if (unit->GetType()->BoolFlag[ITEM_INDEX].value && !unit->Unique) { //save the initial cycle items were placed in the ground to destroy them if they have been there for too long
		int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
		if (unit->Prefix != nullptr || unit->Suffix != nullptr || unit->Spell != nullptr || unit->Work != nullptr || unit->Elixir != nullptr) {
			ttl_cycles *= 4;
		}
		unit->TTL = GameCycle + ttl_cycles;
	}
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
  -- Select units
  ----------------------------------------------------------------------------*/

/**
**  Unit on map screen.
**
**  Select units on screen. (x, y are in pixels relative to map 0,0).
**  Not GAMEPLAY safe, uses ReplayRevealMap
**
**  More units on same position.
**    Cycle through units.
**    First take highest unit.
**
**  @param x      X pixel position.
**  @param y      Y pixel position.
**
**  @return       An unit on x, y position.
*/
CUnit *UnitOnScreen(int x, int y)
{
	CUnit *candidate = nullptr;
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (unit.GetMapLayer() != UI.CurrentMapLayer) {
			continue;
		}
		if (!ReplayRevealMap && !unit.IsVisibleAsGoal(*CPlayer::GetThisPlayer())) {
			continue;
		}
		const CUnitType &type = *unit.GetType();
		if (!type.Sprite) {
			continue;
		}

		//
		// Check if mouse is over the unit.
		//
		PixelPos unitSpritePos = unit.GetMapPixelPosCenter();
		//Wyrmgus start
//		unitSpritePos.x = unitSpritePos.x - type.GetBoxWidth() / 2 -
//						  (type.GetFrameSize().x - type.Sprite->Width) / 2 + type.GetBoxOffsetX();
//		unitSpritePos.y = unitSpritePos.y - type.GetBoxHeight() / 2 -
//						  (type.GetFrameSize().y - type.Sprite->Height) / 2 + type.GetBoxOffsetY();
		const UnitTypeVariation *variation = unit.GetVariation();
		if (variation && variation->GetFrameSize().x != 0 && variation->GetFrameSize().y != 0 && variation->GetImage() != nullptr) {
			unitSpritePos.x = unitSpritePos.x - type.GetBoxWidth() / 2 -
							  (variation->GetFrameSize().x - variation->Sprite->Width) / 2 + type.GetBoxOffsetX();
			unitSpritePos.y = unitSpritePos.y - type.GetBoxHeight() / 2 -
							  (variation->GetFrameSize().y - variation->Sprite->Height) / 2 + type.GetBoxOffsetY();
		} else {
			unitSpritePos.x = unitSpritePos.x - type.GetBoxWidth() / 2 -
							  (type.GetFrameSize().x - type.Sprite->Width) / 2 + type.GetBoxOffsetX();
			unitSpritePos.y = unitSpritePos.y - type.GetBoxHeight() / 2 -
							  (type.GetFrameSize().y - type.Sprite->Height) / 2 + type.GetBoxOffsetY();
		}
		//Wyrmgus end
		if (x >= unitSpritePos.x && x < unitSpritePos.x + type.GetBoxWidth()
			&& y >= unitSpritePos.y  && y < unitSpritePos.y + type.GetBoxHeight()) {
			// Check if there are other units on this place
			candidate = &unit;
			//Wyrmgus start
			std::vector<CUnit *> table;
			Select(candidate->GetTilePos(), candidate->GetTilePos(), table, candidate->GetMapLayer()->GetIndex(), HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]));
//			if (IsOnlySelected(*candidate) || candidate->GetType()->BoolFlag[ISNOTSELECTABLE_INDEX].value) {
			if (IsOnlySelected(*candidate) || candidate->GetType()->BoolFlag[ISNOTSELECTABLE_INDEX].value || (candidate->GetPlayer()->Type == PlayerNeutral && table.size()) || !candidate->IsAlive()) { // don't select a neutral unit if there's a player-owned unit there as well; don't selected a dead unit
			//Wyrmgus end
				continue;
			} else {
				break;
			}
		} else {
			continue;
		}
	}
	return candidate;
}

PixelPos CUnit::GetMapPixelPosTopLeft() const
{
	const PixelPos pos(this->GetTilePos().x * CMap::Map.GetMapLayerPixelTileSize(this->GetMapLayer()->GetIndex()).x + this->GetPixelOffset().x, this->GetTilePos().y * CMap::Map.GetMapLayerPixelTileSize(this->GetMapLayer()->GetIndex()).y + this->GetPixelOffset().y);
	return pos;
}

PixelSize CUnit::GetTilePixelSize() const
{
	return PixelSize(this->GetTileSize()) * CMap::PixelTileSize;
}

const PaletteImage *CUnit::GetImage() const
{
	const UnitTypeVariation *variation = this->GetVariation();

	if (this->GetType()->BoolFlag[HARVESTER_INDEX].value && this->GetCurrentResource()) {
		const CResource *resource = CResource::Get(this->GetCurrentResource());
		if (this->GetResourcesHeld() > 0) {
			if (variation != nullptr) {
				auto find_iterator = variation->ResourceLoadedImages.find(resource);
				if (find_iterator != variation->ResourceLoadedImages.end()) {
					return find_iterator->second;
				}
			}
			
			if (this->GetType()->ResInfo[this->GetCurrentResource()]->ResourceLoadedImage != nullptr) {
				return this->GetType()->ResInfo[this->GetCurrentResource()]->ResourceLoadedImage;
			}
		} else {
			if (variation != nullptr) {
				auto find_iterator = variation->ResourceEmptyImages.find(resource);
				if (find_iterator != variation->ResourceEmptyImages.end()) {
					return find_iterator->second;
				}
			}
			
			if (this->GetType()->ResInfo[this->GetCurrentResource()]->ResourceEmptyImage != nullptr) {
				return this->GetType()->ResInfo[this->GetCurrentResource()]->ResourceEmptyImage;
			}
		}
	}
	
	if (variation != nullptr && variation->GetImage() != nullptr) {
		return variation->GetImage();
	}
	
	return this->GetType()->GetImage();
}

void CUnit::SetFrame(const int frame)
{
	if (frame == this->Frame) {
		return;
	}
	
	if ((frame < 0) != (this->Frame < 0)) {
		if (!this->Removed) {
			this->emit_signal("flipped_changed", (frame < 0));
		}
	}
	
	this->Frame = frame;
	
	if (!this->Removed) {
		this->emit_signal("frame_changed", this->GetSpriteFrame());
	}
}

void CUnit::SetPixelOffset(const Vector2i &offset)
{
	if (offset == this->PixelOffset) {
		return;
	}
	
	this->PixelOffset = offset;
	
	if (!this->Removed) {
		this->emit_signal("pixel_offset_changed", Vector2(this->GetPixelOffset()));
	}
}

//Wyrmgus start
void CUnit::SetIndividualUpgrade(const CUpgrade *upgrade, int quantity)
{
	if (!upgrade) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->IndividualUpgrades.find(upgrade->GetIndex()) != this->IndividualUpgrades.end()) {
			this->IndividualUpgrades.erase(upgrade->GetIndex());
		}
	} else {
		this->IndividualUpgrades[upgrade->GetIndex()] = quantity;
	}
}

int CUnit::GetIndividualUpgrade(const CUpgrade *upgrade) const
{
	if (upgrade && this->IndividualUpgrades.find(upgrade->GetIndex()) != this->IndividualUpgrades.end()) {
		return this->IndividualUpgrades.find(upgrade->GetIndex())->second;
	} else {
		return 0;
	}
}

int CUnit::GetAvailableLevelUpUpgrades(bool only_units) const
{
	int value = 0;
	int upgrade_value = 0;
	
	if (((int) AiHelpers.ExperienceUpgrades.size()) > Type->GetIndex()) {
		for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[Type->GetIndex()].size(); ++i) {
			if (this->Character == nullptr || std::find(this->Character->ForbiddenUpgrades.begin(), this->Character->ForbiddenUpgrades.end(), AiHelpers.ExperienceUpgrades[Type->GetIndex()][i]) == this->Character->ForbiddenUpgrades.end()) {
				int local_upgrade_value = 1;
				
				if (!only_units) {
					local_upgrade_value += AiHelpers.ExperienceUpgrades[Type->GetIndex()][i]->GetAvailableLevelUpUpgrades();
				}
				
				if (local_upgrade_value > upgrade_value) {
					upgrade_value = local_upgrade_value;
				}
			}
		}
	}
	
	value += upgrade_value;
	
	if (!only_units && ((int) AiHelpers.LearnableAbilities.size()) > this->Type->GetIndex()) {
		for (size_t i = 0; i != AiHelpers.LearnableAbilities[this->Type->GetIndex()].size(); ++i) {
			value += AiHelpers.LearnableAbilities[this->Type->GetIndex()][i]->MaxLimit - this->GetIndividualUpgrade(AiHelpers.LearnableAbilities[this->Type->GetIndex()][i]);
		}
	}
	
	return value;
}

/**
**	@brief	Get the value for a given variable for the unit.
**
**	@param	index	The variable's index.
**
**	@return	The variable's value.
*/
int CUnit::GetVariableValue(const int index) const
{
	return this->Variable[index].Value;
}

/**
**	@brief	Get the maximum value for a given variable for the unit.
**
**	@param	index	The variable's index.
**
**	@return	The variable's maximum value.
*/
int CUnit::GetVariableMax(const int index) const
{
	return this->Variable[index].Max;
}

/**
**	@brief	Get the per-cycle increase value for a given variable for the unit.
**
**	@param	index	The variable's index.
**
**	@return	The variable's per-cycle increase value.
*/
char CUnit::GetVariableIncrease(const int index) const
{
	return this->Variable[index].Increase;
}

/**
**	@brief	Get whether a given variable is enabled for the unit.
**
**	@param	index	The variable's index.
**
**	@return	True if the variable is enabled, or false otherwise.
*/
bool CUnit::IsVariableEnabled(const int index) const
{
	return this->Variable[index].Enable != 0;
}

int CUnit::GetModifiedVariable(const int index, const int variable_type) const
{
	int value = 0;
	if (variable_type == VariableValue) {
		value = this->Variable[index].Value;
	} else if (variable_type == VariableMax) {
		value = this->Variable[index].Max;
	} else if (variable_type == VariableIncrease) {
		value = this->Variable[index].Increase;
	}
	
	if (index == ATTACKRANGE_INDEX) {
		if (this->Container && this->Container->Variable[GARRISONEDRANGEBONUS_INDEX].Enable) {
			value += this->Container->Variable[GARRISONEDRANGEBONUS_INDEX].Value; //treat the container's attack range as a bonus to the unit's attack range
		}
		if (this->CurrentSightRange > 0) {
			value = std::min<int>(this->CurrentSightRange, value); //if the unit's current sight range is smaller than its attack range, use it instead
		}
	} else if (index == SPEED_INDEX) {
		if (this->MapLayer && this->Type->UnitType != UnitTypeFly && this->Type->UnitType != UnitTypeFlyLow) {
			value += DefaultTileMovementCost - this->GetMapLayer()->Field(this->Offset)->getCost();
		}
	}
	
	return value;
}

int CUnit::GetReactionRange() const
{
	int reaction_range = this->CurrentSightRange;

	if (this->Player->Type != PlayerPerson) {
		reaction_range += 2;
	}
	
	return reaction_range;
}

int CUnit::GetItemSlotQuantity(const ItemSlot *item_slot) const
{
	if (!this->HasInventory()) {
		return 0;
	}
	
	if ( //if the item are arrows and the weapon of this unit's type is not a bow, return false
		item_slot->IsArrows()
		&& this->Type->WeaponClasses[0]->AllowsArrows() == false
	) {
		return 0;
	}
	
	return item_slot->GetQuantity();
}

const ItemClass *CUnit::GetCurrentWeaponClass() const
{
	if (this->HasInventory()) {
		for (std::map<const ItemSlot *, std::vector<CUnit *>>::const_iterator iterator = this->EquippedItems.begin(); iterator != this->EquippedItems.end(); ++iterator) {
			const ItemSlot *item_slot = iterator->first;
			if (item_slot->IsWeapon() && iterator->second.size() > 0) {
				return iterator->second.front()->Type->ItemClass;
			}
		}
	}
	
	return this->Type->WeaponClasses[0];
}

int CUnit::GetItemVariableChange(const CUnit *item, int variable_index, bool increase) const
{
	if (item->Type->ItemClass == nullptr) {
		return 0;
	}
	
	const ItemSlot *item_slot = item->Type->ItemClass->GetSlot();
	if (item->Work == nullptr && item->Elixir == nullptr && (item_slot == nullptr || this->GetItemSlotQuantity(item_slot) == 0 || !this->CanEquipItemClass(item->Type->ItemClass))) {
		return 0;
	}
	
	int value = 0;
	if (item->Work != nullptr) {
		if (this->GetIndividualUpgrade(item->Work) == 0) {
			for (size_t z = 0; z < item->Work->UpgradeModifiers.size(); ++z) {
				if (!increase) {
					value += item->Work->UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
				} else {
					value += item->Work->UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
				}
			}
		}
	} else if (item->Elixir != nullptr) {
		if (this->GetIndividualUpgrade(item->Elixir) == 0) {
			for (size_t z = 0; z < item->Elixir->UpgradeModifiers.size(); ++z) {
				if (!increase) {
					value += item->Elixir->UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
				} else {
					value += item->Elixir->UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
				}
			}
		}
	} else {
		if (!increase) {
			value = item->Variable[variable_index].Value;
		} else {
			value = item->Variable[variable_index].Increase;
		}
		
		if (!item->Identified) { //if the item is unidentified, don't show the effects of its affixes
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				if (
					(item->Prefix != nullptr && modifier->UpgradeId == item->Prefix->GetIndex())
					|| (item->Suffix != nullptr && modifier->UpgradeId == item->Suffix->GetIndex())
				) {
					if (!increase) {
						value -= modifier->Modifier.Variables[variable_index].Value;
					} else {
						value -= modifier->Modifier.Variables[variable_index].Increase;
					}
				}
			}
		}
		
		if (item->Unique && item->Unique->Set) {
			if (this->EquippingItemCompletesSet(item)) {
				for (size_t z = 0; z < item->Unique->Set->UpgradeModifiers.size(); ++z) {
					if (!increase) {
						value += item->Unique->Set->UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
					} else {
						value += item->Unique->Set->UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
					}
				}
			}
		}
		
		if (item_slot != nullptr && this->EquippedItems.find(item_slot) != this->EquippedItems.end() && this->EquippedItems.find(item_slot)->second.size() == this->GetItemSlotQuantity(item_slot)) {
			const std::vector<CUnit *> &item_slot_equipped_items = this->EquippedItems.find(item_slot)->second;
			int item_slot_used = item_slot_equipped_items.size() - 1;
			for (size_t i = 0; i < item_slot_equipped_items.size(); ++i) {
				if (item_slot_equipped_items[i] == item) {
					item_slot_used = i;
				}
			}
			if (!increase) {
				value -= item_slot_equipped_items[item_slot_used]->Variable[variable_index].Value;
			} else {
				value -= item_slot_equipped_items[item_slot_used]->Variable[variable_index].Increase;
			}
			if (item_slot_equipped_items[item_slot_used] != item && item_slot_equipped_items[item_slot_used]->Unique && item_slot_equipped_items[item_slot_used]->Unique->Set) {
				if (this->DeequippingItemBreaksSet(item_slot_equipped_items[item_slot_used])) {
					for (size_t z = 0; z < item_slot_equipped_items[item_slot_used]->Unique->Set->UpgradeModifiers.size(); ++z) {
						if (!increase) {
							value -= item_slot_equipped_items[item_slot_used]->Unique->Set->UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
						} else {
							value -= item_slot_equipped_items[item_slot_used]->Unique->Set->UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
						}
					}
				}
			}
		} else if (
			item_slot != nullptr
			&& (this->EquippedItems.find(item_slot) == this->EquippedItems.end() || this->EquippedItems.find(item_slot)->second.empty())
			&& (item_slot->IsWeapon() || item_slot->IsShield() || item_slot->IsBoots() || item_slot->IsArrows())
		) {
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
				const CUpgrade *modifier_upgrade = CUpgrade::Get(modifier->UpgradeId);
				if (
					(
						modifier_upgrade->GetItemSlot() == item_slot
						&& Player->Allow.Upgrades[modifier_upgrade->GetIndex()] == 'R' && modifier->AppliesToUnitType(this->GetType())
					)
					|| (item_slot->IsWeapon() && modifier_upgrade->IsAbility() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->GetCurrentWeaponClass()) != modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item->Type->ItemClass) == modifier_upgrade->WeaponClasses.end())
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							if (!increase) {
								value -= modifier->Modifier.Variables[variable_index].Value;
							} else {
								value -= modifier->Modifier.Variables[variable_index].Increase;
							}
						}
					} else {
						if (!increase) {
							value -= modifier->Modifier.Variables[variable_index].Value;
						} else {
							value -= modifier->Modifier.Variables[variable_index].Increase;
						}
					}
				} else if (
					modifier_upgrade->IsAbility() && this->GetIndividualUpgrade(modifier_upgrade) && modifier_upgrade->WeaponClasses.size() > 0 && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), this->GetCurrentWeaponClass()) == modifier_upgrade->WeaponClasses.end() && std::find(modifier_upgrade->WeaponClasses.begin(), modifier_upgrade->WeaponClasses.end(), item->Type->ItemClass) != modifier_upgrade->WeaponClasses.end()
				) {
					if (this->GetIndividualUpgrade(modifier_upgrade)) {
						for (int i = 0; i < this->GetIndividualUpgrade(modifier_upgrade); ++i) {
							if (!increase) {
								value += modifier->Modifier.Variables[variable_index].Value;
							} else {
								value += modifier->Modifier.Variables[variable_index].Increase;
							}
						}
					} else {
						if (!increase) {
							value += modifier->Modifier.Variables[variable_index].Value;
						} else {
							value += modifier->Modifier.Variables[variable_index].Increase;
						}
					}
				}
			}
		}
	}
	
	return value;
}

int CUnit::GetDisplayPlayer() const
{
	if (this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->Player != CPlayer::GetThisPlayer()) {
		return PlayerNumNeutral;
	} else {
		return this->RescuedFrom ? this->RescuedFrom->GetIndex() : this->Player->GetIndex();
	}
}

int CUnit::GetPrice() const
{
	int cost = this->Type->Stats[this->Player->GetIndex()].GetPrice();
	
	if (this->Prefix != nullptr) {
		cost += this->Prefix->GetMagicLevel() * 1000;
	}
	if (this->Suffix != nullptr) {
		cost += this->Suffix->GetMagicLevel() * 1000;
	}
	if (this->Spell != nullptr) {
		cost += 1000;
	}
	if (this->Work != nullptr) {
		cost += this->Work->GetMagicLevel() * 1000;
	}
	if (this->Elixir != nullptr) {
		cost += this->Elixir->GetMagicLevel() * 1000;
	}
	if (this->Character) {
		cost += (this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->GetIndex()].Variables[LEVEL_INDEX].Value) * 250;
	}
	
	return cost;
}

int CUnit::GetUnitStock(const CUnitType *unit_type) const
{
	if (unit_type && this->UnitStock.find(unit_type) != this->UnitStock.end()) {
		return this->UnitStock.find(unit_type)->second;
	} else {
		return 0;
	}
}

void CUnit::SetUnitStock(const CUnitType *unit_type, const int quantity)
{
	if (!unit_type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitStock.find(unit_type) != this->UnitStock.end()) {
			this->UnitStock.erase(unit_type);
		}
	} else {
		this->UnitStock[unit_type] = quantity;
	}
}

void CUnit::ChangeUnitStock(const CUnitType *unit_type, const int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}

int CUnit::GetUnitStockReplenishmentTimer(const CUnitType *unit_type) const
{
	if (this->UnitStockReplenishmentTimers.find(unit_type) != this->UnitStockReplenishmentTimers.end()) {
		return this->UnitStockReplenishmentTimers.find(unit_type)->second;
	} else {
		return 0;
	}
}

void CUnit::SetUnitStockReplenishmentTimer(const CUnitType *unit_type, const int quantity)
{
	if (!unit_type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitStockReplenishmentTimers.find(unit_type) != this->UnitStockReplenishmentTimers.end()) {
			this->UnitStockReplenishmentTimers.erase(unit_type);
		}
	} else {
		this->UnitStockReplenishmentTimers[unit_type] = quantity;
	}
}

void CUnit::ChangeUnitStockReplenishmentTimer(const CUnitType *unit_type, const int quantity)
{
	this->SetUnitStockReplenishmentTimer(unit_type, this->GetUnitStockReplenishmentTimer(unit_type) + quantity);
}

int CUnit::GetResourceStep(const int resource) const
{
	if (!this->Type->ResInfo[resource]) {
		return 0;
	}

	int resource_step = this->Type->ResInfo[resource]->ResourceStep;
	
	resource_step += this->Variable[GATHERINGBONUS_INDEX].Value;
	
	if (resource == CopperCost) {
		resource_step += this->Variable[COPPERGATHERINGBONUS_INDEX].Value;
	} else if (resource == SilverCost) {
		resource_step += this->Variable[SILVERGATHERINGBONUS_INDEX].Value;
	} else if (resource == GoldCost) {
		resource_step += this->Variable[GOLDGATHERINGBONUS_INDEX].Value;
	} else if (resource == IronCost) {
		resource_step += this->Variable[IRONGATHERINGBONUS_INDEX].Value;
	} else if (resource == MithrilCost) {
		resource_step += this->Variable[MITHRILGATHERINGBONUS_INDEX].Value;
	} else if (resource == WoodCost) {
		resource_step += this->Variable[LUMBERGATHERINGBONUS_INDEX].Value;
	} else if (resource == StoneCost || resource == LimestoneCost) {
		resource_step += this->Variable[STONEGATHERINGBONUS_INDEX].Value;
	} else if (resource == CoalCost) {
		resource_step += this->Variable[COALGATHERINGBONUS_INDEX].Value;
	} else if (resource == JewelryCost) {
		resource_step += this->Variable[JEWELRYGATHERINGBONUS_INDEX].Value;
	} else if (resource == FurnitureCost) {
		resource_step += this->Variable[FURNITUREGATHERINGBONUS_INDEX].Value;
	} else if (resource == LeatherCost) {
		resource_step += this->Variable[LEATHERGATHERINGBONUS_INDEX].Value;
	} else if (resource == DiamondsCost || resource == EmeraldsCost) {
		resource_step += this->Variable[GEMSGATHERINGBONUS_INDEX].Value;
	}
	
	return resource_step;
}

int CUnit::GetTotalInsideCount(const CPlayer *player, const bool ignore_items, const bool ignore_saved_cargo, const CUnitType *type) const
{
	if (!this->UnitInside) {
		return 0;
	}
	
	if (this->Type->BoolFlag[SAVECARGO_INDEX].value && ignore_saved_cargo) {
		return 0;
	}
	
	int inside_count = 0;
	
	CUnit *inside_unit = this->UnitInside;
	for (int j = 0; j < this->InsideCount; ++j, inside_unit = inside_unit->NextContained) {
		if ( //only count units of the faction, ignore items
			(!player || inside_unit->GetPlayer() == player)
			&& (!ignore_items || !inside_unit->GetType()->BoolFlag[ITEM_INDEX].value)
			&& (!type || inside_unit->GetType() == type)
		) {
			inside_count++;
		}
		inside_count += inside_unit->GetTotalInsideCount(player, ignore_items, ignore_saved_cargo);
	}

	return inside_count;
}

bool CUnit::CanAttack(bool count_inside) const
{
	if (this->Type->CanTransport() && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && !this->Type->BoolFlag[CANATTACK_INDEX].value) { //transporters without an attack can only attack through a unit within them
		if (count_inside && this->BoardCount > 0) {
			CUnit *boarded_unit = this->UnitInside;
			for (int i = 0; i < this->InsideCount; ++i, boarded_unit = boarded_unit->NextContained) {
				if (boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX) > 1 && boarded_unit->GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) {
					return true;
				}
			}
		}
		return false;
	}
	
	if (this->Container && (!this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value || !this->Container->GetType()->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value)) {
		return false;
	}
	
	return this->Type->BoolFlag[CANATTACK_INDEX].value;
}

bool CUnit::IsInCombat() const
{
	// Select all units around the unit
	std::vector<CUnit *> table;
	SelectAroundUnit(*this, this->GetReactionRange(), table, IsEnemyWith(*this->Player));

	for (size_t i = 0; i < table.size(); ++i) {
		const CUnit &target = *table[i];

		if (target.IsVisibleAsGoal(*this->Player) && (CanTarget(*this->Type, *target.GetType()) || CanTarget(*target.GetType(), *this->Type))) {
			return true;
		}
	}
		
	return false;
}

bool CUnit::CanHarvest(const CUnit *dest, bool only_harvestable) const
{
	if (!dest) {
		return false;
	}
	
	if (!dest->GivesResource) {
		return false;
	}
	
	if (!this->Type->ResInfo[dest->GivesResource]) {
		return false;
	}
	
	if (!dest->GetType()->BoolFlag[CANHARVEST_INDEX].value && only_harvestable) {
		return false;
	}
	
	if (!this->Type->BoolFlag[HARVESTER_INDEX].value) {
		return false;
	}
	
	if (dest->GivesResource == TradeCost) {
		if (dest->GetPlayer() == this->Player) { //can only trade with markets owned by other players
			return false;
		}
		
		if (this->Type->UnitType != UnitTypeNaval && dest->GetType()->BoolFlag[SHOREBUILDING_INDEX].value) { //only ships can trade with docks
			return false;
		}
		if (this->Type->UnitType == UnitTypeNaval && !dest->GetType()->BoolFlag[SHOREBUILDING_INDEX].value && dest->GetType()->UnitType != UnitTypeNaval) { //ships cannot trade with land markets
			return false;
		}
	} else {
		if (dest->GetPlayer() != this->Player && !(dest->GetPlayer()->IsAllied(*this->Player) && this->Player->IsAllied(*dest->GetPlayer())) && dest->GetPlayer()->GetIndex() != PlayerNumNeutral) {
			return false;
		}
	}
	
	if (this->BoardCount) { //cannot harvest if carrying units
		return false;
	}
	
	return true;
}

bool CUnit::CanReturnGoodsTo(const CUnit *dest, int resource) const
{
	if (!dest) {
		return false;
	}
	
	if (!resource) {
		resource = this->GetCurrentResource();
	}
	
	if (!resource) {
		return false;
	}
	
	if (!dest->GetType()->CanStore[this->GetCurrentResource()]) {
		return false;
	}
	
	if (resource == TradeCost) {
		if (dest->GetPlayer() != this->Player) { //can only return trade to markets owned by the same player
			return false;
		}
		
		if (this->Type->UnitType != UnitTypeNaval && dest->GetType()->BoolFlag[SHOREBUILDING_INDEX].value) { //only ships can return trade to docks
			return false;
		}
		if (this->Type->UnitType == UnitTypeNaval && !dest->GetType()->BoolFlag[SHOREBUILDING_INDEX].value && dest->GetType()->UnitType != UnitTypeNaval) { //ships cannot return trade to land markets
			return false;
		}
	} else {
		if (dest->GetPlayer() != this->Player && !(dest->GetPlayer()->IsAllied(*this->Player) && this->Player->IsAllied(*dest->GetPlayer()))) {
			return false;
		}
	}
	
	return true;
}

/**
**	@brief	Get whether a unit can cast a given spell
**
**	@return	True if the unit can cast the given spell, or false otherwise
*/
bool CUnit::CanCastSpell(const CSpell *spell, const bool ignore_mana_and_cooldown) const
{
	if (spell->IsAvailableForUnit(*this)) {
		if (!ignore_mana_and_cooldown) {
			if (this->Variable[MANA_INDEX].Value < spell->ManaCost) {
				return false;
			}
			
			if (this->SpellCoolDownTimers.find(spell) != this->SpellCoolDownTimers.end()) {
				return false;
			}
		}
		
		return true;
	} else {
		return false;
	}
}

/**
**	@brief	Get whether a unit can cast any spell
**
**	@return	True if the unit can cast any spell, or false otherwise
*/
bool CUnit::CanCastAnySpell() const
{
	for (size_t i = 0; i < this->Type->Spells.size(); ++i) {
		if (this->CanCastSpell(this->Type->Spells[i], true)) {
			return true;
		}
	}
	
	return false;
}

/**
**	@brief	Get whether the unit can autocast a given spell
**
**	@param	spell	The spell
**
**	@return	True if the unit can autocast the spell, false otherwise
*/
bool CUnit::CanAutoCastSpell(const CSpell *spell) const
{
	if (this->AutoCastSpells.empty() || !spell || this->AutoCastSpells.find(spell) == this->AutoCastSpells.end() || !spell->AutoCast) {
		return false;
	}
	
	if (!CanCastSpell(spell, false)) {
		return false;
	}
	
	return true;
}

bool CUnit::IsItemEquipped(const CUnit *item) const
{
	if (item->Type->ItemClass == nullptr) {
		return false;
	}
	
	const ItemSlot *item_slot = item->Type->ItemClass->GetSlot();
	
	if (item_slot == nullptr) {
		return false;
	}
	
	auto find_iterator = this->EquippedItems.find(item_slot);
	if (find_iterator == this->EquippedItems.end()) {
		return false;
	}
	
	if (std::find(find_iterator->second.begin(), find_iterator->second.end(), item) != find_iterator->second.end()) {
		return true;
	}
	
	return false;
}

bool CUnit::IsItemClassEquipped(const ItemClass *item_class) const
{
	if (item_class == nullptr) {
		return false;
	}
	
	const ItemSlot *item_slot = item_class->GetSlot();
	
	if (item_slot == nullptr) {
		return false;
	}
	
	auto find_iterator = this->EquippedItems.find(item_slot);
	if (find_iterator == this->EquippedItems.end()) {
		return false;
	}
	
	for (const CUnit *equipped_item : find_iterator->second) {
		if (equipped_item->Type->ItemClass == item_class) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::IsItemTypeEquipped(const CUnitType *item_type) const
{
	if (item_type->ItemClass == nullptr) {
		return false;
	}
	
	const ItemSlot *item_slot = item_type->ItemClass->GetSlot();
	
	if (item_slot == nullptr) {
		return false;
	}
	
	auto find_iterator = this->EquippedItems.find(item_slot);
	if (find_iterator == this->EquippedItems.end()) {
		return false;
	}
	
	for (const CUnit *equipped_item : find_iterator->second) {
		if (equipped_item->Type == item_type) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::IsUniqueItemEquipped(const UniqueItem *unique) const
{
	if (unique->Type->ItemClass == nullptr) {
		return false;
	}
	
	const ItemSlot *item_slot = unique->Type->ItemClass->GetSlot();
		
	if (item_slot == nullptr) {
		return false;
	}
		
	auto find_iterator = this->EquippedItems.find(item_slot);
	if (find_iterator == this->EquippedItems.end()) {
		return false;
	}
	
	for (const CUnit *equipped_item : find_iterator->second) {
		if (equipped_item->Unique == unique) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::CanEquipItem(CUnit *item) const
{
	if (item->Container != this) {
		return false;
	}
	
	if (!item->Identified) {
		return false;
	}
	
	if (!this->CanEquipItemClass(item->Type->ItemClass)) {
		return false;
	}
	
	return true;
}

bool CUnit::CanEquipItemClass(const ItemClass *item_class) const
{
	if (item_class == nullptr) {
		return false;
	}
	
	if (item_class->GetSlot() == nullptr) { //can't equip items that don't correspond to an equippable slot
		return false;
	}
	
	if (item_class->GetSlot()->IsWeapon() && std::find(this->Type->WeaponClasses.begin(), this->Type->WeaponClasses.end(), item_class) == this->Type->WeaponClasses.end()) { //if the item is a weapon and its item class isn't a weapon class used by this unit's type, return false
		return false;
	}
	
	if ( //if the item uses the shield (off-hand) slot, but that slot is unavailable for the weapon (because it is two-handed), return false
		item_class->GetSlot()->IsShield()
		&& this->Type->WeaponClasses.size() > 0
		&& this->Type->WeaponClasses[0]->IsTwoHanded()
	) {
		return false;
	}
	
	if ( //if the item is a shield and the weapon of this unit's type is incompatible with shields, return false
		item_class->IsShield()
		&& (
			Type->WeaponClasses.size() == 0
			|| Type->WeaponClasses[0]->IsShieldCompatible() == false
			|| Type->BoolFlag[HARVESTER_INDEX].value //workers can't use shields
		)
	) {
		return false;
	}
	
	if (this->GetItemSlotQuantity(item_class->GetSlot()) == 0) {
		return false;
	}
	
	return true;
}

bool CUnit::CanUseItem(CUnit *item) const
{
	if (item->ConnectingDestination != nullptr) {
		if (item->Type->BoolFlag[ETHEREAL_INDEX].value && !this->Variable[ETHEREALVISION_INDEX].Value) {
			return false;
		}
		
		if (this->Type->BoolFlag[RAIL_INDEX].value && !item->ConnectingDestination->HasAdjacentRailForUnitType(this->Type)) {
			return false;
		}
		
		if (this->Player == item->Player || this->Player->IsAllied(*item->Player) || item->Player->Type == PlayerNeutral) {
			return true;
		}
	}
	
	if (!item->Type->BoolFlag[ITEM_INDEX].value && !item->Type->BoolFlag[POWERUP_INDEX].value) {
		return false;
	}
	
	if (item->Type->BoolFlag[ITEM_INDEX].value && item->Type->ItemClass->IsConsumable() == false) {
		return false;
	}
	
	if (item->Spell != nullptr) {
		if (!this->HasInventory() || !::CanCastSpell(*this, *item->Spell, this, this->GetTilePos(), this->MapLayer)) {
			return false;
		}
	}
	
	if (item->Work != nullptr) {
		if (!this->HasInventory() || this->GetIndividualUpgrade(item->Work)) {
			return false;
		}
	}
	
	if (item->Elixir != nullptr) {
		if (!this->HasInventory() || this->GetIndividualUpgrade(item->Elixir)) {
			return false;
		}
	}
	
	if (item->Elixir == nullptr && item->Variable[HITPOINTHEALING_INDEX].Value > 0 && this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableMax)) {
		return false;
	}
	
	return true;
}

bool CUnit::IsItemSetComplete(const CUnit *item) const
{
	for (const UniqueItem *set_item : item->Unique->Set->UniqueItems) {
		if (!this->IsUniqueItemEquipped(set_item)) {
			return false;
		}
	}

	return true;
}

bool CUnit::EquippingItemCompletesSet(const CUnit *item) const
{
	for (const UniqueItem *set_item : item->Unique->Set->UniqueItems) {
		const ItemSlot *item_slot = set_item->Type->ItemClass->GetSlot();
		
		if (item_slot == nullptr) {
			return false;
		}
		
		bool has_item_equipped = false;
		auto find_iterator = this->EquippedItems.find(item_slot);
		if (find_iterator != this->EquippedItems.end()) {
			for (const CUnit *equipped_item : find_iterator->second) {
				if (equipped_item->Unique == set_item) {
					has_item_equipped = true;
					break;
				}
			}
		}
		
		if (has_item_equipped && set_item == item->Unique) { //if the unique item is already equipped, it won't complete the set (either the set is already complete, or needs something else)
			return false;
		} else if (!has_item_equipped && set_item != item->Unique) {
			return false;
		}
		
	}

	return true;
}

bool CUnit::DeequippingItemBreaksSet(const CUnit *item) const
{
	if (!this->IsItemSetComplete(item)) {
		return false;
	}
	
	const ItemSlot *item_slot = item->Type->ItemClass->GetSlot();
		
	if (item_slot == nullptr) {
		return false;
	}
	
	int item_equipped_quantity = 0;
	
	auto find_iterator = this->EquippedItems.find(item_slot);
	if (find_iterator != this->EquippedItems.end()) {
		for (const CUnit *equipped_item : find_iterator->second) {
			if (equipped_item->Unique == item->Unique) {
				item_equipped_quantity += 1;
			}
		}
	}
	
	if (item_equipped_quantity > 1) {
		return false;
	} else {
		return true;
	}
}

bool CUnit::HasInventory() const
{
	if (this->Type->BoolFlag[INVENTORY_INDEX].value) {
		return true;
	}
	
	if (!this->Type->BoolFlag[FAUNA_INDEX].value) {
		if (this->Character != nullptr) {
			return true;
		}
		
		if (this->Variable[LEVEL_INDEX].Value >= 3 && this->Type->BoolFlag[ORGANIC_INDEX].value) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::CanLearnAbility(const CUpgrade *ability, const bool pre) const
{
	if (!strncmp(ability->Ident.c_str(), "upgrade-deity-", 14)) { //if is a deity choice "ability", only allow for custom heroes (but display the icon for already-acquired deities for all heroes)
		if (!this->Character) {
			return false;
		}
		if (!this->Character->Custom && this->GetIndividualUpgrade(ability) == 0) {
			return false;
		}
		if (!pre && this->UpgradeRemovesExistingUpgrade(ability)) {
			return false;
		}
	}
	
	if (!pre && this->GetIndividualUpgrade(ability) >= ability->MaxLimit) { // already learned
		return false;
	}
	
	if (!pre && this->Variable[LEVELUP_INDEX].Value < 1 && ability->IsAbility()) {
		return false;
	}
	
	if (!CheckDependencies(ability, this, false, pre)) {
		return false;
	}
	
	return true;
}

bool CUnit::CanHireMercenary(CUnitType *type, int civilization_id) const
{
	if (civilization_id == -1) {
		civilization_id = type->GetCivilization() ? type->GetCivilization()->GetIndex() : -1;
	}
	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->Type != PlayerNobody && CPlayer::Players[p]->Type != PlayerNeutral && civilization_id == CPlayer::Players[p]->Race && CheckDependencies(type, CPlayer::Players[p], true) && CPlayer::Players[p]->StartMapLayer == this->GetMapLayer()->GetIndex()) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::CanEat(const CUnit &unit) const
{
	if (this->Type->BoolFlag[CARNIVORE_INDEX].value && unit.GetType()->BoolFlag[FLESH_INDEX].value) {
		return true;
	}
	
	if (this->Type->BoolFlag[INSECTIVORE_INDEX].value && unit.GetType()->BoolFlag[INSECT_INDEX].value) {
		return true;
	}
	
	if (this->Type->BoolFlag[HERBIVORE_INDEX].value && unit.GetType()->BoolFlag[VEGETABLE_INDEX].value) {
		return true;
	}
	
	if (
		this->Type->BoolFlag[DETRITIVORE_INDEX].value
		&& (
			unit.GetType()->BoolFlag[DETRITUS_INDEX].value
			|| (unit.CurrentAction() == UnitActionDie && (unit.GetType()->BoolFlag[FLESH_INDEX].value || unit.GetType()->BoolFlag[INSECT_INDEX].value))
		)
	) {
		return true;
	}
		
	return false;
}

bool CUnit::LevelCheck(const int level) const
{
	if (this->Variable[LEVEL_INDEX].Value == 0) {
		return false;
	}
	
	return SyncRand((this->Variable[LEVEL_INDEX].Value * 2) + 1) >= level;
}

bool CUnit::IsAbilityEmpowered(const CUpgrade *ability) const
{
	const CPlane *plane = this->GetMapLayer()->GetPlane();
	if (plane) {
		if (!plane->GetEmpoweredDeityDomains().empty()) {
			for (const CDeityDomain *deity_domain : ability->DeityDomains) {
				if (plane->GetEmpoweredDeityDomains().find(deity_domain) != plane->GetEmpoweredDeityDomains().end()) {
					return true;
				}
			}
		}
		
		if (!plane->GetEmpoweredSchoolsOfMagic().empty()) {
			for (const CSchoolOfMagic *school_of_magic : ability->SchoolsOfMagic) {
				if (plane->GetEmpoweredSchoolsOfMagic().find(school_of_magic) != plane->GetEmpoweredSchoolsOfMagic().end()) {
					return true;
				}
			}
		}
	}
	
	return false;
}

bool CUnit::IsSpellEmpowered(const CSpell *spell) const
{
	if (spell->DependencyId != -1) {
		return this->IsAbilityEmpowered(CUpgrade::Get(spell->DependencyId));
	} else {
		return false;
	}
}

/**
**  Check if the upgrade removes an existing individual upgrade of the unit.
**
**  @param upgrade    Upgrade.
*/
bool CUnit::UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade) const
{
	for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
		for (size_t j = 0; j < upgrade->UpgradeModifiers[z]->RemoveUpgrades.size(); ++j) {
			if (this->GetIndividualUpgrade(upgrade->UpgradeModifiers[z]->RemoveUpgrades[j]) > 0) {
				return true;
			}
		}
	}
	
	return false;
}

bool CUnit::HasAdjacentRailForUnitType(const CUnitType *type) const
{
	bool has_adjacent_rail = false;
	Vec2i top_left_pos(this->GetTilePos() - Vec2i(1, 1));
	Vec2i bottom_right_pos(this->GetTilePos() + this->Type->TileSize);
			
	for (int x = top_left_pos.x; x <= bottom_right_pos.x; ++x) {
		Vec2i tile_pos(x, top_left_pos.y);
		if (CMap::Map.Info.IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->GetMapLayer()->GetIndex())) {
			has_adjacent_rail = true;
			break;
		}
				
		tile_pos.y = bottom_right_pos.y;
		if (CMap::Map.Info.IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->GetMapLayer()->GetIndex())) {
			has_adjacent_rail = true;
			break;
		}
	}
			
	if (!has_adjacent_rail) {
		for (int y = top_left_pos.y; y <= bottom_right_pos.y; ++y) {
			Vec2i tile_pos(top_left_pos.x, y);
			if (CMap::Map.Info.IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->GetMapLayer()->GetIndex())) {
				has_adjacent_rail = true;
				break;
			}
					
			tile_pos.x = bottom_right_pos.x;
			if (CMap::Map.Info.IsPointOnMap(tile_pos, this->MapLayer) && UnitTypeCanBeAt(*type, tile_pos, this->GetMapLayer()->GetIndex())) {
				has_adjacent_rail = true;
				break;
			}
		}
	}
			
	return has_adjacent_rail;
}

CAnimations *CUnit::GetAnimations() const
{
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && variation->Animations) {
		return variation->Animations;
	} else {
		return this->Type->Animations;
	}
}

CConstruction *CUnit::GetConstruction() const
{
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && variation->Construction) {
		return variation->Construction;
	} else {
		return this->Type->Construction;
	}
}

const CIcon *CUnit::GetIcon() const
{
	if (this->Character != nullptr && this->Character->GetLevel() >= 3 && this->Character->HeroicIcon.Icon) {
		return this->Character->HeroicIcon.Icon;
	} else if (this->Character != nullptr && this->Character->Icon.Icon) {
		return this->Character->Icon.Icon;
	} else if (this->Unique != nullptr && this->Unique->GetSpecificIcon()) {
		return this->Unique->GetSpecificIcon();
	}
	
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && variation->GetIcon() != nullptr) {
		return variation->GetIcon();
	} else {
		return this->Type->GetIcon();
	}
}

const CIcon *CUnit::GetButtonIcon(int button_action) const
{
	if (this->ButtonIcons.find(button_action) != this->ButtonIcons.end()) {
		return this->ButtonIcons.find(button_action)->second;
	} else if (this->Player == CPlayer::GetThisPlayer() && CPlayer::GetThisPlayer()->GetFaction() != nullptr && CPlayer::GetThisPlayer()->GetFaction()->ButtonIcons.find(button_action) != CPlayer::GetThisPlayer()->GetFaction()->ButtonIcons.end()) {
		return CPlayer::GetThisPlayer()->GetFaction()->ButtonIcons.find(button_action)->second.Icon;
	} else if (this->Player == CPlayer::GetThisPlayer() && CCivilization::Get(CPlayer::GetThisPlayer()->Race)->ButtonIcons.find(button_action) != CCivilization::Get(CPlayer::GetThisPlayer()->Race)->ButtonIcons.end()) {
		return CCivilization::Get(CPlayer::GetThisPlayer()->Race)->ButtonIcons.find(button_action)->second.Icon;
	}
	
	return nullptr;
}

MissileConfig CUnit::GetMissile() const
{
	if (this->Variable[FIREDAMAGE_INDEX].Value > 0 && this->Type->FireMissile.Missile) {
		return this->Type->FireMissile;
	} else {
		return this->Type->Missile;
	}
}

CPlayerColorGraphic *CUnit::GetLayerSprite(int image_layer) const
{
	const UnitTypeVariation *layer_variation = this->GetLayerVariation(image_layer);
	if (layer_variation && layer_variation->Sprite) {
		return layer_variation->Sprite;
	}
	
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && variation->LayerSprites[image_layer]) {
		return variation->LayerSprites[image_layer];
	} else if (this->Type->LayerSprites[image_layer])  {
		return this->Type->LayerSprites[image_layer];
	} else {
		return nullptr;
	}
}

std::string CUnit::GetName() const
{
	if (GameRunning && this->Character && this->Character->Deity) {
		if (CPlayer::GetThisPlayer()->Race >= 0) {
			std::string cultural_name = this->Character->Deity->GetCulturalName(CCivilization::Get(CPlayer::GetThisPlayer()->Race)).utf8().get_data();
			
			if (!cultural_name.empty()) {
				return cultural_name;
			}
		}
		
		return this->Character->Deity->GetName().utf8().get_data();
	}
	
	std::string name = this->Name;
	
	if (name.empty()) {
		return name;
	}
	
	if (!this->ExtraName.empty()) {
		name += " ";
		name += this->ExtraName;
	}

	if (!this->FamilyName.empty()) {
		name += " ";
		name += this->FamilyName;
	}
	
	return name;
}

std::string CUnit::GetTypeName() const
{
	if (this->Character && this->Character->Deity) {
		return _("Deity");
	}
	
	const UnitTypeVariation *variation = this->GetVariation();
	if (variation && !variation->TypeName.empty()) {
		return _(variation->TypeName.c_str());
	} else {
		return _(this->Type->GetName().utf8().get_data());
	}
}

std::string CUnit::GetMessageName() const
{
	std::string name = GetName();
	if (name.empty()) {
		return GetTypeName();
	}
	
	if (!this->Identified) {
		return GetTypeName() + " (" + _("Unidentified") + ")";
	}
	
	if (!this->Unique && this->Work == nullptr && (this->Prefix != nullptr || this->Suffix != nullptr || this->Spell != nullptr)) {
		return name;
	}
	
	return name + " (" + GetTypeName() + ")";
}

const CLanguage *CUnit::GetLanguage() const
{
	const CCivilization *civilization = this->Type->GetCivilization();
	
	const CFaction *faction = nullptr;
	if (this->Player->GetFaction() != nullptr) {
		faction = this->Player->GetFaction();
		
		if (civilization != nullptr && civilization != faction->GetCivilization() && civilization->GetSpecies() == faction->GetCivilization()->GetSpecies() && this->Type == CFaction::GetFactionClassUnitType(faction, this->Type->GetClass())) {
			civilization = faction->GetCivilization();
		}
	}
	
	if (civilization != nullptr) {
		return civilization->GetLanguage();
	}
	
	return nullptr;
}
//Wyrmgus end

bool CUnit::IsDiurnal() const
{
	return this->Variable[DAYSIGHTRANGEBONUS_INDEX].Value > 0 && this->Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value < 0;
}

bool CUnit::IsNocturnal() const
{
	return this->Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value > 0 && this->Variable[DAYSIGHTRANGEBONUS_INDEX].Value < 0;
}

/**
**  Let an unit die.
**
**  @param unit    Unit to be destroyed.
*/
void LetUnitDie(CUnit &unit, bool suicide)
{
	unit.Variable[HP_INDEX].Value = std::min<int>(0, unit.Variable[HP_INDEX].Value);
	unit.Moving = 0;
	unit.TTL = 0;
	unit.Anim.Unbreakable = 0;

	const CUnitType *type = unit.GetType();

	while (unit.Resource.Workers) {
		unit.Resource.Workers->DeAssignWorkerFromMine(unit);
	}

	// removed units,  just remove.
	if (unit.Removed) {
		DebugPrint("Killing a removed unit?\n");
		if (unit.UnitInside) {
			DestroyAllInside(unit);
		}
		UnitLost(unit);
		UnitClearOrders(unit);
		unit.Release();
		return;
	}

	PlayUnitSound(unit, VoiceDying);

	//
	// Catapults,... explodes.
	//
	if (type->ExplodeWhenKilled) {
		const PixelPos pixelPos = unit.GetMapPixelPosCenter();

		MakeMissile(*type->Explosion.Missile, pixelPos, pixelPos, unit.GetMapLayer()->GetIndex());
	}
	if (type->DeathExplosion) {
		const PixelPos pixelPos = unit.GetMapPixelPosCenter();

		type->DeathExplosion->pushPreamble();
		//Wyrmgus start
		type->DeathExplosion->pushInteger(UnitNumber(unit));
		//Wyrmgus end
		type->DeathExplosion->pushInteger(pixelPos.x);
		type->DeathExplosion->pushInteger(pixelPos.y);
		type->DeathExplosion->run();
	}
	if (suicide) {
		const PixelPos pixelPos = unit.GetMapPixelPosCenter();
		
		if (unit.GetMissile().Missile) {
			MakeMissile(*unit.GetMissile().Missile, pixelPos, pixelPos, unit.GetMapLayer()->GetIndex());
		}
	}
	// Handle Teleporter Destination Removal
	if (type->BoolFlag[TELEPORTER_INDEX].value && unit.Goal) {
		unit.Goal->Remove(nullptr);
		UnitLost(*unit.Goal);
		UnitClearOrders(*unit.Goal);
		unit.Goal->Release();
		unit.Goal = nullptr;
	}
	
	//Wyrmgus start
	for (size_t i = 0; i < unit.SoldUnits.size(); ++i) {
		DestroyAllInside(*unit.SoldUnits[i]);
		LetUnitDie(*unit.SoldUnits[i]);
	}
	unit.SoldUnits.clear();
	//Wyrmgus end

	// Transporters lose or save their units and buildings their workers
	//Wyrmgus start
//	if (unit.UnitInside && unit.GetType()->BoolFlag[SAVECARGO_INDEX].value) {
	if (
		unit.UnitInside
		&& (
			unit.GetType()->BoolFlag[SAVECARGO_INDEX].value
			|| (unit.HasInventory() && unit.Character == nullptr)
		)
	) {
	//Wyrmgus end
		DropOutAll(unit);
	} else if (unit.UnitInside) {
		DestroyAllInside(unit);
	}
	
	//Wyrmgus start
	//if is a raft or bridge, destroy all land units on it
	if (unit.GetType()->BoolFlag[BRIDGE_INDEX].value) {
		std::vector<CUnit *> table;
		Select(unit.GetTilePos(), unit.GetTilePos(), table, unit.GetMapLayer()->GetIndex());
		for (size_t i = 0; i != table.size(); ++i) {
			if (table[i]->IsAliveOnMap() && !table[i]->GetType()->BoolFlag[BRIDGE_INDEX].value && table[i]->GetType()->UnitType == UnitTypeLand) {
				table[i]->Variable[HP_INDEX].Value = std::min<int>(0, unit.Variable[HP_INDEX].Value);
				table[i]->Moving = 0;
				table[i]->TTL = 0;
				table[i]->Anim.Unbreakable = 0;
				PlayUnitSound(*table[i], VoiceDying);
				table[i]->Remove(nullptr);
				UnitLost(*table[i]);
				UnitClearOrders(*table[i]);
				table[i]->Release();
			}
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	//drop items upon death
	if (!suicide && unit.CurrentAction() != UnitActionBuilt && (unit.Character || unit.GetType()->BoolFlag[BUILDING_INDEX].value || SyncRand(100) >= 66)) { //66% chance nothing will be dropped, unless the unit is a character or building, in which it case it will always drop an item
		unit.GenerateDrop();
	}
	//Wyrmgus end
	
	//Wyrmgus start
	std::vector<CUnit *> seeing_table; //units seeing this unit
	if (type->BoolFlag[AIRUNPASSABLE_INDEX].value) {
		SelectAroundUnit(unit, 16, seeing_table); //a range of 16 should be safe enough; there should be no unit or building in the game with a sight range that high, let alone higher
		for (size_t i = 0; i != seeing_table.size(); ++i) {
			MapUnmarkUnitSight(*seeing_table[i]);
		}
	}
	//Wyrmgus end

	unit.Remove(nullptr);
	UnitLost(unit);
	UnitClearOrders(unit);


	// Unit has death animation.

	// Not good: UnitUpdateHeading(unit);
	delete unit.Orders[0];
	unit.Orders[0] = COrder::NewActionDie();
	if (type->CorpseType) {
#ifdef DYNAMIC_LOAD
		if (!type->Sprite) {
			LoadUnitTypeSprite(type);
		}
#endif
		unit.SetPixelOffset((type->CorpseType->GetFrameSize().x - type->CorpseType->Sprite->Width) / 2, (type->CorpseType->GetFrameSize().y - type->CorpseType->Sprite->Height) / 2);

		unit.CurrentSightRange = type->CorpseType->Stats[unit.GetPlayer()->GetIndex()].Variables[SIGHTRANGE_INDEX].Max;
	} else {
		unit.CurrentSightRange = 0;
	}

	// If we have a corpse, or a death animation, we are put back on the map
	// This enables us to be tracked.  Possibly for spells (eg raise dead)
	//Wyrmgus start
//	if (type->CorpseType || (type->Animations && type->Animations->Death)) {
	if (type->CorpseType || (unit.GetAnimations() && unit.GetAnimations()->Death)) {
	//Wyrmgus end
		unit.Removed = 0;
		CMap::Map.Insert(unit);

		// FIXME: rb: Maybe we need this here because corpse of cloaked units
		//	may crash Sign code

		// Recalculate the seen count.
		//UnitCountSeen(unit);
	}
	
	MapMarkUnitSight(unit);
	
	//Wyrmgus start
	if (unit.Settlement) {
		unit.UpdateBuildingSettlementAssignment(unit.Settlement);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	if (type->BoolFlag[AIRUNPASSABLE_INDEX].value) {
		for (size_t i = 0; i != seeing_table.size(); ++i) {
			MapMarkUnitSight(*seeing_table[i]);
		}
	}
	//Wyrmgus end
}

/**
**  Destroy all units inside unit.
**
**  @param source  container.
*/
void DestroyAllInside(CUnit &source)
{
	CUnit *unit = source.UnitInside;

	// No Corpses, we are inside something, and we can't be seen
	for (int i = source.InsideCount; i; --i, unit = unit->NextContained) {
		// Transporter inside a transporter?
		if (unit->UnitInside) {
			DestroyAllInside(*unit);
		}
		UnitLost(*unit);
		UnitClearOrders(*unit);
		unit->Release();
	}
}

/*----------------------------------------------------------------------------
  -- Unit AI
  ----------------------------------------------------------------------------*/

int ThreatCalculate(const CUnit &unit, const CUnit &dest)
{
	const CUnitType &type = *unit.GetType();
	const CUnitType &dtype = *dest.GetType();
	int cost = 0;

	// Buildings, non-aggressive and invincible units have the lowest priority
	if (dest.IsAgressive() == false || dest.Variable[UNHOLYARMOR_INDEX].Value > 0
		|| dest.GetType()->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
		if (dest.GetType()->CanMove() == false) {
			return INT_MAX;
		} else {
			return INT_MAX / 2;
		}
	}

	// Priority 0-255
	cost -= dest.Variable[PRIORITY_INDEX].Value * PRIORITY_FACTOR;
	// Remaining HP (Health) 0-65535
	//Wyrmgus start
//	cost += dest.Variable[HP_INDEX].Value * 100 / dest.Variable[HP_INDEX].Max * HEALTH_FACTOR;
	cost += dest.Variable[HP_INDEX].Value * 100 / dest.GetModifiedVariable(HP_INDEX, VariableMax) * HEALTH_FACTOR;
	//Wyrmgus end

	const int d = unit.MapDistanceTo(dest);

	if (d <= unit.GetModifiedVariable(ATTACKRANGE_INDEX) && d >= type.MinAttackRange) {
		cost += d * INRANGE_FACTOR;
		cost -= INRANGE_BONUS;
	} else {
		cost += d * DISTANCE_FACTOR;
	}

	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
		if (type.BoolFlag[i].AiPriorityTarget != CONDITION_TRUE) {
			if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_ONLY) & (dtype.BoolFlag[i].value)) {
				cost -= AIPRIORITY_BONUS;
			}
			if ((type.BoolFlag[i].AiPriorityTarget == CONDITION_FALSE) & (dtype.BoolFlag[i].value)) {
				cost += AIPRIORITY_BONUS;
			}
		}
	}

	// Unit can attack back.
	if (CanTarget(dtype, type)) {
		cost -= CANATTACK_BONUS;
	}
	return cost;
}

static void HitUnit_LastAttack(const CUnit *attacker, CUnit &target)
{
	const unsigned long lastattack = target.Attacked;

	target.Attacked = GameCycle ? GameCycle : 1;
	if (target.GetType()->BoolFlag[WALL_INDEX].value || (lastattack && GameCycle <= lastattack + 2 * CYCLES_PER_SECOND)) {
		return;
	}
	// NOTE: perhaps this should also be moved into the notify?
	if (target.GetPlayer() == CPlayer::GetThisPlayer()) {
		// FIXME: Problem with load+save.

		//
		// One help cry each 2 second is enough
		// If on same area ignore it for 2 minutes.
		//
		if (HelpMeLastCycle < GameCycle) {
			if (!HelpMeLastCycle
				|| HelpMeLastCycle + CYCLES_PER_SECOND * 120 < GameCycle
				|| target.GetTilePos().x < HelpMeLastX - 14
				|| target.GetTilePos().x > HelpMeLastX + 14
				|| target.GetTilePos().y < HelpMeLastY - 14
				|| target.GetTilePos().y > HelpMeLastY + 14) {
				HelpMeLastCycle = GameCycle + CYCLES_PER_SECOND * 2;
				HelpMeLastX = target.GetTilePos().x;
				HelpMeLastY = target.GetTilePos().y;
				PlayUnitSound(target, VoiceHelpMe);
				target.GetPlayer()->Notify(NotifyRed, target.GetTilePos(), target.GetMapLayer()->GetIndex(), _("%s attacked"), target.GetMessageName().c_str());
			}
		}
	}

	if (GameCycle > (lastattack + 2 * (CYCLES_PER_SECOND * 60)) && attacker && !target.GetType()->BoolFlag[BUILDING_INDEX].value) { //only trigger this every two minutes for the unit
		if (
			target.GetPlayer()->AiEnabled
			&& !attacker->GetType()->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
		) {
			AiHelpMe(attacker->GetFirstContainer(), target);
		}
	}
}

static void HitUnit_Raid(CUnit *attacker, CUnit &target, int damage)
{
	if (!attacker) {
		return;
	}
	
	if (attacker->GetPlayer() == target.GetPlayer() || attacker->GetPlayer()->GetIndex() == PlayerNumNeutral || target.GetPlayer()->GetIndex() == PlayerNumNeutral) {
		return;
	}
	
	int var_index;
	if (target.GetType()->BoolFlag[BUILDING_INDEX].value) {
		var_index = RAIDING_INDEX;
	} else {
		var_index = MUGGING_INDEX;
	}
	
	if (!attacker->Variable[var_index].Value) {
		return;
	}
	
	if (!attacker->Variable[SHIELDPIERCING_INDEX].Value) {
		int shieldDamage = target.Variable[SHIELDPERMEABILITY_INDEX].Value < 100
							? std::min(target.Variable[SHIELD_INDEX].Value, damage * (100 - target.Variable[SHIELDPERMEABILITY_INDEX].Value) / 100)
							: 0;
		
		damage -= shieldDamage;
	}
	
	damage = std::min(damage, target.Variable[HP_INDEX].Value);
	
	if (damage <= 0) {
		return;
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		if (target.GetType()->Stats[target.GetPlayer()->GetIndex()].Costs[i] > 0) {
			int resource_change = target.GetType()->Stats[target.GetPlayer()->GetIndex()].Costs[i] * damage * attacker->Variable[var_index].Value / target.GetModifiedVariable(HP_INDEX, VariableMax) / 100;
			resource_change = std::min(resource_change, target.GetPlayer()->GetResource(i, STORE_BOTH));
			attacker->GetPlayer()->ChangeResource(i, resource_change);
			attacker->GetPlayer()->TotalResources[i] += resource_change;
			target.GetPlayer()->ChangeResource(i, -resource_change);
		}
	}
}

static bool HitUnit_IsUnitWillDie(const CUnit *attacker, const CUnit &target, int damage)
{
	int shieldDamage = target.Variable[SHIELDPERMEABILITY_INDEX].Value < 100
					   ? std::min(target.Variable[SHIELD_INDEX].Value, damage * (100 - target.Variable[SHIELDPERMEABILITY_INDEX].Value) / 100)
					   : 0;
	return (target.Variable[HP_INDEX].Value <= damage && attacker && attacker->Variable[SHIELDPIERCING_INDEX].Value)
		   || (target.Variable[HP_INDEX].Value <= damage - shieldDamage)
		   || (target.Variable[HP_INDEX].Value == 0);
}

static void HitUnit_IncreaseScoreForKill(CUnit &attacker, CUnit &target)
{
	attacker.GetPlayer()->Score += target.Variable[POINTS_INDEX].Value;
	if (target.GetType()->BoolFlag[BUILDING_INDEX].value) {
		attacker.GetPlayer()->TotalRazings++;
	} else {
		attacker.GetPlayer()->TotalKills++;
	}
	
	//Wyrmgus start
	attacker.GetPlayer()->UnitTypeKills[target.GetType()->GetIndex()]++;
	
	//distribute experience between nearby units belonging to the same player
	if (!target.GetType()->BoolFlag[BUILDING_INDEX].value) {
		attacker.ChangeExperience(UseHPForXp ? target.Variable[HP_INDEX].Value : target.Variable[POINTS_INDEX].Value, ExperienceRange);
	}
	//Wyrmgus end
	
	attacker.Variable[KILL_INDEX].Value++;
	attacker.Variable[KILL_INDEX].Max++;
	attacker.Variable[KILL_INDEX].Enable = 1;
	
	//Wyrmgus start
	for (CPlayerQuestObjective *objective : attacker.GetPlayer()->QuestObjectives) {
		if (
			(
				objective->ObjectiveType == DestroyUnitsObjectiveType
				&& (
					std::find(objective->UnitTypes.begin(), objective->UnitTypes.end(), target.GetType()) != objective->UnitTypes.end()
					|| std::find(objective->UnitClasses.begin(), objective->UnitClasses.end(), target.GetType()->GetClass()) != objective->UnitClasses.end()
				)
				&& (!objective->Settlement || objective->Settlement == target.Settlement)
			)
			|| (objective->ObjectiveType == DestroyHeroObjectiveType && target.Character && objective->Character == target.Character)
			|| (objective->ObjectiveType == DestroyUniqueObjectiveType && target.Unique && objective->Unique == target.Unique)
		) {
			if (!objective->Faction || objective->Faction == target.GetPlayer()->GetFaction()) {
				objective->Counter = std::min(objective->Counter + 1, objective->Quantity);
			}
		} else if (objective->ObjectiveType == DestroyFactionObjectiveType) {
			const CPlayer *faction_player = CPlayer::GetFactionPlayer(objective->Faction);
			
			if (faction_player) {
				int dying_faction_units = faction_player == target.GetPlayer() ? 1 : 0;
				dying_faction_units += target.GetTotalInsideCount(faction_player, true, true);
				
				if (dying_faction_units > 0 && faction_player->GetUnitCount() <= dying_faction_units) {
					objective->Counter = 1;
				}
			}
		}
	}
	
	//also increase score for units inside the target that will be destroyed when the target dies
	if (
		target.UnitInside
		&& !target.GetType()->BoolFlag[SAVECARGO_INDEX].value
	) {
		CUnit *boarded_unit = target.UnitInside;
		for (int i = 0; i < target.InsideCount; ++i, boarded_unit = boarded_unit->NextContained) {
			if (!boarded_unit->GetType()->BoolFlag[ITEM_INDEX].value) { //ignore items
				HitUnit_IncreaseScoreForKill(attacker, *boarded_unit);
			}
		}
	}
	//Wyrmgus end
}

static void HitUnit_ApplyDamage(CUnit *attacker, CUnit &target, int damage)
{
	if (attacker && attacker->Variable[SHIELDPIERCING_INDEX].Value) {
		target.Variable[HP_INDEX].Value -= damage;
	} else {
		int shieldDamage = target.Variable[SHIELDPERMEABILITY_INDEX].Value < 100
						   ? std::min(target.Variable[SHIELD_INDEX].Value, damage * (100 - target.Variable[SHIELDPERMEABILITY_INDEX].Value) / 100)
						   : 0;
		if (shieldDamage) {
			target.Variable[SHIELD_INDEX].Value -= shieldDamage;
			target.Variable[SHIELD_INDEX].Value = std::clamp(target.Variable[SHIELD_INDEX].Value, 0, target.Variable[SHIELD_INDEX].Max);
		}
		target.Variable[HP_INDEX].Value -= damage - shieldDamage;
	}
	
	//Wyrmgus start
	//distribute experience between nearby units belonging to the same player

//	if (UseHPForXp && attacker && target.IsEnemy(*attacker)) {
	if (UseHPForXp && attacker && (target.IsEnemy(*attacker) || target.GetPlayer()->Type == PlayerNeutral) && !target.GetType()->BoolFlag[BUILDING_INDEX].value) {
		attacker->ChangeExperience(damage, ExperienceRange);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	//use a healing item if any are available
	if (target.HasInventory()) {
		target.HealingItemAutoUse();
	}
	//Wyrmgus end
}

static void HitUnit_BuildingCapture(CUnit *attacker, CUnit &target, int damage)
{
	// FIXME: this is dumb. I made repairers capture. crap.
	// david: capture enemy buildings
	// Only worker types can capture.
	// Still possible to destroy building if not careful (too many attackers)
	if (EnableBuildingCapture && attacker
		&& target.GetType()->BoolFlag[BUILDING_INDEX].value && target.Variable[HP_INDEX].Value <= damage * 3
		&& attacker->IsEnemy(target)
		&& attacker->GetType()->RepairRange) {
		target.ChangeOwner(*attacker->GetPlayer());
		CommandStopUnit(*attacker); // Attacker shouldn't continue attack!
	}
}

static void HitUnit_ShowDamageMissile(const CUnit &target, int damage)
{
	const PixelPos targetPixelCenter = target.GetMapPixelPosCenter();

	if ((target.IsVisibleOnMap(*CPlayer::GetThisPlayer()) || ReplayRevealMap) && !DamageMissile.empty()) {
		const MissileType *mtype = MissileTypeByIdent(DamageMissile);
		const PixelDiff offset(3, -mtype->Range);

		MakeLocalMissile(*mtype, targetPixelCenter, targetPixelCenter + offset, target.GetMapLayer()->GetIndex())->Damage = -damage;
	}
}

static void HitUnit_ShowImpactMissile(const CUnit &target)
{
	const PixelPos targetPixelCenter = target.GetMapPixelPosCenter();
	const CUnitType &type = *target.GetType();

	if (target.Variable[SHIELD_INDEX].Value > 0
		&& !type.Impact[ANIMATIONS_DEATHTYPES + 1].Name.empty()) { // shield impact
		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES + 1].Missile, targetPixelCenter, targetPixelCenter, target.GetMapLayer()->GetIndex());
	} else if (target.DamagedType && !type.Impact[target.DamagedType].Name.empty()) { // specific to damage type impact
		MakeMissile(*type.Impact[target.DamagedType].Missile, targetPixelCenter, targetPixelCenter, target.GetMapLayer()->GetIndex());
	} else if (!type.Impact[ANIMATIONS_DEATHTYPES].Name.empty()) { // generic impact
		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES].Missile, targetPixelCenter, targetPixelCenter, target.GetMapLayer()->GetIndex());
	}
}

static void HitUnit_ChangeVariable(CUnit &target, const Missile &missile)
{
	const int var = missile.Type->ChangeVariable;

	target.Variable[var].Enable = 1;
	target.Variable[var].Value += missile.Type->ChangeAmount;
	if (target.Variable[var].Value > target.Variable[var].Max) {
		if (missile.Type->ChangeMax) {
			target.Variable[var].Max = target.Variable[var].Value;
		//Wyrmgus start
//		} else {
		} else if (target.Variable[var].Value > target.GetModifiedVariable(var, VariableMax)) {
		//Wyrmgus end
			//Wyrmgus start
//			target.Variable[var].Value = target.Variable[var].Max;
			target.Variable[var].Value = target.GetModifiedVariable(var, VariableMax);
			//Wyrmgus end
		}
	}
	
	//Wyrmgus start
	if (var == ATTACKRANGE_INDEX && target.Container) {
		target.Container->UpdateContainerAttackRange();
	} else if (var == LEVEL_INDEX || var == POINTS_INDEX) {
		target.UpdateXPRequired();
	} else if (var == XP_INDEX) {
		target.XPChanged();
	} else if (var == STUN_INDEX && target.Variable[var].Value > 0) { //if unit has become stunned, stop it
		CommandStopUnit(target);
	} else if (var == KNOWLEDGEMAGIC_INDEX) {
		target.CheckIdentification();
	}
	//Wyrmgus end
}


static void HitUnit_Burning(CUnit &target)
{
	//Wyrmgus start
//	const int f = (100 * target.Variable[HP_INDEX].Value) / target.Variable[HP_INDEX].Max;
	const int f = (100 * target.Variable[HP_INDEX].Value) / target.GetModifiedVariable(HP_INDEX, VariableMax);
	//Wyrmgus end
	MissileType *fire = MissileBurningBuilding(f);

	if (fire) {
		const PixelPos targetPixelCenter = target.GetMapPixelPosCenter();
		const PixelDiff offset(0, -CMap::Map.GetMapLayerPixelTileSize(target.GetMapLayer()->GetIndex()).y);
		Missile *missile = MakeMissile(*fire, targetPixelCenter + offset, targetPixelCenter + offset, target.GetMapLayer()->GetIndex());

		missile->SourceUnit = &target;
		target.Burning = 1;
	}
}

//Wyrmgus start
void HitUnit_NormalHitSpecialDamageEffects(CUnit &attacker, CUnit &target)
{
	if (attacker.Variable[FIREDAMAGE_INDEX].Value > 0 && attacker.Variable[FIREDAMAGE_INDEX].Value >= attacker.Variable[COLDDAMAGE_INDEX].Value) { // apply only either the fire damage or cold damage effects, but not both at the same time; apply the one with greater value, but if both are equal fire damage takes precedence
		HitUnit_SpecialDamageEffect(target, FIREDAMAGE_INDEX);
	} else if (attacker.Variable[COLDDAMAGE_INDEX].Value > 0) {
		HitUnit_SpecialDamageEffect(target, COLDDAMAGE_INDEX);
	}
	
	if (attacker.Variable[LIGHTNINGDAMAGE_INDEX].Value > 0) {
		HitUnit_SpecialDamageEffect(target, LIGHTNINGDAMAGE_INDEX);
	}
}

void HitUnit_SpecialDamageEffect(CUnit &target, int dmg_var)
{
	if (dmg_var == COLDDAMAGE_INDEX && target.Variable[COLDRESISTANCE_INDEX].Value < 100 && target.GetType()->BoolFlag[ORGANIC_INDEX].value) { //if resistance to cold is 100%, the effect has no chance of being applied
		int rand_max = 100 * 100 / (100 - target.Variable[COLDRESISTANCE_INDEX].Value);
		if (SyncRand(rand_max) == 0) {
			target.Variable[SLOW_INDEX].Enable = 1;
			target.Variable[SLOW_INDEX].Value = std::max(200, target.Variable[SLOW_INDEX].Value);
			target.Variable[SLOW_INDEX].Max = 1000;
		}
	} else if (dmg_var == LIGHTNINGDAMAGE_INDEX && target.Variable[LIGHTNINGRESISTANCE_INDEX].Value < 100 && target.GetType()->BoolFlag[ORGANIC_INDEX].value) {
		int rand_max = 100 * 100 / (100 - target.Variable[LIGHTNINGRESISTANCE_INDEX].Value);
		if (SyncRand(rand_max) == 0) {
			target.Variable[STUN_INDEX].Enable = 1;
			target.Variable[STUN_INDEX].Value = std::max(50, target.Variable[STUN_INDEX].Value);
			target.Variable[STUN_INDEX].Max = 1000;
		}
	}
}
//Wyrmgus end

//Wyrmgus start
//static void HitUnit_RunAway(CUnit &target, const CUnit &attacker)
void HitUnit_RunAway(CUnit &target, const CUnit &attacker)
//Wyrmgus end
{
	Vec2i pos = target.GetTilePos() - attacker.GetTilePos();
	int d = isqrt(pos.x * pos.x + pos.y * pos.y);

	if (!d) {
		d = 1;
	}
	pos.x = target.GetTilePos().x + (pos.x * 5) / d + (SyncRand() & 3);
	pos.y = target.GetTilePos().y + (pos.y * 5) / d + (SyncRand() & 3);
	CMap::Map.Clamp(pos, target.GetMapLayer()->GetIndex());
	CommandStopUnit(target);
	CommandMove(target, pos, 0, target.GetMapLayer()->GetIndex());
}

static void HitUnit_AttackBack(CUnit &attacker, CUnit &target)
{
	const int threshold = 30;
	COrder *savedOrder = nullptr;

	//Wyrmgus start
//	if (target.GetPlayer()->AiEnabled == false) {
	if (target.GetPlayer()->AiEnabled == false && target.GetPlayer()->Type != PlayerNeutral) { // allow neutral units to strike back
	//Wyrmgus end
		if (target.CurrentAction() == UnitActionAttack) {
			COrder_Attack &order = dynamic_cast<COrder_Attack &>(*target.CurrentOrder());
			if (order.IsWeakTargetSelected() == false) {
				return;
			}
		//Wyrmgus start
//		} else {
		} else if (target.CurrentAction() != UnitActionStill) {
		//Wyrmgus end
			return;
		}
	}
	if (target.CanStoreOrder(target.CurrentOrder())) {
		savedOrder = target.CurrentOrder()->Clone();
	}
	CUnit *oldgoal = target.CurrentOrder()->GetGoal();
	CUnit *goal, *best = oldgoal;

	if (RevealAttacker && CanTarget(*target.GetType(), *attacker.GetType())) {
		// Reveal Unit that is attacking
		goal = &attacker;
	} else {
		if (target.CurrentAction() == UnitActionStandGround) {
			goal = AttackUnitsInRange(target);
		} else {
			// Check for any other units in range
			goal = AttackUnitsInReactRange(target);
		}
	}

	// Calculate the best target we could attack
	if (!best || (goal && (ThreatCalculate(target, *goal) < ThreatCalculate(target, *best)))) {
		best = goal;
	}
	if (CanTarget(*target.GetType(), *attacker.GetType())
		&& (!best || (goal != &attacker
					  && (ThreatCalculate(target, attacker) < ThreatCalculate(target, *best))))) {
		best = &attacker;
	}
	//Wyrmgus start
//	if (best && best != oldgoal && best->GetPlayer() != target.GetPlayer() && best->IsAllied(target) == false) {
	if (best && best != oldgoal && (best->GetPlayer() != target.GetPlayer() || target.GetPlayer()->Type == PlayerNeutral) && best->IsAllied(target) == false) {
	//Wyrmgus end
		CommandAttack(target, best->GetTilePos(), best, FlushCommands, best->GetMapLayer()->GetIndex());
		// Set threshold value only for aggressive units
		if (best->IsAgressive()) {
			target.Threshold = threshold;
		}
		if (savedOrder != nullptr) {
			target.SavedOrder = savedOrder;
		}
	}
}

/**
**  Unit is hit by missile or other damage.
**
**  @param attacker    Unit that attacks.
**  @param target      Unit that is hit.
**  @param damage      How many damage to take.
**  @param missile     Which missile took the damage.
*/
//Wyrmgus start
//void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile)
void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile, bool show_damage)
//Wyrmgus end
{
	const CUnitType *type = target.GetType();
	if (!damage) {
		// Can now happen by splash damage
		// Multiple places send x/y as damage, which may be zero
		return;
	}

	if (target.Variable[UNHOLYARMOR_INDEX].Value > 0 || target.GetType()->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
		// vladi: units with active UnholyArmour are invulnerable
		return;
	}
	if (target.Removed) {
		DebugPrint("Removed target hit\n");
		return;
	}

	Assert(damage != 0 && target.CurrentAction() != UnitActionDie && !target.GetType()->BoolFlag[VANISHES_INDEX].value);

	if (
		(attacker != nullptr && attacker->GetPlayer() == CPlayer::GetThisPlayer())
		&& target.GetPlayer() != CPlayer::GetThisPlayer()
	) {
		Wyrmgus::GetInstance()->emit_signal("unit_hit");
	}

	if (GodMode) {
		if (attacker && attacker->GetPlayer() == CPlayer::GetThisPlayer()) {
			damage = target.Variable[HP_INDEX].Value;
		}
		if (target.GetPlayer() == CPlayer::GetThisPlayer()) {
			damage = 0;
		}
	}
	//Wyrmgus start
//	HitUnit_LastAttack(attacker, target);
	//Wyrmgus end
	if (attacker) {
		//Wyrmgus start
		HitUnit_LastAttack(attacker, target); //only trigger the help me notification and AI code if there is actually an attacker
		//Wyrmgus end
		target.DamagedType = ExtraDeathIndex(attacker->GetType()->DamageType.c_str());
	}

	// Increase variables and call OnImpact
	if (missile && missile->Type) {
		if (missile->Type->ChangeVariable != -1) {
			HitUnit_ChangeVariable(target, *missile);
		}
		if (missile->Type->OnImpact) {
			const int attackerSlot = attacker ? UnitNumber(*attacker) : -1;
			const int targetSlot = UnitNumber(target);
			missile->Type->OnImpact->pushPreamble();
			missile->Type->OnImpact->pushInteger(attackerSlot);
			missile->Type->OnImpact->pushInteger(targetSlot);
			missile->Type->OnImpact->pushInteger(damage);
			missile->Type->OnImpact->run();
		}
	}
	
	HitUnit_Raid(attacker, target, damage);

	if (HitUnit_IsUnitWillDie(attacker, target, damage)) { // unit is killed or destroyed
		if (attacker) {
			//  Setting ai threshold counter to 0 so it can target other units
			attacker->Threshold = 0;
		}
		
		CUnit *destroyer = attacker;
		if (!destroyer) {
			int best_distance = 0;
			std::vector<CUnit *> table;
			SelectAroundUnit(target, ExperienceRange, table, IsEnemyWith(*target.GetPlayer()));
			for (size_t i = 0; i < table.size(); i++) {
				CUnit *potential_destroyer = table[i];
				int distance = target.MapDistanceTo(*potential_destroyer);
				if (!destroyer || distance < best_distance) {
					destroyer = potential_destroyer;
					best_distance = distance;
				}
			}
		}
		if (destroyer) {
			if (target.IsEnemy(*destroyer) || target.GetPlayer()->Type == PlayerNeutral) {
				HitUnit_IncreaseScoreForKill(*destroyer, target);
			}
		}
		LetUnitDie(target);
		return;
	}

	HitUnit_ApplyDamage(attacker, target, damage);
	HitUnit_BuildingCapture(attacker, target, damage);
	//Wyrmgus start
//	HitUnit_ShowDamageMissile(target, damage);
	if (show_damage) {
		HitUnit_ShowDamageMissile(target, damage);
	}
	//Wyrmgus end

	HitUnit_ShowImpactMissile(target);

	//Wyrmgus start
//	if (type->BoolFlag[BUILDING_INDEX].value && !target.Burning) {
	if (type->BoolFlag[BUILDING_INDEX].value && !target.Burning && !target.UnderConstruction && target.GetType()->TileSize.x != 1 && target.GetType()->TileSize.y != 1) { //the building shouldn't burn if it's still under construction, or if it's too small
	//Wyrmgus end
		HitUnit_Burning(target);
	}

	/* Target Reaction on Hit */
	if (target.GetPlayer()->AiEnabled) {
		if (target.CurrentOrder()->OnAiHitUnit(target, attacker, damage)) {
			return;
		}
	}

	if (!attacker) {
		return;
	}

	// Can't attack run away.
	//Wyrmgus start
//	if (!target.IsAgressive() && target.CanMove() && target.CurrentAction() == UnitActionStill && !target.BoardCount) {
	if (
		(!target.IsAgressive() || attacker->GetType()->BoolFlag[INDESTRUCTIBLE_INDEX].value)
		&& target.CanMove()
		&& (target.CurrentAction() == UnitActionStill || target.Variable[TERROR_INDEX].Value > 0)
		&& !target.BoardCount
		&& !target.GetType()->BoolFlag[BRIDGE_INDEX].value
	) {
	//Wyrmgus end
		HitUnit_RunAway(target, *attacker);
	}

	const int threshold = 30;

	if (target.Threshold && target.CurrentOrder()->HasGoal() && target.CurrentOrder()->GetGoal() == attacker) {
		target.Threshold = threshold;
		return;
	}

	//Wyrmgus start
//	if (target.Threshold == 0 && target.IsAgressive() && target.CanMove() && !target.ReCast) {
	if (
		target.Threshold == 0
		&& (target.IsAgressive() || (target.CanAttack() && target.GetType()->BoolFlag[COWARD_INDEX].value && (attacker->GetType()->BoolFlag[COWARD_INDEX].value || attacker->Variable[HP_INDEX].Value <= 3))) // attacks back if isn't coward, or if attacker is also coward, or if attacker has 3 HP or less 
		&& target.CanMove()
		&& !target.ReCast
		&& !attacker->GetType()->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
	) {
	//Wyrmgus end
		// Attack units in range (which or the attacker?)
		// Don't bother unit if it casting repeatable spell
		HitUnit_AttackBack(*attacker->GetFirstContainer(), target); //if the unit is in a container, attack it instead of the unit (which is removed and thus unreachable)
	}

	// What should we do with workers on :
	// case UnitActionRepair:
	// Drop orders and run away or return after escape?
}

/*----------------------------------------------------------------------------
--  Conflicts
----------------------------------------------------------------------------*/

/**
 **	@brief	Returns the map distance between this unit and another one
 **
 **	@param	dst	The unit the distance to which is to be obtained
 **
 **	@return	The distance to the other unit, in tiles
 */
int CUnit::MapDistanceTo(const CUnit &dst) const
{
	if (this->MapLayer != dst.MapLayer) {
		return 16384;
	}
	
	return MapDistanceBetweenTypes(*this->GetFirstContainer()->Type, this->GetTilePos(), this->GetMapLayer()->GetIndex(), *dst.GetType(), dst.GetTilePos(), dst.GetMapLayer()->GetIndex());
}

/**
 **  Returns the map distance to unit.
 **
 **  @param pos   map tile position.
 **
 **  @return      The distance between in tiles.
 */
int CUnit::MapDistanceTo(const Vec2i &pos, int z) const
{
	//Wyrmgus start
	if (z != this->GetMapLayer()->GetIndex()) {
		return 16384;
	}
	//Wyrmgus end
	
	int dx;
	int dy;

	if (pos.x <= this->GetTilePos().x) {
		dx = this->GetTilePos().x - pos.x;
	//Wyrmgus start
	} else if (this->Container) { //if unit is within another, use the tile size of the transporter to calculate the distance
		dx = std::max<int>(0, pos.x - this->GetTilePos().x - this->Container->GetType()->TileSize.x + 1);
	//Wyrmgus end
	} else {
		dx = std::max<int>(0, pos.x - this->GetTilePos().x - this->GetType()->TileSize.x + 1);
	}
	if (pos.y <= this->GetTilePos().y) {
		dy = this->GetTilePos().y - pos.y;
	//Wyrmgus start
	} else if (this->Container) {
		dy = std::max<int>(0, pos.y - this->GetTilePos().y - this->Container->GetType()->TileSize.y + 1);
	//Wyrmgus end
	} else {
		dy = std::max<int>(0, pos.y - this->GetTilePos().y - this->GetType()->TileSize.y + 1);
	}
	return isqrt(dy * dy + dx * dx);
}

/**
**	@brief	Returns the map distance between two points with unit type
**
**	@param	src		Source unit type
**	@param	pos1	Map tile position of the source (upper-left)
**	@param	dst		Destination unit type to take into account
**	@param	pos2	Map tile position of the destination
**
**	@return	The distance between the types
*/
int MapDistanceBetweenTypes(const CUnitType &src, const Vec2i &pos1, int src_z, const CUnitType &dst, const Vec2i &pos2, int dst_z)
{
	return MapDistance(src.TileSize, pos1, src_z, dst.TileSize, pos2, dst_z);
}

int MapDistance(const Vec2i &src_size, const Vec2i &pos1, int src_z, const Vec2i &dst_size, const Vec2i &pos2, int dst_z)
{
	if (src_z != dst_z) {
		return 16384;
	}
	
	int dx;
	int dy;

	if (pos1.x + src_size.x <= pos2.x) {
		dx = std::max<int>(0, pos2.x - pos1.x - src_size.x + 1);
	} else {
		dx = std::max<int>(0, pos1.x - pos2.x - dst_size.x + 1);
	}
	if (pos1.y + src_size.y <= pos2.y) {
		dy = pos2.y - pos1.y - src_size.y + 1;
	} else {
		dy = std::max<int>(0, pos1.y - pos2.y - dst_size.y + 1);
	}
	return isqrt(dy * dy + dx * dx);
}

/**
**  Compute the distance from the view point to a given point.
**
**  @param pos  map tile position.
**
**  @todo FIXME: is it the correct place to put this function in?
*/
int ViewPointDistance(const Vec2i &pos)
{
	const CViewport &vp = *UI.SelectedViewport;
	const Vec2i vpSize(vp.MapWidth, vp.MapHeight);
	const Vec2i middle = vp.MapPos + vpSize / 2;

	return Distance(middle, pos);
}

/**
**  Compute the distance from the view point to a given unit.
**
**  @param dest  Distance to this unit.
**
**  @todo FIXME: is it the correct place to put this function in?
*/
int ViewPointDistanceToUnit(const CUnit &dest)
{
	const CViewport &vp = *UI.SelectedViewport;
	const Vec2i vpSize(vp.MapWidth, vp.MapHeight);
	const Vec2i midPos = vp.MapPos + vpSize / 2;

	//Wyrmgus start
//	return dest.MapDistanceTo(midPos);
	return dest.MapDistanceTo(midPos, UI.CurrentMapLayer->GetIndex());
	//Wyrmgus end
}

/**
**  Can the source unit attack the destination unit.
**
**  @param source  Unit type pointer of the attacker.
**  @param dest    Unit type pointer of the target.
**
**  @return        0 if attacker can't target the unit, else a positive number.
*/
int CanTarget(const CUnitType &source, const CUnitType &dest)
{
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
		if (source.BoolFlag[i].CanTargetFlag != CONDITION_TRUE) {
			if ((source.BoolFlag[i].CanTargetFlag == CONDITION_ONLY) ^
				(dest.BoolFlag[i].value)) {
				return 0;
			}
		}
	}
	if (dest.UnitType == UnitTypeLand) {
		if (dest.BoolFlag[SHOREBUILDING_INDEX].value) {
			return source.CanTarget & (CanTargetLand | CanTargetSea);
		}
		return source.CanTarget & CanTargetLand;
	}
	if (dest.UnitType == UnitTypeFly) {
		return source.CanTarget & CanTargetAir;
	}
	//Wyrmgus start
	if (dest.UnitType == UnitTypeFlyLow) {
		return (source.CanTarget & CanTargetLand) || (source.CanTarget & CanTargetAir) || (source.CanTarget & CanTargetSea);
	}
	//Wyrmgus end
	if (dest.UnitType == UnitTypeNaval) {
		return source.CanTarget & CanTargetSea;
	}
	return 0;
}

/**
**  Can the transporter transport the other unit.
**
**  @param transporter  Unit which is the transporter.
**  @param unit         Unit which wants to go in the transporter.
**
**  @return             1 if transporter can transport unit, 0 else.
*/
int CanTransport(const CUnit &transporter, const CUnit &unit)
{
	if (!transporter.GetType()->CanTransport()) {
		return 0;
	}
	if (transporter.CurrentAction() == UnitActionBuilt) { // Under construction
		return 0;
	}
	if (&transporter == &unit) { // Cannot transporter itself.
		return 0;
	}
	//Wyrmgus start
	/*
	if (transporter.BoardCount >= transporter.GetType()->MaxOnBoard) { // full
		return 0;
	}
	*/
	
	if (transporter.GetResourcesHeld() > 0 && transporter.GetCurrentResource()) { //cannot transport units if already has cargo
		return 0;
	}
	//Wyrmgus end

	if (transporter.BoardCount + unit.GetType()->GetBoardSize() > transporter.GetType()->MaxOnBoard) { // too big unit
		return 0;
	}

	// Can transport only allied unit.
	// FIXME : should be parametrable.
	//Wyrmgus start
//	if (!transporter.IsTeamed(unit)) {
	if (!transporter.IsTeamed(unit) && !transporter.IsAllied(unit) && transporter.GetPlayer()->Type != PlayerNeutral && unit.GetPlayer()->Type != PlayerNeutral) {
	//Wyrmgus end
		return 0;
	}
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
		if (transporter.GetType()->BoolFlag[i].CanTransport != CONDITION_TRUE) {
			if ((transporter.GetType()->BoolFlag[i].CanTransport == CONDITION_ONLY) ^ unit.GetType()->BoolFlag[i].value) {
				return 0;
			}
		}
	}
	return 1;
}

//Wyrmgus start
/**
**  Can the unit pick up the other unit.
**
**  @param picker		Unit which is the picker.
**  @param unit         Unit which wants to be picked.
**
**  @return             true if picker can pick up unit, false else.
*/
bool CanPickUp(const CUnit &picker, const CUnit &unit)
{
	if (!picker.GetType()->BoolFlag[ORGANIC_INDEX].value) { //only organic units can pick up power-ups and items
		return false;
	}
	if (!unit.GetType()->BoolFlag[ITEM_INDEX].value && !unit.GetType()->BoolFlag[POWERUP_INDEX].value) { //only item and powerup units can be picked up
		return false;
	}
	if (!unit.GetType()->BoolFlag[POWERUP_INDEX].value && !picker.HasInventory() && unit.GetType()->ItemClass->IsConsumable() == false) { //only consumable items can be picked up as if they were power-ups for units with no inventory
		return false;
	}
	if (picker.CurrentAction() == UnitActionBuilt) { // Under construction
		return false;
	}
	if (&picker == &unit) { // Cannot pick up itself.
		return false;
	}
	if (picker.HasInventory() && unit.GetType()->BoolFlag[ITEM_INDEX].value && picker.InsideCount >= ((int) UI.InventoryButtons.size())) { // full
		if (picker.GetPlayer() == CPlayer::GetThisPlayer()) {
			std::string picker_name = picker.Name + "'s (" + picker.GetTypeName() + ")";
			picker.GetPlayer()->Notify(NotifyRed, picker.GetTilePos(), picker.GetMapLayer()->GetIndex(), _("%s inventory is full."), picker_name.c_str());
		}
		return false;
	}

	return true;
}
//Wyrmgus end

/**
**  Check if the player is an enemy
**
**  @param player  Player to check
*/
bool CUnit::IsEnemy(const CPlayer &player) const
{
	//Wyrmgus start
	if (this->Player->GetIndex() != player.GetIndex() && player.Type != PlayerNeutral && !this->Player->HasBuildingAccess(player) && this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->IsAgressive()) {
		return true;
	}
	//Wyrmgus end
	
	return this->Player->IsEnemy(player);
}

/**
**  Check if the unit is an enemy
**
**  @param unit  Unit to check
*/
bool CUnit::IsEnemy(const CUnit &unit) const
{
	//Wyrmgus start
	if (
		this->Player->Type == PlayerNeutral
		&& this->Type->BoolFlag[FAUNA_INDEX].value
		&& this->Type->BoolFlag[ORGANIC_INDEX].value
		&& unit.GetType()->BoolFlag[ORGANIC_INDEX].value
		&& this->Type != unit.GetType()
	) {
		if (
			this->Type->BoolFlag[PREDATOR_INDEX].value
			&& !unit.GetType()->BoolFlag[PREDATOR_INDEX].value
			&& this->CanEat(unit)
		) {
			return true;
		} else if (
			this->Type->BoolFlag[PEOPLEAVERSION_INDEX].value
			&& !unit.GetType()->BoolFlag[FAUNA_INDEX].value
			&& unit.GetPlayer()->Type != PlayerNeutral
			&& this->MapDistanceTo(unit) <= 1
		) {
			return true;
		}
	}
		
	if (
		unit.GetPlayer()->Type == PlayerNeutral
		&& unit.GetType()->BoolFlag[PREDATOR_INDEX].value
		&& this->Player->Type != PlayerNeutral
	) {
		return true;
	}
	
	if (
		this->Player != unit.GetPlayer()
		&& this->Player->Type != PlayerNeutral
		&& unit.CurrentAction() == UnitActionAttack
		&& unit.CurrentOrder()->HasGoal()
		&& unit.CurrentOrder()->GetGoal()->GetPlayer() == this->Player
		&& !unit.CurrentOrder()->GetGoal()->GetType()->BoolFlag[HIDDENOWNERSHIP_INDEX].value
	) {
		return true;
	}
	
	if (
		this->Player != unit.GetPlayer() && this->Player->Type != PlayerNeutral && unit.GetPlayer()->Type != PlayerNeutral && !this->Player->HasBuildingAccess(*unit.GetPlayer()) && !this->Player->HasNeutralFactionType()
		&& ((this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->IsAgressive()) || (unit.GetType()->BoolFlag[HIDDENOWNERSHIP_INDEX].value && unit.IsAgressive()))
	) {
		return true;
	}

	return IsEnemy(*unit.GetPlayer());
	//Wyrmgus end
}

/**
**  Check if the player is an ally
**
**  @param player  Player to check
*/
bool CUnit::IsAllied(const CPlayer &player) const
{
	return this->Player->IsAllied(player);
}

/**
**  Check if the unit is an ally
**
**  @param x  Unit to check
*/
bool CUnit::IsAllied(const CUnit &unit) const
{
	return IsAllied(*unit.GetPlayer());
}

/**
**  Check if unit shares vision with the player
**
**  @param x  Player to check
*/
bool CUnit::IsSharedVision(const CPlayer &player) const
{
	return this->Player->IsSharedVision(player);
}

/**
**  Check if the unit shares vision with the unit
**
**  @param x  Unit to check
*/
bool CUnit::IsSharedVision(const CUnit &unit) const
{
	return IsSharedVision(*unit.GetPlayer());
}

/**
**  Check if both players share vision
**
**  @param x  Player to check
*/
bool CUnit::IsBothSharedVision(const CPlayer &player) const
{
	return this->Player->IsBothSharedVision(player);
}

/**
**  Check if both units share vision
**
**  @param x  Unit to check
*/
bool CUnit::IsBothSharedVision(const CUnit &unit) const
{
	return IsBothSharedVision(*unit.GetPlayer());
}

/**
**  Check if the player is on the same team
**
**  @param x  Player to check
*/
bool CUnit::IsTeamed(const CPlayer &player) const
{
	return (this->Player->Team == player.Team);
}

/**
**  Check if the unit is on the same team
**
**  @param x  Unit to check
*/
bool CUnit::IsTeamed(const CUnit &unit) const
{
	return this->IsTeamed(*unit.GetPlayer());
}

/**
**  Check if the unit is unusable (for attacking...)
**  @todo look if correct used (UnitActionBuilt is no problem if attacked)?
*/
bool CUnit::IsUnusable(bool ignore_built_state) const
{
	return (!IsAliveOnMap() || (!ignore_built_state && CurrentAction() == UnitActionBuilt));
}

/**
**  Check if the unit attacking its goal will result in a ranged attack
*/
//Wyrmgus start
//bool CUnit::IsAttackRanged(CUnit *goal, const Vec2i &goalPos)
bool CUnit::IsAttackRanged(CUnit *goal, const Vec2i &goalPos, int z)
//Wyrmgus end
{
	if (this->Variable[ATTACKRANGE_INDEX].Value <= 1) { //always return false if the units attack range is 1 or lower
		return false;
	}
	
	if (this->Container) { //if the unit is inside a container, the attack will always be ranged
		return true;
	}
	
	if (
		goal
		&& goal->IsAliveOnMap()
		&& (
			this->MapDistanceTo(*goal) > 1
			|| (this->Type->UnitType != UnitTypeFly && goal->GetType()->UnitType == UnitTypeFly)
			|| (this->Type->UnitType == UnitTypeFly && goal->GetType()->UnitType != UnitTypeFly)
		)
	) {
		return true;
	}
	
	if (!goal && CMap::Map.Info.IsPointOnMap(goalPos, z) && this->MapDistanceTo(goalPos, z) > 1) {
		return true;
	}
	
	return false;
}

void CUnit::IncreaseVariable(const int index)
{
	this->Variable[index].Value += this->Variable[index].Increase;
	//Wyrmgus start
//	this->Variable[index].Value = std::clamp(this->Variable[index].Value, 0, this->Variable[index].Max);
	this->Variable[index].Value = std::clamp(this->Variable[index].Value, 0, this->GetModifiedVariable(index, VariableMax));
	//Wyrmgus end
	
	//Wyrmgus start
	if (index == HP_INDEX && this->GetVariableIncrease(index) < 0 && this->HasInventory()) {
		this->HealingItemAutoUse();
	} else if (index == GIVERESOURCE_INDEX && !this->Type->BoolFlag[INEXHAUSTIBLE_INDEX].value) {
		this->ChangeResourcesHeld(this->GetVariableIncrease(index));
		this->ResourcesHeld = std::clamp(this->ResourcesHeld, 0, this->GetModifiedVariable(index, VariableMax));
	}
	//Wyrmgus end

	//if variable is HP and increase is negative, unit dies if HP reached 0
	if (index == HP_INDEX && this->GetVariableValue(HP_INDEX) <= 0) {
		LetUnitDie(*this);
	}
	
	//Wyrmgus start
	//if variable is resources held and increase is negative, unit dies if resources held reached 0 (only for units which cannot be harvested, as the ones that can be harvested need more complex code for dying)
	if (index == GIVERESOURCE_INDEX && this->GetVariableIncrease(GIVERESOURCE_INDEX) < 0 && this->GetVariableValue(GIVERESOURCE_INDEX) <= 0 && this->GivesResource && !this->Type->BoolFlag[CANHARVEST_INDEX].value) {
		LetUnitDie(*this);
	}
	//Wyrmgus end
}

/**
**	@brief	Handle things about the unit that decay over time each cycle
*/
void CUnit::HandleBuffsEachCycle()
{
	// Look if the time to live is over.
	if (this->TTL && this->IsAlive() && this->TTL < GameCycle) {
		DebugPrint("Unit must die %lu %lu!\n" _C_ this->TTL _C_ GameCycle);

		// Hit unit does some funky stuff...
		--this->Variable[HP_INDEX].Value;
		if (this->GetVariableValue(HP_INDEX) <= 0) {
			LetUnitDie(*this);
			return;
		}
	}

	if (--this->Threshold < 0) {
		this->Threshold = 0;
	}

	// decrease spell countdown timers
	if (!this->SpellCoolDownTimers.empty()) {
		for (const CSpell *spell : this->Type->Spells) {
			if (this->SpellCoolDownTimers.find(spell) != this->SpellCoolDownTimers.end()) {
				--this->SpellCoolDownTimers[spell];
				
				if (this->SpellCoolDownTimers[spell] <= 0) {
					this->SpellCoolDownTimers.erase(spell);
				}
			}
		}
	}

	for (std::map<const CUnitType *, int>::const_iterator iterator = this->Type->Stats[this->Player->GetIndex()].UnitStock.begin(); iterator != this->Type->Stats[this->Player->GetIndex()].UnitStock.end(); ++iterator) {
		const CUnitType *unit_type = iterator->first;
		int unit_stock = iterator->second;
		
		if (unit_stock <= 0) {
			continue;
		}
		
		if (this->GetUnitStockReplenishmentTimer(unit_type) > 0) {
			this->ChangeUnitStockReplenishmentTimer(unit_type, -1);
			if (this->GetUnitStockReplenishmentTimer(unit_type) == 0 && this->GetUnitStock(unit_type) < unit_stock) { //if timer reached 0, replenish 1 of the stock
				this->ChangeUnitStock(unit_type, 1);
			}
		}
			
		//if the unit still has less stock than its max, re-init the unit stock timer
		if (this->GetUnitStockReplenishmentTimer(unit_type) == 0 && this->GetUnitStock(unit_type) < unit_stock && CheckDependencies(unit_type, this->Player)) {
			this->SetUnitStockReplenishmentTimer(unit_type, unit_type->Stats[this->Player->GetIndex()].Costs[TimeCost] * 50);
		}
	}
	
	const bool last_status_is_hidden = this->GetVariableValue(INVISIBLE_INDEX) > 0;
	
	//  decrease spells effects time.
	for (unsigned int i = 0; i < sizeof(CUnitTypeVar::SpellEffects) / sizeof(int); ++i) {
		if (this->Variable[CUnitTypeVar::SpellEffects[i]].Value == 0) {
			continue;
		}
		
		this->Variable[CUnitTypeVar::SpellEffects[i]].Increase = -1;
		this->IncreaseVariable(CUnitTypeVar::SpellEffects[i]);
	}
	
	if (last_status_is_hidden && this->GetVariableValue(INVISIBLE_INDEX) == 0) {
		UnHideUnit(*this);
	}
}

/**
**	@brief	Handle things about the unit that decay over time each second
*/
void CUnit::HandleBuffsEachSecond()
{
	//Wyrmgus start
	if (this->Type->BoolFlag[DECORATION_INDEX].value) {
		return;
	}
	//Wyrmgus end
	
	// User defined variables
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (i == BLOODLUST_INDEX || i == HASTE_INDEX || i == SLOW_INDEX
			|| i == INVISIBLE_INDEX || i == UNHOLYARMOR_INDEX || i == POISON_INDEX || i == STUN_INDEX || i == BLEEDING_INDEX || i == LEADERSHIP_INDEX || i == BLESSING_INDEX || i == INSPIRE_INDEX || i == PRECISION_INDEX || i == REGENERATION_INDEX || i == BARKSKIN_INDEX || i == INFUSION_INDEX || i == TERROR_INDEX || i == WITHER_INDEX || i == DEHYDRATION_INDEX || i == HYDRATING_INDEX) {
			continue;
		}
		if (i == HP_INDEX && this->HandleBurnAndPoison()) {
			continue;
		}
		//Wyrmgus start
		if (i == HP_INDEX && this->GetVariableValue(REGENERATION_INDEX) > 0) {
			this->Variable[i].Value += 1;
			this->Variable[i].Value = std::clamp(this->Variable[i].Value, 0, this->GetModifiedVariable(i, VariableMax));
		}
		//Wyrmgus end
		if (this->IsVariableEnabled(i) && this->GetVariableIncrease(i) != 0) {
			this->IncreaseVariable(i);
		}
	}
	
	//Wyrmgus start
	if (this->IsAlive() && this->CurrentAction() != UnitActionBuilt) {
		//apply auras
		if (this->GetVariableValue(LEADERSHIPAURA_INDEX) > 0) {
			this->ApplyAura(LEADERSHIPAURA_INDEX);
		}
		if (this->GetVariableValue(REGENERATIONAURA_INDEX) > 0) {
			this->ApplyAura(REGENERATIONAURA_INDEX);
		}
		if (this->GetVariableValue(HYDRATINGAURA_INDEX) > 0) {
			this->ApplyAura(HYDRATINGAURA_INDEX);
		}
		
		//apply "-stalk" abilities
		if ((this->GetVariableValue(DESERTSTALK_INDEX) > 0 || this->GetVariableValue(FORESTSTALK_INDEX) > 0 || this->GetVariableValue(SWAMPSTALK_INDEX) > 0) && CMap::Map.Info.IsPointOnMap(this->GetTilePos().x, this->GetTilePos().y, this->MapLayer)) {
			if (
				(
					(this->GetVariableValue(DESERTSTALK_INDEX) > 0 && this->GetMapLayer()->Field(this->GetTilePos().x, this->GetTilePos().y)->GetTerrain(false)->IsDesert())
					|| (this->GetVariableValue(FORESTSTALK_INDEX) > 0 && this->GetMapLayer()->TileBlockHasTree(this->GetTilePos() - 1, this->Type->TileSize + 2))
					|| (this->GetVariableValue(SWAMPSTALK_INDEX) > 0 && this->GetMapLayer()->Field(this->GetTilePos().x, this->GetTilePos().y)->GetTerrain(false)->IsSwamp())
				)
				&& (this->GetVariableValue(INVISIBLE_INDEX) > 0 || !this->IsInCombat())
			) {				
				std::vector<CUnit *> table;
				SelectAroundUnit(*this, 1, table, IsEnemyWith(*this->Player));
				if (table.size() == 0) { //only apply the -stalk invisibility if the unit is not adjacent to an enemy unit
					this->Variable[INVISIBLE_INDEX].Enable = 1;
					this->Variable[INVISIBLE_INDEX].Max = std::max(CYCLES_PER_SECOND + 1, this->Variable[INVISIBLE_INDEX].Max);
					this->Variable[INVISIBLE_INDEX].Value = std::max(CYCLES_PER_SECOND + 1, this->Variable[INVISIBLE_INDEX].Value);
				}
			}
		}
		
		if ( //apply dehydration to an organic unit on a desert tile; only apply dehydration during day-time
			this->GetMapLayer()->Field(this->GetTilePos().x, this->GetTilePos().y)->GetTerrain(false)->IsDesert()
			&& this->Type->BoolFlag[ORGANIC_INDEX].value
			&& CMap::Map.Info.IsPointOnMap(this->GetTilePos().x, this->GetTilePos().y, this->MapLayer)
			&& this->GetMapLayer()->Field(this->GetTilePos().x, this->GetTilePos().y)->Owner != this->Player->GetIndex()
			&& this->GetMapLayer()->GetTimeOfDay() != nullptr
			&& this->GetMapLayer()->GetTimeOfDay()->IsDay()
			&& this->GetVariableValue(HYDRATING_INDEX) <= 0
			&& this->GetVariableValue(DEHYDRATIONIMMUNITY_INDEX) <= 0
		) {
			this->Variable[DEHYDRATION_INDEX].Enable = 1;
			this->Variable[DEHYDRATION_INDEX].Max = std::max(CYCLES_PER_SECOND + 1, this->Variable[DEHYDRATION_INDEX].Max);
			this->Variable[DEHYDRATION_INDEX].Value = std::max(CYCLES_PER_SECOND + 1, this->Variable[DEHYDRATION_INDEX].Value);
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	if (this->GetVariableValue(TERROR_INDEX) > 0) { // if unit is terrified, flee at the sight of enemies
		std::vector<CUnit *> table;
		SelectAroundUnit(*this, this->CurrentSightRange, table, IsAggresiveUnit(), true);
		for (size_t i = 0; i != table.size(); ++i) {
			if (this->IsEnemy(*table[i])) {
				HitUnit_RunAway(*this, *table[i]);
				break;
			}
		}
	}
	//Wyrmgus end
}

/**
**	@brief	Modify the unit's health according to burn and poison.
*/
bool CUnit::HandleBurnAndPoison()
{
	if (
		this->Removed || this->Destroyed || this->GetVariableMax(HP_INDEX) == 0
		|| this->CurrentAction() == UnitActionBuilt
		|| this->CurrentAction() == UnitActionDie
	) {
		return false;
	}
	
	// Burn and poison
	//Wyrmgus start
//	const int hp_percent = (100 * this->GetVariableValue(HP_INDEX)) / this->GetVariableMax(HP_INDEX);
	const int hp_percent = (100 * this->GetVariableValue(HP_INDEX)) / this->GetModifiedVariable(HP_INDEX, VariableMax);
	//Wyrmgus end
	if (hp_percent <= this->Type->BurnPercent && this->Type->BurnDamageRate) {
		//Wyrmgus start
//		HitUnit(NoUnitP, *this, this->Type->BurnDamageRate);
		HitUnit(NoUnitP, *this, this->Type->BurnDamageRate, nullptr, false); //a bit too repetitive to show damage every single time the burn effect is applied
		//Wyrmgus end
		return true;
	}
	if (this->GetVariableValue(POISON_INDEX) > 0 && this->Type->PoisonDrain) {
		//Wyrmgus start
//		HitUnit(NoUnitP, *this, this->Type->PoisonDrain);
		HitUnit(NoUnitP, *this, this->Type->PoisonDrain, nullptr, false); //a bit too repetitive to show damage every single time the poison effect is applied
		//Wyrmgus end
		return true;
	}
	//Wyrmgus start
	if (this->GetVariableValue(BLEEDING_INDEX) > 0 || this->GetVariableValue(DEHYDRATION_INDEX) > 0) {
		HitUnit(NoUnitP, *this, 1, nullptr, false);
		//don't return true since we don't want to stop regeneration (positive or negative) from happening
	}
	//Wyrmgus end
	return false;
}

/**
**  Handle the action of a unit.
**
**  @param unit  Pointer to handled unit.
*/
void CUnit::HandleUnitAction()
{
	// If current action is breakable proceed with next one.
	if (!this->Anim.Unbreakable) {
		if (this->CriticalOrder != nullptr) {
			this->CriticalOrder->Execute(*this);
			delete this->CriticalOrder;
			this->CriticalOrder = nullptr;
		}

		if (this->Orders[0]->Finished && this->Orders[0]->Action != UnitActionStill && this->Orders.size() == 1) {

			delete this->Orders[0];
			this->Orders[0] = COrder::NewActionStill();
			if (IsOnlySelected(*this)) { // update display for new action
				SelectedUnitChanged();
			}
		}

		// o Look if we have a new order and old finished.
		// o Or the order queue should be flushed.
		if (
			(this->Orders[0]->Action == UnitActionStandGround || this->Orders[0]->Finished)
			&& this->Orders.size() > 1
		) {
			if (this->Removed && this->Orders[0]->Action != UnitActionBoard) { // FIXME: johns I see this as an error
				DebugPrint("Flushing removed unit\n");
				// This happens, if building with ALT+SHIFT.
				return;
			}

			delete this->Orders[0];
			this->Orders.erase(this->Orders.begin());

			this->Wait = 0;
			if (IsOnlySelected(*this)) { // update display for new action
				SelectedUnitChanged();
			}
		}
	}
	this->Orders[0]->Execute(*this);
}

IntColor CUnit::GetSelectionColor() const
{
	if (Editor.Running && UnitUnderCursor == this && Editor.State == EditorSelecting) {
		return ColorWhite;
	} else if (this->IsSelected() || this->TeamSelected || (this->Blink & 1)) {
		if (this->GetPlayer()->GetIndex() == PlayerNumNeutral) {
			return ColorYellow;
		} else if (
			(this->IsSelected() || (this->Blink & 1))
			&& (this->GetPlayer() == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*this))
		) {
			return ColorGreen;
		} else if (CPlayer::GetThisPlayer()->IsEnemy(*this)) {
			return ColorRed;
		} else {
			IntColor color = this->GetPlayer()->Color;

			for (int i = 0; i < PlayerMax; ++i) {
				if (this->TeamSelected & (1 << i)) {
					color = CPlayer::Players[i]->Color;
				}
			}
			
			return color;
		}
	} else if (
		CursorBuilding && this->GetType()->BoolFlag[BUILDING_INDEX].value
		&& this->CurrentAction() != UnitActionDie
		&& (this->GetPlayer() == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(*this))
	) {
		//if building mark all own buildings
		return ColorGray;
	}
	
	return 0;
}

void CUnit::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_name"), +[](const CUnit *unit){ return String(unit->GetName().c_str()); });
	ClassDB::bind_method(D_METHOD("get_player"), &CUnit::GetPlayer);
	ClassDB::bind_method(D_METHOD("get_icon"), +[](const CUnit *unit){ return const_cast<CIcon *>(unit->GetIcon()); });
	ClassDB::bind_method(D_METHOD("get_type"), +[](const CUnit *unit){ return const_cast<CUnitType *>(unit->GetType()); });
	
	ClassDB::bind_method(D_METHOD("get_tile_pos"), +[](const CUnit *unit){ return Vector2(unit->GetTilePos()); });
	ADD_SIGNAL(MethodInfo("tile_pos_changed", PropertyInfo(Variant::VECTOR2, "tile_pos")));
	
	ClassDB::bind_method(D_METHOD("get_tile_pixel_size"), +[](const CUnit *unit){ return Vector2(unit->GetTilePixelSize()); });
	ClassDB::bind_method(D_METHOD("get_half_tile_pixel_size"), +[](const CUnit *unit){ return Vector2(unit->GetHalfTilePixelSize()); });
	
	ClassDB::bind_method(D_METHOD("get_map_layer"), +[](const CUnit *unit){ return unit->GetMapLayer(); });
	ADD_SIGNAL(MethodInfo("map_layer_changed", PropertyInfo(Variant::OBJECT, "map_layer")));
	
	ClassDB::bind_method(D_METHOD("get_image"), +[](const CUnit *unit){ return const_cast<PaletteImage *>(unit->GetImage()); });
	ADD_SIGNAL(MethodInfo("image_changed", PropertyInfo(Variant::OBJECT, "image")));
	
	ClassDB::bind_method(D_METHOD("get_frame"), &CUnit::GetSpriteFrame);
	ADD_SIGNAL(MethodInfo("frame_changed", PropertyInfo(Variant::INT, "frame")));
	
	ClassDB::bind_method(D_METHOD("is_flipped"), +[](const CUnit *unit){ return (unit->GetFrame() < 0); });
	ADD_SIGNAL(MethodInfo("flipped_changed", PropertyInfo(Variant::BOOL, "flipped")));
	
	ClassDB::bind_method(D_METHOD("get_pixel_offset"), +[](const CUnit *unit){ return Vector2(unit->GetPixelOffset()); });
	ADD_SIGNAL(MethodInfo("pixel_offset_changed", PropertyInfo(Variant::VECTOR2, "pixel_offset")));
	
	ClassDB::bind_method(D_METHOD("is_selected"), &CUnit::IsSelected);
	ADD_SIGNAL(MethodInfo("selected_changed", PropertyInfo(Variant::BOOL, "selected"), PropertyInfo(Variant::COLOR, "selection_color")));
	
	ClassDB::bind_method(D_METHOD("get_selection_color"), +[](const CUnit *unit){ return IntColorToColor(unit->GetSelectionColor()); });
	
	//this signal is triggered when a unit is removed from the map, so that it is no longer displayed
	ADD_SIGNAL(MethodInfo("removed"));
}

/*----------------------------------------------------------------------------
--  Initialize/Cleanup
----------------------------------------------------------------------------*/

/**
**  Initialize unit module.
*/
void InitUnits()
{
	if (!SaveGameLoading) {
		UnitManager.Init();
	}
}

/**
**  Clean up unit module.
*/
void CleanUnits()
{
	//  Free memory for all units in unit table.
	std::vector<CUnit *> units(UnitManager.begin(), UnitManager.end());

	for (std::vector<CUnit *>::iterator it = units.begin(); it != units.end(); ++it) {
		//Wyrmgus start
		if (*it == nullptr) {
			fprintf(stderr, "Error in CleanUnits: unit is null.\n");
			continue;
		}
		//Wyrmgus end
		CUnit &unit = **it;

		//Wyrmgus start
		/*
		if (&unit == nullptr) {
			continue;
		}
		*/
		//Wyrmgus end
		//Wyrmgus start
		if (unit.GetType() == nullptr) {
			fprintf(stderr, "Unit \"%d\"'s type is null.\n", UnitNumber(unit));
		}
		//Wyrmgus end
		if (!unit.Destroyed) {
			if (!unit.Removed) {
				unit.Remove(nullptr);
			}
			UnitClearOrders(unit);
		}
		unit.Release(true);
	}

	UnitManager.Init();

	FancyBuildings = false;
	HelpMeLastCycle = 0;
}
