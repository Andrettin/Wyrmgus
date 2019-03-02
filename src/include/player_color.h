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
/**@name player_color.h - The player color header file. */
//
//      (c) Copyright 2019 by Andrettin
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

#ifndef __PLAYER_COLOR_H__
#define __PLAYER_COLOR_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"

#include <core/color.h>
#include <core/object.h>

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CPlayerColor : public CDataType, public Object
{
	GDCLASS(CPlayerColor, Object)
	
public:
	static CPlayerColor *GetPlayerColor(const std::string &ident, bool should_find = true);
	static CPlayerColor *GetOrAddPlayerColor(const std::string &ident);
	static void ClearPlayerColors();
	
	static std::vector<CPlayerColor *> PlayerColors;	/// Player colors
	static std::map<std::string, CPlayerColor *> PlayerColorsByIdent;

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	String GetIdent() const
	{
		return CDataType::GetIdent();
	}
	
	String GetName() const
	{
		return this->Name.c_str();
	}

	Array GetColors() const
	{
		Array colors;
		
		for (Color color : this->Colors) {
			colors.push_back(color);
		}
		
		return colors;
	}

private:
	std::string Name;				/// Name of the player color
	std::vector<Color> Colors;		/// The colors of the player color

protected:
	static void _bind_methods();
};

#endif
