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

#include "database/data_type.h"
#include "species/taxon_base.h"

struct lua_State;

static int CclDefineSpecies(lua_State *l);

namespace wyrmgus {

class taxon;
class terrain_type;
class unit_type;
class world;
enum class ecological_niche;
enum class geological_era;
enum class taxonomic_rank;

class species final : public taxon_base, public data_type<species>
{
	Q_OBJECT

	Q_PROPERTY(QString specific_name READ get_specific_name_qstring)
	Q_PROPERTY(wyrmgus::geological_era era MEMBER era READ get_era)
	Q_PROPERTY(wyrmgus::world* homeworld MEMBER homeworld READ get_homeworld)
	Q_PROPERTY(bool sapient MEMBER sapient READ is_sapient)
	Q_PROPERTY(bool asexual MEMBER asexual READ is_asexual)
	Q_PROPERTY(wyrmgus::ecological_niche ecological_niche MEMBER ecological_niche READ get_ecological_niche)

public:
	static constexpr const char *class_identifier = "species";
	static constexpr const char *database_folder = "species";

	static std::map<const taxon *, int> get_supertaxon_counts(const std::vector<const species *> &source_species_list, const std::vector<const taxon *> &taxons);
	static std::vector<std::string> get_name_list(const std::vector<const species *> &species_list);

	explicit species(const std::string &identifier);

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize();
	virtual void check() const;

	virtual taxonomic_rank get_rank() const override;

	const std::string &get_specific_name() const
	{
		return this->specific_name;
	}

	QString get_specific_name_qstring() const
	{
		return QString::fromStdString(this->specific_name);
	}

	Q_INVOKABLE void set_specific_name(const std::string &name)
	{
		this->specific_name = name;
	}

	std::string get_scientific_name() const;

	geological_era get_era() const
	{
		return this->era;
	}

	world *get_homeworld() const
	{
		return this->homeworld;
	}

	bool is_sapient() const
	{
		return this->sapient;
	}

	bool is_prehistoric() const;

	bool is_asexual() const
	{
		return this->asexual;
	}

	wyrmgus::ecological_niche get_ecological_niche() const
	{
		return this->ecological_niche;
	}

	const wyrmgus::unit_type *get_unit_type() const
	{
		return this->unit_type;
	}

	void set_unit_type(const wyrmgus::unit_type *unit_type)
	{
		this->unit_type = unit_type;
	}

	const std::vector<const terrain_type *> &get_native_terrain_types() const
	{
		return this->native_terrain_types;
	}

	const std::vector<const species *> &get_pre_evolutions() const
	{
		return this->pre_evolutions;
	}

	const std::vector<const species *> &get_evolutions() const
	{
		return this->evolutions;
	}

	bool has_evolution(const terrain_type *terrain = nullptr, bool sapient_only = false) const;
	const species *get_random_evolution(const terrain_type *terrain) const;

private:
	std::string specific_name;
	geological_era era;
	world *homeworld = nullptr;
	bool sapient = false;
	bool asexual = false;
	wyrmgus::ecological_niche ecological_niche;
	const wyrmgus::unit_type *unit_type = nullptr;
	std::vector<const terrain_type *> native_terrain_types; //in which terrains does this species live
	std::vector<const species *> pre_evolutions; //species from which this one can evolve
	std::vector<const species *> evolutions; //species to which this one can evolve

	friend int ::CclDefineSpecies(lua_State *l);
};

}
