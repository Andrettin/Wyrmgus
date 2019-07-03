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
/**@name vec2i.h - Vec2i header file. */
//
//      (c) Copyright 2010-2019 by Joris Dauphin and Andrettin
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

#ifndef VEC2I_H
#define VEC2I_H

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <core/math/vector2.h>

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

template <typename T>
class Vec2T
{
public:
	Vec2T() {}
	Vec2T(T x, T y) : x(x), y(y) {}

	template <typename T2>
	Vec2T(Vec2T<T2> v) : x(v.x), y(v.y) {}
	
	Vec2T(const Vector2i &v) : x(v.x), y(v.y) {}
	
	operator Vector2i() const
	{
		return Vector2i(this->x, this->y);
	}
	
	operator Vector2() const
	{
		return Vector2(this->x, this->y);
	}
	
	Vec2T ToRelativePoint(const Vec2T &origin_end_pos, const Vec2T &dest_end_pos) const
	{
		return *this * dest_end_pos / origin_end_pos;
	}
	
	Vec2T ToRelativePoint(const std::pair<Vec2T, Vec2T> &origin_rect, const std::pair<Vec2T, Vec2T> &dest_rect) const
	{
		return this->ToRelativePoint(origin_rect.second - origin_rect.first, dest_rect.second - dest_rect.first);
	}
	
public:
	T x = 0;
	T y = 0;
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
inline const Vec2T<T> &operator /= (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
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
inline Vec2T<T> operator / (const Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);

	res /= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator / (const Vec2T<T> &lhs, int rhs)
{
	Vec2T<T> res(lhs);

	res /= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator / (int lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(rhs);

	res /= lhs;
	return res;
}

template <typename T>
inline int SquareDistance(const Vec2T<T> &pos1, const Vec2T<T> &pos2)
{
	const Vec2T<T> diff = pos2 - pos1;

	return diff.x * diff.x + diff.y * diff.y;
}

inline int SquareDistance(const Vector2i &pos1, const Vector2i &pos2)
{
	const Vector2i diff = pos2 - pos1;

	return diff.x * diff.x + diff.y * diff.y;
}

template <typename T>
inline int Distance(const Vec2T<T> &pos1, const Vec2T<T> &pos2)
{
	return isqrt(SquareDistance(pos1, pos2));
}

inline int Distance(const Vector2i &pos1, const Vector2i &pos2)
{
	return isqrt(SquareDistance(pos1, pos2));
}

typedef Vec2T<short int> Vec2i;
typedef Vec2T<int> PixelPos;
typedef Vec2T<int> PixelDiff;
typedef Vec2T<int> PixelSize;
typedef Vec2T<double> PixelPrecise;

#endif
