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
//      (c) Copyright 2019-2022 by Andrettin
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

namespace wyrmgus {

class region_history;
class site;

class region final : public data_entry, public data_type<region>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList superregions READ get_superregions_qvariant_list)

public:
	static constexpr const char *class_identifier = "region";
	static constexpr const char *database_folder = "regions";

	static void load_history_database();

	explicit region(const std::string &identifier);
	virtual ~region() override;

	virtual void initialize() override;
	virtual data_entry_history *get_history_base() override;

	const region_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const std::vector<site *> &get_sites() const
	{
		return this->sites;
	}

	void add_site(site *site);
	void remove_site(site *site);

	QVariantList get_superregions_qvariant_list() const;

	Q_INVOKABLE void add_superregion(region *superregion)
	{
		this->superregions.push_back(superregion);
		superregion->subregions.push_back(this);
	}

	Q_INVOKABLE void remove_superregion(region *superregion);

	bool is_part_of(const region *other_region) const;

private:
	std::vector<site *> sites; //sites located in the region
	std::vector<site *> settlements; //settlements located in the region
	std::vector<region *> subregions; //subregions of this region
	std::vector<region *> superregions; //regions for which this region is a subregion
	std::unique_ptr<region_history> history;
};

}
