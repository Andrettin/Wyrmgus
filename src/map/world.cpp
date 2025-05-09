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
//      (c) Copyright 2016-2022 by Andrettin
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

#include "stratagus.h"

#include "map/world.h"

#include "database/defines.h"
#include "magic_domain.h"
#include "map/site.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/world_game_data.h"
#include "province.h"
#include "species/species.h"
#include "species/taxon.h"
#include "species/taxonomic_rank.h"
#include "ui/ui.h"
#include "util/geojson_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

world *world::add(const std::string &identifier, const wyrmgus::data_module *data_module)
{
	world *world = data_type::add(identifier, data_module);
	world->ID = static_cast<int>(world::get_all().size()) - 1;
	UI.WorldButtons.resize(world::get_all().size());
	UI.WorldButtons[world->ID].X = -1;
	UI.WorldButtons[world->ID].Y = -1;
	return world;
}

world::world(const std::string &identifier) : detailed_data_entry(identifier)
{
}

world::~world()
{
	for (CProvince *province : this->Provinces) {
		delete province;
	}
}

void world::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "empowered_magic_domains") {
		for (const std::string &value : values) {
			const magic_domain *domain = magic_domain::get(value);
			this->EmpoweredMagicDomains.push_back(domain);
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void world::initialize()
{
	if (this->time_of_day_schedule == nullptr) {
		this->time_of_day_schedule = defines::get()->get_default_time_of_day_schedule();
	}

	if (this->season_schedule == nullptr) {
		this->season_schedule = defines::get()->get_default_season_schedule();
	}

	//create the world's game data object
	this->reset_game_data();

	data_entry::initialize();
}

std::string world::get_encyclopedia_text() const
{
	std::string text;

	std::vector<const species *> sapient_species = this->get_native_sapient_species();

	if (!sapient_species.empty()) {
		std::sort(sapient_species.begin(), sapient_species.end(), [](const species *lhs, const species *rhs) {
			return lhs->get_name() < rhs->get_name();
		});

		std::string species_text = "Native Sapients: ";
		for (size_t i = 0; i < sapient_species.size(); ++i) {
			if (i > 0) {
				species_text += ", ";
			}

			const species *species = sapient_species[i];
			species_text += string::get_plural_form(species->get_name());
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(species_text));
	}

	const std::vector<const species *> fauna_species = this->get_native_fauna_species();
	if (!fauna_species.empty()) {
		std::vector<const taxon_base *> fauna_taxons;
		for (const species *species : fauna_species) {
			fauna_taxons.push_back(species);
		}

		//if the number of fauna species is too large, group them in their taxons
		static constexpr size_t max_taxon_names_size = 20;
		taxonomic_rank current_rank = taxonomic_rank::genus;
		while (fauna_taxons.size() > max_taxon_names_size && current_rank <= taxonomic_rank::empire) {
			std::map<const taxon *, int> supertaxon_counts;

			for (const taxon_base *taxon : fauna_taxons) {
				if (taxon->get_rank() >= current_rank) {
					continue;
				}

				const wyrmgus::taxon *rank_supertaxon = taxon->get_supertaxon_of_rank(current_rank);
				if (rank_supertaxon == nullptr) {
					continue;
				}

				++supertaxon_counts[rank_supertaxon];
			}

			for (const auto &[supertaxon, count] : supertaxon_counts) {
				if (count <= 1) {
					continue;
				}

				std::erase_if(fauna_taxons, [supertaxon](const taxon_base *taxon) {
					return taxon->is_subtaxon_of(supertaxon);
				});

				fauna_taxons.push_back(supertaxon);
			}

			current_rank = static_cast<taxonomic_rank>(static_cast<int>(current_rank) + 1);
		}

		std::sort(fauna_taxons.begin(), fauna_taxons.end(), [](const taxon_base *lhs, const taxon_base *rhs) {
			return lhs->get_common_name() < rhs->get_common_name();
		});

		std::string taxons_text = "Native Fauna: ";
		for (size_t i = 0; i < fauna_taxons.size(); ++i) {
			if (i > 0) {
				taxons_text += ", ";
			}

			const taxon_base *taxon = fauna_taxons[i];
			taxons_text += string::get_plural_form(taxon->get_common_name());
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(taxons_text));
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	return text;
}

void world::reset_game_data()
{
	this->game_data = std::make_unique<world_game_data>(this);
}

std::vector<QVariantList> world::parse_geojson_folder(const std::string_view &folder) const
{
	std::vector<QVariantList> geojson_data_list;

	for (const std::filesystem::path &path : database::get()->get_maps_paths()) {
		const std::filesystem::path map_path = path / this->get_identifier() / folder;

		if (!std::filesystem::exists(map_path)) {
			continue;
		}

		std::vector<QVariantList> folder_geojson_data_list = geojson::parse_folder(map_path);
		vector::merge(geojson_data_list, std::move(folder_geojson_data_list));
	}

	return geojson_data_list;
}

terrain_geodata_map world::parse_terrain_geojson_folder() const
{
	const std::vector<QVariantList> geojson_data_list = this->parse_geojson_folder(world::terrain_map_folder);

	return geojson::create_geodata_map<terrain_geodata_map>(geojson_data_list, [](const QVariantMap &properties) -> terrain_geodata_key {
		if (properties.contains("terrain_feature")) {
			const QString terrain_feature_identifier = properties.value("terrain_feature").toString();
			return terrain_feature::get(terrain_feature_identifier.toStdString());
		} else {
			const QString terrain_type_identifier = properties.value("terrain_type").toString();
			return terrain_type::get(terrain_type_identifier.toStdString());
		}
	}, [](const terrain_geodata_key &key) {
		if (std::holds_alternative<const terrain_feature *>(key)) {
			const terrain_feature *terrain_feature = std::get<const wyrmgus::terrain_feature *>(key);
			if (terrain_feature != nullptr && terrain_feature->get_geopath_width() != 0) {
				return terrain_feature->get_geopath_width();
			}
		}

		return -1;
	});
}

site_map<std::vector<std::unique_ptr<QGeoShape>>> world::parse_territories_geojson_folder() const
{
	using territory_geodata_map = site_map<std::vector<std::unique_ptr<QGeoShape>>>;

	const std::vector<QVariantList> geojson_data_list = this->parse_geojson_folder(world::territories_map_folder);

	return geojson::create_geodata_map<territory_geodata_map>(geojson_data_list, [](const QVariantMap &properties) -> territory_geodata_map::key_type {
		const QString settlement_identifier = properties.value("settlement").toString();
		const site *settlement = site::get(settlement_identifier.toStdString());
		return settlement;
	}, nullptr);
}

std::vector<const species *> world::get_native_sapient_species() const
{
	std::vector<const species *> sapient_species;
	for (const species *species : this->get_native_species()) {
		if (species->is_sapient()) {
			sapient_species.push_back(species);
		}
	}
	return sapient_species;
}

std::vector<const species *> world::get_native_fauna_species() const
{
	std::vector<const species *> fauna_species;
	for (const species *species : this->get_native_species()) {
		if (!species->is_sapient()) {
			fauna_species.push_back(species);
		}
	}
	return fauna_species;
}

}
