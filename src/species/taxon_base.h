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

#include "database/detailed_data_entry.h"

struct lua_State;

static int CclDefineSpecies(lua_State *l);
static int CclDefineSpeciesGenus(lua_State *l);
static int CclDefineSpeciesFamily(lua_State *l);
static int CclDefineSpeciesOrder(lua_State *l);
static int CclDefineSpeciesClass(lua_State *l);
static int CclDefineSpeciesPhylum(lua_State *l);

namespace wyrmgus {

class taxon;
enum class gender;
enum class taxonomic_rank;

class taxon_base : public detailed_data_entry
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::taxon* supertaxon MEMBER supertaxon READ get_supertaxon)

protected:
	explicit taxon_base(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

public:
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	virtual taxonomic_rank get_rank() const = 0;

	taxon *get_supertaxon() const
	{
		return this->supertaxon;
	}

	const taxon *get_supertaxon_of_rank(taxonomic_rank rank) const;
	bool is_subtaxon_of(const taxon *other_taxon) const;

	const std::map<gender, std::vector<std::string>> &get_specimen_names() const;

	void add_specimen_name(gender gender, const std::string &name);
	void add_specimen_names_from(const taxon_base *other);

private:
	taxon *supertaxon = nullptr;
	std::map<gender, std::vector<std::string>> specimen_names; //specimen names, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)

	friend static int ::CclDefineSpecies(lua_State *l);
	friend static int ::CclDefineSpeciesGenus(lua_State *l);
	friend static int ::CclDefineSpeciesFamily(lua_State *l);
	friend static int ::CclDefineSpeciesOrder(lua_State *l);
	friend static int ::CclDefineSpeciesClass(lua_State *l);
	friend static int ::CclDefineSpeciesPhylum(lua_State *l);
};

}
