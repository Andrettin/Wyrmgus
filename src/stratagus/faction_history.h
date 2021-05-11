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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "database/data_entry_history.h"

class CUpgrade;

namespace wyrmgus {

class dynasty;
class faction;
class resource;
class site;
class upgrade_class;
enum class diplomacy_state;
enum class faction_tier;
enum class government_type;

class faction_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::faction_tier tier MEMBER tier READ get_tier)
	Q_PROPERTY(wyrmgus::government_type government_type MEMBER government_type READ get_government_type)
	Q_PROPERTY(wyrmgus::site* capital MEMBER capital)
	Q_PROPERTY(wyrmgus::dynasty* dynasty MEMBER dynasty)
	Q_PROPERTY(std::vector<const wyrmgus::upgrade_class *> acquired_upgrade_classes READ get_acquired_upgrade_classes)
	Q_PROPERTY(std::vector<const CUpgrade *> acquired_upgrades READ get_acquired_upgrades)
	Q_PROPERTY(std::vector<const wyrmgus::site *> explored_settlements READ get_explored_settlements)

public:
	explicit faction_history(const faction_tier default_tier, const wyrmgus::government_type default_government_type, site *default_capital)
		: tier(default_tier), government_type(default_government_type), capital(default_capital)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

	faction_tier get_tier() const
	{
		return this->tier;
	}

	wyrmgus::government_type get_government_type() const
	{
		return this->government_type;
	}

	const site *get_capital() const
	{
		return this->capital;
	}

	const wyrmgus::dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	const std::map<const resource *, int> &get_resources() const
	{
		return this->resources;
	}

	const std::map<const faction *, diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

	const std::vector<const upgrade_class *> &get_acquired_upgrade_classes() const
	{
		return this->acquired_upgrade_classes;
	}

	Q_INVOKABLE void add_acquired_upgrade_class(const upgrade_class *upgrade_class)
	{
		this->acquired_upgrade_classes.push_back(upgrade_class);
	}

	Q_INVOKABLE void remove_acquired_upgrade_class(const upgrade_class *upgrade_class);

	const std::vector<const CUpgrade *> &get_acquired_upgrades() const
	{
		return this->acquired_upgrades;
	}

	Q_INVOKABLE void add_acquired_upgrade(const CUpgrade *upgrade)
	{
		this->acquired_upgrades.push_back(upgrade);
	}

	Q_INVOKABLE void remove_acquired_upgrade(const CUpgrade *upgrade);

	const std::vector<const site *> &get_explored_settlements() const
	{
		return this->explored_settlements;
	}

	Q_INVOKABLE void add_explored_settlement(const site *settlement)
	{
		this->explored_settlements.push_back(settlement);
	}

	Q_INVOKABLE void remove_explored_settlement(const site *settlement);

private:
	faction_tier tier;
	wyrmgus::government_type government_type;
	site *capital = nullptr;
	wyrmgus::dynasty *dynasty = nullptr;
	std::map<const resource *, int> resources;
	std::map<const faction *, diplomacy_state> diplomacy_states;
	std::vector<const upgrade_class *> acquired_upgrade_classes;
	std::vector<const CUpgrade *> acquired_upgrades;
	std::vector<const site *> explored_settlements;
};

}
