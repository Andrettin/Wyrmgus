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
//      (c) Copyright 1999-2025 by Lutz Sammer, Andreas Arens,
//                                 Jimmy Salmon, Pali Rohár and Andrettin
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

#include "database/database.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "engine_interface.h"
//Wyrmgus start
#include "game/game.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "menus.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "sound/sound.h"
#include "title.h"
#include "translator.h"
#include "ui/contenttype.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/popup.h"
#include "unit/unit.h"
#include "util/assert_util.h"
#include "video/font.h"
#include "video/video.h"

#include <SDL.h>

static void SetViewportModeSingle();

bool RightButtonAttacks;                   /// right button attacks

/**
**  The user interface configuration
*/
CUserInterface &UI = *CUserInterface::get();

/**
**  Show load progress.
**
**  @param fmt  printf format string.
*/
void ShowLoadProgress(const char *fmt, ...)
{
	if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
		return;
	}

	static unsigned int lastProgressUpdate = SDL_GetTicks();
	const uint32_t ticks = SDL_GetTicks();
	if (ticks < lastProgressUpdate + 16) {
		//only show progress updates every c. 1/60th of a second, otherwise we're waiting for the screen too much
		return;
	}
	lastProgressUpdate = ticks;

	CheckMusicFinished(); //update music
	
	va_list va;
	char temp[4096];

	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp) - 1, fmt, va);
	temp[sizeof(temp) - 1] = '\0';
	va_end(va);

	//remove non printable chars
	for (unsigned char *s = (unsigned char *) temp; *s; ++s) {
		if (*s < 32) {
			*s = ' ';
		}
	}

	engine_interface::get()->set_loading_message(temp);
}

CUnitInfoPanel::~CUnitInfoPanel()
{
}

CUserInterface::CUserInterface()
{
	this->minimap = std::make_unique<wyrmgus::minimap>();
	MouseWarpPos.x = MouseWarpPos.y = -1;
}

/**
**  Get popup class pointer by string identifier.
**
**  @param ident  Popup identifier.
**
**  @return       popup class pointer.
*/
CPopup *PopupByIdent(const std::string &ident)
{
	for (const std::unique_ptr<CPopup> &popup : UI.ButtonPopups) {
		if (popup->Ident == ident) {
			return popup.get();
		}
	}
	return nullptr;
}

/**
**  Initialize the user interface.
*/
void InitUserInterface()
{
	if (!GameRunning) {
		ShowLoadProgress("%s", _("Loading User Interface..."));
	}
	
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	const int scaled_map_area_top_margin = (defines::get()->get_map_area_top_margin() * scale_factor).to_int();
	const int scaled_map_area_bottom_margin = (defines::get()->get_map_area_bottom_margin() * scale_factor).to_int();
	QRect map_area_rect(0, scaled_map_area_top_margin, Video.Width, Video.Height - scaled_map_area_top_margin - scaled_map_area_bottom_margin);

	//
	// Calculations
	//
	if (CMap::get()->Info->get_map_width()) {
		const QSize map_pixel_size = CMap::get()->Info->get_map_size() * defines::get()->get_scaled_tile_size();

		if (map_pixel_size.width() < map_area_rect.width()) {
			map_area_rect.setWidth(map_pixel_size.width());
		}

		if (map_pixel_size.height() < map_area_rect.height()) {
			map_area_rect.setHeight(map_pixel_size.height());
		}
	}

	UI.MapArea = CMapArea(map_area_rect);

	UI.SelectedViewport = UI.Viewports;

	SetViewportModeSingle();

	UI.CompletedBarColor = CVideo::MapRGB(UI.CompletedBarColorRGB);
	UI.ViewportCursorColor = ColorWhite;

	engine_interface::get()->update_current_time_of_day();
	engine_interface::get()->update_current_season();
	engine_interface::get()->update_map_view_top_left_pixel_pos();
}

/**
**  Load the user interface graphics.
*/
void CUserInterface::Load()
{
	// set the correct UI
	this->Fillers.clear();

	const wyrmgus::civilization *civilization = nullptr;
	const wyrmgus::faction *faction = nullptr;

	if (CPlayer::GetThisPlayer() != nullptr) {
		if (CPlayer::GetThisPlayer()->get_faction() != nullptr) {
			faction = CPlayer::GetThisPlayer()->get_faction();
		}
		
		if (CPlayer::GetThisPlayer()->Race != -1) {
			civilization = civilization::get_all()[CPlayer::GetThisPlayer()->Race];
		}
	}
	
	if (faction != nullptr) {
		this->Fillers = faction->get_ui_fillers();
	} else if (civilization != nullptr) {
		this->Fillers = civilization->get_ui_fillers();
	}

	//  Load graphics
	for (size_t i = 0; i < this->Fillers.size(); ++i) {
		this->Fillers.at(i).Load();
	}

	if (InfoPanel.G) {
		InfoPanel.G->Load(preferences::get()->get_scale_factor());
	}
	if (ButtonPanel.G) {
		ButtonPanel.G->Load(preferences::get()->get_scale_factor());
	}

	//  Resolve cursors
	for (int i = 0; i < static_cast<int>(cursor_type::count); ++i) {
		auto cursor_type = static_cast<wyrmgus::cursor_type>(i);
		cursor *cursor = nullptr;

		if (civilization != nullptr) {
			cursor = civilization->get_cursor(cursor_type);
		} else {
			cursor = cursor::get_cursor_by_type(cursor_type);
		}

		if (cursor != nullptr) {
			this->cursors[cursor_type] = cursor;
		}
	}

	if (cursor::get_current_cursor() == nullptr && database::get()->is_initialized()) {
		//set the initial cursor
		//the database needs to have been initialized so that the cursor definitions are present
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
	}
}


bool CMapArea::contains(const QPoint &screen_pos) const
{
	return this->rect.contains(screen_pos);
}

/**
**  Save the viewports.
**
**  @param file  Save file handle
**  @param ui    User interface to save
*/
static void SaveViewports(CFile &file, const CUserInterface &ui)
{
	// FIXME: don't save the number
	file.printf("DefineViewports(");
	for (int i = 0; i < ui.NumViewports; ++i) {
		const CViewport &vp = ui.Viewports[i];
		if (i > 0) {
			file.printf(",");
		}
		file.printf("\n  \"viewport\", {%d, %d, %d}", vp.MapPos.x, vp.MapPos.y,
					vp.Unit ? UnitNumber(*vp.Unit) : -1);
	}
	file.printf(")\n\n");
}

/**
**  Save the user interface module.
**
**  @param file  Save file handle
*/
void SaveUserInterface(CFile &file)
{
	SaveViewports(file, UI);
}

/**
**  Clean up a user interface.
*/
CUserInterface::~CUserInterface()
{
}

/**
**  Clean up the user interface module.
*/
void CleanUserInterface()
{
	// Filler
	UI.Fillers.clear();

	// Info Panel
	UI.InfoPanel.G.reset();
	UI.InfoPanelContents.clear();
	
	// Button Popups
	UI.ButtonPopups.clear();

	delete UI.SingleSelectedButton;
	UI.SelectedButtons.clear();
	delete UI.SingleTrainingButton;
	UI.SingleTrainingText.clear();
	UI.TrainingButtons.clear();
	UI.TrainingText.clear();
	delete UI.UpgradingButton;
	delete UI.ResearchingButton;
	UI.TransportingButtons.clear();
	//Wyrmgus start
	delete UI.IdleWorkerButton;
	delete UI.LevelUpUnitButton;
	UI.HeroUnitButtons.clear();
	UI.InventoryButtons.clear();
	//Wyrmgus end
	UI.UserButtons.clear();

	// Button Panel
	UI.ButtonPanel.G.reset();

	// Backgrounds
	UI.VictoryBackgroundG.reset();
	UI.DefeatBackgroundG.reset();

	// Title Screens
	TitleScreens.clear();
}

void FreeButtonStyles()
{
	std::map<std::string, ButtonStyle *>::iterator i;
	for (i = ButtonStyleHash.begin(); i != ButtonStyleHash.end(); ++i) {
		delete(*i).second;
	}
	ButtonStyleHash.clear();
}

/**
**  Takes coordinates of a pixel in stratagus's window and computes
**  the map viewport which contains this pixel.
**
**  @param screenPos  pixel coordinate with origin at UL corner of screen
**
**  @return viewport pointer or null if this pixel is not inside
**  any of the viewports.
**
**  @note This functions only works with rectangular viewports, when
**  we support shaped map window, this must be rewritten.
*/
CViewport *GetViewport(const PixelPos &screenPos)
{
	for (CViewport *vp = UI.Viewports; vp < UI.Viewports + UI.NumViewports; ++vp) {
		if (vp->contains(screenPos)) {
			return vp;
		}
	}
	return nullptr;
}

/**
**  Takes an array of new Viewports which are supposed to have their
**  pixel geometry (CViewport::[XY] and CViewport::End[XY]) already
**  computed. Using this information as well as old viewport's
**  parameters fills in new viewports' CViewport::Map* parameters.
**  Then it replaces the old viewports with the new ones and finishes
**  the set-up of the new mode.
**
**  @param new_vps  The array of the new viewports
**  @param num_vps  The number of elements in the new_vps[] array.
*/
static void FinishViewportModeConfiguration(CViewport new_vps[], int num_vps)
{
	//  Compute location of the viewport using oldviewport
	for (int i = 0; i < num_vps; ++i) {
		new_vps[i].MapPos.x = 0;
		new_vps[i].MapPos.y = 0;
		const CViewport *vp = GetViewport(new_vps[i].get_top_left_pos());
		if (vp) {
			const PixelDiff relDiff = new_vps[i].get_top_left_pos() - vp->get_top_left_pos();

			new_vps[i].Offset = relDiff + CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(vp->MapPos) + vp->Offset;
		} else {
			new_vps[i].Offset.x = 0;
			new_vps[i].Offset.y = 0;
		}
	}

	// Affect the old viewport.
	for (int i = 0; i < num_vps; ++i) {
		CViewport &vp = UI.Viewports[i];

		vp.set_rect(new_vps[i].get_rect());
		vp.Set(new_vps[i].MapPos, new_vps[i].Offset);
	}

	UI.NumViewports = num_vps;

	//
	//  Update the viewport pointers
	//
	UI.MouseViewport = GetViewport(CursorScreenPos);
	UI.SelectedViewport = std::min(UI.Viewports + UI.NumViewports - 1, UI.SelectedViewport);
}

/**
**  Takes a viewport which is supposed to have its CViewport::[XY]
**  correctly filled-in and computes CViewport::End[XY] attributes
**  according to clipping information passed in other two arguments.
**
**  @param vp     The viewport.
**  @param ClipX  Maximum x-coordinate of the viewport's right side
**                as dictated by current UI's geometry and ViewportMode.
**  @param ClipY  Maximum y-coordinate of the viewport's bottom side
**                as dictated by current UI's geometry and ViewportMode.
**
**  @note It is supposed that values passed in Clip[XY] will
**  never be greater than UI::MapArea::End[XY].
**  However, they can be smaller according to the place
**  the viewport vp takes in context of current ViewportMode.
*/
static void ClipViewport(CViewport &vp, int ClipX, int ClipY)
{
	// begin with maximum possible viewport size
	//Wyrmgus start
//	vp.BottomRightPos.x = vp.TopLeftPos.x + Map.Info.MapWidth * defines::get()->get_scaled_tile_width() - 1;
//	vp.BottomRightPos.y = vp.TopLeftPos.y + Map.Info.MapHeight * defines::get()->get_scaled_tile_height() - 1;

	const QSize scaled_tile_size = defines::get()->get_scaled_tile_size();

	const QSize map_size = (CMap::get()->Info->MapWidths.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->get_size() : CMap::get()->Info->get_map_size());

	int right = vp.get_top_left_pos().x();
	if (map_size.width() != 0) {
		right += map_size.width() * scaled_tile_size.width() - 1;
	}

	int bottom = vp.get_top_left_pos().y();
	if (map_size.height() != 0) {
		bottom += map_size.height() * scaled_tile_size.height() - 1;
	}
	//Wyrmgus end

	// first clip it to MapArea size if necessary
	right = std::min<int>(right, ClipX);
	bottom = std::min<int>(bottom, ClipY);

	assert_throw(right <= UI.MapArea.get_rect().right());
	assert_throw(right >= vp.get_top_left_pos().x());
	assert_throw(bottom <= UI.MapArea.get_rect().bottom());
	assert_throw(bottom >= vp.get_top_left_pos().y());

	vp.set_bottom_right_pos(QPoint(right, bottom));
}

/**
**  Compute viewport parameters for single viewport mode.
**
**  The parameters include viewport's width and height expressed
**  in pixels, its position with respect to Stratagus's window
**  origin, and the corresponding map parameters expressed in map
**  tiles with origin at map origin (map tile (0,0)).
*/
static void SetViewportModeSingle()
{
	CViewport new_vps[MAX_NUM_VIEWPORTS];

	DebugPrint("Single viewport set\n");

	new_vps[0].set_top_left_pos(UI.MapArea.get_rect().topLeft());
	ClipViewport(new_vps[0], UI.MapArea.get_rect().right(), UI.MapArea.get_rect().bottom());

	FinishViewportModeConfiguration(new_vps, 1);
}

/**
**  Check if mouse scrolling is enabled
*/
bool GetMouseScroll()
{
	return UI.MouseScroll;
}

/**
**  Enable/disable scrolling with the mouse
**
**  @param enabled  True to enable mouse scrolling, false to disable
*/
void SetMouseScroll(bool enabled)
{
	UI.MouseScroll = enabled;
}

/**
**  Check if keyboard scrolling is enabled
*/
bool GetKeyScroll()
{
	return UI.KeyScroll;
}

/**
**  Enable/disable scrolling with the keyboard
**
**  @param enabled  True to enable keyboard scrolling, false to disable
*/
void SetKeyScroll(bool enabled)
{
	UI.KeyScroll = enabled;
}

/**
**  Check if scrolling stops when leaving the window
*/
bool GetLeaveStops()
{
	return LeaveStops;
}

/**
**  Enable/disable leaving the window stops scrolling
**
**  @param enabled  True to stop scrolling, false to disable
*/
void SetLeaveStops(bool enabled)
{
	LeaveStops = enabled;
}
