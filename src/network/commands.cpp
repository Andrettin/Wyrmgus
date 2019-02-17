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
/**@name commands.cpp - Global command handler - network support. */
//
//      (c) Copyright 2000-2007 by Lutz Sammer, Andreas Arens, and Jimmy Salmon.
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

#include "commands.h"

#include "actions.h"
//Wyrmgus start
#include "map/map.h" //it contains map width and height
//Wyrmgus end
#include "net_message.h"
#include "network.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
#include "replay.h"
#include "spells.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unittype.h"

 /*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
** Send command: Unit stop.
**
** @param unit pointer to unit.
*/
void SendCommandStopUnit(CUnit &unit)
{
	if (!IsNetworkGame()) {
		CommandLog("stop", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
		CommandStopUnit(unit);
	} else {
		NetworkSendCommand(MessageCommandStop, unit, 0, 0, NoUnitP, 0, FlushCommands);
	}
}

/**
** Send command: Unit stand ground.
**
** @param unit     pointer to unit.
** @param flush    Flag flush all pending commands.
*/
void SendCommandStandGround(CUnit &unit, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("stand-ground", &unit, flush, -1, -1, NoUnitP, nullptr, -1);
		CommandStandGround(unit, flush);
	} else {
		NetworkSendCommand(MessageCommandStand, unit, 0, 0, NoUnitP, 0, flush);
	}
}

/**
** Send command: Defend some unit.
**
** @param unit    pointer to unit.
** @param dest    defend this unit.
** @param flush   Flag flush all pending commands.
*/
void SendCommandDefend(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("defend", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandDefend(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandDefend, unit, 0, 0, &dest, 0, flush);
	}
}

/**
** Send command: Follow unit to position.
**
** @param unit    pointer to unit.
** @param dest    follow this unit.
** @param flush   Flag flush all pending commands.
*/
void SendCommandFollow(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("follow", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandFollow(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandFollow, unit, 0, 0, &dest, 0, flush);
	}
}

/**
** Send command: Move unit to position.
**
** @param unit    pointer to unit.
** @param pos     map tile position to move to.
** @param flush   Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandMove(CUnit &unit, const Vec2i &pos, int flush)
void SendCommandMove(CUnit &unit, const Vec2i &pos, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("move", &unit, flush, pos.x, pos.y, NoUnitP, nullptr, -1);
		//Wyrmgus start
//		CommandMove(unit, pos, flush);
		CommandMove(unit, pos, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandMove, unit, pos.x, pos.y, NoUnitP, 0, flush);
	}
}

//Wyrmgus start
/**
** Send command: Set new rally point for unit.
**
** @param unit    pointer to unit.
** @param pos     map tile position to move to.
*/
void SendCommandRallyPoint(CUnit &unit, const Vec2i &pos, int z)
{
	if (!IsNetworkGame()) {
		CommandLog("rally-point", &unit, 0, pos.x, pos.y, NoUnitP, nullptr, -1);
		CommandRallyPoint(unit, pos, z);
	} else {
		NetworkSendCommand(MessageCommandMove, unit, pos.x, pos.y, NoUnitP, 0, 0);
	}
}

/**
** Send command: Accept new quest for the unit's player.
**
** @param unit    pointer to unit.
** @param pos     map tile position to move to.
*/
void SendCommandQuest(CUnit &unit, CQuest *quest)
{
	if (!IsNetworkGame()) {
		CommandLog("quest", &unit, 0, 0, 0, NoUnitP, quest->Ident.c_str(), -1);
		CommandQuest(unit, quest);
	} else {
		NetworkSendCommand(MessageCommandQuest, unit, quest->ID, 0, NoUnitP, nullptr, 0);
	}
}

/**
** Send command: Buy an item from a building.
**
** @param unit    pointer to unit.
** @param pos     map tile position to move to.
*/
void SendCommandBuy(CUnit &unit, CUnit *sold_unit, int player)
{
	if (!IsNetworkGame()) {
		CommandLog("buy", &unit, 0, -1, -1, sold_unit, nullptr, player);
		CommandBuy(unit, sold_unit, player);
	} else {
		NetworkSendCommand(MessageCommandBuy, unit, player, 0, sold_unit, 0, 0);
	}
}

/**
** Send command: Produce a resource.
*/
void SendCommandProduceResource(CUnit &unit, int resource)
{
	if (!IsNetworkGame()) {
		CommandLog("produce-resource", &unit, 0, 0, -1, NoUnitP, nullptr, resource);
		CommandProduceResource(unit, resource);
	} else {
		NetworkSendCommand(MessageCommandProduceResource, unit, resource, 0, NoUnitP, nullptr, 0);
	}
}

/**
** Send command: Sell a resource for copper.
*/
void SendCommandSellResource(CUnit &unit, int resource, int player)
{
	if (!IsNetworkGame()) {
		CommandLog("sell-resource", &unit, 0, resource, -1, NoUnitP, nullptr, player);
		CommandSellResource(unit, resource, player);
	} else {
		NetworkSendCommand(MessageCommandSellResource, unit, resource, player, NoUnitP, nullptr, 0);
	}
}

/**
** Send command: Buy a resource with copper.
*/
void SendCommandBuyResource(CUnit &unit, int resource, int player)
{
	if (!IsNetworkGame()) {
		CommandLog("buy-resource", &unit, 0, resource, -1, NoUnitP, nullptr, player);
		CommandBuyResource(unit, resource, player);
	} else {
		NetworkSendCommand(MessageCommandBuyResource, unit, resource, player, NoUnitP, nullptr, 0);
	}
}

/**
** Send command: Pick up item.
**
** @param unit    pointer to unit.
** @param dest    pick up this item.
** @param flush   Flag flush all pending commands.
*/
void SendCommandPickUp(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("pick-up", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandPickUp(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandPickUp, unit, 0, 0, &dest, 0, flush);
	}
}
//Wyrmgus end

/**
** Send command: Unit repair.
**
** @param unit    Pointer to unit.
** @param pos     map tile position to repair.
** @param dest    Unit to be repaired.
** @param flush   Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush)
void SendCommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("repair", &unit, flush, pos.x, pos.y, dest, nullptr, -1);
		//Wyrmgus start
//		CommandRepair(unit, pos, dest, flush);
		CommandRepair(unit, pos, dest, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandRepair, unit, pos.x, pos.y, dest, 0, flush);
	}
}

/**
** Send command: Unit auto repair.
**
** @param unit      pointer to unit.
** @param on        1 for auto repair on, 0 for off.
*/
void SendCommandAutoRepair(CUnit &unit, int on)
{
	if (!IsNetworkGame()) {
		CommandLog("auto-repair", &unit, FlushCommands, on, -1, NoUnitP, nullptr, 0);
		CommandAutoRepair(unit, on);
	} else {
		NetworkSendCommand(MessageCommandAutoRepair, unit, on, -1, NoUnitP, nullptr, FlushCommands);
	}
}

/**
** Send command: Unit attack unit or at position.
**
** @param unit     pointer to unit.
** @param pos      map tile position to attack.
** @param attack   or !=NoUnitP unit to be attacked.
** @param flush    Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandAttack(CUnit &unit, const Vec2i &pos, CUnit *attack, int flush)
void SendCommandAttack(CUnit &unit, const Vec2i &pos, CUnit *attack, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("attack", &unit, flush, pos.x, pos.y, attack, nullptr, -1);
		//Wyrmgus start
//		CommandAttack(unit, pos, attack, flush);
		CommandAttack(unit, pos, attack, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandAttack, unit, pos.x, pos.y, attack, 0, flush);
	}
}

/**
** Send command: Unit attack ground.
**
** @param unit     pointer to unit.
** @param pos      map tile position to fire on.
** @param flush    Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandAttackGround(CUnit &unit, const Vec2i &pos, int flush)
void SendCommandAttackGround(CUnit &unit, const Vec2i &pos, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("attack-ground", &unit, flush, pos.x, pos.y, NoUnitP, nullptr, -1);
		//Wyrmgus start
//		CommandAttackGround(unit, pos, flush);
		CommandAttackGround(unit, pos, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandGround, unit, pos.x, pos.y, NoUnitP, 0, flush);
	}
}

//Wyrmgus start
/**
** Send command: Use a unit.
**
** @param unit    pointer to unit.
** @param dest    use this unit.
** @param flush   Flag flush all pending commands.
*/
void SendCommandUse(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("use", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandUse(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandUse, unit, 0, 0, &dest, 0, flush);
	}
}

/**
** Send command: Trade with a unit.
**
** @param unit    pointer to unit.
** @param dest    trade with this unit.
** @param flush   Flag flush all pending commands.
*/
void SendCommandTrade(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("use", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandTrade(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandTrade, unit, 0, 0, &dest, 0, flush);
	}
}
//Wyrmgus end

/**
** Send command: Unit patrol between current and position.
**
** @param unit     pointer to unit.
** @param pos      map tile position to patrol between.
** @param flush    Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandPatrol(CUnit &unit, const Vec2i &pos, int flush)
void SendCommandPatrol(CUnit &unit, const Vec2i &pos, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("patrol", &unit, flush, pos.x, pos.y, NoUnitP, nullptr, -1);
		//Wyrmgus start
//		CommandPatrolUnit(unit, pos, flush);
		CommandPatrolUnit(unit, pos, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandPatrol, unit, pos.x, pos.y, NoUnitP, 0, flush);
	}
}

/**
** Send command: Unit board unit.
**
** @param unit     pointer to unit.
** @param dest     Destination to be boarded.
** @param flush    Flag flush all pending commands.
*/
void SendCommandBoard(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("board", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandBoard(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandBoard, unit, -1, -1, &dest, 0, flush);
	}
}

/**
** Send command: Unit unload unit.
**
** @param unit    pointer to unit.
** @param pos     map tile position of unload.
** @param what    Passagier to be unloaded.
** @param flush   Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush)
void SendCommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("unload", &unit, flush, pos.x, pos.y, what, nullptr, -1);
		//Wyrmgus start
//		CommandUnload(unit, pos, what, flush);
		CommandUnload(unit, pos, what, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandUnload, unit, pos.x, pos.y, what, 0, flush);
	}
}

/**
** Send command: Unit builds building at position.
**
** @param unit    pointer to unit.
** @param pos     map tile position of construction.
** @param what    pointer to unit-type of the building.
** @param flush   Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandBuildBuilding(CUnit &unit, const Vec2i &pos, CUnitType &what, int flush)
void SendCommandBuildBuilding(CUnit &unit, const Vec2i &pos, CUnitType &what, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("build", &unit, flush, pos.x, pos.y, NoUnitP, what.Ident.c_str(), -1);
		//Wyrmgus start
//		CommandBuildBuilding(unit, pos, what, flush);
		CommandBuildBuilding(unit, pos, what, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandBuild, unit, pos.x, pos.y, NoUnitP, &what, flush);
	}
}

/**
**  Send command: Cancel this building construction.
**
**  @param unit  pointer to unit.
*/
void SendCommandDismiss(CUnit &unit, bool salvage)
{
	// FIXME: currently unit and worker are same?
	if (!IsNetworkGame()) {
		CommandLog("dismiss", &unit, FlushCommands, -1, -1, nullptr, nullptr, -1);
		CommandDismiss(unit, salvage);
	} else {
		NetworkSendCommand(MessageCommandDismiss, unit, salvage, 0, nullptr, 0, FlushCommands);
	}
}

/**
**  Send command: Unit harvests a location (wood for now).
**
** @param unit     pointer to unit.
** @param pos      map tile position where to harvest.
** @param flush    Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush)
void SendCommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush, int z)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		CommandLog("resource-loc", &unit, flush, pos.x, pos.y, NoUnitP, nullptr, -1);
		//Wyrmgus start
//		CommandResourceLoc(unit, pos, flush);
		CommandResourceLoc(unit, pos, flush, z);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandResourceLoc, unit, pos.x, pos.y, NoUnitP, 0, flush);
	}
}

/**
** Send command: Unit harvest resources
**
** @param unit    pointer to unit.
** @param dest    pointer to destination (oil-platform,gold mine).
** @param flush   Flag flush all pending commands.
*/
void SendCommandResource(CUnit &unit, CUnit &dest, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("resource", &unit, flush, -1, -1, &dest, nullptr, -1);
		CommandResource(unit, dest, flush);
	} else {
		NetworkSendCommand(MessageCommandResource, unit, 0, 0, &dest, 0, flush);
	}
}

/**
** Send command: Unit return goods.
**
** @param unit    pointer to unit.
** @param goal    pointer to destination of the goods. (null=search best)
** @param flush   Flag flush all pending commands.
*/
void SendCommandReturnGoods(CUnit &unit, CUnit *goal, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("return", &unit, flush, -1, -1, goal, nullptr, -1);
		CommandReturnGoods(unit, goal, flush);
	} else {
		NetworkSendCommand(MessageCommandReturn, unit, 0, 0, goal, 0, flush);
	}
}

/**
** Send command: Building/unit train new unit.
**
** @param unit    pointer to unit.
** @param what    pointer to unit-type of the unit to be trained.
** @param flush   Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandTrainUnit(CUnit &unit, CUnitType &what, int flush)
void SendCommandTrainUnit(CUnit &unit, CUnitType &what, int player, int flush)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		//Wyrmgus start
//		CommandLog("train", &unit, flush, -1, -1, NoUnitP, what.Ident.c_str(), -1);
//		CommandTrainUnit(unit, what, flush);
		CommandLog("train", &unit, flush, -1, -1, NoUnitP, what.Ident.c_str(), player);
		CommandTrainUnit(unit, what, player, flush);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		NetworkSendCommand(MessageCommandTrain, unit, 0, 0, NoUnitP, &what, flush);
		NetworkSendCommand(MessageCommandTrain, unit, player, 0, NoUnitP, &what, flush);
		//Wyrmgus end
	}
}

/**
** Send command: Cancel training.
**
** @param unit    Pointer to unit.
** @param slot    Slot of training queue to cancel.
** @param type    Unit-type of unit to cancel.
*/
void SendCommandCancelTraining(CUnit &unit, int slot, const CUnitType *type)
{
	if (!IsNetworkGame()) {
		CommandLog("cancel-train", &unit, FlushCommands, -1, -1, NoUnitP,
				   type ? type->Ident.c_str() : nullptr, slot);
		CommandCancelTraining(unit, slot, type);
	} else {
		NetworkSendCommand(MessageCommandCancelTrain, unit, slot, 0, NoUnitP,
						   type, FlushCommands);
	}
}

/**
** Send command: Building starts upgrading to.
**
** @param unit     pointer to unit.
** @param what     pointer to unit-type of the unit upgrade.
** @param flush    Flag flush all pending commands.
*/
void SendCommandUpgradeTo(CUnit &unit, CUnitType &what, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("upgrade-to", &unit, flush, -1, -1, NoUnitP, what.Ident.c_str(), -1);
		CommandUpgradeTo(unit, what, flush);
	} else {
		NetworkSendCommand(MessageCommandUpgrade, unit, 0, 0, NoUnitP, &what, flush);
	}
}

/**
** Send command: Cancel building upgrading to.
**
** @param unit  Pointer to unit.
*/
void SendCommandCancelUpgradeTo(CUnit &unit)
{
	if (!IsNetworkGame()) {
		CommandLog("cancel-upgrade-to", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
		CommandCancelUpgradeTo(unit);
	} else {
		NetworkSendCommand(MessageCommandCancelUpgrade, unit,
						   0, 0, NoUnitP, nullptr, FlushCommands);
	}
}

//Wyrmgus start
/**
** Send command: Unit starts upgrading to.
**
** @param unit     pointer to unit.
** @param what     pointer to unit-type of the unit upgrade.
** @param flush    Flag flush all pending commands.
*/
void SendCommandTransformInto(CUnit &unit, CUnitType &what, int flush)
{
	if (!IsNetworkGame()) {
		CommandLog("transform-into", &unit, flush, -1, -1, NoUnitP, what.Ident.c_str(), -1);
		CommandTransformIntoType(unit, what);
	} else {
		NetworkSendCommand(MessageCommandUpgrade, unit, 2, 0, NoUnitP, &what, flush); //use X as a way to mark that this is a transformation and not an upgrade
	}
}
//Wyrmgus end

/**
** Send command: Building/unit research.
**
** @param unit     pointer to unit.
** @param what     research-type of the research.
** @param flush    Flag flush all pending commands.
*/
//Wyrmgus start
//void SendCommandResearch(CUnit &unit, CUpgrade &what, int flush)
void SendCommandResearch(CUnit &unit, CUpgrade &what, int player, int flush)
//Wyrmgus end
{
	if (!IsNetworkGame()) {
		//Wyrmgus start
//		CommandLog("research", &unit, flush, -1, -1, NoUnitP, what.Ident.c_str(), -1);
//		CommandResearch(unit, what, flush);
		CommandLog("research", &unit, flush, -1, -1, NoUnitP, what.Ident.c_str(), player);
		CommandResearch(unit, what, player, flush);
		//Wyrmgus end
	} else {
		NetworkSendCommand(MessageCommandResearch, unit,
						   //Wyrmgus start
//						   what.ID, 0, NoUnitP, nullptr, flush);
						   what.ID, player, NoUnitP, nullptr, flush);
						   //Wyrmgus end
	}
}

/**
** Send command: Cancel Building/unit research.
**
** @param unit pointer to unit.
*/
void SendCommandCancelResearch(CUnit &unit)
{
	if (!IsNetworkGame()) {
		CommandLog("cancel-research", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
		CommandCancelResearch(unit);
	} else {
		NetworkSendCommand(MessageCommandCancelResearch, unit,
						   0, 0, NoUnitP, nullptr, FlushCommands);
	}
}

//Wyrmgus start
/**
** Send command: Unit learn ability.
**
** @param unit     pointer to unit.
** @param what     upgrade-type of the ability.
*/
void SendCommandLearnAbility(CUnit &unit, CUpgrade &what)
{
	if (!IsNetworkGame()) {
		CommandLog("learn-ability", &unit, 0, -1, -1, NoUnitP, what.Ident.c_str(), -1);
		CommandLearnAbility(unit, what);
	} else {
		NetworkSendCommand(MessageCommandLearnAbility, unit,
						   what.ID, 0, NoUnitP, nullptr, 0);
	}
}
//Wyrmgus end

/**
** Send command: Unit spell cast on position/unit.
**
** @param unit      pointer to unit.
** @param pos       map tile position where to cast spell.
** @param dest      Cast spell on unit (if exist).
** @param spellid   Spell type id.
** @param flush     Flag flush all pending commands.
*/
void SendCommandSpellCast(CUnit &unit, const Vec2i &pos, CUnit *dest, int spellid, int flush, int z)
{
	if (!IsNetworkGame()) {
		CommandLog("spell-cast", &unit, flush, pos.x, pos.y, dest, nullptr, spellid);
		CommandSpellCast(unit, pos, dest, *CSpell::Spells[spellid], flush, z);
	} else {
		NetworkSendCommand(MessageCommandSpellCast + spellid,
						   unit, pos.x, pos.y, dest, nullptr, flush);
	}
}

/**
** Send command: Unit auto spell cast.
**
** @param unit      pointer to unit.
** @param spellid   Spell type id.
** @param on        1 for auto cast on, 0 for off.
*/
void SendCommandAutoSpellCast(CUnit &unit, int spellid, int on)
{
	if (!IsNetworkGame()) {
		CommandLog("auto-spell-cast", &unit, FlushCommands, on, -1, NoUnitP, nullptr, spellid);
		CommandAutoSpellCast(unit, spellid, on);
	} else {
		NetworkSendCommand(MessageCommandSpellCast + spellid,
						   unit, on, -1, NoUnitP, nullptr, FlushCommands);
	}
}

/**
** Send command: Diplomacy changed.
**
** @param player     Player which changes his state.
** @param state      New diplomacy state.
** @param opponent   Opponent.
*/
void SendCommandDiplomacy(int player, int state, int opponent)
{
	if (!IsNetworkGame()) {
		switch (state) {
			case DiplomacyNeutral:
				CommandLog("diplomacy", NoUnitP, 0, player, opponent,
						   NoUnitP, "neutral", -1);
				break;
			case DiplomacyAllied:
				CommandLog("diplomacy", NoUnitP, 0, player, opponent,
						   NoUnitP, "allied", -1);
				break;
			case DiplomacyEnemy:
				CommandLog("diplomacy", NoUnitP, 0, player, opponent,
						   NoUnitP, "enemy", -1);
				break;
			//Wyrmgus start
			case DiplomacyOverlord:
				CommandLog("diplomacy", NoUnitP, 0, player, opponent,
						   NoUnitP, "overlord", -1);
				break;
			case DiplomacyVassal:
				CommandLog("diplomacy", NoUnitP, 0, player, opponent,
						   NoUnitP, "vassal", -1);
				break;
			//Wyrmgus end
			case DiplomacyCrazy:
				CommandLog("diplomacy", NoUnitP, 0, player, opponent,
						   NoUnitP, "crazy", -1);
				break;
		}
		CommandDiplomacy(player, state, opponent);
	} else {
		NetworkSendExtendedCommand(ExtendedMessageDiplomacy,
								   -1, player, state, opponent, 0);
	}
}

/**
** Send command: Shared vision changed.
**
** @param player     Player which changes his state.
** @param state      New shared vision state.
** @param opponent   Opponent.
*/
void SendCommandSharedVision(int player, bool state, int opponent)
{
	if (!IsNetworkGame()) {
		if (state == false) {
			CommandLog("shared-vision", NoUnitP, 0, player, opponent,
					   NoUnitP, "0", -1);
		} else {
			CommandLog("shared-vision", NoUnitP, 0, player, opponent,
					   NoUnitP, "1", -1);
		}
		CommandSharedVision(player, state, opponent);
	} else {
		NetworkSendExtendedCommand(ExtendedMessageSharedVision,
								   -1, player, state, opponent, 0);
	}
}

//Wyrmgus start
/**
** Send command: Faction changed.
**
** @param player     Player which changes his faction.
** @param faction    New faction.
*/
void SendCommandSetFaction(int player, int faction)
{
	if (!IsNetworkGame()) {
		//FIXME: should add log of faction change here
		if (faction != -1) {
			CPlayer::Players[player]->SetFaction(PlayerRaces.Factions[faction]);
		} else {
			CPlayer::Players[player]->SetFaction(nullptr);
		}
	} else {
		NetworkSendExtendedCommand(ExtendedMessageSetFaction, -1, player, faction, 0, 0);
	}
}
//Wyrmgus end

/**
** Send command: Toggle resource autosell.
**
** @param player     Player which toggles the resource autosell.
** @param resource   The resource.
*/
void SendCommandAutosellResource(int player, int resource)
{
	if (!IsNetworkGame()) {
		CPlayer::Players[player]->AutosellResource(resource);
	} else {
		NetworkSendExtendedCommand(ExtendedMessageAutosellResource, -1, player, resource, 0, 0);
	}
}

//----------------------------------------------------------------------------
// Parse the message, from the network.
//----------------------------------------------------------------------------

/**@name parse */
//@{

/**
** Execute a command (from network).
**
** @param msgnr    Network message type
** @param unum     Unit number (slot) that receive the command.
** @param x        optional X map position.
** @param y        optional y map position.
** @param dstnr    optional destination unit.
*/
void ExecCommand(unsigned char msgnr, UnitRef unum,
				 unsigned short x, unsigned short y, UnitRef dstnr)
{
	CUnit &unit = UnitManager.GetSlotUnit(unum);
	const Vec2i pos(x, y);
	const int arg1 = x;
	const int arg2 = y;
	//
	// Check if unit is already killed?
	//
	if (unit.Destroyed) {
		DebugPrint(" destroyed unit skipping %d\n" _C_ UnitNumber(unit));
		return;
	}
	Assert(unit.Type);

	const int status = (msgnr & 0x80) >> 7;
	// Note: destroyed destination unit is handled by the action routines.

	switch (msgnr & 0x7F) {
		case MessageSync:
			return;
		case MessageQuit:
			return;
		case MessageChat:
			return;

		case MessageCommandStop:
			CommandLog("stop", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
			CommandStopUnit(unit);
			break;
		case MessageCommandStand:
			CommandLog("stand-ground", &unit, status, -1, -1, NoUnitP, nullptr, -1);
			CommandStandGround(unit, status);
			break;
		case MessageCommandDefend: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("defend", &unit, status, -1, -1, &dest, nullptr, -1);
				CommandDefend(unit, dest, status);
			}
			break;
		}
		case MessageCommandFollow: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("follow", &unit, status, -1, -1, &dest, nullptr, -1);
				CommandFollow(unit, dest, status);
			}
			break;
		}
		case MessageCommandMove:
			//Wyrmgus start
//			CommandLog("move", &unit, status, pos.x, pos.y, NoUnitP, nullptr, -1);
//			CommandMove(unit, pos, status);
			if (!unit.CanMove()) { //FIXME: find better way to identify whether the unit should move or set a rally point
				CommandLog("rally-point", &unit, status, pos.x, pos.y, NoUnitP, nullptr, -1);
				CommandRallyPoint(unit, pos);
			} else {
				CommandLog("move", &unit, status, pos.x, pos.y, NoUnitP, nullptr, -1);
				CommandMove(unit, pos, status);
			}
			//Wyrmgus end
			break;
		//Wyrmgus start
		case MessageCommandPickUp: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("pick-up", &unit, status, -1, -1, &dest, nullptr, -1);
				CommandPickUp(unit, dest, status);
			}
			break;
		}
		//Wyrmgus end
		case MessageCommandRepair: {
			CUnit *dest = NoUnitP;
			if (dstnr != (unsigned short)0xFFFF) {
				dest = &UnitManager.GetSlotUnit(dstnr);
				Assert(dest && dest->Type);
			}
			CommandLog("repair", &unit, status, pos.x, pos.y, dest, nullptr, -1);
			CommandRepair(unit, pos, dest, status);
			break;
		}
		case MessageCommandAutoRepair:
			CommandLog("auto-repair", &unit, status, arg1, arg2, NoUnitP, nullptr, 0);
			CommandAutoRepair(unit, arg1);
			break;
		case MessageCommandAttack: {
			CUnit *dest = NoUnitP;
			if (dstnr != (unsigned short)0xFFFF) {
				dest = &UnitManager.GetSlotUnit(dstnr);
				Assert(dest && dest->Type);
			}
			CommandLog("attack", &unit, status, pos.x, pos.y, dest, nullptr, -1);
			CommandAttack(unit, pos, dest, status);
			break;
		}
		case MessageCommandGround:
			CommandLog("attack-ground", &unit, status, pos.x, pos.y, NoUnitP, nullptr, -1);
			CommandAttackGround(unit, pos, status);
			break;
		//Wyrmgus start
		case MessageCommandUse: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("use", &unit, status, -1, -1, &dest, nullptr, -1);
				CommandUse(unit, dest, status);
			}
			break;
		}
		case MessageCommandTrade: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("trade", &unit, status, -1, -1, &dest, nullptr, -1);
				CommandTrade(unit, dest, status);
			}
			break;
		}
		//Wyrmgus end
		case MessageCommandPatrol:
			CommandLog("patrol", &unit, status, pos.x, pos.y, NoUnitP, nullptr, -1);
			CommandPatrolUnit(unit, pos, status);
			break;
		case MessageCommandBoard: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("board", &unit, status, arg1, arg2, &dest, nullptr, -1);
				CommandBoard(unit, dest, status);
			}
			break;
		}
		case MessageCommandUnload: {
			CUnit *dest = nullptr;
			if (dstnr != (unsigned short)0xFFFF) {
				dest = &UnitManager.GetSlotUnit(dstnr);
				Assert(dest && dest->Type);
			}
			CommandLog("unload", &unit, status, pos.x, pos.y, dest, nullptr, -1);
			CommandUnload(unit, pos, dest, status);
			break;
		}
		case MessageCommandBuild:
			CommandLog("build", &unit, status, pos.x, pos.y, NoUnitP, UnitTypes[dstnr]->Ident.c_str(), -1);
			CommandBuildBuilding(unit, pos, *UnitTypes[dstnr], status);
			break;
		case MessageCommandDismiss:
			CommandLog("dismiss", &unit, FlushCommands, arg1, -1, nullptr, nullptr, -1);
			CommandDismiss(unit, arg1 > 0);
			break;
		case MessageCommandResourceLoc:
			CommandLog("resource-loc", &unit, status, pos.x, pos.y, NoUnitP, nullptr, -1);
			CommandResourceLoc(unit, pos, status);
			break;
		case MessageCommandResource: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("resource", &unit, status, -1, -1, &dest, nullptr, -1);
				CommandResource(unit, dest, status);
			}
			break;
		}
		case MessageCommandReturn: {
			CUnit *dest = (dstnr != (unsigned short)0xFFFF) ? &UnitManager.GetSlotUnit(dstnr) : nullptr;
			CommandLog("return", &unit, status, -1, -1, dest, nullptr, -1);
			CommandReturnGoods(unit, dest, status);
			break;
		}
		case MessageCommandTrain:
			//Wyrmgus start
//			CommandLog("train", &unit, status, -1, -1, NoUnitP, UnitTypes[dstnr]->Ident.c_str(), -1);
//			CommandTrainUnit(unit, *UnitTypes[dstnr], status);
			CommandLog("train", &unit, status, -1, -1, NoUnitP, UnitTypes[dstnr]->Ident.c_str(), arg1); // use X as a way to mark the player
			CommandTrainUnit(unit, *UnitTypes[dstnr], arg1, status);
			//Wyrmgus end
			break;
		case MessageCommandCancelTrain:
			// We need (short)x for the last slot -1
			if (dstnr != (unsigned short)0xFFFF) {
				CommandLog("cancel-train", &unit, FlushCommands, -1, -1, NoUnitP,
						   UnitTypes[dstnr]->Ident.c_str(), (short)x);
				CommandCancelTraining(unit, (short)x, UnitTypes[dstnr]);
			} else {
				CommandLog("cancel-train", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, (short)x);
				CommandCancelTraining(unit, (short)x, nullptr);
			}
			break;
		case MessageCommandUpgrade:
			//Wyrmgus start
			/*
			CommandLog("upgrade-to", &unit, status, -1, -1, NoUnitP,
					   UnitTypes[dstnr]->Ident.c_str(), -1);
			CommandUpgradeTo(unit, *UnitTypes[dstnr], status);
			break;
			*/
			if (arg1 == 2) { //use X as a way to mark whether this is an upgrade or a transformation
				CommandLog("transform-into", &unit, status, -1, -1, NoUnitP,
						   UnitTypes[dstnr]->Ident.c_str(), -1);
				CommandTransformIntoType(unit, *UnitTypes[dstnr]);
			} else {
				CommandLog("upgrade-to", &unit, status, -1, -1, NoUnitP,
						   UnitTypes[dstnr]->Ident.c_str(), -1);
				CommandUpgradeTo(unit, *UnitTypes[dstnr], status);
			}
			break;
			//Wyrmgus end
		case MessageCommandCancelUpgrade:
			CommandLog("cancel-upgrade-to", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
			CommandCancelUpgradeTo(unit);
			break;
		case MessageCommandResearch:
			CommandLog("research", &unit, status, -1, -1, NoUnitP,
					   //Wyrmgus start
//					   AllUpgrades[arg1]->Ident.c_str(), -1);
					   AllUpgrades[arg1]->Ident.c_str(), arg2);
					   //Wyrmgus end
			//Wyrmgus start
//			CommandResearch(unit, *AllUpgrades[arg1], status);
			CommandResearch(unit, *AllUpgrades[arg1], arg2, status);
			//Wyrmgus end
			break;
		case MessageCommandCancelResearch:
			CommandLog("cancel-research", &unit, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
			CommandCancelResearch(unit);
			break;
		//Wyrmgus start
		case MessageCommandLearnAbility:
			CommandLog("learn-ability", &unit, 0, -1, -1, NoUnitP,
					   AllUpgrades[arg1]->Ident.c_str(), -1);
			CommandLearnAbility(unit, *AllUpgrades[arg1]);
			break;
		case MessageCommandQuest: {
			CommandLog("quest", &unit, 0, 0, 0, NoUnitP, Quests[arg1]->Ident.c_str(), -1);
			CommandQuest(unit, Quests[arg1]);
			break;
		}
		case MessageCommandBuy: {
			if (dstnr != (unsigned short)0xFFFF) {
				CUnit &dest = UnitManager.GetSlotUnit(dstnr);
				Assert(dest.Type);
				CommandLog("buy", &unit, 0, -1, -1, &dest, nullptr, arg1);
				CommandBuy(unit, &dest, arg1);
			}
			break;
		}
		case MessageCommandProduceResource: {
			CommandLog("produce-resource", &unit, 0, 0, -1, NoUnitP, nullptr, arg1);
			CommandProduceResource(unit, arg1);
			break;
		}
		case MessageCommandSellResource: {
			CommandLog("sell-resource", &unit, 0, arg1, -1, NoUnitP, nullptr, arg2);
			CommandSellResource(unit, arg1, arg2);
			break;
		}
		case MessageCommandBuyResource: {
			CommandLog("buy-resource", &unit, 0, arg1, -1, NoUnitP, nullptr, arg2);
			CommandBuyResource(unit, arg1, arg2);
			break;
		}
		//Wyrmgus end
		default: {
			int id = (msgnr & 0x7f) - MessageCommandSpellCast;
			if (arg2 != (unsigned short)0xFFFF) {
				CUnit *dest = nullptr;
				if (dstnr != (unsigned short)0xFFFF) {
					dest = &UnitManager.GetSlotUnit(dstnr);
					Assert(dest && dest->Type);
				}
				CommandLog("spell-cast", &unit, status, pos.x, pos.y, dest, nullptr, id);
				CommandSpellCast(unit, pos, dest, *CSpell::Spells[id], status);
			} else {
				CommandLog("auto-spell-cast", &unit, status, arg1, -1, NoUnitP, nullptr, id);
				CommandAutoSpellCast(unit, id, arg1);
			}
			break;
		}
	}
}

static const char *GetDiplomacyName(enum _diplomacy_ e)
{
	Assert(int(e) < 4);
	const char *diplomacyNames[] = {"allied", "neutral", "enemy", "crazy"};

	return diplomacyNames[int(e)];
}

/**
** Execute an extended command (from network).
**
** @param type     Network extended message type
** @param status   Bit 7 of message type
** @param arg1     Messe argument 1
** @param arg2     Messe argument 2
** @param arg3     Messe argument 3
** @param arg4     Messe argument 4
*/
void ExecExtendedCommand(unsigned char type, int status,
						 unsigned char arg1, unsigned short arg2, unsigned short arg3,
						 unsigned short arg4)
{
	// Note: destroyed units are handled by the action routines.

	switch (type) {
		case ExtendedMessageDiplomacy: {
			const char *diplomacyName = GetDiplomacyName(_diplomacy_(arg3));
			CommandLog("diplomacy", NoUnitP, 0, arg2, arg4, NoUnitP, diplomacyName, -1);
			CommandDiplomacy(arg2, arg3, arg4);
			break;
		}
		case ExtendedMessageSharedVision:
			if (arg3 == 0) {
				CommandLog("shared-vision", NoUnitP, 0, arg2, arg4, NoUnitP, "0", -1);
			} else {
				CommandLog("shared-vision", NoUnitP, 0, arg2, arg4, NoUnitP, "1", -1);
			}
			CommandSharedVision(arg2, arg3 ? true : false, arg4);
			break;
		//Wyrmgus start
		case ExtendedMessageSetFaction: {
			//FIXME: should add log for faction change here
			CPlayer::Players[arg2]->SetFaction(PlayerRaces.Factions[arg3]);
			break;
		}
		case ExtendedMessageAutosellResource: {
			CPlayer::Players[arg2]->AutosellResource(arg3);
			break;
		}
		//Wyrmgus end
		default:
			DebugPrint("Unknown extended message %u/%s %u %u %u %u\n" _C_
					   type _C_ status ? "flush" : "-" _C_
					   arg1 _C_ arg2 _C_ arg3 _C_ arg4);
			break;
	}
}
