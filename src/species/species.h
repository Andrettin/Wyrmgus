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
#include "database/detailed_data_entry.h"

struct lua_State;

int CclDefineSpecies(lua_State *l);

namespace wyrmgus {

class plane;
class taxon;
class terrain_type;
class unit_type;
class world;
enum class taxonomic_rank;

class species final : public detailed_data_entry, public data_type<species>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::taxon* genus MEMBER genus READ get_genus)

public:
	static constexpr const char *class_identifier = "species";
	static constexpr const char *database_folder = "species";

	explicit species(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	virtual void check() const;

	taxon *get_genus() const
	{
		return this->genus;
	}

	const taxon *get_supertaxon_of_rank(const taxonomic_rank rank) const;

	bool CanEvolveToAUnitType(terrain_type *terrain = nullptr, bool sapient_only = false) const;
	species *GetRandomEvolution(terrain_type *terrain) const;
	
	int Era = -1;					/// Era ID
	bool Sapient = false;			/// Whether the species is sapient
	bool Prehistoric = false;		/// Whether the species is prehistoric or not
private:
	taxon *genus = nullptr;
public:
	std::string Species;
	std::string ChildUpgrade;		/// Which individual upgrade the children of this species get
	plane *home_plane = nullptr;
	world *homeworld = nullptr;
	unit_type *Type = nullptr;
	std::vector<terrain_type *> Terrains;	/// in which terrains does this species live
	std::vector<species *> EvolvesFrom;	/// from which species this one can evolve
	std::vector<species *> EvolvesTo;		/// to which species this one can evolve

	friend int ::CclDefineSpecies(lua_State *l);
};

}