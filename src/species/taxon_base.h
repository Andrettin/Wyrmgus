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

#include "database/detailed_data_entry.h"

struct lua_State;

static int CclDefineSpecies(lua_State *l);
static int CclDefineSpeciesGenus(lua_State *l);
static int CclDefineSpeciesFamily(lua_State *l);
static int CclDefineSpeciesOrder(lua_State *l);
static int CclDefineSpeciesClass(lua_State *l);
static int CclDefineSpeciesPhylum(lua_State *l);

namespace wyrmgus {

class name_generator;
class taxon;
enum class gender;
enum class taxonomic_rank;

class taxon_base : public detailed_data_entry
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::taxon* supertaxon MEMBER supertaxon READ get_supertaxon)

protected:
	explicit taxon_base(const std::string &identifier);
	~taxon_base();

public:
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	virtual taxonomic_rank get_rank() const = 0;

	taxon *get_supertaxon() const
	{
		return this->supertaxon;
	}

	const taxon *get_supertaxon_of_rank(const taxonomic_rank rank) const;
	bool is_subtaxon_of(const taxon *other_taxon) const;

	const name_generator *get_specimen_name_generator(const gender gender) const;

	void add_specimen_name(const gender gender, const std::string &name);
	void add_specimen_names_from(const taxon_base *other);

private:
	taxon *supertaxon = nullptr;
	std::map<gender, std::unique_ptr<name_generator>> specimen_name_generators; //specimen names, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)

	friend int ::CclDefineSpecies(lua_State *l);
	friend int ::CclDefineSpeciesGenus(lua_State *l);
	friend int ::CclDefineSpeciesFamily(lua_State *l);
	friend int ::CclDefineSpeciesOrder(lua_State *l);
	friend int ::CclDefineSpeciesClass(lua_State *l);
	friend int ::CclDefineSpeciesPhylum(lua_State *l);
};

}
