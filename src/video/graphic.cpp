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
	if (UseOpenGL) {
		DrawTexture(this, Textures, gx, gy, gx + w, gy + h, x, y, 0);
	} else
#endif
	{
		SDL_Rect srect = {Sint16(gx), Sint16(gy), Uint16(w), Uint16(h)};
		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};
		//Wyrmgus start
//		SDL_BlitSurface(Surface, &srect, TheScreen, &drect);
		SDL_BlitSurface(surface ? surface : Surface, &srect, TheScreen, &drect);
		//Wyrmgus end
		//Wyrmgus start
		//code for drawing a scaled image under xBRZ - use later for implementing zoom mode
		/*
		SDL_Rect srect = {Sint16(gx * 2), Sint16(gy * 2), Uint16(w * 2), Uint16(h * 2)};
		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};
		SDL_LockSurface(Surface);
		
		SDL_Surface *neutral_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,1,1,32,0xFF0000,0xFF00,0xFF,0xFF000000);
		SDL_PixelFormat format = *neutral_surface->format;
		
		int Rmask = format.Rmask;
		int Gmask = format.Gmask;
		int Bmask = format.Bmask;
		int Amask = format.Amask;
		int bpp = format.BitsPerPixel;
		
		SDL_Surface *src = SDL_ConvertSurface(Surface,&format,SDL_SWSURFACE);
							 
		SDL_Surface *dst = SDL_CreateRGBSurface(SDL_SWSURFACE, Width * 2, Height * 2,
							 bpp, Rmask, Gmask, Bmask, Amask);
		SDL_LockSurface(src);
		SDL_LockSurface(dst);
		const Uint32* old_pixels = reinterpret_cast<const Uint32*>(src->pixels);
		Uint32* new_pixels = reinterpret_cast<Uint32*>(dst->pixels);
		xbrz::scale(2, old_pixels, new_pixels, Width, Height);
		SDL_SetAlpha(SDL_DisplayFormatAlpha(dst),SDL_SRCALPHA|SDL_RLEACCEL,SDL_ALPHA_OPAQUE);
		SDL_UnlockSurface(Surface);
		SDL_UnlockSurface(src);
		SDL_UnlockSurface(dst);
		SDL_BlitSurface(dst, &srect, TheScreen, &drect);

		unsigned char *src_pixels = nullptr;

		if (src->flags & SDL_PREALLOC) {
			src_pixels = (unsigned char *)src->pixels;
		}
		SDL_FreeSurface(src);
		delete[] src_pixels;
		src = nullptr;

		unsigned char *dst_pixels = nullptr;

		if (dst->flags & SDL_PREALLOC) {
			dst_pixels = (unsigned char *)dst->pixels;
		}
		SDL_FreeSurface(dst);
		delete[] dst_pixels;
		dst = nullptr;
		*/
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
		if (!PlayerColorTextures[player]) {
			MakePlayerColorTexture(this, player, nullptr);
		}
		DrawTexture(this, PlayerColorTextures[player], gx, gy, gx + w, gy + h, x, y, 0);
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(player, *this);
		//Wyrmgus end
		SDL_Rect srect = {Sint16(gx), Sint16(gy), Uint16(w), Uint16(h)};
		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};
		//Wyrmgus start
//		SDL_BlitSurface(Surface, &srect, TheScreen, &drect);
		if (!PlayerColorSurfaces[player]) {
			MakePlayerColorSurface(player, true, nullptr);
		}
		SDL_BlitSurface(PlayerColorSurfaces[player], &srect, TheScreen, &drect);
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		DrawSub(gx, gy, w, h, x, y);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
		/*
		int oldalpha = Surface->format->alpha;
		SDL_SetAlpha(Surface, SDL_SRCALPHA, alpha);
		DrawSub(gx, gy, w, h, x, y);
		SDL_SetAlpha(Surface, SDL_SRCALPHA, oldalpha);
		*/
		if (!surface) {
			surface = Surface;
		}
		
		int oldalpha = surface->format->alpha;
		SDL_SetAlpha(surface, SDL_SRCALPHA, alpha);
		DrawSub(gx, gy, w, h, x, y, surface);
		SDL_SetAlpha(surface, SDL_SRCALPHA, oldalpha);
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
		//Wyrmgus start
//		DoDrawFrameClip(Textures, frame, x, y);
		if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
			DoDrawFrameClip(Textures, frame, x, y, show_percent);
		} else {
			if (TextureColorModifications.find(time_of_day->ColorModification) == TextureColorModifications.end()) {
				MakeTexture(this, time_of_day);
			}
			DoDrawFrameClip(TextureColorModifications[time_of_day->ColorModification], frame, x, y, show_percent);
		}
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
		if (time_of_day != nullptr && !surface) {
			surface = SetTimeOfDay(time_of_day, false);
		}
		//Wyrmgus end
		DrawSubClip(frame_map[frame].x, frame_map[frame].y,
					//Wyrmgus start
//					Width, Height, x, y);
					Width, Height * show_percent / 100, x, y, surface);
					//Wyrmgus end
	}
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
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		//Wyrmgus start
//		DrawFrameClip(frame, x, y);
		DrawFrameClip(frame, x, y, time_of_day, surface, show_percent);
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
		if (time_of_day != nullptr && !surface) {
			surface = SetTimeOfDay(time_of_day, false);
		}
		//Wyrmgus end
		DrawSubClipTrans(frame_map[frame].x, frame_map[frame].y,
						 //Wyrmgus start
//						 Width, Height, x, y, alpha);
						 Width, Height * show_percent / 100, x, y, alpha, surface);
						 //Wyrmgus end
	}
}

//Wyrmgus start
void CPlayerColorGraphic::MakePlayerColorSurface(int player_color, bool flipped, const stratagus::time_of_day *time_of_day)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		return;
	}
#endif
	SDL_Surface *surface = nullptr;
	if (time_of_day && time_of_day->is_dawn()) {
		if (flipped) {
			surface = PlayerColorSurfacesDawnFlip[player_color];
		} else {
			surface = PlayerColorSurfacesDawn[player_color];
		}
	} else if (time_of_day && time_of_day->is_dusk()) {
		if (flipped) {
			surface = PlayerColorSurfacesDuskFlip[player_color];
		} else {
			surface = PlayerColorSurfacesDusk[player_color];
		}
	} else if (time_of_day && time_of_day->is_night()) {
		if (flipped) {
			surface = PlayerColorSurfacesNightFlip[player_color];
		} else {
			surface = PlayerColorSurfacesNight[player_color];
		}
	} else {
		if (flipped) {
			surface = PlayerColorSurfacesFlip[player_color];
		} else {
			surface = PlayerColorSurfaces[player_color];
		}
	}

	if (surface) {
		return;
	}
	
	SDL_Surface *base_surface = flipped ? SurfaceFlip : Surface;
	
	if (time_of_day && time_of_day->is_dawn()) {
		if (flipped) {
			surface = PlayerColorSurfacesDawnFlip[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfacesDawn[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	} else if (time_of_day && time_of_day->is_dusk()) {
		if (flipped) {
			surface = PlayerColorSurfacesDuskFlip[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfacesDusk[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	} else if (time_of_day && time_of_day->is_night()) {
		if (flipped) {
			surface = PlayerColorSurfacesNightFlip[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfacesNight[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	} else {
		if (flipped) {
			surface = PlayerColorSurfacesFlip[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfaces[player_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	}

	if (base_surface->flags & SDL_SRCCOLORKEY) {
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, base_surface->format->colorkey);
	}
	if (surface->format->BytesPerPixel == 1) {
		VideoPaletteListAdd(surface);
	}
	
	int found_player_color = -1;
	
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (!this->Grayscale) { // don't alter the colors of grayscale graphics
		if (time_of_day && time_of_day->is_dawn()) { // dawn
			time_of_day_red = -20;
			time_of_day_green = -20;
			time_of_day_blue = 0;
		} else if (time_of_day && time_of_day->is_dusk()) { // dusk
			time_of_day_red = 0;
			time_of_day_green = -20;
			time_of_day_blue = -20;
		} else if (time_of_day && time_of_day->is_night()) { // night
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
		}
	}
	
	SDL_LockSurface(surface);
	
	switch (surface->format->BytesPerPixel) {
		case 1: {
			SDL_Color colors[256];
			SDL_Palette &pal = *surface->format->palette;
			for (int i = 0; i < 256; ++i) {
				int red = pal.colors[i].r;
				int green = pal.colors[i].g;
				int blue = pal.colors[i].b;
				
				if (player_color != -1 && !this->Grayscale) {
					for (size_t k = 0; k < ConversiblePlayerColors.size(); ++k) {
						if (PlayerColorNames[k].empty()) {
							break;
						}
						if (ConversiblePlayerColors[k] == player_color) {
							continue;
						}
							
						for (size_t z = 0; z < PlayerColorsRGB[ConversiblePlayerColors[k]].size(); ++z) {
							if (pal.colors[i].r == PlayerColorsRGB[ConversiblePlayerColors[k]][z].R && pal.colors[i].g == PlayerColorsRGB[ConversiblePlayerColors[k]][z].G && pal.colors[i].b == PlayerColorsRGB[ConversiblePlayerColors[k]][z].B) {
								red = PlayerColorsRGB[player_color][z].R;
								green = PlayerColorsRGB[player_color][z].G;
								blue = PlayerColorsRGB[player_color][z].B;
								
								if (found_player_color == -1) {
									found_player_color = ConversiblePlayerColors[k];
								} else if (found_player_color != ConversiblePlayerColors[k]) {
									fprintf(stderr, "\"%s\" contains tones of both \"%s\" and \"%s\" player colors.\n", this->File.c_str(), PlayerColorNames[ConversiblePlayerColors[k]].c_str(), PlayerColorNames[found_player_color].c_str());
								}
							}
						}
					}
				}
				
				colors[i].r = std::max<int>(0,std::min<int>(255,int(red) + time_of_day_red));
				colors[i].g = std::max<int>(0,std::min<int>(255,int(green) + time_of_day_green));
				colors[i].b = std::max<int>(0,std::min<int>(255,int(blue) + time_of_day_blue));;
			}
			SDL_SetColors(surface, &colors[0], 0, 256);
			break;
		}
		case 4: {
			break;
		}
	}
	
	SDL_UnlockSurface(surface);
}
//Wyrmgus end

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
	if (UseOpenGL) {
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
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(*CPlayer::Players[player], *this);

		SDL_Surface *surface = nullptr;
		if (time_of_day == nullptr || time_of_day->is_day()) {
			if (!PlayerColorSurfaces[player]) {
				MakePlayerColorSurface(player, false, nullptr);
			}
			surface = PlayerColorSurfaces[player];
		} else if (time_of_day->is_dawn()) {
			if (!PlayerColorSurfacesDawn[player]) {
				MakePlayerColorSurface(player, false, time_of_day);
			}
			surface = PlayerColorSurfacesDawn[player];
		} else if (time_of_day->is_dusk()) {
			if (!PlayerColorSurfacesDusk[player]) {
				MakePlayerColorSurface(player, false, time_of_day);
			}
			surface = PlayerColorSurfacesDusk[player];
		} else if (time_of_day->is_night()) {
			if (!PlayerColorSurfacesNight[player]) {
				MakePlayerColorSurface(player, false, time_of_day);
			}
			surface = PlayerColorSurfacesNight[player];
		}
		
//		DrawFrameClip(frame, x, y);
		DrawFrameClip(frame, x, y, nullptr, surface);
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
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
//		DoDrawFrameClip(PlayerColorTextures[player], frame, x, y);
		if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
			DoDrawFrameClip(PlayerColorTextures[player], frame, x, y, show_percent);
		} else {
			DoDrawFrameClip(PlayerColorTextureColorModifications[player][time_of_day->ColorModification], frame, x, y, show_percent);
		}
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(*CPlayer::Players[player], *this);

		SDL_Surface *surface = nullptr;
		if (time_of_day == nullptr || time_of_day->is_day()) {
			if (!PlayerColorSurfaces[player]) {
				MakePlayerColorSurface(player, false, nullptr);
			}
			surface = PlayerColorSurfaces[player];
		} else if (time_of_day->is_dawn()) {
			if (!PlayerColorSurfacesDawn[player]) {
				MakePlayerColorSurface(player, false, time_of_day);
			}
			surface = PlayerColorSurfacesDawn[player];
		} else if (time_of_day->is_dusk()) {
			if (!PlayerColorSurfacesDusk[player]) {
				MakePlayerColorSurface(player, false, time_of_day);
			}
			surface = PlayerColorSurfacesDusk[player];
		} else if (time_of_day->is_night()) {
			if (!PlayerColorSurfacesNight[player]) {
				MakePlayerColorSurface(player, false, time_of_day);
			}
			surface = PlayerColorSurfacesNight[player];
		}
		
//		DrawFrameClipTrans(frame, x, y, alpha);
		DrawFrameClipTrans(frame, x, y, alpha, nullptr, surface);
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
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
//		DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
		if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
			DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
		} else {
			DoDrawFrameClipX(PlayerColorTextureColorModifications[player][time_of_day->ColorModification], frame, x, y);
		}
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(Players[player], *this);

		SDL_Surface *surface = nullptr;
		if (time_of_day == nullptr || time_of_day->is_day()) {
			if (!PlayerColorSurfacesFlip[player]) {
				MakePlayerColorSurface(player, true, nullptr);
			}
			surface = PlayerColorSurfacesFlip[player];
		} else if (time_of_day->is_dawn()) {
			if (!PlayerColorSurfacesDawnFlip[player]) {
				MakePlayerColorSurface(player, true, time_of_day);
			}
			surface = PlayerColorSurfacesDawnFlip[player];
		} else if (time_of_day->is_dusk()) {
			if (!PlayerColorSurfacesDuskFlip[player]) {
				MakePlayerColorSurface(player, true, time_of_day);
			}
			surface = PlayerColorSurfacesDuskFlip[player];
		} else if (time_of_day->is_night()) {
			if (!PlayerColorSurfacesNightFlip[player]) {
				MakePlayerColorSurface(player, true, time_of_day);
			}
			surface = PlayerColorSurfacesNightFlip[player];
		}
		
//		DrawFrameClipTransX(frame, x, y, alpha);
		DrawFrameClipTransX(frame, x, y, alpha, nullptr, surface);
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
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
	} else
#endif
	{
		//Wyrmgus start
		if (time_of_day != nullptr && !surface) {
			surface = SetTimeOfDay(time_of_day, true);
		}
		//Wyrmgus end
		SDL_Rect srect = {frameFlip_map[frame].x, frameFlip_map[frame].y, Uint16(Width), Uint16(Height)};

		const int oldx = x;
		const int oldy = y;
		CLIP_RECTANGLE(x, y, srect.w, srect.h);
		srect.x += x - oldx;
		srect.y += y - oldy;

		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};

		//Wyrmgus start
//		SDL_BlitSurface(SurfaceFlip, &srect, TheScreen, &drect);
		
		if (!surface) {
			surface = SurfaceFlip;
		}

		SDL_BlitSurface(surface, &srect, TheScreen, &drect);
		//Wyrmgus end
	}
}

void CGraphic::DrawFrameTransX(unsigned frame, int x, int y, int alpha) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		DrawFrameX(frame, x, y);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		SDL_Rect srect = {frameFlip_map[frame].x, frameFlip_map[frame].y, Uint16(Width), Uint16(Height)};
		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};
		const int oldalpha = Surface->format->alpha;

		SDL_SetAlpha(Surface, SDL_SRCALPHA, alpha);
		SDL_BlitSurface(SurfaceFlip, &srect, TheScreen, &drect);
		SDL_SetAlpha(Surface, SDL_SRCALPHA, oldalpha);
	}
}

//Wyrmgus start
//void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha) const
void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		//Wyrmgus start
//		DrawFrameClipX(frame, x, y);
		DrawFrameClipX(frame, x, y, time_of_day);
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
		if (time_of_day != nullptr && !surface) {
			surface = SetTimeOfDay(time_of_day, true);
		}
		//Wyrmgus end
		SDL_Rect srect = {frameFlip_map[frame].x, frameFlip_map[frame].y, Uint16(Width), Uint16(Height)};

		const int oldx = x;
		const int oldy = y;
		CLIP_RECTANGLE(x, y, srect.w, srect.h);
		srect.x += x - oldx;
		srect.y += y - oldy;

		SDL_Rect drect = {Sint16(x), Sint16(y), 0, 0};
		//Wyrmgus start
		/*
		const int oldalpha = SurfaceFlip->format->alpha;

		SDL_SetAlpha(SurfaceFlip, SDL_SRCALPHA, alpha);
		SDL_BlitSurface(SurfaceFlip, &srect, TheScreen, &drect);
		SDL_SetAlpha(SurfaceFlip, SDL_SRCALPHA, oldalpha);
		*/
		
		if (!surface) {
			surface = SurfaceFlip;
		}
		
		const int oldalpha = surface->format->alpha;

		SDL_SetAlpha(surface, SDL_SRCALPHA, alpha);
		SDL_BlitSurface(surface, &srect, TheScreen, &drect);
		SDL_SetAlpha(surface, SDL_SRCALPHA, oldalpha);
		//Wyrmgus end
	}
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
	if (UseOpenGL) {
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
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(*CPlayer::Players[player], *this);

		SDL_Surface *surface = nullptr;
		if (time_of_day == nullptr || time_of_day->is_day()) {
			if (!PlayerColorSurfacesFlip[player]) {
				MakePlayerColorSurface(player, true, nullptr);
			}
			surface = PlayerColorSurfacesFlip[player];
		} else if (time_of_day->is_dawn()) {
			if (!PlayerColorSurfacesDawnFlip[player]) {
				MakePlayerColorSurface(player, true, time_of_day);
			}
			surface = PlayerColorSurfacesDawnFlip[player];
		} else if (time_of_day->is_dusk()) {
			if (!PlayerColorSurfacesDuskFlip[player]) {
				MakePlayerColorSurface(player, true, time_of_day);
			}
			surface = PlayerColorSurfacesDuskFlip[player];
		} else if (time_of_day->is_night()) {
			if (!PlayerColorSurfacesNightFlip[player]) {
				MakePlayerColorSurface(player, true, time_of_day);
			}
			surface = PlayerColorSurfacesNightFlip[player];
		}
		
//		DrawFrameClipX(frame, x, y);
		DrawFrameClipX(frame, x, y, nullptr, surface);
		//Wyrmgus end
	}
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
CGraphic *CGraphic::New(const std::string &filename, int w, int h)
{
	if (filename.empty()) {
		return new CGraphic;
	}

	const std::string file = LibraryFileName(filename.c_str());
	CGraphic *&g = GraphicHash[file];
	if (g == nullptr) {
		g = new CGraphic;
		if (!g) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
		// FIXME: use a constructor for this
		g->File = file;
		g->HashFile = g->File;
		g->Width = w;
		g->Height = h;
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
CPlayerColorGraphic *CPlayerColorGraphic::New(const std::string &filename, int w, int h)
{
	if (filename.empty()) {
		return new CPlayerColorGraphic;
	}

	const std::string file = LibraryFileName(filename.c_str());
	CPlayerColorGraphic *g = dynamic_cast<CPlayerColorGraphic *>(GraphicHash[file]);
	if (g == nullptr) {
		g = new CPlayerColorGraphic;
		if (!g) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
		// FIXME: use a constructor for this
		g->File = file;
		g->HashFile = g->File;
		g->Width = w;
		g->Height = h;
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
	if (UseOpenGL) {
		Assert(GraphicWidth != 0);
	} else
#endif
	{
		Assert(Surface != nullptr);
	}
	Assert(Width != 0);
	Assert(Height != 0);

	delete[] frame_map;

	frame_map = new frame_pos_t[NumFrames];

	for (int frame = 0; frame < NumFrames; ++frame) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		if (UseOpenGL) {
			frame_map[frame].x = (frame % (GraphicWidth / Width)) * Width;
			frame_map[frame].y = (frame / (GraphicWidth / Width)) * Height;
		} else
#endif
		{
			frame_map[frame].x = (frame % (Surface->w / Width)) * Width;
			frame_map[frame].y = (frame / (Surface->w / Width)) * Height;
		}
	}
}

static void ApplyGrayScale(SDL_Surface *Surface, int Width, int Height)
{
	SDL_LockSurface(Surface);
	const SDL_PixelFormat *f = Surface->format;
	const int bpp = Surface->format->BytesPerPixel;
	const double redGray = 0.21;
	const double greenGray = 0.72;
	const double blueGray = 0.07;

	switch (bpp) {
		case 1: {
			SDL_Color colors[256];
			SDL_Palette &pal = *Surface->format->palette;
			for (int i = 0; i < 256; ++i) {
				const int gray = redGray * pal.colors[i].r + greenGray * pal.colors[i].g + blueGray * pal.colors[i].b;
				colors[i].r = colors[i].g = colors[i].b = gray;
			}
			SDL_SetColors(Surface, &colors[0], 0, 256);
			break;
		}
		case 4: {
			Uint32 *p;
			for (int i = 0; i < Height; ++i) {
				for (int j = 0; j < Width; ++j) {
					p = (Uint32 *)(Surface->pixels) + i * Width + j * bpp;
					const Uint32 gray = ((Uint8)((*p) * redGray) >> f->Rshift) +
										((Uint8)(*(p + 1) * greenGray) >> f->Gshift) +
										((Uint8)(*(p + 2) * blueGray) >> f->Bshift) +
										((Uint8)(*(p + 3)) >> f->Ashift);
					*p = gray;
				}
			}
			break;
		}
	}
	SDL_UnlockSurface(Surface);
}

//Wyrmgus start
static void ApplySepiaScale(SDL_Surface *Surface, int Width, int Height)
{
	SDL_LockSurface(Surface);
	const SDL_PixelFormat *f = Surface->format;
	const int bpp = Surface->format->BytesPerPixel;

	switch (bpp) {
		case 1: {
			SDL_Color colors[256];
			SDL_Palette &pal = *Surface->format->palette;
			for (int i = 0; i < 256; ++i) {
				int input_red = pal.colors[i].r;
				int input_green = pal.colors[i].g;
				int input_blue = pal.colors[i].b;
				
				colors[i].r = std::min<int>(255, (input_red * .393) + (input_green *.769) + (input_blue * .189));
				colors[i].g = std::min<int>(255, (input_red * .349) + (input_green *.686) + (input_blue * .168));
				colors[i].b = std::min<int>(255, (input_red * .272) + (input_green *.534) + (input_blue * .131));
			}
			SDL_SetColors(Surface, &colors[0], 0, 256);
			break;
		}
		case 4: {
			Uint32 *p;
			for (int i = 0; i < Height; ++i) {
				for (int j = 0; j < Width; ++j) {
					p = (Uint32 *)(Surface->pixels) + i * Width + j * bpp;
					
					int input_red = (*p);
					int input_green = *(p + 1);
					int input_blue = *(p + 2);
					
					const Uint32 sepia = ((Uint8)(std::min<int>(255, (input_red * .393) + (input_green *.769) + (input_blue * .189))) >> f->Rshift) +
										((Uint8)(std::min<int>(255, (input_red * .349) + (input_green *.686) + (input_blue * .168))) >> f->Gshift) +
										((Uint8)(std::min<int>(255, (input_red * .272) + (input_green *.534) + (input_blue * .131))) >> f->Bshift) +
										((Uint8)(*(p + 3)) >> f->Ashift);
					*p = sepia;
				}
			}
			break;
		}
	}
	SDL_UnlockSurface(Surface);
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
	if (Surface) {
		return;
	}

	// TODO: More formats?
	if (LoadGraphicPNG(this) == -1) {
		fprintf(stderr, "Can't load the graphic '%s'\n", File.c_str());
		ExitFatal(-1);
	}

	if (Surface->format->BytesPerPixel == 1) {
		VideoPaletteListAdd(Surface);
	}

	if (!Width) {
		Width = GraphicWidth;
	}
	if (!Height) {
		Height = GraphicHeight;
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
			ApplySepiaScale(Surface, Width, Height);
		} else {
			ApplyGrayScale(Surface, Width, Height);
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

		FreeSurface(&g->Surface);
		delete[] g->frame_map;
		g->frame_map = nullptr;

#if defined(USE_OPENGL) || defined(USE_GLES)
		if (!UseOpenGL)
#endif
		{
			FreeSurface(&g->SurfaceFlip);
			delete[] g->frameFlip_map;
			g->frameFlip_map = nullptr;
			
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

/**
**  Flip graphic and store in graphic->SurfaceFlip
*/
void CGraphic::Flip()
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		return;
	}
#endif
	if (SurfaceFlip) {
		return;
	}

	SDL_Surface *s = SurfaceFlip = SDL_ConvertSurface(Surface, Surface->format, SDL_SWSURFACE);
	if (Surface->flags & SDL_SRCCOLORKEY) {
		SDL_SetColorKey(SurfaceFlip, SDL_SRCCOLORKEY | SDL_RLEACCEL, Surface->format->colorkey);
	}
	if (SurfaceFlip->format->BytesPerPixel == 1) {
		VideoPaletteListAdd(SurfaceFlip);
	}
	SDL_LockSurface(Surface);
	SDL_LockSurface(s);
	switch (s->format->BytesPerPixel) {
		case 1:
			for (int i = 0; i < s->h; ++i) {
				for (int j = 0; j < s->w; ++j) {
					((char *)s->pixels)[j + i * s->pitch] =
						((char *)Surface->pixels)[s->w - j - 1 + i * Surface->pitch];
				}
			}
			break;
		case 3:
			for (int i = 0; i < s->h; ++i) {
				for (int j = 0; j < s->w; ++j) {
					memcpy(&((char *)s->pixels)[j + i * s->pitch],
						   &((char *)Surface->pixels)[(s->w - j - 1) * 3 + i * Surface->pitch], 3);
				}
			}
			break;
		//Wyrmgus start
//		case 4: {
		case 4:
		//Wyrmgus end
			//Wyrmgus start
			for (int i = 0; i < s->h; ++i) {
				for (int j = 0; j < s->w; ++j) {
					*(Uint32 *)&((Uint8 *)s->pixels)[((s->w - j - 1) + i * s->w) * 4] = *(Uint32 *)&((Uint8 *)Surface->pixels)[j * 4 + i * Surface->pitch];
				}
			}
			break;
			/*
			unsigned int p0 = s->pitch;
			unsigned int p1 = Surface->pitch;
			const int width = s->w;
			int j = 0;
			for (int i = 0; i < s->h; ++i) {
#ifdef _MSC_VER
				for (j = 0; j < width; ++j) {
					*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
						*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
				}
#else
				int n = (width + 7) / 8;
				switch (width & 7) {
					case 0: do {
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 7:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 6:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 5:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 4:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 3:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 2:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						case 1:
							*(Uint32 *)&((char *)s->pixels)[j * 4 + p0] =
								*(Uint32 *) & ((char *)Surface->pixels)[(width - j - 1) * 4 + p1];
							j++;
						} while (--n > 0);
				}
#endif
				p0 += s->pitch;
				p1 += Surface->pitch;
			}
		}
		break;
		*/
		//Wyrmgus end
	}
	SDL_UnlockSurface(Surface);
	SDL_UnlockSurface(s);

	delete[] frameFlip_map;

	frameFlip_map = new frame_pos_t[NumFrames];

	for (int frame = 0; frame < NumFrames; ++frame) {
		frameFlip_map[frame].x = (SurfaceFlip->w - (frame % (SurfaceFlip->w /
															 Width)) * Width) - Width;
		frameFlip_map[frame].y = (frame / (SurfaceFlip->w / Width)) * Height;
	}
}

/**
**  Convert the SDL surface to the display format
*/
void CGraphic::UseDisplayFormat()
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) { return; }
#endif

	SDL_Surface *s = Surface;

	if (s->format->Amask != 0) {
		Surface = SDL_DisplayFormatAlpha(s);
	} else {
		Surface = SDL_DisplayFormat(s);
	}
	VideoPaletteListRemove(s);
	SDL_FreeSurface(s);

	if (SurfaceFlip) {
		s = SurfaceFlip;
		if (s->format->Amask != 0) {
			SurfaceFlip = SDL_DisplayFormatAlpha(s);
		} else {
			SurfaceFlip = SDL_DisplayFormat(s);
		}
		VideoPaletteListRemove(s);
		SDL_FreeSurface(s);
	}
}

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
void MakeTextures2(CGraphic *g, GLuint texture, CUnitColors *colors,
						//Wyrmgus start
//						  int ow, int oh)
						  const int ow, const int oh, const stratagus::time_of_day *time_of_day)
						//Wyrmgus end
{
	int useckey = g->Surface->flags & SDL_SRCCOLORKEY;
	SDL_PixelFormat *f = g->Surface->format;
	int bpp = f->BytesPerPixel;
	Uint32 ckey = f->colorkey;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	int maxw = std::min<int>(g->GraphicWidth - ow, GLMaxTextureSize);
	int maxh = std::min<int>(g->GraphicHeight - oh, GLMaxTextureSize);
	int w = PowerOf2(maxw);
	int h = PowerOf2(maxh);
	unsigned char *tex = new unsigned char[w * h * 4];
	memset(tex, 0, w * h * 4);
	unsigned char alpha;
	if (g->Surface->flags & SDL_SRCALPHA) {
		alpha = f->alpha;
	} else {
		alpha = 0xff;
	}

	SDL_LockSurface(g->Surface);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	unsigned char *tp;
	const unsigned char *sp;
	Uint32 b;
	Uint32 c;
	Uint32 pc;
	
	//Wyrmgus start
	int found_player_color = -1;
	//Wyrmgus end
	
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

	for (int i = 0; i < maxh; ++i) {
		sp = (const unsigned char *)g->Surface->pixels + ow * bpp +
			 (oh + i) * g->Surface->pitch;
		tp = tex + i * w * 4;
		for (int j = 0; j < maxw; ++j) {
			if (bpp == 1) {
				if (useckey && *sp == ckey) {
					tp[3] = 0;
				} else {
					SDL_Color p = f->palette->colors[*sp];
					//Wyrmgus start
					/*
					tp[0] = p.r;
					tp[1] = p.g;
					tp[2] = p.b;
					*/
					
					int red = p.r;
					int green = p.g;
					int blue = p.b;
					
					if (colors && !g->Grayscale) {
						for (size_t k = 0; k < ConversiblePlayerColors.size(); ++k) {
							if (PlayerColorNames[ConversiblePlayerColors[k]].empty()) {
								break;
							}
							
							for (size_t z = 0; z < PlayerColorsRGB[ConversiblePlayerColors[k]].size(); ++z) {
								if (p.r == PlayerColorsRGB[ConversiblePlayerColors[k]][z].R && p.g == PlayerColorsRGB[ConversiblePlayerColors[k]][z].G && p.b == PlayerColorsRGB[ConversiblePlayerColors[k]][z].B) {
									red = colors->Colors[z].R;
									green = colors->Colors[z].G;
									blue = colors->Colors[z].B;
									
									if (found_player_color == -1) {
										found_player_color = ConversiblePlayerColors[k];
									} else if (found_player_color != ConversiblePlayerColors[k]) {
										fprintf(stderr, "\"%s\" contains tones of both \"%s\" and \"%s\" player colors.\n", g->File.c_str(), PlayerColorNames[ConversiblePlayerColors[k]].c_str(), PlayerColorNames[found_player_color].c_str());
									}
								}
							}
						}
					}
				
					tp[0] = std::max<int>(0,std::min<int>(255,int(red) + time_of_day_red));
					tp[1] = std::max<int>(0,std::min<int>(255,int(green) + time_of_day_green));
					tp[2] = std::max<int>(0,std::min<int>(255,int(blue) + time_of_day_blue));;
					//Wyrmgus end
					tp[3] = alpha;
				}
				//Wyrmgus start
				/*
				if (colors) {
					for (int z = 0; z < PlayerColorIndexCount; ++z) {
						if (*sp == PlayerColorIndexStart + z) {
							SDL_Color p = colors->Colors[z];
							tp[0] = p.r;
							tp[1] = p.g;
							tp[2] = p.b;
							tp[3] = 0xff;
							break;
						}
					}
				}
				*/
				//Wyrmgus end
				++sp;
			} else {
				if (bpp == 4) {
					c = *(Uint32 *)sp;
				} else {
					c = (sp[f->Rshift >> 3] << f->Rshift) |
						(sp[f->Gshift >> 3] << f->Gshift) |
						(sp[f->Bshift >> 3] << f->Bshift);
					c |= ((alpha | (alpha << 8) | (alpha << 16) | (alpha << 24)) ^
						  (f->Rmask | f->Gmask | f->Bmask));
				}
				*(Uint32 *)tp = c;
				//Wyrmgus start
//				if (colors) {
				if (!g->Grayscale) {
				//Wyrmgus end
					b = (c & f->Bmask) >> f->Bshift;
//					if (b && ((c & f->Rmask) >> f->Rshift) == 0 &&
//						((c & f->Gmask) >> f->Gshift) == b) {
						//Wyrmgus start
						/*
						pc = ((colors->Colors[0].R * b / 255) << f->Rshift) |
							 ((colors->Colors[0].G * b / 255) << f->Gshift) |
							 ((colors->Colors[0].B * b / 255) << f->Bshift);
						*/
						pc = ((std::max<int>(0,std::min<int>(255, (*tp) + time_of_day_red))) << f->Rshift) |
							 ((std::max<int>(0,std::min<int>(255, *(tp + 1) + time_of_day_green))) << f->Gshift) |
							 ((std::max<int>(0,std::min<int>(255, *(tp + 2) + time_of_day_blue))) << f->Bshift);
						//Wyrmgus end
						if (bpp == 4) {
							pc |= (c & f->Amask);
						} else {
							pc |= (0xFFFFFFFF ^ (f->Rmask | f->Gmask | f->Bmask));
						}
						*(Uint32 *)tp = pc;
//					}
				}
				sp += bpp;
			}
			tp += 4;
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
	SDL_UnlockSurface(g->Surface);
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
	int tw = (g->GraphicWidth - 1) / GLMaxTextureSize + 1;
	const int th = (g->GraphicHeight - 1) / GLMaxTextureSize + 1;

	int w = g->GraphicWidth % GLMaxTextureSize;
	if (w == 0) {
		w = GLMaxTextureSize;
	}
	g->TextureWidth = (GLfloat)w / PowerOf2(w);

	int h = g->GraphicHeight % GLMaxTextureSize;
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

	for (int j = 0; j < th; ++j) {
		for (int i = 0; i < tw; ++i) {
			//Wyrmgus start
//			MakeTextures2(g, textures[j * tw + i], colors, GLMaxTextureSize * i, GLMaxTextureSize * j);
			MakeTextures2(g, textures[j * tw + i], colors, GLMaxTextureSize * i, GLMaxTextureSize * j, time_of_day);
			//Wyrmgus end
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
	Assert(Surface); // can't resize before it's been loaded

	if (GraphicWidth == w && GraphicHeight == h) {
		return;
	}

	const QSize old_size = QSize(this->GraphicWidth, this->GraphicHeight);

	// Resizing the same image multiple times looks horrible
	// If the image has already been resized then get a clean copy first
	if (Resized) {
		this->SetOriginalSize();
		if (GraphicWidth == w && GraphicHeight == h) {
			return;
		}
	}

	Resized = true;
	Uint32 ckey = Surface->format->colorkey;
	int useckey = Surface->flags & SDL_SRCCOLORKEY;

	int bpp = Surface->format->BytesPerPixel;
	if (bpp == 1) {
		SDL_Color pal[256];

		SDL_LockSurface(Surface);

		unsigned char *pixels = (unsigned char *)Surface->pixels;
		unsigned char *data = new unsigned char[w * h];
		int x = 0;

		for (int i = 0; i < h; ++i) {
			for (int j = 0; j < w; ++j) {
				data[x] = pixels[(i * GraphicHeight / h) * Surface->pitch + j * GraphicWidth / w];
				++x;
			}
		}

		SDL_UnlockSurface(Surface);
		VideoPaletteListRemove(Surface);

		memcpy(pal, Surface->format->palette->colors, sizeof(SDL_Color) * 256);
		SDL_FreeSurface(Surface);

		Surface = SDL_CreateRGBSurfaceFrom(data, w, h, 8, w, 0, 0, 0, 0);
		if (Surface->format->BytesPerPixel == 1) {
			VideoPaletteListAdd(Surface);
		}
		SDL_SetPalette(Surface, SDL_LOGPAL | SDL_PHYSPAL, pal, 0, 256);
	} else {
		SDL_LockSurface(Surface);

		unsigned char *pixels = (unsigned char *)Surface->pixels;
		unsigned char *data = new unsigned char[w * h * bpp];
		int x = 0;

		for (int i = 0; i < h; ++i) {
			float fy = (float)i * GraphicHeight / h;
			int iy = (int)fy;
			fy -= iy;
			for (int j = 0; j < w; ++j) {
				float fx = (float)j * GraphicWidth / w;
				int ix = (int)fx;
				fx -= ix;
				float fz = (fx + fy) / 2;

				unsigned char *p1 = &pixels[iy * Surface->pitch + ix * bpp];
				unsigned char *p2 = (iy != Surface->h - 1) ?
									&pixels[(iy + 1) * Surface->pitch + ix * bpp] :
									p1;
				unsigned char *p3 = (ix != Surface->w - 1) ?
									&pixels[iy * Surface->pitch + (ix + 1) * bpp] :
									p1;
				unsigned char *p4 = (iy != Surface->h - 1 && ix != Surface->w - 1) ?
									&pixels[(iy + 1) * Surface->pitch + (ix + 1) * bpp] :
									p1;

				data[x * bpp + 0] = static_cast<unsigned char>(
										(p1[0] * (1 - fy) + p2[0] * fy +
										 p1[0] * (1 - fx) + p3[0] * fx +
										 p1[0] * (1 - fz) + p4[0] * fz) / 3.0 + .5);
				data[x * bpp + 1] = static_cast<unsigned char>(
										(p1[1] * (1 - fy) + p2[1] * fy +
										 p1[1] * (1 - fx) + p3[1] * fx +
										 p1[1] * (1 - fz) + p4[1] * fz) / 3.0 + .5);
				data[x * bpp + 2] = static_cast<unsigned char>(
										(p1[2] * (1 - fy) + p2[2] * fy +
										 p1[2] * (1 - fx) + p3[2] * fx +
										 p1[2] * (1 - fz) + p4[2] * fz) / 3.0 + .5);
				if (bpp == 4) {
					data[x * bpp + 3] = static_cast<unsigned char>(
											(p1[3] * (1 - fy) + p2[3] * fy +
											 p1[3] * (1 - fx) + p3[3] * fx +
											 p1[3] * (1 - fz) + p4[3] * fz) / 3.0 + .5);
				}
				++x;
			}
		}

		int Rmask = Surface->format->Rmask;
		int Gmask = Surface->format->Gmask;
		int Bmask = Surface->format->Bmask;
		int Amask = Surface->format->Amask;

		SDL_UnlockSurface(Surface);
		VideoPaletteListRemove(Surface);
		SDL_FreeSurface(Surface);

		Surface = SDL_CreateRGBSurfaceFrom(data, w, h, 8 * bpp, w * bpp,
										   Rmask, Gmask, Bmask, Amask);
	}
	if (useckey) {
		SDL_SetColorKey(Surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, ckey);
	}
	GraphicWidth = w;
	GraphicHeight = h;

	this->Width *= w;
	this->Width /= old_size.width();
	this->Height *= h;
	this->Height /= old_size.height();

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
	Assert(Surface); // can't resize before it's been loaded

	if (!Resized) {
		return;
	}

	
	if (Surface) {
		FreeSurface(&Surface);
		Surface = nullptr;
	}
	delete[] frame_map;
	frame_map = nullptr;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		if (SurfaceFlip) {
			FreeSurface(&SurfaceFlip);
			SurfaceFlip = nullptr;
		}
		delete[] frameFlip_map;
		frameFlip_map = nullptr;
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL && Textures) {
		glDeleteTextures(NumTextures, Textures);
		delete[] Textures;
		Textures = nullptr;
	}
#endif

	this->Width = this->Height = 0;
	this->Surface = nullptr;
	this->Load();

	Resized = false;
}

//Wyrmgus start
/**
**  Set a graphic's time of day
**
**  @param time  New time of day of graphic.
*/
SDL_Surface *CGraphic::SetTimeOfDay(const stratagus::time_of_day *time_of_day, bool flipped)
{
	Assert(Surface);

	if (this->Grayscale || !time_of_day) {
		if (flipped && SurfaceFlip) {
			return SurfaceFlip;
		} else {
			return Surface;
		}
	} else if (time_of_day->is_dawn()) {
		if (flipped) {
			if (DawnSurfaceFlip) {
				return DawnSurfaceFlip;
			}
		} else {
			if (DawnSurface) {
				return DawnSurface;
			}
		}
	} else if (time_of_day->is_dusk()) {
		if (flipped) {
			if (DuskSurfaceFlip) {
				return DuskSurfaceFlip;
			}
		} else {
			if (DuskSurface) {
				return DuskSurface;
			}
		}
	} else if (time_of_day->is_night()) {
		if (flipped) {
			if (NightSurfaceFlip) {
				return NightSurfaceFlip;
			}
		} else {
			if (NightSurface) {
				return NightSurface;
			}
		}
	} else {
		if (flipped) {
			return SurfaceFlip;
		} else {
			return Surface;
		}
	}

	SDL_Surface *surface = nullptr;
	SDL_Surface *base_surface = flipped ? SurfaceFlip : Surface;
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;

	if (time_of_day->is_dawn()) {
		if (flipped) {
			surface = DawnSurfaceFlip = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = DawnSurface = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
		time_of_day_red = -20;
		time_of_day_green = -20;
		time_of_day_blue = 0;
	} else if (time_of_day->is_dusk()) {
		if (flipped) {
			surface = DuskSurfaceFlip = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = DuskSurface = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
		time_of_day_red = 0;
		time_of_day_green = -20;
		time_of_day_blue = -20;
	} else if (time_of_day->is_night()) {
		if (flipped) {
			surface = NightSurfaceFlip = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = NightSurface = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
		time_of_day_red = -45;
		time_of_day_green = -35;
		time_of_day_blue = -10;
	}
	
	if (time_of_day_red != 0 || time_of_day_green != 0 || time_of_day_blue != 0) {
		const int bpp = surface->format->BytesPerPixel;
		if (bpp == 1) {
			SDL_LockSurface(surface);
			SDL_Color colors[256];
			SDL_Palette &pal = *surface->format->palette;
			for (int i = 0; i < 256; ++i) {
				colors[i].r = std::max<int>(0,std::min<int>(255,int(pal.colors[i].r) + time_of_day_red));
				colors[i].g = std::max<int>(0,std::min<int>(255,int(pal.colors[i].g) + time_of_day_green));
				colors[i].b = std::max<int>(0,std::min<int>(255,int(pal.colors[i].b) + time_of_day_blue));
			}
			SDL_SetColors(surface, &colors[0], 0, 256);
			if (SurfaceFlip) {
				SDL_SetColors(SurfaceFlip, &colors[0], 0, 256);
			}
			SDL_UnlockSurface(surface);
		} else if (bpp == 4) {
			for (int y = 0; y < surface->h; ++y) {
				for (int x = 0; x < surface->w; ++x) {
					Uint32 c;
					SDL_PixelFormat *f = surface->format;
					c = *(Uint32 *)&((Uint8 *)surface->pixels)[x * bpp + y * surface->pitch];
					Uint8 red = (std::max<int>(0,std::min<int>(255, ((c & f->Rmask) >> f->Rshift) + time_of_day_red)));
					Uint8 green = (std::max<int>(0,std::min<int>(255, ((c & f->Gmask) >> f->Gshift) + time_of_day_green)));
					Uint8 blue = (std::max<int>(0,std::min<int>(255, ((c & f->Bmask) >> f->Bshift) + time_of_day_blue)));
					Uint8 alpha = ((c & f->Amask) >> f->Ashift);
					c = Video.MapRGBA(f, red, green, blue, alpha);
					*(Uint32 *)&((Uint8 *)surface->pixels)[(x + y * surface->w) * bpp] = c;
				}
			}
		}
	}
	
	return surface;
}
//Wyrmgus end

/**
**  Check if a pixel is transparent
**
**  @param x  X coordinate
**  @param y  Y coordinate
**
**  @return   True if the pixel is transparent, False otherwise
*/
bool CGraphic::TransparentPixel(int x, int y)
{
	int bpp = Surface->format->BytesPerPixel;
	if ((bpp == 1 && !(Surface->flags & SDL_SRCCOLORKEY)) || bpp == 3) {
		return false;
	}

	bool ret = false;
	SDL_LockSurface(Surface);
	unsigned char *p = (unsigned char *)Surface->pixels + y * Surface->pitch + x * bpp;
	if (bpp == 1) {
		if (*p == Surface->format->colorkey) {
			ret = true;
		}
	} else {
		bool ckey = (Surface->flags & SDL_SRCCOLORKEY) > 0;
		if (ckey && *p == Surface->format->colorkey) {
			ret = true;
		} else if (p[Surface->format->Ashift >> 3] == 255) {
			ret = true;
		}
	}
	SDL_UnlockSurface(Surface);

	return ret;
}

/**
**  Make shadow sprite
**
**  @todo FIXME: 32bpp
*/
void CGraphic::MakeShadow()
{
	SDL_Color colors[256];

	// Set all colors in the palette to black and use 50% alpha
	memset(colors, 0, sizeof(colors));

	SDL_SetPalette(Surface, SDL_LOGPAL | SDL_PHYSPAL, colors, 0, 256);
	//Wyrmgus start
//	SDL_SetAlpha(Surface, SDL_SRCALPHA | SDL_RLEACCEL, 128);
	SDL_SetAlpha(Surface, SDL_SRCALPHA | SDL_RLEACCEL, 192);
	//Wyrmgus end

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (Textures) {
			glDeleteTextures(NumTextures, Textures);
			delete[] Textures;
			Textures = nullptr;
		}
		MakeTexture(this);
	} else
#endif
	{
		if (SurfaceFlip) {
			SDL_SetPalette(SurfaceFlip, SDL_LOGPAL | SDL_PHYSPAL, colors, 0, 256);
			//Wyrmgus start
//			SDL_SetAlpha(SurfaceFlip, SDL_SRCALPHA | SDL_RLEACCEL, 128);
			SDL_SetAlpha(SurfaceFlip, SDL_SRCALPHA | SDL_RLEACCEL, 192);
			//Wyrmgus end
		}
	}
}

void FreeGraphics()
{
	std::map<std::string, CGraphic *>::iterator i;
	while (!GraphicHash.empty()) {
		i = GraphicHash.begin();
		CGraphic::Free((*i).second);
	}
}

CFiller::bits_map::~bits_map()
{
	if (bstore) {
		free(bstore);
		bstore = nullptr;
	}
	Width = 0;
	Height = 0;
}

void CFiller::bits_map::Init(CGraphic *g)
{
	SDL_Surface *s = g->Surface;
	int bpp = s->format->BytesPerPixel;

	if (bstore) {
		free(bstore);
		bstore = nullptr;
		Width = 0;
		Height = 0;
	}

	if ((bpp == 1 && !(s->flags & SDL_SRCCOLORKEY)) || bpp == 3) {
		return;
	}

	Width = g->Width;
	Height = g->Height;

	size_t line = (Width + (sizeof(int) * 8) - 1) / (sizeof(int) * 8);
	size_t size = line * Height;

	bstore = (unsigned int *)calloc(size, sizeof(unsigned int));

	SDL_LockSurface(s);

	switch (s->format->BytesPerPixel) {
		case 1: {
			int ckey = s->format->colorkey;
			unsigned char *ptr = (unsigned char *)s->pixels;

			for (int i = 0; i < Height; ++i) {
				int l = i * Width;
				int lm = i * line;
				int k = 0;
				int p = 0;
				for (int j = 0; j < Width; ++j) {
					bstore[lm + k] |= ((ptr[j + l] != ckey) ? (1 << p) : 0);
					if (++p > 31) {
						p = 0;
						k++;
					}
				}
			}
			break;
		}
		case 2:
		case 3:
			break;
		case 4:
			if ((s->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY) {
				unsigned int ckey = s->format->colorkey;
				unsigned int *ptr = (unsigned int *)s->pixels;

				for (int i = 0; i < Height; ++i) {
					int l = i * Width;
					int lm = i * line;
					int k = 0;
					int p = 0;
					for (int j = 0; j < Width; ++j) {
						bstore[lm + k] |= ((ptr[j + l] != ckey) ? (1 << p) : 0);
						if (++p > 31) {
							p = 0;
							k++;
						}
					}
				}
			} else {
				unsigned int *ptr = (unsigned int *)s->pixels;

				for (int i = 0; i < Height; ++i) {
					int l = i * Width;
					int lm = i * line;
					int k = 0;
					int p = 0;
					for (int j = 0; j < Width; ++j) {
						bstore[lm + k] |= ((ptr[j + l] & AMASK) ? (1 << p) : 0);
						if (++p > 31) {
							p = 0;
							k++;
						}
					}
				}
			}
			break;
		default:
			break;
	}

	SDL_UnlockSurface(s);
}

void CFiller::Load()
{
	if (G) {
		G->Load();
		map.Init(G);
		G->UseDisplayFormat();
	}
	//Wyrmgus start
	if (this->X < 0) {
		this->X = Video.Width + this->X;
	}
	
	if (this->Y < 0) {
		this->Y = Video.Height + this->Y;
	}
	//Wyrmgus end
}
