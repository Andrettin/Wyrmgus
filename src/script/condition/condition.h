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
//      (c) Copyright 2000-2021 by Vladi Belperchinov-Shabanski and Andrettin
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

class CConfigData;
class CPlayer;
class CUnit;
class CUpgrade;

namespace wyrmgus {

class button;
class civilization;
class named_data_entry;
class sml_data;
class sml_property;
class unit_type;

class condition
{
public:
	static std::unique_ptr<const condition> from_sml_property(const sml_property &property);
	static std::unique_ptr<const condition> from_sml_scope(const sml_data &scope);

	static std::string get_conditions_string(const std::vector<std::unique_ptr<const condition>> &conditions, const size_t indent, const bool links_allowed)
	{
		std::string conditions_string;
		bool first = true;
		for (const std::unique_ptr<const condition> &condition : conditions) {
			if (condition->is_hidden()) {
				continue;
			}

			const std::string condition_string = condition->get_string(indent, links_allowed);
			if (condition_string.empty()) {
				continue;
			}

			if (first) {
				first = false;
			} else {
				conditions_string += "\n";
			}

			if (indent > 0) {
				conditions_string += std::string(indent, '\t');
			}

			conditions_string += condition_string;
		}
		return conditions_string;
	}

	//get the string for the object of a condition, e.g. the unit type for a unit type condition
	template <typename T>
	static std::string get_object_string(const T *object, const bool links_allowed, const std::string &name_string = "")
	{
		if (links_allowed) {
			return object->get_link_string(name_string, true);
		} else {
			return condition::get_object_highlighted_name(object, name_string);
		}
	}

	static std::string get_object_highlighted_name(const named_data_entry *object, const std::string &name_string);

	virtual ~condition()
	{
	}

	void ProcessConfigData(const CConfigData *config_data);
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property);
	virtual void ProcessConfigDataSection(const CConfigData *section);
	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);

	virtual void check_validity() const
	{
	}

	virtual bool check(const civilization *civilization) const
	{
		//check whether a civilization can, in principle, fulfill the condition
		Q_UNUSED(civilization)
		return true;
	}

	virtual bool check(const CPlayer *player, bool ignore_units = false) const = 0;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const;

	//get the condition as a string
	virtual std::string get_string(const size_t indent, const bool links_allowed) const = 0;

	virtual bool is_hidden() const
	{
		return false;
	}
};

template <bool precondition>
extern bool check_special_conditions(const unit_type *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use);

template <bool precondition>
extern bool check_special_conditions(const CUpgrade *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use);

//check conditions for player
template <bool precondition = false, typename T>
extern bool check_conditions(const T *target, const CPlayer *player, const bool ignore_units = false, const bool is_neutral_use = false)
{
	if constexpr (!precondition) {
		if (!check_conditions<true>(target, player, ignore_units, is_neutral_use)) {
			return false;
		}
	}

	if constexpr (std::is_same_v<T, unit_type>) {
		if (!check_special_conditions<precondition>(target, player, ignore_units, is_neutral_use)) {
			return false;
		}
	} else if constexpr (std::is_same_v<T, CUpgrade>) {
		if (!check_special_conditions<precondition>(target, player, ignore_units, is_neutral_use)) {
			return false;
		}
	}

	if constexpr (precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->check(player, ignore_units);
	} else {
		return target->get_conditions() == nullptr || target->get_conditions()->check(player, ignore_units);
	}
}

template <bool precondition>
extern bool check_special_conditions(const unit_type *target, const CUnit *unit, const bool ignore_units);

template <bool precondition>
extern bool check_special_conditions(const CUpgrade *target, const CUnit *unit, const bool ignore_units);

//check conditions for unit
template <bool precondition = false, typename T>
extern bool check_conditions(const T *target, const CUnit *unit, const bool ignore_units = false)
{
	if constexpr (!precondition) {
		if (!check_conditions<true>(target, unit, ignore_units)) {
			return false;
		}
	}

	if constexpr (std::is_same_v<T, unit_type>) {
		if (!check_special_conditions<precondition>(target, unit, ignore_units)) {
			return false;
		}
	} else if constexpr (std::is_same_v<T, CUpgrade>) {
		if (!check_special_conditions<precondition>(target, unit, ignore_units)) {
			return false;
		}
	}

	if constexpr (precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->check(unit, ignore_units);
	} else {
		return target->get_conditions() == nullptr || target->get_conditions()->check(unit, ignore_units);
	}
}

}

/// Register CCL features for dependencies
extern void DependenciesCclRegister();

/// Print all unit conditions into string
extern std::string PrintConditions(const wyrmgus::button &button);
