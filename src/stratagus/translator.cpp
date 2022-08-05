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

#include "stratagus.h"

#include "translator.h"

#include "database/database.h"
#include "iolib.h"
#include "util/assert_util.h"
#include "util/path_util.h"

namespace wyrmgus {

void translator::set_locale(const std::string &locale_id)
{
	this->entries.clear();

	if (locale_id.empty()) {
		return;
	}

	const std::string locale_filename = "wyr-" + locale_id + ".po";

	const std::vector<std::filesystem::path> translations_paths = database::get()->get_translations_paths();

	for (const std::filesystem::path &translations_path : translations_paths) {
		if (!std::filesystem::exists(translations_path)) {
			continue;
		}

		const std::filesystem::path translation_filepath = translations_path / locale_filename;

		if (!std::filesystem::exists(translation_filepath)) {
			continue;
		}

		this->load_po(translation_filepath);
	}
}

void translator::load_po(const std::filesystem::path &filepath)
{
	if (filepath.empty()) {
		return;
	}

	assert_throw(filepath.extension() == ".po");

	const std::string filepath_str = path::to_string(filepath);

	FILE *fd = fopen(filepath_str.c_str(), "rb");
	if (!fd) {
		throw std::runtime_error("Could not open file: " + filepath_str);
	}

	enum { MSGNONE, MSGID, MSGSTR } state = MSGNONE;
	std::array<char, 16 * 1024> msgid{};
	std::array<char, 16 * 1024> msgstr{};
	char *currmsg = nullptr;

	msgid[0] = msgstr[0] = '\0';

	// skip 0xEF utf8 intro if found
	char c = fgetc(fd);
	if (c == static_cast<const char>(0xEF)) {
		fgetc(fd);
		fgetc(fd);
	} else {
		rewind(fd);
	}

	std::array<char, 4096> buf{};

	while (fgets(buf.data(), sizeof(buf), fd)) {
		// Comment
		if (buf[0] == '#') {
			continue;
		}
		char *s = buf.data();

		// msgid or msgstr
		if (!strncmp(s, "msgid ", 6)) {
			if (state == MSGSTR) {
				*currmsg = '\0';
				if (msgid.front() != '\0') {
					this->add_translation(msgid.data(), msgstr.data());
				}
			}
			state = MSGID;
			currmsg = msgid.data();
			*currmsg = '\0';
			s += 6;
			while (*s == ' ') { ++s; }
		} else if (!strncmp(s, "msgstr ", 7)) {
			if (state == MSGID) {
				*currmsg = '\0';
			}
			state = MSGSTR;
			currmsg = msgstr.data();
			*currmsg = '\0';
			s += 7;
			while (*s == ' ') { ++s; }
		}

		// String
		if (*s == '"') {
			++s;
			while (*s && *s != '"') {
				if (*s == '\\') {
					++s;
					if (*s) {
						if (*s == 'n') {
							*currmsg++ = '\n';
						} else if (*s == 't') {
							*currmsg++ = '\t';
						} else if (*s == 'r') {
							*currmsg++ = '\r';
						} else if (*s == '"') {
							*currmsg++ = '"';
						} else if (*s == '\\') {
							*currmsg++ = '\\';
						} else {
							fprintf(stderr, "Invalid escape character: %c\n", *s);
						}
						++s;
					} else {
						fprintf(stderr, "Unterminated string\n");
					}
				} else {
					*currmsg++ = *s++;
				}
			}
			continue;
		}
	}

	if (state == MSGSTR) {
		*currmsg = '\0';
		this->add_translation(msgid.data(), msgstr.data());
	}

	fclose(fd);
}

}

const char *Translate(const char *str)
{
	return translator::get()->translate(str);
}

const std::string &Translate(const std::string &str)
{
	return translator::get()->translate(str);
}
