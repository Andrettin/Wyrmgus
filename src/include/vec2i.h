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
//      (c) Copyright 2010 by Joris Dauphin
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

//@{

#include <type_traits>
template <typename T>
class Vec2T
{
public:
	Vec2T() : x(0), y(0) {}
	Vec2T(T x, T y) : x(x), y(y) {}
	Vec2T(T v) : x(v), y(v) {}
public:
	T x;
	T y;
};


template <typename T>
inline bool operator == (const Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
inline bool operator != (const Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	return !(lhs == rhs);
}

template <typename T>
inline Vec2T<T> &operator += (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

template <typename T, typename U, typename = std::enable_if<std::is_integral<U>::value>>
inline Vec2T<T> &operator += (Vec2T<T> &lhs, U rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	return lhs;
}

template <typename T, typename U>
inline Vec2T<T> operator + (const Vec2T<T> &lhs, const U &rhs)
{
	Vec2T<T> res(lhs);
	res += rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator + (const T &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);
	res += rhs;
	return res;
}

template <typename T>
inline Vec2T<T> &operator -= (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

template <typename T, typename U, typename = std::enable_if<std::is_integral<U>::value>>
inline Vec2T<T> &operator -= (Vec2T<T> &lhs, U rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	return lhs;
}

template <typename T, typename U>
inline Vec2T<T> operator - (const Vec2T<T> &lhs, const U &rhs)
{
	Vec2T<T> res(lhs);
	res -= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator - (const T &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);
	res -= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> &operator *= (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	return lhs;
}

template <typename T, typename U, typename = std::enable_if<std::is_integral<U>::value>>
inline Vec2T<T> &operator *= (Vec2T<T> &lhs, U rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	return lhs;
}

template <typename T, typename U>
inline Vec2T<T> operator * (const Vec2T<T> &lhs, const U &rhs)
{
	Vec2T<T> res(lhs);
	res *= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator * (const T &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);
	res *= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> &operator /= (Vec2T<T> &lhs, const Vec2T<T> &rhs)
{
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	return lhs;
}

template <typename T, typename U, typename = std::enable_if<std::is_integral<U>::value>>
inline Vec2T<T> &operator /= (Vec2T<T> &lhs, U rhs)
{
	lhs.x /= rhs;
	lhs.y /= rhs;
	return lhs;
}

template <typename T, typename U>
inline Vec2T<T> operator / (const Vec2T<T> &lhs, const U &rhs)
{
	Vec2T<T> res(lhs);
	res /= rhs;
	return res;
}

template <typename T>
inline Vec2T<T> operator / (const T &lhs, const Vec2T<T> &rhs)
{
	Vec2T<T> res(lhs);
	res /= rhs;
	return res;
}

template <typename T>
inline int SquareDistance(const Vec2T<T> &pos1, const Vec2T<T> &pos2)
{
	const Vec2T<T> diff = pos2 - pos1;

	return diff.x * diff.x + diff.y * diff.y;
}

template <typename T>
inline int Distance(const Vec2T<T> &pos1, const Vec2T<T> &pos2)
{
	return isqrt(SquareDistance(pos1, pos2));
}

typedef Vec2T<short int> Vec2i;
typedef Vec2T<short int> PixelPos;
typedef Vec2T<short int> PixelDiff;
typedef Vec2T<short int> PixelSize;
typedef Vec2T<double> PixelPrecise;

//@}

#endif // !VEC2I_H
