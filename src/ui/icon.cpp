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
//      (c) Copyright 1998-2025 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "ui/icon.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "menus.h"
#include "mod.h"
#include "player/player.h"
#include "translator.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "util/assert_util.h"
#include "util/colorization_type.h"
#include "video/renderer.h"
#include "video/video.h"

namespace wyrmgus {

int icon::get_to_load_count()
{
	int count = 0;

	for (const icon *icon : icon::get_all()) {
		if (!icon->is_loaded()) {
			++count;
		}
	}

	return count;
}

void icon::load_all()
{
	int loaded_count = 0;
	const int to_load_count = icon::get_to_load_count();

	if (to_load_count > 0) {
		ShowLoadProgress("%s", _("Loading Icons... (0%)"));
	}

	for (icon *icon : icon::get_all()) {
		if (!icon->is_loaded()) {
			icon->load();
			++loaded_count;
			IncItemsLoaded();
			ShowLoadProgress(_("Loading Icons... (%d%%)"), (loaded_count + 1) * 100 / to_load_count);
		}
	}
}


icon::icon(const std::string &identifier) : icon_base(identifier), colorization(colorization_type::none)
{
}

void icon::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "hue_ignored_colors") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			this->hue_ignored_colors.insert(child_scope.to_color());
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void icon::initialize()
{
	if (!this->get_file().empty() && this->get_graphics() == nullptr) {
		const QSize &icon_size = this->get_size();
		this->set_graphics(CPlayerColorGraphic::New(this->get_file().string(), icon_size, this->get_conversible_player_color()));
	}

	icon_base::initialize();
}

const QSize &icon::get_size() const
{
	return defines::get()->get_icon_size();
}

std::shared_ptr<CPlayerColorGraphic> icon::get_graphics() const
{
	return std::static_pointer_cast<CPlayerColorGraphic>(icon_base::get_graphics());
}

/**
**  Draw icon at pos.
**
**  @param player  Player pointer used for icon colors
**  @param pos     display pixel position
*/
void icon::DrawIcon(const PixelPos &pos, const player_color *player_color, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	this->get_graphics()->render_frame(this->get_frame(), pos, color_modification(this->get_hue_rotation(), this->get_colorization(), this->get_hue_ignored_colors(), player_color), render_commands);
}

/**
**  Draw grayscale icon at pos.
**
**  @param pos     display pixel position
*/
void icon::DrawGrayscaleIcon(const PixelPos &pos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	this->get_graphics()->DrawGrayscaleFrameClip(this->get_frame(), pos.x, pos.y, render_commands);
}

/**
**  Draw cooldown spell effect on icon at pos.
**
**  @param pos       display pixel position
**  @param percent   cooldown percent
*/
void icon::DrawCooldownSpellIcon(const PixelPos &pos, const int percent, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	// TO-DO: implement more effect types (clock-like)
	this->get_graphics()->DrawGrayscaleFrameClip(this->get_frame(), pos.x, pos.y, render_commands);
	const int height = (this->get_graphics()->Height * (100 - percent)) / 100;
	this->get_graphics()->DrawSubClip(this->get_graphics()->frame_map[this->get_frame()].x, this->get_graphics()->frame_map[this->get_frame()].y + this->get_graphics()->Height - height, this->get_graphics()->Width, height, pos.x, pos.y + this->get_graphics()->Height - height, render_commands);
}

/**
**  Draw unit icon 'icon' with border on x,y
**
**  @param style   Button style
**  @param flags   State of icon (clicked, mouse over...)
**  @param pos     display pixel position
**  @param text    Optional text to display
*/
void icon::DrawUnitIcon(const ButtonStyle &style, const unsigned flags, const PixelPos &pos, const std::string &text, const player_color *player_color, QColor border_color, const bool transparent, const bool grayscale, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const color_modification color_modification(this->get_hue_rotation(), this->get_colorization(), this->get_hue_ignored_colors(), player_color);

	ButtonStyle s(style);

	s.Default.Sprite = s.Hover.Sprite = s.Clicked.Sprite = this->get_graphics();
	//Wyrmgus end
	s.Default.Frame = s.Hover.Frame = s.Clicked.Frame = this->get_frame();

	if (flags & IconSelected) {
		border_color = defines::get()->get_selected_border_color();
	} else if (flags & IconAutoCast) {
		border_color = defines::get()->get_autocast_border_color();
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	if (Preference.IconsShift && defines::get()->get_icon_frame_graphics() != nullptr && defines::get()->get_pressed_icon_frame_graphics() != nullptr) {
		Video.FillRectangle(ColorBlack, pos.x, pos.y, (46 * scale_factor).to_int(), (38 * scale_factor).to_int(), render_commands);
		if (flags & IconClicked) { // Shift the icon a bit to make it look like it's been pressed.
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x + (1 * scale_factor).to_int(), pos.y + (1 * scale_factor).to_int(), text, grayscale, color_modification, true, render_commands);
			}

			DrawUIButton(&s, flags, pos.x + (1 * scale_factor).to_int(), pos.y + (1 * scale_factor).to_int(), text, grayscale, color_modification, transparent, show_percent, render_commands);

			if (defines::get()->get_command_button_frame_graphics() == nullptr || !(flags & IconCommandButton)) {
				defines::get()->get_pressed_icon_frame_graphics()->DrawClip(pos.x - (4 * scale_factor).to_int(), pos.y - (4 * scale_factor).to_int(), render_commands);
			} else {
				defines::get()->get_command_button_frame_graphics()->DrawClip(pos.x - (5 * scale_factor).to_int(), pos.y - (4 * scale_factor).to_int(), render_commands);
			}

			if (border_color.isValid()) {
				render_commands.push_back([pos, border_color, scale_factor](renderer *renderer) {
					renderer->draw_rect(pos + QPoint((1 * scale_factor).to_int(), (1 * scale_factor).to_int()), defines::get()->get_scaled_icon_size() - QSize((1 * scale_factor).to_int(), (1 * scale_factor).to_int()), border_color);
				});
			}
		} else {
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, grayscale, color_modification, true, render_commands);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, grayscale, color_modification, transparent, show_percent, render_commands);

			if (defines::get()->get_command_button_frame_graphics() != nullptr && (flags & IconCommandButton)) {
				defines::get()->get_command_button_frame_graphics()->DrawClip(pos.x - (5 * scale_factor).to_int(), pos.y - (4 * scale_factor).to_int(), render_commands);
			} else {
				defines::get()->get_icon_frame_graphics()->DrawClip(pos.x - (4 * scale_factor).to_int(), pos.y - (4 * scale_factor).to_int(), render_commands);
			}

			if (border_color.isValid()) {
				render_commands.push_back([pos, border_color](renderer *renderer) {
					renderer->draw_rect(pos, defines::get()->get_scaled_icon_size(), border_color);
				});
			}
		}
	} else {
		if (show_percent < 100) {
			DrawUIButton(&s, flags, pos.x, pos.y, text, grayscale, color_modification, true, render_commands);
		}
		DrawUIButton(&s, flags, pos.x, pos.y, text, grayscale, color_modification, transparent, show_percent, render_commands);
	}
}

}
