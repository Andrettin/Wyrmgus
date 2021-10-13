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

inline constexpr const QSize &operator *=(QSize &lhs, const QSize &rhs)
{
	lhs.setWidth(lhs.width() * rhs.width());
	lhs.setHeight(lhs.height() * rhs.height());
	return lhs;
}

inline constexpr QSize operator *(const QSize &lhs, const QSize &rhs)
{
	QSize res(lhs);
	res *= rhs;
	return res;
}

inline constexpr const QSize &operator /=(QSize &lhs, const QSize &rhs)
{
	lhs.setWidth(lhs.width() / rhs.width());
	lhs.setHeight(lhs.height() / rhs.height());
	return lhs;
}

inline constexpr const QSize &operator /=(QSize &lhs, const int rhs)
{
	lhs.setWidth(lhs.width() / rhs);
	lhs.setHeight(lhs.height() / rhs);
	return lhs;
}

inline constexpr const QSize &operator /=(QSize &lhs, const int64_t rhs)
{
	return lhs /= static_cast<int>(rhs);
}

inline constexpr QSize operator /(const QSize &lhs, const QSize &rhs)
{
	QSize res(lhs);
	res /= rhs;
	return res;
}

inline constexpr QSize operator /(const QSize &lhs, const int rhs)
{
	QSize res(lhs);
	res /= rhs;
	return res;
}

inline constexpr QSize operator /(const QSize &lhs, const int64_t rhs)
{
	return lhs / static_cast<int>(rhs);
}
