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
/**@name vec2i.h - Vec2i headerfile. */
//
//      (c) Copyright 2010-2021 by Joris Dauphin and Andrettin
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

template <typename T>
class Vec2T final
{
public:
	constexpr Vec2T() : x(0), y(0) {}
	constexpr Vec2T(T x, T y) : x(x), y(y) {}

	template <typename T2>
	constexpr Vec2T(Vec2T<T2> v) : x(v.x), y(v.y) {}

	constexpr Vec2T(const QPoint &point) : x(point.x()), y(point.y())
	{
	}

	constexpr Vec2T(const QSize &size) : x(size.width()), y(size.height())
	{
	}

	constexpr operator QPoint() const
	{
		return QPoint(this->x, this->y);
	}

	constexpr operator QSize() const
	{
		return QSize(this->x, this->y);
	}

public:
	T x;
	T y;
};

template <typename T, typename T2>
inline bool operator == (const Vec2T<T> &lhs, const Vec2T<T2> &rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T, typename T2>
inline bool operator != (const Vec2T<T> &lhs, const Vec2T<T2> &rhs)
{
	return !(lhs == rhs);
}

template <typename T, typename T2>
inline bool operator < (const Vec2T<T> &lhs, const Vec2T<T2> &rhs)
{
	return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
}

template <typename T>
inline const Vec2T<T> &operator += (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator += (Vec2T<T> &lhs, int rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator -= (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator -= (Vec2T<T> &lhs, int rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator *= (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator *= (Vec2T<T> &lhs, int rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator *= (Vec2T<T> &lhs, const QSize &rhs)
{
	lhs.x *= rhs.width();
	lhs.y *= rhs.height();
	return lhs;
}

template <typename T>
inline const Vec2T<T> &operator /= (Vec2T<T> &lhs, int rhs)
{
	lhs.x /= rhs;
	lhs.y /= rhs;
	return lhs;
}

template <typename T>
inline Vec2T<T> operator + (const Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);

	res += rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator + (const Vec2T<T> &lhs, int rhs)
{
	Vec2T<T> res(lhs);

	res += rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator - (const Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);

	res -= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator - (const Vec2T<T> &lhs, int rhs)
{
	Vec2T<T> res(lhs);

	res -= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator * (const Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);

	res *= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator * (const Vec2T<T> &lhs, int rhs)
{
	Vec2T<T> res(lhs);

	res *= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator * (int lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(rhs);

	res *= lhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator * (const Vec2T<T> &lhs, const QSize &rhs)
{
	Vec2T<T> res(lhs);

	res *= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator / (const Vec2T<T> &lhs, int rhs)
{
	Vec2T<T> res(lhs);

	res /= rhs;
	return res;
}

using Vec2i = Vec2T<int>;
using PixelPos = Vec2T<int>;
using PixelDiff = Vec2T<int>;
using PixelSize = Vec2T<int>;
using PixelPrecise = Vec2T<double>;
