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
/**@name unit_draw.cpp - The draw routines for units. */
//
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
//		and Andrettin
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

#include "actions.h"
#include "action/action_build.h"
#include "action/action_built.h"
#include "action/action_upgradeto.h"
//Wyrmgus start
#include "animation/animation_set.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "translate.h"
#include "ui/cursor.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "util/assert_util.h"
#include "util/size_util.h"
#include "video/font.h"
#include "video/renderer.h"
#include "video/video.h"

/**
**  Decoration: health, mana.
*/
class Decoration
{
public:
	std::string File; /// File containing the graphics data
	PixelPos HotPos = PixelPos(0, 0);  /// drawing position (relative)
	int Width = 0;        /// width of the decoration
	int Height = 0;       /// height of the decoration

	// --- FILLED UP ---
	std::shared_ptr<CGraphic> Sprite;  /// loaded sprite images
};


/**
**  Structure grouping all Sprites for decoration.
*/
class DecoSpriteType
{
public:
	std::vector<std::string> Name;            /// Name of the sprite.
	std::vector<Decoration> SpriteArray;      /// Sprite to display variable.
};

static DecoSpriteType DecoSprite; /// All sprite's infos.

unsigned long ShowOrdersCount;    /// Show orders for some time

unsigned long ShowNameDelay;                 /// Delay to show unit's name
unsigned long ShowNameTime;                  /// Show unit's name for some time

// FIXME: not all variables of this file are here
// FIXME: perhaps split this file into two or three parts?

/**
**  Show that units are selected.
**
**  @param color    FIXME
**  @param x1,y1    Coordinates of the top left corner.
**  @param x2,y2    Coordinates of the bottom right corner.
*/
void (*DrawSelection)(IntColor color, IntColor secondary_color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands) = DrawSelectionNone;

// FIXME: clean split screen support
// FIXME: integrate this with global versions of these functions in map.c

const CViewport *CurrentViewport;  /// FIXME: quick hack for split screen

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

/**
**  Show selection marker around a unit.
**
**  @param unit  Pointer to unit.
*/
void DrawUnitSelection(const CViewport &vp, const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const QPoint unit_screen_center_pos = vp.scaled_map_to_screen_pixel_pos(unit.get_scaled_map_pixel_pos_center());
	const QRect box_rect = unit.Type->get_scaled_box_rect(unit_screen_center_pos);

	// show player color circle below unit if that is activated
	if (preferences::get()->is_player_color_circle_enabled() && unit.Player->get_index() != PlayerNumNeutral && unit.CurrentAction() != UnitAction::Die) {
		const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
		const QPoint player_color_circle_offset = QPoint(1, 1) * scale_factor;
		const QRect player_color_circle_rect(box_rect.topLeft() + player_color_circle_offset, box_rect.bottomRight() - player_color_circle_offset);

		DrawSelectionCircleWithTrans(CVideo::MapRGB(unit.Player->get_minimap_color()), IntColor(0), player_color_circle_rect.x(), player_color_circle_rect.y(), player_color_circle_rect.right(), player_color_circle_rect.bottom(), render_commands);
	}
	
	// FIXME: make these colors customizable with scripts.

	IntColor color = 0;
	IntColor secondary_color = 0;

	if (CEditor::get()->is_running() && UnitUnderCursor == &unit && CEditor::get()->State == EditorSelecting) {
		color = ColorWhite;
	} else if (unit.Selected || unit.TeamSelected || (unit.Blink & 1)) {
		if (CPlayer::GetThisPlayer()->is_enemy_of(unit)) {
			color = ColorRed;
		} else if (unit.Player->get_index() == PlayerNumNeutral) {
			color = ColorYellow;
		} else if ((unit.Selected || (unit.Blink & 1))
				   && (unit.Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(unit))) {
			color = ColorGreen;

			if (unit.Variable[HP_INDEX].Value < unit.Variable[HP_INDEX].Max) {
				if (unit.Variable[HP_INDEX].get_percent_value() <= 25) {
					secondary_color = ColorRed;
				} else if (unit.Variable[HP_INDEX].get_percent_value() <= 50) {
					secondary_color = ColorOrange;
				} else if (unit.Variable[HP_INDEX].get_percent_value() <= 75) {
					secondary_color = ColorYellow;
				}
			}
		} else {
			color = CVideo::MapRGB(unit.Player->get_minimap_color());

			for (int i = 0; i < PlayerMax; ++i) {
				if (unit.TeamSelected & (1 << i)) {
					color = CVideo::MapRGB(CPlayer::Players[i]->get_minimap_color());
				}
			}
		}
	} else if (CursorBuilding && unit.Type->BoolFlag[BUILDING_INDEX].value
			   && unit.CurrentAction() != UnitAction::Die
			   && (unit.Player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(unit))) {
		// If building mark all own buildings
		color = ColorGray;
	} else {
		return;
	}

	if (secondary_color == 0) {
		secondary_color = color;
	}

	DrawSelection(color, secondary_color, box_rect.x(), box_rect.y(), box_rect.right(), box_rect.bottom(), render_commands);
}

/**
**  Don't show selected units.
**
**  @param color  Color to draw, nothing in this case.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionNone(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands)
{
	Q_UNUSED(render_commands)
}

/**
**  Show selected units with circle.
**
**  @param color  Color to draw circle
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCircle(IntColor color, IntColor secondary_color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	Q_UNUSED(secondary_color);

	Video.DrawCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
						 std::min((x2 - x1) / 2, (y2 - y1) / 2) + 2, render_commands);
}

/**
**  Show selected units with circle.
**
**  @param color  Color to draw and fill circle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCircleWithTrans(IntColor color, IntColor secondary_color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	Q_UNUSED(secondary_color);

	Video.FillTransCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
							  std::min((x2 - x1) / 2, (y2 - y1) / 2), 95, render_commands);
	//Wyrmgus start
	/*
	Video.DrawCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
						 std::min((x2 - x1) / 2, (y2 - y1) / 2));
	*/
	//Wyrmgus end
}

/**
**  Draw selected rectangle around the unit.
**
**  @param color  Color to draw rectangle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionRectangle(const IntColor int_color, const IntColor secondary_int_color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const QColor color = Video.GetRGBA(int_color);
	const QColor secondary_color = Video.GetRGBA(secondary_int_color);

	render_commands.push_back([color, secondary_color, x1, y1, x2, y2](renderer *renderer) {
		const double line_width = (1 * preferences::get()->get_scale_factor()).to_double();

		if (color == secondary_color) {
			renderer->draw_rect(QPoint(x1, y1), QSize(x2 + 1 - x1, y2 + 1 - y1), color, line_width);
		} else {
			static constexpr int base_corner_pixels = 6;

			const int corner_pixels = (base_corner_pixels * preferences::get()->get_scale_factor()).to_int();

			renderer->draw_vertical_line(QPoint(x1, y1), corner_pixels, color, line_width);
			renderer->draw_horizontal_line(QPoint(x1, y1), corner_pixels, color, line_width);

			renderer->draw_vertical_line(QPoint(x2, y1), corner_pixels, color, line_width);
			renderer->draw_horizontal_line(QPoint(x2, y1), -corner_pixels, color, line_width);

			renderer->draw_vertical_line(QPoint(x1, y2), -corner_pixels, color, line_width);
			renderer->draw_horizontal_line(QPoint(x1, y2), corner_pixels, color, line_width);

			renderer->draw_vertical_line(QPoint(x2, y2), -corner_pixels, color, line_width);
			renderer->draw_horizontal_line(QPoint(x2, y2), -corner_pixels, color, line_width);

			const int secondary_start_x = x1 + corner_pixels;
			const int secondary_width = (x2 + 1 - x1) - corner_pixels * 2;
			renderer->draw_horizontal_line(QPoint(secondary_start_x, y1), secondary_width, secondary_color, line_width);
			renderer->draw_horizontal_line(QPoint(secondary_start_x, y2), secondary_width, secondary_color, line_width);

			const int secondary_start_y = y1 + corner_pixels;
			const int secondary_height = (y2 + 1 - y1) - corner_pixels * 2;
			renderer->draw_vertical_line(QPoint(x1, secondary_start_y), secondary_height, secondary_color, line_width);
			renderer->draw_vertical_line(QPoint(x2, secondary_start_y), secondary_height, secondary_color, line_width);
		}
	});
}

/**
**  Draw selected rectangle around the unit.
**
**  @param color  Color to draw and fill rectangle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionRectangleWithTrans(IntColor color, IntColor secondary_color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	Q_UNUSED(secondary_color);

	Video.DrawRectangleClip(color, x1, y1, x2 - x1, y2 - y1, render_commands);
	Video.FillTransRectangleClip(color, x1 + 1, y1 + 1,
								 x2 - x1 - 2, y2 - y1 - 2, 75, render_commands);
}

/**
**  Draw selected corners around the unit.
**
**  @param color  Color to draw corners.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCorners(const IntColor int_color, const IntColor secondary_int_color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	static constexpr int base_corner_pixels = 6;

	Q_UNUSED(secondary_int_color);

	const QColor color = Video.GetRGBA(int_color);

	render_commands.push_back([color, x1, y1, x2, y2](renderer *renderer) {
		const double line_width = (1 * preferences::get()->get_scale_factor()).to_double();
		const int corner_pixels = (base_corner_pixels * preferences::get()->get_scale_factor()).to_int();

		renderer->draw_vertical_line(QPoint(x1, y1), corner_pixels, color, line_width);
		renderer->draw_horizontal_line(QPoint(x1, y1), corner_pixels, color, line_width);

		renderer->draw_vertical_line(QPoint(x2, y1), corner_pixels, color, line_width);
		renderer->draw_horizontal_line(QPoint(x2, y1), -corner_pixels, color, line_width);

		renderer->draw_vertical_line(QPoint(x1, y2), -corner_pixels, color, line_width);
		renderer->draw_horizontal_line(QPoint(x1, y2), corner_pixels, color, line_width);

		renderer->draw_vertical_line(QPoint(x2, y2), -corner_pixels, color, line_width);
		renderer->draw_horizontal_line(QPoint(x2, y2), -corner_pixels, color, line_width);
	});
}


/**
**  Return the index of the sprite named SpriteName.
**
**  @param SpriteName    Name of the sprite.
**
**  @return              Index of the sprite. -1 if not found.
*/
int GetSpriteIndex(const char *SpriteName)
{
	assert_throw(SpriteName != nullptr);
	for (unsigned int i = 0; i < DecoSprite.Name.size(); ++i) {
		if (!strcmp(SpriteName, DecoSprite.Name[i].c_str())) {
			return i;
		}
	}
	return -1;
}

/**
**  Define the sprite to show variables.
**
**  @param l    Lua_state
*/
static int CclDefineSprites(lua_State *l)
{
	const int args = lua_gettop(l);

	for (int i = 0; i < args; ++i) {
		Decoration deco;

		lua_pushnil(l);
		const char *name = nullptr;// name of the current sprite.
		while (lua_next(l, i + 1)) {
			const char *key = LuaToString(l, -2); // key name
			if (!strcmp(key, "Name")) {
				name = LuaToString(l, -1);
			} else if (!strcmp(key, "File")) {
				deco.File = LuaToString(l, -1);
			} else if (!strcmp(key, "Offset")) {
				CclGetPos(l, &deco.HotPos.x, &deco.HotPos.y);
			} else if (!strcmp(key, "Size")) {
				CclGetPos(l, &deco.Width, &deco.Height);
			} else { // Error.
				LuaError(l, "incorrect field '%s' for the DefineSprite." _C_ key);
			}
			lua_pop(l, 1); // pop the value;
		}
		if (name == nullptr) {
			LuaError(l, "CclDefineSprites requires the Name flag for sprite.");
		}
		int index = GetSpriteIndex(name); // Index of the Sprite.
		if (index == -1) { // new sprite.
			index = DecoSprite.SpriteArray.size();
			DecoSprite.Name.push_back(name);
			DecoSprite.SpriteArray.push_back(deco);
		} else {
			DecoSprite.SpriteArray[index].File.clear();
			DecoSprite.SpriteArray[index] = deco;
		}
		// Now verify validity.
		if (DecoSprite.SpriteArray[index].File.empty()) {
			LuaError(l, "CclDefineSprites requires the File flag for sprite.");
		}
		// FIXME check if file is valid with good size ?
	}
	return 0;
}

/**
**  Register CCL features for decorations.
*/
void DecorationCclRegister()
{
	DecoSprite.Name.clear();
	DecoSprite.SpriteArray.clear();

	lua_register(Lua, "DefineSprites", CclDefineSprites);
}

/**
**  Return the amount of decorations.
*/
int GetDecorationsCount()
{
	return DecoSprite.SpriteArray.size();
}

/**
**  Load decoration.
*/
void LoadDecorations()
{
	ShowLoadProgress("%s", _("Loading Decorations..."));
		
	std::vector<Decoration>::iterator i;
	for (i = DecoSprite.SpriteArray.begin(); i != DecoSprite.SpriteArray.end(); ++i) {
		if ((*i).Sprite) {
			continue;
		}

		(*i).Sprite = CGraphic::New((*i).File, (*i).Width, (*i).Height);
		(*i).Sprite->Load(preferences::get()->get_scale_factor());
	}
}

void CleanDecorations()
{
	DecoSprite.Name.clear();
	DecoSprite.SpriteArray.clear();
}

/**
**  Draw bar for variables.
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @todo fix color configuration.
*/
void CDecoVarBar::Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	assert_throw(var.Max > 0);

	int height = this->Height;
	if (height == 0) { // Default value
		height = type.get_box_height(); // Better size ? {,Box, Tile}
	}
	int width = this->Width;
	if (width == 0) { // Default value
		width = type.get_box_width(); // Better size ? {,Box, Tile}
	}
	int h;
	int w;
	if (this->IsVertical)  { // Vertical
		w = width;
		h = var.Value * height / var.Max;
	} else {
		w = var.Value * width / var.Max;
		h = height;
	}
	if (this->IsCenteredInX) {
		x -= w / 2;
	}
	if (this->IsCenteredInY) {
		y -= h / 2;
	}

	char b = this->BorderSize; // BorderSize.
	// Could depend of (value / max)
	int f = var.Value * 100 / var.Max;
	IntColor bcolor = ColorBlack; // Deco->Data.Bar.BColor  // Border color.
	IntColor color = f > 50 ? (f > 75 ? ColorGreen : ColorYellow) : (f > 25 ? ColorOrange : ColorRed);// inside color.
	// Deco->Data.Bar.Color
	if (b) {
		if (this->ShowFullBackground) {
			Video.FillRectangleClip(bcolor, x - b, y - b, 2 * b + width, 2 * b + height, render_commands);
		} else {
			if (this->SEToNW) {
				Video.FillRectangleClip(bcolor, x - b - w + width, y - b - h + height,
										2 * b + w, 2 * b + h, render_commands);
			} else {
				Video.FillRectangleClip(bcolor, x - b, y - b, 2 * b + w, 2 * b + h, render_commands);
			}
		}
	}
	if (this->SEToNW) {
		Video.FillRectangleClip(color, x - w + width, y - h + height, w, h, render_commands);
	} else {
		Video.FillRectangleClip(color, x, y, w, h, render_commands);
	}
}

/**
**  Print variable values (and max....).
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @todo fix font/color configuration.
*/
void CDecoVarText::Draw(int x, int y, const unit_type &/*type*/, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	if (this->IsCenteredInX) {
		x -= 2; // wyrmgus::defines::get()->get_game_font()->Width(buf) / 2, with buf = str(Value)
	}
	if (this->IsCenteredInY) {
		y -= this->Font->Height() / 2;
	}
	CLabel(this->Font).DrawClip(x, y, var.Value, render_commands);
}

/**
**  Draw a sprite with is like a bar (several stages)
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @todo fix sprite configuration.
*/
void CDecoVarSpriteBar::Draw(int x, int y, const unit_type &/*type*/, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	assert_throw(var.Max > 0);
	assert_throw(this->NSprite != -1);

	Decoration &decosprite = DecoSprite.SpriteArray[(int)this->NSprite];
	CGraphic &sprite = *decosprite.Sprite;
	x += decosprite.HotPos.x; // in addition of OffsetX... Useful ?
	y += decosprite.HotPos.y; // in addition of OffsetY... Useful ?

	//Wyrmgus start
	if (!decosprite.Sprite) {
		fprintf(stderr, "Tried to load non-existent DecoSprite\n");
	}
	//Wyrmgus end
	
	int n = sprite.NumFrames - 1; // frame of the sprite to show.
	n -= (n * var.Value) / var.Max;

	if (this->IsCenteredInX) {
		x -= sprite.Width / 2;
	}
	if (this->IsCenteredInY) {
		y -= sprite.Height / 2;
	}
	sprite.DrawFrameClip(n, x, y, render_commands);
}

/**
**  Draw a static sprite.
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**
**  @todo fix sprite configuration configuration.
*/
void CDecoVarStaticSprite::Draw(int x, int y, const unit_type &/*type*/, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	Decoration &decosprite = DecoSprite.SpriteArray[(int)this->NSprite];
	CGraphic &sprite = *decosprite.Sprite;

	x += decosprite.HotPos.x; // in addition of OffsetX... Useful ?
	y += decosprite.HotPos.y; // in addition of OffsetY... Useful ?
	if (this->IsCenteredInX) {
		x -= sprite.Width / 2;
	}
	if (this->IsCenteredInY) {
		y -= sprite.Height / 2;
	}
	if (this->FadeValue && var.Value < this->FadeValue) {
		int alpha = var.Value * 255 / this->FadeValue;
		sprite.DrawFrameClipTrans(this->n, x, y, alpha, render_commands);
	} else {
		sprite.DrawFrameClip(this->n, x, y, render_commands);
	}
}

/**
**  Draw decoration (invis, for the unit.)
**
**  @param unit       Pointer to the unit.
**  @param type       Type of the unit.
**  @param screenPos  Screen position of the unit.
*/
static void DrawDecoration(const CUnit &unit, const wyrmgus::unit_type &type, const PixelPos &screenPos, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int x = screenPos.x;
	int y = screenPos.y;

	size_t status_effect_count = 0;

	static constexpr std::array status_effect_offsets = { QPoint(0, 0), QPoint(16, 0), QPoint(0, 16), QPoint(16, 16), QPoint(32, 0), QPoint(32, 16), QPoint(0, 32), QPoint(16, 32), QPoint(32, 32), QPoint(48, 0), QPoint(48, 16), QPoint(48, 32), QPoint(0, 48), QPoint(16, 48), QPoint(32, 48), QPoint(48, 48) };

	UpdateUnitVariables(const_cast<CUnit &>(unit));
	// Now show decoration for each variable.
	for (std::vector<CDecoVar *>::const_iterator i = UnitTypeVar.DecoVar.begin(); i < UnitTypeVar.DecoVar.end(); ++i) {
		const CDecoVar &var = *(*i);
		const bool is_status_effect = var.status_effect.has_value();
		const int value = is_status_effect ? unit.get_status_effect_timer(var.status_effect.value()) : unit.Variable[var.Index].Value;
		//Wyrmgus start
//		const int max = unit.Variable[var.Index].Max;
		const int max = is_status_effect ? value : unit.GetModifiedVariable(var.Index, VariableAttribute::Max);
		//Wyrmgus end

		const bool enable = is_status_effect ? (value > 0) : (unit.Variable[var.Index].Enable != 0);

		assert_throw(value <= max);

		if ((value == 0 && !var.ShowWhenNull) || (value == max && !var.ShowWhenMax)
			|| (var.HideHalf && value != 0 && value != max)
			|| (!var.ShowIfNotEnable && !enable)
			|| (var.ShowOnlySelected && !unit.Selected)
			|| (unit.Player->get_type() == player_type::neutral && var.HideNeutral)
			//Wyrmgus start
			|| (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->is_enemy_of(unit) && !CPlayer::GetThisPlayer()->is_allied_with(unit) && var.HideNeutral)
			//Wyrmgus end
			|| (CPlayer::GetThisPlayer()->is_enemy_of(unit) && !var.ShowOpponent)
			|| (CPlayer::GetThisPlayer()->is_allied_with(unit) && (unit.Player != CPlayer::GetThisPlayer()) && var.HideAllied)
			//Wyrmgus start
			|| (unit.Player == CPlayer::GetThisPlayer() && var.HideSelf)
			|| unit.Type->BoolFlag[DECORATION_INDEX].value // don't show decorations for decoration units
//			|| max == 0)) {
			|| (var.ShowIfCanCastAnySpell && !unit.CanCastAnySpell())
			|| (var.hero_symbol && !preferences::get()->is_hero_symbol_enabled())
			|| (var.hp_bar && !preferences::get()->is_hp_bar_enabled())
			|| (var.resource_bar && !preferences::get()->is_resource_bar_enabled())
			|| max == 0 || max < var.MinValue
		) {
			continue;
			//Wyrmgus end
		}

		int offset_x = var.OffsetX;
		int offset_y = var.OffsetY;

		if (is_status_effect || var.show_as_status_effect) {
			const QPoint &status_effect_offset = status_effect_offsets.at(status_effect_count);
			offset_x += status_effect_offset.x();
			offset_y += status_effect_offset.y();
			++status_effect_count;
		}

		var.Draw(
			x + (offset_x * preferences::get()->get_scale_factor()).to_int() + var.OffsetXPercent * unit.Type->get_tile_width() * defines::get()->get_scaled_tile_width() / 100,
			y + (offset_y * preferences::get()->get_scale_factor()).to_int() + var.OffsetYPercent * unit.Type->get_tile_height() * defines::get()->get_scaled_tile_height() / 100,
			type, unit.Variable[var.Index], render_commands);
	}

	// Draw group number
	if (unit.Selected && unit.GroupId != 0
#ifndef DEBUG
		&& unit.Player == CPlayer::GetThisPlayer()
#endif
	   ) {
		int groupId = 0;

		if (unit.Player->AiEnabled) {
			groupId = unit.GroupId - 1;
		} else {
			for (groupId = 0; !(unit.GroupId & (1 << groupId)); ++groupId) {
			}
		}
		const int width = wyrmgus::defines::get()->get_game_font()->Width(groupId);
		x += (unit.Type->get_tile_width() * defines::get()->get_scaled_tile_width() + (unit.Type->get_box_width() * preferences::get()->get_scale_factor()).to_int()) / 2 - width;
		const int height = wyrmgus::defines::get()->get_game_font()->Height();
		y += (unit.Type->get_tile_height() * defines::get()->get_scaled_tile_height() + (unit.Type->get_box_height() * preferences::get()->get_scale_factor()).to_int()) / 2 - height;
		CLabel(defines::get()->get_game_font()).DrawClip(x, y, groupId, render_commands);
	}
}

/**
**  Draw unit's shadow.
**
**  @param type   Pointer to the unit type.
**  @param frame  Frame number
**  @param screenPos  Screen position of the unit.
**
**  @todo FIXME: combine new shadow code with old shadow code.
*/
void DrawShadow(const unit_type &type, const std::shared_ptr<CGraphic> &sprite, int frame, const PixelPos &screenPos, std::vector<std::function<void(renderer *)>> &render_commands)
{
	// Draw normal shadow sprite if available
	//Wyrmgus start
//	if (!type.ShadowSprite) {
	if (!sprite) {
	//Wyrmgus end
		return;
	}
	PixelPos pos = screenPos;
	pos -= PixelPos((sprite->get_frame_size() - type.get_tile_size() * defines::get()->get_scaled_tile_size()) / 2);
	pos.x += ((type.get_offset().x() + type.ShadowOffsetX) * preferences::get()->get_scale_factor()).to_int();
	pos.y += ((type.get_offset().y() + type.ShadowOffsetY) * preferences::get()->get_scale_factor()).to_int();

	if (type.Flip) {
		if (frame < 0) {
			sprite->DrawFrameClipX(-frame - 1, pos.x, pos.y, render_commands);
		} else {
			sprite->DrawFrameClip(frame, pos.x, pos.y, render_commands);
		}
	} else {
		int row = type.get_num_directions() / 2 + 1;
		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.get_num_directions() + type.get_num_directions() - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.get_num_directions() + frame % row;
		}
		//Wyrmgus start
//		type.ShadowSprite->DrawFrameClip(frame, pos.x, pos.y);
		sprite->DrawFrameClip(frame, pos.x, pos.y, render_commands);
		//Wyrmgus end
	}
}

//Wyrmgus start
void DrawPlayerColorOverlay(const wyrmgus::unit_type &type, const std::shared_ptr<CPlayerColorGraphic> &sprite, const int player, int frame, const PixelPos &screenPos, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (!sprite) {
		return;
	}

	const player_color *player_color = CPlayer::Players[player]->get_player_color();

	PixelPos pos = screenPos;
	// FIXME: move this calculation to high level.
	pos -= PixelPos((sprite->get_frame_size() - type.get_tile_size() * defines::get()->get_scaled_tile_size()) / 2);
	pos.x += (type.get_offset().x() * preferences::get()->get_scale_factor()).to_int();
	pos.y += (type.get_offset().y() * preferences::get()->get_scale_factor()).to_int();

	bool flip = false;

	if (type.Flip) {
		if (frame < 0) {
			flip = true;
			frame = -frame - 1;
		}
	} else {
		const int row = type.get_num_directions() / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.get_num_directions() + type.get_num_directions() - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.get_num_directions() + frame % row;
		}
	}

	unsigned char opacity = 255;
	if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
		opacity = int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value);
	}

	const color_modification color_modification(type.get_hue_rotation(), type.get_colorization(), type.get_hue_ignored_colors(), player_color, time_of_day);
	sprite->render_frame(frame, pos, color_modification, false, flip, opacity, 100, render_commands);
}

void DrawOverlay(const unit_type &type, const std::shared_ptr<CGraphic> &sprite, int player, int frame, const PixelPos &screenPos, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (!sprite) {
		return;
	}
	PixelPos pos = screenPos;
	// FIXME: move this calculation to high level.
	pos -= PixelPos((sprite->get_frame_size() - type.get_tile_size() * wyrmgus::defines::get()->get_scaled_tile_size()) / 2);
	pos.x += (type.get_offset().x() * preferences::get()->get_scale_factor()).to_int();
	pos.y += (type.get_offset().y() * preferences::get()->get_scale_factor()).to_int();

	if (type.Flip) {
		if (frame < 0) {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day, render_commands);
			} else {
				sprite->DrawFrameClipX(-frame - 1, pos.x, pos.y, time_of_day, render_commands);
			}
		} else {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day, render_commands);
			} else {
				sprite->DrawFrameClip(frame, pos.x, pos.y, time_of_day, render_commands);
			}
		}
	} else {
		const int row = type.get_num_directions() / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.get_num_directions() + type.get_num_directions() - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.get_num_directions() + frame % row;
		}
		if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
			sprite->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), time_of_day, render_commands);
		} else {
			sprite->DrawFrameClip(frame, pos.x, pos.y, time_of_day, render_commands);
		}
	}
}
//Wyrmgus end

/**
**  Show the current order of a unit.
**
**  @param unit  Pointer to the unit.
*/
void ShowOrder(const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (unit.Destroyed || unit.Removed) {
		return;
	}
#ifndef DEBUG
	if (!CPlayer::GetThisPlayer()->is_allied_with(unit) && unit.Player != CPlayer::GetThisPlayer()) {
		return;
	}
#endif
	// Get current position
	const PixelPos mapPos = unit.get_scaled_map_pixel_pos_center();
	PixelPos screenStartPos = CurrentViewport->scaled_map_to_screen_pixel_pos(mapPos);
	const bool flushed = unit.Orders[0]->Finished;

	COrder *order = nullptr;
	// If the current order is cancelled show the next one
	if (unit.Orders.size() > 1 && flushed) {
		order = unit.Orders[1].get();
	} else {
		order = unit.Orders[0].get();
	}
	PixelPos screenPos = order->Show(*CurrentViewport, screenStartPos, render_commands);
	// Show the rest of the orders
	for (size_t i = 1 + (flushed ? 1 : 0); i < unit.Orders.size(); ++i) {
		screenPos = unit.Orders[i]->Show(*CurrentViewport, screenPos, render_commands);
	}

	// Show order for new trained units
	if (unit.NewOrder) {
		unit.NewOrder->Show(*CurrentViewport, screenStartPos, render_commands);
	}
	
	//Wyrmgus start
	//if unit has rally point, show it
	if (unit.get_rally_point_pos().x() != -1 && unit.get_rally_point_pos().y() != -1 && unit.get_rally_point_map_layer() != nullptr && unit.get_rally_point_map_layer() == UI.CurrentMapLayer) {
		Video.FillCircleClip(ColorGreen, CurrentViewport->TilePosToScreen_Center(unit.get_rally_point_pos()), (3 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}
	//Wyrmgus end
}

/**
**  Draw additional informations of a unit.
**
**  @param unit  Unit pointer of drawn unit.
**  @param type  Unit-type pointer.
**  @param screenPos  screen pixel (top left) position of unit.
**
**  @todo FIXME: The different styles should become a function call.
*/
static void DrawInformations(const CUnit &unit, const unit_type &type, const PixelPos &screenPos, std::vector<std::function<void(renderer *)>> &render_commands)
{
	// For debug draw sight, react and attack range!
	if (IsOnlySelected(unit)) {
		const PixelPos center(screenPos + type.get_scaled_half_tile_pixel_size());

		if (Preference.ShowSightRange) {
			const int value = unit.CurrentSightRange;
			const int radius = value * wyrmgus::defines::get()->get_scaled_tile_width() + (type.get_tile_width() - 1) * wyrmgus::defines::get()->get_scaled_tile_width() / 2;

			if (value) {
				// Radius -1 so you can see all ranges
				Video.DrawCircleClip(ColorGreen, center.x, center.y, radius - 1, render_commands);
			}
		}
		//Wyrmgus start
//		if (type.CanAttack) {
		if (unit.CanAttack(true)) {
		//Wyrmgus end
			if (Preference.ShowReactionRange) {
				const int value = unit.GetReactionRange();
				const int radius = value * wyrmgus::defines::get()->get_scaled_tile_width() + (type.get_tile_width() - 1) * defines::get()->get_scaled_tile_width() / 2;

				if (value) {
					Video.DrawCircleClip(ColorBlue, center.x, center.y, radius, render_commands);
				}
			}
			if (Preference.ShowAttackRange) {
				//Wyrmgus start
//				const int value = stats.Variables[ATTACKRANGE_INDEX].Max;
				const int value = unit.get_best_attack_range();
				
				//Wyrmgus end
				const int radius = value * defines::get()->get_scaled_tile_width() + (type.get_tile_width() - 1) * defines::get()->get_scaled_tile_width() / 2;

				if (value) {
					// Radius +1 so you can see all ranges
					Video.DrawCircleClip(ColorGreen, center.x, center.y, radius - 1, render_commands);
				}
			}
		}
		
		//Wyrmgus start
		if (unit.IsAlive() && unit.CurrentAction() != UnitAction::Built) {
			//show aura range if the unit has an aura
			if (unit.Variable[LEADERSHIPAURA_INDEX].Value > 0 || unit.Variable[REGENERATIONAURA_INDEX].Value > 0 || unit.Variable[HYDRATINGAURA_INDEX].Value > 0) {
				const int value = unit.get_aura_range();
				const int radius = value * defines::get()->get_scaled_tile_width() + (type.get_tile_width() - 1) * defines::get()->get_scaled_tile_width() / 2;

				if (value) {
					Video.DrawCircleClip(ColorBlue, center.x, center.y, radius, render_commands);
				}
			}
		}
		//Wyrmgus end
	}

	// FIXME: johns: ugly check here, should be removed!
	if (unit.CurrentAction() != UnitAction::Die && (unit.IsVisible(*CPlayer::GetThisPlayer()) || ReplayRevealMap)) {
		DrawDecoration(unit, type, screenPos, render_commands);
	}
}

/**
**  Draw construction shadow.
**
**  @param unit    Unit pointer.
**  @param cframe  Construction frame
**  @param frame   Frame number to draw.
**  @param screenPos  screen (top left) position of the unit.
*/
static void DrawConstructionShadow(const CUnit &unit, const unit_type &type, const construction_frame *cframe, int frame, const PixelPos &screenPos, std::vector<std::function<void(renderer *)>> &render_commands)
{
	PixelPos pos = screenPos;
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	const unit_type_variation *variation = unit.GetVariation();
	if (cframe->get_image_type() != construction_image_type::construction) {
		if (variation && variation->ShadowSprite) {
			pos -= PixelPos((variation->ShadowSprite->get_frame_size() - type.get_tile_size() * wyrmgus::defines::get()->get_scaled_tile_size()) / 2);
			pos.x += ((type.ShadowOffsetX + type.get_offset().x()) * scale_factor).to_int();
			pos.y += ((type.ShadowOffsetY + type.get_offset().y()) * scale_factor).to_int();
			if (frame < 0) {
				variation->ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y, render_commands);
			} else {
				variation->ShadowSprite->DrawFrameClip(frame, pos.x, pos.y, render_commands);
			}
		} else if (type.ShadowSprite) {
			pos -= PixelPos((type.ShadowSprite->get_frame_size() - type.get_tile_size() * wyrmgus::defines::get()->get_scaled_tile_size()) / 2);
			pos.x += ((type.ShadowOffsetX + type.get_offset().x()) * scale_factor).to_int();
			pos.y += ((type.ShadowOffsetY + type.get_offset().y()) * scale_factor).to_int();
			if (frame < 0) {
				type.ShadowSprite->DrawFrameClipX(-frame - 1, pos.x, pos.y, render_commands);
			} else {
				type.ShadowSprite->DrawFrameClip(frame, pos.x, pos.y, render_commands);
			}
		}
	}
}

/**
**  Draw construction.
**
**  @param unit    Unit pointer.
**  @param cframe  Construction frame to draw.
**  @param type    Unit type.
**  @param frame   Frame number.
**  @param screenPos  screen (top left) position of the unit.
*/
static void DrawConstruction(const int player, const construction_frame *cframe,
							 const CUnit &unit, const unit_type &type, int frame, const PixelPos &screenPos, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
{
	PixelPos pos = screenPos;
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	const player_color *player_color = CPlayer::Players[player]->get_player_color();

	if (cframe->get_image_type() == construction_image_type::construction) {
		const unit_type_variation *variation = unit.GetVariation();
		if (variation != nullptr && variation->get_construction() != nullptr) {
			const construction *construction = variation->get_construction();
			pos.x -= (construction->get_frame_width() * scale_factor).to_int() / 2;
			pos.y -= (construction->get_frame_height() * scale_factor).to_int() / 2;
			if (frame < 0) {
				construction->get_graphics()->DrawPlayerColorFrameClipX(player_color, -frame - 1, pos.x, pos.y, time_of_day, render_commands);
			} else {
				construction->get_graphics()->DrawPlayerColorFrameClip(player_color, frame, pos.x, pos.y, time_of_day, render_commands);
			}
		} else {
			const construction *construction = type.get_construction();
			pos.x -= (construction->get_frame_width() * scale_factor).to_int() / 2;
			pos.y -= (construction->get_frame_height() * scale_factor).to_int() / 2;
			if (frame < 0) {
				construction->get_graphics()->DrawPlayerColorFrameClipX(player_color, -frame - 1, pos.x, pos.y, time_of_day, render_commands);
			} else {
				construction->get_graphics()->DrawPlayerColorFrameClip(player_color, frame, pos.x, pos.y, time_of_day, render_commands);
			}
		}
	} else {
		//Wyrmgus start
//		pos.x += type.get_offset().x() - type.Width / 2;
//		pos.y += type.get_offset().y() - type.Height / 2;
		QSize frame_size = type.get_frame_size();
		const wyrmgus::unit_type_variation *variation = unit.GetVariation();
		if (variation != nullptr && variation->get_frame_size() != QSize(0, 0)) {
			frame_size = variation->get_frame_size();
		}
		frame_size *= scale_factor;
		pos.x += (type.get_offset().x() * scale_factor).to_int() - frame_size.width() / 2;
		pos.y += (type.get_offset().y() * scale_factor).to_int() - frame_size.height() / 2;
		//Wyrmgus end
		if (frame < 0) {
			frame = -frame - 1;
		}
		//Wyrmgus start
//		type.Sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y);
		if (variation && variation->Sprite) {
			variation->Sprite->DrawPlayerColorFrameClip(player_color, frame, pos.x, pos.y, time_of_day, render_commands);
		} else {
			type.Sprite->DrawPlayerColorFrameClip(player_color, frame, pos.x, pos.y, time_of_day, render_commands);
		}
		//Wyrmgus end
	}
}

/**
**  Units on map:
*/

/**
**  Draw unit on map.
*/
void CUnit::Draw(const CViewport &vp, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	int frame;
	int state;
	int under_construction;
	const wyrmgus::construction_frame *cframe = nullptr;
	const wyrmgus::unit_type *type = nullptr;

	if (this->Destroyed || this->Container || this->Type->BoolFlag[REVEALER_INDEX].value) { // Revealers are not drawn
		return;
	}

	bool IsVisible = this->IsVisible(*CPlayer::GetThisPlayer());

	// Those should have been filtered. Check doesn't make sense with ReplayRevealMap
	assert_throw(ReplayRevealMap || this->Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value || IsVisible);

	//Wyrmgus start
//	int player = this->RescuedFrom ? this->RescuedFrom->get_index() : this->Player->get_index();
	int player = this->GetDisplayPlayer();
	//Wyrmgus end
	const UnitAction action = this->CurrentAction();
	PixelPos screenPos;
	if (ReplayRevealMap || IsVisible) {
		screenPos = vp.scaled_map_to_screen_pixel_pos(this->get_scaled_map_pixel_pos_top_left());
		type = this->Type;
		frame = this->Frame;
		state = (action == UnitAction::Built) | ((action == UnitAction::UpgradeTo) << 1);
		under_construction = this->UnderConstruction;
		// Reset Type to the type being upgraded to
		if (action == UnitAction::UpgradeTo) {
			const COrder_UpgradeTo &order = *static_cast<COrder_UpgradeTo *>(this->CurrentOrder());

			type = &order.GetUnitType();
		}

		if (this->CurrentAction() == UnitAction::Built) {
			COrder_Built &order = *static_cast<COrder_Built *>(this->CurrentOrder());

			cframe = order.get_frame();
		} else {
			cframe = nullptr;
		}
	} else {
		screenPos = vp.TilePosToScreen_TopLeft(this->Seen.tilePos);

		screenPos.x += (this->Seen.pixel_offset.x() * preferences::get()->get_scale_factor()).to_int();
		screenPos.y += (this->Seen.pixel_offset.y() * preferences::get()->get_scale_factor()).to_int();
		frame = this->Seen.Frame;
		type = this->Seen.Type;
		under_construction = this->Seen.UnderConstruction;
		state = this->Seen.State;
		cframe = this->Seen.cframe;
	}

#ifdef DYNAMIC_LOAD
	if (!type->Sprite) {
		LoadUnitTypeSprite(type);
	}
#endif

	if (!IsVisible && frame == UnitNotSeen) {
		DebugPrint("FIXME: Something is wrong, unit %d not seen but drawn time %lu?.\n" _C_
				   UnitNumber(*this) _C_ GameCycle);
		return;
	}

	//Wyrmgus start
	//
	// Show that the unit is selected
	//
	// draw it under everything else
	DrawUnitSelection(vp, *this, render_commands);
	//Wyrmgus end

	const wyrmgus::unit_type_variation *variation = this->GetVariation();

	if (state == 1 && under_construction && cframe) {
		//Wyrmgus start
//		DrawConstructionShadow(*type, cframe, frame, screenPos);
		DrawConstructionShadow(*this, *type, cframe, frame, screenPos, render_commands);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		if (action != UnitAction::Die) {
		//Wyrmgus end
			//Wyrmgus start
//			DrawShadow(*type, frame, screenPos);
			if (variation && variation->ShadowSprite) {
				DrawShadow(*type, variation->ShadowSprite, frame, screenPos, render_commands);
			} else if (type->ShadowSprite) {
				DrawShadow(*type, type->ShadowSprite, frame, screenPos, render_commands);
			}
			//Wyrmgus end
		//Wyrmgus start
//		}
		//Wyrmgus end
	}

	//Wyrmgus start
	//
	// Show that the unit is selected
	//
//	DrawUnitSelection(vp, *this);
	//Wyrmgus end

	const time_of_day *time_of_day = this->get_center_tile_time_of_day();
	
	//Wyrmgus start
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(MountImageLayer), player, frame, screenPos, time_of_day, render_commands); // draw the mount just before the body
	
	//draw the backpack before everything but the shadow if facing south (or the still frame, since that also faces south), southeast or southwest
	if (this->Direction == LookingS || frame == type->StillFrame || this->Direction == LookingSE || this->Direction == LookingSW) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(BackpackImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}
	
	//draw the left arm before the body if not facing south (or the still frame, since that also faces south); if the position of the arms in the southeast frame is inverted, don't draw the left arm yet either
	if (
		(this->Direction != LookingS || this->CurrentAction() == UnitAction::Die)
		&& frame != type->StillFrame
		&& !(
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		&& !(
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		//draw the shield before the left arm if not facing south
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ShieldImageLayer), player, frame, screenPos, time_of_day, render_commands);

		DrawPlayerColorOverlay(*type, this->GetLayerSprite(LeftArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}
	
	//draw the right arm before the body if facing north, or if facing southeast/southwest and the arms are inverted for that direction
	if (
		(this->Direction == LookingN && this->CurrentAction() != UnitAction::Die)
		|| (
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		|| (
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(WeaponImageLayer), player, frame, screenPos, time_of_day, render_commands);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightHandImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}
	//Wyrmgus end

	//
	// Adjust sprite for Harvesters.
	//
	std::shared_ptr<CPlayerColorGraphic> sprite = type->Sprite;
	if (type->BoolFlag[HARVESTER_INDEX].value && this->get_current_resource() != nullptr) {
		const resource_info *res_info = type->get_resource_info(this->get_current_resource());
		if (this->ResourcesHeld) {
			if (res_info->SpriteWhenLoaded) {
				sprite = res_info->SpriteWhenLoaded;
			}
		} else {
			if (res_info->SpriteWhenEmpty) {
				sprite = res_info->SpriteWhenEmpty;
			}
		}
	}

	//Wyrmgus start
	// Adjust sprite for variations.
	if (variation && variation->get_unit_type() == type) {
		if (variation->Sprite) {
			sprite = variation->Sprite;
		}
		if (type->BoolFlag[HARVESTER_INDEX].value && this->CurrentResource) {
			if (this->ResourcesHeld) {
				if (variation->SpriteWhenLoaded[this->CurrentResource]) {
					sprite = variation->SpriteWhenLoaded[this->CurrentResource];
				}
			} else {
				if (variation->SpriteWhenEmpty[this->CurrentResource]) {
					sprite = variation->SpriteWhenEmpty[this->CurrentResource];
				}
			}
		}
	}
	//Wyrmgus end

	//
	// Now draw!
	// Buildings under construction/upgrade/ready.
	//
	if (state == 1) {
		if (under_construction && cframe) {
			const PixelPos pos(screenPos + type->get_scaled_half_tile_pixel_size());
			DrawConstruction(player, cframe, *this, *type, frame, pos, time_of_day, render_commands);
		} else {
			DrawUnitType(*type, sprite, player, frame, screenPos, time_of_day, render_commands);
		}
		//
		// Draw the future unit type, if upgrading to it.
		//
	} else {
		DrawUnitType(*type, sprite, player, frame, screenPos, time_of_day, render_commands);
	}
	
	//Wyrmgus start
	//draw the left arm and right arm clothing after the body, even if the arms were drawn before
	if ((this->Direction != LookingS || this->CurrentAction() == UnitAction::Die) && frame != type->StillFrame) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingLeftArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}
	if (
		(this->Direction == LookingN && this->CurrentAction() != UnitAction::Die)
		|| (
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		|| (
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingRightArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}

	DrawPlayerColorOverlay(*type, this->GetLayerSprite(PantsImageLayer), player, frame, screenPos, time_of_day, render_commands);
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingImageLayer), player, frame, screenPos, time_of_day, render_commands);
	
	//draw the backpack after the clothing if facing east or west, if isn't dying (dying animations for east and west use northeast frames)
	if ((this->Direction == LookingE || this->Direction == LookingW) && this->CurrentAction() != UnitAction::Die) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(BackpackImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}
	
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(HairImageLayer), player, frame, screenPos, time_of_day, render_commands);
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(HelmetImageLayer), player, frame, screenPos, time_of_day, render_commands);
	DrawPlayerColorOverlay(*type, this->GetLayerSprite(BootsImageLayer), player, frame, screenPos, time_of_day, render_commands);
	
	//draw the left arm just after the body if facing south
	if (
		(this->Direction == LookingS && this->CurrentAction() != UnitAction::Die)
		|| frame == type->StillFrame
		|| (
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		|| (
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(LeftArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingLeftArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(ShieldImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}

	//draw the right arm just after the body if not facing north
	if (
		(this->Direction != LookingN || this->CurrentAction() == UnitAction::Die)
		&& !(
			type->BoolFlag[INVERTEDEASTARMS_INDEX].value
			&& (this->Direction == LookingE || this->Direction == LookingW)
			&& this->CurrentAction() != UnitAction::Die
		)
		&& !(
			type->BoolFlag[INVERTEDSOUTHEASTARMS_INDEX].value
			&& (this->Direction == LookingSE || this->Direction == LookingSW || (this->Direction == LookingS && this->CurrentAction() == UnitAction::Die))
		)
	) {
		if ((this->Direction == LookingS || this->Direction == LookingSE || this->Direction == LookingSW) && this->CurrentAction() != UnitAction::Die && this->GetLayerSprite(RightHandImageLayer) != nullptr) { // if the unit has a right hand sprite, draw the weapon after the right arm, but before the hand
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingRightArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(WeaponImageLayer), player, frame, screenPos, time_of_day, render_commands);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightHandImageLayer), player, frame, screenPos, time_of_day, render_commands);
		} else {
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(WeaponImageLayer), player, frame, screenPos, time_of_day, render_commands);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(RightHandImageLayer), player, frame, screenPos, time_of_day, render_commands);
			DrawPlayerColorOverlay(*type, this->GetLayerSprite(ClothingRightArmImageLayer), player, frame, screenPos, time_of_day, render_commands);
		}
	}

	//draw the backpack after everything if facing north, northeast or northwest, or if facing east or west and is dying (dying animations for east and west use northeast frames)
	if (
		this->Direction == LookingN
		|| this->Direction == LookingNE
		|| this->Direction == LookingNW
		|| (
			(this->Direction == LookingE || this->Direction == LookingW) && this->CurrentAction() == UnitAction::Die
		)
	) {
		DrawPlayerColorOverlay(*type, this->GetLayerSprite(BackpackImageLayer), player, frame, screenPos, time_of_day, render_commands);
	}

	if (variation && variation->LightSprite) {
		DrawOverlay(*type, variation->LightSprite, player, frame, screenPos, time_of_day, render_commands);
	} else if (type->LightSprite) {
		DrawOverlay(*type, type->LightSprite, player, frame, screenPos, time_of_day, render_commands);
	}
	//Wyrmgus end
	
	// Unit's extras not fully supported.. need to be decorations themselves.
	DrawInformations(*this, *type, screenPos, render_commands);
}

/**
**  Compare what order 2 units should be drawn on the map
**
**  @param c1  First Unit to compare (*Unit)
**  @param c2  Second Unit to compare (*Unit)
**
*/
static inline bool DrawLevelCompare(const CUnit *c1, const CUnit *c2)
{
	int drawlevel1 = c1->GetDrawLevel();
	int drawlevel2 = c2->GetDrawLevel();

	if (drawlevel1 == drawlevel2) {
		// diffpos compares unit's Y positions (bottom of sprite) on the map
		// and uses X position in case Y positions are equal.
		const int pos1 = (c1->tilePos.y + c1->Type->get_tile_height() - 1) * wyrmgus::defines::get()->get_tile_height() + c1->get_pixel_offset().y();
		const int pos2 = (c2->tilePos.y + c2->Type->get_tile_height() - 1) * wyrmgus::defines::get()->get_tile_height() + c2->get_pixel_offset().y();
		return pos1 == pos2 ?
			   (c1->tilePos.x != c2->tilePos.x ? c1->tilePos.x < c2->tilePos.x : UnitNumber(*c1) < UnitNumber(*c2)) : pos1 < pos2;
	} else {
		return drawlevel1 < drawlevel2;
	}
}

/**
**  Find all units to draw in viewport.
**
**  @param vp     Viewport to be drawn.
**  @param table  Table of units to return in sorted order
**
*/
int FindAndSortUnits(const CViewport &vp, std::vector<CUnit *> &table)
{
	//  Select all units touching the viewpoint.
	const Vec2i offset(1, 1);
	const Vec2i vpSize(vp.MapWidth, vp.MapHeight);
	const Vec2i minPos = vp.MapPos - offset;
	const Vec2i maxPos = vp.MapPos + vpSize + offset;

	//Wyrmgus start
//	Select(minPos, maxPos, table);
	Select(minPos, maxPos, table, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	size_t n = table.size();
	for (size_t i = 0; i < table.size(); ++i) {
		if (!table[i]->IsVisibleInViewport(vp)) {
			table[i--] = table[--n];
			table.pop_back();
		}
	}
	assert_throw(n == table.size());
	std::sort(table.begin(), table.begin() + n, DrawLevelCompare);
	return n;
}
