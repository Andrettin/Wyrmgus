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

class site;
class upgrade_class;

class civilization_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(std::vector<const wyrmgus::upgrade_class *> acquired_upgrade_classes READ get_acquired_upgrade_classes)
	Q_PROPERTY(std::vector<const CUpgrade *> acquired_upgrades READ get_acquired_upgrades)
	Q_PROPERTY(std::vector<const wyrmgus::site *> explored_settlements READ get_explored_settlements)

public:
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
	std::vector<const upgrade_class *> acquired_upgrade_classes;
	std::vector<const CUpgrade *> acquired_upgrades;
	std::vector<const site *> explored_settlements;
};

}
