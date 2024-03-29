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
/**@name statusline.cpp - The statusline. */
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

#include "stratagus.h"

#include "ui/statusline.h"

#include "database/defines.h"
#include "game/game.h"
#include "ui/interface.h"
#include "ui/resource_icon.h"
#include "ui/ui.h"
#include "video/font.h"
#include "video/video.h"

void CStatusLine::Draw(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (!this->StatusLine.empty()) {
		PushClipping();
		SetClipping(this->TextX, this->TextY,
					this->TextX + this->Width - 1, Video.Height - 1);
		CLabel(this->Font).DrawClip(this->TextX, this->TextY, this->StatusLine, render_commands);
		PopClipping();
	}
}

/**
**  Change status line to new text.
**
**  @param status  New status line information.
*/
void CStatusLine::Set(const std::string &status, const bool is_input)
{
	if (game::get()->is_console_active() && !is_input) {
		return;
	}

	this->StatusLine = status;
}

/**
**  Set costs in status line.
**
**  @param mana   Mana costs.
**  @param food   Food costs.
**  @param costs  Resource costs, null pointer if all are zero.
*/
void CStatusLine::SetCosts(int mana, int food, const int *costs)
{
	if (costs) {
		memcpy(Costs, costs, MaxCosts * sizeof(*costs));
	} else {
		memset(Costs, 0, sizeof(Costs));
	}
	Costs[ManaResCost] = mana;
	Costs[FoodCost] = food;
}

void CStatusLine::ClearCosts()
{
	SetCosts(0, 0, nullptr);
}

/**
**  Draw costs in status line.
**
**  @todo FIXME : make DrawCosts more configurable.
**  @todo FIXME : 'time' resource should be shown too.
**
**  @internal MaxCost == FoodCost.
*/
void CStatusLine::DrawCosts(std::vector<std::function<void(renderer *)>> &render_commands)
{
	int x = UI.StatusLine.TextX + 268;
	CLabel label(wyrmgus::defines::get()->get_game_font());
	if (this->Costs[ManaResCost]) {
		const wyrmgus::resource_icon *mana_icon = wyrmgus::defines::get()->get_mana_icon();
		mana_icon->get_graphics()->DrawFrameClip(mana_icon->get_frame(), x, UI.StatusLine.TextY, render_commands);

		x += 20;
		x += label.Draw(x, UI.StatusLine.TextY, this->Costs[ManaResCost], render_commands);
	}

	for (unsigned int i = 1; i <= MaxCosts; ++i) {
		if (this->Costs[i]) {
			x += 5;

			const wyrmgus::resource_icon *icon = nullptr;

			switch (i) {
				case FoodCost:
					icon = defines::get()->get_food_icon();
					break;
				default: {
					const wyrmgus::resource *resource = wyrmgus::resource::get_all()[i];
					icon = resource->get_icon();
					break;
				}
			}

			if (icon != nullptr) {
				const std::shared_ptr<CGraphic> &icon_graphics = icon->get_graphics();
				icon_graphics->DrawFrameClip(icon->get_frame(), x, UI.StatusLine.TextY, render_commands);
				x += 20;
			}
			x += label.Draw(x, UI.StatusLine.TextY, this->Costs[i], render_commands);
			if (x > Video.Width - 60) {
				break;
			}
		}
	}
}

void CStatusLine::Clear(const bool is_input)
{
	if (game::get()->is_console_active() && !is_input) {
		return;
	}

	this->StatusLine.clear();
}
