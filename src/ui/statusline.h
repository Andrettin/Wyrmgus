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
/**@name statusline.h - The status line header file. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#include "economy/resource.h"

namespace wyrmgus {
	class font;
}

class CStatusLine final
{
public:
	CStatusLine()
	{
		memset(Costs, 0, (ManaResCost + 1) * sizeof(int));
	}

	void Draw(std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawCosts(std::vector<std::function<void(renderer *)>> &render_commands);
	void Set(const std::string &status);
	void SetCosts(int mana, int food, const int *costs);
	const std::string &Get() const { return this->StatusLine; }
	void Clear();
	void ClearCosts();

public:
	int Width = 0;
	int TextX = 0;
	int TextY = 0;
	wyrmgus::font *Font = nullptr;
	int Costs[ManaResCost + 1];

private:
	std::string StatusLine;
};
