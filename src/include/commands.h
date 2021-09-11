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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#pragma once

#include "vec2i.h"

class CFile;
class CPlayer;
class CUnit;
class CUpgrade;

namespace wyrmgus {
	class dynasty;
	class landmass;
	class quest;
	class resource;
	class site;
	class spell;
	class unit_type;
	enum class diplomacy_state;
}

/*----------------------------------------------------------------------------
--  Commands: in command.c
----------------------------------------------------------------------------*/

/**
**  This function gives a unit a new command. If the command is given
**  by the user the function with Send prefix should be used.
*/

/// Prepare command quit
extern void CommandQuit(int player);
/// Prepare command stop
extern void CommandStopUnit(CUnit &unit);
/// Prepare command stand ground
extern void CommandStandGround(CUnit &unit, int flush);
/// Prepare command defend
extern void CommandDefend(CUnit &unit, CUnit &dest, int flush);
/// Prepare command follow
extern void CommandFollow(CUnit &unit, CUnit &dest, int flush);
/// Prepare command move
//Wyrmgus start
//extern void CommandMove(CUnit &unit, const Vec2i &pos, int flush);
extern void CommandMove(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
//Wyrmgus start
/// Prepare command rally point
extern void CommandRallyPoint(CUnit &unit, const Vec2i &pos, int z = 0);
/// Prepare command pick up
extern void CommandPickUp(CUnit &unit, CUnit &dest, int flush);
/// Prepare command quest
extern void CommandQuest(CUnit &unit, wyrmgus::quest *quest);
/// Prepare command buy
extern void CommandBuy(CUnit &unit, CUnit *sold_unit, int player);
/// Prepare command produce resource
extern void CommandProduceResource(CUnit &unit, const wyrmgus::resource *resource);
/// Prepare command sell resource
extern void CommandSellResource(CUnit &unit, int resource, int player);
/// Prepare command buy resource
extern void CommandBuyResource(CUnit &unit, int resource, int player);
//Wyrmgus end
/// Prepare command repair
//Wyrmgus start
//extern void CommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush);
extern void CommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush, int z = 0);
//Wyrmgus end
/// Send auto repair command
extern void CommandAutoRepair(CUnit &unit, int on);
/// Prepare command attack
//Wyrmgus start
//extern void CommandAttack(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush);
extern void CommandAttack(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush, int z = 0);
//Wyrmgus end
/// Prepare command attack ground
//Wyrmgus start
//extern void CommandAttackGround(CUnit &unit, const Vec2i &pos, int flush);
extern void CommandAttackGround(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
//Wyrmgus start
/// Prepare command use
extern void CommandUse(CUnit &unit, CUnit &dest, int flush, bool reach_layer = true);
/// Prepare command trade
extern void CommandTrade(CUnit &unit, CUnit &dest, int flush, bool reach_layer = true);
//Wyrmgus end
/// Prepare command patrol
//Wyrmgus start
//extern void CommandPatrolUnit(CUnit &unit, const Vec2i &pos, int flush);
extern void CommandPatrolUnit(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
/// Prepare command board
extern void CommandBoard(CUnit &unit, CUnit &dest, int flush);
/// Prepare command unload
//Wyrmgus start
//extern void CommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush);
extern void CommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush, int z = 0, const landmass *landmass = nullptr);
//Wyrmgus end
/// Prepare command build
extern void CommandBuildBuilding(CUnit &unit, const Vec2i &pos, const wyrmgus::unit_type &, int flush, int z = 0, const wyrmgus::site *settlement = nullptr);
/// Prepare command dismiss
extern void CommandDismiss(CUnit &unit, bool salvage = false);
/// Prepare command resource location
//Wyrmgus start
//extern void CommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush);
extern void CommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
/// Prepare command resource
extern void CommandResource(CUnit &unit, CUnit &dest, int flush);
/// Prepare command return
extern void CommandReturnGoods(CUnit &unit, CUnit *depot, int flush);
/// Prepare command train
extern void CommandTrainUnit(CUnit &unit, const wyrmgus::unit_type &what, int player, int flush);
/// Prepare command cancel training
extern void CommandCancelTraining(CUnit &unit, int slot, const wyrmgus::unit_type *type);
/// Prepare command upgrade to
extern void CommandUpgradeTo(CUnit &unit, const wyrmgus::unit_type &what, int flush);
/// immediate transforming into type.
extern void CommandTransformIntoType(CUnit &unit, wyrmgus::unit_type &type);
/// Prepare command cancel upgrade to
extern void CommandCancelUpgradeTo(CUnit &unit);
/// Prepare command research
extern void CommandResearch(CUnit &unit, const CUpgrade &what, CPlayer *player, int flush);
/// Prepare command cancel research
extern void CommandCancelResearch(CUnit &unit);
//Wyrmgus start
/// Prepare command learn ability
extern void CommandLearnAbility(CUnit &unit, CUpgrade &what);
//Wyrmgus end
/// Prepare command spellcast
extern void CommandSpellCast(CUnit &unit, const Vec2i &pos, CUnit *dest, const wyrmgus::spell &spell, int flush, int z = 0);
/// Prepare command auto spellcast
extern void CommandAutoSpellCast(CUnit &unit, const wyrmgus::spell *spell, const bool on);
/// Prepare diplomacy command
extern void CommandDiplomacy(const int player_index, const diplomacy_state state, const int other_player_index);
/// Prepare shared vision command
extern void CommandSharedVision(const int player_index, const bool state, const int other_player_index);

/*
**  The send command functions sends a command, if needed over the
**  Network, this is only for user commands. Automatic reactions which
**  are on all computers equal, should use the functions without Send.
*/

/**
**  Unit references over network, or for memory saving.
*/
typedef unsigned short UnitRef;

/// Send stop command
extern void SendCommandStopUnit(CUnit &unit);
/// Send stand ground command
extern void SendCommandStandGround(CUnit &unit, int flush);
/// Send defend command
extern void SendCommandDefend(CUnit &unit, CUnit &dest, int flush);
/// Send follow command
extern void SendCommandFollow(CUnit &unit, CUnit &dest, int flush);
/// Send move command
//Wyrmgus start
//extern void SendCommandMove(CUnit &unit, const Vec2i &pos, int flush);
extern void SendCommandMove(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
//Wyrmgus start
/// Send rally point command
extern void SendCommandRallyPoint(CUnit &unit, const Vec2i &pos, int z = 0);
/// Send quest command
extern void SendCommandQuest(CUnit &unit, wyrmgus::quest *quest);
/// Send buy command
extern void SendCommandBuy(CUnit &unit, CUnit *sold_unit, int player);
/// Send produce resource command
extern void SendCommandProduceResource(CUnit &unit, const wyrmgus::resource *resource);
/// Send sell resource command
extern void SendCommandSellResource(CUnit &unit, int resource, int player);
/// Send buy resource command
extern void SendCommandBuyResource(CUnit &unit, int resource, int player);
/// Send pick up command
extern void SendCommandPickUp(CUnit &unit, CUnit &dest, int flush);
//Wyrmgus end
/// Send repair command
//Wyrmgus start
//extern void SendCommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush);
extern void SendCommandRepair(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush, int z = 0);
//Wyrmgus end
/// Send auto repair command
extern void SendCommandAutoRepair(CUnit &unit, int on);
/// Send attack command
//Wyrmgus start
//extern void SendCommandAttack(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush);
extern void SendCommandAttack(CUnit &unit, const Vec2i &pos, CUnit *dest, int flush, int z = 0);
//Wyrmgus end
/// Send attack ground command
//Wyrmgus start
//extern void SendCommandAttackGround(CUnit &unit, const Vec2i &pos, int flush);
extern void SendCommandAttackGround(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
//Wyrmgus start
/// Send use command
extern void SendCommandUse(CUnit &unit, CUnit &dest, int flush);
/// Send trade command
extern void SendCommandTrade(CUnit &unit, CUnit &dest, int flush);
//Wyrmgus end
/// Send patrol command
//Wyrmgus start
//extern void SendCommandPatrol(CUnit &unit, const Vec2i &pos, int flush);
extern void SendCommandPatrol(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//WYrmgus end
/// Send board command
extern void SendCommandBoard(CUnit &unit, CUnit &dest, int flush);
/// Send unload command
//Wyrmgus start
//extern void SendCommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush);
extern void SendCommandUnload(CUnit &unit, const Vec2i &pos, CUnit *what, int flush, int z = 0);
//Wyrmgus end
/// Send build building command
extern void SendCommandBuildBuilding(CUnit &unit, const Vec2i &pos, const wyrmgus::unit_type &what, int flush, int z = 0);
/// Send cancel building command
extern void SendCommandDismiss(CUnit &unit, bool salvage = false);
/// Send harvest location command
//Wyrmgus start
//extern void SendCommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush);
extern void SendCommandResourceLoc(CUnit &unit, const Vec2i &pos, int flush, int z = 0);
//Wyrmgus end
/// Send harvest command
extern void SendCommandResource(CUnit &unit, CUnit &dest, int flush);
/// Send return goods command
extern void SendCommandReturnGoods(CUnit &unit, CUnit *dest, int flush);
/// Send train command
extern void SendCommandTrainUnit(CUnit &unit, const wyrmgus::unit_type &what, int player, int flush);
/// Send cancel training command
extern void SendCommandCancelTraining(CUnit &unit, int slot, const wyrmgus::unit_type *type);
/// Send upgrade to command
extern void SendCommandUpgradeTo(CUnit &unit, const wyrmgus::unit_type &what, int flush);
/// Send cancel upgrade to command
extern void SendCommandCancelUpgradeTo(CUnit &unit);
//Wyrmgus start
/// Send transform into command
extern void SendCommandTransformInto(CUnit &unit, wyrmgus::unit_type &what, int flush);
//Wyrmgus end
/// Send research command
extern void SendCommandResearch(CUnit &unit, const CUpgrade &what, int player, int flush);
/// Send cancel research command
extern void SendCommandCancelResearch(CUnit &unit);
//Wyrmgus start
/// Send learn ability command
extern void SendCommandLearnAbility(CUnit &unit, CUpgrade &what);
//Wyrmgus end
/// Send spell cast command
//Wyrmgus start
//extern void SendCommandSpellCast(CUnit &unit, const Vec2i &pos, CUnit *dest, int spellid, int flush);
extern void SendCommandSpellCast(CUnit &unit, const Vec2i &pos, CUnit *dest, int spellid, int flush, int z = 0);
//Wyrmgus end
/// Send auto spell cast command
extern void SendCommandAutoSpellCast(CUnit &unit, const wyrmgus::spell *spell, const bool on);
/// Send diplomacy command
extern void SendCommandDiplomacy(const int player, const diplomacy_state state, const int opponent);
/// Send shared vision command
extern void SendCommandSharedVision(int player, bool state, int opponent);
extern void SendCommandSetFaction(int player, int faction);
extern void SendCommandSetDynasty(CPlayer *player, const wyrmgus::dynasty *dynasty);
extern void SendCommandAutosellResource(int player, int resource);

/// Execute a command (from network).
extern void ExecCommand(unsigned char type, UnitRef unum, unsigned short x,
						unsigned short y, UnitRef dest);
/// Execute an extended command (from network).
extern void ExecExtendedCommand(unsigned char type, int status, unsigned char arg1,
								unsigned short arg2, unsigned short arg3,
								unsigned short arg4);

constexpr int FlushCommands = 1;          /// Flush commands in queue
