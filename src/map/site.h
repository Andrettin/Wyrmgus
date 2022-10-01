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
#include "database/named_data_entry.h"
#include "data_type.h"
#include "time/date.h"
#include "util/color_container.h"
#include "util/geocoordinate.h"
#include "util/qunique_ptr.h"

class CMapLayer;
class CPlayer;
class CUnit;
struct lua_State;

extern int CclDefineSite(lua_State *l);

namespace wyrmgus {

class character;
class civilization;
class faction;
class map_template;
class player_color;
class region;
class site_game_data;
class site_history;
class unique_item;
class unit_class;
class unit_type;

class site final : public named_data_entry, public data_type<site>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::map_template* map_template MEMBER map_template)
	Q_PROPERTY(QPoint pos MEMBER pos READ get_pos)
	Q_PROPERTY(wyrmgus::site* pos_reference_site MEMBER pos_reference_site)
	Q_PROPERTY(archimedes::geocoordinate geocoordinate MEMBER geocoordinate READ get_geocoordinate)
	Q_PROPERTY(archimedes::geocoordinate geocoordinate_offset MEMBER geocoordinate_offset)
	Q_PROPERTY(wyrmgus::site* geocoordinate_reference_site MEMBER geocoordinate_reference_site)
	Q_PROPERTY(int longitude_scale MEMBER longitude_scale)
	Q_PROPERTY(int latitude_scale MEMBER latitude_scale)
	Q_PROPERTY(archimedes::geocoordinate astrocoordinate MEMBER astrocoordinate READ get_astrocoordinate)
	Q_PROPERTY(QTime right_ascension READ get_right_ascension WRITE set_right_ascension)
	Q_PROPERTY(archimedes::decimillesimal_int declination READ get_declination WRITE set_declination)
	Q_PROPERTY(bool map_template_orbit MEMBER map_template_orbit READ orbits_map_template)
	Q_PROPERTY(wyrmgus::map_template* astrocoordinate_reference_subtemplate MEMBER astrocoordinate_reference_subtemplate)
	Q_PROPERTY(archimedes::centesimal_int astrodistance MEMBER astrodistance READ get_astrodistance)
	Q_PROPERTY(archimedes::centesimal_int astrodistance_pc READ get_astrodistance_pc WRITE set_astrodistance_pc)
	Q_PROPERTY(int astrodistance_additive_modifier MEMBER astrodistance_additive_modifier READ get_astrodistance_additive_modifier)
	Q_PROPERTY(wyrmgus::site* orbit_center MEMBER orbit_center WRITE set_orbit_center)
	Q_PROPERTY(int distance_from_orbit_center MEMBER distance_from_orbit_center)
	Q_PROPERTY(archimedes::centesimal_int distance_from_orbit_center_au READ get_distance_from_orbit_center_au WRITE set_distance_from_orbit_center_au)
	Q_PROPERTY(wyrmgus::unit_type* base_unit_type MEMBER base_unit_type)
	Q_PROPERTY(wyrmgus::site* settlement MEMBER settlement)
	Q_PROPERTY(quint64 mass MEMBER mass READ get_mass)
	Q_PROPERTY(archimedes::centesimal_int mass_jm READ get_mass_jm WRITE set_mass_jm)
	Q_PROPERTY(wyrmgus::site* connection_destination MEMBER connection_destination)
	Q_PROPERTY(QVariantList regions READ get_regions_qvariant_list)
	Q_PROPERTY(QColor color READ get_color WRITE set_color)

public:
	static constexpr const char *class_identifier = "site";
	static constexpr const char property_class_identifier[] = "wyrmgus::site*";
	static constexpr const char *database_folder = "sites";
	static constexpr int base_astrodistance_additive_modifier = 8;
	static constexpr int base_orbit_distance = 2; //the tile space between the orbit center and its first orbiting body

	static site *get_by_color(const QColor &color)
	{
		site *site = site::try_get_by_color(color);

		if (site == nullptr) {
			throw std::runtime_error("No site found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return site;
	}

	static site *try_get_by_color(const QColor &color)
	{
		auto find_iterator = site::sites_by_color.find(color);
		if (find_iterator != site::sites_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		site::sites_by_color.clear();
	}

private:
	static inline color_map<site *> sites_by_color;

public:
	explicit site(const std::string &identifier);
	~site();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	site_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	site_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	const std::string &get_cultural_name(const civilization *civilization) const;

	bool is_settlement() const;
	bool can_have_population() const;
	bool can_use_name_for_site_unit() const;

	const wyrmgus::map_template *get_map_template() const
	{
		return this->map_template;
	}

	const QPoint &get_pos() const
	{
		return this->pos;
	}

	const archimedes::geocoordinate &get_geocoordinate() const
	{
		return this->geocoordinate;
	}

	const archimedes::geocoordinate &get_astrocoordinate() const
	{
		return this->astrocoordinate;
	}

	QTime get_right_ascension() const;
	void set_right_ascension(const QTime &ra);

	const decimillesimal_int &get_declination() const
	{
		return this->get_astrocoordinate().get_latitude();
	}

	void set_declination(const decimillesimal_int &declination)
	{
		this->astrocoordinate.set_latitude(declination);
	}

	bool orbits_map_template() const
	{
		return this->map_template_orbit;
	}

	const centesimal_int &get_astrodistance() const
	{
		return this->astrodistance;
	}

	centesimal_int get_astrodistance_pc() const;
	void set_astrodistance_pc(const centesimal_int &astrodistance_pc);

	int get_astrodistance_additive_modifier() const
	{
		return this->astrodistance_additive_modifier;
	}

	const site *get_orbit_center() const
	{
		return this->orbit_center;
	}

	void set_orbit_center(site *orbit_center);

	const site *get_top_orbit_center() const
	{
		if (this->get_orbit_center() != nullptr) {
			return this->get_orbit_center()->get_top_orbit_center();
		}

		return this;
	}

	const std::vector<site *> &get_satellites() const
	{
		return this->satellites;
	}

	//get the nearest substantial (i.e. not asteroid) satellites to a given distance from orbit center
	std::pair<const site *, const site *> get_nearest_satellites(const int64_t distance) const;

	int get_distance_from_orbit_center() const
	{
		return this->distance_from_orbit_center;
	}

	centesimal_int get_distance_from_orbit_center_au() const;
	void set_distance_from_orbit_center_au(const centesimal_int &distance_au);

	QPoint astrocoordinate_to_relative_pos(const archimedes::geocoordinate &astrocoordinate, const QSize &reference_subtemplate_applied_size) const;

	template <bool use_map_pos>
	QPoint astrocoordinate_to_pos(const archimedes::geocoordinate &astrocoordinate) const;

	const unit_type *get_base_unit_type() const
	{
		return this->base_unit_type;
	}

	const QSize &get_size() const;
	QSize get_satellite_orbit_size() const;
	QSize get_size_with_satellites() const;

	const site *get_settlement() const
	{
		return this->settlement;
	}

	uint64_t get_mass() const
	{
		return this->mass;
	}

	centesimal_int get_mass_jm() const;
	void set_mass_jm(const centesimal_int &mass_jm);

	const site *get_connection_destination() const
	{
		return this->connection_destination;
	}

	bool is_connector() const
	{
		return this->get_connection_destination() != nullptr;
	}

	bool can_be_randomly_generated_settlement() const
	{
		return this->is_settlement() && !this->get_name().empty() && !this->is_connector();
	}

	const std::vector<region *> &get_regions() const
	{
		return this->regions;
	}

	QVariantList get_regions_qvariant_list() const;

	Q_INVOKABLE void add_region(region *region);
	Q_INVOKABLE void remove_region(region *region);

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color)
	{
		if (color == this->get_color()) {
			return;
		}

		if (site::try_get_by_color(color) != nullptr) {
			throw std::runtime_error("Color is already used by another site.");
		}

		this->color = color;
		site::sites_by_color[color] = this;
	}

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

	const std::vector<const faction *> &get_neutral_factions() const
	{
		return this->neutral_factions;
	}

	void add_neutral_faction(const faction *faction)
	{
		this->neutral_factions.push_back(faction);
	}

private:
	wyrmgus::map_template *map_template = nullptr;
	QPoint pos = QPoint(-1, -1); //position of the site in its map template
	site *pos_reference_site = nullptr; //the site's reference position site, used as an offset for its position
	archimedes::geocoordinate geocoordinate; //the site's position as a geocoordinate
	archimedes::geocoordinate geocoordinate_offset;
	site *geocoordinate_reference_site = nullptr; //the site's reference geocoordinate site, used as an offset for its geocoordinate
	int longitude_scale = 100;
	int latitude_scale = 100;
	archimedes::geocoordinate astrocoordinate; //the site's position as an astrocoordinate
	bool map_template_orbit = false; //whether this site orbits a map template, with the consequence that a random astrocoordinate is used to apply the site's position when applied to the map
	wyrmgus::map_template *astrocoordinate_reference_subtemplate = nullptr;
	centesimal_int astrodistance; //the site's distance from its map template's center (in light-years)
	int astrodistance_additive_modifier = 0;
	site *orbit_center = nullptr;
	int distance_from_orbit_center = 0; //in gigameters (millions of kilometers)
	std::vector<site *> satellites;
	unit_type *base_unit_type = nullptr;
	site *settlement = nullptr; //the settlement in whose territory the site should be applied
	uint64_t mass = 0; //the mass of the site in zettagrams, if it is a celestial body
	site *connection_destination = nullptr;
	std::vector<region *> regions; //regions where this site is located
	std::map<const civilization *, std::string> cultural_names;	/// Names for the site for each different culture/civilization
	QColor color; //color used to represent the site on the minimap, and to identify its territory on territory images
	std::vector<character *> characters; //characters which can be recruited at this site
	std::vector<const faction *> neutral_factions;
	std::unique_ptr<site_history> history;
	qunique_ptr<site_game_data> game_data;
public:
	std::map<CDate, const faction *> HistoricalOwners;			/// Historical owners of the site
	std::vector<std::tuple<CDate, CDate, const unit_type *, int, const faction *>> HistoricalUnits;	/// Historical quantity of a particular unit type (number of people for units representing a person)
	std::vector<std::tuple<CDate, CDate, const unit_class *, unique_item *, const faction *>> HistoricalBuildings; /// Historical buildings, with start and end date

	friend int ::CclDefineSite(lua_State *l);
};

extern template QPoint site::astrocoordinate_to_pos<false>(const archimedes::geocoordinate &) const;
extern template QPoint site::astrocoordinate_to_pos<true>(const archimedes::geocoordinate &) const;

}
