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
/**@name graphic.cpp - The general graphic functions. */
//
//      (c) Copyright 1999-2020 by Lutz Sammer, Nehal Mistry, Jimmy Salmon,
//                                 Pali Rohár and Andrettin
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

#include "database/defines.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "intern_video.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map_layer.h"
#include "player.h"
//Wyrmgus start
#include "results.h"
//Wyrmgus end
#include "time/time_of_day.h"
#include "ui/ui.h"
//Wyrmgus start
#include "unit/unit.h" //for using CPreference
//Wyrmgus end
#include "util/image_util.h"
#include "video.h"
#include "xbrz.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

static int HashCount;
static std::map<std::string, CGraphic *> GraphicHash;
static std::list<CGraphic *> Graphics;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Video draw the graphic clipped.
**
**  @param x   X screen position
**  @param y   Y screen position
*/
void CGraphic::DrawClip(int x, int y) const
{
	int oldx = x;
	int oldy = y;
	int w = Width;
	int h = Height;
	CLIP_RECTANGLE(x, y, w, h);
	DrawSub(x - oldx, y - oldy, w, h, x, y);
}

/**
**  Video draw part of graphic.
**
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
//Wyrmgus start
//void CGraphic::DrawSub(int gx, int gy, int w, int h, int x, int y) const
void CGraphic::DrawSub(int gx, int gy, int w, int h, int x, int y, SDL_Surface *surface) const
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	DrawTexture(this, Textures, gx, gy, gx + w, gy + h, x, y, 0);
#endif
}

//Wyrmgus start
/**
**  Video draw part of a player color graphic.
**
**  @param player  player number
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
void CPlayerColorGraphic::DrawPlayerColorSub(int player, int gx, int gy, int w, int h, int x, int y)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!PlayerColorTextures[player]) {
		MakePlayerColorTexture(this, player, nullptr);
	}
	DrawTexture(this, PlayerColorTextures[player], gx, gy, gx + w, gy + h, x, y, 0);
#endif
}
//Wyrmgus end

/**
**  Video draw part of graphic clipped.
**
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
//Wyrmgus start
//void CGraphic::DrawSubClip(int gx, int gy, int w, int h, int x, int y) const
void CGraphic::DrawSubClip(int gx, int gy, int w, int h, int x, int y, SDL_Surface *surface) const
//Wyrmgus end
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	//Wyrmgus start
//	DrawSub(gx + x - oldx, gy + y - oldy, w, h, x, y);
	DrawSub(gx + x - oldx, gy + y - oldy, w, h, x, y, surface);
	//Wyrmgus end
}

//Wyrmgus start
/**
**  Video draw part of graphic clipped.
**
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
void CPlayerColorGraphic::DrawPlayerColorSubClip(int player, int gx, int gy, int w, int h, int x, int y)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawPlayerColorSub(player, gx + x - oldx, gy + y - oldy, w, h, x, y);
}
//Wyrmgus end

/**
**  Video draw part of graphic with alpha.
**
**  @param gx     X offset into object
**  @param gy     Y offset into object
**  @param w      width to display
**  @param h      height to display
**  @param x      X screen position
**  @param y      Y screen position
**  @param alpha  Alpha
*/
void CGraphic::DrawSubTrans(int gx, int gy, int w, int h, int x, int y,
							//Wyrmgus start
//							unsigned char alpha) const
							unsigned char alpha, SDL_Surface *surface) const
							//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	DrawSub(gx, gy, w, h, x, y);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

/**
**  Video draw part of graphic with alpha and clipped.
**
**  @param gx     X offset into object
**  @param gy     Y offset into object
**  @param w      width to display
**  @param h      height to display
**  @param x      X screen position
**  @param y      Y screen position
**  @param alpha  Alpha
*/
void CGraphic::DrawSubClipTrans(int gx, int gy, int w, int h, int x, int y,
								//Wyrmgus start
//								unsigned char alpha) const
								unsigned char alpha, SDL_Surface *surface) const
								//Wyrmgus end
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	//Wyrmgus start
//	DrawSubTrans(gx + x - oldx, gy + y - oldy, w, h, x, y, alpha);
	DrawSubTrans(gx + x - oldx, gy + y - oldy, w, h, x, y, alpha, surface);
	//Wyrmgus end
}

/**
**  Draw graphic object unclipped.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrame(unsigned frame, int x, int y) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		DrawTexture(this, Textures, frame_map[frame].x, frame_map[frame].y,
					frame_map[frame].x +  Width, frame_map[frame].y + Height, x, y, 0);
	} else
#endif
	{
		DrawSub(frame_map[frame].x, frame_map[frame].y,
				Width, Height, x, y);
	}
}

#if defined(USE_OPENGL) || defined(USE_GLES)
void CGraphic::DoDrawFrameClip(GLuint *textures,
							   unsigned frame, int x, int y, int show_percent) const
{
	int ox;
	int oy;
	int skip;
	int w = Width;
	int h = Height;

	CLIP_RECTANGLE_OFS(x, y, w, h, ox, oy, skip);
	UNUSED(skip);
	DrawTexture(this, textures, frame_map[frame].x + ox,
				frame_map[frame].y + oy,
				frame_map[frame].x + ox + (w),
				frame_map[frame].y + oy + (h * show_percent / 100), x, y, 0);
}
#endif

/**
**  Draw graphic object clipped.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
//Wyrmgus start
//void CGraphic::DrawFrameClip(unsigned frame, int x, int y) const
void CGraphic::DrawFrameClip(unsigned frame, int x, int y, const stratagus::time_of_day *time_of_day, SDL_Surface *surface, int show_percent)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	//Wyrmgus start
//	DoDrawFrameClip(Textures, frame, x, y);
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClip(Textures, frame, x, y, show_percent);
	} else {
		if (TextureColorModifications.find(time_of_day->ColorModification) == TextureColorModifications.end()) {
			MakeTexture(this, time_of_day);
		}
		DoDrawFrameClip(TextureColorModifications[time_of_day->ColorModification], frame, x, y, show_percent);
	}
	//Wyrmgus end
#endif
}

void CGraphic::DrawFrameTrans(unsigned frame, int x, int y, int alpha) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		DrawFrame(frame, x, y);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		DrawSubTrans(frame_map[frame].x, frame_map[frame].y,
					 Width, Height, x, y, alpha);
	}
}

//Wyrmgus start
//void CGraphic::DrawFrameClipTrans(unsigned frame, int x, int y, int alpha) const
void CGraphic::DrawFrameClipTrans(unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, SDL_Surface *surface, int show_percent)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	//Wyrmgus start
//	DrawFrameClip(frame, x, y);
	DrawFrameClip(frame, x, y, time_of_day, surface, show_percent);
	//Wyrmgus end
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

/**
**  Draw graphic object clipped and with player colors.
**
**  @param player  player number
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CPlayerColorGraphic::DrawPlayerColorFrameClip(int player, unsigned frame,
//Wyrmgus start
//												   int x, int y)
												   int x, int y, const stratagus::time_of_day *time_of_day, int show_percent)
//Wyrmgus end
{
	//Wyrmgus start
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == CPlayer::Players[player]->Color) {
			player = i;
			break;
		}
	}
	
	//Wyrmgus end
#if defined(USE_OPENGL) || defined(USE_GLES)
	//Wyrmgus start
	/*
	if (!PlayerColorTextures[player]) {
		MakePlayerColorTexture(this, player);
	}
	DoDrawFrameClip(PlayerColorTextures[player], frame, x, y);
	*/
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (!PlayerColorTextures[player]) {
			MakePlayerColorTexture(this, player, nullptr);
		}
		DoDrawFrameClip(PlayerColorTextures[player], frame, x, y, show_percent);
	} else {
		if (PlayerColorTextureColorModifications.find(player) == PlayerColorTextureColorModifications.end() || PlayerColorTextureColorModifications[player].find(time_of_day->ColorModification) == PlayerColorTextureColorModifications[player].end()) {
			MakePlayerColorTexture(this, player, time_of_day);
		}
		DoDrawFrameClip(PlayerColorTextureColorModifications[player][time_of_day->ColorModification], frame, x, y, show_percent);
	}
	//Wyrmgus end
#endif
}

//Wyrmgus start
void CPlayerColorGraphic::DrawPlayerColorFrameClipTrans(int player, unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, int show_percent)
{
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == CPlayer::Players[player]->Color) {
			player = i;
			break;
		}		
	}
#if defined(USE_OPENGL) || defined(USE_GLES)
	//Wyrmgus start
	/*
	if (!PlayerColorTextures[player]) {
		MakePlayerColorTexture(this, player);
	}
	*/
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (!PlayerColorTextures[player]) {
			MakePlayerColorTexture(this, player, nullptr);
		}
	} else {
		if (PlayerColorTextureColorModifications.find(player) == PlayerColorTextureColorModifications.end() || PlayerColorTextureColorModifications[player].find(time_of_day->ColorModification) == PlayerColorTextureColorModifications[player].end()) {
			MakePlayerColorTexture(this, player, time_of_day);
		}
	}
	//Wyrmgus end
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	//Wyrmgus start
//	DoDrawFrameClip(PlayerColorTextures[player], frame, x, y);
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClip(PlayerColorTextures[player], frame, x, y, show_percent);
	} else {
		DoDrawFrameClip(PlayerColorTextureColorModifications[player][time_of_day->ColorModification], frame, x, y, show_percent);
	}
	//Wyrmgus end
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

//Wyrmgus start
//void CPlayerColorGraphic::DrawPlayerColorFrameClipTransX(int player, unsigned frame, int x, int y, int alpha)
void CPlayerColorGraphic::DrawPlayerColorFrameClipTransX(int player, unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day)
//Wyrmgus end
{
	//Wyrmgus start
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == CPlayer::Players[player]->Color) {
			player = i;
			break;
		}		
	}
	//Wyrmgus end
#if defined(USE_OPENGL) || defined(USE_GLES)
	//Wyrmgus start
	/*
	if (!PlayerColorTextures[player]) {
		MakePlayerColorTexture(this, player);
	}
	*/
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (!PlayerColorTextures[player]) {
			MakePlayerColorTexture(this, player, nullptr);
		}
	} else {
		if (PlayerColorTextureColorModifications.find(player) == PlayerColorTextureColorModifications.end() || PlayerColorTextureColorModifications[player].find(time_of_day->ColorModification) == PlayerColorTextureColorModifications[player].end()) {
			MakePlayerColorTexture(this, player, time_of_day);
		}
	}
	//Wyrmgus end
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	//Wyrmgus start
//	DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
	} else {
		DoDrawFrameClipX(PlayerColorTextureColorModifications[player][time_of_day->ColorModification], frame, x, y);
	}
	//Wyrmgus end
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}
//Wyrmgus end

/**
**  Draw graphic object unclipped and flipped in X direction.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrameX(unsigned frame, int x, int y) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		DrawTexture(this, Textures, frame_map[frame].x, frame_map[frame].y,
					frame_map[frame].x +  Width, frame_map[frame].y + Height, x, y, 1);
	} else
#endif
	{
		SDL_Rect srect = {frameFlip_map[frame].x, frameFlip_map[frame].y, Uint16(Width), Uint16(Height)};
		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};

		SDL_BlitSurface(SurfaceFlip, &srect, TheScreen, &drect);
	}
}

#if defined(USE_OPENGL) || defined(USE_GLES)
void CGraphic::DoDrawFrameClipX(GLuint *textures, unsigned frame,
								int x, int y) const
{
	int ox;
	int oy;
	int skip;
	int w = Width;
	int h = Height;
	CLIP_RECTANGLE_OFS(x, y, w, h, ox, oy, skip);
	UNUSED(skip);

	if (w < Width) {
		if (ox == 0) {
			ox = Width - w;
		} else {
			ox = 0;
		}
	}

	DrawTexture(this, textures, frame_map[frame].x + ox,
				frame_map[frame].y + oy,
				frame_map[frame].x + ox + w,
				frame_map[frame].y + oy + h, x, y, 1);
}
#endif

/**
**  Draw graphic object clipped and flipped in X direction.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
//Wyrmgus start
//void CGraphic::DrawFrameClipX(unsigned frame, int x, int y) const
void CGraphic::DrawFrameClipX(unsigned frame, int x, int y, const stratagus::time_of_day *time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	//Wyrmgus start
	//DoDrawFrameClipX(Textures, frame, x, y);
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClipX(Textures, frame, x, y);
	} else {
		if (TextureColorModifications.find(time_of_day->ColorModification) == TextureColorModifications.end()) {
			MakeTexture(this, time_of_day);
		}
		DoDrawFrameClipX(TextureColorModifications[time_of_day->ColorModification], frame, x, y);
	}
	//Wyrmgus end
#endif
}

void CGraphic::DrawFrameTransX(unsigned frame, int x, int y, int alpha) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	DrawFrameX(frame, x, y);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

//Wyrmgus start
//void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha) const
void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	//Wyrmgus start
//	DrawFrameClipX(frame, x, y);
	DrawFrameClipX(frame, x, y, time_of_day);
	//Wyrmgus end
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

/**
**  Draw graphic object clipped, flipped, and with player colors.
**
**  @param player  player number
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CPlayerColorGraphic::DrawPlayerColorFrameClipX(int player, unsigned frame,
//Wyrmgus start
//												   int x, int y)
												   int x, int y, const stratagus::time_of_day *time_of_day)
//Wyrmgus end
{
	//Wyrmgus start
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == CPlayer::Players[player]->Color) {
			player = i;
			break;
		}		
	}
	//Wyrmgus end
#if defined(USE_OPENGL) || defined(USE_GLES)
	//Wyrmgus start
	/*
	if (!PlayerColorTextures[player]) {
		MakePlayerColorTexture(this, player);
	}
	DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
	*/
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (!PlayerColorTextures[player]) {
			MakePlayerColorTexture(this, player, nullptr);
		}
		DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
	} else {
		if (PlayerColorTextureColorModifications.find(player) == PlayerColorTextureColorModifications.end() || PlayerColorTextureColorModifications[player].find(time_of_day->ColorModification) == PlayerColorTextureColorModifications[player].end()) {
			MakePlayerColorTexture(this, player, time_of_day);
		}
		DoDrawFrameClipX(PlayerColorTextureColorModifications[player][time_of_day->ColorModification], frame, x, y);
	}
	//Wyrmgus end
#endif
}

/*----------------------------------------------------------------------------
--  Global functions
----------------------------------------------------------------------------*/

/**
**  Make a new graphic object.
**
**  @param filename  Filename
**  @param w     Width of a frame (optional)
**  @param h     Height of a frame (optional)
**
**  @return      New graphic object
*/
CGraphic *CGraphic::New(const std::string &filename, const int w, const int h)
{
	if (filename.empty()) {
		return new CGraphic();
	}

	const std::string file = LibraryFileName(filename.c_str());
	CGraphic *&g = GraphicHash[file];
	if (g == nullptr) {
		g = new CGraphic();
		if (!g) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
		// FIXME: use a constructor for this
		g->File = file;
		g->HashFile = g->File;
		g->Width = w;
		g->Height = h;
		g->original_frame_size = QSize(w, h);
	} else {
		++g->Refs;
		Assert((w == 0 || g->Width == w) && (g->Height == h || h == 0));
	}

	return g;
}

/**
**  Make a new player color graphic object.
**
**  @param filename  Filename
**  @param w     Width of a frame (optional)
**  @param h     Height of a frame (optional)
**
**  @return      New graphic object
*/
CPlayerColorGraphic *CPlayerColorGraphic::New(const std::string &filename, const int w, const int h)
{
	if (filename.empty()) {
		return new CPlayerColorGraphic();
	}

	const std::string file = LibraryFileName(filename.c_str());
	CPlayerColorGraphic *g = dynamic_cast<CPlayerColorGraphic *>(GraphicHash[file]);
	if (g == nullptr) {
		g = new CPlayerColorGraphic();
		if (!g) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
		// FIXME: use a constructor for this
		g->File = file;
		g->HashFile = g->File;
		g->Width = w;
		g->Height = h;
		g->original_frame_size = QSize(w, h);
		GraphicHash[g->HashFile] = g;
	} else {
		++g->Refs;
		Assert((w == 0 || g->Width == w) && (g->Height == h || h == 0));
	}

	return g;
}

/**
**  Make a new graphic object.  Don't reuse a graphic from the hash table.
**
**  @param file  Filename
**  @param w     Width of a frame (optional)
**  @param h     Height of a frame (optional)
**
**  @return      New graphic object
*/
CGraphic *CGraphic::ForceNew(const std::string &file, int w, int h)
{
	CGraphic *g = new CGraphic;
	if (!g) {
		fprintf(stderr, "Out of memory\n");
		ExitFatal(-1);
	}
	g->File = file;
	int bufSize = file.size() + 32;
	char *hashfile = new char[bufSize];
	snprintf(hashfile, bufSize, "%s%d", file.c_str(), HashCount++);
	g->HashFile = hashfile;
	delete[] hashfile;
	g->Width = w;
	g->Height = h;
	g->original_frame_size = QSize(w, h);
	GraphicHash[g->HashFile] = g;

	return g;
}

/**
**  Clone a graphic
**
**  @param grayscale  Make grayscale texture
*/
CPlayerColorGraphic *CPlayerColorGraphic::Clone(bool grayscale) const
{
	CPlayerColorGraphic *g = CPlayerColorGraphic::ForceNew(this->File, this->Resized ? 0 : this->Width, this->Resized ? 0 : this->Height);

	if (this->IsLoaded()) {
		g->Load(grayscale);
		if (this->Resized) {
			g->Resize(this->GraphicWidth, this->GraphicHeight);
		}
	}

	return g;
}

/**
**  Make a new player color graphic object.  Don't reuse a graphic from the
**  hash table.
**
**  @param file  Filename
**  @param w     Width of a frame (optional)
**  @param h     Height of a frame (optional)
**
**  @return      New graphic object
*/
CPlayerColorGraphic *CPlayerColorGraphic::ForceNew(const std::string &file, int w, int h)
{
	CPlayerColorGraphic *g = new CPlayerColorGraphic;
	if (!g) {
		fprintf(stderr, "Out of memory\n");
		ExitFatal(-1);
	}
	g->File = file;
	size_t bufSize = file.size() + 32;
	char *hashfile = new char[bufSize];
	snprintf(hashfile, bufSize, "%s%d", file.c_str(), HashCount++);
	g->HashFile = hashfile;
	delete[] hashfile;
	g->Width = w;
	g->Height = h;
	g->original_frame_size = QSize(w, h);
	GraphicHash[g->HashFile] = g;

	return g;
}

/**
**  Get a graphic object.
**
**  @param filename  Filename
**
**  @return      Graphic object
*/
CGraphic *CGraphic::Get(const std::string &filename)
{
	if (filename.empty()) {
		return nullptr;
	}

	const std::string file = LibraryFileName(filename.c_str());
	CGraphic *&g = GraphicHash[file];

	return g;
}

/**
**  Get a player color graphic object.
**
**  @param filename  Filename
**
**  @return      Graphic object
*/
CPlayerColorGraphic *CPlayerColorGraphic::Get(const std::string &filename)
{
	if (filename.empty()) {
		return nullptr;
	}

	const std::string file = LibraryFileName(filename.c_str());
	CPlayerColorGraphic *g = dynamic_cast<CPlayerColorGraphic *>(GraphicHash[file]);

	return g;
}

void CGraphic::GenFramesMap()
{
	Assert(NumFrames != 0);
#if defined(USE_OPENGL) || defined(USE_GLES)
	Assert(GraphicWidth != 0);
#endif
	Assert(Width != 0);
	Assert(Height != 0);

	frame_map.resize(NumFrames);

	for (int frame = 0; frame < NumFrames; ++frame) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		frame_map[frame].x = (frame % (GraphicWidth / Width)) * Width;
		frame_map[frame].y = (frame / (GraphicWidth / Width)) * Height;
#endif
	}
}

static void ApplyGrayScale(QImage &image, int Width, int Height)
{
	const int bpp = image.depth() / 8;
	const double redGray = 0.21;
	const double greenGray = 0.72;
	const double blueGray = 0.07;

	switch (bpp) {
		case 1: {
			for (int i = 0; i < image.colorCount(); ++i) {
				const QRgb rgb = image.color(i);
				QColor color(rgb);
				const int gray = redGray * color.red() + greenGray * color.green() + blueGray * color.blue();
				color.setRed(gray);
				color.setGreen(gray);
				color.setBlue(gray);
				image.setColor(i, color.rgba());
			}
			break;
		}
		case 4: {
			unsigned char *image_data = image.bits();
			for (int x = 0; x < image.width(); ++x) {
				for (int y = 0; y < image.height(); ++y) {
					const int pixel_index = (y * image.height() + x) * bpp;
					unsigned char &red = image_data[pixel_index];
					unsigned char &green = image_data[pixel_index + 1];
					unsigned char &blue = image_data[pixel_index + 2];

					const int gray = redGray * red + greenGray * green + blueGray * blue;
					red = gray;
					green = gray;
					blue = gray;
				}
			}
			break;
		}
	}
}

//Wyrmgus start
static void ApplySepiaScale(QImage &image)
{
	const int bpp = image.depth() / 8;

	switch (bpp) {
		case 1: {
			for (int i = 0; i < image.colorCount(); ++i) {
				const QRgb rgb = image.color(i);
				QColor color(rgb);

				const int input_red = color.red();
				const int input_green = color.green();
				const int input_blue = color.blue();

				color.setRed(std::min<int>(255, (input_red * .393) + (input_green * .769) + (input_blue * .189)));
				color.setGreen(std::min<int>(255, (input_red * .349) + (input_green * .686) + (input_blue * .168)));
				color.setBlue(std::min<int>(255, (input_red * .272) + (input_green * .534) + (input_blue * .131)));

				image.setColor(i, color.rgba());
			}
			break;
		}
		case 4: {
			unsigned char *image_data = image.bits();
			for (int x = 0; x < image.width(); ++x) {
				for (int y = 0; y < image.height(); ++y) {
					const int pixel_index = (y * image.height() + x) * bpp;
					unsigned char &red = image_data[pixel_index];
					unsigned char &green = image_data[pixel_index + 1];
					unsigned char &blue = image_data[pixel_index + 2];
					
					const int input_red = red;
					const int input_green = green;
					const int input_blue = blue;
					
					red = std::min<int>(255, (input_red * .393) + (input_green * .769) + (input_blue * .189));
					green = std::min<int>(255, (input_red * .349) + (input_green * .686) + (input_blue * .168));
					blue = std::min<int>(255, (input_red * .272) + (input_green * .534) + (input_blue * .131));
				}
			}
			break;
		}
	}
}

/*
static std::string map_terrains[8192][4096];

static void ConvertImageToMap(SDL_Surface *Surface, int Width, int Height)
{
	SDL_LockSurface(Surface);
	const SDL_PixelFormat *f = Surface->format;
	const int bpp = Surface->format->BytesPerPixel;
	Uint8 r, g, b;

	for (int j = 0; j < Height; ++j) {
		for (int i = 0; i < Width; ++i) {
			Uint32 c = *reinterpret_cast<Uint32 *>(&reinterpret_cast<Uint8 *>(Surface->pixels)[i * 4 + j * Surface->pitch]);
			Uint8 a;

			Video.GetRGBA(c, Surface->format, &r, &g, &b, &a);
			if (a >= 128) {
				if (r == 235 && g == 235 && b == 235) {
					map_terrains[i][j] = "s";
				} else if (r == 138 && g == 11 && b == 26) {
					map_terrains[i][j] = "r";
				} else if (r == 69 && g == 91 && b == 186) {
					map_terrains[i][j] = "w";
				} else if (r == 178 && g == 34 && b == 34) {
					map_terrains[i][j] = "m";
				} else if (r == 0 && g == 255 && b == 255) {
					map_terrains[i][j] = "e";
				} else {
					map_terrains[i][j] = "d";
				}
			} else {
				map_terrains[i][j] = "0";
			}
		}
	}
	SDL_UnlockSurface(Surface);
	
	FileWriter *fw = nullptr;
	std::string map_filename = "scripts/map_templates/new.map";

	try {
		fw = CreateFileWriter(map_filename);

		for (int y = 0; y < Height; ++y) {
			for (int x = 0; x < Width; ++x) {
				fw->printf("%s", map_terrains[x][y].c_str());
			}
			fw->printf("\n");
		}
			
		fw->printf("\n");
	} catch (const FileException &) {
		fprintf(stderr, "Couldn't write the map setup: \"%s\"\n", map_filename.c_str());
		delete fw;
		return;
	}
	
	delete fw;
}
*/
//Wyrmgus end

/**
**  Load a graphic
**
**  @param grayscale  Make a grayscale surface
*/
void CGraphic::Load(const bool grayscale, const int scale_factor)
{
	if (this->IsLoaded()) {
		return;
	}

	// TODO: More formats?
	if (LoadGraphicPNG(this) == -1) {
		fprintf(stderr, "Can't load the graphic '%s'\n", File.c_str());
		ExitFatal(-1);
	}

	if (!Width) {
		Width = GraphicWidth;
		this->original_frame_size.setWidth(Width);
	}
	if (!Height) {
		Height = GraphicHeight;
		this->original_frame_size.setHeight(Height);
	}

	Assert(Width <= GraphicWidth && Height <= GraphicHeight);

	if ((GraphicWidth / Width) * Width != GraphicWidth ||
		(GraphicHeight / Height) * Height != GraphicHeight) {
		fprintf(stderr, "Invalid graphic (width, height) %s\n", File.c_str());
		fprintf(stderr, "Expected: (%d,%d)  Found: (%d,%d)\n",
				Width, Height, GraphicWidth, GraphicHeight);
		ExitFatal(-1);
	}

	NumFrames = GraphicWidth / Width * GraphicHeight / Height;

	if (grayscale) {
		//Wyrmgus start
		this->Grayscale = true;
//		ApplyGrayScale(Surface, Width, Height);
		if (Preference.SepiaForGrayscale) {
			ApplySepiaScale(this->image);
		} else {
			ApplyGrayScale(this->image, Width, Height);
		}
		//Wyrmgus end
	}
	
	/*
	if (Width == 1792) {
		ConvertImageToMap(Surface, Width, Height);
	}
	*/
	
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		MakeTexture(this);
		Graphics.push_back(this);
	}
#endif
	GenFramesMap();

	if (scale_factor != 1) {
		this->Resize(this->GraphicWidth * scale_factor, this->GraphicHeight * scale_factor);
	}
}

/**
**  Free a SDL surface
**
**  @param surface  SDL surface to free
*/
static void FreeSurface(SDL_Surface **surface)
{
	if (!*surface) {
		return;
	}
	VideoPaletteListRemove(*surface);

	unsigned char *pixels = nullptr;

	if ((*surface)->flags & SDL_PREALLOC) {
		pixels = (unsigned char *)(*surface)->pixels;
	}

	SDL_FreeSurface(*surface);
	delete[] pixels;
	*surface = nullptr;
}

/**
**  Free a graphic
**
**  @param g  Pointer to the graphic
*/
void CGraphic::Free(CGraphic *g)
{
	if (!g) {
		return;
	}

	Assert(g->Refs);

	--g->Refs;
	if (!g->Refs) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		// No more uses of this graphic
		if (UseOpenGL) {
			if (g->Textures) {
				glDeleteTextures(g->NumTextures, g->Textures);
				delete[] g->Textures;
			}
			
			for (std::map<CColor, GLuint *>::iterator iterator = g->TextureColorModifications.begin(); iterator != g->TextureColorModifications.end(); ++iterator) {
				glDeleteTextures(g->NumTextures, iterator->second);
				delete[] iterator->second;
			}
			g->TextureColorModifications.clear();

			CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(g);
			if (cg) {
				for (int i = 0; i < PlayerColorMax; ++i) {
					if (cg->PlayerColorTextures[i]) {
						glDeleteTextures(cg->NumTextures, cg->PlayerColorTextures[i]);
						delete[] cg->PlayerColorTextures[i];
					}
				}
					
				for (std::map<int, std::map<CColor, GLuint *>>::iterator iterator = cg->PlayerColorTextureColorModifications.begin(); iterator != cg->PlayerColorTextureColorModifications.end(); ++iterator) {
					for (std::map<CColor, GLuint *>::iterator sub_iterator = iterator->second.begin(); sub_iterator != iterator->second.end(); ++sub_iterator) {
						glDeleteTextures(cg->NumTextures, sub_iterator->second);
						delete[] sub_iterator->second;
					}
					iterator->second.clear();
				}
				cg->PlayerColorTextureColorModifications.clear();
			}
			Graphics.remove(g);
		}
#endif

		g->frame_map.clear();

#if defined(USE_OPENGL) || defined(USE_GLES)
		if (!UseOpenGL)
#endif
		{
			FreeSurface(&g->SurfaceFlip);
			g->frameFlip_map.clear();
			
			//Wyrmgus start
			if (g->DawnSurface) {
				FreeSurface(&g->DawnSurface);
			}
			if (g->DawnSurfaceFlip) {
				FreeSurface(&g->DawnSurfaceFlip);
			}
			if (g->DuskSurface) {
				FreeSurface(&g->DuskSurface);
			}
			if (g->DuskSurfaceFlip) {
				FreeSurface(&g->DuskSurfaceFlip);
			}
			if (g->NightSurface) {
				FreeSurface(&g->NightSurface);
			}
			if (g->NightSurfaceFlip) {
				FreeSurface(&g->NightSurfaceFlip);
			}

			CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(g);
			if (cg) {
				for (int i = 0; i < PlayerColorMax; ++i) {
					if (cg->PlayerColorSurfaces[i]) {
						FreeSurface(&cg->PlayerColorSurfaces[i]);
					}
					if (cg->PlayerColorSurfacesFlip[i]) {
						FreeSurface(&cg->PlayerColorSurfacesFlip[i]);
					}
					if (cg->PlayerColorSurfacesDawn[i]) {
						FreeSurface(&cg->PlayerColorSurfacesDawn[i]);
					}
					if (cg->PlayerColorSurfacesDawnFlip[i]) {
						FreeSurface(&cg->PlayerColorSurfacesDawnFlip[i]);
					}
					if (cg->PlayerColorSurfacesDusk[i]) {
						FreeSurface(&cg->PlayerColorSurfacesDusk[i]);
					}
					if (cg->PlayerColorSurfacesDuskFlip[i]) {
						FreeSurface(&cg->PlayerColorSurfacesDuskFlip[i]);
					}
					if (cg->PlayerColorSurfacesNight[i]) {
						FreeSurface(&cg->PlayerColorSurfacesNight[i]);
					}
					if (cg->PlayerColorSurfacesNightFlip[i]) {
						FreeSurface(&cg->PlayerColorSurfacesNightFlip[i]);
					}
				}
			}
			//Wyrmgus end
		}

		if (!g->HashFile.empty()) {
			GraphicHash.erase(g->HashFile);
		}
		delete g;
	}
}

#if defined(USE_OPENGL) || defined(USE_GLES)

/**
**  Free OpenGL graphics
*/
void FreeOpenGLGraphics()
{
	std::list<CGraphic *>::iterator i;
	for (i = Graphics.begin(); i != Graphics.end(); ++i) {
		if ((*i)->Textures) {
			glDeleteTextures((*i)->NumTextures, (*i)->Textures);
		}
		
		for (std::map<CColor, GLuint *>::iterator iterator = (*i)->TextureColorModifications.begin(); iterator != (*i)->TextureColorModifications.end(); ++iterator) {
			glDeleteTextures((*i)->NumTextures, iterator->second);
		}
		(*i)->TextureColorModifications.clear();
		
		CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(*i);
		if (cg) {
			for (int j = 0; j < PlayerColorMax; ++j) {
				if (cg->PlayerColorTextures[j]) {
					glDeleteTextures(cg->NumTextures, cg->PlayerColorTextures[j]);
				}
				
			}
			
			for (std::map<int, std::map<CColor, GLuint *>>::iterator iterator = cg->PlayerColorTextureColorModifications.begin(); iterator != cg->PlayerColorTextureColorModifications.end(); ++iterator) {
				for (std::map<CColor, GLuint *>::iterator sub_iterator = iterator->second.begin(); sub_iterator != iterator->second.end(); ++sub_iterator) {
					glDeleteTextures(cg->NumTextures, sub_iterator->second);
				}
				iterator->second.clear();
			}
			cg->PlayerColorTextureColorModifications.clear();
		}
	}
}

/**
**	@brief	Reload OpenGL graphics
*/
void ReloadGraphics()
{
	std::list<CGraphic *>::iterator i;
	for (i = Graphics.begin(); i != Graphics.end(); ++i) {
		if ((*i)->Textures) {
			delete[](*i)->Textures;
			(*i)->Textures = nullptr;
			MakeTexture(*i);
		}
		
		std::vector<CColor> color_modifications;
		for (std::map<CColor, GLuint *>::iterator iterator = (*i)->TextureColorModifications.begin(); iterator != (*i)->TextureColorModifications.end(); ++iterator) {
			delete[] iterator->second;
			color_modifications.push_back(iterator->first);
		}
		(*i)->TextureColorModifications.clear();
				
		CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(*i);
		if (cg) {
			for (std::map<int, std::map<CColor, GLuint *>>::iterator iterator = cg->PlayerColorTextureColorModifications.begin(); iterator != cg->PlayerColorTextureColorModifications.end(); ++iterator) {
				for (std::map<CColor, GLuint *>::iterator sub_iterator = iterator->second.begin(); sub_iterator != iterator->second.end(); ++sub_iterator) {
					delete[] sub_iterator->second;
				}
				iterator->second.clear();
			}
			cg->PlayerColorTextureColorModifications.clear();				
			
			for (int j = 0; j < PlayerColorMax; ++j) {
				if (cg->PlayerColorTextures[j]) {
					delete[] cg->PlayerColorTextures[j];
					cg->PlayerColorTextures[j] = nullptr;
					MakePlayerColorTexture(cg, j, nullptr);
				}
			}
		}
	}
}

#endif

#if defined(USE_OPENGL) || defined(USE_GLES)

/**
**  Find the next power of 2 >= x
*/
static int PowerOf2(int x)
{
	int i;
	for (i = 1; i < x; i <<= 1) ;
	return i;
}

/**
**  Make an OpenGL texture or textures out of a graphic object.
**
**  @param g        The graphic object.
**  @param texture  Texture.
**  @param colors   Unit colors.
**  @param ow       Offset width.
**  @param oh       Offset height.
*/
void MakeTextures2(const CGraphic *g, const QImage &image, GLuint texture, CUnitColors *colors, const int ow, const int oh, const stratagus::time_of_day *time_of_day)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	int maxw = std::min<int>(image.width() - ow, GLMaxTextureSize);
	int maxh = std::min<int>(image.height() - oh, GLMaxTextureSize);
	int w = PowerOf2(maxw);
	int h = PowerOf2(maxh);
	unsigned char *tex = new unsigned char[w * h * 4];
	memset(tex, 0, w * h * 4);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//Wyrmgus start
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (!g->Grayscale) { // don't alter the colors of grayscale graphics
		if (time_of_day && time_of_day->HasColorModification()) {
			time_of_day_red = time_of_day->ColorModification.R;
			time_of_day_green = time_of_day->ColorModification.G;
			time_of_day_blue = time_of_day->ColorModification.B;
		}
	}
	//Wyrmgus end

	if (image.isNull()) {
		throw std::runtime_error("Cannot generate a texture for a null image.");
	}

	const int depth = image.depth();
	const int bpp = depth / 8;

	if (bpp != 4) {
		throw std::runtime_error("The image BPP must be 4 for generating textures.");
	}

	const unsigned char *image_data = image.constBits();
	for (int x = 0; x < maxw; ++x) {
		for (int y = 0; y < maxh; ++y) {
			const int src_index = ((oh + y) * image.width() + ow + x) * bpp;
			const unsigned char &src_red = image_data[src_index];
			const unsigned char &src_green = image_data[src_index + 1];
			const unsigned char &src_blue = image_data[src_index + 2];
			const unsigned char &src_alpha = image_data[src_index + 3];

			const int dst_index = (y * w + x) * 4;
			unsigned char &dst_red = tex[dst_index];
			unsigned char &dst_green = tex[dst_index + 1];
			unsigned char &dst_blue = tex[dst_index + 2];
			unsigned char &dst_alpha = tex[dst_index + 3];

			dst_red = src_red;
			dst_green = src_green;
			dst_blue = src_blue;
			dst_alpha = src_alpha;

			if (g->Grayscale) {
				continue;
			}

			if (colors != nullptr) {
				for (size_t k = 0; k < ConversiblePlayerColors.size(); ++k) {
					if (PlayerColorNames[ConversiblePlayerColors[k]].empty()) {
						break;
					}

					for (size_t z = 0; z < PlayerColorsRGB[ConversiblePlayerColors[k]].size(); ++z) {
						if (dst_red == PlayerColorsRGB[ConversiblePlayerColors[k]][z].R && dst_green == PlayerColorsRGB[ConversiblePlayerColors[k]][z].G && dst_blue == PlayerColorsRGB[ConversiblePlayerColors[k]][z].B) {
							dst_red = colors->Colors[z].R;
							dst_green = colors->Colors[z].G;
							dst_blue = colors->Colors[z].B;
						}
					}
				}
			}

			if (time_of_day_red != 0) {
				dst_red = std::max<int>(0, std::min<int>(255, dst_red + time_of_day_red));
			}
			if (time_of_day_green != 0) {
				dst_green = std::max<int>(0, std::min<int>(255, dst_green + time_of_day_green));
			}
			if (time_of_day_blue != 0) {
				dst_blue = std::max<int>(0, std::min<int>(255, dst_blue + time_of_day_blue));
			}
		}
	}

	GLint internalformat = GL_RGBA;
#ifdef USE_OPENGL
	if (GLTextureCompressionSupported && UseGLTextureCompression) {
		internalformat = GL_COMPRESSED_RGBA;
	}
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);

#ifdef DEBUG
	int x;
	if ((x = glGetError())) {
		DebugPrint("glTexImage2D(%x)\n" _C_ x);
	}
#endif
	delete[] tex;
}

/**
**  Make an OpenGL texture or textures out of a graphic object.
**
**  @param g       The graphic object.
**  @param player  Player number.
**  @param colors  Unit colors.
*/
//Wyrmgus start
//static void MakeTextures(CGraphic *g, int player, CUnitColors *colors)
static void MakeTextures(CGraphic *g, int player, CUnitColors *colors, const stratagus::time_of_day *time_of_day)
//Wyrmgus end
{
	int tw = (g->get_image().width() - 1) / GLMaxTextureSize + 1;
	const int th = (g->get_image().height() - 1) / GLMaxTextureSize + 1;

	int w = g->get_image().width() % GLMaxTextureSize;
	if (w == 0) {
		w = GLMaxTextureSize;
	}
	g->TextureWidth = (GLfloat)w / PowerOf2(w);

	int h = g->get_image().height() % GLMaxTextureSize;
	if (h == 0) {
		h = GLMaxTextureSize;
	}
	g->TextureHeight = (GLfloat)h / PowerOf2(h);

	g->NumTextures = tw * th;

	CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(g);
	GLuint *textures;
	if (!colors || !cg) {
		if (time_of_day && time_of_day->HasColorModification()) {
			textures = g->TextureColorModifications[time_of_day->ColorModification] = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->TextureColorModifications[time_of_day->ColorModification]);
		} else {
			textures = g->Textures = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->Textures);
		}
	} else if (time_of_day && time_of_day->HasColorModification()) {
		textures = cg->PlayerColorTextureColorModifications[player][time_of_day->ColorModification] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->PlayerColorTextureColorModifications[player][time_of_day->ColorModification]);
	} else {
		textures = cg->PlayerColorTextures[player] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->PlayerColorTextures[player]);
	}

	const QImage *src_image = nullptr;
	QImage reformatted_image;
	if (g->get_image().format() == QImage::Format_RGBA8888) {
		src_image = &g->get_image();
	} else {
		reformatted_image = g->get_image().convertToFormat(QImage::Format_RGBA8888);
		src_image = &reformatted_image;
	}

	for (int j = 0; j < th; ++j) {
		for (int i = 0; i < tw; ++i) {
			MakeTextures2(g, *src_image, textures[j * tw + i], colors, GLMaxTextureSize * i, GLMaxTextureSize * j, time_of_day);
		}
	}
}

/**
**  Make an OpenGL texture or textures out of a graphic object.
**
**  @param g  The graphic object.
*/
//Wyrmgus start
//void MakeTexture(CGraphic *g)
void MakeTexture(CGraphic *g, const stratagus::time_of_day *time_of_day)
//Wyrmgus end
{
	//Wyrmgus start
//	if (g->Textures) {
//		return;
//	}
	if (time_of_day && time_of_day->HasColorModification()) {
		if (g->TextureColorModifications.find(time_of_day->ColorModification) != g->TextureColorModifications.end()) {
			return;
		}
	} else {
		if (g->Textures) {
			return;
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	MakeTextures(g, 0, nullptr);
	MakeTextures(g, 0, nullptr, time_of_day);
	//Wyrmgus end
}

/**
**  Make an OpenGL texture with the player colors.
**
**  @param g       The graphic to texture with player colors.
**  @param player  Player number to make textures for.
*/
//Wyrmgus start
//void MakePlayerColorTexture(CPlayerColorGraphic *g, int player)
void MakePlayerColorTexture(CPlayerColorGraphic *g, int player, const stratagus::time_of_day *time_of_day)
//Wyrmgus end
{
	//Wyrmgus start
	/*
	if (g->PlayerColorTextures[player]) {
		return;
	}
	*/
	if (time_of_day && time_of_day->HasColorModification()) {
		if (g->PlayerColorTextureColorModifications.find(player) != g->PlayerColorTextureColorModifications.end() && g->PlayerColorTextureColorModifications[player].find(time_of_day->ColorModification) != g->PlayerColorTextureColorModifications[player].end()) {
			return;
		}
	} else {
		if (g->PlayerColorTextures[player]) {
			return;
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	MakeTextures(g, player, &CPlayer::Players[player]->UnitColors);
	CUnitColors texture_unit_colors;
	texture_unit_colors.Colors = PlayerColorsRGB[player];
	MakeTextures(g, player, &texture_unit_colors, time_of_day);
	//Wyrmgus end
}

#endif

/**
**  Resize a graphic
**
**  @param w  New width of graphic.
**  @param h  New height of graphic.
*/
void CGraphic::Resize(int w, int h)
{
	Assert(this->IsLoaded()); // can't resize before it's been loaded

	if (GraphicWidth == w && GraphicHeight == h) {
		return;
	}

	const QSize old_size(this->GraphicWidth, this->GraphicHeight);
	const QSize old_frame_size(this->Width, this->Height);
	const QSize frame_size(this->Width * w / old_size.width(), this->Height * h / old_size.height());

	// Resizing the same image multiple times looks horrible
	// If the image has already been resized then get a clean copy first
	if (Resized) {
		this->SetOriginalSize();
		if (GraphicWidth == w && GraphicHeight == h) {
			return;
		}
	}

	Resized = true;

	const int bpp = this->get_image().depth() / 8;
	if (bpp < 4) {
		this->image = this->image.scaled(w, h);
	} else if (w > old_size.width() && (w % old_size.width()) == 0 && (h % old_size.height()) == 0 && h > old_size.height() && (w / old_size.width()) == (h / old_size.height())) {
		if (bpp != 4) {
			throw std::runtime_error("Image \"" + this->getFile() + "\" cannot be scaled with xBRZ, as its bytes-per-pixel are not 4.");
		}

		//if a simple scale factor is being used for the resizing, then use xBRZ for the rescaling
		const int scale_factor = w / old_size.width();
		this->image = stratagus::image::scale(this->image, scale_factor, old_frame_size);
	} else {
		this->image = this->image.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}
	GraphicWidth = w;
	GraphicHeight = h;

	this->Width = frame_size.width();
	this->Height = frame_size.height();

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL && Textures) {
		glDeleteTextures(NumTextures, Textures);
		delete[] Textures;
		Textures = nullptr;
		MakeTexture(this);
	}
#endif
	GenFramesMap();
}

/**
**  Sets the original size for a graphic
**
*/
void CGraphic::SetOriginalSize()
{
	Assert(this->IsLoaded()); // can't resize before it's been loaded

	if (!Resized) {
		return;
	}
	
	if (!this->get_image().isNull()) {
		this->image = QImage();
	}
	this->frame_map.clear();

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL && Textures) {
		glDeleteTextures(NumTextures, Textures);
		delete[] Textures;
		Textures = nullptr;
	}
#endif

	this->Width = this->Height = 0;
	this->image = QImage();
	this->Load();

	this->Resized = false;
}

void FreeGraphics()
{
	std::map<std::string, CGraphic *>::iterator i;
	while (!GraphicHash.empty()) {
		i = GraphicHash.begin();
		CGraphic::Free((*i).second);
	}
}

void CFiller::Load()
{
	if (G) {
		G->Load(false, stratagus::defines::get()->get_scale_factor());
	}

	//Wyrmgus start
	this->X *= stratagus::defines::get()->get_scale_factor();
	this->Y *= stratagus::defines::get()->get_scale_factor();

	if (this->X < 0) {
		this->X = Video.Width + this->X;
	}
	
	if (this->Y < 0) {
		this->Y = Video.Height + this->Y;
	}
	//Wyrmgus end
}

bool CFiller::OnGraphic(int x, int y) const
{
	x -= X;
	y -= Y;
	if (x >= 0 && y >= 0 && x < this->G->get_image().width() && y < this->G->get_image().height()) {
		return this->G->get_image().pixelColor(x, y).alpha() != 0;
	}
	return false;
}
