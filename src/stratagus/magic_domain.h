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
//      (c) Copyright 2018-2022 by Andrettin
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
#include "database/detailed_data_entry.h"
#include "data_type.h"

class CUpgrade;

namespace wyrmgus {

class spell;

class magic_domain final : public detailed_data_entry, public data_type<magic_domain>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "magic_domain";
	static constexpr const char *database_folder = "magic_domains";

public:
	static magic_domain *add(const std::string &identifier, const wyrmgus::data_module *data_module);

	explicit magic_domain(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

	const CUpgrade *get_deity_domain_upgrade() const
	{
		return this->deity_domain_upgrade;
	}

	const std::vector<const spell *> &get_spells() const
	{
		return this->spells;
	}

	void add_spell(const spell *spell)
	{
		this->spells.push_back(spell);
	}

private:
	CUpgrade *deity_domain_upgrade = nullptr; //the upgrade corresponding to the domain as a deity domain
	std::vector<const spell *> spells; //spells which belong to this domain
};

}
