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
/**@name actions.h - The actions headerfile. */
//
//      (c) Copyright 1998-2012 by Lutz Sammer and Jimmy Salmon
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

#pragma once

#include "unit/unitptr.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/**
**  All possible unit actions.
**
**  @note  Always change the table ::HandleActionTable
**
**  @see HandleActionTable
*/
enum class UnitAction : char {
	None,         /// No valid action

	Still,        /// unit stand still, does nothing
	StandGround,  /// unit stands ground
	Follow,       /// unit follows units
	Defend,       /// unit defends unit
	Move,         /// unit moves to position/unit
	Attack,       /// unit attacks position/unit
	AttackGround, /// unit attacks ground
	//Wyrmgus start
	PickUp,		/// unit picks up an item
	Use,		/// unit uses another unit (i.e. an item)
	Trade,		/// unit trades with another unit (i.e. a merchant or caravan trades with a market)
	//Wyrmgus end
	Die,		/// unit dies

	SpellCast,    /// unit casts spell

	Train,        /// building is training
	UpgradeTo,    /// building is upgrading itself
	Research,     /// building is researching spell
	Built,		/// building is under construction

	// Compound actions
	Board,        /// unit entering transporter
	Unload,       /// unit leaving transporter
	Patrol,       /// unit paroling area
	Build,        /// unit builds building

	Repair,       /// unit repairing
	Resource,     /// unit harvesting resources
	TransformInto /// unit transform into type.
};

class CAnimation;
class CFile;
class CUnit;
class CUpgrade;
class PathFinderInput;
class CViewport;
struct lua_State;

namespace wyrmgus {
	class construction_frame;
	class site;
	class spell;
	class unit_type;
}

/**
**  Unit order structure.
*/
class COrder
{
public:
	explicit COrder(const UnitAction action) : Goal(), Action(action), Finished(false)
	{
	}
	virtual ~COrder();

	virtual COrder *Clone() const = 0;
	virtual void Execute(CUnit &unit) = 0;
	virtual void Cancel(CUnit &unit) {}
	virtual bool IsValid() const = 0;

	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos) const = 0;

	virtual void OnAnimationAttack(CUnit &unit);

	virtual void Save(CFile &file, const CUnit &unit) const = 0;
	bool ParseGenericData(lua_State *l, int &j, const char *value);
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) = 0;

	virtual void UpdateUnitVariables(CUnit &unit) const {}
	virtual void FillSeenValues(CUnit &unit) const;
	virtual void AiUnitKilled(CUnit &unit);

	virtual void UpdatePathFinderData(PathFinderInput &input) = 0;

	bool HasGoal() const { return Goal != nullptr; }
	CUnit *GetGoal() const { return Goal; };
	void SetGoal(CUnit *const new_goal);
	void ClearGoal();
	virtual const Vec2i GetGoalPos() const;
	//Wyrmgus start
	virtual const int GetGoalMapLayer() const;
	//Wyrmgus end

	virtual bool OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/);

	static COrder *NewActionAttack(const CUnit &attacker, CUnit &target);
	//Wyrmgus start
//	static COrder *NewActionAttack(const CUnit &attacker, const Vec2i &dest);
//	static COrder *NewActionAttackGround(const CUnit &attacker, const Vec2i &dest);
	static COrder *NewActionAttack(const CUnit &attacker, const Vec2i &dest, int z);
	static COrder *NewActionAttackGround(const CUnit &attacker, const Vec2i &dest, int z);
	//Wyrmgus end
	static COrder *NewActionBoard(CUnit &unit);
	static COrder *NewActionBuild(const CUnit &builder, const Vec2i &pos, wyrmgus::unit_type &building, int z, const wyrmgus::site *settlement);
	static COrder *NewActionBuilt(CUnit &builder, CUnit &unit);
	static COrder *NewActionDefend(CUnit &dest);
	static COrder *NewActionDie();
	static COrder *NewActionFollow(CUnit &dest);
	//Wyrmgus start
//	static COrder *NewActionMove(const Vec2i &pos);
//	static COrder *NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest);
	static COrder *NewActionMove(const Vec2i &pos, int z);
	static COrder *NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest, int current_z, int dest_z);
	//Wyrmgus end
	//Wyrmgus start
	static COrder *NewActionPickUp(CUnit &dest);
	//Wyrmgus end
	static COrder *NewActionRepair(CUnit &unit, CUnit &target);
	//Wyrmgus start
//	static COrder *NewActionRepair(const Vec2i &pos);
	static COrder *NewActionRepair(const Vec2i &pos, int z);
	//Wyrmgus end
	static COrder *NewActionResearch(CUnit &unit, const CUpgrade &upgrade, int player);
	static COrder *NewActionResource(CUnit &harvester, const Vec2i &pos, int z);
	static COrder *NewActionResource(CUnit &harvester, CUnit &mine);
	static COrder *NewActionReturnGoods(CUnit &harvester, CUnit *depot);
	static COrder *NewActionSpellCast(const wyrmgus::spell &spell, const Vec2i &pos, CUnit *target, int z, bool isAutocast = false);
	static COrder *NewActionStandGround();
	static COrder *NewActionStill();
	static COrder *NewActionTrain(CUnit &trainer, wyrmgus::unit_type &type, int player);
	static COrder *NewActionTransformInto(wyrmgus::unit_type &type);
	static COrder *NewActionUnload(const Vec2i &pos, CUnit *what, int z, int landmass);
	static COrder *NewActionUpgradeTo(CUnit &unit, wyrmgus::unit_type &type);
	//Wyrmgus start
	static COrder *NewActionUse(CUnit &dest);
	static COrder *NewActionTrade(CUnit &dest, CUnit &home_market);
	//Wyrmgus end

protected:
	void UpdatePathFinderData_NotCalled(PathFinderInput &input);

private:
	CUnitPtr Goal;
public:
	const UnitAction Action;   /// global action
	bool Finished; /// true when order is finished
};

typedef COrder *COrderPtr;


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern unsigned SyncHash;  /// Hash calculated to find sync failures

/*----------------------------------------------------------------------------
--  Actions: in action_<name>.c
----------------------------------------------------------------------------*/

extern int GetNumWaitingWorkers(const CUnit &mine);
extern bool AutoAttack(CUnit &unit);
extern bool AutoRepair(CUnit &unit);
extern bool AutoCast(CUnit &unit);
extern void UnHideUnit(CUnit &unit);

/// Generic move action
extern int DoActionMove(CUnit &unit);
/// Show attack animation
extern void AnimateActionAttack(CUnit &unit, COrder &order);

/*----------------------------------------------------------------------------
--  Actions: actions.c
----------------------------------------------------------------------------*/

/// Parse order
extern void CclParseOrder(lua_State *l, CUnit &unit, COrderPtr *order);

/// Handle the actions of all units each game cycle
extern void UnitActions();
