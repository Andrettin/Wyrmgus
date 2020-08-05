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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"

class CUpgrade;

namespace stratagus {

class deity_domain : public detailed_data_entry, public data_type<deity_domain>, public CDataType
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "deity_domain";
	static constexpr const char *database_folder = "deity_domains";

	static deity_domain *get_by_upgrade(const CUpgrade *upgrade);

	static void clear()
	{
		data_type::clear();
		deity_domain::domains_by_upgrade.clear();
	}

private:
	static inline std::map<const CUpgrade *, deity_domain *> domains_by_upgrade;

public:
	explicit deity_domain(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void ProcessConfigData(const CConfigData *config_data) override;

	CUpgrade *Upgrade = nullptr;						/// Upgrade corresponding to the domain
	std::vector<CUpgrade *> Abilities;					/// Abilities linked to this domain
};

}
