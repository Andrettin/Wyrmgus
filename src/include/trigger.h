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
/**@name trigger.h - The trigger header file. */
//
//      (c) Copyright 2002-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CDependency;
class CFaction;
class CFile;
class CTriggerEffect;
class CUnit;
class CUnitType;
class CUpgrade;
class LuaCallback;
struct lua_State;

/**
**  Timer structure
*/
class CTimer
{
public:
	void Reset()
	{
		Init = false;
		Running = false;
		Increasing = false;
		Cycles = 0;
		LastUpdate = 0;
	}

	bool Init = false;				/// timer is initialized
	bool Running = false;			/// timer is running
	bool Increasing = false;		/// increasing or decreasing
	long Cycles = 0;				/// current value in game cycles
	unsigned long LastUpdate = 0;	/// GameCycle of last update
};

class CTrigger : public CDataType
{
public:
	enum class TriggerType
	{
		GlobalTrigger = 0, //checked once
		PlayerTrigger //checked for each player
	};

	static CTrigger *GetTrigger(const std::string &ident, const bool should_find = true);
	static CTrigger *GetOrAddTrigger(const std::string &ident);
	static void ClearTriggers();		/// Cleanup the trigger module
	static void InitActiveTriggers();	/// Setup triggers
	static void ClearActiveTriggers();

	static std::vector<CTrigger *> Triggers;
	static std::vector<CTrigger *> ActiveTriggers; //triggers that are active for the current game
	static std::map<std::string, CTrigger *> TriggersByIdent;
	static std::vector<std::string> DeactivatedTriggers;
	static unsigned int CurrentTriggerId;

	~CTrigger();
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	TriggerType Type = TriggerType::GlobalTrigger;
	bool Local = false;
	bool OnlyOnce = false;				/// Whether the trigger should occur only once in a game
	bool CampaignOnly = false;			/// Whether the trigger should only occur in the campaign mode
	LuaCallback *Conditions = nullptr;
	LuaCallback *Effects = nullptr;
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;
	std::vector<CTriggerEffect *> TriggerEffects;
};

#define ANY_UNIT ((const CUnitType *)0)
#define ALL_FOODUNITS ((const CUnitType *)-1)
#define ALL_BUILDINGS ((const CUnitType *)-2)

/**
**  Data to referer game info when game running.
*/
struct TriggerDataType {
	CUnit *Attacker;  /// Unit which send the missile.
	CUnit *Defender;  /// Unit which is hit by missile.
	CUnit *Active;    /// Unit which is selected or else under cursor unit.
	//Wyrmgus start
	CUnit *Unit;	  /// Unit used in trigger
	//Wyrmgus end
	CUnitType *Type;  /// Type used in trigger;
	//Wyrmgus start
	CUpgrade *Upgrade; /// Upgrade used in trigger
	int *Resource;		/// Resource used in trigger
	CFaction *Faction; /// Faction used in trigger
	//Wyrmgus end
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern CTimer GameTimer; /// the game timer

/// Some data accessible for script during the game.
extern TriggerDataType TriggerData;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern int TriggerGetPlayer(lua_State *l);/// get player number.
extern const CUnitType *TriggerGetUnitType(lua_State *l); /// get the unit-type
extern void TriggersEachCycle();    /// test triggers

extern void TriggerCclRegister();   /// Register ccl features
extern void SaveTriggers(CFile &file); /// Save the trigger module

#endif
