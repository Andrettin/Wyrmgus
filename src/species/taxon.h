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
#include "database/named_data_entry.h"

struct lua_State;

int CclDefineSpeciesGenus(lua_State *l);
int CclDefineSpeciesFamily(lua_State *l);
int CclDefineSpeciesOrder(lua_State *l);
int CclDefineSpeciesClass(lua_State *l);
int CclDefineSpeciesPhylum(lua_State *l);

namespace wyrmgus {

enum class taxonomic_rank;

class taxon final : public named_data_entry, public data_type<taxon>
{
	Q_OBJECT

	Q_PROPERTY(QString common_name READ get_common_name_qstring)
	Q_PROPERTY(wyrmgus::taxonomic_rank rank MEMBER rank READ get_rank)
	Q_PROPERTY(wyrmgus::taxon* supertaxon MEMBER supertaxon READ get_supertaxon)

public:
	static constexpr const char *class_identifier = "taxon";
	static constexpr const char *database_folder = "taxons";

	explicit taxon(const std::string &identifier);

	virtual void check() const override;

	const std::string &get_common_name() const
	{
		return this->common_name;
	}

	QString get_common_name_qstring() const
	{
		return QString::fromStdString(this->common_name);
	}

	Q_INVOKABLE void set_common_name(const std::string &name)
	{
		this->common_name = name;
	}

	taxonomic_rank get_rank() const
	{
		return this->rank;
	}

	taxon *get_supertaxon() const
	{
		return this->supertaxon;
	}

	const taxon *get_supertaxon_of_rank(const taxonomic_rank rank) const
	{
		if (this->get_supertaxon() == nullptr) {
			return nullptr;
		}

		if (this->get_supertaxon()->get_rank() > rank) {
			return nullptr;
		}

		if (this->get_supertaxon()->get_rank() == rank) {
			return this->get_supertaxon();
		}

		return this->get_supertaxon()->get_supertaxon_of_rank(rank);
	}

	bool is_subtaxon_of(const taxon *other_taxon) const
	{
		if (this->get_supertaxon() == nullptr) {
			return false;
		}

		if (other_taxon->get_rank() <= this->get_rank()) {
			return false;
		}

		if (other_taxon == this->get_supertaxon()) {
			return true;
		}

		return this->get_supertaxon()->is_subtaxon_of(other_taxon);
	}

private:
	std::string common_name;
	taxonomic_rank rank;
	taxon *supertaxon = nullptr;

	friend int ::CclDefineSpeciesGenus(lua_State *l);
	friend int ::CclDefineSpeciesFamily(lua_State *l);
	friend int ::CclDefineSpeciesOrder(lua_State *l);
	friend int ::CclDefineSpeciesClass(lua_State *l);
	friend int ::CclDefineSpeciesPhylum(lua_State *l);
};

}