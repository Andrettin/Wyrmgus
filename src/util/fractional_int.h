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
//      (c) Copyright 2019-2021 by Andrettin
//
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "util/number_util.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

//a fractional number with fixed decimal precision, implemented via an underlying integer
template <int N>
class fractional_int final
{
public:
	static constexpr int64_t divisor = number::pow(10, N);

	constexpr fractional_int()
	{
	}

	explicit constexpr fractional_int(const std::string &str)
	{
		this->value = string::fractional_number_string_to_int<N>(str);
	}

	explicit constexpr fractional_int(const int n)
	{
		this->value = n * fractional_int::divisor;
	}

	constexpr int to_int() const
	{
		return static_cast<int>(this->value / fractional_int::divisor);
	}

	constexpr double to_double() const
	{
		return static_cast<double>(this->value) / fractional_int::divisor;
	}

	constexpr bool operator ==(const fractional_int<N> &other) const
	{
		return this->value == other.value;
	}

	constexpr bool operator ==(const int other) const
	{
		return (this->value / fractional_int::divisor) == other && (this->value % fractional_int::divisor) == 0;
	}

	constexpr bool operator !=(const fractional_int<N> &other) const
	{
		return this->value != other.value;
	}

	constexpr bool operator <(const fractional_int<N> &other) const
	{
		return this->value < other.value;
	}

	constexpr bool operator <=(const fractional_int<N> &other) const
	{
		return this->value <= other.value;
	}

	constexpr bool operator >(const fractional_int<N> &other) const
	{
		return this->value > other.value;
	}

	constexpr bool operator >=(const fractional_int<N> &other) const
	{
		return this->value >= other.value;
	}

	constexpr const fractional_int<N> &operator +=(const fractional_int<N> &other)
	{
		this->value += other.value;
		return *this;
	}

	constexpr const fractional_int<N> &operator -=(const fractional_int<N> &other)
	{
		this->value -= other.value;
		return *this;
	}

	template <int N2>
	constexpr const fractional_int<N> &operator *=(const fractional_int<N2> &other)
	{
		this->value *= other.value;
		this->value /= fractional_int<N2>::divisor;
		return *this;
	}

	constexpr const fractional_int<N> &operator *=(const int other)
	{
		this->value *= other;
		return *this;
	}

	template <int N2>
	constexpr const fractional_int<N> &operator /=(const fractional_int<N2> &other)
	{
		this->value *= fractional_int<N2>::divisor;
		this->value /= other.value;
		return *this;
	}

	constexpr const fractional_int<N> &operator /=(const int other)
	{
		this->value /= other;
		return *this;
	}

	constexpr fractional_int<N> operator +(const fractional_int<N> &other) const
	{
		fractional_int res(*this);
		res += other;
		return res;
	}

	constexpr fractional_int<N> operator -(const fractional_int<N> &other) const
	{
		fractional_int res(*this);
		res -= other;
		return res;
	}

	template <int N2>
	constexpr fractional_int<N> operator *(const fractional_int<N2> &other) const
	{
		fractional_int res(*this);
		res *= other;
		return res;
	}

	constexpr fractional_int<N> operator *(const int other) const
	{
		fractional_int res(*this);
		res *= other;
		return res;
	}

	template <int N2>
	constexpr fractional_int<N> operator /(const fractional_int<N2> &other) const
	{
		fractional_int res(*this);
		res /= other;
		return res;
	}

	constexpr fractional_int<N> operator /(const int other) const
	{
		fractional_int res(*this);
		res /= other;
		return res;
	}

	friend constexpr const fractional_int<N> &operator *=(int &lhs, const fractional_int<N> &rhs)
	{
		fractional_int res(rhs);
		res *= lhs;
		return res;
	}

	friend constexpr int operator *(const int lhs, const fractional_int<N> &rhs)
	{
		fractional_int res(rhs);
		res *= lhs;
		return res;
	}

private:
	int64_t value = 0;
};

using decimal_int = fractional_int<1>;
using centesimal_int = fractional_int<2>;
using millesimal_int = fractional_int<3>;

}
