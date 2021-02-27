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

#include "util/number_util.h"

namespace wyrmgus {

//a fractional number with fixed decimal precision, implemented via an underlying integer
template <int N>
class fractional_int final
{
public:
	static constexpr int64_t divisor = number::pow(10, N);

	static fractional_int from_value(const int64_t value)
	{
		fractional_int n;
		n.value = value;
		return n;
	}

	static std::string to_rest_string(int rest)
	{
		std::string rest_str;
		rest_str.resize(N);

		rest = std::abs(rest);
		if (rest > 0) {
			rest_str += ".";
			int divisor = fractional_int::divisor;
			for (int i = 0; i < N; ++i) {
				if (rest == 0) {
					break;
				}

				rest_str += std::to_string(rest / divisor);
				rest %= divisor;
				divisor /= 10;
			}
		}

		return rest_str;
	}

	constexpr fractional_int()
	{
	}

	template <int N2>
	explicit constexpr fractional_int(const fractional_int<N2> &other)
	{
		this->value = other.get_value();
		if constexpr (N != N2) {
			this->value *= fractional_int<N>::divisor;
			this->value /= fractional_int<N2>::divisor;
		}
	}

	explicit constexpr fractional_int(const int n)
	{
		this->value = n * fractional_int::divisor;
	}

	explicit constexpr fractional_int(const double n)
	{
		this->value = static_cast<int64_t>(std::round(n * fractional_int::divisor));
	}

	explicit fractional_int(const std::string &str)
	{
		try {
			size_t decimal_point_pos = str.find('.');
			int integer = 0;
			int fraction = 0;
			if (decimal_point_pos != std::string::npos) {
				integer = std::stoi(str.substr(0, decimal_point_pos));
				const size_t decimal_pos = decimal_point_pos + 1;
				const size_t decimal_places = std::min(str.length() - decimal_pos, static_cast<size_t>(N));
				fraction = std::stoi(str.substr(decimal_pos, decimal_places));
				if (decimal_places < N) {
					for (int i = static_cast<int>(decimal_places); i < N; ++i) {
						fraction *= 10;
					}
				}
				const bool negative = str.front() == '-';
				if (negative) {
					fraction *= -1;
				}
			} else {
				integer = std::stoi(str);
			}

			for (int i = 0; i < N; ++i) {
				integer *= 10;
			}
			integer += fraction;

			this->value = integer;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to convert the fractional number string \"" + str + "\" to an int."));
		}
	}

	constexpr int64_t get_value() const
	{
		return this->value;
	}

	constexpr int to_int() const
	{
		return static_cast<int>(this->value / fractional_int::divisor);
	}

	constexpr double to_double() const
	{
		return static_cast<double>(this->value) / fractional_int::divisor;
	}

	constexpr QTime to_time() const
	{
		const int hours = this->to_int();
		int64_t rest = this->get_value() % fractional_int::divisor;
		const int minutes = rest * 60 / fractional_int::divisor;
		rest -= minutes * fractional_int::divisor / 60;
		const int seconds = rest * 60 * 60 / fractional_int::divisor;
		rest -= seconds * fractional_int::divisor / 60 / 60;
		const int milliseconds = rest * 60 * 60 / (fractional_int::divisor / 1000);

		return QTime(hours, minutes, seconds, milliseconds);
	}

	std::string to_string() const
	{
		std::string number_str = std::to_string(this->value / fractional_int::divisor);
		number_str += fractional_int::to_rest_string(this->value % fractional_int::divisor);
		return number_str;
	}

	std::string to_percent_string() const
	{
		static constexpr int N2 = (N > 2) ? (N - 2) : 0;

		const int percent_value = this->value * 100;
		std::string number_str = std::to_string(percent_value / fractional_int<N2>::divisor);
		number_str += fractional_int<N2>::to_rest_string(percent_value % fractional_int<N2>::divisor);
		return number_str;
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

	constexpr bool operator <(const int other) const
	{
		return this->to_int() < other;
	}

	constexpr bool operator <=(const fractional_int<N> &other) const
	{
		return this->value <= other.value;
	}

	constexpr bool operator >(const fractional_int<N> &other) const
	{
		return this->value > other.value;
	}

	constexpr bool operator >(const int other) const
	{
		const int int_value = this->to_int();
		return int_value > other || (int_value == other && (this->value % fractional_int::divisor) > 0);
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

	constexpr const fractional_int<N> &operator +=(const int other)
	{
		this->value += other * fractional_int<N>::divisor;
		return *this;
	}

	constexpr const fractional_int<N> &operator -=(const fractional_int<N> &other)
	{
		this->value -= other.value;
		return *this;
	}

	constexpr const fractional_int<N> &operator -=(const int other)
	{
		this->value -= other * fractional_int<N>::divisor;
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

	constexpr fractional_int<N> operator +(const int other) const
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

	constexpr fractional_int<N> operator -(const int other) const
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

	friend constexpr fractional_int<N> operator -(const int lhs, const fractional_int<N> &rhs)
	{
		fractional_int res(lhs);
		res -= rhs;
		return res;
	}

	friend constexpr fractional_int<N> operator *(const int lhs, const fractional_int<N> &rhs)
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
using decimillesimal_int = fractional_int<4>;

}

Q_DECLARE_METATYPE(wyrmgus::centesimal_int)
Q_DECLARE_METATYPE(wyrmgus::decimillesimal_int)
