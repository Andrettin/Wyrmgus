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
//      (c) Copyright 2021-2022 by Andrettin
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

namespace wyrmgus::path {

inline std::string to_string(const std::filesystem::path &path)
{
#ifdef __clang__
	return path.string();
#else
	//convert a path to a UTF-8 encoded string
	const std::u8string u8str = path.u8string();
	return std::string(u8str.begin(), u8str.end());
#endif
}

inline std::filesystem::path from_string(const std::string &path_str)
{
#ifdef __clang__
	return std::filesystem::path(path_str);
#else
	//convert a UTF-8 encoded string to a path
	const std::u8string u8str(path_str.begin(), path_str.end());
	return std::filesystem::path(u8str);
#endif
}

inline QString to_qstring(const std::filesystem::path &path)
{
#ifdef __clang__
	return QString::fromStdString(path.string());
#else
	const std::u8string u8str = path.u8string();
	return QString::fromUtf8(reinterpret_cast<const char *>(u8str.c_str()));
#endif
}

inline std::filesystem::path from_qstring(const QString &path_str)
{
#ifdef USE_WIN32
	return std::filesystem::path(path_str.toStdU16String());
#else
	return std::filesystem::path(path_str.toStdString());
#endif
}

inline std::filesystem::path from_qurl(const QUrl &url)
{
	return path::from_qstring(url.toLocalFile());
}

}

Q_DECLARE_METATYPE(std::filesystem::path)
