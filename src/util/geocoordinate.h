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
//

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

	static constexpr geocoordinate from_unsigned_geocoordinate(const geocoordinate &unsigned_geocoordinate)
	{
		const number_type lon = unsigned_geocoordinate.get_longitude() - number_type(geocoordinate::longitude_size / 2);
		const number_type lat = (unsigned_geocoordinate.get_latitude() - number_type(geocoordinate::latitude_size / 2)) * -1;
		return geocoordinate(lon, lat);
	}

	static constexpr number_type longitude_per_pixel(const int lon_size, const QSize &size)
	{
		return number_type(lon_size) / size.width();
	}

	static constexpr number_type latitude_per_pixel(const int lat_size, const QSize &size)
	{
		return number_type(lat_size) / size.height();
	}

	static constexpr int unsigned_longitude_to_x(const number_type &unsigned_longitude, const number_type &lon_per_pixel)
	{
		const number_type x = unsigned_longitude / lon_per_pixel;
		return x.to_int();
	}

	static constexpr int unsigned_latitude_to_y(const number_type &unsigned_latitude, const number_type &lat_per_pixel)
	{
		const number_type y = unsigned_latitude / lat_per_pixel;
		return y.to_int();
	}

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
		return QGeoCoordinate(this->latitude.to_double(), this->longitude.to_double());
	}

	constexpr number_type longitude_to_unsigned_longitude() const
	{
		return this->get_longitude() + number_type(geocoordinate::longitude_size / 2);
	}

	constexpr number_type latitude_to_unsigned_latitude() const
	{
		return this->get_latitude() * -1 + number_type(geocoordinate::latitude_size / 2);
	}

	geocoordinate to_unsigned_geocoordinate() const
	{
		//converts a geocoordinate into an unsigned one, i.e. having x values from 0 to 360, and y values from 0 to 180 (top to bottom, contrary to geocoordinates, which work south to north)
		const number_type x = this->longitude_to_unsigned_longitude();
		const number_type y = this->latitude_to_unsigned_latitude();
		return geocoordinate(x, y);
	}

	constexpr int longitude_to_x(const number_type &lon_per_pixel) const
	{
		const number_type x = this->longitude_to_unsigned_longitude();
		return geocoordinate::unsigned_longitude_to_x(x, lon_per_pixel);
	}

	constexpr int latitude_to_y(const number_type &lat_per_pixel) const
	{
		const number_type y = this->latitude_to_unsigned_latitude();
		return geocoordinate::unsigned_latitude_to_y(y, lat_per_pixel);
	}

	constexpr QPoint to_point() const
	{
		return QPoint(this->get_longitude().to_int(), this->get_latitude().to_int());
	}

	constexpr QPoint to_point(const number_type &lon_per_pixel, const number_type &lat_per_pixel) const
	{
		const int x = this->longitude_to_x(lon_per_pixel);
		const int y = this->latitude_to_y(lat_per_pixel);
		return QPoint(x, y);
	}

	constexpr QPoint to_point(const QRect &georectangle, const QSize &area_size) const
	{
		const number_type lon_per_pixel = geocoordinate::longitude_per_pixel(georectangle.width(), area_size);
		const number_type lat_per_pixel = geocoordinate::latitude_per_pixel(georectangle.height(), area_size);
		const QPoint geocoordinate_offset = geocoordinate(georectangle.bottomLeft()).to_point(lon_per_pixel, lat_per_pixel);
		return this->to_point(lon_per_pixel, lat_per_pixel) - geocoordinate_offset;
	}

	constexpr QPoint to_circle_point() const
	{
		return QPoint(centesimal_int(this->get_longitude()).get_value(), centesimal_int(this->get_latitude()).get_value() * 2 * -1);
	}

	QPoint to_circle_edge_point() const;

	constexpr bool operator ==(const geocoordinate &other) const
	{
		return this->get_longitude() == other.get_longitude() && this->get_latitude() == other.get_latitude();
	}

	constexpr bool operator !=(const geocoordinate &other) const
	{
		return !((*this) == other);
	}

private:
	number_type longitude;
	number_type latitude;
};

using longitude = geocoordinate::number_type;
using latitude = geocoordinate::number_type;

}

Q_DECLARE_METATYPE(wyrmgus::geocoordinate)
