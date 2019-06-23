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
/**@name icon.cpp - The icon source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "ui/icon.h"

#include "config.h"
#include "config_operator.h"
#include "menus.h"
#include "module.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "video/palette_image.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  CIcon destructor
*/
CIcon::~CIcon()
{
	CPlayerColorGraphic::Free(this->G);
	CPlayerColorGraphic::Free(this->GScale);
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CIcon::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "image") { //allow defining the icon's palette image as a section of the icon, using the icon's ident for it
		PaletteImage *image = PaletteImage::GetOrAdd(this->Ident);
		image->ProcessConfigData(section);
		this->Image = image;
		
		this->G = CPlayerColorGraphic::New(image->GetFile().utf8().get_data(), image->GetFrameSize().width, image->GetFrameSize().height);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the icon
*/
void CIcon::Initialize()
{
	if (this->Image == nullptr) {
		print_error("Icon \"" + this->GetIdent() + "\" has no palette image.");
	}
	
	this->Initialized = true;
}

void CIcon::Load()
{
	//Wyrmgus start
	if (this->Loaded) {
		return;
	}
	//Wyrmgus end
	
	Assert(G);
	G->Load();
	if (Preference.GrayscaleIcons) {
		GScale = G->Clone(true);
	}
	if (Frame >= G->NumFrames) {
		fprintf(stderr, "Invalid icon frame: %s - %d\n", Ident.c_str(), Frame);
		Frame = 0;
	}
	//Wyrmgus start
	this->Loaded = true;
	//Wyrmgus end
}

/**
**  Draw icon at pos.
**
**  @param player  Player pointer used for icon colors
**  @param pos     display pixel position
*/
void CIcon::DrawIcon(const PixelPos &pos, const int player) const
{
	if (player != -1 ) {
		//Wyrmgus start
//		this->G->DrawPlayerColorFrameClip(player, this->GetFrame(), pos.x, pos.y);
		this->G->DrawPlayerColorFrameClip(player, this->GetFrame(), pos.x, pos.y, true);
		//Wyrmgus end
	} else {
		this->G->DrawFrameClip(this->GetFrame(), pos.x, pos.y);
	}
}

/**
**  Draw grayscale icon at pos.
**
**  @param pos     display pixel position
*/
void CIcon::DrawGrayscaleIcon(const PixelPos &pos, const int player) const
{
	if (this->GScale) {
		if (player != -1) {
			this->GScale->DrawPlayerColorFrameClip(player, this->GetFrame(), pos.x, pos.y);
		} else {
			this->GScale->DrawFrameClip(this->GetFrame(), pos.x, pos.y);
		}
	}
}

/**
**  Draw cooldown spell effect on icon at pos.
**
**  @param pos       display pixel position
**  @param percent   cooldown percent
*/
void CIcon::DrawCooldownSpellIcon(const PixelPos &pos, const int percent) const
{
	// TO-DO: implement more effect types (clock-like)
	if (this->GScale) {
		this->GScale->DrawFrameClip(this->GetFrame(), pos.x, pos.y);
		const int height = (G->Height * (100 - percent)) / 100;
		this->G->DrawSubClip(G->frame_map[this->GetFrame()].x, G->frame_map[this->GetFrame()].y + G->Height - height,
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
void CIcon::DrawUnitIcon(const ButtonStyle &style, unsigned flags,
						 //Wyrmgus start
//						 const PixelPos &pos, const std::string &text, int player) const
						 const PixelPos &pos, const std::string &text, int player, bool transparent, bool grayscale, int show_percent) const
						 //Wyrmgus end
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
	s.Default.Frame = s.Hover.Frame = s.Clicked.Frame = this->GetFrame();
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
	//Wyrmgus start
	if (Preference.IconsShift && Preference.IconFrameG && Preference.PressedIconFrameG) {
		Video.FillRectangle(ColorBlack, pos.x, pos.y, 46, 38);
		if (flags & IconActive) { // Code to make a border appear around the icon when the mouse hovers over it.
//			Video.DrawRectangle(ColorGray, pos.x - 4, pos.y - 4, 54, 46);
//			DrawUIButton(&s, flags, pos.x, pos.y, text, player, transparent);
		}

		if (flags & IconClicked) { // Shift the icon a bit to make it look like it's been pressed.
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x + 1, pos.y + 1, text, player, true);
			}
			DrawUIButton(&s, flags, pos.x + 1, pos.y + 1, text, player, transparent, show_percent);
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x, pos.y, 48, 40);
			} else if (flags & IconAutoCast) {
				Video.DrawRectangle(Video.MapRGB(TheScreen->format, UI.ButtonPanel.AutoCastBorderColorRGB), pos.x, pos.y, 48, 40);
			}
			if (!Preference.CommandButtonFrameG || !(flags & IconCommandButton)) {
				Preference.PressedIconFrameG->DrawClip(pos.x - 4, pos.y - 4);
			} else {
				Preference.CommandButtonFrameG->DrawClip(pos.x - 5, pos.y - 4);
			}
//			Video.DrawRectangle(ColorGray, pos.x - 4, pos.y - 4, 54, 46);
		} else {
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, player, true);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, player, transparent, show_percent);
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x, pos.y, 46, 38);
			} else if (flags & IconAutoCast) {
				Video.DrawRectangle(Video.MapRGB(TheScreen->format, UI.ButtonPanel.AutoCastBorderColorRGB), pos.x, pos.y, 46, 38);
			}
			if (Preference.CommandButtonFrameG && (flags & IconCommandButton)) {
				Preference.CommandButtonFrameG->DrawClip(pos.x - 5, pos.y - 4);
			} else {
				Preference.IconFrameG->DrawClip(pos.x - 4, pos.y - 4);
			}
		}
	//Wyrmgus end
	//Wyrmgus start
//	if (Preference.IconsShift) {
	} else if (Preference.IconsShift) {
	//Wyrmgus end
		// Left and top edge of Icon
		Video.DrawHLine(ColorWhite, pos.x - 1, pos.y - 1, 49);
		Video.DrawVLine(ColorWhite, pos.x - 1, pos.y, 40);
		Video.DrawVLine(ColorWhite, pos.x, pos.y + 38, 2);
		Video.DrawHLine(ColorWhite, pos.x + 46, pos.y, 2);

		// Bottom and Right edge of Icon
		Video.DrawHLine(ColorGray, pos.x + 1, pos.y + 38, 47);
		Video.DrawHLine(ColorGray, pos.x + 1, pos.y + 39, 47);
		Video.DrawVLine(ColorGray, pos.x + 46, pos.y + 1, 37);
		Video.DrawVLine(ColorGray, pos.x + 47, pos.y + 1, 37);

		Video.DrawRectangle(ColorBlack, pos.x - 3, pos.y - 3, 52, 44);
		Video.DrawRectangle(ColorBlack, pos.x - 4, pos.y - 4, 54, 46);

		if (flags & IconActive) { // Code to make a border appear around the icon when the mouse hovers over it.
			Video.DrawRectangle(ColorGray, pos.x - 4, pos.y - 4, 54, 46);
			//Wyrmgus start
//			DrawUIButton(&s, flags, pos.x, pos.y, text, player);
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, player, true);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, player, transparent, show_percent);
			//Wyrmgus end
		}

		if (flags & IconClicked) { // Shift the icon a bit to make it look like it's been pressed.
			//Wyrmgus start
//			DrawUIButton(&s, flags, pos.x + 1, pos.y + 1, text, player);
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x + 1, pos.y + 1, text, player, true);
			}
			DrawUIButton(&s, flags, pos.x + 1, pos.y + 1, text, player, transparent, show_percent);
			//Wyrmgus end
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x + 1, pos.y + 1, 46, 38);
			}			
			Video.DrawRectangle(ColorGray, pos.x, pos.y, 48, 40);
			Video.DrawVLine(ColorDarkGray, pos.x - 1, pos.y - 1, 40);
			Video.DrawHLine(ColorDarkGray, pos.x - 1, pos.y - 1, 49);
			Video.DrawHLine(ColorDarkGray, pos.x - 1, pos.y + 39, 2);

			Video.DrawRectangle(ColorGray, pos.x - 4, pos.y - 4, 54, 46);
		} else {
			//Wyrmgus start
//			DrawUIButton(&s, flags, pos.x, pos.y, text, player);
			if (show_percent < 100) {
				DrawUIButton(&s, flags, pos.x, pos.y, text, player, true);
			}
			DrawUIButton(&s, flags, pos.x, pos.y, text, player, transparent, show_percent);
			//Wyrmgus end
			if (flags & IconSelected) {
				Video.DrawRectangle(ColorGreen, pos.x, pos.y, 46, 38);
			}
		}
	} else {
		//Wyrmgus start
//		DrawUIButton(&s, flags, pos.x, pos.y, text, player);
		if (show_percent < 100) {
			DrawUIButton(&s, flags, pos.x, pos.y, text, player, true);
		}
		DrawUIButton(&s, flags, pos.x, pos.y, text, player, transparent, show_percent);
		//Wyrmgus end
	}
}

void CIcon::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_frame", "frame"), +[](CIcon *icon, const int frame){ icon->Frame = frame; });
	ClassDB::bind_method(D_METHOD("get_frame"), &CIcon::GetFrame);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frame"), "set_frame", "get_frame");
	
	ClassDB::bind_method(D_METHOD("set_image", "ident"), +[](CIcon *icon, const String &ident){ icon->Image = PaletteImage::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_image"), +[](const CIcon *icon){ return const_cast<PaletteImage *>(icon->GetImage()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "image"), "set_image", "get_image");
}

/**
**  Get the numbers of icons.
*/
int GetIconsCount()
{
	return CIcon::GetAll().size();
}

/**
**  Load the graphics for the icons.
*/
void LoadIcons()
{
	ShowLoadProgress("%s", _("Loading Icons"));
		
	for (CIcon *icon : CIcon::GetAll()) {
		UpdateLoadProgress();
		icon->Load();

		IncItemsLoaded();
	}
}
