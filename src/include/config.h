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
/**@name config.h - The config header file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "config_property.h"

#include <core/color.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class DataElement;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CConfigData
{
public:
	CConfigData(const std::string &tag) : Tag(tag)
	{
	}
	
	static void ParseConfigData(const std::string &filepath, const bool define_only);
	
private:
	static std::vector<std::string> ParseLine(const std::string &line);
	static bool ParseEscapedCharacter(std::string &current_string, const char character);
	static void ParseTokens(const std::vector<std::string> &tokens, CConfigData **current_config_data, std::vector<CConfigData *> &config_data_elements);
	
public:
	static void ProcessConfigData(const std::vector<CConfigData *> &config_data_list, const bool define_only);
	
	static inline std::map<std::string, std::function<DataElement *(const std::string &)>> DataTypeGetFunctions;	/// functions for getting data type instances, mapped to the string identifier of their respective class
	static inline std::map<std::string, std::function<DataElement *(const std::string &)>> DataTypeGetOrAddFunctions;	/// functions for getting or adding data type instances, mapped to the string identifier of their respective class
	static inline std::map<std::string, std::function<DataElement *(const std::string &)>> DataTypeAddAliasFunctions;	/// functions for adding aliases for data type instances, mapped to the string identifier of their respective class
	
	template <typename T>
	void Process(T &data_element) const
	{
		for (const CConfigProperty &config_property : this->Properties) {
			try {
				if (!config_property.Process(data_element)) {
					fprintf(stderr, "Invalid %s property: \"%s\".\n", this->Tag.c_str(), config_property.Key.c_str());
				}
			} catch (std::exception &exception) {
				fprintf(stderr, "%s\n", exception.what());
			}
		}
	}
	
	Color ProcessColor() const;
	
	std::string Tag;
	std::string Ident;
	std::vector<std::string> Aliases;	/// alias string identifiers for the data element
	CConfigData *Parent = nullptr;
	std::vector<CConfigProperty> Properties;
	std::vector<CConfigData *> Sections;
};

#endif
