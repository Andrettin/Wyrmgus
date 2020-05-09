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
//      (c) Copyright 1999-2020 by Vladi Belperchinov-Shabanski,
//                                 Joris Dauphin, Jimmy Salmon and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "luacallback.h"
#include "sound/unitsound.h"
#include "vec2i.h"

#ifdef __MORPHOS__
#undef Enable
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CMapLayer;
class CPlayer;
class CSpell;
class CUnit;
struct lua_State;

namespace stratagus {
	class faction;
	class missile_type;
	class unit_type;
}

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

/**
**  Generic spell action virtual class.
**  Spells are sub class of this one
*/
class SpellActionType
{
public:
	SpellActionType(int mod = 0) : ModifyManaCaster(mod) {};
	virtual ~SpellActionType() {};

	virtual void ProcessConfigData(const CConfigData *config_data) = 0;
	virtual int Cast(CUnit &caster, const CSpell &spell, CUnit *target, const Vec2i &goalPos, int z, int modifier) = 0;
	virtual void Parse(lua_State *l, int startIndex, int endIndex) = 0;

	const int ModifyManaCaster;
};


/**
**  Different targets.
*/
enum class TargetType {
	Self,
	Position,
	Unit
};

/*
** *******************
** Target definition.
** *******************
*/

class Target
{
public:
	Target(const TargetType type, CUnit *unit, const Vec2i &pos, int z) :
		Type(type), Unit(unit), targetPos(pos), MapLayer(z) {}

	TargetType Type;                  /// type of target.
	CUnit *Unit;                      /// Unit target.
	Vec2i targetPos;
	int MapLayer;
};

/*
** *******************
** Conditions definition.
** *******************
*/

class ConditionInfoVariable
{
public:
	ConditionInfoVariable() : Enable(0), Check(false), ExactValue(0), ExceptValue(0),
		MinValue(0), MaxValue(0), MinMax(0), MinValuePercent(0), MaxValuePercent(0),
		ConditionApplyOnCaster(0) {};

	char Enable;                /// Target is 'user defined variable'.
	bool Check;                 /// True if need to check that variable.

	int ExactValue;             /// Target must have exactly ExactValue of it's value.
	int ExceptValue;            /// Target mustn't have ExceptValue of it's value.
	int MinValue;               /// Target must have more Value than that.
	int MaxValue;               /// Target must have less Value than that.
	int MinMax;                 /// Target must have more Max than that.
	int MinValuePercent;        /// Target must have more (100 * Value / Max) than that.
	int MaxValuePercent;        /// Target must have less (100 * Value / Max) than that.

	char ConditionApplyOnCaster; /// true if these condition are for caster.
	// FIXME : More (increase, MaxMax) ?
};

/**
**  Conditions for a spell.
**
**  @todo  Move more parameters into this structure.
*/
class ConditionInfo
{
public:
	ConditionInfo() : Alliance(0), Opponent(0), TargetSelf(0),
		//Wyrmgus start
		ThrustingWeapon(0), FactionUnit(0),
		//Wyrmgus end
		BoolFlag(nullptr), Variable(nullptr), CheckFunc(nullptr) {};
	~ConditionInfo()
	{
		delete[] BoolFlag;
		delete[] Variable;
		delete CheckFunc;
	};
	
	void ProcessConfigData(const CConfigData *config_data);
	
	//
	//  Conditions that check specific flags. Possible values are the defines below.
	//
#define CONDITION_FALSE 1
#define CONDITION_TRUE  0
#define CONDITION_ONLY  2
	char Alliance;          /// Target is allied. (neutral is neither allied, nor opponent)
	char Opponent;          /// Target is opponent. (neutral is neither allied, nor opponent)
	char TargetSelf;        /// Target is the same as the caster.
	//Wyrmgus start
	char ThrustingWeapon;	/// Caster has a thrusting weapon as the current weapon.
	char FactionUnit;		/// Caster is a faction-specific unit.
	const stratagus::civilization *civilization_equivalent = nullptr;
	const stratagus::faction *FactionEquivalent = nullptr;	/// Caster is of the same civilization as this faction, and the faction has its own unit of the caster's class.
	//Wyrmgus end

	char *BoolFlag;         /// User defined boolean flag.

	ConditionInfoVariable *Variable;
	LuaCallback *CheckFunc;
	//
	//  @todo more? feel free to add, here and to
	//  @todo PassCondition, CclSpellParseCondition, SaveSpells
	//
};

/**
**  Information about the autocasting mode.
*/
class AutoCastInfo
{
public:
	// Special flags for priority sorting
#define ACP_NOVALUE -1
#define ACP_DISTANCE -2
	~AutoCastInfo()
	{
		if (this->Condition) {
			delete Condition;
		}
		if (this->PositionAutoCast) {
			delete PositionAutoCast;
		}
	};
	
	void ProcessConfigData(const CConfigData *config_data);
	
	/// @todo this below is SQUARE!!!
	int Range = 0;					/// Max range of the target.
	int MinRange = 0;				/// Min range of the target.

	int PriorityVar = ACP_NOVALUE;	/// Variable to sort autocast targets by priority.
	bool ReverseSort = false;		/// If true, small values have the highest priority.

	ConditionInfo *Condition = nullptr;	/// Conditions to cast the spell.

	/// Detailed generic conditions (not per-target, where Condition is evaluated.)
	/// Combat mode is when there are hostile non-coward units around
	int Combat = 0;					/// If it should be casted in combat
	int Attacker = 0;				/// If it should be casted on unit which attacks
	int Corpse = CONDITION_FALSE;	/// If it should be casted on corpses

	// Position autocast callback
	LuaCallback *PositionAutoCast = nullptr;
};

/**
**  Base structure of a spell type.
*/
class CSpell : public CDataType
{
public:
	CSpell(int slot, const std::string &ident);
	~CSpell();

	/// return spell type by ident string
	static CSpell *GetSpell(const std::string &ident, const bool should_find = true);
	static CSpell *GetOrAddSpell(const std::string &ident);
	static void ClearSpells();

	static std::vector<CSpell *> Spells;
	static std::map<std::string, CSpell *> SpellsByIdent;
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	/// return 1 if spell is available, 0 if not (must upgrade)
	bool IsAvailableForUnit(const CUnit &unit) const;
	const AutoCastInfo *GetAutoCastInfo(const bool ai) const;
	bool CheckAutoCastGenericConditions(const CUnit &caster, const AutoCastInfo *autocast, const bool ignore_combat_status = false) const;
	bool IsUnitValidAutoCastTarget(const CUnit *target, const CUnit &caster, const AutoCastInfo *autocast, const int max_path_length = 0) const;
	std::vector<CUnit *> GetPotentialAutoCastTargets(const CUnit &caster, const AutoCastInfo *autocast) const;
	
	// Identification stuff
	std::string Name;     /// Spell name shown by the engine
	std::string Description;	/// Spell description
	int Slot;             /// Spell numeric identifier

	// Spell Specifications
	TargetType Target;          /// Targeting information. See TargetType.
	std::vector<SpellActionType *> Action; /// More arguments for spell (damage, delay, additional sounds...).

	int Range;                  /// Max range of the target.
#define INFINITE_RANGE 0xFFFFFFF
	int ManaCost;               /// Required mana for each cast.
	int RepeatCast;             /// If the spell will be cast again until out of targets.
	bool Stackable;				/// Whether the spell has an effect if cast multiple times at the same target
	int Costs[MaxCosts];        /// Resource costs of spell.
	int CoolDown;               /// How much time spell needs to be cast again.

	int DependencyId;           /// Id of upgrade, -1 if no upgrade needed for cast the spell.
	ConditionInfo *Condition;   /// Conditions to cast the spell. (generic (no test for each target))

	// Autocast information. No AICast means the AI use AutoCast.
	AutoCastInfo *AutoCast;     /// AutoCast information for your own units
	AutoCastInfo *AICast;       /// AutoCast information for ai. More detalied.

	// Graphics and sounds. Add something else here?
	SoundConfig SoundWhenCast;  /// Sound played if cast
	
	//Wyrmgus start
	bool ItemSpell[MaxItemClasses];
	//Wyrmgus end

	bool IsCasterOnly() const
	{
		return !Range && Target == TargetType::Self;
	}
	bool ForceUseAnimation;
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// register fonction.
extern void SpellCclRegister();

/// init spell tables
extern void InitSpells();

/// returns true if spell can be casted (enough mana, valid target)
extern bool CanCastSpell(const CUnit &caster, const CSpell &spell,
						 const CUnit *target, const Vec2i &goalPos, const CMapLayer *map_layer);

/// cast spell on target unit or place at x,y
extern int SpellCast(CUnit &caster, const CSpell &spell,
					 CUnit *target, const Vec2i &goalPos, CMapLayer *map_layer);

/// auto cast the spell if possible
extern int AutoCastSpell(CUnit &caster, const CSpell &spell);

/// return 0, 1, 2 for true, only, false.
extern char Ccl2Condition(lua_State *l, const char *value);
