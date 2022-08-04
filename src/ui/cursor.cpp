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
//      (c) Copyright 1998-2022 by Lutz Sammer, Nehal Mistry,
//                                 Jimmy Salmon and Andrettin
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

#include "ui/cursor.h"
#include "video/intern_video.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/player.h"
#include "translator.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "video/renderer.h"
#include "video/video.h"

#pragma warning(push, 0)
#include <QPixmap>
#pragma warning(pop)

CursorState CurrentCursorState;    /// current cursor state (point,...)
ButtonCmd CursorAction;            /// action for selection
int CursorValue;             /// value for CursorAction (spell type f.e.)
std::string CustomCursor;    /// custom cursor for button

// Event changed mouse position, can alter at any moment
PixelPos CursorScreenPos;    /// cursor position on screen
PixelPos CursorStartScreenPos;  /// rectangle started on screen
PixelPos CursorStartMapPos;/// position of starting point of selection rectangle, in Map pixels.


/*--- DRAW BUILDING  CURSOR ------------------------------------------------*/
const wyrmgus::unit_type *CursorBuilding;           /// building cursor

namespace wyrmgus {

void cursor::clear()
{
	data_type::clear();

	CursorBuilding = nullptr;
	cursor::current_cursor = nullptr;
	UnitUnderCursor = nullptr;
}

void cursor::set_current_cursor(cursor *cursor, const bool notify)
{
	if (cursor == cursor::current_cursor) {
		return;
	}

	cursor::current_cursor = cursor;

	if (notify) {
		cursor::on_current_cursor_changed();
	} else {
		cursor::current_cursor_changed = true;
	}
}

void cursor::check_current_cursor_changed()
{
	if (cursor::current_cursor_changed) {
		on_current_cursor_changed();
		cursor::current_cursor_changed = false;
	}
}

void cursor::on_current_cursor_changed()
{
	cursor *cursor = cursor::get_current_cursor();

	if (cursor != nullptr) {
		if (!cursor->get_graphics()->IsLoaded()) {
			cursor->get_graphics()->Load(preferences::get()->get_scale_factor());
		}

		const QPixmap pixmap = QPixmap::fromImage(cursor->get_graphics()->get_or_create_frame_image(0, color_modification(), false));
		const QPoint hot_pos = cursor->get_hot_pos() * preferences::get()->get_scale_factor();
		const QCursor qcursor(pixmap, hot_pos.x(), hot_pos.y());

		QMetaObject::invokeMethod(QApplication::instance(), [qcursor] {
			if (QApplication::overrideCursor() != nullptr) {
				QApplication::changeOverrideCursor(qcursor);
			} else {
				QApplication::setOverrideCursor(qcursor);
			}
		}, Qt::QueuedConnection);
	}
}

cursor::cursor(const std::string &identifier) : data_entry(identifier), type(cursor_type::point)
{
}


cursor::~cursor()
{
}

void cursor::initialize()
{
	this->graphics = CGraphic::New(this->get_file().string(), this->get_frame_size());

	if (this->civilization != nullptr) {
		this->civilization->set_cursor(this->get_type(), this);
	} else if (this->civilization_group != nullptr) {
		this->civilization_group->set_cursor(this->get_type(), this);
	} else {
		cursor::map_cursor(this->get_type(), this);
	}

	data_entry::initialize();
}

void cursor::set_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_file()) {
		return;
	}

	this->file = database::get()->get_graphics_filepath(filepath);
}

}

/**
**  Get the amount of cursors sprites to load
*/
int GetCursorsCount()
{
	int count = 0;
	for (const cursor *cursor : cursor::get_all()) {
		if (cursor->get_graphics() != nullptr && !cursor->get_graphics()->IsLoaded()) {
			count++;
		}
	}

	return count;
}

/**
**  Draw rectangle cursor when visible
**
**  @param corner1   Screen start position of rectangle
**  @param corner2   Screen end position of rectangle
*/
static void DrawVisibleRectangleCursor(PixelPos corner1, PixelPos corner2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const CViewport &vp = *UI.SelectedViewport;

	//  Clip to map window.
	//  FIXME: should re-use CLIP_RECTANGLE in some way from linedraw.c ?
	vp.Restrict(corner2.x, corner2.y);

	if (corner1.x > corner2.x) {
		std::swap(corner1.x, corner2.x);
	}
	if (corner1.y > corner2.y) {
		std::swap(corner1.y, corner2.y);
	}
	const int w = corner2.x - corner1.x + 1;
	const int h = corner2.y - corner1.y + 1;

	Video.DrawRectangleClip(ColorGreen, corner1.x, corner1.y, w, h, render_commands);
}

/**
**  Draw cursor for selecting building position.
*/
//Wyrmgus start
//static void DrawBuildingCursor()
void DrawBuildingCursor(std::vector<std::function<void(renderer *)>> &render_commands)
//Wyrmgus end
{
	// Align to grid
	const CViewport &vp = *UI.MouseViewport;
	const QPoint top_left_tile_pos = vp.ScreenToTilePos(CursorScreenPos);
	const PixelPos screenPos = vp.TilePosToScreen_TopLeft(top_left_tile_pos);
	const int z = UI.CurrentMapLayer->ID;

	CUnit *ontop = nullptr;

	//
	//  Draw building
	//
#ifdef DYNAMIC_LOAD
	if (!CursorBuilding->G->IsLoaded()) {
		LoadUnitTypeSprite(CursorBuilding);
	}
#endif
	PushClipping();
	vp.SetClipping();

	QPoint center_tile_pos = top_left_tile_pos + CursorBuilding->get_tile_center_pos_offset();
	CMap::get()->clamp(center_tile_pos, UI.CurrentMapLayer->ID);

	const wyrmgus::time_of_day *time_of_day = UI.CurrentMapLayer->get_tile_time_of_day(center_tile_pos);

//	DrawShadow(*CursorBuilding, CursorBuilding->StillFrame, screenPos);
	if (CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer()) && CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->ShadowSprite) {
		DrawShadow(*CursorBuilding, CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->ShadowSprite, CursorBuilding->StillFrame, screenPos, render_commands);
	} else if (CursorBuilding->ShadowSprite) {
		DrawShadow(*CursorBuilding, CursorBuilding->ShadowSprite, CursorBuilding->StillFrame, screenPos, render_commands);
	}
	//Wyrmgus end
	//Wyrmgus start
	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), BackpackImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), MountImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

//	DrawUnitType(*CursorBuilding, CursorBuilding->Sprite, CPlayer::GetThisPlayer()->Index,
//				 CursorBuilding->StillFrame, screenPos, render_commands);
	// get the first variation which has the proper upgrades for this player (to have the proper appearance of buildings drawn in the cursor, according to the upgrades)
	if (CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer()) && CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->Sprite) {
		DrawUnitType(*CursorBuilding, CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->Sprite, CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	} else {
		DrawUnitType(*CursorBuilding, CursorBuilding->Sprite, CPlayer::GetThisPlayer()->get_index(),
				CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), HairImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), PantsImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), HelmetImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), BootsImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), LeftArmImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingLeftArmImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ShieldImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	
	if (CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightHandImageLayer) != nullptr) {
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightArmImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingRightArmImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), WeaponImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
		
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightHandImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	} else {
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), WeaponImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightArmImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingRightArmImageLayer), CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	}

	if (CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer()) && CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->LightSprite) {
		DrawOverlay(*CursorBuilding, CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->LightSprite, CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	} else if (CursorBuilding->LightSprite) {
		DrawOverlay(*CursorBuilding, CursorBuilding->LightSprite, CPlayer::GetThisPlayer()->get_index(), CursorBuilding->StillFrame, screenPos, time_of_day, render_commands);
	}
	//Wyrmgus end
	
	if (CursorBuilding->CanAttack && CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Value > 0) {
		const PixelPos center(screenPos + CursorBuilding->get_scaled_half_tile_pixel_size());
		const int radius = (CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Max + (CursorBuilding->get_tile_width() - 1)) * defines::get()->get_scaled_tile_width() + 1;

		render_commands.push_back([center, radius](renderer *renderer) {
			renderer->draw_circle(center, radius, CVideo::GetRGBA(ColorRed));
		});
	}

	//
	//  Draw the allow overlay
	//
	int f;
	if (!Selected.empty()) {
		f = 1;
		for (size_t i = 0; f && i < Selected.size(); ++i) {
			f = ((ontop = CanBuildHere(Selected[i], *CursorBuilding, top_left_tile_pos, z)) != nullptr);
			// Assign ontop or null
			ontop = (ontop == Selected[i] ? nullptr : ontop);
		}
	} else {
		f = ((ontop = CanBuildHere(NoUnitP, *CursorBuilding, top_left_tile_pos, z)) != nullptr);
		if (!CEditor::get()->is_running() || ontop == (CUnit *)1) {
			ontop = nullptr;
		}
	}

	const tile_flag mask = CursorBuilding->MovementMask;
	int h = CursorBuilding->get_tile_height();
	// reduce to view limits
	h = std::min(h, vp.MapPos.y + vp.MapHeight - top_left_tile_pos.y());
	int w0 = CursorBuilding->get_tile_width();
	w0 = std::min(w0, vp.MapPos.x + vp.MapWidth - top_left_tile_pos.x());

	while (h--) {
		int w = w0;
		while (w--) {
			const Vec2i posIt(top_left_tile_pos.x() + w, top_left_tile_pos.y() + h);
			uint32_t color;

			const tile *tile = UI.CurrentMapLayer->Field(posIt);

			if (f && (ontop ||
					  CanBuildOn(posIt, MapFogFilterFlags(*CPlayer::GetThisPlayer(), posIt,
														  mask & ((!Selected.empty() && Selected[0]->tilePos == posIt) ?
																  ~(tile_flag::land_unit | tile_flag::sea_unit) : static_cast<tile_flag>(-1)), z), z, CPlayer::GetThisPlayer(), CursorBuilding))
				&& tile->player_info->IsTeamExplored(*CPlayer::GetThisPlayer())
				&& (tile->get_owner() == nullptr || tile->get_owner() == CPlayer::GetThisPlayer())
			) {
				color = ColorGreen;
			} else {
				color = ColorRed;
			}

			Video.FillTransRectangleClip(color, screenPos.x + w * defines::get()->get_scaled_tile_width(),
										 screenPos.y + h * defines::get()->get_scaled_tile_height(), defines::get()->get_scaled_tile_width(), defines::get()->get_scaled_tile_height(), 95, render_commands);
		}
	}
	PopClipping();
}

/**
**  Draw the cursor.
*/
void DrawCursor(std::vector<std::function<void(renderer *)>> &render_commands)
{
	// Selecting rectangle
	if (CurrentCursorState == CursorState::Rectangle && CursorStartScreenPos != CursorScreenPos) {
		const PixelPos cursorStartScreenPos = UI.MouseViewport->scaled_map_to_screen_pixel_pos(CursorStartMapPos);

		DrawVisibleRectangleCursor(cursorStartScreenPos, CursorScreenPos, render_commands);
	}
}

/**
**  Animate the cursor.
**
**  @param ticks  Current tick
*/
void CursorAnimate(unsigned ticks)
{
	static unsigned last = 0;

	if (cursor::get_current_cursor() == nullptr || !cursor::get_current_cursor()->get_frame_rate()) {
		return;
	}

	if (ticks > last + cursor::get_current_cursor()->get_frame_rate()) {
		last = ticks + cursor::get_current_cursor()->get_frame_rate();
		cursor::get_current_cursor()->increment_current_frame();
		if ((cursor::get_current_cursor()->get_frame_rate() & 127) >= cursor::get_current_cursor()->get_graphics()->NumFrames) {
			cursor::get_current_cursor()->reset_current_frame();
		}
	}
}
