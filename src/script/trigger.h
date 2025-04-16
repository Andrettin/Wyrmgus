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
//      (c) Copyright 2002-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"

Q_MOC_INCLUDE("script/trigger_random_group.h")

class CFile;
class CPlayer;
class CUnit;
class CUpgrade;
class LuaCallback;
struct lua_State;

extern int CclAddTrigger(lua_State *l);
extern void TriggersEachCycle();

namespace wyrmgus {

class dynasty;
class faction;
class resource;
class tile;
class trigger_random_group;
class unit_type;
enum class trigger_target;
enum class trigger_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class factor;

class trigger final : public data_entry, public data_type<trigger>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::trigger_type type MEMBER type READ get_type)
	Q_PROPERTY(wyrmgus::trigger_target target MEMBER target READ get_target)
	Q_PROPERTY(wyrmgus::trigger_random_group* random_group MEMBER random_group)
	Q_PROPERTY(bool random READ is_random WRITE set_random)
	Q_PROPERTY(bool only_once MEMBER only_once READ fires_only_once)
	Q_PROPERTY(bool campaign_only MEMBER campaign_only READ is_campaign_only)
	Q_PROPERTY(QDate historical_date MEMBER historical_date READ get_historical_date)

public:
	static constexpr const char *class_identifier = "trigger";
	static constexpr const char property_class_identifier[] = "wyrmgus::trigger*";
	static constexpr const char *database_folder = "triggers";
	static constexpr int default_random_weight = 100;

	static void clear();
	static void InitActiveTriggers();	/// Setup triggers
	static void ClearActiveTriggers();

	static void check_pulse_type(const trigger_type type);
	static void check_triggers(const trigger_type type);
	static void check_triggers_for_player(CPlayer *player, const trigger_type type);
	static void check_random_triggers_for_player(CPlayer *player, const std::vector<const trigger *> &triggers);
	static int get_type_cycles(const trigger_type type);
	static int generate_random_offset_for_type(const trigger_type type);

private:
	static inline std::map<trigger_type, std::vector<trigger *>> active_triggers; //triggers that are active for the current game
	static inline std::map<trigger_type, std::vector<const trigger *>> active_random_triggers;

public:
	static std::vector<std::string> DeactivatedTriggers;
	static unsigned int CurrentTriggerId;
private:
	static inline std::map<trigger_type, int> pulse_random_offsets;

public:
	explicit trigger(const std::string &identifier);
	~trigger();
	
	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	trigger_type get_type() const
	{
		return this->type;
	}

	void set_type(const trigger_type type)
	{
		this->type = type;
	}

	trigger_target get_target() const
	{
		return this->target;
	}

	void set_target(const trigger_target target)
	{
		this->target = target;
	}

	trigger_random_group *get_random_group() const
	{
		return this->random_group;
	}

	void set_random_group(trigger_random_group *random_group)
	{
		this->random_group = random_group;
	}

	bool is_random() const
	{
		return this->get_random_weight_factor() != nullptr;
	}

	void set_random(const bool random)
	{
		if (random == this->is_random()) {
			return;
		}

		if (random) {
			this->set_random_weight(trigger::default_random_weight);
		} else {
			this->set_random_weight(0);
		}
	}

	void set_random_weight(const int weight);

	const factor<CPlayer> *get_random_weight_factor() const
	{
		return this->random_weight_factor.get();
	}

	bool fires_only_once() const
	{
		return this->only_once;
	}

	bool is_campaign_only() const
	{
		return this->campaign_only;
	}

	void set_campaign_only(const bool campaign_only)
	{
		this->campaign_only = campaign_only;
	}

	const QDate &get_historical_date() const
	{
		return this->historical_date;
	}

	const std::unique_ptr<and_condition<CPlayer>> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<and_condition<CPlayer>> &get_conditions() const
	{
		return this->conditions;
	}

	void add_condition(std::unique_ptr<condition<CPlayer>> &&condition);

	const std::unique_ptr<effect_list<CPlayer>> &get_effects() const
	{
		return this->effects;
	}

	void add_effect(std::unique_ptr<effect<CPlayer>> &&effect);

	bool is_player_valid_target(const CPlayer *player) const;
	bool check_for_player(CPlayer *player) const;

private:
	trigger_type type;
	trigger_target target;
	trigger_random_group *random_group = nullptr;
	std::unique_ptr<factor<CPlayer>> random_weight_factor;
public:
	bool Local = false;
private:
	bool only_once = false; //whether the trigger should occur only once in a game
	bool campaign_only = true; //whether the trigger should only occur in the campaign mode
	QDate historical_date;
public:
	std::unique_ptr<LuaCallback> Conditions;
	std::unique_ptr<LuaCallback> Effects;
private:
	std::unique_ptr<and_condition<CPlayer>> preconditions;
	std::unique_ptr<and_condition<CPlayer>> conditions;
	std::unique_ptr<effect_list<CPlayer>> effects;

	friend int ::CclAddTrigger(lua_State *l);
	friend void ::TriggersEachCycle();
};

}

#define ANY_UNIT ((const wyrmgus::unit_type *)0)
#define ALL_FOODUNITS ((const wyrmgus::unit_type *)-1)
#define ALL_BUILDINGS ((const wyrmgus::unit_type *)-2)

/**
**  Data to referer game info when game running.
*/
struct TriggerDataType final {
	CUnit *Attacker = nullptr;  /// Unit which send the missile.
	CUnit *Defender = nullptr;  /// Unit which is hit by missile.
	CUnit *Active = nullptr;    /// Unit which is selected or else under cursor unit.
	//Wyrmgus start
	CUnit *Unit = nullptr;	  /// Unit used in trigger
	//Wyrmgus end
	const unit_type *Type = nullptr;		/// Type used in trigger;
	const CUpgrade *Upgrade = nullptr;		/// Upgrade used in trigger
	const wyrmgus::resource *resource = nullptr;	/// Resource used in trigger
	const wyrmgus::faction *faction = nullptr;		/// Faction used in trigger
	const wyrmgus::dynasty *dynasty = nullptr;		/// Dynasty used in trigger
	const CPlayer *player = nullptr;
	const wyrmgus::tile *tile = nullptr;
};

/// Some data accessible for script during the game.
extern TriggerDataType TriggerData;

extern int TriggerGetPlayer(lua_State *l);/// get player number.
extern const unit_type *TriggerGetUnitType(lua_State *l); /// get the unit-type
extern void TriggersEachCycle();    /// test triggers
extern void call_trigger(const std::string &identifier);

extern void TriggerCclRegister();   /// Register ccl features
extern void SaveTriggers(CFile &file); /// Save the trigger module
