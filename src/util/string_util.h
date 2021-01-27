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

namespace wyrmgus::string {

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

template <typename string_list_type = std::vector<std::string>>
inline string_list_type split(const std::string &str, const char delimiter)
{
	string_list_type string_list{};

	size_t start_pos = 0;
	size_t find_pos = 0;
	while ((find_pos = str.find(delimiter, start_pos)) != std::string::npos) {
		std::string string_element = str.substr(start_pos, find_pos - start_pos);

		if constexpr (std::is_same_v<string_list_type, std::queue<std::string>>) {
			string_list.push(std::move(string_element));
		} else {
			string_list.push_back(std::move(string_element));
		}

		start_pos = find_pos + 1;
	}

	std::string string_element = str.substr(start_pos, str.length() - start_pos);

	if constexpr (std::is_same_v<string_list_type, std::queue<std::string>>) {
		string_list.push(std::move(string_element));
	} else {
		string_list.push_back(std::move(string_element));
	}

	return string_list;
}

inline std::queue<std::string> split_to_queue(const std::string &str, const char delimiter)
{
	return string::split<std::queue<std::string>>(str, delimiter);
}

inline void replace(std::string &str, const std::string_view &find, const std::string_view &replace)
{
	size_t pos = 0;
	while ((pos = str.find(find, pos)) != std::string::npos) {
		str.replace(pos, find.length(), replace);
		pos += replace.length();
	}
}

inline void replace(std::string &str, const char find, const char replace)
{
	size_t pos = 0;
	while ((pos = str.find(find, pos)) != std::string::npos) {
		str.at(pos) = replace;
	}
}

inline void to_lower(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](const char c) {
		return std::tolower(c);
	});
}

inline std::string lowered(std::string &&str)
{
	string::to_lower(str);
	return str;
}

inline std::string lowered(const std::string &str)
{
	std::string result(str);
	return string::lowered(std::move(result));
}

inline void to_upper(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](const char c) {
		return std::toupper(c);
	});
}

inline std::string uppered(std::string &&str)
{
	string::to_upper(str);
	return str;
}

inline std::string uppered(const std::string &str)
{
	std::string result(str);
	return string::uppered(std::move(result));
}

inline void capitalize(std::string &str)
{
	if (str.empty()) {
		return;
	}

	str[0] = toupper(str[0]);
}

inline void normalize(std::string &str)
{
	//remove special characters from the string that shouldn't be displayed even in strings for which accented characters are acceptable

	//remove macrons
	string::replace(str, "Ā", "A");
	string::replace(str, "ā", "a");
	string::replace(str, "Ē", "E");
	string::replace(str, "ē", "e");
	string::replace(str, "Ī", "I");
	string::replace(str, "ī", "i");
	string::replace(str, "Ō", "O");
	string::replace(str, "ō", "o");
	string::replace(str, "Ū", "U");
	string::replace(str, "ū", "u");

	//replace superscript versions of characters with their normal versions
	string::replace(str, "ʷ", "w");
}

inline std::string normalized(std::string &&str)
{
	string::normalize(str);
	return str;
}

inline std::string normalized(const std::string &str)
{
	std::string result(str);
	return string::normalized(std::move(result));
}

inline void escape(std::string &str)
{
	//escape special characters
	string::replace(str, "\n", "\\n");
	string::replace(str, "\t", "\\t");
	string::replace(str, "\"", "\\\"");
}

inline std::string escaped(std::string &&str)
{
	string::escape(str);
	return str;

}

inline std::string escaped(const std::string &str)
{
	std::string result(str);
	return string::escaped(std::move(result));

}

inline bool is_bool(const std::string &str)
{
	return str == "true" || str == "false";
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
	} else if (result.ends_with("mouse")) {
		string::replace(result, "mouse", "mice");
	} else if (result.ends_with("Mouse")) {
		string::replace(result, "Mouse", "Mice");
	}

	return result;
}

std::string get_indefinite_article(const std::string &str);

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

inline std::string to_tooltip(const std::string &str)
{
	std::string tooltip = str;
	string::replace(tooltip, "\t", "    "); //the game can't count character width correctly for tabs
	return tooltip;
}

}

Q_DECLARE_METATYPE(std::string)
