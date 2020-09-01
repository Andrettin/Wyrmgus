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
//      (c) Copyright 2019-2020 by Andrettin
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

namespace string {

static inline const std::string empty_str;

//case-insensitive find
inline size_t ci_find(const std::string &str, const std::string &find)
{
	auto iterator = std::search(str.begin(), str.end(), find.begin(), find.end(), [](const char c1, const char c2) {
		return std::toupper(c1) == std::toupper(c2);
	});

	if (iterator == str.end()) {
		return std::string::npos;
	}

	return iterator - str.begin();
}

inline std::vector<std::string> split(const std::string &str, const char delimiter)
{
	std::vector<std::string> string_list;

	size_t start_pos = 0;
	size_t find_pos = 0;
	while ((find_pos = str.find(delimiter, start_pos)) != std::string::npos) {
		std::string string_element = str.substr(start_pos, find_pos - start_pos);
		string_list.push_back(string_element);
		start_pos = find_pos + 1;
	}

	std::string string_element = str.substr(start_pos, str.length() - start_pos);
	string_list.push_back(string_element);

	return string_list;
}

inline void replace(std::string &str, const std::string &find, const std::string &replace)
{
	size_t pos = 0;
	while ((pos = str.find(find, pos)) != std::string::npos) {
		str.replace(pos, find.length(), replace);
		pos += replace.length();
	}
}

inline std::string to_lower(const std::string &str)
{
	std::string result(str);

	std::transform(result.begin(), result.end(), result.begin(), [](const char c) {
		return std::tolower(c);
	});

	return result;
}

inline std::string to_upper(const std::string &str)
{
	std::string result(str);

	std::transform(result.begin(), result.end(), result.begin(), [](const char c) {
		return std::toupper(c);
	});

	return result;
}

inline void capitalize(std::string &str)
{
	if (str.empty()) {
		return;
	}

	str[0] = toupper(str[0]);
}

inline bool to_bool(const std::string &str)
{
	if (str == "true" || str == "yes" || str == "1") {
		return true;
	} else if (str == "false" || str == "no" || str == "0") {
		return false;
	}

	throw std::runtime_error("Invalid string used for conversion to boolean: \"" + str + "\".");
}

inline bool is_number(const std::string &str)
{
	bool digit_found = false;

	for (size_t i = 0; i < str.size(); ++i) {
		const char c = str[i];

		if (c == '-' && i == 0) {
			continue;
		}

		if (std::isdigit(c)) {
			digit_found = true;
			continue;
		}

		return false;
	}

	return digit_found;
}

inline QDateTime to_date(const std::string &date_str)
{
	const std::vector<std::string> date_string_list = string::split(date_str, '.');

	int years = 0;
	int months = 1;
	int days = 1;
	int hours = 12;

	if (date_string_list.size() >= 1) {
		years = std::stoi(date_string_list[0]);

		if (date_string_list.size() >= 2) {
			months = std::stoi(date_string_list[1]);

			if (date_string_list.size() >= 3) {
				days = std::stoi(date_string_list[2]);

				if (date_string_list.size() >= 4) {
					hours = std::stoi(date_string_list[3]);
				}
			}
		}
	}

	QDateTime date(QDate(years, months, days), QTime(hours, 0), Qt::UTC);

	if (!date.isValid()) {
		throw std::runtime_error("Date \"" + date_str + "\" is not a valid date.");
	}

	return date;
}

inline std::string get_singular_form(const std::string &str)
{
	if (str.substr(str.size() - 4, 4) == "sses") {
		return str.substr(0, str.size() - 2); //e.g. "classes"
	} else if (str.substr(str.size() - 2, 2) == "ys") {
		return str.substr(0, str.size() - 2);
	} else if (str.substr(str.size() - 3, 3) == "ies") {
		if (str != "species") {
			return str.substr(0, str.size() - 3) + "y";
		}
	} else if (str.substr(str.size() - 1, 1) == "s") {
		return str.substr(0, str.size() - 1);
	}

	return str;
}

inline std::string get_plural_form(const std::string &str)
{
	if (str == "Einherjar" || str == "Wose") {
		return str; // no difference
	}

	std::string result(str);

	if (result.ends_with("y") && result != "Monkey") {
		string::replace(result, "y", "ies");
	} else if (result.ends_with("ch") || result.ends_with("os") || result.ends_with("ps") || result.ends_with("sh") || result.ends_with("us") || result.ends_with("x")) {
		result += "es";
	} else if (result.ends_with("man") && result != "Human") {
		string::replace(result, "man", "men");
	} else if (result.ends_with("f")) {
		string::replace(result, "f", "ves");
	} else if (!result.ends_with("s")) {
		result += "s";
	}

	return result;
}

inline std::string snake_case_to_pascal_case(const std::string &str)
{
	if (str.empty()) {
		return str;
	}

	if (str.empty()) {
		return str;
	}

	std::string result;
	result += static_cast<char>(toupper(str[0]));

	for (size_t pos = 1; pos < str.length(); ++pos) {
		if (str[pos] == '_') {
			++pos;
			result += static_cast<char>(toupper(str[pos]));
		} else {
			result += str[pos];
		}
	}

	return result;
}

inline std::string highlight(const std::string &str)
{
	return "~<" + str + "~>";
}

}
