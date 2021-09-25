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

#include "util/geocoordinate.h"

namespace wyrmgus {

class georectangle final
{
public:
	using number_type = geocoordinate::number_type;

	static georectangle from_qgeorectangle(const QGeoRectangle &qgeorectangle)
	{
		const QGeoCoordinate bottom_left = qgeorectangle.bottomLeft();
		const QGeoCoordinate top_right = qgeorectangle.topRight();

		return georectangle(geocoordinate(bottom_left), geocoordinate(top_right));
	}

	explicit constexpr georectangle(const geocoordinate &min_geocoordinate, const geocoordinate &max_geocoordinate)
		: min_geocoordinate(min_geocoordinate), max_geocoordinate(max_geocoordinate)
	{
	}

	constexpr const geocoordinate &get_min_geocoordinate() const
	{
		return this->min_geocoordinate;
	}

	constexpr const geocoordinate &get_max_geocoordinate() const
	{
		return this->max_geocoordinate;
	}

	constexpr const decimillesimal_int &get_min_longitude() const
	{
		return this->get_min_geocoordinate().get_longitude();
	}

	constexpr void set_min_longitude(const decimillesimal_int &lon)
	{
		this->min_geocoordinate.set_longitude(lon);
	}

	constexpr const decimillesimal_int &get_max_longitude() const
	{
		return this->get_max_geocoordinate().get_longitude();
	}

	constexpr void set_max_longitude(const decimillesimal_int &lon)
	{
		this->max_geocoordinate.set_longitude(lon);
	}

	constexpr const decimillesimal_int &get_min_latitude() const
	{
		return this->get_min_geocoordinate().get_latitude();
	}

	constexpr void set_min_latitude(const decimillesimal_int &lat)
	{
		this->min_geocoordinate.set_latitude(lat);
	}

	constexpr const decimillesimal_int &get_max_latitude() const
	{
		return this->get_max_geocoordinate().get_latitude();
	}

	constexpr void set_max_latitude(const decimillesimal_int &lat)
	{
		this->max_geocoordinate.set_latitude(lat);
	}

	constexpr number_type get_width() const
	{
		return this->get_max_geocoordinate().get_longitude() - this->get_min_geocoordinate().get_longitude();
	}

	QGeoRectangle to_qgeorectangle() const
	{
		return QGeoRectangle(QGeoCoordinate(this->get_max_latitude().to_double(), this->get_min_longitude().to_double()), QGeoCoordinate(this->get_min_latitude().to_double(), this->get_max_longitude().to_double()));
	}

	constexpr bool operator ==(const georectangle &other) const
	{
		return this->get_min_geocoordinate() == other.get_min_geocoordinate() && this->get_max_geocoordinate() == other.get_max_geocoordinate();
	}

private:
	geocoordinate min_geocoordinate;
	geocoordinate max_geocoordinate;
};

}
