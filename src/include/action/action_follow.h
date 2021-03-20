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
/**@name action_follow.h - The actions headerfile. */
//
//      (c) Copyright 1998-2012 by Lutz Sammer and Jimmy Salmon
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

#include "actions.h"

enum class follow_state {
	init = 0,
	initialized = 1,

	target_reached = 128,
};

class COrder_Follow final : public COrder
{
public:
	COrder_Follow() : COrder(UnitAction::Follow)
	{
		goalPos.x = -1;
		goalPos.y = -1;
	}

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_Follow>(*this);
	}

	virtual bool IsValid() const override;

	virtual void Save(CFile &file, const CUnit &unit) const override;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) override;

	virtual void Execute(CUnit &unit) override;
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void UpdatePathFinderData(PathFinderInput &input) override;

private:
	follow_state state = follow_state::init;
	int Range = 0;
	Vec2i goalPos;
	//Wyrmgus start
	int MapLayer = 0;
	//Wyrmgus end

	friend std::unique_ptr<COrder> COrder::NewActionFollow(CUnit &dest);
};
