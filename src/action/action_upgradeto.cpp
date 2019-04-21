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
/**@name action_upgradeto.cpp - The unit upgrading to new action. */
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

#include "action/action_upgradeto.h"

#include "ai/ai.h"
#include "animation/animation.h"
#include "civilization.h"
#include "dependency/dependency.h"
#include "faction.h"
//Wyrmgus start
#include "game/game.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "player.h"
#include "quest/quest.h"
#include "script.h"
#include "spell/spells.h"
#include "translate.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end

/// How many resources the player gets back if canceling upgrade
constexpr int CANCEL_UPGRADE_COSTS_FACTOR = 100;


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionTransformInto(CUnitType &type)
{
	COrder_TransformInto *order = new COrder_TransformInto;

	order->Type = &type;
	return order;
}

/* static */ COrder *COrder::NewActionUpgradeTo(CUnit &unit, CUnitType &type)
{
	COrder_UpgradeTo *order = new COrder_UpgradeTo;

	// FIXME: if you give quick an other order, the resources are lost!
	unit.Player->SubUnitType(type);
	order->Type = &type;
	return order;
}

/**
**  Transform a unit in another.
**
**  @param unit     unit to transform.
**  @param newtype  new type of the unit.
**
**  @return 0 on error, 1 if nothing happens, 2 else.
*/
//Wyrmgus start
//static int TransformUnitIntoType(CUnit &unit, const CUnitType &newtype)
int TransformUnitIntoType(CUnit &unit, const CUnitType &newtype)
//Wyrmgus end
{
	const CUnitType &oldtype = *unit.Type;
	if (&oldtype == &newtype) { // nothing to do
		return 1;
	}
	const Vec2i pos = unit.tilePos + oldtype.GetHalfTileSize() - newtype.GetHalfTileSize();
	CUnit *container = unit.Container;

	//Wyrmgus start
	/*
	if (container) {
		MapUnmarkUnitSight(unit);
	} else {
		SaveSelection();
		unit.Remove(nullptr);
		if (!UnitTypeCanBeAt(newtype, pos)) {
			unit.Place(unit.tilePos);
			RestoreSelection();
			// FIXME unit is not modified, try later ?
			return 0;
		}
	}
	*/
	if (!SaveGameLoading) {
		if (container) {
			MapUnmarkUnitSight(unit);
		} else {
			SaveSelection();
			unit.Remove(nullptr);
			//Wyrmgus start
			/*
			if (!UnitTypeCanBeAt(newtype, pos)) {
				unit.Place(unit.tilePos);
				RestoreSelection();
				// FIXME unit is not modified, try later ?
				return 0;
			}
			*/
			//Wyrmgus end
		}
	}
	//Wyrmgus end

	CPlayer &player = *unit.Player;
	if (!unit.UnderConstruction) {
		player.DecreaseCountsForUnit(&unit, true);
		
		player.Demand += newtype.Stats[player.Index].Variables[DEMAND_INDEX].Value - oldtype.Stats[player.Index].Variables[DEMAND_INDEX].Value;
		player.Supply += newtype.Stats[player.Index].Variables[SUPPLY_INDEX].Value - oldtype.Stats[player.Index].Variables[SUPPLY_INDEX].Value;

		// Change resource limit
		for (size_t i = 0; i < CResource::GetAll().size(); ++i) {
			if (player.MaxResources[i] != -1) {
				player.MaxResources[i] += newtype.Stats[player.Index].Storing[i] - oldtype.Stats[player.Index].Storing[i];
				player.SetResource(i, player.StoredResources[i], STORE_BUILDING);
			}
		}
	}

	//  adjust Variables with percent.
	const CUnitStats &newstats = newtype.Stats[player.Index];
	//Wyrmgus start
	const CUnitStats &oldstats = oldtype.Stats[player.Index];
	//Wyrmgus end
	
	//if the old unit type had a starting ability that the new one doesn't have, remove it; and apply it if the reverse happens
	for (const CUpgrade *upgrade : AllUpgrades) {
		if (upgrade->Ability) {
			if (unit.GetIndividualUpgrade(upgrade) && std::find(oldtype.StartingAbilities.begin(), oldtype.StartingAbilities.end(), upgrade) != oldtype.StartingAbilities.end() && std::find(newtype.StartingAbilities.begin(), newtype.StartingAbilities.end(), upgrade) == newtype.StartingAbilities.end()) {
				IndividualUpgradeLost(unit, upgrade);
			} else if (!unit.GetIndividualUpgrade(upgrade) && std::find(newtype.StartingAbilities.begin(), newtype.StartingAbilities.end(), upgrade) != newtype.StartingAbilities.end() && CheckDependencies(upgrade, &unit)) {
				IndividualUpgradeAcquire(unit, upgrade);
			}
		}
	}	

	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
		//Wyrmgus start
		/*
		if (unit.Variable[i].Max && unit.Variable[i].Value) {
			unit.Variable[i].Value = newstats.Variables[i].Max *
									 unit.Variable[i].Value / unit.Variable[i].Max;
		} else {
			unit.Variable[i].Value = newstats.Variables[i].Value;
		}
		if (i == KILL_INDEX || i == XP_INDEX) {
			unit.Variable[i].Value = unit.Variable[i].Max;
		} else {
			unit.Variable[i].Max = newstats.Variables[i].Max;
			unit.Variable[i].Increase = newstats.Variables[i].Increase;
			unit.Variable[i].Enable = newstats.Variables[i].Enable;
		}
		*/
		if (i == LEVEL_INDEX || i == LEVELUP_INDEX) { //if the unit's level changed in accordance to the difference between the levels of the two unit types, then its level change would be duplicated when leveling up, so let's skip the level variable here
			continue;
		}
		
		if (unit.Variable[i].Max && unit.Variable[i].Value) {
			if (i != MANA_INDEX || (newstats.Variables[i].Max - oldstats.Variables[i].Max) < 0) {
				unit.Variable[i].Value += newstats.Variables[i].Max - oldstats.Variables[i].Max;
			}
		} else {
			if (i != MANA_INDEX || (newstats.Variables[i].Value - oldstats.Variables[i].Value) < 0) {
				unit.Variable[i].Value += newstats.Variables[i].Value - oldstats.Variables[i].Value;
			}
		}
		if (i == KILL_INDEX || i == XP_INDEX) {
			unit.Variable[i].Value = unit.Variable[i].Max;
		} else {
			unit.Variable[i].Max += newstats.Variables[i].Max - oldstats.Variables[i].Max;
			unit.Variable[i].Increase += newstats.Variables[i].Increase - oldstats.Variables[i].Increase;
			unit.Variable[i].Enable = newstats.Variables[i].Enable;
		}
		//Wyrmgus end
	}

	//Wyrmgus start
	for (std::map<CUnitType *, int>::iterator iterator = unit.UnitStock.begin(); iterator != unit.UnitStock.end(); ++iterator) {
		CUnitType *unit_type = iterator->first;
		
		int unit_stock_change = newstats.GetUnitStock(unit_type) - oldstats.GetUnitStock(unit_type);
		if (unit_stock_change < 0) {
			unit.ChangeUnitStock(unit_type, unit_stock_change);
		}
	}
	//Wyrmgus end
	
	//drop units that can no longer be in the container
	if (unit.UnitInside) {
		CUnit *uins = unit.UnitInside;
		for (int i = unit.InsideCount; i && unit.BoardCount > newtype.MaxOnBoard; --i, uins = uins->NextContained) {
			if (uins->Boarded) {
				uins->Boarded = 0;
				unit.BoardCount -= uins->Type->BoardSize;
				DropOutOnSide(*uins, LookingW, &unit);
			}
		}
	}
					
	//Wyrmgus start
	//change variation if upgrading (new unit type may have different variations)
	unit.ChooseVariation(&newtype);
	for (int i = 0; i < MaxImageLayers; ++i) {
		unit.ChooseVariation(&newtype, false, i);
	}
	//Wyrmgus end
	
	unit.Type = const_cast<CUnitType *>(&newtype);
	unit.Stats = &unit.Type->Stats[player.Index];
	
	//Wyrmgus start
	//change the civilization/faction upgrade markers for those of the new type
	if (oldtype.GetCivilization() != nullptr) {
		const CUpgrade *civilization_upgrade = oldtype.GetCivilization()->GetUpgrade();
		if (civilization_upgrade != nullptr) {
			unit.SetIndividualUpgrade(civilization_upgrade, 0);
		}
		
		if (oldtype.GetFaction() != nullptr && !oldtype.GetFaction()->FactionUpgrade.empty()) {
			CUpgrade *faction_upgrade = CUpgrade::Get(oldtype.GetFaction()->FactionUpgrade);
			if (faction_upgrade) {
				unit.SetIndividualUpgrade(faction_upgrade, 0);
			}
		}
	}
	
	if (newtype.GetCivilization() != nullptr) {
		const CUpgrade *civilization_upgrade = newtype.GetCivilization()->GetUpgrade();
		if (civilization_upgrade != nullptr) {
			unit.SetIndividualUpgrade(civilization_upgrade, 1);
		}

		if (newtype.GetFaction() != nullptr && !newtype.GetFaction()->FactionUpgrade.empty()) {
			CUpgrade *faction_upgrade = CUpgrade::Get(newtype.GetFaction()->FactionUpgrade);
			if (faction_upgrade) {
				unit.SetIndividualUpgrade(faction_upgrade, 1);
			}
		}
	}
	
	//deequip the current equipment if they are incompatible with the new unit type
	for (std::map<const ItemSlot *, std::vector<CUnit *>>::iterator iterator = unit.EquippedItems.begin(); iterator != unit.EquippedItems.end(); ++iterator) {
		for (CUnit *equipped_item : iterator->second) {
			if (!unit.CanEquipItemClass(equipped_item->Type->ItemClass)) {
				unit.DeequipItem(equipped_item);
			}
		}
	}
	//Wyrmgus end
	
	//Wyrmgus start
	//change personal name if new unit type's civilization is different from old unit type's civilization
	if (
		unit.Character == nullptr
		&& (
			oldtype.PersonalNames != newtype.PersonalNames
			|| (
				oldtype.GetCivilization() != nullptr && newtype.GetCivilization() != nullptr && oldtype.GetCivilization() != newtype.GetCivilization()
				&& (
					newtype.BoolFlag[ORGANIC_INDEX].value
					|| (newtype.PersonalNames.size() == 0 && !newtype.BoolFlag[ORGANIC_INDEX].value && newtype.UnitType == UnitTypeNaval)
					|| (oldtype.GetCivilization()->GetUnitClassNames(oldtype.Class) != newtype.GetCivilization()->GetUnitClassNames(newtype.Class))
					|| (oldtype.GetCivilization()->GetUnitClassNames(oldtype.Class) != CCivilization::Get(player.Race)->GetUnitClassNames(newtype.Class))
				)
			)
		)
	) {
		unit.UpdatePersonalName(false);
	}
	//Wyrmgus end

	//Wyrmgus start
//	if (newtype.CanCastSpell && !unit.AutoCastSpell) {
	if (!unit.AutoCastSpell) { //to avoid crashes with spell items for units who cannot ordinarily cast spells
	//Wyrmgus end
		unit.AutoCastSpell = new char[CSpell::Spells.size()];
		unit.SpellCoolDownTimers = new int[CSpell::Spells.size()];
		memset(unit.AutoCastSpell, 0, CSpell::Spells.size() * sizeof(char));
		memset(unit.SpellCoolDownTimers, 0, CSpell::Spells.size() * sizeof(int));
	}

	if (!unit.UnderConstruction) {
		UpdateForNewUnit(unit, 1);
		player.IncreaseCountsForUnit(&unit, true);
	}
	//Wyrmgus start
	/*
	//  Update Possible sight range change
	UpdateUnitSightRange(unit);
	if (!container) {
		unit.Place(pos);
		RestoreSelection();
	} else {
		MapMarkUnitSight(unit);
	}
	*/
	if (!SaveGameLoading) {
		//  Update Possible sight range change
		UpdateUnitSightRange(unit);
		if (!container) {
			if (!UnitTypeCanBeAt(newtype, pos, unit.MapLayer->ID)) {
				DropOutNearest(unit, pos, nullptr);
			} else {
				unit.Place(pos, unit.MapLayer->ID);
			}
			RestoreSelection();
		} else {
			MapMarkUnitSight(unit);
			//Wyrmgus start
			//if unit has a container, update the container's attack range, as the unit's range may have been changed with the upgrade
			container->UpdateContainerAttackRange();
			//Wyrmgus end
		}
	}
	//Wyrmgus end
	//Wyrmgus start
	//update the unit's XP required, as its level or points may have changed
	unit.UpdateXPRequired();
	
	unit.UpdateButtonIcons();
	
	unit.UpdateSoldUnits();
	//Wyrmgus end
	
	//Wyrmgus start
	/*
	//
	// Update possible changed buttons.
	//
	if (IsOnlySelected(unit) || &player == CPlayer::GetThisPlayer()) {
		// could affect the buttons of any selected unit
		SelectedUnitChanged();
	}
	*/
	if (!SaveGameLoading) {
		//
		// Update possible changed buttons.
		//
		if (IsOnlySelected(unit) || &player == CPlayer::GetThisPlayer()) {
			// could affect the buttons of any selected unit
			SelectedUnitChanged();
		}
		
		if (!unit.UnderConstruction) {
			for (CPlayerQuestObjective *objective : player.QuestObjectives) {
				if (
					(objective->ObjectiveType == BuildUnitsObjectiveType && std::find(objective->UnitTypes.begin(), objective->UnitTypes.end(), &newtype) != objective->UnitTypes.end())
					|| (objective->ObjectiveType == BuildUnitsOfClassObjectiveType && objective->UnitClass == newtype.Class)
				) {
					if (!objective->Settlement || objective->Settlement == unit.Settlement) {
						objective->Counter = std::min(objective->Counter + 1, objective->Quantity);
					}
				}
			}
		}
	}
	//Wyrmgus end
	return 1;
}


#if 1 // TransFormInto

/* virtual */ void COrder_TransformInto::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-transform-into\",");
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"type\", \"%s\"", this->Type->Ident.c_str());
	file.printf("}");
}

/* virtual */ bool COrder_TransformInto::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "type")) {
		++j;
		this->Type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
	} else {
		return false;
	}
	return true;
}

/* virtual */ bool COrder_TransformInto::IsValid() const
{
	return true;
}


/* virtual */ PixelPos COrder_TransformInto::Show(const CViewport &, const PixelPos &lastScreenPos) const
{
	return lastScreenPos;
}


/* virtual */ void COrder_TransformInto::Execute(CUnit &unit)
{
	TransformUnitIntoType(unit, *this->Type);
	this->Finished = true;
}

//Wyrmgus start
void COrder_TransformInto::ConvertUnitType(const CUnit &unit, CUnitType &newType)
{
	const CPlayer &player = *unit.Player;
	this->Type = &newType;
}
//Wyrmgus end

#endif

#if 1  //  COrder_UpgradeTo

/* virtual */ void COrder_UpgradeTo::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-upgrade-to\",");
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"type\", \"%s\",", this->Type->Ident.c_str());
	file.printf(" \"ticks\", %d", this->Ticks);
	file.printf("}");
}

/* virtual */ bool COrder_UpgradeTo::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "type")) {
		++j;
		this->Type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
	} else if (!strcmp(value, "ticks")) {
		++j;
		this->Ticks = LuaToNumber(l, -1, j + 1);
	} else {
		return false;
	}
	return true;
}

/* virtual */ bool COrder_UpgradeTo::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_UpgradeTo::Show(const CViewport &, const PixelPos &lastScreenPos) const
{
	return lastScreenPos;
}


static void AnimateActionUpgradeTo(CUnit &unit)
{
	//Wyrmgus start
//	CAnimations &animations = *unit.Type->Animations;
//	UnitShowAnimation(unit, animations.Upgrade ? animations.Upgrade : animations.Still);
	CAnimations &animations = *unit.GetAnimations();
	UnitShowAnimation(unit, animations.Upgrade ? animations.Upgrade : animations.Still);
	//Wyrmgus end
}

/* virtual */ void COrder_UpgradeTo::Execute(CUnit &unit)
{
	AnimateActionUpgradeTo(unit);
	if (unit.Wait) {
		unit.Wait--;
		return ;
	}
	CPlayer &player = *unit.Player;
	const CUnitType &newtype = *this->Type;
	const CUnitStats &newstats = newtype.Stats[player.Index];

	//Wyrmgus start
//	this->Ticks += std::max(1, player.SpeedUpgrade / SPEEDUP_FACTOR);
	this->Ticks += std::max(1, (player.SpeedUpgrade + unit.Variable[TIMEEFFICIENCYBONUS_INDEX].Value) / SPEEDUP_FACTOR);
	//Wyrmgus end
	if (this->Ticks < newstats.Costs[TimeCost]) {
		unit.Wait = CYCLES_PER_SECOND / 6;
		return ;
	}

	if (unit.Anim.Unbreakable) {
		this->Ticks = newstats.Costs[TimeCost];
		return ;
	}

	if (TransformUnitIntoType(unit, newtype) == 0) {
		//Wyrmgus start
		//I think it is too much to notify the player whenever an individual upgrade is cancelled
//		player.Notify(NotifyYellow, unit.tilePos, _("Upgrade to %s canceled"), newtype.Name.c_str());
		//Wyrmgus end
		this->Finished = true;
		return ;
	}
	//Wyrmgus start
	//I think it is too much to notify the player whenever an individual upgrade is completed
//	player.Notify(NotifyGreen, unit.tilePos, _("Upgrade to %s complete"), unit.Type->GetName().c_str());
	//Wyrmgus end

	//  Warn AI.
	if (player.AiEnabled) {
		AiUpgradeToComplete(unit, newtype);
	}
	this->Finished = true;
	
	//Wyrmgus start
	if (newtype.BoolFlag[BUILDING_INDEX].value && !unit.Player->AiEnabled) { //if the unit is a building, remove it from its group (if any) when upgraded (buildings can only be selected together if they are of the same type
		if (unit.GroupId) {
			RemoveUnitFromNonSingleGroups(unit);
		}
	}
	//Wyrmgus end
}

/* virtual */ void COrder_UpgradeTo::Cancel(CUnit &unit)
{
	CPlayer &player = *unit.Player;
	
	int type_costs[MaxCosts];
	player.GetUnitTypeCosts(this->Type, type_costs);

	player.AddCostsFactor(type_costs, CANCEL_UPGRADE_COSTS_FACTOR);
}

/* virtual */ void COrder_UpgradeTo::UpdateUnitVariables(CUnit &unit) const
{
	Assert(unit.CurrentOrder() == this);

	unit.Variable[UPGRADINGTO_INDEX].Value = this->Ticks;
	unit.Variable[UPGRADINGTO_INDEX].Max = this->Type->Stats[unit.Player->Index].Costs[TimeCost];
}

//Wyrmgus start
void COrder_UpgradeTo::ConvertUnitType(const CUnit &unit, CUnitType &newType)
{
	const CPlayer &player = *unit.Player;
	const int oldCost = this->Type->Stats[player.Index].Costs[TimeCost];
	const int newCost = newType.Stats[player.Index].Costs[TimeCost];

	// Must Adjust Ticks to the fraction that was upgraded
	this->Ticks = this->Ticks * newCost / oldCost;
	this->Type = &newType;
}

//Wyrmgus end
#endif
