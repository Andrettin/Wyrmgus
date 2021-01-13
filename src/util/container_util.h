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

namespace wyrmgus::container {

template <typename T, typename U>
inline bool intersects_with(const T &container, const U &other_container)
{
	for (const typename T::value_type &element : container) {
		for (const typename U::value_type &other_element : other_container) {
			if (element == other_element) {
				return true;
			}
		}
	}

	return false;
}

template <typename T>
inline QVariantList to_qvariant_list(const T &container)
{
	QVariantList list;

	for (const typename T::value_type &element : container) {
		if constexpr (std::is_same_v<typename T::value_type, std::filesystem::path>) {
			list.append(QVariant::fromValue(QString::fromStdString(element.string())));
		} else if constexpr (std::is_same_v<typename T::value_type, std::string>) {
			list.append(QVariant::fromValue(QString::fromStdString(element)));
		} else {
			list.append(QVariant::fromValue(element));
		}
	}

	return list;
}

template <typename T>
QStringList to_qstring_list(const T &string_container)
{
	static_assert(std::is_same_v<typename T::value_type, std::string>);

	QStringList qstring_list;
	for (const std::string &str : string_container) {
		qstring_list.push_back(QString::fromStdString(str));
	}
	return qstring_list;
}

template <typename T>
inline std::set<typename T::value_type> to_set(const T &container)
{
	std::set<typename T::value_type> set;

	for (const typename T::value_type &element : container) {
		set.insert(element);
	}

	return set;
}

template <typename T>
inline std::queue<typename T::value_type> to_queue(const T &container)
{
	std::queue<typename T::value_type> queue;

	for (const typename T::value_type &element : container) {
		queue.push(element);
	}

	return queue;
}

template <typename T>
inline std::vector<typename T::value_type> to_vector(const T &container)
{
	std::vector<typename T::value_type> vector;
	vector.reserve(container.size());

	for (const typename T::value_type &element : container) {
		vector.push_back(element);
	}

	return vector;
}

}
