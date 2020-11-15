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
//      (c) Copyright 2020 by Andrettin
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

class dynasty;
class faction;
class resource;
class site;
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
	Q_PROPERTY(QVariantList acquired_upgrades READ get_acquired_upgrades_qstring_list)

public:
	explicit faction_history(const faction_tier default_tier, const government_type default_government_type, site *default_capital)
		: tier(default_tier), government_type(default_government_type), capital(default_capital)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

	faction_tier get_tier() const
	{
		return this->tier;
	}

	government_type get_government_type() const
	{
		return this->government_type;
	}

	const site *get_capital() const
	{
		return this->capital;
	}

	const dynasty *get_dynasty() const
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
	faction_tier tier;
	government_type government_type;
	site *capital = nullptr;
	wyrmgus::dynasty *dynasty = nullptr;
	std::map<const resource *, int> resources;
	std::map<const faction *, diplomacy_state> diplomacy_states;
	std::vector<CUpgrade *> acquired_upgrades;
};

}
