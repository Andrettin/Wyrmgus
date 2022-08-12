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
//      (c) Copyright 2005-2022 by Jimmy Salmon and Andrettin
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

#include "util/singleton.h"

namespace wyrmgus {

class translator final : public QTranslator, public singleton<translator>
{
	Q_OBJECT

public:
	virtual bool isEmpty() const override
	{
		return this->entries.empty();
	}

	void set_locale(const std::string &locale_id);

	virtual QString translate(const char *context, const char *source_text, const char *disambiguation, const int n) const override
	{
		Q_UNUSED(context);
		Q_UNUSED(disambiguation);
		Q_UNUSED(n);

		return this->translate(source_text);
	}

	const std::string &translate(const std::string &source_text) const
	{
		try {
			const auto find_iterator = this->entries.find(source_text);
			if (find_iterator != this->entries.end()) {
				return find_iterator->second;
			}

			return source_text;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to get translation for string \"" + source_text + "\"."));
		}
	}

	const char *translate(const char *source_text) const
	{
		try {
			const auto find_iterator = this->entries.find(source_text);
			if (find_iterator != this->entries.end()) {
				return find_iterator->second.c_str();
			}

			return source_text;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to get translation for string \"" + std::string(source_text) + "\"."));
		}
	}

private:
	void load_po(const std::filesystem::path &filepath);

	void add_translation(std::string &&str1, std::string &&str2)
	{
		this->entries[str1] = str2;
	}

signals:
	void locale_changed();

private:
	std::unordered_map<std::string, std::string> entries;
};

}

/// Translate a string
extern const char *Translate(const char *str);
extern const std::string &Translate(const std::string &str);

#define _(str) Translate(str)
#define N_(str) str
