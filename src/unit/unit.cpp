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
/**@name unit.cpp - The units. */
//
//      (c) Copyright 1998-2015 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "stratagus.h"

#include "unit.h"

#include "action/action_attack.h"
//Wyrmgus start
#include "action/action_upgradeto.h"
//Wyrmgus end

#include "actions.h"
#include "ai.h"
//Wyrmgus start
#include "../ai/ai_local.h" //for using AiHelpers
//Wyrmgus end
#include "animation.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "construct.h"
//Wyrmgus start
#include "depend.h"	//for using dependency checks
//Wyrmgus end
#include "game.h"
#include "editor.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "interface.h"
//Wyrmgus start
#include "item.h"
//Wyrmgus end
#include "luacallback.h"
#include "map.h"
#include "missile.h"
#include "network.h"
#include "pathfinder.h"
#include "player.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
#include "script.h"
#include "sound.h"
#include "sound_server.h"
#include "spells.h"
//Wyrmgus start
#include "tileset.h"
//Wyrmgus end
#include "translate.h"
#include "ui.h"
#include "unit_find.h"
#include "unit_manager.h"
#include "unitsound.h"
#include "unittype.h"
#include "upgrade.h"
//Wyrmgus start
#include "util.h"
//Wyrmgus end
#include "video.h"

#include <math.h>


/*----------------------------------------------------------------------------
-- Documentation
----------------------------------------------------------------------------*/

/**
**  @class CUnit unit.h
**
**  \#include "unit.h"
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
**  Pointer to the unit containing it, or NULL if the unit is
**  free. This points to the transporter for units on board, or to
**  the building for peasants inside(when they are mining).
**
**  CUnit::UnitInside
**
**  Pointer to the last unit added inside. Order doesn't really
**  matter. All units inside are kept in a circular linked list.
**  This is NULL if there are no units inside. Multiple levels
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
**  CUnit::IX CUnit::IY
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
**  CUnit::Constructed
**  Set when a building is under construction, and still using the
**  generic building animation.
**
**  CUnit::SeenConstructed
**  Last seen state of construction.  Used to draw correct building
**  frame. See CUnit::Constructed for more information.
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
**  Pointer to the original owner of a unit. It will be NULL if
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
//Wyrmgus start
//bool EnableBuildingCapture;               /// Config: capture buildings enabled
bool EnableBuildingCapture = false;               /// Config: capture buildings enabled
//Wyrmgus end
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
	Refs = 0;
	ReleaseCycle = 0;
	PlayerSlot = static_cast<size_t>(-1);
	InsideCount = 0;
	BoardCount = 0;
	UnitInside = NULL;
	Container = NULL;
	NextContained = NULL;
	PrevContained = NULL;
	NextWorker = NULL;

	Resource.Workers = NULL;
	Resource.Assigned = 0;
	Resource.Active = 0;
	
	//Wyrmgus start
	for (int i = 0; i < MaxItemSlots; ++i) {
		EquippedItems[i].clear();
	}
	SoldUnits.clear();
	//Wyrmgus end

	tilePos.x = 0;
	tilePos.y = 0;
	//Wyrmgus start
	RallyPointPos.x = -1;
	RallyPointPos.y = -1;
	MapLayer = 0;
	RallyPointMapLayer = 0;
	//Wyrmgus end
	Offset = 0;
	Type = NULL;
	Player = NULL;
	Stats = NULL;
	//Wyrmgus start
	Character = NULL;
	Trait = NULL;
	Prefix = NULL;
	Suffix = NULL;
	Spell = NULL;
	Work = NULL;
	Elixir = NULL;
	Unique = NULL;
	Bound = false;
	Identified = true;
	ConnectingDestination = NULL;
	//Wyrmgus end
	CurrentSightRange = 0;

	pathFinderData = new PathFinderData;
	pathFinderData->input.SetUnit(*this);

	Colors = NULL;
	//Wyrmgus start
	Name = "";
	Variation = 0;
	memset(LayerVariation, -1, sizeof(LayerVariation));
	//Wyrmgus end
	IX = 0;
	IY = 0;
	Frame = 0;
	Direction = 0;
	DamagedType = ANIMATIONS_DEATHTYPES;
	Attacked = 0;
	Burning = 0;
	Destroyed = 0;
	Removed = 0;
	Selected = 0;
	TeamSelected = 0;
	Constructed = 0;
	Active = 0;
	Boarded = 0;
	RescuedFrom = NULL;
	memset(VisCount, 0, sizeof(VisCount));
	memset(&Seen, 0, sizeof(Seen));
	Variable = NULL;
	TTL = 0;
	Threshold = 0;
	GroupId = 0;
	LastGroup = 0;
	ResourcesHeld = 0;
	Wait = 0;
	Blink = 0;
	Moving = 0;
	ReCast = 0;
	CacheLock = 0;
	Summoned = 0;
	Waiting = 0;
	MineLow = 0;
	//Wyrmgus start
	Starting = 0;
	//Wyrmgus end
	memset(&Anim, 0, sizeof(Anim));
	memset(&WaitBackup, 0, sizeof(WaitBackup));
	CurrentResource = 0;
	Orders.clear();
	delete SavedOrder;
	SavedOrder = NULL;
	delete NewOrder;
	NewOrder = NULL;
	delete CriticalOrder;
	CriticalOrder = NULL;
	AutoCastSpell = NULL;
	SpellCoolDownTimers = NULL;
	AutoRepair = 0;
	Goal = NULL;
	memset(IndividualUpgrades, 0, sizeof(IndividualUpgrades));
}

/**
**  Release an unit.
**
**  The unit is only released, if all references are dropped.
*/
void CUnit::Release(bool final)
{
	if (Type == NULL) {
		DebugPrint("unit already free\n");
		return;
	}
	Assert(Orders.size() == 1);
	// Must be removed before here
	Assert(Removed);

	// First release, remove from lists/tables.
	if (!Destroyed) {
		DebugPrint("%d: First release %d\n" _C_ Player->Index _C_ UnitNumber(*this));

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

	Type = NULL;
	//Wyrmgus start
	Character = NULL;
	Trait = NULL;
	Prefix = NULL;
	Suffix = NULL;
	Spell = NULL;
	Work = NULL;
	Elixir = NULL;
	Unique = NULL;
	Bound = false;
	Identified = true;
	ConnectingDestination = NULL;
	
	for (int i = 0; i < MaxItemSlots; ++i) {
		EquippedItems[i].clear();
	}
	SoldUnits.clear();
	//Wyrmgus end

	delete pathFinderData;
	delete[] AutoCastSpell;
	delete[] SpellCoolDownTimers;
	delete[] Variable;
	for (std::vector<COrder *>::iterator order = Orders.begin(); order != Orders.end(); ++order) {
		delete *order;
	}
	Orders.clear();

	// Remove the unit from the global units table.
	UnitManager.ReleaseUnit(this);
}

//Wyrmgus start
void CUnit::SetResourcesHeld(int quantity)
{
	this->ResourcesHeld = quantity;
	
	VariationInfo *varinfo = this->Type->VarInfo[this->Variation];
	if (varinfo && ((varinfo->ResourceMin && this->ResourcesHeld < varinfo->ResourceMin) || (varinfo->ResourceMax && this->ResourcesHeld > varinfo->ResourceMax))) {
		this->ChooseVariation();
	}
}

void CUnit::ChangeResourcesHeld(int quantity)
{
	this->SetResourcesHeld(this->ResourcesHeld + quantity);
}

void CUnit::IncreaseLevel(int level_quantity)
{
	while (level_quantity > 0) {
		this->Variable[LEVEL_INDEX].Value += 1;
		if (this->Type->Stats[this->Player->Index].Variables[LEVEL_INDEX].Value < this->Variable[LEVEL_INDEX].Value) {
			if (GetAvailableLevelUpUpgrades(true) == 0 || (this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->Index].Variables[LEVEL_INDEX].Value) > 1) {
				this->Variable[POINTS_INDEX].Max += 5 * (this->Variable[LEVEL_INDEX].Value + 1);
				this->Variable[POINTS_INDEX].Value += 5 * (this->Variable[LEVEL_INDEX].Value + 1);
			}
			
			this->Variable[LEVELUP_INDEX].Value += 1;
			this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
			// if there are no level-up upgrades available for the unit, increase its HP instead
			if (this->GetAvailableLevelUpUpgrades() < this->Variable[LEVELUP_INDEX].Value) {
				this->Variable[HP_INDEX].Max += 15;
				this->Variable[LEVELUP_INDEX].Value -= 1;
				this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
			}
		}
		this->Variable[HP_INDEX].Value = this->GetModifiedVariable(HP_INDEX, VariableMax);
		level_quantity -= 1;
	}
	
	UpdateXPRequired();
	
	if (Player->AiEnabled) {
		bool upgrade_found = true;
		while (this->Variable[LEVELUP_INDEX].Value > 0 && upgrade_found) {
			upgrade_found = false;
			if (((int) AiHelpers.ExperienceUpgrades.size()) > Type->Slot) {
				std::vector<CUnitType *> potential_upgrades;
				for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[Type->Slot].size(); ++i) {
					if (CheckDependByType(*Player, *AiHelpers.ExperienceUpgrades[Type->Slot][i], true)) {
						if (Character == NULL || !Character->ForbiddenUpgrades[AiHelpers.ExperienceUpgrades[Type->Slot][i]->Slot]) {
							potential_upgrades.push_back(AiHelpers.ExperienceUpgrades[Type->Slot][i]);
						}
					}
				}
				if (potential_upgrades.size() > 0) {
					this->Variable[LEVELUP_INDEX].Value -= 1;
					this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
					TransformUnitIntoType(*this, *potential_upgrades[SyncRand(potential_upgrades.size())]);
					upgrade_found = true;
				}
			}
			
			if (this->Variable[LEVELUP_INDEX].Value) {
				if (((int) AiHelpers.LearnableAbilities.size()) > Type->Slot) {
					std::vector<CUpgrade *> potential_abilities;
					for (size_t i = 0; i != AiHelpers.LearnableAbilities[Type->Slot].size(); ++i) {
						if (CanLearnAbility(AiHelpers.LearnableAbilities[Type->Slot][i])) {
							potential_abilities.push_back(AiHelpers.LearnableAbilities[Type->Slot][i]);
						}
					}
					if (potential_abilities.size() > 0) {
						AbilityAcquire(*this, potential_abilities[SyncRand(potential_abilities.size())]);
						upgrade_found = true;
					}
				}
			}
		}
	}
	
	this->Variable[LEVELUP_INDEX].Enable = 1;
	
	Player->UpdateLevelUpUnits();
}

void CUnit::Retrain()
{
	//lose all abilities (the AbilityLost function also returns the level-ups to the unit)
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (this->IndividualUpgrades[AllUpgrades[i]->ID] && AllUpgrades[i]->Ability && std::find(this->Type->StartingAbilities.begin(), this->Type->StartingAbilities.end(), AllUpgrades[i]) == this->Type->StartingAbilities.end()) {
			AbilityLost(*this, AllUpgrades[i]);
		}
	}
	
	std::string unit_name = GetMessageName();
	
	//now, revert the unit's type to the level 1 one
	while (this->Type->Stats[this->Player->Index].Variables[LEVEL_INDEX].Value > 1) {
		bool found_previous_unit_type = false;
		for (size_t i = 0; i != UnitTypes.size(); ++i) {
			if (Character != NULL && Character->ForbiddenUpgrades[i]) {
				continue;
			}
			if (((int) AiHelpers.ExperienceUpgrades.size()) > i) {
				for (size_t j = 0; j != AiHelpers.ExperienceUpgrades[i].size(); ++j) {
					if (AiHelpers.ExperienceUpgrades[i][j] == this->Type) {
						this->Variable[LEVELUP_INDEX].Value += 1;
						this->Variable[LEVELUP_INDEX].Max = this->Variable[LEVELUP_INDEX].Value;
						TransformUnitIntoType(*this, *UnitTypes[i]);
						if (!IsNetworkGame() && Character != NULL) {	//save the unit-type experience upgrade for persistent characters
							if (Character->Type->Slot != i) {
								if (Character->Persistent && Player->AiEnabled == false) {
									Character->Type = UnitTypes[i];
									SaveHero(Character);
									CheckAchievements();
								}
								if (GrandStrategy) { //also update the corresponding grand strategy hero, if in grand strategy mode
									CGrandStrategyHero *hero = GrandStrategyGame.GetHero(Character->GetFullName());
									if (hero) {
										hero->SetType(i);
									}
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
	
	if (this->Player == ThisPlayer) {
		this->Player->Notify(NotifyGreen, this->tilePos, this->MapLayer, _("%s's level-up choices have been reset."), unit_name.c_str());
	}
}

void CUnit::HealingItemAutoUse()
{
	if (!HasInventory()) {
		return;
	}
	
	CUnit *uins = this->UnitInside;
	
	for (int i = 0; i < this->InsideCount; ++i, uins = uins->NextContained) {
		if (!uins->Type->BoolFlag[ITEM_INDEX].value || uins->Elixir) {
			continue;
		}
		
		if (!IsItemClassConsumable(uins->Type->ItemClass)) {
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

void CUnit::SetCharacter(std::string character_full_name, bool custom_hero)
{
	if (this->CurrentAction() == UnitActionDie) {
		return;
	}
	
	if (this->Character == NULL) {
		this->Player->UnitTypesNonHeroCount[this->Type->Slot]--;
		if (this->Starting) {
			this->Player->UnitTypesStartingNonHeroCount[this->Type->Slot]--;
		}
	} else {
		this->Player->Heroes.erase(std::remove(this->Player->Heroes.begin(), this->Player->Heroes.end(), this->Character->GetFullName()), this->Player->Heroes.end());
		
		if (this == this->Player->CustomHeroUnit) {
			this->Player->CustomHeroUnit = NULL;
		}
	}
	
	CCharacter *character = NULL;
	if (!custom_hero) {
		character = GetCharacter(character_full_name);
		if (GrandStrategy && (character == NULL || !character->Persistent || this->Player->AiEnabled)) {
			character = GrandStrategyGame.GetHero(character_full_name);
		}
	} else {
		character = GetCustomHero(character_full_name);
	}
	
	if (character) {
		this->Character = character;
	} else {
		fprintf(stderr, "Character \"%s\" doesn't exist.\n", character_full_name.c_str());
		return;
	}
		
	this->Name = this->Character->GetFullName();
	
	if (this->Character->Type != NULL) {
		if (this->Character->Type != this->Type) { //set type to that of the character
			TransformUnitIntoType(*this, *this->Character->Type);
		}
		
		memcpy(Variable, this->Character->Type->Stats[this->Player->Index].Variables, UnitTypeVar.GetNumberVariable() * sizeof(*Variable));
	} else {
		fprintf(stderr, "Character \"%s\" has no unit type.\n", character_full_name.c_str());
		return;
	}
	
	memset(IndividualUpgrades, 0, sizeof(IndividualUpgrades)); //reset the individual upgrades and then apply the character's
	this->Trait = NULL;

	for (size_t i = 0; i < this->Type->StartingAbilities.size(); ++i) {
		IndividualUpgradeAcquire(*this, this->Type->StartingAbilities[i]);
	}
	
	if (this->Character->Trait != NULL) { //set trait
		TraitAcquire(*this, this->Character->Trait);
	}
	
	if (this->Character->Deity != NULL && this->Character->Deity->CharacterUpgrade != NULL) {
		IndividualUpgradeAcquire(*this, this->Character->Deity->CharacterUpgrade);
	}
	
	this->Variable[LEVEL_INDEX].Max = 100000; // because the code above sets the max level to the unit type stats' Level variable (which is the same as its value)
	if (this->Variable[LEVEL_INDEX].Value < this->Character->Level) {
		this->IncreaseLevel(this->Character->Level - this->Variable[LEVEL_INDEX].Value);
	}
	
	this->Variable[XP_INDEX].Enable = 1;
	this->Variable[XP_INDEX].Value = (this->Variable[XPREQUIRED_INDEX].Value * this->Character->ExperiencePercent) / 100;
	this->Variable[XP_INDEX].Max = this->Variable[XP_INDEX].Value;
			
	//load learned abilities
	for (size_t i = 0; i < this->Character->Abilities.size(); ++i) {
		AbilityAcquire(*this, this->Character->Abilities[i]);
	}
	
	//load read works
	for (size_t i = 0; i < this->Character->ReadWorks.size(); ++i) {
		ReadWork(this->Character->ReadWorks[i], false);
	}
	
	//load consumed elixirs
	for (size_t i = 0; i < this->Character->ConsumedElixirs.size(); ++i) {
		ConsumeElixir(this->Character->ConsumedElixirs[i], false);
	}
	
	//load items
	for (size_t i = 0; i < this->Character->Items.size(); ++i) {
		CUnit *item = MakeUnitAndPlace(this->tilePos, *this->Character->Items[i]->Type, &Players[PlayerNumNeutral], this->MapLayer);
		if (this->Character->Items[i]->Prefix != NULL) {
			item->SetPrefix(this->Character->Items[i]->Prefix);
		}
		if (this->Character->Items[i]->Suffix != NULL) {
			item->SetSuffix(this->Character->Items[i]->Suffix);
		}
		if (this->Character->Items[i]->Spell != NULL) {
			item->SetSpell(this->Character->Items[i]->Spell);
		}
		if (this->Character->Items[i]->Work != NULL) {
			item->SetWork(this->Character->Items[i]->Work);
		}
		if (this->Character->Items[i]->Elixir != NULL) {
			item->SetElixir(this->Character->Items[i]->Elixir);
		}
		item->Unique = this->Character->Items[i]->Unique;
		if (!this->Character->Items[i]->Name.empty()) {
			item->Name = this->Character->Items[i]->Name;
		}
		item->Bound = this->Character->Items[i]->Bound;
		item->Identified = this->Character->Items[i]->Identified;
		item->Remove(this);
		if (this->Character->IsItemEquipped(this->Character->Items[i])) {
			EquipItem(*item, false);
		}
	}
	
	if (this->Character == NULL) {
		this->Player->UnitTypesNonHeroCount[this->Type->Slot]++;
		if (this->Starting) {
			this->Player->UnitTypesStartingNonHeroCount[this->Type->Slot]++;
		}
	} else {
		this->Player->Heroes.push_back(this->Character->GetFullName());
		
		if (this->Character->Custom) {
			this->Player->CustomHeroUnit = this;
		}
	}
	
	this->ChooseVariation(); //choose a new variation now
	for (int i = 0; i < MaxImageLayers; ++i) {
		ChooseVariation(NULL, false, i);
	}
	this->UpdateButtonIcons();
	this->UpdateXPRequired();
}

void CUnit::ChooseVariation(const CUnitType *new_type, bool ignore_old_variation, int image_layer)
{
	std::string priority_variation;
	if (image_layer == -1) {
		if (this->Character != NULL && !this->Character->HairVariation.empty()) {
			priority_variation = this->Character->HairVariation;
		} else if (this->Type->VarInfo[this->Variation]) {
			priority_variation = this->Type->VarInfo[this->Variation]->VariationId;
		}
	} else {
		if (image_layer == HairImageLayer && this->Character != NULL && !this->Character->HairVariation.empty()) {
			priority_variation = this->Character->HairVariation;
		} else if (this->LayerVariation[image_layer] != -1 && this->LayerVariation[image_layer] < ((int) this->Type->LayerVarInfo[image_layer].size())) {
			priority_variation = this->Type->LayerVarInfo[image_layer][this->LayerVariation[image_layer]]->VariationId;
		}
	}
	
	std::vector<int> type_variations;
	int variation_max = image_layer == -1 ? VariationMax : (new_type != NULL ? new_type->LayerVarInfo[image_layer].size() : this->Type->LayerVarInfo[image_layer].size());
	for (int i = 0; i < variation_max; ++i) {
		VariationInfo *varinfo = image_layer == -1 ? new_type != NULL ? new_type->VarInfo[i] : this->Type->VarInfo[i] : (new_type != NULL ? new_type->LayerVarInfo[image_layer][i] : this->Type->LayerVarInfo[image_layer][i]);
		if (!varinfo) {
			continue;
		}
		if (varinfo->ResourceMin && this->ResourcesHeld < varinfo->ResourceMin) {
			continue;
		}
		if (varinfo->ResourceMax && this->ResourcesHeld > varinfo->ResourceMax) {
			continue;
		}
		if (varinfo->Terrains.size() > 0) {
			if (!Map.Info.IsPointOnMap(this->tilePos, this->MapLayer)) {
				continue;
			}
			bool terrain_check = false;
			for (int x = 0; x < this->Type->TileWidth; ++x) {
				for (int y = 0; y < this->Type->TileHeight; ++y) {
					if (Map.Info.IsPointOnMap(this->tilePos + Vec2i(x, y), this->MapLayer) && std::find(varinfo->Terrains.begin(), varinfo->Terrains.end(), Map.GetTileTopTerrain(this->tilePos + Vec2i(x, y), false, this->MapLayer)) != varinfo->Terrains.end()) {
						terrain_check = true;
						break;
					}
				}
				if (terrain_check) {
					break;
				}
			}
			if (!terrain_check) {
				continue;
			}
		}
		bool upgrades_check = true;
		bool requires_weapon = false;
		bool found_weapon = false;
		bool requires_shield = false;
		bool found_shield = false;
		for (int u = 0; u < VariationMax; ++u) {
			if (!varinfo->UpgradesRequired[u].empty()) {
				if (CUpgrade::Get(varinfo->UpgradesRequired[u])->Weapon) {
					requires_weapon = true;
					if (UpgradeIdentAllowed(*this->Player, varinfo->UpgradesRequired[u].c_str()) == 'R' || this->IndividualUpgrades[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID]) {
						found_weapon = true;
					}
				} else if (CUpgrade::Get(varinfo->UpgradesRequired[u])->Shield) {
					requires_shield = true;
					if (UpgradeIdentAllowed(*this->Player, varinfo->UpgradesRequired[u].c_str()) == 'R' || this->IndividualUpgrades[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID]) {
						found_shield = true;
					}
				} else if (UpgradeIdentAllowed(*this->Player, varinfo->UpgradesRequired[u].c_str()) != 'R' && this->IndividualUpgrades[CUpgrade::Get(varinfo->UpgradesRequired[u])->ID] == false) {
					upgrades_check = false;
					break;
				}
			}
			if (!varinfo->UpgradesForbidden[u].empty() && (UpgradeIdentAllowed(*this->Player, varinfo->UpgradesForbidden[u].c_str()) == 'R' || this->IndividualUpgrades[CUpgrade::Get(varinfo->UpgradesForbidden[u])->ID] == true)) {
				upgrades_check = false;
				break;
			}
		}
		for (size_t j = 0; j < varinfo->ItemClassesNotEquipped.size(); ++j) {
			if (IsItemClassEquipped(varinfo->ItemClassesNotEquipped[j])) {
				upgrades_check = false;
				break;
			}
		}
		for (size_t j = 0; j < varinfo->ItemsNotEquipped.size(); ++j) {
			if (IsItemTypeEquipped(varinfo->ItemsNotEquipped[j])) {
				upgrades_check = false;
				break;
			}
		}
		if (upgrades_check == false) {
			continue;
		}
		for (size_t j = 0; j < varinfo->ItemClassesEquipped.size(); ++j) {
			if (GetItemClassSlot(varinfo->ItemClassesEquipped[j]) == WeaponItemSlot) {
				requires_weapon = true;
				if (IsItemClassEquipped(varinfo->ItemClassesEquipped[j])) {
					found_weapon = true;
				}
			} else if (GetItemClassSlot(varinfo->ItemClassesEquipped[j]) == ShieldItemSlot) {
				requires_shield = true;
				if (IsItemClassEquipped(varinfo->ItemClassesEquipped[j])) {
					found_shield = true;
				}
			}
		}
		for (size_t j = 0; j < varinfo->ItemsEquipped.size(); ++j) {
			if (GetItemClassSlot(varinfo->ItemsEquipped[j]->ItemClass) == WeaponItemSlot) {
				requires_weapon = true;
				if (IsItemTypeEquipped(varinfo->ItemsEquipped[j])) {
					found_weapon = true;
				}
			} else if (GetItemClassSlot(varinfo->ItemsEquipped[j]->ItemClass) == ShieldItemSlot) {
				requires_shield = true;
				if (IsItemTypeEquipped(varinfo->ItemsEquipped[j])) {
					found_shield = true;
				}
			}
		}
		if ((requires_weapon && !found_weapon) || (requires_shield && !found_shield)) {
			continue;
		}
		if (!ignore_old_variation && !priority_variation.empty() && (varinfo->VariationId.find(priority_variation) != std::string::npos || priority_variation.find(varinfo->VariationId) != std::string::npos)) { // if the priority variation's ident is included in that of a new viable variation (or vice-versa), choose the latter automatically
			this->SetVariation(i, new_type, image_layer);
			type_variations.clear();
			break;
		}
		type_variations.push_back(i);
	}
	if (type_variations.size() > 0) {
		this->SetVariation(type_variations[SyncRand(type_variations.size())], new_type, image_layer);
	}
}

void CUnit::SetVariation(int new_variation, const CUnitType *new_type, int image_layer)
{
	if (image_layer == -1) {
		if (
			(this->Type->VarInfo[this->Variation] && this->Type->VarInfo[this->Variation]->Animations)
			|| (new_type == NULL && this->Type->VarInfo[new_variation] && this->Type->VarInfo[new_variation]->Animations)
			|| (new_type != NULL && new_type->VarInfo[new_variation]->Animations)
		) { //if the old (if any) or the new variation has specific animations, set the unit's frame to its type's still frame
			this->Frame = this->Type->StillFrame;
		}
		this->Variation = new_variation;
	} else {
		this->LayerVariation[image_layer] = new_variation;
	}
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

void CUnit::ChooseButtonIcon(int button_action)
{
	if (button_action == ButtonAttack) {
		if (this->EquippedItems[ArrowsItemSlot].size() > 0 && this->EquippedItems[ArrowsItemSlot][0]->GetIcon().Icon != NULL) {
			this->ButtonIcons[button_action] = this->EquippedItems[ArrowsItemSlot][0]->GetIcon().Icon;
			return;
		}
		
		if (this->EquippedItems[WeaponItemSlot].size() > 0 && this->EquippedItems[WeaponItemSlot][0]->GetIcon().Icon != NULL) {
			this->ButtonIcons[button_action] = this->EquippedItems[WeaponItemSlot][0]->GetIcon().Icon;
			return;
		}
	} else if (button_action == ButtonStop) {
		if (this->EquippedItems[ShieldItemSlot].size() > 0 && this->EquippedItems[ShieldItemSlot][0]->Type->ItemClass == ShieldItemClass && this->EquippedItems[ShieldItemSlot][0]->GetIcon().Icon != NULL) {
			this->ButtonIcons[button_action] = this->EquippedItems[ShieldItemSlot][0]->GetIcon().Icon;
			return;
		}
	} else if (button_action == ButtonMove) {
		if (this->EquippedItems[BootsItemSlot].size() > 0 && this->EquippedItems[BootsItemSlot][0]->GetIcon().Icon != NULL) {
			this->ButtonIcons[button_action] = this->EquippedItems[BootsItemSlot][0]->GetIcon().Icon;
			return;
		}
	} else if (button_action == ButtonStandGround) {
		if (this->EquippedItems[ArrowsItemSlot].size() > 0 && this->EquippedItems[ArrowsItemSlot][0]->Type->ButtonIcons.find(button_action) != this->EquippedItems[ArrowsItemSlot][0]->Type->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->EquippedItems[ArrowsItemSlot][0]->Type->ButtonIcons.find(button_action)->second.Icon;
			return;
		}

		if (this->EquippedItems[WeaponItemSlot].size() > 0 && this->EquippedItems[WeaponItemSlot][0]->Type->ButtonIcons.find(button_action) != this->EquippedItems[WeaponItemSlot][0]->Type->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->EquippedItems[WeaponItemSlot][0]->Type->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
	}
	
	if (this->Type->VarInfo[this->Variation] && this->Type->VarInfo[this->Variation]->ButtonIcons.find(button_action) != this->Type->VarInfo[this->Variation]->ButtonIcons.end()) {
		this->ButtonIcons[button_action] = this->Type->VarInfo[this->Variation]->ButtonIcons[button_action].Icon;
		return;
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (this->LayerVariation[i] == -1 || this->LayerVariation[i] >= ((int) this->Type->LayerVarInfo[i].size())) {
			continue;
		}
		VariationInfo *varinfo = this->Type->LayerVarInfo[i][this->LayerVariation[i]];
		if (varinfo && varinfo->ButtonIcons.find(button_action) != varinfo->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = varinfo->ButtonIcons[button_action].Icon;
			return;
		}
	}

	int all_upgrades_size = AllUpgrades.size();
	
	for (int i = (NumUpgradeModifiers - 1); i >= 0; --i) {
		if (this->Player->Allow.Upgrades[UpgradeModifiers[i]->UpgradeId] == 'R' && UpgradeModifiers[i]->ApplyTo[this->Type->Slot] == 'X') {
			if (
				(
					(button_action == ButtonAttack && (AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Weapon || AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Arrows))
					|| (button_action == ButtonStop && AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Shield)
					|| (button_action == ButtonMove && AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Boots)
				)
				&& AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Item->Icon.Icon != NULL
			) {
				this->ButtonIcons[button_action] = AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Item->Icon.Icon;
				return;
			} else if (button_action == ButtonStandGround && (AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Weapon || AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Arrows) && AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Item->ButtonIcons.find(button_action) != AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Item->ButtonIcons.end()) {
				this->ButtonIcons[button_action] = AllUpgrades[UpgradeModifiers[i]->UpgradeId]->Item->ButtonIcons.find(button_action)->second.Icon;
				return;
			}
		}
	}
	
	if (button_action == ButtonAttack) {
		if (this->Type->DefaultEquipment.find(ArrowsItemSlot) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(ArrowsItemSlot)->second->Icon.Icon != NULL) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(ArrowsItemSlot)->second->Icon.Icon;
			return;
		}
		
		if (this->Type->DefaultEquipment.find(WeaponItemSlot) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(WeaponItemSlot)->second->Icon.Icon != NULL) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(WeaponItemSlot)->second->Icon.Icon;
			return;
		}
	} else if (button_action == ButtonStop) {
		if (this->Type->DefaultEquipment.find(ShieldItemSlot) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(ShieldItemSlot)->second->ItemClass == ShieldItemClass && this->Type->DefaultEquipment.find(ShieldItemSlot)->second->Icon.Icon != NULL) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(ShieldItemSlot)->second->Icon.Icon;
			return;
		}
	} else if (button_action == ButtonMove) {
		if (this->Type->DefaultEquipment.find(BootsItemSlot) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(BootsItemSlot)->second->Icon.Icon != NULL) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(BootsItemSlot)->second->Icon.Icon;
			return;
		}
	} else if (button_action == ButtonStandGround) {
		if (this->Type->DefaultEquipment.find(ArrowsItemSlot) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(ArrowsItemSlot)->second->ButtonIcons.find(button_action) != this->Type->DefaultEquipment.find(ArrowsItemSlot)->second->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(ArrowsItemSlot)->second->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
		
		if (this->Type->DefaultEquipment.find(WeaponItemSlot) != this->Type->DefaultEquipment.end() && this->Type->DefaultEquipment.find(WeaponItemSlot)->second->ButtonIcons.find(button_action) != this->Type->DefaultEquipment.find(WeaponItemSlot)->second->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = this->Type->DefaultEquipment.find(WeaponItemSlot)->second->ButtonIcons.find(button_action)->second.Icon;
			return;
		}
	}
	
	if (this->Type->ButtonIcons.find(button_action) != this->Type->ButtonIcons.end()) {
		this->ButtonIcons[button_action] = this->Type->ButtonIcons.find(button_action)->second.Icon;
		return;
	}
	
	if (this->Type->Civilization != -1) {
		int civilization = this->Type->Civilization;
		int faction = this->Type->Faction;
		
		if (faction == -1 && this->Player->Race == civilization) {
			faction = this->Player->Faction;
		}
		
		if (faction != -1 && PlayerRaces.Factions[civilization][faction]->ButtonIcons.find(button_action) != PlayerRaces.Factions[civilization][faction]->ButtonIcons.end()) {
			this->ButtonIcons[button_action] = PlayerRaces.Factions[civilization][faction]->ButtonIcons[button_action].Icon;
			return;
		} else if (PlayerRaces.ButtonIcons[civilization].find(button_action) != PlayerRaces.ButtonIcons[civilization].end()) {
			this->ButtonIcons[button_action] = PlayerRaces.ButtonIcons[civilization][button_action].Icon;
			return;
		}
	}
	
	if (this->ButtonIcons.find(button_action) != this->ButtonIcons.end()) { //if no proper button icon found, make sure any old button icon set for this button action isn't used either
		this->ButtonIcons.erase(button_action);
	}
}

void CUnit::EquipItem(CUnit &item, bool affect_character)
{
	int item_class = item.Type->ItemClass;
	int item_slot = GetItemClassSlot(item_class);
	
	if (item_slot == -1) {
		fprintf(stderr, "Trying to equip item of type \"%s\", which has no item slot.\n", item.GetTypeName().c_str());
		return;
	}
	
	if (GetItemSlotQuantity(item_slot) > 0 && EquippedItems[item_slot].size() == GetItemSlotQuantity(item_slot)) {
		DeequipItem(*EquippedItems[item_slot][EquippedItems[item_slot].size() - 1]);
	}
	
	if (item_slot == WeaponItemSlot && EquippedItems[item_slot].size() == 0) {
		// remove the upgrade modifiers from weapon technologies or from abilities which require the base weapon class but aren't compatible with this weapon's class; and apply upgrade modifiers from abilities which require this weapon's class
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (
				(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Weapon && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X')
				|| (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ability && this->IndividualUpgrades[UpgradeModifiers[z]->UpgradeId] && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.size() > 0 && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), this->Type->WeaponClasses[0]) != AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end() && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), item_class) == AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end())
			) {
				RemoveIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			} else if (
				AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ability && this->IndividualUpgrades[UpgradeModifiers[z]->UpgradeId] && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.size() > 0 && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), this->Type->WeaponClasses[0]) == AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end() && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), item_class) != AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end()
			) {
				ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	} else if (item_slot == ShieldItemSlot && EquippedItems[item_slot].size() == 0) {
		// remove the upgrade modifiers from shield technologies
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Shield && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') {
				RemoveIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	} else if (item_slot == BootsItemSlot && EquippedItems[item_slot].size() == 0) {
		// remove the upgrade modifiers from boots technologies
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Boots && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') {
				RemoveIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	} else if (item_slot == ArrowsItemSlot && EquippedItems[item_slot].size() == 0) {
		// remove the upgrade modifiers from arrows technologies
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Arrows && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') {
				RemoveIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	}
	
	if (!IsNetworkGame() && Character && Character->Persistent && this->Player->AiEnabled == false && affect_character) {
		if (Character->GetItem(item) != NULL) {
			if (!Character->IsItemEquipped(Character->GetItem(item))) {
				Character->EquippedItems[item_slot].push_back(Character->GetItem(item));
				SaveHero(Character);
			} else {
				fprintf(stderr, "Item is not equipped by character %s's unit, but is equipped by the character itself.\n", Character->GetFullName().c_str());
			}
		} else {
			fprintf(stderr, "Item is present in the inventory of the character %s's unit, but not in the character's inventory itself.\n", Character->GetFullName().c_str());
		}
	}
	EquippedItems[item_slot].push_back(&item);
	
	//change variation, if the current one has become forbidden
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (varinfo && (std::find(varinfo->ItemClassesNotEquipped.begin(), varinfo->ItemClassesNotEquipped.end(), item.Type->ItemClass) != varinfo->ItemClassesNotEquipped.end() || std::find(varinfo->ItemsNotEquipped.begin(), varinfo->ItemsNotEquipped.end(), item.Type) != varinfo->ItemsNotEquipped.end())) {
		ChooseVariation(); //choose a new variation now
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (this->LayerVariation[i] == -1 || this->LayerVariation[i] >= ((int) this->Type->LayerVarInfo[i].size())) {
			continue;
		}
		VariationInfo *varinfo = Type->LayerVarInfo[i][this->LayerVariation[i]];
		if (std::find(varinfo->ItemClassesNotEquipped.begin(), varinfo->ItemClassesNotEquipped.end(), item.Type->ItemClass) != varinfo->ItemClassesNotEquipped.end() || std::find(varinfo->ItemsNotEquipped.begin(), varinfo->ItemsNotEquipped.end(), item.Type) != varinfo->ItemsNotEquipped.end()) {
			ChooseVariation(NULL, false, i);
		}
	}
	
	if (item_slot == WeaponItemSlot || item_slot == ArrowsItemSlot) {
		this->ChooseButtonIcon(ButtonAttack);
		this->ChooseButtonIcon(ButtonStandGround);
	} else if (item_slot == ShieldItemSlot) {
		this->ChooseButtonIcon(ButtonStop);
	} else if (item_slot == BootsItemSlot) {
		this->ChooseButtonIcon(ButtonMove);
	}
	this->ChooseButtonIcon(ButtonPatrol);
	
	//add item bonuses
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (
			i == BASICDAMAGE_INDEX || i == PIERCINGDAMAGE_INDEX || i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX
			|| i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX
			|| i == ARMOR_INDEX || i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX
			|| i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX || i == SPEED_INDEX || i == BACKSTAB_INDEX
		) {
			Variable[i].Value += item.Variable[i].Value;
			Variable[i].Max += item.Variable[i].Max;
		} else if (i == HITPOINTBONUS_INDEX) {
			Variable[HP_INDEX].Value += item.Variable[i].Value;
			Variable[HP_INDEX].Max += item.Variable[i].Max;
			Variable[HP_INDEX].Increase += item.Variable[i].Increase;
		} else if (i == SIGHTRANGE_INDEX) {
			if (!SaveGameLoading) {
				MapUnmarkUnitSight(*this);
			}
			Variable[i].Value += item.Variable[i].Value;
			Variable[i].Max += item.Variable[i].Max;
			if (!SaveGameLoading) {
				CurrentSightRange = Variable[i].Value;
				UpdateUnitSightRange(*this);
				MapMarkUnitSight(*this);
			}
		}
	}
}

void CUnit::DeequipItem(CUnit &item, bool affect_character)
{
	//remove item bonuses
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (
			i == BASICDAMAGE_INDEX || i == PIERCINGDAMAGE_INDEX || i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX
			|| i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX
			|| i == ARMOR_INDEX || i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX
			|| i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX || i == SPEED_INDEX || i == BACKSTAB_INDEX
		) {
			Variable[i].Value -= item.Variable[i].Value;
			Variable[i].Max -= item.Variable[i].Max;
		} else if (i == HITPOINTBONUS_INDEX) {
			Variable[HP_INDEX].Value -= item.Variable[i].Value;
			Variable[HP_INDEX].Max -= item.Variable[i].Max;
			Variable[HP_INDEX].Increase -= item.Variable[i].Increase;
		} else if (i == SIGHTRANGE_INDEX) {
			MapUnmarkUnitSight(*this);
			Variable[i].Value -= item.Variable[i].Value;
			Variable[i].Max -= item.Variable[i].Max;
			CurrentSightRange = Variable[i].Value;
			UpdateUnitSightRange(*this);
			MapMarkUnitSight(*this);
		}
	}
	
	int item_class = item.Type->ItemClass;
	int item_slot = GetItemClassSlot(item_class);
	
	if (item_slot == -1) {
		fprintf(stderr, "Trying to de-equip item of type \"%s\", which has no item slot.\n", item.GetTypeName().c_str());
		return;
	}
	
	if (!IsNetworkGame() && Character && Character->Persistent && this->Player->AiEnabled == false && affect_character) {
		if (Character->GetItem(item) != NULL) {
			if (Character->IsItemEquipped(Character->GetItem(item))) {
				Character->EquippedItems[item_slot].erase(std::remove(Character->EquippedItems[item_slot].begin(), Character->EquippedItems[item_slot].end(), Character->GetItem(item)), Character->EquippedItems[item_slot].end());
				SaveHero(Character);
			} else {
				fprintf(stderr, "Item is equipped by character %s's unit, but not by the character itself.\n", Character->GetFullName().c_str());
			}
		} else {
			fprintf(stderr, "Item is present in the inventory of the character %s's unit, but not in the character's inventory itself.\n", Character->GetFullName().c_str());
		}
	}
	EquippedItems[item_slot].erase(std::remove(EquippedItems[item_slot].begin(), EquippedItems[item_slot].end(), &item), EquippedItems[item_slot].end());
	
	if (item_slot == WeaponItemSlot && EquippedItems[item_slot].size() == 0) {
		// restore the upgrade modifiers from weapon technologies, and apply ability effects that are weapon class-specific accordingly
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (
				(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Weapon && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X')
				|| (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ability && this->IndividualUpgrades[UpgradeModifiers[z]->UpgradeId] && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.size() > 0 && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), this->Type->WeaponClasses[0]) != AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end() && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), item_class) == AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end())
			) {
				ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			} else if (
				AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ability && this->IndividualUpgrades[UpgradeModifiers[z]->UpgradeId] && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.size() > 0 && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), this->Type->WeaponClasses[0]) == AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end() && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), item_class) != AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end()
			) {
				RemoveIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	} else if (item_slot == ShieldItemSlot && EquippedItems[item_slot].size() == 0) {
		// restore the upgrade modifiers from shield technologies
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Shield && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') {
				ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	} else if (item_slot == BootsItemSlot && EquippedItems[item_slot].size() == 0) {
		// restore the upgrade modifiers from boots technologies
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Boots && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') {
				ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	} else if (item_slot == ArrowsItemSlot && EquippedItems[item_slot].size() == 0) {
		// restore the upgrade modifiers from arrows technologies
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Arrows && Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') {
				ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
		}
	}
	
	//change variation, if the current one has become forbidden
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (varinfo && (std::find(varinfo->ItemClassesEquipped.begin(), varinfo->ItemClassesEquipped.end(), item.Type->ItemClass) != varinfo->ItemClassesEquipped.end() || std::find(varinfo->ItemsEquipped.begin(), varinfo->ItemsEquipped.end(), item.Type) != varinfo->ItemsEquipped.end())) {
		ChooseVariation(); //choose a new variation now
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (this->LayerVariation[i] == -1 || this->LayerVariation[i] >= ((int) this->Type->LayerVarInfo[i].size())) {
			continue;
		}
		VariationInfo *varinfo = Type->LayerVarInfo[i][this->LayerVariation[i]];
		if (std::find(varinfo->ItemClassesEquipped.begin(), varinfo->ItemClassesEquipped.end(), item.Type->ItemClass) != varinfo->ItemClassesEquipped.end() || std::find(varinfo->ItemsEquipped.begin(), varinfo->ItemsEquipped.end(), item.Type) != varinfo->ItemsEquipped.end()) {
			ChooseVariation(NULL, false, i);
		}
	}
	
	if (item_slot == WeaponItemSlot || item_slot == ArrowsItemSlot) {
		this->ChooseButtonIcon(ButtonAttack);
		this->ChooseButtonIcon(ButtonStandGround);
	} else if (item_slot == ShieldItemSlot) {
		this->ChooseButtonIcon(ButtonStop);
	} else if (item_slot == BootsItemSlot) {
		this->ChooseButtonIcon(ButtonMove);
	}
	this->ChooseButtonIcon(ButtonPatrol);
}

void CUnit::ReadWork(CUpgrade *work, bool affect_character)
{
	IndividualUpgradeAcquire(*this, work);
	
	if (!IsNetworkGame() && Character && Character->Persistent && this->Player->AiEnabled == false && affect_character) {
		if (std::find(Character->ReadWorks.begin(), Character->ReadWorks.end(), work) == Character->ReadWorks.end()) {
			Character->ReadWorks.push_back(work);
			SaveHero(Character);
		}
	}
}

void CUnit::ConsumeElixir(CUpgrade *elixir, bool affect_character)
{
	IndividualUpgradeAcquire(*this, elixir);
	
	if (!IsNetworkGame() && Character && Character->Persistent && this->Player->AiEnabled == false && affect_character) {
		if (std::find(Character->ConsumedElixirs.begin(), Character->ConsumedElixirs.end(), elixir) == Character->ConsumedElixirs.end()) {
			Character->ConsumedElixirs.push_back(elixir);
			SaveHero(Character);
		}
	}
}

void CUnit::ApplyAura(int aura_index)
{
	this->ApplyAuraEffect(aura_index);
			
	//apply aura to all appropriate nearby units
	int aura_range = AuraRange - (this->Type->TileWidth - 1);
	std::vector<CUnit *> table;
	SelectAroundUnit(*this, aura_range, table, MakeOrPredicate(HasSamePlayerAs(*this->Player), IsAlliedWith(*this->Player)), true);
	for (size_t i = 0; i != table.size(); ++i) {
		table[i]->ApplyAuraEffect(aura_index);
	}
	
	table.clear();
	SelectAroundUnit(*this, aura_range, table, MakeOrPredicate(MakeOrPredicate(HasSamePlayerAs(*this->Player), IsAlliedWith(*this->Player)), HasSamePlayerAs(Players[PlayerNumNeutral])), true);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->UnitInside) {
			CUnit *uins = table[i]->UnitInside;
			for (int j = 0; j < table[i]->InsideCount; ++j, uins = uins->NextContained) {
				if (uins->Player == this->Player || uins->IsAllied(*this->Player)) {
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
		if (this->Type->BoolFlag[BUILDING_INDEX].value || !this->IsInCombat()) {
			return;
		}
		effect_index = LEADERSHIP_INDEX;
	} else if (aura_index == REGENERATIONAURA_INDEX) {
		if (!this->Type->BoolFlag[ORGANIC_INDEX].value || this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableMax)) {
			return;
		}
		effect_index = REGENERATION_INDEX;
	}
	
	if (effect_index == -1) {
		return;
	}
	
	this->Variable[effect_index].Enable = 1;
	this->Variable[effect_index].Max = std::max(2, this->Variable[effect_index].Max);
	this->Variable[effect_index].Value = std::max(2, this->Variable[effect_index].Value);
}

void CUnit::SetPrefix(CUpgrade *prefix)
{
	if (Prefix != NULL) {
		for (size_t z = 0; z < Prefix->UpgradeModifiers.size(); ++z) {
			RemoveIndividualUpgradeModifier(*this, Prefix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value -= Prefix->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max -= Prefix->MagicLevel;
	}
	if (!IsNetworkGame() && Container && Container->Character && Container->Character->Persistent && Container->Player->AiEnabled == false && Container->Character->GetItem(*this) != NULL && Container->Character->GetItem(*this)->Prefix != prefix) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(*this)->Prefix = prefix;
		SaveHero(Container->Character);
	}
	Prefix = prefix;
	if (Prefix != NULL) {
		for (size_t z = 0; z < Prefix->UpgradeModifiers.size(); ++z) {
			ApplyIndividualUpgradeModifier(*this, Prefix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value += Prefix->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max += Prefix->MagicLevel;
	}
	
	this->UpdateItemName();
}

void CUnit::SetSuffix(CUpgrade *suffix)
{
	if (Suffix != NULL) {
		for (size_t z = 0; z < Suffix->UpgradeModifiers.size(); ++z) {
			RemoveIndividualUpgradeModifier(*this, Suffix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value -= Suffix->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max -= Suffix->MagicLevel;
	}
	if (!IsNetworkGame() && Container && Container->Character && Container->Character->Persistent && Container->Player->AiEnabled == false && Container->Character->GetItem(*this) != NULL && Container->Character->GetItem(*this)->Suffix != suffix) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(*this)->Suffix = suffix;
		SaveHero(Container->Character);
	}
	Suffix = suffix;
	if (Suffix != NULL) {
		for (size_t z = 0; z < Suffix->UpgradeModifiers.size(); ++z) {
			ApplyIndividualUpgradeModifier(*this, Suffix->UpgradeModifiers[z]);
		}
		this->Variable[MAGICLEVEL_INDEX].Value += Suffix->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max += Suffix->MagicLevel;
	}
	
	this->UpdateItemName();
}

void CUnit::SetSpell(SpellType *spell)
{
	if (!IsNetworkGame() && Container && Container->Character && Container->Character->Persistent && Container->Player->AiEnabled == false && Container->Character->GetItem(*this) != NULL && Container->Character->GetItem(*this)->Spell != spell) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(*this)->Spell = spell;
		SaveHero(Container->Character);
	}
	Spell = spell;
	
	this->UpdateItemName();
}

void CUnit::SetWork(CUpgrade *work)
{
	if (this->Work != NULL) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Work->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Work->MagicLevel;
	}
	
	if (!IsNetworkGame() && Container && Container->Character && Container->Character->Persistent && Container->Player->AiEnabled == false && Container->Character->GetItem(*this) != NULL && Container->Character->GetItem(*this)->Work != work) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(*this)->Work = work;
		SaveHero(Container->Character);
	}
	
	Work = work;
	
	if (this->Work != NULL) {
		this->Variable[MAGICLEVEL_INDEX].Value += this->Work->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max += this->Work->MagicLevel;
	}
	
	this->UpdateItemName();
}

void CUnit::SetElixir(CUpgrade *elixir)
{
	if (this->Elixir != NULL) {
		this->Variable[MAGICLEVEL_INDEX].Value -= this->Elixir->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max -= this->Elixir->MagicLevel;
	}
	
	if (!IsNetworkGame() && Container && Container->Character && Container->Character->Persistent && Container->Player->AiEnabled == false && Container->Character->GetItem(*this) != NULL && Container->Character->GetItem(*this)->Elixir != elixir) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(*this)->Elixir = elixir;
		SaveHero(Container->Character);
	}
	
	Elixir = elixir;
	
	if (this->Elixir != NULL) {
		this->Variable[MAGICLEVEL_INDEX].Value += this->Elixir->MagicLevel;
		this->Variable[MAGICLEVEL_INDEX].Max += this->Elixir->MagicLevel;
	}
	
	this->UpdateItemName();
}

void CUnit::SetUnique(CUniqueItem *unique)
{
	if (unique != NULL) {
		SetPrefix(unique->Prefix);
		SetSuffix(unique->Suffix);
		SetSpell(unique->Spell);
		SetWork(unique->Work);
		SetElixir(unique->Elixir);
		if (unique->ResourcesHeld != 0) {
			this->SetResourcesHeld(unique->ResourcesHeld);
			this->Variable[GIVERESOURCE_INDEX].Value = unique->ResourcesHeld;
			this->Variable[GIVERESOURCE_INDEX].Max = unique->ResourcesHeld;
			this->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
		Name = unique->Name;
		Unique = unique;
	} else {
		Name = "";
		Unique = NULL;
		SetPrefix(NULL);
		SetSuffix(NULL);
		SetSpell(NULL);
		SetWork(NULL);
		SetElixir(NULL);
	}
}

void CUnit::Identify()
{
	if (!IsNetworkGame() && Container && Container->Character && Container->Character->Persistent && Container->Player->AiEnabled == false && Container->Character->GetItem(*this) != NULL && Container->Character->GetItem(*this)->Identified != true) { //update the persistent item, if applicable and if it hasn't been updated yet
		Container->Character->GetItem(*this)->Identified = true;
		SaveHero(Container->Character);
	}
	
	this->Identified = true;
	
	if (this->Container != NULL && this->Container->Player == ThisPlayer) {
		this->Container->Player->Notify(NotifyGreen, this->Container->tilePos, this->Container->MapLayer, _("%s has identified the %s!"), this->Container->GetMessageName().c_str(), this->GetMessageName().c_str());
	}
}

void CUnit::CheckIdentification()
{
	if (!HasInventory()) {
		return;
	}
	
	CUnit *uins = this->UnitInside;
	
	for (int i = 0; i < this->InsideCount; ++i, uins = uins->NextContained) {
		if (!uins->Type->BoolFlag[ITEM_INDEX].value) {
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
		this->Variable[MANA_INDEX].Value += mana_change;
		
		this->CheckIdentification();
	} else if (variable == KNOWLEDGEWARFARE_INDEX) {
		int hp_change = (this->Variable[variable].Value / 5) - ((this->Variable[variable].Value - change) / 5); // +1 max HP for every 5 levels in Knowledge (Warfare)
		this->Variable[HP_INDEX].Max += hp_change;
		this->Variable[HP_INDEX].Value += hp_change;
	}
}

void CUnit::UpdateItemName()
{
	if (this->Unique) {
		Name = _(this->Unique->Name.c_str());
		return;
	}
	
	Name = "";
	if (Prefix == NULL && Spell == NULL && Work == NULL && Elixir == NULL && Suffix == NULL) {
		return;
	}
	
	if (Prefix != NULL) {
		Name += _(Prefix->Name.c_str());
		Name += " ";
	}
	if (Work != NULL) {
		Name += _(Work->Name.c_str());
	} else if (Elixir != NULL) {
		Name += _(Elixir->Name.c_str());
	} else {
		Name += GetTypeName();
	}
	if (Suffix != NULL) {
		Name += " ";
		Name += _(Suffix->Name.c_str());
	} else if (Spell != NULL) {
		Name += " ";
		Name += _("of");
		Name += " ";
		Name += _(Spell->Name.c_str());
	}
}

void CUnit::GenerateDrop()
{
	bool base_based_mission = false;
	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].NumTownHalls > 0 || Players[p].LostTownHallTimer) {
			base_based_mission = true;
		}
	}
	
	if (this->Type->BoolFlag[ORGANIC_INDEX].value && !this->Character && !this->Type->BoolFlag[FAUNA_INDEX].value && base_based_mission) { //if the unit is organic and isn't a character (and isn't fauna) and this is a base-based mission, don't generate a drop
		return;
	}
	
	int dropper_civilization = this->Type->Civilization;
	int dropper_faction = -1;
	if (dropper_civilization != -1 && this->Type->Faction != -1) {
		dropper_faction = this->Type->Faction;
	} else if (dropper_civilization != -1 && this->Player->Race == dropper_civilization && this->Player->Faction != -1) {
		dropper_faction = this->Player->Faction;
	}
	
	Vec2i drop_pos = this->tilePos;
	drop_pos.x += SyncRand(this->Type->TileWidth);
	drop_pos.y += SyncRand(this->Type->TileHeight);
	CUnit *droppedUnit = NULL;
	CUnitType *chosen_drop = NULL;
	std::vector<CUnitType *> potential_drops;
	for (size_t i = 0; i < this->Type->Drops.size(); ++i) {
		potential_drops.push_back(this->Type->Drops[i]);
	}
	if (this->Player->AiEnabled) {
		for (size_t i = 0; i < this->Type->AiDrops.size(); ++i) {
			potential_drops.push_back(this->Type->AiDrops[i]);
		}
		for (std::map<std::string, std::vector<CUnitType *>>::const_iterator iterator = this->Type->ModAiDrops.begin(); iterator != this->Type->ModAiDrops.end(); ++iterator) {
			for (size_t i = 0; i < iterator->second.size(); ++i) {
				potential_drops.push_back(iterator->second[i]);
			}
		}
	}
	if (potential_drops.size() > 0) {
		chosen_drop = potential_drops[SyncRand(potential_drops.size())];
	}
		
	if (chosen_drop != NULL) {
		if ((chosen_drop->BoolFlag[ITEM_INDEX].value || chosen_drop->BoolFlag[POWERUP_INDEX].value) && (Map.Field(drop_pos, this->MapLayer)->Flags & MapFieldItem)) { //if the dropped unit is an item, and there's already another item there, search for another spot
			Vec2i resPos;
			FindNearestDrop(*chosen_drop, drop_pos, resPos, LookingW, this->MapLayer);
			droppedUnit = MakeUnitAndPlace(resPos, *chosen_drop, &Players[PlayerNumNeutral], this->MapLayer);
		} else {
			droppedUnit = MakeUnitAndPlace(drop_pos, *chosen_drop, &Players[PlayerNumNeutral], this->MapLayer);
		}
			
		if (droppedUnit != NULL) {
			if (droppedUnit->Type->BoolFlag[FAUNA_INDEX].value) {
				droppedUnit->Name = droppedUnit->Type->GeneratePersonalName(NULL, droppedUnit->Variable[GENDER_INDEX].Value);
			}
			
			droppedUnit->GenerateSpecialProperties(this);
			
			if (droppedUnit->Type->BoolFlag[ITEM_INDEX].value && !droppedUnit->Unique) { //save the initial cycle items were placed in the ground to destroy them if they have been there for too long
				int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
				if (droppedUnit->Prefix != NULL || droppedUnit->Suffix != NULL || droppedUnit->Spell != NULL || droppedUnit->Work != NULL || droppedUnit->Elixir != NULL) {
					ttl_cycles *= 4;
				}
				droppedUnit->TTL = GameCycle + ttl_cycles;
			}
		}
	}
}

void CUnit::GenerateSpecialProperties(CUnit *dropper)
{
	int magic_affix_chance = 10; //10% chance of the unit having a magic prefix or suffix
	int unique_chance = 5; //0.5% chance of the unit being unique
	if (dropper != NULL) {
		if (dropper->Character) { //if the dropper is a character, multiply the chances of the item being magic or unique by the character's level
			magic_affix_chance *= dropper->Character->Level;
			unique_chance *= dropper->Character->Level;
		} else if (dropper->Type->BoolFlag[BUILDING_INDEX].value) { //if the dropper is a building, multiply the chances of the drop being magic or unique by a factor according to whether the building itself is magic/unique
			int chance_multiplier = 2;
			if (dropper->Unique) {
				chance_multiplier += 8;
			} else {
				if (dropper->Prefix != NULL) {
					chance_multiplier += 1;
				}
				if (dropper->Suffix != NULL) {
					chance_multiplier += 1;
				}
			}
			magic_affix_chance *= chance_multiplier;
			unique_chance *= chance_multiplier;
		}
	}

	if (SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GenerateWork(dropper);
	}
	if (SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GeneratePrefix(dropper);
	}
	if (SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GenerateSuffix(dropper);
	}
	if (this->Prefix == NULL && this->Suffix == NULL && this->Work == NULL && this->Elixir == NULL && SyncRand(100) >= (100 - magic_affix_chance)) {
		this->GenerateSpell(dropper);
	}
	if (SyncRand(1000) >= (1000 - unique_chance)) {
		this->GenerateUnique(dropper);
	}
	
	if (this->Type->BoolFlag[ITEM_INDEX].value && (this->Prefix != NULL || this->Suffix != NULL)) {
		this->Identified = false;
	}
	
	if (
		this->Prefix == NULL && this->Suffix == NULL && this->Spell == NULL && this->Work == NULL && this->Elixir == NULL
		&& (this->Type->ItemClass == ScrollItemClass || this->Type->ItemClass == BookItemClass || this->Type->ItemClass == RingItemClass || this->Type->ItemClass == AmuletItemClass || this->Type->ItemClass == HornItemClass)
	) { //scrolls, books, jewelry and horns must always have a property
		this->GenerateSpecialProperties(dropper);
	}
}
			
void CUnit::GeneratePrefix(CUnit *dropper)
{
	std::vector<CUpgrade *> potential_prefixes;
	for (size_t i = 0; i < this->Type->Affixes.size(); ++i) {
		if ((this->Type->ItemClass == -1 && this->Type->Affixes[i]->MagicPrefix) || (this->Type->ItemClass != -1 && this->Type->Affixes[i]->ItemPrefix[Type->ItemClass])) {
			potential_prefixes.push_back(this->Type->Affixes[i]);
		}
	}
	if (dropper != NULL) {
		for (size_t i = 0; i < dropper->Type->DropAffixes.size(); ++i) {
			if (this->Type->ItemClass != -1 && dropper->Type->DropAffixes[i]->ItemPrefix[Type->ItemClass]) {
				potential_prefixes.push_back(dropper->Type->DropAffixes[i]);
			}
		}
	}
	
	if (potential_prefixes.size() > 0) {
		SetPrefix(potential_prefixes[SyncRand(potential_prefixes.size())]);
	}
}

void CUnit::GenerateSuffix(CUnit *dropper)
{
	std::vector<CUpgrade *> potential_suffixes;
	for (size_t i = 0; i < this->Type->Affixes.size(); ++i) {
		if ((this->Type->ItemClass == -1 && this->Type->Affixes[i]->MagicSuffix) || (this->Type->ItemClass != -1 && this->Type->Affixes[i]->ItemSuffix[Type->ItemClass])) {
			if (Prefix == NULL || !this->Type->Affixes[i]->IncompatibleAffixes[Prefix->ID]) { //don't allow a suffix incompatible with the prefix to appear
				potential_suffixes.push_back(this->Type->Affixes[i]);
			}
		}
	}
	if (dropper != NULL) {
		for (size_t i = 0; i < dropper->Type->DropAffixes.size(); ++i) {
			if (this->Type->ItemClass != -1 && dropper->Type->DropAffixes[i]->ItemSuffix[Type->ItemClass]) {
				if (Prefix == NULL || !dropper->Type->DropAffixes[i]->IncompatibleAffixes[Prefix->ID]) { //don't allow a suffix incompatible with the prefix to appear
					potential_suffixes.push_back(dropper->Type->DropAffixes[i]);
				}
			}
		}
	}
	
	if (potential_suffixes.size() > 0) {
		SetSuffix(potential_suffixes[SyncRand(potential_suffixes.size())]);
	}
}

void CUnit::GenerateSpell(CUnit *dropper)
{
	std::vector<SpellType *> potential_spells;
	if (dropper != NULL) {
		for (size_t i = 0; i < dropper->Type->DropSpells.size(); ++i) {
			if (this->Type->ItemClass != -1 && dropper->Type->DropSpells[i]->ItemSpell[Type->ItemClass]) {
				potential_spells.push_back(dropper->Type->DropSpells[i]);
			}
		}
	}
	
	if (potential_spells.size() > 0) {
		SetSpell(potential_spells[SyncRand(potential_spells.size())]);
	}
}

void CUnit::GenerateWork(CUnit *dropper)
{
	std::vector<CUpgrade *> potential_works;
	for (size_t i = 0; i < this->Type->Affixes.size(); ++i) {
		if (this->Type->ItemClass != -1 && this->Type->Affixes[i]->Work == this->Type->ItemClass && !this->Type->Affixes[i]->UniqueOnly) {
			potential_works.push_back(this->Type->Affixes[i]);
		}
	}
	if (dropper != NULL) {
		for (size_t i = 0; i < dropper->Type->DropAffixes.size(); ++i) {
			if (this->Type->ItemClass != -1 && dropper->Type->DropAffixes[i]->Work == this->Type->ItemClass && !dropper->Type->DropAffixes[i]->UniqueOnly) {
				potential_works.push_back(dropper->Type->DropAffixes[i]);
			}
		}
		
		int dropper_civilization = dropper->Type->Civilization;
		if (dropper_civilization != -1) {
			for (size_t i = 0; i < PlayerRaces.LiteraryWorks[dropper_civilization].size(); ++i) {
				if (this->Type->ItemClass != -1 && PlayerRaces.LiteraryWorks[dropper_civilization][i]->Work == this->Type->ItemClass && !PlayerRaces.LiteraryWorks[dropper_civilization][i]->UniqueOnly) {
					potential_works.push_back(PlayerRaces.LiteraryWorks[dropper_civilization][i]);
				}
			}
		}
	}
	
	if (potential_works.size() > 0) {
		SetWork(potential_works[SyncRand(potential_works.size())]);
	}
}

void CUnit::GenerateUnique(CUnit *dropper)
{
	int dropper_civilization = dropper != NULL ? dropper->Type->Civilization : -1;
	
	std::vector<CUniqueItem *> potential_uniques;
	for (size_t i = 0; i < UniqueItems.size(); ++i) {
		if (
			Type == UniqueItems[i]->Type
			&& ( //the dropper unit must be capable of generating this unique item's prefix to drop the item, or else the unit must be capable of generating it on its own
				UniqueItems[i]->Prefix == NULL
				|| (dropper != NULL && std::find(dropper->Type->DropAffixes.begin(), dropper->Type->DropAffixes.end(), UniqueItems[i]->Prefix) != dropper->Type->DropAffixes.end())
				|| std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), UniqueItems[i]->Prefix) != this->Type->Affixes.end()
			)
			&& ( //the dropper unit must be capable of generating this unique item's suffix to drop the item, or else the unit must be capable of generating it on its own
				UniqueItems[i]->Suffix == NULL
				|| (dropper != NULL && std::find(dropper->Type->DropAffixes.begin(), dropper->Type->DropAffixes.end(), UniqueItems[i]->Suffix) != dropper->Type->DropAffixes.end())
				|| std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), UniqueItems[i]->Suffix) != this->Type->Affixes.end()
			)
			&& ( //the dropper unit must be capable of generating this unique item's spell to drop the item
				UniqueItems[i]->Spell == NULL
				|| (dropper != NULL && std::find(dropper->Type->DropSpells.begin(), dropper->Type->DropSpells.end(), UniqueItems[i]->Spell) != dropper->Type->DropSpells.end())
			)
			&& ( //the dropper unit must be capable of generating this unique item's work to drop the item, or else the unit must be capable of generating it on its own
				UniqueItems[i]->Work == NULL
				|| (dropper != NULL && std::find(dropper->Type->DropAffixes.begin(), dropper->Type->DropAffixes.end(), UniqueItems[i]->Work) != dropper->Type->DropAffixes.end())
				|| std::find(this->Type->Affixes.begin(), this->Type->Affixes.end(), UniqueItems[i]->Work) != this->Type->Affixes.end()
				|| (dropper_civilization != -1 && std::find(PlayerRaces.LiteraryWorks[dropper_civilization].begin(), PlayerRaces.LiteraryWorks[dropper_civilization].end(), UniqueItems[i]->Work) != PlayerRaces.LiteraryWorks[dropper_civilization].end())
			)
			&& UniqueItems[i]->CanDrop()
		) {
			potential_uniques.push_back(UniqueItems[i]);
		}
	}
	
	if (potential_uniques.size() > 0) {
		CUniqueItem *chosen_unique = potential_uniques[SyncRand(potential_uniques.size())];
		SetUnique(chosen_unique);
	}
}

void CUnit::UpdateSoldUnits()
{
	for (size_t i = 0; i < this->SoldUnits.size(); ++i) {
		DestroyAllInside(*this->SoldUnits[i]);
		LetUnitDie(*this->SoldUnits[i]);
	}
	this->SoldUnits.clear();
	
	std::vector<CCharacter *> potential_heroes;
	if (this->Type->BoolFlag[RECRUITHEROES_INDEX].value && !IsNetworkGame() && CurrentQuest == NULL && !GrandStrategy) { // allow heroes to be recruited at town halls
		int civilization_id = this->Type->Civilization;
		for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
			if (!iterator->second->Persistent || iterator->second->Deity != NULL) {
				continue;
			}
			bool hero_allowed = false;
			if (this->Player->Index == PlayerNumNeutral) { // if this is a neutral building (i.e. a mercenary camp), see if any player present can have this hero
				hero_allowed = this->CanHireMercenary(iterator->second->Type, iterator->second->Civilization);
			} else {
				hero_allowed = (iterator->second->Civilization == civilization_id && CheckDependByType(*this->Player, *iterator->second->Type, true));
			}
			if (hero_allowed && iterator->second->CanAppear()) {
				if (iterator->second->Conditions) {
					CclCommand("trigger_player = " + std::to_string((long long) this->Player->Index) + ";");
					iterator->second->Conditions->pushPreamble();
					iterator->second->Conditions->run(1);
					if (iterator->second->Conditions->popBoolean() == false) {
						continue;
					}
				}
				potential_heroes.push_back(iterator->second);
			}
		}
		if (this->Player == ThisPlayer) {
			for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
				if (iterator->second->Civilization == civilization_id && iterator->second->CanAppear()) {
					potential_heroes.push_back(iterator->second);
				}
			}
		}
	}
	
	if (this->Type->SoldUnits.empty() && potential_heroes.empty()) {
		return;
	}
	
	int sold_unit_max = 3;
	if (this->Type->BoolFlag[RECRUITHEROES_INDEX].value) {
		sold_unit_max = 1;
	}
	
	for (int i = 0; i < sold_unit_max; ++i) {
		CUnit *new_unit = NULL;
		if (!potential_heroes.empty()) {
			CCharacter *chosen_hero = potential_heroes[SyncRand(potential_heroes.size())];
			new_unit = MakeUnit(*chosen_hero->Type, &Players[PlayerNumNeutral]);
			new_unit->SetCharacter(chosen_hero->GetFullName(), chosen_hero->Custom);
			potential_heroes.erase(std::remove(potential_heroes.begin(), potential_heroes.end(), chosen_hero), potential_heroes.end());
		} else {
			CUnitType *chosen_unit_type = this->Type->SoldUnits[SyncRand(this->Type->SoldUnits.size())];
			new_unit = MakeUnit(*chosen_unit_type, &Players[PlayerNumNeutral]);
			new_unit->GenerateSpecialProperties(this);
			new_unit->Identified = true;
		}
		new_unit->Remove(this);
		this->SoldUnits.push_back(new_unit);
		if (potential_heroes.empty() && this->Type->SoldUnits.empty()) {
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
	if (!sold_unit->Type->BoolFlag[ITEM_INDEX].value) {
		sold_unit->ChangeOwner(Players[player]);
	}
	Players[player].ChangeResource(CopperCost, -sold_unit->GetPrice(), true);
	if (IsOnlySelected(*this)) {
		UI.ButtonPanel.Update();
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
	return ((Type->CorpseType && CurrentAction() == UnitActionDie) ?
		Type->CorpseType->DrawLevel :
	((CurrentAction() == UnitActionDie) ? Type->DrawLevel - 10 : Type->DrawLevel));
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

	Frame = type.StillFrame;

	if (UnitTypeVar.GetNumberVariable()) {
		Assert(!Variable);
		const unsigned int size = UnitTypeVar.GetNumberVariable();
		Variable = new CVariable[size];
		std::copy(type.MapDefaultStat.Variables, type.MapDefaultStat.Variables + size, Variable);
	} else {
		Variable = NULL;
	}

	memset(IndividualUpgrades, 0, sizeof(IndividualUpgrades));

	// Set a heading for the unit if it Handles Directions
	// Don't set a building heading, as only 1 construction direction
	//   is allowed.
	if (type.NumDirections > 1 && type.BoolFlag[NORANDOMPLACING_INDEX].value == false && type.Sprite && !type.Building) {
		Direction = (SyncRand() >> 8) & 0xFF; // random heading
		UnitUpdateHeading(*this);
	}

	// Create AutoCastSpell and SpellCoolDownTimers arrays for casters
	//Wyrmgus start
//	if (type.CanCastSpell) {
	//to avoid crashes with spell items for units who cannot ordinarily cast spells
	//Wyrmgus end
		AutoCastSpell = new char[SpellTypeTable.size()];
		SpellCoolDownTimers = new int[SpellTypeTable.size()];
		memset(SpellCoolDownTimers, 0, SpellTypeTable.size() * sizeof(int));
		if (Type->AutoCastActive) {
			memcpy(AutoCastSpell, Type->AutoCastActive, SpellTypeTable.size());
		} else {
			memset(AutoCastSpell, 0, SpellTypeTable.size());
		}
	//Wyrmgus start
//	}
	//Wyrmgus end
	Active = 1;
	Removed = 1;
	//Wyrmgus start
	if (!GameRunning && !SaveGameLoading) { //if unit was created before the game started, it is a starting unit
		Starting = 1;
	}
	//Wyrmgus end

	// Has StartingResources, Use those
	//Wyrmgus start
//	this->ResourcesHeld = type.StartingResources;
	if (type.StartingResources.size() > 0) {
		this->ResourcesHeld = type.StartingResources[SyncRand(type.StartingResources.size())];
	} else {
		this->ResourcesHeld = 0;
	}
	//Wyrmgus end

	Assert(Orders.empty());

	Orders.push_back(COrder::NewActionStill());

	Assert(NewOrder == NULL);
	NewOrder = NULL;
	Assert(SavedOrder == NULL);
	SavedOrder = NULL;
	Assert(CriticalOrder == NULL);
	CriticalOrder = NULL;
}

/**
**  Restore the saved order
**
**  @return      True if the saved order was restored
*/
bool CUnit::RestoreOrder()
{
	COrder *savedOrder = this->SavedOrder;

	if (savedOrder == NULL) {
		return false;
	}

	if (savedOrder->IsValid() == false) {
		delete savedOrder;
		this->SavedOrder = NULL;
		return false;
	}

	// Cannot delete this->Orders[0] since it is generally that order
	// which call this method.
	this->Orders[0]->Finished = true;

	//copy
	this->Orders.insert(this->Orders.begin() + 1, savedOrder);

	this->SavedOrder = NULL;
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
	if (this->SavedOrder != NULL) {
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
	if (!type.BoolFlag[VANISHES_INDEX].value && CurrentAction() != UnitActionDie) {
		player.AddUnit(*this);
		if (!SaveGameLoading) {
			// If unit is dying, it's already been lost by all players
			// don't count again
			if (type.Building) {
				// FIXME: support more races
				//Wyrmgus start
//				if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
				if (!type.BoolFlag[WALL_INDEX].value) {
				//Wyrmgus end
					player.TotalBuildings++;
				}
			} else {
				player.TotalUnits++;
				
				//Wyrmgus start
				for (size_t i = 0; i < player.QuestBuildUnits.size(); ++i) { // buildings get subtracted from the BuildUnits array in action_built
					if (std::get<1>(player.QuestBuildUnits[i]) == &type) {
						std::get<2>(player.QuestBuildUnits[i]) -= 1;
					}
				}
				
				for (size_t i = 0; i < player.QuestBuildUnitsOfClass.size(); ++i) { // buildings get subtracted from the BuildUnits array in action_built
					if (std::get<1>(player.QuestBuildUnitsOfClass[i]) == type.Class) {
						std::get<2>(player.QuestBuildUnitsOfClass[i]) -= 1;
					}
				}
				//Wyrmgus end
			}
		}
		player.UnitTypesCount[type.Slot]++;
		if (Active) {
			player.UnitTypesAiActiveCount[type.Slot]++;
		}
		//Wyrmgus start
		if (type.BoolFlag[TOWNHALL_INDEX].value) {
			player.NumTownHalls++;
		}
		if (this->Character == NULL) {
			player.UnitTypesNonHeroCount[type.Slot]++;
			if (this->Starting) {
				player.UnitTypesStartingNonHeroCount[type.Slot]++;
			}
		} else {
			player.Heroes.push_back(this->Character->GetFullName());
		}
		
		if (player.AiEnabled && player.Ai && type.BoolFlag[COWARD_INDEX].value && !type.BoolFlag[HARVESTER_INDEX].value && !type.CanTransport() && !type.CanCastSpell && Map.Info.IsPointOnMap(this->tilePos, this->MapLayer) && this->CanMove() && this->Active && this->GroupId != 0 && this->Variable[SIGHTRANGE_INDEX].Value > 0) { //assign coward, non-worker, non-transporter, non-spellcaster units to be scouts
			player.Ai->Scouts.push_back(this);
		}
		//Wyrmgus end
		player.Demand += type.Stats[player.Index].Variables[DEMAND_INDEX].Value; // food needed
	}

	// Don't Add the building if it's dying, used to load a save game
	if (type.Building && CurrentAction() != UnitActionDie) {
		// FIXME: support more races
		//Wyrmgus start
//		if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
		if (!type.BoolFlag[WALL_INDEX].value) {
		//Wyrmgus end
			player.NumBuildings++;
			//Wyrmgus start
			if (CurrentAction() == UnitActionBuilt) {
				player.NumBuildingsUnderConstruction++;
			}
			//Wyrmgus end
		}
	}
	Player = &player;
	Stats = &type.Stats[Player->Index];
	Colors = &player.UnitColors;
	if (!SaveGameLoading) {
		if (UnitTypeVar.GetNumberVariable()) {
			Assert(Variable);
			Assert(Stats->Variables);
			memcpy(Variable, Stats->Variables, UnitTypeVar.GetNumberVariable() * sizeof(*Variable));
		}
	}
	
	//Wyrmgus start
	//assign a gender to the unit
	if (this->Variable[GENDER_INDEX].Value == NoGender && this->Type->BoolFlag[ORGANIC_INDEX].value) { // Gender: 0 = Not Set, 1 = Male, 2 = Female, 3 = Asexual
		this->Variable[GENDER_INDEX].Value = SyncRand(2) + 1;
		this->Variable[GENDER_INDEX].Max = MaxGenders;
		this->Variable[GENDER_INDEX].Enable = 1;
	}
	
	//generate a personal name for the unit, if applicable
	if (!SaveGameLoading && this->Character == NULL) {
		this->UpdatePersonalName();
	}
	
	this->UpdateSoldUnits();
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
	if (unit == NULL) {
		return NULL;
	}
	unit->Init(type);
	// Only Assign if a Player was specified
	if (player) {
		unit->AssignToPlayer(*player);

		//Wyrmgus start
		unit->ChooseVariation(NULL, true);
		for (int i = 0; i < MaxImageLayers; ++i) {
			unit->ChooseVariation(NULL, true, i);
		}
		unit->UpdateButtonIcons();
		unit->UpdateXPRequired();
		//Wyrmgus end
	}

	//Wyrmgus start
	// generate a trait for the unit, if any are available (only if the editor isn't running)
	if (Editor.Running == EditorNotRunning && unit->Type->Traits.size() > 0) {
		TraitAcquire(*unit, unit->Type->Traits[SyncRand(unit->Type->Traits.size())]);
	}
	
	for (size_t i = 0; i < unit->Type->StartingAbilities.size(); ++i) {
		IndividualUpgradeAcquire(*unit, unit->Type->StartingAbilities[i]);
	}
	//Wyrmgus end
	
	if (unit->Type->OnInit) {
		unit->Type->OnInit->pushPreamble();
		unit->Type->OnInit->pushInteger(UnitNumber(*unit));
		unit->Type->OnInit->run();
	}

	//  fancy buildings: mirror buildings (but shadows not correct)
	if (type.Building && FancyBuildings
		&& unit->Type->BoolFlag[NORANDOMPLACING_INDEX].value == false && (SyncRand() & 1) != 0) {
		unit->Frame = -unit->Frame - 1;
	}
	
	//Wyrmgus start
	// make mercenary units only be available once per match
	if (type.BoolFlag[MERCENARY_INDEX].value) {
		for (int p = 0; p < PlayerMax; ++p) {
			AllowUnitId(Players[p], type.Slot, 0);
		}
	}
	//Wyrmgus end
	
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
	MapSight(*unit.Player, pos, width, height,
			 unit.Container ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f);

	if (unit.Type && unit.Type->BoolFlag[DETECTCLOAK_INDEX].value && f2) {
		MapSight(*unit.Player, pos, width, height,
				 unit.Container ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f2);
	}
	*/

	MapSight(*unit.Player, pos, width, height,
			 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f, unit.MapLayer);

	if (unit.Type && unit.Type->BoolFlag[DETECTCLOAK_INDEX].value && f2) {
		MapSight(*unit.Player, pos, width, height,
				 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f2, unit.MapLayer);
	}
	
	if (unit.Variable[ETHEREALVISION_INDEX].Value && f3) {
		MapSight(*unit.Player, pos, width, height,
				 unit.Container && unit.Container->CurrentSightRange >= unit.CurrentSightRange ? unit.Container->CurrentSightRange : unit.CurrentSightRange, f3, unit.MapLayer);
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
**  Return the unit not transported, by viewing the container recursively.
**
**  @param unit  unit from where look the first conatiner.
**
**  @return      Container of container of ... of unit. It is not null.
*/
CUnit *GetFirstContainer(const CUnit &unit)
{
	const CUnit *container = &unit;

	while (container->Container) {
		container = container->Container;
	}
	return const_cast<CUnit *>(container);
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
	CUnit *container = GetFirstContainer(unit);// First container of the unit.
	Assert(container->Type);

	MapMarkUnitSightRec(unit, container->tilePos, container->Type->TileWidth, container->Type->TileHeight,
						//Wyrmgus start
//						MapMarkTileSight, MapMarkTileDetectCloak);
						MapMarkTileSight, MapMarkTileDetectCloak, MapMarkTileDetectEthereal);
						//Wyrmgus end

	// Never mark radar, except if the top unit, and unit is usable
	if (&unit == container && !unit.IsUnusable()) {
		if (unit.Stats->Variables[RADAR_INDEX].Value) {
			MapMarkRadar(*unit.Player, unit.tilePos, unit.Type->TileWidth,
						 //Wyrmgus start
//						 unit.Type->TileHeight, unit.Stats->Variables[RADAR_INDEX].Value);
						 unit.Type->TileHeight, unit.Stats->Variables[RADAR_INDEX].Value, unit.MapLayer);
						 //Wyrmgus end
		}
		if (unit.Stats->Variables[RADARJAMMER_INDEX].Value) {
			MapMarkRadarJammer(*unit.Player, unit.tilePos, unit.Type->TileWidth,
							   //Wyrmgus start
//							   unit.Type->TileHeight, unit.Stats->Variables[RADARJAMMER_INDEX].Value);
							   unit.Type->TileHeight, unit.Stats->Variables[RADARJAMMER_INDEX].Value, unit.MapLayer);
							   //Wyrmgus end
		}
	}
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
	Assert(unit.Type);

	CUnit *container = GetFirstContainer(unit);
	Assert(container->Type);
	MapMarkUnitSightRec(unit,
						container->tilePos, container->Type->TileWidth, container->Type->TileHeight,
						//Wyrmgus start
//						MapUnmarkTileSight, MapUnmarkTileDetectCloak);
						MapUnmarkTileSight, MapUnmarkTileDetectCloak, MapUnmarkTileDetectEthereal);
						//Wyrmgus end

	// Never mark radar, except if the top unit?
	if (&unit == container && !unit.IsUnusable()) {
		if (unit.Stats->Variables[RADAR_INDEX].Value) {
			MapUnmarkRadar(*unit.Player, unit.tilePos, unit.Type->TileWidth,
						   //Wyrmgus start
//						   unit.Type->TileHeight, unit.Stats->Variables[RADAR_INDEX].Value);
						   unit.Type->TileHeight, unit.Stats->Variables[RADAR_INDEX].Value, unit.MapLayer);
						   //Wyrmgus end
		}
		if (unit.Stats->Variables[RADARJAMMER_INDEX].Value) {
			MapUnmarkRadarJammer(*unit.Player, unit.tilePos, unit.Type->TileWidth,
								 //Wyrmgus start
//								 unit.Type->TileHeight, unit.Stats->Variables[RADARJAMMER_INDEX].Value);
								 unit.Type->TileHeight, unit.Stats->Variables[RADARJAMMER_INDEX].Value, unit.MapLayer);
								 //Wyrmgus end
		}
	}
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
	if (Map.TimeOfDay[unit.MapLayer] == MorningTimeOfDay || Map.TimeOfDay[unit.MapLayer] == MiddayTimeOfDay || Map.TimeOfDay[unit.MapLayer] == AfternoonTimeOfDay) {
		unit_sight_range += unit.Variable[DAYSIGHTRANGEBONUS_INDEX].Value;
	} else if (Map.TimeOfDay[unit.MapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[unit.MapLayer] == MidnightTimeOfDay || Map.TimeOfDay[unit.MapLayer] == SecondWatchTimeOfDay) {
		unit_sight_range += unit.Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value;
	}
	unit_sight_range = std::max<int>(1, unit_sight_range);
	//Wyrmgus end
	if (unit.Constructed) { // Units under construction have no sight range.
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
	const unsigned int flags = unit.Type->FieldFlags;
	int h = unit.Type->TileHeight;          // Tile height of the unit.
	const int width = unit.Type->TileWidth; // Tile width of the unit.
	unsigned int index = unit.Offset;

	//Wyrmgus start
//	if (unit.Type->BoolFlag[VANISHES_INDEX].value) {
	if (unit.Type->BoolFlag[VANISHES_INDEX].value || unit.CurrentAction() == UnitActionDie) {
	//Wyrmgus end
		return ;
	}
	do {
		//Wyrmgus start
//		CMapField *mf = Map.Field(index);
		CMapField *mf = Map.Field(index, unit.MapLayer);
		//Wyrmgus end
		int w = width;
		do {
			mf->Flags |= flags;
			++mf;
		} while (--w);
		//Wyrmgus start
//		index += Map.Info.MapWidth;
		index += Map.Info.MapWidths[unit.MapLayer];
		//Wyrmgus end
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
			mf->Flags |= unit->Type->FieldFlags;
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
	const unsigned int flags = ~unit.Type->FieldFlags;
	const int width = unit.Type->TileWidth;
	int h = unit.Type->TileHeight;
	unsigned int index = unit.Offset;

	if (unit.Type->BoolFlag[VANISHES_INDEX].value) {
		return ;
	}
	do {
		//Wyrmgus start
//		CMapField *mf = Map.Field(index);
		CMapField *mf = Map.Field(index, unit.MapLayer);
		//Wyrmgus end

		int w = width;
		do {
			mf->Flags &= flags;//clean flags
			_UnmarkUnitFieldFlags funct(unit, mf);

			mf->UnitCache.for_each(funct);
			++mf;
		} while (--w);
		//Wyrmgus start
//		index += Map.Info.MapWidth;
		index += Map.Info.MapWidths[unit.MapLayer];
		//Wyrmgus end
	} while (--h);
}

/**
**  Add unit to a container. It only updates linked list stuff.
**
**  @param host  Pointer to container.
*/
void CUnit::AddInContainer(CUnit &host)
{
	Assert(Container == NULL);
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
		host->UnitInside = NULL;
	} else {
		if (host->UnitInside == &unit) {
			host->UnitInside = unit.NextContained;
		}
	}
	unit.Container = NULL;
	//Wyrmgus start
	//reset host attack range
	host->UpdateContainerAttackRange();
	//Wyrmgus end
}

//Wyrmgus start
void CUnit::UpdateContainerAttackRange()
{
	//reset attack range, if this unit is a transporter (or garrisonable building) from which units can attack
	if (this->Type->CanTransport() && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && this->Type->CanAttack) {
		this->Variable[ATTACKRANGE_INDEX].Enable = 0;
		this->Variable[ATTACKRANGE_INDEX].Max = 0;
		this->Variable[ATTACKRANGE_INDEX].Value = 0;
		if (this->BoardCount > 0) {
			CUnit *boarded_unit = this->UnitInside;
			for (int i = 0; i < this->InsideCount; ++i, boarded_unit = boarded_unit->NextContained) {
				if (boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX) > this->Variable[ATTACKRANGE_INDEX].Value && boarded_unit->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) { //if container has no range by itself, but the unit has range, and the unit can attack from a transporter, change the container's range to the unit's
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
	
	this->Variable[XPREQUIRED_INDEX].Value = this->Type->Stats[this->Player->Index].Variables[POINTS_INDEX].Value * 4 * this->Type->Stats[this->Player->Index].Variables[LEVEL_INDEX].Value;
	int extra_levels = this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->Index].Variables[LEVEL_INDEX].Value;
	for (int i = 1; i <= extra_levels; ++i) {
		this->Variable[XPREQUIRED_INDEX].Value += 50 * 4 * i;
	}
	this->Variable[XPREQUIRED_INDEX].Max = this->Variable[XPREQUIRED_INDEX].Value;
	this->Variable[XPREQUIRED_INDEX].Enable = 1;
	this->Variable[XP_INDEX].Enable = 1;
}

void CUnit::UpdatePersonalName(bool update_settlement_name)
{
	if (this->Character != NULL) {
		return;
	} else if (this->Type->BoolFlag[ITEM_INDEX].value || this->Unique || this->Prefix || this->Suffix) {
		this->UpdateItemName();
		return;
	}
	
	int civilization = this->Type->Civilization;
	int faction = -1;
	if (civilization != -1 && this->Type->Faction != -1) {
		faction = this->Type->Faction;
	} else if (civilization != -1 && this->Player->Race == civilization && this->Player->Faction != -1) {
		faction = this->Player->Faction;
	}
	int language = PlayerRaces.GetFactionLanguage(civilization, faction);

	// first see if can translate the current personal name
	std::string new_personal_name = PlayerRaces.TranslateName(this->Name, language);
	if (!new_personal_name.empty()) {
		this->Name = new_personal_name;
	} else {
		CFaction *faction = NULL;
		if (this->Player->Race != -1 && this->Player->Faction != -1) {
			faction = PlayerRaces.Factions[this->Player->Race][this->Player->Faction];
		}
		this->Name = this->Type->GeneratePersonalName(faction, this->Variable[GENDER_INDEX].Value);
		if (!this->Type->BoolFlag[FAUNA_INDEX].value && !this->Name.empty() && this->Trait != NULL && this->Trait->Epithets.size() > 0 && SyncRand(4) == 0) { // 25% chance to give the unit an epithet based on their trait
			this->Name += " " + this->Trait->Epithets[SyncRand(this->Trait->Epithets.size())];
		}
	}
	
	if (update_settlement_name && (this->Type->BoolFlag[TOWNHALL_INDEX].value || (this->Type->BoolFlag[BUILDING_INDEX].value && this->SettlementName.empty()))) {
		this->UpdateSettlementName();
	}
}

void CUnit::UpdateSettlementName()
{
	if (this->Removed) {
		return;
	}
	
	if (this->Type->BoolFlag[TOWNHALL_INDEX].value) {
		std::string old_settlement_name = this->SettlementName;
		
		int civilization = this->Type->Civilization;
		int faction = -1;
		if (civilization != -1 && this->Type->Faction != -1) {
			faction = this->Type->Faction;
		} else if (civilization != -1 && this->Player->Race == civilization && this->Player->Faction != -1) {
			faction = this->Player->Faction;
		}
		int language = PlayerRaces.GetFactionLanguage(civilization, faction);

		// first see if can translate the current personal name
		std::string new_settlement_name = PlayerRaces.TranslateName(this->SettlementName, language);
		if (!new_settlement_name.empty()) {
			this->SettlementName = new_settlement_name;
		} else {
			new_settlement_name = Map.GetSettlementName(this->tilePos, this->MapLayer, Vec2i(this->Type->TileWidth, this->Type->TileHeight), civilization, faction);
			if (!new_settlement_name.empty()) {
				this->SettlementName = new_settlement_name;
			} else if (civilization != -1 && faction != -1 && PlayerRaces.Factions[civilization][faction]->SettlementNames.size() > 0) {
				this->SettlementName = PlayerRaces.Factions[civilization][faction]->SettlementNames[SyncRand(PlayerRaces.Factions[civilization][faction]->SettlementNames.size())];
			} else if (civilization != -1 && PlayerRaces.Civilizations[civilization]->SettlementNames.size() > 0) {
				this->SettlementName = PlayerRaces.Civilizations[civilization]->SettlementNames[SyncRand(PlayerRaces.Civilizations[civilization]->SettlementNames.size())];
			}
		}
		
		for (int i = 0; i < this->Player->GetUnitCount(); ++i) {
			CUnit *settlement_unit = &this->Player->GetUnit(i);
			if (!settlement_unit || !settlement_unit->IsAliveOnMap() || !settlement_unit->Type->BoolFlag[BUILDING_INDEX].value || settlement_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
				continue;
			}
			if (!old_settlement_name.empty() && settlement_unit->SettlementName != old_settlement_name) {
				continue;
			}
			settlement_unit->UpdateSettlementName();
		}
	} else {
		CUnit *best_hall = NULL;
		int best_distance = -1;
		for (int i = 0; i < this->Player->GetUnitCount(); ++i) {
			CUnit *settlement_unit = &this->Player->GetUnit(i);
			if (!settlement_unit || !settlement_unit->IsAliveOnMap() || !settlement_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
				continue;
			}
			int distance = this->MapDistanceTo(*settlement_unit);
			if (!best_hall || distance < best_distance) {
				best_hall = settlement_unit;
				best_distance = distance;
			}
		}
		if (best_hall) {
			this->SettlementName = best_hall->SettlementName;
		}
	}
}

void CUnit::XPChanged()
{
	if (!this->Type->BoolFlag[ORGANIC_INDEX].value || this->Type->BoolFlag[BUILDING_INDEX].value || this->Type->BoolFlag[FAUNA_INDEX].value) {
		return;
	}
	
	if (this->Variable[XPREQUIRED_INDEX].Value == 0) {
		return;
	}
	
	while (this->Variable[XP_INDEX].Value >= this->Variable[XPREQUIRED_INDEX].Value) {
		this->Variable[XP_INDEX].Max -= this->Variable[XPREQUIRED_INDEX].Max;
		this->Variable[XP_INDEX].Value -= this->Variable[XPREQUIRED_INDEX].Value;
		if (this->Player == ThisPlayer) {
			this->Player->Notify(NotifyGreen, this->tilePos, this->MapLayer, _("%s has leveled up!"), GetMessageName().c_str());
		}
		this->IncreaseLevel(1);
	}
	
	if (!IsNetworkGame() && this->Character != NULL && this->Character->Persistent && this->Player->AiEnabled == false) {
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
**  @internal before use it, Map.Remove(unit), MapUnmarkUnitSight(unit)
**  and after Map.Insert(unit), MapMarkUnitSight(unit)
**  are often necessary. Check Flag also for Pathfinder.
*/
//Wyrmgus start
//static void UnitInXY(CUnit &unit, const Vec2i &pos)
static void UnitInXY(CUnit &unit, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
	int old_z = unit.MapLayer;
	//Wyrmgus end
	
	CUnit *unit_inside = unit.UnitInside;

	unit.tilePos = pos;
	//Wyrmgus start
//	unit.Offset = Map.getIndex(pos);
	unit.Offset = Map.getIndex(pos, z);
	unit.MapLayer = z;
	
	if (!SaveGameLoading && old_z != z) {
		UpdateUnitSightRange(unit);
	}
	//Wyrmgus end

	for (int i = unit.InsideCount; i--; unit_inside = unit_inside->NextContained) {
		//Wyrmgus start
//		UnitInXY(*unit_inside, pos);
		UnitInXY(*unit_inside, pos, z);
		//Wyrmgus end
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
	Map.Remove(*this);
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

	Map.Insert(*this);
	MarkUnitFieldFlags(*this);
	//  Recalculate the seen count.
	UnitCountSeen(*this);
	MapMarkUnitSight(*this);
	
	//Wyrmgus start
	// if there is a trap in the new tile, trigger it
	if ((this->Type->UnitType != UnitTypeFly && this->Type->UnitType != UnitTypeFlyLow) || !this->Type->BoolFlag[ORGANIC_INDEX].value) {
		const CUnitCache &cache = Map.Field(pos, z)->UnitCache;
		for (size_t i = 0; i != cache.size(); ++i) {
			CUnit &unit = *cache[i];
			if (!&unit) {
				fprintf(stderr, "Error in CUnit::MoveToXY (pos %d, %d): a unit in the tile's unit cache is NULL.\n", pos.x, pos.y);
			}
			if (unit.IsAliveOnMap() && unit.Type->BoolFlag[TRAP_INDEX].value) {
				FireMissile(unit, this, this->tilePos, this->MapLayer);
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
//Wyrmgus start
//void CUnit::Place(const Vec2i &pos)
void CUnit::Place(const Vec2i &pos, int z)
//Wyrmgus end
{
	Assert(Removed);

	if (Container) {
		MapUnmarkUnitSight(*this);
		RemoveUnitFromContainer(*this);
	}
	if (!SaveGameLoading) {
		UpdateUnitSightRange(*this);
	}
	Removed = 0;
	//Wyrmgus start
//	UnitInXY(*this, pos);
	UnitInXY(*this, pos, z);
	//Wyrmgus end
	// Pathfinding info.
	MarkUnitFieldFlags(*this);
	// Tha cache list.
	Map.Insert(*this);
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
	if (this->Type->BoolFlag[BUILDING_INDEX].value && (!this->Type->BoolFlag[TOWNHALL_INDEX].value || (!this->Constructed && this->SettlementName.empty()))) {
		this->UpdateSettlementName(); // update the settlement name of a building when placing it
	}
	
	VariationInfo *varinfo = this->Type->VarInfo[this->Variation];
	if (varinfo && varinfo->Terrains.size() > 0 && std::find(varinfo->Terrains.begin(), varinfo->Terrains.end(), Map.GetTileTopTerrain(this->tilePos, false, this->MapLayer)) == varinfo->Terrains.end()) { // if a unit that is on the tile has a terrain-dependent variation that is not compatible with the current variation, repick the unit's variation (this isn't perfect though, since the unit may still be allowed to keep the old variation if it has a tile size greater than 1x1)
		this->ChooseVariation();
	}
	//Wyrmgus end
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

	if (unit != NULL) {
		//Wyrmgus start
//		unit->Place(pos);
		unit->Place(pos, z);
		//Wyrmgus end
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
CUnit *CreateUnit(const Vec2i &pos, const CUnitType &type, CPlayer *player, int z)
{
	CUnit *unit = MakeUnit(type, player);

	if (unit != NULL) {
		Vec2i res_pos;
		const int heading = SyncRand() % 256;
		FindNearestDrop(type, pos, res_pos, heading, z);
		
		if (type.BoolFlag[BUILDING_INDEX].value) {
			CBuildRestrictionOnTop *b = OnTopDetails(type, NULL);
			if (b && b->ReplaceOnBuild) {
				CUnitCache &unitCache = Map.Field(res_pos, z)->UnitCache;
				CUnitCache::iterator it = std::find_if(unitCache.begin(), unitCache.end(), HasSameTypeAs(*b->Parent));

				if (it != unitCache.end()) {
					CUnit &replacedUnit = **it;
					unit->SetResourcesHeld(replacedUnit.ResourcesHeld);
					unit->Variable[GIVERESOURCE_INDEX].Value = replacedUnit.Variable[GIVERESOURCE_INDEX].Value;
					unit->Variable[GIVERESOURCE_INDEX].Max = replacedUnit.Variable[GIVERESOURCE_INDEX].Max;
					unit->Variable[GIVERESOURCE_INDEX].Enable = replacedUnit.Variable[GIVERESOURCE_INDEX].Enable;
					if (replacedUnit.Unique != NULL) {
						unit->SetUnique(replacedUnit.Unique);
					} else {
						if (replacedUnit.Prefix != NULL) {
							unit->SetPrefix(replacedUnit.Prefix);
						}
						if (replacedUnit.Suffix != NULL) {
							unit->SetSuffix(replacedUnit.Suffix);
						}
						if (replacedUnit.Spell != NULL) {
							unit->SetSpell(replacedUnit.Spell);
						}
					}
					replacedUnit.Remove(NULL);
					UnitLost(replacedUnit);
					UnitClearOrders(replacedUnit);
					replacedUnit.Release();
				}
			}
		}
		
		unit->Place(res_pos, z);
		UpdateForNewUnit(*unit, 0);
	}
	return unit;
}

CUnit *CreateResourceUnit(const Vec2i &pos, const CUnitType &type, int z)
{
	CUnit *unit = CreateUnit(pos, type, &Players[PlayerNumNeutral], z);
	unit->GenerateSpecialProperties();
			
	// create metal rocks near metal resources
	CUnitType *metal_rock_type = NULL;
	if (type.Ident == "unit-gold-deposit") {
		metal_rock_type = UnitTypeByIdent("unit-gold-rock");
	} else if (type.Ident == "unit-silver-deposit") {
		metal_rock_type = UnitTypeByIdent("unit-silver-rock");
	} else if (type.Ident == "unit-copper-deposit") {
		metal_rock_type = UnitTypeByIdent("unit-copper-rock");
	}
	if (metal_rock_type) {
		Vec2i metal_rock_offset((type.TileWidth - 1) / 2, (type.TileHeight - 1) / 2);
		for (int i = 0; i < 9; ++i) {
			CUnit *metal_rock_unit = CreateUnit(unit->tilePos + metal_rock_offset, *metal_rock_type, &Players[PlayerNumNeutral], z);
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
void FindNearestDrop(const CUnitType &type, const Vec2i &goalPos, Vec2i &resPos, int heading, int z)
//Wyrmgus end
{
	int addx = 0;
	int addy = 0;
	Vec2i pos = goalPos;

	if (heading < LookingNE || heading > LookingNW) {
		goto starts;
	} else if (heading < LookingSE) {
		goto startw;
	} else if (heading < LookingSW) {
		goto startn;
	} else {
		goto starte;
	}

	// FIXME: don't search outside of the map
	for (;;) {
startw:
		for (int i = addy; i--; ++pos.y) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, NULL)))
				&& (!type.BoolFlag[BUILDING_INDEX].value || CanBuildHere(NULL, type, pos, z) != NULL)
			) {
			//Wyrmgus end
				goto found;
			}
		}
		++addx;
starts:
		for (int i = addx; i--; ++pos.x) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, NULL)))
				&& (!type.BoolFlag[BUILDING_INDEX].value || CanBuildHere(NULL, type, pos, z) != NULL)
			) {
			//Wyrmgus end
				goto found;
			}
		}
		++addy;
starte:
		for (int i = addy; i--; --pos.y) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, NULL)))
				&& (!type.BoolFlag[BUILDING_INDEX].value || CanBuildHere(NULL, type, pos, z) != NULL)
			) {
			//Wyrmgus end
				goto found;
			}
		}
		++addx;
startn:
		for (int i = addx; i--; --pos.x) {
			//Wyrmgus start
//			if (UnitTypeCanBeAt(type, pos)) {
			if (
				(UnitTypeCanBeAt(type, pos, z) || (type.BoolFlag[BUILDING_INDEX].value && OnTopDetails(type, NULL)))
				&& (!type.BoolFlag[BUILDING_INDEX].value || CanBuildHere(NULL, type, pos, z) != NULL)
			) {
			//Wyrmgus end
				goto found;
			}
		}
		++addy;
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
	if (Removed) { // could happen!
		// If unit is removed (inside) and building is destroyed.
		DebugPrint("unit '%s(%d)' already removed\n" _C_ Type->Ident.c_str() _C_ UnitNumber(*this));
		return;
	}
	Map.Remove(*this);
	MapUnmarkUnitSight(*this);
	UnmarkUnitFieldFlags(*this);
	if (host) {
		AddInContainer(*host);
		UpdateUnitSightRange(*this);
		//Wyrmgus start
//		UnitInXY(*this, host->tilePos);
		UnitInXY(*this, host->tilePos, host->MapLayer);
		//Wyrmgus end
		MapMarkUnitSight(*this);
	}

	Removed = 1;

	// Correct surrounding walls directions
	if (this->Type->BoolFlag[WALL_INDEX].value) {
		CorrectWallNeighBours(*this);
	}

	//  Remove unit from the current selection
	if (this->Selected) {
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
		UnitUnderCursor = NULL;
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
	CPlayer &player = *unit.Player;

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

	const CUnitType &type = *unit.Type;
	if (!type.BoolFlag[VANISHES_INDEX].value) {
		player.RemoveUnit(unit);

		if (type.Building) {
			// FIXME: support more races
			//Wyrmgus start
//			if (!type.BoolFlag[WALL_INDEX].value && &type != UnitTypeOrcWall && &type != UnitTypeHumanWall) {
			if (!type.BoolFlag[WALL_INDEX].value) {
			//Wyrmgus end
				player.NumBuildings--;
				//Wyrmgus start
				if (unit.CurrentAction() == UnitActionBuilt) {
					player.NumBuildingsUnderConstruction--;
				}
				//Wyrmgus end
			}
		}
		if (unit.CurrentAction() != UnitActionBuilt) {
			player.UnitTypesCount[type.Slot]--;
			if (unit.Active) {
				player.UnitTypesAiActiveCount[type.Slot]--;
			}
			//Wyrmgus start
			if (type.BoolFlag[TOWNHALL_INDEX].value) {
				player.NumTownHalls--;
			}
			if (unit.Character == NULL) {
				player.UnitTypesNonHeroCount[type.Slot]--;
				if (unit.Starting) {
					player.UnitTypesStartingNonHeroCount[type.Slot]--;
				}
			} else {
				player.Heroes.erase(std::remove(player.Heroes.begin(), player.Heroes.end(), unit.Character->GetFullName()), player.Heroes.end());
				if (&unit == ThisPlayer->CustomHeroUnit) {
					ThisPlayer->CustomHeroUnit = NULL;
				}
			}
			
			if (player.AiEnabled && player.Ai && std::find(player.Ai->Scouts.begin(), player.Ai->Scouts.end(), &unit) != player.Ai->Scouts.end()) {
				player.Ai->Scouts.erase(std::remove(player.Ai->Scouts.begin(), player.Ai->Scouts.end(), &unit), player.Ai->Scouts.end());
				
				int force = player.Ai->Force.GetForce(unit);
				if (force != -1 && player.Ai->Force[force].Scouting) { //if a force's scout has been lost, unmark it as "scouting" so that the force can see if it now has a viable target
					player.Ai->Force[force].Scouting = false;
				}
			}
			//Wyrmgus end
		}
	}

	//  Handle unit demand. (Currently only food supported.)
	player.Demand -= type.Stats[player.Index].Variables[DEMAND_INDEX].Value;

	//  Update information.
	if (unit.CurrentAction() != UnitActionBuilt) {
		player.Supply -= type.Stats[player.Index].Variables[SUPPLY_INDEX].Value;
		// Decrease resource limit
		for (int i = 0; i < MaxCosts; ++i) {
			if (player.MaxResources[i] != -1 && type.Stats[player.Index].Storing[i]) {
				const int newMaxValue = player.MaxResources[i] - type.Stats[player.Index].Storing[i];

				player.MaxResources[i] = std::max(0, newMaxValue);
				player.SetResource(i, player.StoredResources[i], STORE_BUILDING);
			}
		}
		//  Handle income improvements, look if a player loses a building
		//  which have given him a better income, find the next best
		//  income.
		for (int i = 1; i < MaxCosts; ++i) {
			if (player.Incomes[i] && type.Stats[player.Index].ImproveIncomes[i] == player.Incomes[i]) {
				int m = DefaultIncomes[i];

				for (int j = 0; j < player.GetUnitCount(); ++j) {
					m = std::max(m, player.GetUnit(j).Type->Stats[player.Index].ImproveIncomes[i]);
				}
				player.Incomes[i] = m;
			}
		}
		
		//Wyrmgus start
		if (type.BoolFlag[TOWNHALL_INDEX].value) {
			bool lost_town_hall = true;
			for (int j = 0; j < player.GetUnitCount(); ++j) {
				if (player.GetUnit(j).Type->BoolFlag[TOWNHALL_INDEX].value) {
					lost_town_hall = false;
				}
			}
			if (lost_town_hall && ThisPlayer->HasContactWith(player)) {
				player.LostTownHallTimer = GameCycle + (30 * CYCLES_PER_SECOND); //30 seconds until being revealed
				for (int j = 0; j < NumPlayers; ++j) {
					if (player.Index != j && Players[j].Type != PlayerNobody) {
						Players[j].Notify(_("%s has lost their last town hall, and will be revealed in thirty seconds!"), player.Name.c_str());
					} else {
						Players[j].Notify("%s", _("You have lost your last town hall, and will be revealed in thirty seconds!"));
					}
				}
			}
		}
		//Wyrmgus end
	}

	//  Handle order cancels.
	unit.CurrentOrder()->Cancel(unit);

	DebugPrint("%d: Lost %s(%d)\n" _C_ player.Index _C_ type.Ident.c_str() _C_ UnitNumber(unit));

	// Destroy resource-platform, must re-make resource patch.
	//Wyrmgus start
//	CBuildRestrictionOnTop *b = OnTopDetails(unit, NULL);
	CBuildRestrictionOnTop *b = OnTopDetails(*unit.Type, NULL);
	//Wyrmgus end
	if (b != NULL) {
		if (b->ReplaceOnDie && (type.GivesResource && unit.ResourcesHeld != 0)) {
			//Wyrmgus start
//			CUnit *temp = MakeUnitAndPlace(unit.tilePos, *b->Parent, &Players[PlayerNumNeutral]);
			CUnit *temp = MakeUnitAndPlace(unit.tilePos, *b->Parent, &Players[PlayerNumNeutral], unit.MapLayer);
			//Wyrmgus end
			if (temp == NULL) {
				DebugPrint("Unable to allocate Unit");
			} else {
				//Wyrmgus start
//				temp->ResourcesHeld = unit.ResourcesHeld;
				temp->SetResourcesHeld(unit.ResourcesHeld);
				//Wyrmgus end
				temp->Variable[GIVERESOURCE_INDEX].Value = unit.Variable[GIVERESOURCE_INDEX].Value;
				temp->Variable[GIVERESOURCE_INDEX].Max = unit.Variable[GIVERESOURCE_INDEX].Max;
				temp->Variable[GIVERESOURCE_INDEX].Enable = unit.Variable[GIVERESOURCE_INDEX].Enable;
				//Wyrmgus start
				if (unit.Unique != NULL) {
					temp->SetUnique(unit.Unique);
				} else {
					if (unit.Prefix != NULL) {
						temp->SetPrefix(unit.Prefix);
					}
					if (unit.Suffix != NULL) {
						temp->SetSuffix(unit.Suffix);
					}
					if (unit.Spell != NULL) {
						temp->SetSpell(unit.Spell);
					}
				}
				//Wyrmgus end
			}
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
	const CUnitType &type = *unit.Type;
	CPlayer &player = *unit.Player;

	// Handle unit supply and max resources.
	// Note an upgraded unit can't give more supply.
	if (!upgrade) {
		player.Supply += type.Stats[player.Index].Variables[SUPPLY_INDEX].Value;
		for (int i = 0; i < MaxCosts; ++i) {
			if (player.MaxResources[i] != -1 && type.Stats[player.Index].Storing[i]) {
				player.MaxResources[i] += type.Stats[player.Index].Storing[i];
			}
		}
	}

	// Update resources
	for (int u = 1; u < MaxCosts; ++u) {
		player.Incomes[u] = std::max(player.Incomes[u], type.Stats[player.Index].ImproveIncomes[u]);
	}
	
	//Wyrmgus start
	if (player.LostTownHallTimer != 0 && type.BoolFlag[TOWNHALL_INDEX].value && ThisPlayer->HasContactWith(player)) {
		player.LostTownHallTimer = 0;
		player.Revealed = false;
		for (int j = 0; j < NumPlayers; ++j) {
			if (player.Index != j && Players[j].Type != PlayerNobody) {
				Players[j].Notify(_("%s has rebuilt a town hall, and will no longer be revealed!"), player.Name.c_str());
			} else {
				Players[j].Notify("%s", _("You have rebuilt a town hall, and will no longer be revealed!"));
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
	const int x = unit.tilePos.x;
	const int y = unit.tilePos.y;

	*dpos = pos;
	clamp<short int>(&dpos->x, x, x + unit.Type->TileWidth - 1);
	clamp<short int>(&dpos->y, y, y + unit.Type->TileHeight - 1);
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
	unit.Seen.tilePos = unit.tilePos;
	unit.Seen.IY = unit.IY;
	unit.Seen.IX = unit.IX;
	unit.Seen.Frame = unit.Frame;
	unit.Seen.Type = unit.Type;
	unit.Seen.Constructed = unit.Constructed;

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
	Assert(unit.Type->BoolFlag[WALL_INDEX].value);
	Assert(unit.Type->NumDirections == 16);
	Assert(!unit.Type->Flip);

	//Wyrmgus start
//	if (!Map.Info.IsPointOnMap(unit.tilePos)) {
	if (!Map.Info.IsPointOnMap(unit.tilePos, unit.MapLayer)) {
	//Wyrmgus end
		return ;
	}
	const struct {
		Vec2i offset;
		const int dirFlag;
	} configs[] = {{Vec2i(0, -1), W_NORTH}, {Vec2i(1, 0), W_EAST},
		{Vec2i(0, 1), W_SOUTH}, {Vec2i(-1, 0), W_WEST}
	};
	int flags = 0;

	for (int i = 0; i != sizeof(configs) / sizeof(*configs); ++i) {
		const Vec2i pos = unit.tilePos + configs[i].offset;
		const int dirFlag = configs[i].dirFlag;

		//Wyrmgus start
//		if (Map.Info.IsPointOnMap(pos) == false) {
		if (Map.Info.IsPointOnMap(pos, unit.MapLayer) == false) {
		//Wyrmgus end
			flags |= dirFlag;
		} else {
			//Wyrmgus start
//			const CUnitCache &unitCache = Map.Field(pos)->UnitCache;
			const CUnitCache &unitCache = Map.Field(pos, unit.MapLayer)->UnitCache;
			//Wyrmgus end
			const CUnit *neighboor = unitCache.find(HasSamePlayerAndTypeAs(unit));

			if (neighboor != NULL) {
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
	Assert(unit.Type->BoolFlag[WALL_INDEX].value);

	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1)};

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		const Vec2i pos = unit.tilePos + offset[i];

		//Wyrmgus start
//		if (Map.Info.IsPointOnMap(pos) == false) {
		if (Map.Info.IsPointOnMap(pos, unit.MapLayer) == false) {
		//Wyrmgus end
			continue;
		}
		//Wyrmgus start
//		CUnitCache &unitCache = Map.Field(pos)->UnitCache;
		CUnitCache &unitCache = Map.Field(pos, unit.MapLayer)->UnitCache;
		//Wyrmgus end
		CUnit *neighboor = unitCache.find(HasSamePlayerAndTypeAs(unit));

		if (neighboor != NULL) {
			CorrectWallDirections(*neighboor);
			UnitUpdateHeading(*neighboor);
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
	if (unit.Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
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
			unit.Seen.Destroyed |= (1 << player.Index);
		}
		if (&player == ThisPlayer) {
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
	if (!unit.Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value) {
		return;
	}
	if (unit.Seen.ByPlayer & (1 << (player.Index))) {
		if ((player.Type == PlayerPerson) && (!(unit.Seen.Destroyed & (1 << player.Index)))) {
			unit.RefsDecrease();
		}
	} else {
		unit.Seen.ByPlayer |= (1 << (player.Index));
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
	Assert(unit.Type);

	// FIXME: optimize, only work on certain players?
	// This is for instance good for updating shared vision...

	//  Store old values in oldv[p]. This store if the player could see the
	//  unit before this calc.
	int oldv[PlayerMax];
	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].Type != PlayerNobody) {
			oldv[p] = unit.IsVisible(Players[p]);
		}
	}

	//  Calculate new VisCount values.
	const int height = unit.Type->TileHeight;
	const int width = unit.Type->TileWidth;

	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].Type != PlayerNobody) {
			int newv = 0;
			int y = height;
			unsigned int index = unit.Offset;
			do {
				//Wyrmgus start
//				CMapField *mf = Map.Field(index);
				CMapField *mf = Map.Field(index, unit.MapLayer);
				//Wyrmgus end
				int x = width;
				do {
					if (unit.Type->BoolFlag[PERMANENTCLOAK_INDEX].value && unit.Player != &Players[p]) {
						if (mf->playerInfo.VisCloak[p]) {
							newv++;
						}
					//Wyrmgus start
					} else if (unit.Type->BoolFlag[ETHEREAL_INDEX].value && unit.Player != &Players[p]) {
						if (mf->playerInfo.VisEthereal[p]) {
							newv++;
						}
					//Wyrmgus end
					} else {
						if (mf->playerInfo.IsVisible(Players[p])) {
							newv++;
						}
					}
					++mf;
				} while (--x);
				//Wyrmgus start
//				index += Map.Info.MapWidth;
				index += Map.Info.MapWidths[unit.MapLayer];
				//Wyrmgus end
			} while (--y);
			unit.VisCount[p] = newv;
		}
	}

	//
	// Now here comes the tricky part. We have to go in and out of fog
	// for players. Hopefully this works with shared vision just great.
	//
	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].Type != PlayerNobody) {
			int newv = unit.IsVisible(Players[p]);
			if (!oldv[p] && newv) {
				// Might have revealed a destroyed unit which caused it to
				// be released
				if (!unit.Type) {
					break;
				}
				UnitGoesOutOfFog(unit, Players[p]);
			}
			if (oldv[p] && !newv) {
				UnitGoesUnderFog(unit, Players[p]);
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
	if (VisCount[player.Index]) {
		return true;
	}
	for (int p = 0; p < PlayerMax; ++p) {
		//Wyrmgus start
//		if (p != player.Index && player.IsBothSharedVision(Players[p])) {
		if (p != player.Index && (player.IsBothSharedVision(Players[p]) || Players[p].Revealed)) {
		//Wyrmgus end
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
	//Wyrmgus start
	if (this->MapLayer != CurrentMapLayer) {
		return false;
	}
	//Wyrmgus end

	// Invisible units.
	if (IsInvisibile(*ThisPlayer)) {
		return false;
	}
	if (IsVisible(*ThisPlayer) || ReplayRevealMap || IsVisibleOnRadar(*ThisPlayer)) {
		return IsAliveOnMap();
	} else {
		return Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value && Seen.State != 3
			   && (Seen.ByPlayer & (1 << ThisPlayer->Index))
			   //Wyrmgus start
//			   && !(Seen.Destroyed & (1 << ThisPlayer->Index));
			   && !(Seen.Destroyed & (1 << ThisPlayer->Index))
			   && !Destroyed
			   && Map.Info.IsPointOnMap(this->tilePos, this->MapLayer)
			   && Map.Field(this->tilePos, this->MapLayer)->playerInfo.IsTeamExplored(*ThisPlayer);
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
//	int x = tilePos.x * PixelTileSize.x + IX - (Type->Width - Type->TileWidth * PixelTileSize.x) / 2 + Type->OffsetX;
//	int y = tilePos.y * PixelTileSize.y + IY - (Type->Height - Type->TileHeight * PixelTileSize.y) / 2 + Type->OffsetY;

	int frame_width = Type->Width;
	int frame_height = Type->Height;
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (varinfo && varinfo->FrameWidth && varinfo->FrameHeight) {
		frame_width = varinfo->FrameWidth;
		frame_height = varinfo->FrameHeight;
	}

	int x = tilePos.x * PixelTileSize.x + IX - (frame_width - Type->TileWidth * PixelTileSize.x) / 2 + Type->OffsetX;
	int y = tilePos.y * PixelTileSize.y + IY - (frame_height - Type->TileHeight * PixelTileSize.y) / 2 + Type->OffsetY;
	//Wyrmgus end
	const PixelSize vpSize = vp.GetPixelSize();
	const PixelPos vpTopLeftMapPos = Map.TilePosToMapPixelPos_TopLeft(vp.MapPos) + vp.Offset;
	const PixelPos vpBottomRightMapPos = vpTopLeftMapPos + vpSize;

	//Wyrmgus start
//	if (x + Type->Width < vpTopLeftMapPos.x || x > vpBottomRightMapPos.x
//		|| y + Type->Height < vpTopLeftMapPos.y || y > vpBottomRightMapPos.y) {
	if (x + frame_width < vpTopLeftMapPos.x || x > vpBottomRightMapPos.x
		|| y + frame_height < vpTopLeftMapPos.y || y > vpBottomRightMapPos.y) {
	//Wyrmgus end
		return false;
	}

	if (!ThisPlayer) {
		//FIXME: ARI: Added here for early game setup state by
		// MakeAndPlaceUnit() from LoadMap(). ThisPlayer not yet set,
		// so don't show anything until first real map-draw.
		DebugPrint("FIXME: ThisPlayer not set yet?!\n");
		return false;
	}

	// Those are never ever visible.
	if (IsInvisibile(*ThisPlayer)) {
		return false;
	}

	if (IsVisible(*ThisPlayer) || ReplayRevealMap) {
		return !Destroyed;
	} else {
		// Unit has to be 'discovered'
		// Destroyed units ARE visible under fog of war, if we haven't seen them like that.
		if (!Destroyed || !(Seen.Destroyed & (1 << ThisPlayer->Index))) {
			return (Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value && (Seen.ByPlayer & (1 << ThisPlayer->Index)));
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

	if (Type->Building) {
		if (!Type->BoolFlag[WALL_INDEX].value) {
			newplayer.TotalBuildings++;
		}
	} else {
		newplayer.TotalUnits++;
	}

	MapUnmarkUnitSight(*this);
	newplayer.AddUnit(*this);
	Stats = &Type->Stats[newplayer.Index];
	UpdateUnitSightRange(*this);
	MapMarkUnitSight(*this);

	//  Must change food/gold and other.
	if (Type->GivesResource) {
		DebugPrint("Resource transfer not supported\n");
	}
	newplayer.Demand += Type->Stats[newplayer.Index].Variables[DEMAND_INDEX].Value;
	newplayer.Supply += Type->Stats[newplayer.Index].Variables[SUPPLY_INDEX].Value;
	// Increase resource limit
	for (int i = 0; i < MaxCosts; ++i) {
		if (newplayer.MaxResources[i] != -1 && Type->Stats[newplayer.Index].Storing[i]) {
			newplayer.MaxResources[i] += Type->Stats[newplayer.Index].Storing[i];
		}
	}
	if (Type->Building && !Type->BoolFlag[WALL_INDEX].value) {
		newplayer.NumBuildings++;
	}
	//Wyrmgus start
	if (CurrentAction() == UnitActionBuilt) {
		newplayer.NumBuildingsUnderConstruction++;
	}
	//Wyrmgus end
	newplayer.UnitTypesCount[Type->Slot]++;
	if (Active) {
		newplayer.UnitTypesAiActiveCount[Type->Slot]++;
	}
	//Wyrmgus start
	if (Type->BoolFlag[TOWNHALL_INDEX].value) {
		newplayer.NumTownHalls++;
	}
	if (this->Character == NULL) {
		newplayer.UnitTypesNonHeroCount[Type->Slot]++;
		if (this->Starting) {
			newplayer.UnitTypesStartingNonHeroCount[Type->Slot]++;
		}
	} else {
		newplayer.Heroes.push_back(this->Character->GetFullName());
	}
	
	if (newplayer.AiEnabled && newplayer.Ai && this->Type->BoolFlag[COWARD_INDEX].value && !this->Type->BoolFlag[HARVESTER_INDEX].value && !this->Type->CanTransport() && !this->Type->CanCastSpell && Map.Info.IsPointOnMap(this->tilePos, this->MapLayer) && this->CanMove() && this->Active && this->GroupId != 0 && this->Variable[SIGHTRANGE_INDEX].Value > 0) { //assign coward, non-worker, non-transporter, non-spellcaster units to be scouts
		newplayer.Ai->Scouts.push_back(this);
	}
	//Wyrmgus end

	//apply upgrades of the new player, if the old one doesn't have that upgrade
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (oldplayer->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] != 'R' && newplayer.Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X') { //if the old player doesn't have the modifier's upgrade (but the new one does), and the upgrade is applicable to the unit
			//Wyrmgus start
//			ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			if ( // don't apply equipment-related upgrades if the unit has an item of that equipment type equipped
				(!AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Weapon || EquippedItems[WeaponItemSlot].size() == 0)
				&& (!AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Shield || EquippedItems[ShieldItemSlot].size() == 0)
				&& (!AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Boots || EquippedItems[BootsItemSlot].size() == 0)
				&& (!AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Arrows || EquippedItems[ArrowsItemSlot].size() == 0)
				&& !(newplayer.Race != -1 && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ident == PlayerRaces.CivilizationUpgrades[newplayer.Race])
				&& !(newplayer.Race != -1 && newplayer.Faction != -1 && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ident == PlayerRaces.Factions[newplayer.Race][newplayer.Faction]->FactionUpgrade)
			) {
				ApplyIndividualUpgradeModifier(*this, UpgradeModifiers[z]);
			}
			//Wyrmgus end
		}
	}

	UpdateForNewUnit(*this, 1);
	
	//Wyrmgus start
	if (newplayer.Index == ThisPlayer->Index && show_change) {
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
	Assert(this->NextWorker == NULL);

	CUnit *head = mine.Resource.Workers;
#if 0
	DebugPrint("%d: Worker [%d] is adding into %s [%d] on %d pos\n"
			   _C_ this->Player->Index _C_ this->Slot
			   _C_ mine.Type->Name.c_str()
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
	CUnit *prev = NULL, *worker = mine.Resource.Workers;
#if 0
	DebugPrint("%d: Worker [%d] is removing from %s [%d] left %d units assigned\n"
			   _C_ this->Player->Index _C_ this->Slot
			   _C_ mine.Type->Name.c_str()
			   _C_ mine.Slot
			   _C_ mine.CurrentOrder()->Data.Resource.Assigned);
#endif
	for (int i = 0; NULL != worker; worker = worker->NextWorker, ++i) {
		if (worker == this) {
			CUnit *next = worker->NextWorker;
			worker->NextWorker = NULL;
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
	for (CPlayer *p = Players; p < Players + NumPlayers; ++p) {
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
//					if (around[i]->Type->CanAttack && unit.IsAllied(*around[i]) && around[i]->Player->Type != PlayerRescuePassive && around[i]->Player->Type != PlayerRescueActive) {
					if (around[i]->CanAttack() && unit.IsAllied(*around[i]) && around[i]->Player->Type != PlayerRescuePassive && around[i]->Player->Type != PlayerRescueActive) {
					//Wyrmgus end
						//  City center converts complete race
						//  NOTE: I use a trick here, centers could
						//        store gold. FIXME!!!
						//Wyrmgus start
//						if (unit.Type->CanStore[GoldCost]) {
						if (unit.Type->CanStore[CopperCost]) {
						//Wyrmgus end
							ChangePlayerOwner(*p, *around[i]->Player);
							break;
						}
						unit.RescuedFrom = unit.Player;
						//Wyrmgus start
//						unit.ChangeOwner(*around[i]->Player);
						unit.ChangeOwner(*around[i]->Player, true);
//						unit.Blink = 5;
//						PlayGameSound(GameSounds.Rescue[unit.Player->Race].Sound, MaxSampleVolume);
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
	//Wyrmgus start
	//fix direction if it does not correspond to one of the defined directions
	int num_dir = std::max<int>(8, unit.Type->NumDirections);
	if (unit.Direction % (256 / num_dir) != 0) {
		unit.Direction = unit.Direction - (unit.Direction % (256 / num_dir));
	}
	//Wyrmgus end
	
	int dir;
	int nextdir;
	bool neg;

	if (unit.Frame < 0) {
		unit.Frame = -unit.Frame - 1;
		neg = true;
	} else {
		neg = false;
	}
	unit.Frame /= unit.Type->NumDirections / 2 + 1;
	unit.Frame *= unit.Type->NumDirections / 2 + 1;
	// Remove heading, keep animation frame

	nextdir = 256 / unit.Type->NumDirections;
	dir = ((unit.Direction + nextdir / 2) & 0xFF) / nextdir;
	if (dir <= LookingS / nextdir) { // north->east->south
		unit.Frame += dir;
	} else {
		unit.Frame += 256 / nextdir - dir;
		unit.Frame = -unit.Frame - 1;
	}
	if (neg && !unit.Frame && unit.Type->Building) {
		unit.Frame = -1;
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
	int num_dir = std::max<int>(8, unit.Type->NumDirections);
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
		pos = container->tilePos;
		pos.x -= unit.Type->TileWidth - 1;
		pos.y -= unit.Type->TileHeight - 1;
		addx = container->Type->TileWidth + unit.Type->TileWidth - 1;
		addy = container->Type->TileHeight + unit.Type->TileHeight - 1;
		//Wyrmgus start
		z = container->MapLayer;
		//Wyrmgus end

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
		pos = unit.tilePos;
		//Wyrmgus start
		z = unit.MapLayer;
		//Wyrmgus end

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
		pos = container->tilePos;
		pos.x -= unit.Type->TileWidth - 1;
		pos.y -= unit.Type->TileHeight - 1;
		addx = container->Type->TileWidth + unit.Type->TileWidth - 1;
		addy = container->Type->TileHeight + unit.Type->TileHeight - 1;
		--pos.x;
		//Wyrmgus start
		z = container->MapLayer;
		//Wyrmgus end
	} else {
		pos = unit.tilePos;
		//Wyrmgus start
		z = unit.MapLayer;
		//Wyrmgus end
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
	if (unit->Type->BoolFlag[ITEM_INDEX].value && !unit->Unique) { //save the initial cycle items were placed in the ground to destroy them if they have been there for too long
		int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
		if (unit->Prefix != NULL || unit->Suffix != NULL || unit->Spell != NULL || unit->Work != NULL || unit->Elixir != NULL) {
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
	CUnit *candidate = NULL;
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		//Wyrmgus start
		if (unit.MapLayer != CurrentMapLayer) {
			continue;
		}
		//Wyrmgus end
		if (!ReplayRevealMap && !unit.IsVisibleAsGoal(*ThisPlayer)) {
			continue;
		}
		const CUnitType &type = *unit.Type;
		if (!type.Sprite) {
			continue;
		}

		//
		// Check if mouse is over the unit.
		//
		PixelPos unitSpritePos = unit.GetMapPixelPosCenter();
		//Wyrmgus start
//		unitSpritePos.x = unitSpritePos.x - type.BoxWidth / 2 -
//						  (type.Width - type.Sprite->Width) / 2 + type.BoxOffsetX;
//		unitSpritePos.y = unitSpritePos.y - type.BoxHeight / 2 -
//						  (type.Height - type.Sprite->Height) / 2 + type.BoxOffsetY;
		VariationInfo *varinfo = type.VarInfo[unit.Variation];
		if (varinfo && varinfo->FrameWidth && varinfo->FrameHeight && !varinfo->File.empty()) {
			unitSpritePos.x = unitSpritePos.x - type.BoxWidth / 2 -
							  (varinfo->FrameWidth - varinfo->Sprite->Width) / 2 + type.BoxOffsetX;
			unitSpritePos.y = unitSpritePos.y - type.BoxHeight / 2 -
							  (varinfo->FrameHeight - varinfo->Sprite->Height) / 2 + type.BoxOffsetY;
		} else {
			unitSpritePos.x = unitSpritePos.x - type.BoxWidth / 2 -
							  (type.Width - type.Sprite->Width) / 2 + type.BoxOffsetX;
			unitSpritePos.y = unitSpritePos.y - type.BoxHeight / 2 -
							  (type.Height - type.Sprite->Height) / 2 + type.BoxOffsetY;
		}
		//Wyrmgus end
		if (x >= unitSpritePos.x && x < unitSpritePos.x + type.BoxWidth
			&& y >= unitSpritePos.y  && y < unitSpritePos.y + type.BoxHeight) {
			// Check if there are other units on this place
			candidate = &unit;
			//Wyrmgus start
			std::vector<CUnit *> table;
			Select(candidate->tilePos, candidate->tilePos, table, candidate->MapLayer, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
//			if (IsOnlySelected(*candidate) || candidate->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value) {
			if (IsOnlySelected(*candidate) || candidate->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value || (candidate->Player->Type == PlayerNeutral && table.size()) || !candidate->IsAlive()) { // don't select a neutral unit if there's a player-owned unit there as well; don't selected a dead unit
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
	const PixelPos pos(tilePos.x * PixelTileSize.x + IX, tilePos.y * PixelTileSize.y + IY);
	return pos;
}

PixelPos CUnit::GetMapPixelPosCenter() const
{
	return GetMapPixelPosTopLeft() + Type->GetPixelSize() / 2;
}

//Wyrmgus start
int CUnit::GetAvailableLevelUpUpgrades(bool only_units) const
{
	int value = 0;
	int upgrade_value = 0;
	
	if (((int) AiHelpers.ExperienceUpgrades.size()) > Type->Slot) {
		for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[Type->Slot].size(); ++i) {
			if (Character == NULL || !Character->ForbiddenUpgrades[AiHelpers.ExperienceUpgrades[Type->Slot][i]->Slot]) {
				int local_upgrade_value = 1;
				
				if (!only_units) {
					local_upgrade_value += AiHelpers.ExperienceUpgrades[Type->Slot][i]->GetAvailableLevelUpUpgrades();
				}
				
				if (local_upgrade_value > upgrade_value) {
					upgrade_value = local_upgrade_value;
				}
			}
		}
	}
	
	value += upgrade_value;
	
	if (!only_units && ((int) AiHelpers.LearnableAbilities.size()) > Type->Slot) {
		for (size_t i = 0; i != AiHelpers.LearnableAbilities[Type->Slot].size(); ++i) {
			if (!IndividualUpgrades[AiHelpers.LearnableAbilities[Type->Slot][i]->ID]) {
				value += 1;
			}
		}
	}
	
	return value;
}

int CUnit::GetModifiedVariable(int index, int variable_type) const
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
		if (this->Container) {
			value += this->Container->Stats->Variables[index].Value; //treat the container's attack range as a bonus to the unit's attack range
		}
		std::min<int>(this->CurrentSightRange, value); // if the unit's current sight range is smaller than its attack range, use it instead
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

int CUnit::GetItemSlotQuantity(int item_slot) const
{
	if (!HasInventory()) {
		return 0;
	}
	
	if ( //if the item are arrows and the weapon of this unit's type is not a bow, return false
		item_slot == ArrowsItemSlot
		&& Type->WeaponClasses[0] != BowItemClass
	) {
		return 0;
	}
	
	if (item_slot == RingItemSlot) {
		return 2;
	}
	
	return 1;
}

int CUnit::GetCurrentWeaponClass() const
{
	if (HasInventory() && EquippedItems[WeaponItemSlot].size() > 0) {
		return EquippedItems[WeaponItemSlot][0]->Type->ItemClass;
	}
	
	return Type->WeaponClasses[0];
}

int CUnit::GetItemVariableChange(const CUnit *item, int variable_index, bool increase) const
{
	if (item->Type->ItemClass == -1) {
		return 0;
	}
	
	int item_slot = GetItemClassSlot(item->Type->ItemClass);
	if (item->Work == NULL && item->Elixir == NULL && (item_slot == -1 || this->GetItemSlotQuantity(item_slot) == 0 || !this->CanEquipItemClass(item->Type->ItemClass))) {
		return 0;
	}
	
	int value = 0;
	if (item->Work != NULL) {
		if (this->IndividualUpgrades[item->Work->ID] == false) {
			for (size_t z = 0; z < item->Work->UpgradeModifiers.size(); ++z) {
				if (!increase) {
					value += item->Work->UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
				} else {
					value += item->Work->UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
				}
			}
		}
	} else if (item->Elixir != NULL) {
		if (this->IndividualUpgrades[item->Elixir->ID] == false) {
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
			for (int z = 0; z < NumUpgradeModifiers; ++z) {
				if (
					(item->Prefix != NULL && UpgradeModifiers[z]->UpgradeId == item->Prefix->ID)
					|| (item->Suffix != NULL && UpgradeModifiers[z]->UpgradeId == item->Suffix->ID)
				) {
					if (!increase) {
						value -= UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
					} else {
						value -= UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
					}
				}
			}
		}
		
		if (EquippedItems[item_slot].size() == this->GetItemSlotQuantity(item_slot)) {
			int item_slot_used = EquippedItems[item_slot].size() - 1;
			for (size_t i = 0; i < EquippedItems[item_slot].size(); ++i) {
				if (EquippedItems[item_slot][i] == item) {
					item_slot_used = i;
				}
			}
			if (!increase) {
				value -= EquippedItems[item_slot][item_slot_used]->Variable[variable_index].Value;
			} else {
				value -= EquippedItems[item_slot][item_slot_used]->Variable[variable_index].Increase;
			}
		} else if (EquippedItems[item_slot].size() == 0 && (item_slot == WeaponItemSlot || item_slot == ShieldItemSlot || item_slot == BootsItemSlot || item_slot == ArrowsItemSlot)) {
			for (int z = 0; z < NumUpgradeModifiers; ++z) {
				if (
					(
						(
							(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Weapon && item_slot == WeaponItemSlot)
							|| (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Shield && item_slot == ShieldItemSlot)
							|| (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Boots && item_slot == BootsItemSlot)
							|| (AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Arrows && item_slot == ArrowsItemSlot)
						)
						&& Player->Allow.Upgrades[UpgradeModifiers[z]->UpgradeId] == 'R' && UpgradeModifiers[z]->ApplyTo[Type->Slot] == 'X'
					)
					|| (item_slot == WeaponItemSlot && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ability && this->IndividualUpgrades[UpgradeModifiers[z]->UpgradeId] && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.size() > 0 && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), this->GetCurrentWeaponClass()) != AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end() && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), item->Type->ItemClass) == AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end())
				) {
					if (!increase) {
						value -= UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
					} else {
						value -= UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
					}
				} else if (
					AllUpgrades[UpgradeModifiers[z]->UpgradeId]->Ability && this->IndividualUpgrades[UpgradeModifiers[z]->UpgradeId] && AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.size() > 0 && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), this->GetCurrentWeaponClass()) == AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end() && std::find(AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.begin(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end(), item->Type->ItemClass) != AllUpgrades[UpgradeModifiers[z]->UpgradeId]->WeaponClasses.end()
				) {
					if (!increase) {
						value += UpgradeModifiers[z]->Modifier.Variables[variable_index].Value;
					} else {
						value += UpgradeModifiers[z]->Modifier.Variables[variable_index].Increase;
					}
				}
			}
		}
	}
	
	return value;
}

int CUnit::GetDisplayPlayer() const
{
	if (this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->Player != ThisPlayer) {
		return PlayerNumNeutral;
	} else {
		return this->RescuedFrom ? this->RescuedFrom->Index : this->Player->Index;
	}
}

int CUnit::GetSkinColor() const
{
	VariationInfo *varinfo = this->Type->VarInfo[this->Variation];
	if (varinfo && varinfo->SkinColor) {
		return varinfo->SkinColor;
	} else {
		return this->Type->SkinColor;
	}
}

int CUnit::GetHairColor() const
{
	VariationInfo *varinfo = this->Type->VarInfo[this->Variation];
	if (this->LayerVariation[HairImageLayer] != -1 && this->LayerVariation[HairImageLayer] < ((int) this->Type->LayerVarInfo[HairImageLayer].size()) && this->Type->LayerVarInfo[HairImageLayer][this->LayerVariation[HairImageLayer]]->HairColor != 0) {
		return this->Type->LayerVarInfo[HairImageLayer][this->LayerVariation[HairImageLayer]]->HairColor;
	} else if (varinfo && varinfo->HairColor != 0) {
		return varinfo->HairColor;
	} else {
		return this->Type->HairColor;
	}
}

int CUnit::GetPrice() const
{
	int cost = 0;

	for (int i = 1; i < MaxCosts; ++i) {
		if (this->Type->Stats[this->Player->Index].Costs[i] > 0) {
			if (i == CopperCost) {
				cost += this->Type->Stats[this->Player->Index].Costs[i];
			} else {
				cost += this->Type->Stats[this->Player->Index].Costs[i] * DefaultResourcePrices[i] / 100;
			}
		}
	}

	if (this->Prefix != NULL) {
		cost += this->Prefix->MagicLevel * 1000;
	}
	if (this->Suffix != NULL) {
		cost += this->Suffix->MagicLevel * 1000;
	}
	if (this->Spell != NULL) {
		cost += 1000;
	}
	if (this->Work != NULL) {
		if (this->Type->ItemClass == BookItemClass) {
			cost += 3000;
		} else {
			cost += 1000;
		}
	}
	if (this->Elixir != NULL) {
		cost += this->Elixir->MagicLevel * 1000;
	}
	if (this->Character) {
		cost += (this->Variable[LEVEL_INDEX].Value - this->Type->Stats[this->Player->Index].Variables[LEVEL_INDEX].Value) * 250;
	}
	
	return cost;
}

int CUnit::GetUnitStock(int unit_type_id) const
{
	if (this->UnitStock.find(unit_type_id) != this->UnitStock.end()) {
		return this->UnitStock.find(unit_type_id)->second;
	} else {
		return 0;
	}
}

int CUnit::GetUnitStockReplenishmentTimer(int unit_type_id) const
{
	if (this->UnitStockReplenishmentTimers.find(unit_type_id) != this->UnitStockReplenishmentTimers.end()) {
		return this->UnitStockReplenishmentTimers.find(unit_type_id)->second;
	} else {
		return 0;
	}
}

bool CUnit::CanAttack(bool count_inside) const
{
	if (this->Type->CanTransport() && this->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && this->Type->BoolFlag[CANATTACK_INDEX].value) { //transporters can only attack through a unit within them
		if (count_inside && this->BoardCount > 0) {
			CUnit *boarded_unit = this->UnitInside;
			for (int i = 0; i < this->InsideCount; ++i, boarded_unit = boarded_unit->NextContained) {
				if (boarded_unit->GetModifiedVariable(ATTACKRANGE_INDEX) > 1 && boarded_unit->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value) {
					return true;
				}
			}
		}
		return false;
	}
	
	return this->Type->BoolFlag[CANATTACK_INDEX].value;
}

bool CUnit::IsInCombat() const
{
	// Select all units around the unit
	std::vector<CUnit *> table;
	SelectAroundUnit(*this, 6, table, HasNotSamePlayerAs(*this->Player));

	// Check each unit if it is hostile.
	bool inCombat = false;
	for (size_t i = 0; i < table.size(); ++i) {
		const CUnit &target = *table[i];

		// Note that CanTarget doesn't take into account (offensive) spells...
		if (target.IsVisibleAsGoal(*this->Player) && this->IsEnemy(target)
			&& (CanTarget(*this->Type, *target.Type) || CanTarget(*target.Type, *this->Type))) {
			inCombat = true;
			break;
		}
	}
		
	return inCombat;
}

bool CUnit::IsItemEquipped(const CUnit *item) const
{
	int item_slot = GetItemClassSlot(item->Type->ItemClass);
	
	if (item_slot == -1) {
		return false;
	}
	
	if (std::find(EquippedItems[item_slot].begin(), EquippedItems[item_slot].end(), item) != EquippedItems[item_slot].end()) {
		return true;
	}
	
	return false;
}

bool CUnit::IsItemClassEquipped(int item_class) const
{
	int item_slot = GetItemClassSlot(item_class);
	
	if (item_slot == -1) {
		return false;
	}
	
	for (size_t i = 0; i < EquippedItems[item_slot].size(); ++i) {
		if (EquippedItems[item_slot][i]->Type->ItemClass == item_class) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::IsItemTypeEquipped(CUnitType *item_type) const
{
	int item_slot = GetItemClassSlot(item_type->ItemClass);
	
	if (item_slot == -1) {
		return false;
	}
	
	for (size_t i = 0; i < EquippedItems[item_slot].size(); ++i) {
		if (EquippedItems[item_slot][i]->Type == item_type) {
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
	
	if (!CanEquipItemClass(item->Type->ItemClass)) {
		return false;
	}
	
	return true;
}

bool CUnit::CanEquipItemClass(int item_class) const
{
	if (item_class == -1) {
		return false;
	}
	
	if (GetItemClassSlot(item_class) == -1) { //can't equip items that don't correspond to an equippable slot
		return false;
	}
	
	if (GetItemClassSlot(item_class) == WeaponItemSlot && std::find(Type->WeaponClasses.begin(), Type->WeaponClasses.end(), item_class) == Type->WeaponClasses.end()) { //if the item is a weapon and its item class isn't a weapon class used by this unit's type, return false
		return false;
	}
	
	if ( //if the item uses the shield (off-hand) slot, but that slot is unavailable for the weapon (because it is two-handed), return false
		GetItemClassSlot(item_class) == ShieldItemSlot
		&& Type->WeaponClasses.size() > 0
		&& (
			Type->WeaponClasses[0] == BowItemClass
			// add other two-handed weapons here as necessary
		)
	) {
		return false;
	}
	
	if ( //if the item is a shield and the weapon of this unit's type is incompatible with shields, return false
		item_class == ShieldItemClass
		&& (
			Type->WeaponClasses.size() == 0
			|| Type->WeaponClasses[0] == DaggerItemClass
			|| Type->WeaponClasses[0] == ThrowingAxeItemClass
			|| Type->WeaponClasses[0] == JavelinItemClass
			|| Type->BoolFlag[HARVESTER_INDEX].value //workers can't use shields
		)
	) {
		return false;
	}
	
	if (this->GetItemSlotQuantity(GetItemClassSlot(item_class)) == 0) {
		return false;
	}
	
	return true;
}

bool CUnit::CanUseItem(CUnit *item) const
{
	if (item->ConnectingDestination != NULL) {
		if (item->Type->BoolFlag[ETHEREAL_INDEX].value && !this->Variable[ETHEREALVISION_INDEX].Value) {
			return false;
		}
		
		if (this->Player == item->Player || this->Player->IsAllied(*item->Player) || item->Player->Type == PlayerNeutral) {
			return true;
		}
	}
	
	if (!item->Type->BoolFlag[ITEM_INDEX].value && !item->Type->BoolFlag[POWERUP_INDEX].value) {
		return false;
	}
	
	if (item->Type->BoolFlag[ITEM_INDEX].value && item->Type->ItemClass != FoodItemClass && item->Type->ItemClass != PotionItemClass && item->Type->ItemClass != ScrollItemClass && item->Type->ItemClass != BookItemClass) {
		return false;
	}
	
	if (item->Spell != NULL) {
		if (!this->HasInventory() || !CanCastSpell(*this, *item->Spell, this, this->tilePos, this->MapLayer)) {
			return false;
		}
	}
	
	if (item->Work != NULL) {
		if (!this->HasInventory() || this->IndividualUpgrades[item->Work->ID]) {
			return false;
		}
	}
	
	if (item->Elixir != NULL) {
		if (!this->HasInventory() || this->IndividualUpgrades[item->Elixir->ID]) {
			return false;
		}
	}
	
	if (item->Elixir == NULL && item->Variable[HITPOINTHEALING_INDEX].Value > 0 && this->Variable[HP_INDEX].Value >= this->GetModifiedVariable(HP_INDEX, VariableMax)) {
		return false;
	}
	
	return true;
}

bool CUnit::HasInventory() const
{
	if (Type->BoolFlag[INVENTORY_INDEX].value) {
		return true;
	}
	
	if (Character != NULL && Character->Persistent) {
		return true;
	}
	
	return false;
}

bool CUnit::CanLearnAbility(CUpgrade *ability) const
{
	if (IndividualUpgrades[ability->ID]) { // already learned
		return false;
	}
	
	if (!CheckDependByIdent(*Player, ability->Ident)) {
		return false;
	}
	
	if (UpgradeIdAllowed(*Player, ability->ID) != 'A') {
		return false;
	}
	
	for (size_t i = 0; i < ability->RequiredAbilities.size(); ++i) {
		if (!IndividualUpgrades[ability->RequiredAbilities[i]->ID]) {
			return false;
		}
	}

	return true;
}

bool CUnit::CanHireMercenary(CUnitType *type, int civilization_id) const
{
	if (civilization_id == -1) {
		civilization_id = type->Civilization;
	}
	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].Type != PlayerNobody && Players[p].Type != PlayerNeutral && civilization_id == Players[p].Race && CheckDependByType(Players[p], *type, true) && Players[p].StartMapLayer == this->MapLayer) {
			return true;
		}
	}
	
	return false;
}

bool CUnit::CanEat(const CUnit &unit) const
{
	if (this->Type->BoolFlag[CARNIVORE_INDEX].value && unit.Type->BoolFlag[FLESH_INDEX].value) {
		return true;
	}
	
	if (this->Type->BoolFlag[INSECTIVORE_INDEX].value && unit.Type->BoolFlag[INSECT_INDEX].value) {
		return true;
	}
	
	if (this->Type->BoolFlag[HERBIVORE_INDEX].value && unit.Type->BoolFlag[VEGETABLE_INDEX].value) {
		return true;
	}
	
	if (
		this->Type->BoolFlag[DETRITIVORE_INDEX].value
		&& (
			unit.Type->BoolFlag[DETRITUS_INDEX].value
			|| (unit.CurrentAction() == UnitActionDie && (unit.Type->BoolFlag[FLESH_INDEX].value || unit.Type->BoolFlag[INSECT_INDEX].value))
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

CAnimations *CUnit::GetAnimations() const
{
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (varinfo && varinfo->Animations) {
		return varinfo->Animations;
	} else {
		return Type->Animations;
	}
}

CConstruction *CUnit::GetConstruction() const
{
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (varinfo && varinfo->Construction) {
		return varinfo->Construction;
	} else {
		return Type->Construction;
	}
}

IconConfig CUnit::GetIcon() const
{
	if (this->Character != NULL && this->Character->Level >= 3 && this->Character->HeroicIcon.Icon) {
		return this->Character->HeroicIcon;
	} else if (this->Character != NULL && this->Character->Icon.Icon) {
		return this->Character->Icon;
	} else if (this->Unique != NULL && this->Unique->Icon.Icon) {
		return this->Unique->Icon;
	} else if (Type->VarInfo[Variation] && Type->VarInfo[Variation]->Icon.Icon) {
		return Type->VarInfo[Variation]->Icon;
	} else {
		return Type->Icon;
	}
}

CIcon *CUnit::GetButtonIcon(int button_action) const
{
	if (this->ButtonIcons.find(button_action) != this->ButtonIcons.end()) {
		return this->ButtonIcons.find(button_action)->second;
	} else if (this->Player == ThisPlayer && ThisPlayer->Faction != -1 && PlayerRaces.Factions[ThisPlayer->Race][ThisPlayer->Faction]->ButtonIcons.find(button_action) != PlayerRaces.Factions[ThisPlayer->Race][ThisPlayer->Faction]->ButtonIcons.end()) {
		return PlayerRaces.Factions[ThisPlayer->Race][ThisPlayer->Faction]->ButtonIcons[button_action].Icon;
	} else if (this->Player == ThisPlayer && PlayerRaces.ButtonIcons[ThisPlayer->Race].find(button_action) != PlayerRaces.ButtonIcons[ThisPlayer->Race].end()) {
		return PlayerRaces.ButtonIcons[ThisPlayer->Race][button_action].Icon;
	}
	
	return NULL;
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
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (this->LayerVariation[image_layer] != -1 && this->LayerVariation[image_layer] < ((int) this->Type->LayerVarInfo[image_layer].size()) && this->Type->LayerVarInfo[image_layer][this->LayerVariation[image_layer]]->Sprite) {
		return this->Type->LayerVarInfo[image_layer][this->LayerVariation[image_layer]]->Sprite;
	} else if (varinfo && varinfo->LayerSprites[image_layer]) {
		return varinfo->LayerSprites[image_layer];
	} else if (Type->LayerSprites[image_layer])  {
		return Type->LayerSprites[image_layer];
	} else {
		return NULL;
	}
}

std::string CUnit::GetName() const
{
	if (this->Character && this->Character->Deity) {
		if (this->Character->Deity->CulturalNames.find(ThisPlayer->Race) != this->Character->Deity->CulturalNames.end()) {
			return this->Character->Deity->CulturalNames.find(ThisPlayer->Race)->second;
		} else {
			return this->Character->Deity->Name;
		}
	}
	
	return this->Name;
}

std::string CUnit::GetTypeName() const
{
	if (this->Character && this->Character->Deity) {
		return _("Deity");
	}
	
	VariationInfo *varinfo = Type->VarInfo[Variation];
	if (varinfo && !varinfo->TypeName.empty()) {
		return _(varinfo->TypeName.c_str());
	} else {
		return _(Type->Name.c_str());
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
	
	if (!this->Unique && this->Work == NULL && this->Elixir == NULL && (this->Prefix != NULL || this->Suffix != NULL || this->Spell != NULL)) {
		return name;
	}
	
	return name + " (" + GetTypeName() + ")";
}
//Wyrmgus end

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

	const CUnitType *type = unit.Type;

	while (unit.Resource.Workers) {
		unit.Resource.Workers->DeAssignWorkerFromMine(unit);
	}

	// removed units,  just remove.
	if (unit.Removed) {
		DebugPrint("Killing a removed unit?\n");
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

		//Wyrmgus start
//		MakeMissile(*type->Explosion.Missile, pixelPos, pixelPos);
		MakeMissile(*type->Explosion.Missile, pixelPos, pixelPos, unit.MapLayer);
		//Wyrmgus end
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
		
		//Wyrmgus start
//		MakeMissile(*type->Missile.Missile, pixelPos, pixelPos);
		MakeMissile(*unit.GetMissile().Missile, pixelPos, pixelPos, unit.MapLayer);
		//Wyrmgus end
	}
	// Handle Teleporter Destination Removal
	if (type->BoolFlag[TELEPORTER_INDEX].value && unit.Goal) {
		unit.Goal->Remove(NULL);
		UnitLost(*unit.Goal);
		UnitClearOrders(*unit.Goal);
		unit.Goal->Release();
		unit.Goal = NULL;
	}

	// Transporters lose or save their units and building their workers
	//Wyrmgus start
//	if (unit.UnitInside && unit.Type->BoolFlag[SAVECARGO_INDEX].value) {
	if (
		unit.UnitInside
		&& (
			unit.Type->BoolFlag[SAVECARGO_INDEX].value
			|| (unit.HasInventory() && (unit.Character == NULL || !unit.Character->Persistent))
		)
	) {
	//Wyrmgus end
		DropOutAll(unit);
	} else if (unit.UnitInside) {
		DestroyAllInside(unit);
	}
	
	//Wyrmgus start
	//if is a raft or bridge, destroy all land units on it
	if (unit.Type->BoolFlag[BRIDGE_INDEX].value) {
		std::vector<CUnit *> table;
		Select(unit.tilePos, unit.tilePos, table, unit.MapLayer);
		for (size_t i = 0; i != table.size(); ++i) {
			if (table[i]->IsAliveOnMap() && !table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->Type->UnitType == UnitTypeLand) {
				table[i]->Variable[HP_INDEX].Value = std::min<int>(0, unit.Variable[HP_INDEX].Value);
				table[i]->Moving = 0;
				table[i]->TTL = 0;
				table[i]->Anim.Unbreakable = 0;
				PlayUnitSound(*table[i], VoiceDying);
				table[i]->Remove(NULL);
				UnitLost(*table[i]);
				UnitClearOrders(*table[i]);
				table[i]->Release();
			}
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	//drop items upon death
	if (unit.CurrentAction() != UnitActionBuilt && (unit.Character || unit.Type->BoolFlag[BUILDING_INDEX].value || SyncRand(100) >= 66)) { //66% chance nothing will be dropped, unless the unit is a character or building, in which it case it will always drop an item
		unit.GenerateDrop();
	}
	//Wyrmgus end

	unit.Remove(NULL);
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
		unit.IX = (type->CorpseType->Width - type->CorpseType->Sprite->Width) / 2;
		unit.IY = (type->CorpseType->Height - type->CorpseType->Sprite->Height) / 2;

		unit.CurrentSightRange = type->CorpseType->Stats[unit.Player->Index].Variables[SIGHTRANGE_INDEX].Max;
	} else {
		unit.CurrentSightRange = 0;
	}

	// If we have a corpse, or a death animation, we are put back on the map
	// This enables us to be tracked.  Possibly for spells (eg raise dead)
	//Wyrmgus start
//	if (type->CorpseType || (type->Animations && type->Animations->Death)) {
	if (type->CorpseType || unit.GetAnimations()) {
	//Wyrmgus end
		unit.Removed = 0;
		Map.Insert(unit);

		// FIXME: rb: Maybe we need this here because corpse of cloaked units
		//	may crash Sign code

		// Recalculate the seen count.
		//UnitCountSeen(unit);
	}
	
	//Wyrmgus start
	if (unit.Variable[LEVELUP_INDEX].Value > 0) {
		unit.Player->UpdateLevelUpUnits(); //recalculate level-up units, since this unit no longer should be in that vector
	}
	//Wyrmgus end
	
	MapMarkUnitSight(unit);
	
	//Wyrmgus start
	if (type->BoolFlag[TOWNHALL_INDEX].value) {
		for (int i = 0; i < unit.Player->GetUnitCount(); ++i) {
			CUnit *settlement_unit = &unit.Player->GetUnit(i);
			if (!settlement_unit || !settlement_unit->IsAliveOnMap() || !settlement_unit->Type->BoolFlag[BUILDING_INDEX].value || settlement_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
				continue;
			}
			settlement_unit->UpdateSettlementName();
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
	const CUnitType &type = *unit.Type;
	const CUnitType &dtype = *dest.Type;
	int cost = 0;

	// Buildings, non-aggressive and invincible units have the lowest priority
	if (dest.IsAgressive() == false || dest.Variable[UNHOLYARMOR_INDEX].Value > 0
		|| dest.Type->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
		if (dest.Type->CanMove() == false) {
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

	//Wyrmgus start
//	if (d <= unit.Stats->Variables[ATTACKRANGE_INDEX].Max && d >= type.MinAttackRange) {
	if (d <= unit.GetModifiedVariable(ATTACKRANGE_INDEX) && d >= type.MinAttackRange) {
	//Wyrmgus end
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
	if (target.Type->BoolFlag[WALL_INDEX].value || (lastattack && GameCycle <= lastattack + 2 * CYCLES_PER_SECOND)) {
		return;
	}
	// NOTE: perhaps this should also be moved into the notify?
	if (target.Player == ThisPlayer) {
		// FIXME: Problem with load+save.

		//
		// One help cry each 2 second is enough
		// If on same area ignore it for 2 minutes.
		//
		if (HelpMeLastCycle < GameCycle) {
			if (!HelpMeLastCycle
				|| HelpMeLastCycle + CYCLES_PER_SECOND * 120 < GameCycle
				|| target.tilePos.x < HelpMeLastX - 14
				|| target.tilePos.x > HelpMeLastX + 14
				|| target.tilePos.y < HelpMeLastY - 14
				|| target.tilePos.y > HelpMeLastY + 14) {
				HelpMeLastCycle = GameCycle + CYCLES_PER_SECOND * 2;
				HelpMeLastX = target.tilePos.x;
				HelpMeLastY = target.tilePos.y;
				PlayUnitSound(target, VoiceHelpMe);
				//Wyrmgus start
				//attacked messages now only appear if the "help" sound would be played too
				target.Player->Notify(NotifyRed, target.tilePos, target.MapLayer, _("%s attacked"), target.GetMessageName().c_str());
				//Wyrmgus end
			}
		}
	}
	//Wyrmgus start
//	target.Player->Notify(NotifyRed, target.tilePos, _("%s attacked"), target.Type->Name.c_str());
	//moved this because it was causing message spam
	//Wyrmgus end

	if (attacker && !target.Type->Building) {
		//Wyrmgus start
//		if (target.Player->AiEnabled) {
		if (
			target.Player->AiEnabled
			&& !attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
		) {

		//Wyrmgus end
			AiHelpMe(attacker, target);
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
	attacker.Player->Score += target.Variable[POINTS_INDEX].Value;
	if (target.Type->Building) {
		attacker.Player->TotalRazings++;
	} else {
		attacker.Player->TotalKills++;
	}
	//Wyrmgus start
	attacker.Player->UnitTypeKills[target.Type->Slot]++;
	/*
	if (UseHPForXp) {
		attacker.Variable[XP_INDEX].Max += target.Variable[HP_INDEX].Value;
	} else {
		attacker.Variable[XP_INDEX].Max += target.Variable[POINTS_INDEX].Value;
	}
	attacker.Variable[XP_INDEX].Value = attacker.Variable[XP_INDEX].Max;
	*/
	
	//distribute experience between nearby units belonging to the same player
	if (!target.Type->BoolFlag[BUILDING_INDEX].value) {
		std::vector<CUnit *> table;
		SelectAroundUnit(attacker, 6, table, MakeAndPredicate(HasSamePlayerAs(*attacker.Player), IsNotBuildingType()));

		if (UseHPForXp) {
			attacker.Variable[XP_INDEX].Max += target.Variable[HP_INDEX].Value / (table.size() + 1);
		} else {
			attacker.Variable[XP_INDEX].Max += target.Variable[POINTS_INDEX].Value / (table.size() + 1);
		}
		attacker.Variable[XP_INDEX].Value = attacker.Variable[XP_INDEX].Max;
		attacker.XPChanged();

		for (size_t i = 0; i != table.size(); ++i) {
			if (UseHPForXp) {
				table[i]->Variable[XP_INDEX].Max += target.Variable[HP_INDEX].Value / (table.size() + 1);
			} else {
				table[i]->Variable[XP_INDEX].Max += target.Variable[POINTS_INDEX].Value / (table.size() + 1);
			}
			table[i]->Variable[XP_INDEX].Value = table[i]->Variable[XP_INDEX].Max;
			table[i]->XPChanged();
		}
	}
	//Wyrmgus end
	attacker.Variable[KILL_INDEX].Value++;
	attacker.Variable[KILL_INDEX].Max++;
	attacker.Variable[KILL_INDEX].Enable = 1;
	
	//Wyrmgus start
	for (size_t i = 0; i < attacker.Player->QuestDestroyUnits.size(); ++i) {
		if (std::get<1>(attacker.Player->QuestDestroyUnits[i]) == target.Type && (std::get<2>(attacker.Player->QuestDestroyUnits[i]) == NULL || (std::get<2>(attacker.Player->QuestDestroyUnits[i])->Civilization == target.Player->Race && std::get<2>(attacker.Player->QuestDestroyUnits[i])->ID == target.Player->Faction))) {
			std::get<3>(attacker.Player->QuestDestroyUnits[i]) -= 1;
		}
	}
	
	if (target.Unique) {
		for (size_t i = 0; i < attacker.Player->QuestDestroyUniques.size(); ++i) {
			if (std::get<1>(attacker.Player->QuestDestroyUniques[i]) == target.Unique) {
				std::get<2>(attacker.Player->QuestDestroyUniques[i]) = true;
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
			clamp(&target.Variable[SHIELD_INDEX].Value, 0, target.Variable[SHIELD_INDEX].Max);
		}
		target.Variable[HP_INDEX].Value -= damage - shieldDamage;
	}
	//Wyrmgus start
	/*
	if (UseHPForXp && attacker && target.IsEnemy(*attacker)) {
		attacker->Variable[XP_INDEX].Value += damage;
		attacker->Variable[XP_INDEX].Max += damage;
	}
	*/
	
	//distribute experience between nearby units belonging to the same player

	//Wyrmgus start
//	if (UseHPForXp && attacker && target.IsEnemy(*attacker)) {
	if (UseHPForXp && attacker && (target.IsEnemy(*attacker) || target.Player->Type == PlayerNeutral) && !target.Type->BoolFlag[BUILDING_INDEX].value) {
	//Wyrmgus end
		std::vector<CUnit *> table;
		SelectAroundUnit(*attacker, 6, table, MakeAndPredicate(HasSamePlayerAs(*attacker->Player), IsNotBuildingType()));

		attacker->Variable[XP_INDEX].Value += damage / (table.size() + 1);
		attacker->Variable[XP_INDEX].Max += damage / (table.size() + 1);
		attacker->XPChanged();

		for (size_t i = 0; i != table.size(); ++i) {
			table[i]->Variable[XP_INDEX].Value += damage / (table.size() + 1);
			table[i]->Variable[XP_INDEX].Max += damage / (table.size() + 1);
			table[i]->XPChanged();
		}
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
		&& target.Type->Building && target.Variable[HP_INDEX].Value <= damage * 3
		&& attacker->IsEnemy(target)
		&& attacker->Type->RepairRange) {
		target.ChangeOwner(*attacker->Player);
		CommandStopUnit(*attacker); // Attacker shouldn't continue attack!
	}
}

static void HitUnit_ShowDamageMissile(const CUnit &target, int damage)
{
	const PixelPos targetPixelCenter = target.GetMapPixelPosCenter();

	if ((target.IsVisibleOnMap(*ThisPlayer) || ReplayRevealMap) && !DamageMissile.empty()) {
		const MissileType *mtype = MissileTypeByIdent(DamageMissile);
		const PixelDiff offset(3, -mtype->Range);

		//Wyrmgus start
//		MakeLocalMissile(*mtype, targetPixelCenter, targetPixelCenter + offset)->Damage = -damage;
		MakeLocalMissile(*mtype, targetPixelCenter, targetPixelCenter + offset, target.MapLayer)->Damage = -damage;
		//Wyrmgus end
	}
}

static void HitUnit_ShowImpactMissile(const CUnit &target)
{
	const PixelPos targetPixelCenter = target.GetMapPixelPosCenter();
	const CUnitType &type = *target.Type;

	if (target.Variable[SHIELD_INDEX].Value > 0
		&& !type.Impact[ANIMATIONS_DEATHTYPES + 1].Name.empty()) { // shield impact
		//Wyrmgus start
//		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES + 1].Missile, targetPixelCenter, targetPixelCenter);
		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES + 1].Missile, targetPixelCenter, targetPixelCenter, target.MapLayer);
		//Wyrmgus end
	} else if (target.DamagedType && !type.Impact[target.DamagedType].Name.empty()) { // specific to damage type impact
		//Wyrmgus start
//		MakeMissile(*type.Impact[target.DamagedType].Missile, targetPixelCenter, targetPixelCenter);
		MakeMissile(*type.Impact[target.DamagedType].Missile, targetPixelCenter, targetPixelCenter, target.MapLayer);
		//Wyrmgus end
	} else if (!type.Impact[ANIMATIONS_DEATHTYPES].Name.empty()) { // generic impact
		//Wyrmgus start
//		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES].Missile, targetPixelCenter, targetPixelCenter);
		MakeMissile(*type.Impact[ANIMATIONS_DEATHTYPES].Missile, targetPixelCenter, targetPixelCenter, target.MapLayer);
		//Wyrmgus end
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
		const PixelDiff offset(0, -PixelTileSize.y);
		//Wyrmgus start
//		Missile *missile = MakeMissile(*fire, targetPixelCenter + offset, targetPixelCenter + offset);
		Missile *missile = MakeMissile(*fire, targetPixelCenter + offset, targetPixelCenter + offset, target.MapLayer);
		//Wyrmgus end

		missile->SourceUnit = &target;
		target.Burning = 1;
	}
}

//Wyrmgus start
//static void HitUnit_RunAway(CUnit &target, const CUnit &attacker)
void HitUnit_RunAway(CUnit &target, const CUnit &attacker)
//Wyrmgus end
{
	Vec2i pos = target.tilePos - attacker.tilePos;
	int d = isqrt(pos.x * pos.x + pos.y * pos.y);

	if (!d) {
		d = 1;
	}
	pos.x = target.tilePos.x + (pos.x * 5) / d + (SyncRand() & 3);
	pos.y = target.tilePos.y + (pos.y * 5) / d + (SyncRand() & 3);
	//Wyrmgus start
//	Map.Clamp(pos);
	Map.Clamp(pos, target.MapLayer);
	//Wyrmgus end
	CommandStopUnit(target);
	//Wyrmgus start
//	CommandMove(target, pos, 0);
	CommandMove(target, pos, 0, target.MapLayer);
	//Wyrmgus end
}

static void HitUnit_AttackBack(CUnit &attacker, CUnit &target)
{
	const int threshold = 30;
	COrder *savedOrder = NULL;

	//Wyrmgus start
//	if (target.Player->AiEnabled == false) {
	if (target.Player->AiEnabled == false && target.Player->Type != PlayerNeutral && attacker.Player->Type != PlayerNeutral) { // allow neutral units to strike back, or units to strike back from neutral units
	//Wyrmgus end
		if (target.CurrentAction() == UnitActionAttack) {
			COrder_Attack &order = dynamic_cast<COrder_Attack &>(*target.CurrentOrder());
			if (order.IsWeakTargetSelected() == false) {
				return;
			}
		} else {
			return;
		}
	}
	if (target.CanStoreOrder(target.CurrentOrder())) {
		savedOrder = target.CurrentOrder()->Clone();
	}
	CUnit *oldgoal = target.CurrentOrder()->GetGoal();
	CUnit *goal, *best = oldgoal;

	if (RevealAttacker && CanTarget(*target.Type, *attacker.Type)) {
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
	if (CanTarget(*target.Type, *attacker.Type)
		&& (!best || (goal != &attacker
					  && (ThreatCalculate(target, attacker) < ThreatCalculate(target, *best))))) {
		best = &attacker;
	}
	//Wyrmgus start
//	if (best && best != oldgoal && best->Player != target.Player && best->IsAllied(target) == false) {
	if (best && best != oldgoal && (best->Player != target.Player || target.Player->Type == PlayerNeutral) && best->IsAllied(target) == false) {
	//Wyrmgus end
		//Wyrmgus start
//		CommandAttack(target, best->tilePos, best, FlushCommands);
		CommandAttack(target, best->tilePos, best, FlushCommands, best->MapLayer);
		//Wyrmgus end
		// Set threshold value only for aggressive units
		if (best->IsAgressive()) {
			target.Threshold = threshold;
		}
		if (savedOrder != NULL) {
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
	const CUnitType *type = target.Type;
	if (!damage) {
		// Can now happen by splash damage
		// Multiple places send x/y as damage, which may be zero
		return;
	}

	if (target.Variable[UNHOLYARMOR_INDEX].Value > 0 || target.Type->BoolFlag[INDESTRUCTIBLE_INDEX].value) {
		// vladi: units with active UnholyArmour are invulnerable
		return;
	}
	if (target.Removed) {
		DebugPrint("Removed target hit\n");
		return;
	}

	Assert(damage != 0 && target.CurrentAction() != UnitActionDie && !target.Type->BoolFlag[VANISHES_INDEX].value);

	//Wyrmgus start
	if ((attacker != NULL && attacker->Player == ThisPlayer) &&
		target.Player != ThisPlayer &&
		(type->Building == 1 ||
		type->CanAttack == 1)
		) {
		// If player is hitting or being hit add tension to our music
		AddMusicTension(1);
	}
	//Wyrmgus end

	if (GodMode) {
		if (attacker && attacker->Player == ThisPlayer) {
			damage = target.Variable[HP_INDEX].Value;
		}
		if (target.Player == ThisPlayer) {
			damage = 0;
		}
	}
	HitUnit_LastAttack(attacker, target);
	if (attacker) {
		target.DamagedType = ExtraDeathIndex(attacker->Type->DamageType.c_str());
	}

	//Wyrmgus start
//	if (attacker && !target.Type->BoolFlag[WALL_INDEX].value && target.Player->AiEnabled) {
	if (
		attacker
		&& !target.Type->BoolFlag[WALL_INDEX].value
		&& target.Player->AiEnabled
		&& !attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
	) {
	//Wyrmgus end
		AiHelpMe(attacker, target);
	}

	// OnHit callback
	if (type->OnHit) {
		const int tarSlot = UnitNumber(target);
		const int atSlot = attacker && attacker->IsAlive() ? UnitNumber(*attacker) : -1;

		type->OnHit->pushPreamble();
		type->OnHit->pushInteger(tarSlot);
		type->OnHit->pushInteger(atSlot);
		type->OnHit->pushInteger(damage);
		type->OnHit->run();
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

	if (HitUnit_IsUnitWillDie(attacker, target, damage)) { // unit is killed or destroyed
		if (attacker) {
			//  Setting ai threshold counter to 0 so it can target other units
			attacker->Threshold = 0;
			//Wyrmgus start
//			if (target.IsEnemy(*attacker)) {
			if (target.IsEnemy(*attacker) || target.Player->Type == PlayerNeutral) {
			//Wyrmgus end
				HitUnit_IncreaseScoreForKill(*attacker, target);
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
//	if (type->Building && !target.Burning) {
	if (type->Building && !target.Burning && !target.Constructed) { //the building shouldn't burn if it's still under construction
	//Wyrmgus end
		HitUnit_Burning(target);
	}

	/* Target Reaction on Hit */
	if (target.Player->AiEnabled) {
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
		(!target.IsAgressive() || attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value)
		&& target.CanMove()
		&& (target.CurrentAction() == UnitActionStill || target.Variable[TERROR_INDEX].Value > 0)
		&& !target.BoardCount
		&& !target.Type->BoolFlag[BRIDGE_INDEX].value
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
		&& (target.IsAgressive() || (target.CanAttack() && target.Type->BoolFlag[COWARD_INDEX].value && (attacker->Type->BoolFlag[COWARD_INDEX].value || attacker->Variable[HP_INDEX].Value <= 3))) // attacks back if isn't coward, or if attacker is also coward, or if attacker has 3 HP or less 
		&& target.CanMove()
		&& !target.ReCast
		&& !attacker->Type->BoolFlag[INDESTRUCTIBLE_INDEX].value // don't attack indestructible units back
	) {
	//Wyrmgus end
		// Attack units in range (which or the attacker?)
		// Don't bother unit if it casting repeatable spell
		HitUnit_AttackBack(*attacker, target);
	}

	// What should we do with workers on :
	// case UnitActionRepair:
	// Drop orders and run away or return after escape?
}

/*----------------------------------------------------------------------------
--  Conflicts
----------------------------------------------------------------------------*/

/**
 **  Returns the map distance to unit.
 **
 **  @param pos   map tile position.
 **
 **  @return      The distance between in tiles.
 */
 //Wyrmgus start
//int CUnit::MapDistanceTo(const Vec2i &pos) const
int CUnit::MapDistanceTo(const Vec2i &pos, int z) const
//Wyrmgus end
{
	//Wyrmgus start
	if (z != this->MapLayer) {
		return 16384;
	}
	//Wyrmgus end
	
	int dx;
	int dy;

	if (pos.x <= tilePos.x) {
		dx = tilePos.x - pos.x;
	//Wyrmgus start
	} else if (this->Container) { //if unit is within another, use the tile size of the transporter to calculate the distance
		dx = std::max<int>(0, pos.x - tilePos.x - this->Container->Type->TileWidth + 1);
	//Wyrmgus end
	} else {
		dx = std::max<int>(0, pos.x - tilePos.x - Type->TileWidth + 1);
	}
	if (pos.y <= tilePos.y) {
		dy = tilePos.y - pos.y;
	//Wyrmgus start
	} else if (this->Container) {
		dy = std::max<int>(0, pos.y - tilePos.y - this->Container->Type->TileHeight + 1);
	//Wyrmgus end
	} else {
		dy = std::max<int>(0, pos.y - tilePos.y - Type->TileHeight + 1);
	}
	return isqrt(dy * dy + dx * dx);
}

/**
**  Returns the map distance between two points with unit type.
**
**  @param src  src unittype
**  @param pos1 map tile position of src (upperleft).
**  @param dst  Unit type to take into account.
**  @param pos2 map tile position of dst.
**
**  @return     The distance between the types.
*/
//Wyrmgus start
//int MapDistanceBetweenTypes(const CUnitType &src, const Vec2i &pos1, const CUnitType &dst, const Vec2i &pos2)
int MapDistanceBetweenTypes(const CUnitType &src, const Vec2i &pos1, int src_z, const CUnitType &dst, const Vec2i &pos2, int dst_z)
//Wyrmgus end
{
	//Wyrmgus start
	if (src_z != dst_z) {
		return 16384;
	}
	//Wyrmgus end
	
	int dx;
	int dy;

	if (pos1.x + src.TileWidth <= pos2.x) {
		dx = std::max<int>(0, pos2.x - pos1.x - src.TileWidth + 1);
	} else {
		dx = std::max<int>(0, pos1.x - pos2.x - dst.TileWidth + 1);
	}
	if (pos1.y + src.TileHeight <= pos2.y) {
		dy = pos2.y - pos1.y - src.TileHeight + 1;
	} else {
		dy = std::max<int>(0, pos1.y - pos2.y - dst.TileHeight + 1);
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
	return dest.MapDistanceTo(midPos, CurrentMapLayer);
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
	if (!transporter.Type->CanTransport()) {
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
	if (transporter.BoardCount >= transporter.Type->MaxOnBoard) { // full
		return 0;
	}
	*/
	//Wyrmgus end

	if (transporter.BoardCount + unit.Type->BoardSize > transporter.Type->MaxOnBoard) { // too big unit
		return 0;
	}

	// Can transport only allied unit.
	// FIXME : should be parametrable.
	//Wyrmgus start
//	if (!transporter.IsTeamed(unit)) {
	if (!transporter.IsTeamed(unit) && !transporter.IsAllied(unit) && transporter.Player->Type != PlayerNeutral && unit.Player->Type != PlayerNeutral) {
	//Wyrmgus end
		return 0;
	}
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); i++) {
		if (transporter.Type->BoolFlag[i].CanTransport != CONDITION_TRUE) {
			if ((transporter.Type->BoolFlag[i].CanTransport == CONDITION_ONLY) ^ unit.Type->BoolFlag[i].value) {
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
	if (!picker.Type->BoolFlag[ORGANIC_INDEX].value) { //only organic units can pick up power-ups and items
		return false;
	}
	if (!unit.Type->BoolFlag[ITEM_INDEX].value && !unit.Type->BoolFlag[POWERUP_INDEX].value) { //only item and powerup units can be picked up
		return false;
	}
	if (!unit.Type->BoolFlag[POWERUP_INDEX].value && !picker.HasInventory() && !IsItemClassConsumable(unit.Type->ItemClass)) { //only consumable items can be picked up as if they were power-ups for units with no inventory
		return false;
	}
	if (picker.CurrentAction() == UnitActionBuilt) { // Under construction
		return false;
	}
	if (&picker == &unit) { // Cannot pick up itself.
		return false;
	}
	if (picker.HasInventory() && unit.Type->BoolFlag[ITEM_INDEX].value && picker.InsideCount >= ((int) UI.InventoryButtons.size())) { // full
		if (picker.Player == ThisPlayer) {
			std::string picker_name = picker.Name + "'s (" + picker.GetTypeName() + ")";
			picker.Player->Notify(NotifyRed, picker.tilePos, picker.MapLayer, _("%s inventory is full."), picker_name.c_str());
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
	if (this->Player->Index != player.Index && player.Type != PlayerNeutral && this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->IsAgressive()) {
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
		&& unit.Type->BoolFlag[ORGANIC_INDEX].value
		&& this->Type->Slot != unit.Type->Slot
	) {
		if (
			this->Type->BoolFlag[PREDATOR_INDEX].value
			&& !unit.Type->BoolFlag[PREDATOR_INDEX].value
			&& this->CanEat(unit)
		) {
			return true;
		} else if (
			this->Type->BoolFlag[PEOPLEAVERSION_INDEX].value
			&& !unit.Type->BoolFlag[FAUNA_INDEX].value
			&& unit.Player->Type != PlayerNeutral
			&& this->MapDistanceTo(unit) <= 1
		) {
			return true;
		}
	}
		
	if (
		unit.Player->Type == PlayerNeutral
		&& unit.Type->BoolFlag[PREDATOR_INDEX].value
		&& this->Player->Type != PlayerNeutral
	) {
		return true;
	}
	
	if (
		this->Player != unit.Player
		&& this->Player->Type != PlayerNeutral
		&& unit.CurrentAction() == UnitActionAttack
		&& unit.CurrentOrder()->HasGoal()
		&& unit.CurrentOrder()->GetGoal()->Player == this->Player
	) {
		return true;
	}
	
	if (
		this->Player != unit.Player && this->Player->Type != PlayerNeutral && unit.Player->Type != PlayerNeutral
		&& ((this->Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && this->IsAgressive()) || (unit.Type->BoolFlag[HIDDENOWNERSHIP_INDEX].value && unit.IsAgressive()))
	) {
		return true;
	}

	return IsEnemy(*unit.Player);
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
	return IsAllied(*unit.Player);
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
	return IsSharedVision(*unit.Player);
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
	return IsBothSharedVision(*unit.Player);
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
	return this->IsTeamed(*unit.Player);
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
			|| (this->Type->UnitType != UnitTypeFly && goal->Type->UnitType == UnitTypeFly)
			|| (this->Type->UnitType == UnitTypeFly && goal->Type->UnitType != UnitTypeFly)
		)
	) {
		return true;
	}
	
	//Wyrmgus start
//	if (!goal && Map.Info.IsPointOnMap(goalPos) && this->MapDistanceTo(goalPos) > 1) {
	if (!goal && Map.Info.IsPointOnMap(goalPos, z) && this->MapDistanceTo(goalPos, z) > 1) {
	//Wyrmgus end
		return true;
	}
	
	return false;
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
		if (*it == NULL) {
			fprintf(stderr, "Error in CleanUnits: unit is NULL.\n");
			continue;
		}
		//Wyrmgus end
		CUnit &unit = **it;

		//Wyrmgus start
		/*
		if (&unit == NULL) {
			continue;
		}
		*/
		//Wyrmgus end
		//Wyrmgus start
		if (unit.Type == NULL) {
			fprintf(stderr, "Unit \"%d\"'s type is NULL.\n", UnitNumber(unit));
		}
		//Wyrmgus end
		if (!unit.Destroyed) {
			if (!unit.Removed) {
				unit.Remove(NULL);
			}
			UnitClearOrders(unit);
		}
		unit.Release(true);
	}

	UnitManager.Init();

	FancyBuildings = false;
	HelpMeLastCycle = 0;
}

//@}
