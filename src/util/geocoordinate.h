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
//      (c) Copyright 2021 by Andrettin
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

#include "util/fractional_int.h"

namespace wyrmgus {

class geocoordinate final
{
public:
	static constexpr int decimal_precision = 4;
	using number_type = fractional_int<geocoordinate::decimal_precision>;

	static constexpr int longitude_size = 360;
	static constexpr int latitude_size = 180;
	static constexpr number_type min_longitude = number_type(geocoordinate::longitude_size / 2 * -1);
	static constexpr number_type max_longitude = number_type(geocoordinate::longitude_size / 2);
	static constexpr number_type min_latitude = number_type(geocoordinate::latitude_size / 2 * -1);
	static constexpr number_type max_latitude = number_type(geocoordinate::latitude_size / 2);

	static void for_each_random_until(const std::function<bool(const geocoordinate &)> &function);

	constexpr geocoordinate()
	{
	}

	explicit constexpr geocoordinate(const number_type &longitude, const number_type &latitude)
		: longitude(longitude), latitude(latitude)
	{
	}

	explicit constexpr geocoordinate(number_type &&longitude, number_type &&latitude)
		: longitude(std::move(longitude)), latitude(std::move(latitude))
	{
	}

	explicit constexpr geocoordinate(const int longitude, const int latitude)
		: geocoordinate(number_type(longitude), number_type(latitude))
	{
	}

	explicit constexpr geocoordinate(const QPoint &point) : geocoordinate(point.x(), point.y())
	{
	}

	explicit geocoordinate(const QGeoCoordinate &qgeocoordinate) : geocoordinate(number_type(qgeocoordinate.longitude()), number_type(qgeocoordinate.latitude()))
	{
	}

	constexpr const number_type &get_longitude() const
	{
		return this->longitude;
	}

	constexpr void set_longitude(const number_type &lon)
	{
		this->longitude = lon;
	}

	constexpr const number_type &get_latitude() const
	{
		return this->latitude;
	}

	constexpr void set_latitude(const number_type &lat)
	{
		this->latitude = lat;
	}

	constexpr bool is_valid() const
	{
		return this->get_longitude() >= geocoordinate::min_longitude && this->get_longitude() <= geocoordinate::max_longitude && this->get_latitude() >= geocoordinate::min_latitude && this->get_latitude() <= geocoordinate::max_latitude;
	}

	constexpr bool is_null() const
	{
		return this->get_longitude() == 0 && this->get_latitude() == 0;
	}

	QGeoCoordinate to_qgeocoordinate() const
	{
		const double latitude = this->latitude.to_double();
		const double longitude = this->longitude.to_double();
		return QGeoCoordinate(latitude, longitude);
	}

	constexpr QPoint to_point() const
	{
		return QPoint(this->get_longitude().to_int(), this->get_latitude().to_int());
	}

	constexpr QPoint to_circle_point() const
	{
		return QPoint(centesimal_int(this->get_longitude()).get_value(), centesimal_int(this->get_latitude()).get_value() * 2 * -1);
	}

	QPoint to_circle_edge_point() const;

	std::string to_string() const
	{
		return "(" + this->get_longitude().to_string() + ", " + this->get_latitude().to_string() + ")";
	}

	constexpr bool operator ==(const geocoordinate &other) const
	{
		return this->get_longitude() == other.get_longitude() && this->get_latitude() == other.get_latitude();
	}

	constexpr bool operator !=(const geocoordinate &other) const
	{
		return !((*this) == other);
	}

	constexpr bool operator <(const geocoordinate &other) const
	{
		if (this->get_longitude() != other.get_longitude()) {
			return this->get_longitude() < other.get_longitude();
		}

		return this->get_latitude() < other.get_latitude();
	}

private:
	number_type longitude;
	number_type latitude;
};

using longitude = geocoordinate::number_type;
using latitude = geocoordinate::number_type;

struct geocoordinate_compare final
{
	bool operator()(const geocoordinate &geocoordinate, const wyrmgus::geocoordinate &other_geocoordinate) const;
};

using geocoordinate_set = std::set<geocoordinate, geocoordinate_compare>;

template <typename T>
using geocoordinate_map = std::map<geocoordinate, T, geocoordinate_compare>;

}

Q_DECLARE_METATYPE(wyrmgus::geocoordinate)
