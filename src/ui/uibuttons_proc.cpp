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
/**@name uibuttons_proc.cpp - The UI buttons processing code. */
//
//      (c) Copyright 1999-2006 by Andreas Arens, Jimmy Salmon, Nehal Mistry
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

#include "ui/ui.h"

#include "database/defines.h"
#include "menus.h"
#include "player.h"
#include "video/font.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
-- UI buttons operation functions
----------------------------------------------------------------------------*/

/**
**  Draw UI button 'button' on x,y
**
**  @param style  Button style
**  @param flags  State of Button (clicked, mouse over...)
**  @param x      X display position
**  @param y      Y display position
**  @param text   text to print on button
*/
void DrawUIButton(ButtonStyle *style, unsigned flags, int x, int y,
				  const std::string &text, const bool grayscale, const color_modification &color_modification, bool transparent, int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	ButtonStyleProperties *p;

	if (flags & MI_FLAGS_CLICKED) {
		p = &style->Clicked;
	} else if (flags & MI_FLAGS_ACTIVE) {
		p = &style->Hover;
	} else {
		p = &style->Default;
	}

	//
	//  Image
	//
	ButtonStyleProperties *pimage = p;
	if (!p->Sprite) {
		// No image.  Try hover, selected, then default
		if ((flags & MI_FLAGS_ACTIVE) && style->Hover.Sprite) {
			pimage = &style->Hover;
		} else if (style->Default.Sprite) {
			pimage = &style->Default;
		}
	}
	if (pimage->Sprite) {
		pimage->Sprite->Load(defines::get()->get_scale_factor());
	}
	if (pimage->Sprite) {
		auto colorGraphic = std::dynamic_pointer_cast<CPlayerColorGraphic>(pimage->Sprite);

		if (grayscale) {
			pimage->Sprite->DrawGrayscaleFrameClip(pimage->Frame, x, y, show_percent, render_commands);
		} else if (colorGraphic && !color_modification.is_null()) {
			const unsigned char opacity = transparent ? 64 : 255;
			colorGraphic->render_frame(pimage->Frame, QPoint(x, y), color_modification, grayscale, false, opacity, show_percent, render_commands);
		} else {
			pimage->Sprite->DrawFrame(pimage->Frame, x, y, render_commands);
		}
	}

	//
	//  Text
	//
	if (!text.empty()) {
		CLabel label(style->Font,
					 (p->TextNormalColor != nullptr ? p->TextNormalColor :
					  style->TextNormalColor != nullptr ? style->TextNormalColor : wyrmgus::defines::get()->get_default_font_color()),
					 (p->TextReverseColor != nullptr ? p->TextReverseColor :
					  style->TextReverseColor != nullptr ? style->TextReverseColor : wyrmgus::defines::get()->get_default_highlight_font_color()));

		if (p->TextAlign == TextAlignment::Center || p->TextAlign == TextAlignment::Undefined) {
			label.DrawCentered(x + p->TextPos.x, y + p->TextPos.y, text, render_commands);
		} else if (p->TextAlign == TextAlignment::Left) {
			label.Draw(x + p->TextPos.x, y + p->TextPos.y, text, render_commands);
		} else {
			label.Draw(x + p->TextPos.x - style->Font->Width(text), y + p->TextPos.y, text, render_commands);
		}
	}

	//
	//  Border
	//
	if (!p->BorderColor) {
		CColor color(p->BorderColorRGB);
		//Wyrmgus start
		/*
		if (p->BorderColorRGB.R > 0 || p->BorderColorRGB.G > 0 || p->BorderColorRGB.B > 0) {
			int shift = GameCycle % 0x20;
			color.R >>= shift / 2;
			color.G >>= shift / 2;
			color.B >>= shift / 2;
			if (shift >= 0x10) {
				color.R = (p->BorderColorRGB.R > 0) << ((shift - 0x10) / 2);
				color.G = (p->BorderColorRGB.G > 0) << ((shift - 0x10) / 2);
				color.B = (p->BorderColorRGB.B > 0) << ((shift - 0x10) / 2);
			}
		}
		*/
		//Wyrmgus end
		p->BorderColor = CVideo::MapRGB(color);
	}
	if (p->BorderSize) {
		for (int i = 0; i < p->BorderSize; ++i) {
			//Wyrmgus start
//			Video.DrawRectangleClip(p->BorderColor, x - i, y - i,
//									style->Width + 2 * i, style->Height + 2 * i);
			Video.DrawRectangleClip(p->BorderColor, x - i - 1, y - i - 1,
									(style->Width + 2 * i) + 2, (style->Height + 2 * i) + 2, render_commands);
			//Wyrmgus end
		}
	}
}

void DrawUIButton(ButtonStyle *style, unsigned flags, int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands)
{
	DrawUIButton(style, flags, x, y, text, false, color_modification(), false, render_commands);
}
