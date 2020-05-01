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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "stratagus.h"

#include "ui/icon.h"

#include "database/defines.h"
#include "menus.h"
#include "mod.h"
#include "player.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "video.h"

namespace stratagus {

icon::icon(const std::string &identifier) : data_entry(identifier)
{
}

icon::~icon()
{
	CPlayerColorGraphic::Free(this->G);
	CPlayerColorGraphic::Free(this->GScale);
}

void icon::initialize()
{
	if (!this->get_file().empty() && this->G == nullptr) {
		const QSize &icon_size = defines::get()->get_icon_size();
		this->G = CPlayerColorGraphic::New(this->get_file().string(), icon_size.width(), icon_size.height());
	}

	this->load();

	data_entry::initialize();
}

void icon::set_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_file()) {
		return;
	}

	this->file = database::get_graphics_path(this->get_module()) / filepath;
}

void icon::load()
{
	if (this->G == nullptr) {
		throw std::runtime_error("Icon \"" + this->get_identifier() + "\" has no graphics.");
	}

	this->G->Load(false, defines::get()->get_scale_factor());
	if (Preference.GrayscaleIcons) {
		this->GScale = this->G->Clone(true);
	}
	if (this->get_frame() >= G->NumFrames) {
		DebugPrint("Invalid icon frame: %s - %d\n" _C_ this->get_identifier().c_str() _C_ this->get_frame());
		this->frame = 0;
	}
}

/**
**  Draw icon at pos.
**
**  @param player  Player pointer used for icon colors
**  @param pos     display pixel position
*/
void icon::DrawIcon(const PixelPos &pos, const player_color *player_color) const
{
	if (player_color != nullptr) {
		this->G->DrawPlayerColorFrameClip(player_color, this->get_frame(), pos.x, pos.y, nullptr);
	} else {
		this->G->DrawFrameClip(this->get_frame(), pos.x, pos.y);
	}
}

/**
**  Draw grayscale icon at pos.
**
**  @param pos     display pixel position
*/
void icon::DrawGrayscaleIcon(const PixelPos &pos, const player_color *player_color) const
{
	if (this->GScale) {
		if (player_color != nullptr) {
			this->GScale->DrawPlayerColorFrameClip(player_color, this->get_frame(), pos.x, pos.y);
		} else {
			this->GScale->DrawFrameClip(this->get_frame(), pos.x, pos.y);
		}
	}
}

/**
**  Draw cooldown spell effect on icon at pos.
**
**  @param pos       display pixel position
**  @param percent   cooldown percent
*/
void icon::DrawCooldownSpellIcon(const PixelPos &pos, const int percent) const
{
	// TO-DO: implement more effect types (clock-like)
	if (this->GScale) {
		this->GScale->DrawFrameClip(this->get_frame(), pos.x, pos.y);
		const int height = (G->Height * (100 - percent)) / 100;
		this->G->DrawSubClip(G->frame_map[this->get_frame()].x, G->frame_map[this->get_frame()].y + G->Height - height,
							 G->Width, height, pos.x, pos.y + G->Height - height);
	} else {
		DebugPrint("Enable grayscale icon drawing in your game to achieve special effects for cooldown spell icons");
		this->DrawIcon(pos);
	}
}

/**
**  Draw unit icon 'icon' with border on x,y
**
**  @param style   Button style
**  @param flags   State of icon (clicked, mouse over...)
**  @param pos     display pixel position
**  @param text    Optional text to display
*/
void icon::DrawUnitIcon(const ButtonStyle &style, unsigned flags,
						 const PixelPos &pos, const std::string &text, const player_color *player_color, bool transparent, bool grayscale, int show_percent) const
{
	ButtonStyle s(style);

	//Wyrmgus start
//	s.Default.Sprite = s.Hover.Sprite = s.Clicked.Sprite = this->G;
	if (grayscale) {
		s.Default.Sprite = s.Hover.Sprite = s.Clicked.Sprite = this->GScale;
	} else {
		s.Default.Sprite = s.Hover.Sprite = s.Clicked.Sprite = this->G;
	}
	//Wyrmgus end
	s.Default.Frame = s.Hover.Frame = s.Clicked.Frame = this->get_frame();
	if (!(flags & IconSelected) && (flags & IconAutoCast)) {
		s.Default.BorderColorRGB = UI.ButtonPanel.AutoCastBorderColorRGB;
		s.Default.BorderColor = 0;
		//Wyrmgus start
//		s.Default.BorderSize = 2;
		//Wyrmgus end
	}
	//Wyrmgus start
	if (!(flags & IconAutoCast)) {
		s.Default.BorderSize = 0;
	}
	//Wyrmgus end

	const int scale_factor = defines::get()->get_scale_factor();

	//Wyrmgus start
	if (Preference.IconsShift && Preference.IconFrameG && Preference.PressedIconFrameG) {
		Video.FillRectangle(ColorBlack, pos.x, pos.y, 46 * scale_factor, 38 * scale_factor);
		if (flags & IconClicked) { // Shift the icon a bit to make it look like it's been pressed.
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, text, player_color, true);
			}
			DrawUIButton(&s, flags, pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, text, player_color, transparent, show_percent);
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, 46 * scale_factor - 1 * scale_factor, 38 * scale_factor - 1 * scale_factor);
			} else if (flags & IconAutoCast) {
				Video.DrawRectangle(Video.MapRGB(TheScreen->format, UI.ButtonPanel.AutoCastBorderColorRGB), pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, 46 * scale_factor - 1 * scale_factor, 38 * scale_factor - 1 * scale_factor);
			}
			if (!Preference.CommandButtonFrameG || !(flags & IconCommandButton)) {
				Preference.PressedIconFrameG->DrawClip(pos.x - 4 * scale_factor, pos.y - 4 * scale_factor);
			} else {
				Preference.CommandButtonFrameG->DrawClip(pos.x - 5 * scale_factor, pos.y - 4 * scale_factor);
			}
		} else {
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, true);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, transparent, show_percent);
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x, pos.y, 46 * scale_factor, 38 * scale_factor);
			} else if (flags & IconAutoCast) {
				Video.DrawRectangle(Video.MapRGB(TheScreen->format, UI.ButtonPanel.AutoCastBorderColorRGB), pos.x, pos.y, 46 * scale_factor, 38 * scale_factor);
			}
			if (Preference.CommandButtonFrameG && (flags & IconCommandButton)) {
				Preference.CommandButtonFrameG->DrawClip(pos.x - 5 * scale_factor, pos.y - 4 * scale_factor);
			} else {
				Preference.IconFrameG->DrawClip(pos.x - 4 * scale_factor, pos.y - 4 * scale_factor);
			}
		}
	//Wyrmgus end
	//Wyrmgus start
//	if (Preference.IconsShift) {
	} else if (Preference.IconsShift) {
	//Wyrmgus end
		// Left and top edge of Icon
		Video.DrawHLine(ColorWhite, pos.x - 1 * scale_factor, pos.y - 1 * scale_factor, 49 * scale_factor);
		Video.DrawVLine(ColorWhite, pos.x - 1 * scale_factor, pos.y, 40 * scale_factor);
		Video.DrawVLine(ColorWhite, pos.x, pos.y + 38 * scale_factor, 2 * scale_factor);
		Video.DrawHLine(ColorWhite, pos.x + 46 * scale_factor, pos.y, 2 * scale_factor);

		// Bottom and Right edge of Icon
		Video.DrawHLine(ColorGray, pos.x + 1 * scale_factor, pos.y + 38 * scale_factor, 47 * scale_factor);
		Video.DrawHLine(ColorGray, pos.x + 1 * scale_factor, pos.y + 39 * scale_factor, 47 * scale_factor);
		Video.DrawVLine(ColorGray, pos.x + 46 * scale_factor, pos.y + 1 * scale_factor, 37 * scale_factor);
		Video.DrawVLine(ColorGray, pos.x + 47 * scale_factor, pos.y + 1 * scale_factor, 37 * scale_factor);

		Video.DrawRectangle(ColorBlack, pos.x - 3 * scale_factor, pos.y - 3 * scale_factor, 52 * scale_factor, 44 * scale_factor);
		Video.DrawRectangle(ColorBlack, pos.x - 4 * scale_factor, pos.y - 4 * scale_factor, 54 * scale_factor, 46 * scale_factor);

		if (flags & IconActive) { // Code to make a border appear around the icon when the mouse hovers over it.
			Video.DrawRectangle(ColorGray, pos.x - 4 * scale_factor, pos.y - 4, 54 * scale_factor, 46 * scale_factor);
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, true);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, transparent, show_percent);
		}

		if (flags & IconClicked) { // Shift the icon a bit to make it look like it's been pressed.
			//Wyrmgus start
//			DrawUIButton(&s, flags, pos.x + 1, pos.y + 1, text, player);
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, text, player_color, true);
			}
			DrawUIButton(&s, flags, pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, text, player_color, transparent, show_percent);
			//Wyrmgus end
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x + 1 * scale_factor, pos.y + 1 * scale_factor, 46 * scale_factor, 38 * scale_factor);
			}			
			Video.DrawRectangle(ColorGray, pos.x, pos.y, 48 * scale_factor, 40 * scale_factor);
			Video.DrawVLine(ColorDarkGray, pos.x - 1 * scale_factor, pos.y - 1 * scale_factor, 40 * scale_factor);
			Video.DrawHLine(ColorDarkGray, pos.x - 1 * scale_factor, pos.y - 1 * scale_factor, 49 * scale_factor);
			Video.DrawHLine(ColorDarkGray, pos.x - 1 * scale_factor, pos.y + 39 * scale_factor, 2 * scale_factor);

			Video.DrawRectangle(ColorGray, pos.x - 4 * scale_factor, pos.y - 4 * scale_factor, 54 * scale_factor, 46 * scale_factor);
		} else {
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, true);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, transparent, show_percent);
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x, pos.y, 46 * scale_factor, 38 * scale_factor);
			}
		}
	} else {
		if (show_percent < 100) {
			DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, true);
		}
		DrawUIButton(&s, flags, pos.x, pos.y, text, player_color, transparent, show_percent);
	}
}

}

/**
**  Load the Icon
*/
bool IconConfig::LoadNoLog()
{
	Assert(!this->Name.empty());

	Icon = stratagus::icon::try_get(this->Name);
	return Icon != nullptr;
}

/**
**  Load the Icon
*/
bool IconConfig::Load()
{
	bool res = LoadNoLog();
	if (res == true) {
		UpdateLoadProgress();
	} else {
		throw std::runtime_error("Can't find icon \"" + this->Name + "\".");
	}
	return res;
}
