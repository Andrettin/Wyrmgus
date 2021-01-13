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
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "database/data_entry_history.h"

class CUpgrade;

namespace wyrmgus {

class upgrade_class;

class civilization_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(QVariantList acquired_upgrade_classes READ get_acquired_upgrade_classes_qstring_list)
	Q_PROPERTY(QVariantList acquired_upgrades READ get_acquired_upgrades_qstring_list)

public:
	const std::vector<upgrade_class *> &get_acquired_upgrade_classes() const
	{
		return this->acquired_upgrade_classes;
	}

	QVariantList get_acquired_upgrade_classes_qstring_list() const;

	Q_INVOKABLE void add_acquired_upgrade_class(upgrade_class *upgrade_class)
	{
		this->acquired_upgrade_classes.push_back(upgrade_class);
	}

	Q_INVOKABLE void remove_acquired_upgrade_class(upgrade_class *upgrade_class);

	const std::vector<CUpgrade *> &get_acquired_upgrades() const
	{
		return this->acquired_upgrades;
	}

	QVariantList get_acquired_upgrades_qstring_list() const;

	Q_INVOKABLE void add_acquired_upgrade(CUpgrade *upgrade)
	{
		this->acquired_upgrades.push_back(upgrade);
	}

	Q_INVOKABLE void remove_acquired_upgrade(CUpgrade *upgrade);

private:
	std::vector<upgrade_class *> acquired_upgrade_classes;
	std::vector<CUpgrade *> acquired_upgrades;
};

}
