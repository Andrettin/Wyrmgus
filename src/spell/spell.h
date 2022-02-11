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
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski,
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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"
#include "economy/resource_container.h"
#include "item/item_class.h"
#include "luacallback.h"
#include "vec2i.h"

#ifdef __MORPHOS__
#undef Enable
#endif

class AutoCastInfo;
class CMapLayer;
class CPlayer;
class CUnit;
class CUpgrade;
struct lua_State;

static int CclDefineSpell(lua_State *l);
static void CclSpellAutocast(lua_State *l, AutoCastInfo *autocast);

namespace wyrmgus {
	class civilization;
	class faction;
	class magic_domain;
	class missile_type;
	class sound;
	class spell;
	class spell_action;
	class unit_type;
	enum class spell_target_type;
	enum class status_effect;
}

/*
** *******************
** Target definition.
** *******************
*/

class Target
{
public:
	explicit Target(const wyrmgus::spell_target_type type, CUnit *unit, const Vec2i &pos, int z) :
		Type(type), Unit(unit), targetPos(pos), MapLayer(z) {}

	wyrmgus::spell_target_type Type;                  /// type of target.
	CUnit *Unit;                      /// Unit target.
	Vec2i targetPos;
	int MapLayer;
};

/*
** *******************
** Conditions definition.
** *******************
*/

class ConditionInfoVariable final
{
public:
	char Enable = 0;                /// Target is 'user defined variable'.
	bool Check = false;                 /// True if need to check that variable.

	std::optional<int> ExactValue;   /// Target must have exactly ExactValue of it's value.
	std::optional<int> ExceptValue;            /// Target mustn't have ExceptValue of it's value.
	int MinValue = -1;               /// Target must have more Value than that.
	std::optional<int> MaxValue;               /// Target must have less Value than that.
	int MinMax = -1;                 /// Target must have more Max than that.
	int MinValuePercent = 0;        /// Target must have more (100 * Value / Max) than that.
	int MaxValuePercent = 0;        /// Target must have less (100 * Value / Max) than that.

	char ConditionApplyOnCaster = 0; /// true if these condition are for caster.
	// FIXME : More (increase, MaxMax) ?
};

/**
**  Conditions for a spell.
**
**  @todo  Move more parameters into this structure.
*/
class ConditionInfo final
{
public:
	ConditionInfo();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	
	//
	//  Conditions that check specific flags. Possible values are the defines below.
	//
#define CONDITION_FALSE 1
#define CONDITION_TRUE  0
#define CONDITION_ONLY  2
	char Alliance = 0;          /// Target is allied. (neutral is neither allied, nor opponent)
	char Opponent = 0;          /// Target is opponent. (neutral is neither allied, nor opponent)
	char TargetSelf = 0;        /// Target is the same as the caster.
	//Wyrmgus start
	char ThrustingWeapon = 0;	/// Caster has a thrusting weapon as the current weapon.
	char FactionUnit = 0;		/// Caster is a faction-specific unit.
	const wyrmgus::civilization *civilization_equivalent = nullptr;
	const wyrmgus::faction *FactionEquivalent = nullptr;	/// Caster is of the same civilization as this faction, and the faction has its own unit of the caster's class.
	//Wyrmgus end

	std::unique_ptr<char[]> BoolFlag;         /// User defined boolean flag.

	std::unique_ptr<ConditionInfoVariable[]> Variable;

	std::map<status_effect, int> status_effect_values;
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
	}
	
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	
	const ConditionInfo *get_cast_conditions() const
	{
		return this->cast_conditions.get();
	}

	/// @todo this below is SQUARE!!!
	int Range = 0;					/// Max range of the target.
	int MinRange = 0;				/// Min range of the target.

	int PriorityVar = ACP_NOVALUE;	/// Variable to sort autocast targets by priority.
	bool ReverseSort = false;		/// If true, small values have the highest priority.

private:
	std::unique_ptr<ConditionInfo> cast_conditions;	/// Conditions to cast the spell.

public:
	/// Detailed generic conditions (not per-target, where Condition is evaluated.)
	/// Combat mode is when there are hostile non-coward units around
	int Combat = 0;					/// If it should be casted in combat
	int Attacker = 0;				/// If it should be casted on unit which attacks
	int Corpse = CONDITION_FALSE;	/// If it should be casted on corpses

	// Position autocast callback
	std::unique_ptr<LuaCallback> PositionAutoCast;

	friend void ::CclSpellAutocast(lua_State *l, AutoCastInfo *autocast);
};

namespace wyrmgus {

class spell final : public named_data_entry, public data_type<spell>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::spell_target_type target MEMBER target READ get_target)
	Q_PROPERTY(int mana_cost MEMBER mana_cost READ get_mana_cost)
	Q_PROPERTY(int cooldown MEMBER cooldown READ get_cooldown)
	Q_PROPERTY(bool repeat_cast MEMBER repeat_cast READ repeats_cast)
	Q_PROPERTY(bool stackable MEMBER stackable READ is_stackable)
	Q_PROPERTY(CUpgrade* dependency_upgrade MEMBER dependency_upgrade READ get_dependency_upgrade)
	Q_PROPERTY(wyrmgus::sound* sound_when_cast MEMBER sound_when_cast READ get_sound_when_cast)
	Q_PROPERTY(bool force_use_animation MEMBER force_use_animation READ forces_use_animation)

public:
	static constexpr const char *class_identifier = "spell";
	static constexpr const char *database_folder = "spells";
	static constexpr int infinite_range = 0xFFFFFFF;

	static spell *add(const std::string &identifier, const wyrmgus::data_module *data_module);

	explicit spell(const std::string &identifier);
	~spell();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	void delete_lua_callbacks();

	const std::vector<const magic_domain *> &get_magic_domains() const
	{
		return this->magic_domains;
	}

	spell_target_type get_target() const
	{
		return this->target;
	}

	const std::vector<std::unique_ptr<spell_action>> &get_actions() const
	{
		return this->actions;
	}

	int get_mana_cost() const
	{
		return this->mana_cost;
	}

	int get_range() const
	{
		return this->range;
	}

	bool repeats_cast() const
	{
		return this->repeat_cast;
	}

	bool is_stackable() const
	{
		return this->stackable;
	}

	const resource_map<int> &get_costs() const
	{
		return this->costs;
	}

	int get_cooldown() const
	{
		return this->cooldown;
	}

	const std::string &get_effects_string() const
	{
		return this->effects_string;
	}

	CUpgrade *get_dependency_upgrade() const
	{
		return this->dependency_upgrade;
	}

	sound *get_sound_when_cast() const
	{
		return this->sound_when_cast;
	}

	const ConditionInfo *get_cast_conditions() const
	{
		return this->cast_conditions.get();
	}

	const AutoCastInfo *get_autocast_info() const
	{
		return this->autocast.get();
	}

	const AutoCastInfo *get_ai_cast_info() const
	{
		return this->ai_cast.get();
	}

	const AutoCastInfo *get_autocast_info(const bool ai) const;

	bool forces_use_animation() const
	{
		return this->force_use_animation;
	}

	/// return 1 if spell is available, 0 if not (must upgrade)
	bool IsAvailableForUnit(const CUnit &unit) const;

	bool CheckAutoCastGenericConditions(const CUnit &caster, const AutoCastInfo *autocast, const bool ignore_combat_status = false) const;
	bool IsUnitValidAutoCastTarget(const CUnit *target, const CUnit &caster, const AutoCastInfo *autocast, const int max_path_length = 0) const;
	std::vector<CUnit *> GetPotentialAutoCastTargets(const CUnit &caster, const AutoCastInfo *autocast) const;

	bool is_caster_only() const;

	int Slot;             /// Spell numeric identifier
private:
	std::vector<const magic_domain *> magic_domains;
	spell_target_type target; //targeting information
	std::vector<std::unique_ptr<spell_action>> actions; //more arguments for spell (damage, delay, additional sounds...).
	int mana_cost = 0;           /// Required mana for each cast.
	int range = 0;              /// Max range of the target.
	bool repeat_cast = false; //if the spell will be cast again until out of targets.
	bool stackable = true;		/// Whether the spell has an effect if cast multiple times at the same target
	resource_map<int> costs;        /// Resource costs of spell.
	int cooldown = 0;           /// How much time spell needs to be cast again.
	std::string effects_string;
	CUpgrade *dependency_upgrade = nullptr;
	sound *sound_when_cast = nullptr;  /// Sound played if cast
	std::unique_ptr<ConditionInfo> cast_conditions; /// Conditions to cast the spell. (generic (no test for each target))

	// Autocast information. No AICast means the AI use AutoCast.
	std::unique_ptr<AutoCastInfo> autocast; /// AutoCast information for your own units
	std::unique_ptr<AutoCastInfo> ai_cast;  /// AutoCast information for AI. More detalied.

public:
	//Wyrmgus start
	bool ItemSpell[static_cast<int>(item_class::count)];
	//Wyrmgus end

private:
	bool force_use_animation = false;

	friend int ::CclDefineSpell(lua_State *l);
};

}

/// register fonction.
extern void SpellCclRegister();

/// returns true if spell can be casted (enough mana, valid target)
extern bool CanCastSpell(const CUnit &caster, const wyrmgus::spell &spell, const CUnit *target);

/// cast spell on target unit or place at x,y
extern int SpellCast(CUnit &caster, const wyrmgus::spell &spell,
					 CUnit *target, const Vec2i &goalPos, CMapLayer *map_layer);

extern char StringToCondition(const std::string &str);

/// auto cast the spell if possible
extern int AutoCastSpell(CUnit &caster, const wyrmgus::spell &spell);

/// return 0, 1, 2 for true, only, false.
extern char Ccl2Condition(lua_State *l, const char *value);
