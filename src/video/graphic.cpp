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
//      (c) Copyright 1999-2011 by Lutz Sammer, Nehal Mistry, Jimmy Salmon and
//                                 Pali Rohár
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include <string>
#include <map>
#include <list>

//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "video.h"
#include "player.h"
#include "intern_video.h"
#include "iocompat.h"
#include "iolib.h"
//Wyrmgus start
#include "results.h"
//Wyrmgus end
#include "ui.h"
//Wyrmgus start
#include "unit.h" //for using CPreference
#include "xbrz.h"
//Wyrmgus end

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

		unsigned char *src_pixels = NULL;

		if (src->flags & SDL_PREALLOC) {
			src_pixels = (unsigned char *)src->pixels;
		}
		SDL_FreeSurface(src);
		delete[] src_pixels;
		src = NULL;

		unsigned char *dst_pixels = NULL;

		if (dst->flags & SDL_PREALLOC) {
			dst_pixels = (unsigned char *)dst->pixels;
		}
		SDL_FreeSurface(dst);
		delete[] dst_pixels;
		dst = NULL;
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
void CPlayerColorGraphic::DrawPlayerColorSub(int player, int gx, int gy, int w, int h, int x, int y, int skin_color, int hair_color)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (!PlayerColorTextures[player][skin_color][hair_color]) {
			MakePlayerColorTexture(this, player, NoTimeOfDay, skin_color, hair_color);
		}
		DrawTexture(this, PlayerColorTextures[player][skin_color][hair_color], gx, gy, gx + w, gy + h, x, y, 0);
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
		if (!PlayerColorSurfaces[player][skin_color][hair_color]) {
			MakePlayerColorSurface(player, true, NoTimeOfDay, skin_color, hair_color);
		}
		SDL_BlitSurface(PlayerColorSurfaces[player][skin_color][hair_color], &srect, TheScreen, &drect);
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
void CPlayerColorGraphic::DrawPlayerColorSubClip(int player, int gx, int gy, int w, int h, int x, int y, int skin_color, int hair_color)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawPlayerColorSub(player, gx + x - oldx, gy + y - oldy, w, h, x, y, skin_color, hair_color);
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
							   unsigned frame, int x, int y) const
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
				frame_map[frame].x + ox + w,
				frame_map[frame].y + oy + h, x, y, 0);
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
void CGraphic::DrawFrameClip(unsigned frame, int x, int y, bool ignore_time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		//Wyrmgus start
//		DoDrawFrameClip(Textures, frame, x, y);
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			DoDrawFrameClip(Textures, frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!TexturesDawn) {
				MakeTexture(this, Map.TimeOfDay[CurrentMapLayer]);
			}
			DoDrawFrameClip(TexturesDawn, frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!TexturesDusk) {
				MakeTexture(this, Map.TimeOfDay[CurrentMapLayer]);
			}
			DoDrawFrameClip(TexturesDusk, frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!TexturesNight) {
				MakeTexture(this, Map.TimeOfDay[CurrentMapLayer]);
			}
			DoDrawFrameClip(TexturesNight, frame, x, y);
		}
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
		if (!ignore_time_of_day) {
			SetTimeOfDay(Map.TimeOfDay[CurrentMapLayer]);
		}
		//Wyrmgus end
		DrawSubClip(frame_map[frame].x, frame_map[frame].y,
					//Wyrmgus start
//					Width, Height, x, y);
					Width, Height, x, y, surface);
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
void CGraphic::DrawFrameClipTrans(unsigned frame, int x, int y, int alpha, bool ignore_time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		//Wyrmgus start
//		DrawFrameClip(frame, x, y);
		DrawFrameClip(frame, x, y, ignore_time_of_day);
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
		if (!ignore_time_of_day) {
			SetTimeOfDay(Map.TimeOfDay[CurrentMapLayer]);
		}
		//Wyrmgus end
		DrawSubClipTrans(frame_map[frame].x, frame_map[frame].y,
						 //Wyrmgus start
//						 Width, Height, x, y, alpha);
						 Width, Height, x, y, alpha, surface);
						 //Wyrmgus end
	}
}

//Wyrmgus start
void CPlayerColorGraphic::MakePlayerColorSurface(int player_color, bool flipped, int time_of_day, int skin_color, int hair_color)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		return;
	}
#endif
	SDL_Surface *surface = NULL;
	if (time_of_day == 1) {
		if (flipped) {
			surface = PlayerColorSurfacesDawnFlip[player_color][skin_color][hair_color];
		} else {
			surface = PlayerColorSurfacesDawn[player_color][skin_color][hair_color];
		}
	} else if (time_of_day == 5) {
		if (flipped) {
			surface = PlayerColorSurfacesDuskFlip[player_color][skin_color][hair_color];
		} else {
			surface = PlayerColorSurfacesDusk[player_color][skin_color][hair_color];
		}
	} else if (time_of_day == 6 || time_of_day == 7 || time_of_day == 8) {
		if (flipped) {
			surface = PlayerColorSurfacesNightFlip[player_color][skin_color][hair_color];
		} else {
			surface = PlayerColorSurfacesNight[player_color][skin_color][hair_color];
		}
	} else {
		if (flipped) {
			surface = PlayerColorSurfacesFlip[player_color][skin_color][hair_color];
		} else {
			surface = PlayerColorSurfaces[player_color][skin_color][hair_color];
		}
	}

	if (surface) {
		return;
	}
	
	SDL_Surface *base_surface = flipped ? SurfaceFlip : Surface;
	
	if (time_of_day == 1) {
		if (flipped) {
			surface = PlayerColorSurfacesDawnFlip[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfacesDawn[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	} else if (time_of_day == 5) {
		if (flipped) {
			surface = PlayerColorSurfacesDuskFlip[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfacesDusk[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	} else if (time_of_day == 6 || time_of_day == 7 || time_of_day == 8) {
		if (flipped) {
			surface = PlayerColorSurfacesNightFlip[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfacesNight[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	} else {
		if (flipped) {
			surface = PlayerColorSurfacesFlip[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		} else {
			surface = PlayerColorSurfaces[player_color][skin_color][hair_color] = SDL_ConvertSurface(base_surface, base_surface->format, SDL_SWSURFACE);
		}
	}

	if (base_surface->flags & SDL_SRCCOLORKEY) {
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, base_surface->format->colorkey);
	}
	if (surface->format->BytesPerPixel == 1) {
		VideoPaletteListAdd(surface);
	}
	
	int found_player_color = -1;
	int found_skin_color = -1;
	int found_hair_color = -1;
	
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (!this->Grayscale) { // don't alter the colors of grayscale graphics
		if (time_of_day == 1) { // dawn
			time_of_day_red = -20;
			time_of_day_green = -20;
			time_of_day_blue = 0;
		} else if (time_of_day == 2) { // morning
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (time_of_day == 3) { // midday
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (time_of_day == 4) { // afternoon
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (time_of_day == 5) { // dusk
			time_of_day_red = 0;
			time_of_day_green = -20;
			time_of_day_blue = -20;
		} else if (time_of_day == 6) { // first watch
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
		} else if (time_of_day == 7) { // midnight
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
		} else if (time_of_day == 8) { // second watch
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
				
				if (skin_color != 0 && !this->Grayscale) {
					for (int k = 1; k < SkinColorMax; ++k) {
						if (SkinColorNames[k].empty()) {
							break;
						}
						if (k == skin_color) {
							continue;
						}
							
						for (size_t z = 0; z < SkinColorsRGB[k].size(); ++z) {
							if (pal.colors[i].r == SkinColorsRGB[k][z].R && pal.colors[i].g == SkinColorsRGB[k][z].G && pal.colors[i].b == SkinColorsRGB[k][z].B) {
								red = SkinColorsRGB[skin_color][z].R;
								green = SkinColorsRGB[skin_color][z].G;
								blue = SkinColorsRGB[skin_color][z].B;
								
								if (found_skin_color == -1) {
									found_skin_color = k;
								} else if (found_skin_color != k) {
									fprintf(stderr, "\"%s\" contains tones of both \"%s\" and \"%s\" skin colors.\n", this->File.c_str(), SkinColorNames[k].c_str(), SkinColorNames[found_skin_color].c_str());
								}
							}
						}
					}
				}
					
				if (hair_color != 0 && !this->Grayscale) {
					for (int k = 1; k < HairColorMax; ++k) {
						if (HairColorNames[k].empty()) {
							break;
						}
						if (k == hair_color) {
							continue;
						}
							
						for (size_t z = 0; z < HairColorsRGB[k].size(); ++z) {
							if (pal.colors[i].r == HairColorsRGB[k][z].R && pal.colors[i].g == HairColorsRGB[k][z].G && pal.colors[i].b == HairColorsRGB[k][z].B) {
								red = HairColorsRGB[hair_color][z].R;
								green = HairColorsRGB[hair_color][z].G;
								blue = HairColorsRGB[hair_color][z].B;
								
								if (found_hair_color == -1) {
									found_hair_color = k;
								} else if (found_hair_color != k) {
									fprintf(stderr, "\"%s\" contains tones of both \"%s\" and \"%s\" hair colors.\n", this->File.c_str(), HairColorNames[k].c_str(), HairColorNames[found_hair_color].c_str());
								}
							}
						}
					}
				}
				
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
												   int x, int y, bool ignore_time_of_day, int skin_color, int hair_color)
//Wyrmgus end
{
	//Wyrmgus start
	if (!(GrandStrategy && !GameRunning && GameResult == GameNoResult)) {
		for (int i = 0; i < PlayerColorMax; ++i) {
			if (PlayerColors[i][0] == Players[player].Color) {
				player = i;
				break;
			}		
		}
	}
	//if in grand strategy mode, then treat the "player" variable as the player color index (necessary for drawing a player color graphic outside of a game)
	
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
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorTextures[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, NoTimeOfDay, skin_color, hair_color);
			}
			DoDrawFrameClip(PlayerColorTextures[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorTexturesDawn[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			DoDrawFrameClip(PlayerColorTexturesDawn[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorTexturesDusk[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			DoDrawFrameClip(PlayerColorTexturesDusk[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorTexturesNight[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			DoDrawFrameClip(PlayerColorTexturesNight[player][skin_color][hair_color], frame, x, y);
		}
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(Players[player], *this);

		SDL_Surface *surface = NULL;
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorSurfaces[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, NoTimeOfDay, skin_color, hair_color);
			}
			surface = PlayerColorSurfaces[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorSurfacesDawn[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDawn[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorSurfacesDusk[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDusk[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorSurfacesNight[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesNight[player][skin_color][hair_color];
		}
		
//		DrawFrameClip(frame, x, y);
		DrawFrameClip(frame, x, y, true, surface);
		//Wyrmgus end
	}
}

//Wyrmgus start
void CPlayerColorGraphic::DrawPlayerColorFrameClipTrans(int player, unsigned frame, int x, int y, int alpha, bool ignore_time_of_day, int skin_color, int hair_color)
{
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == Players[player].Color) {
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
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorTextures[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, NoTimeOfDay, skin_color, hair_color);
			}
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorTexturesDawn[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorTexturesDusk[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorTexturesNight[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
		}
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		//Wyrmgus start
//		DoDrawFrameClip(PlayerColorTextures[player], frame, x, y);
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			DoDrawFrameClip(PlayerColorTextures[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			DoDrawFrameClip(PlayerColorTexturesDawn[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			DoDrawFrameClip(PlayerColorTexturesDusk[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			DoDrawFrameClip(PlayerColorTexturesNight[player][skin_color][hair_color], frame, x, y);
		}
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(Players[player], *this);

		SDL_Surface *surface = NULL;
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorSurfaces[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, NoTimeOfDay, skin_color, hair_color);
			}
			surface = PlayerColorSurfaces[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorSurfacesDawn[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDawn[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorSurfacesDusk[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDusk[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorSurfacesNight[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, false, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesNight[player][skin_color][hair_color];
		}
		
//		DrawFrameClipTrans(frame, x, y, alpha);
		DrawFrameClipTrans(frame, x, y, alpha, true, surface);
		//Wyrmgus end
	}
}

//Wyrmgus start
//void CPlayerColorGraphic::DrawPlayerColorFrameClipTransX(int player, unsigned frame, int x, int y, int alpha)
void CPlayerColorGraphic::DrawPlayerColorFrameClipTransX(int player, unsigned frame, int x, int y, int alpha, bool ignore_time_of_day, int skin_color, int hair_color)
//Wyrmgus end
{
	//Wyrmgus start
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == Players[player].Color) {
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
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorTextures[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, NoTimeOfDay, skin_color, hair_color);
			}
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorTexturesDawn[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorTexturesDusk[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorTexturesNight[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
		}
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		//Wyrmgus start
//		DoDrawFrameClipX(PlayerColorTextures[player], frame, x, y);
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			DoDrawFrameClipX(PlayerColorTextures[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			DoDrawFrameClipX(PlayerColorTexturesDawn[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			DoDrawFrameClipX(PlayerColorTexturesDusk[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			DoDrawFrameClipX(PlayerColorTexturesNight[player][skin_color][hair_color], frame, x, y);
		}
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(Players[player], *this);

		SDL_Surface *surface = NULL;
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorSurfacesFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, NoTimeOfDay, skin_color, hair_color);
			}
			surface = PlayerColorSurfacesFlip[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorSurfacesDawnFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDawnFlip[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorSurfacesDuskFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDuskFlip[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorSurfacesNightFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesNightFlip[player][skin_color][hair_color];
		}
		
//		DrawFrameClipTransX(frame, x, y, alpha);
		DrawFrameClipTransX(frame, x, y, alpha, true, surface);
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
void CGraphic::DrawFrameClipX(unsigned frame, int x, int y, bool ignore_time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		//Wyrmgus start
		//DoDrawFrameClipX(Textures, frame, x, y);
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			DoDrawFrameClipX(Textures, frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!TexturesDawn) {
				MakeTexture(this, Map.TimeOfDay[CurrentMapLayer]);
			}
			DoDrawFrameClipX(TexturesDawn, frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!TexturesDusk) {
				MakeTexture(this, Map.TimeOfDay[CurrentMapLayer]);
			}
			DoDrawFrameClipX(TexturesDusk, frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!TexturesNight) {
				MakeTexture(this, Map.TimeOfDay[CurrentMapLayer]);
			}
			DoDrawFrameClipX(TexturesNight, frame, x, y);
		}
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
		if (!ignore_time_of_day) {
			SetTimeOfDay(Map.TimeOfDay[CurrentMapLayer]);
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
void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, bool ignore_time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4ub(255, 255, 255, alpha);
		//Wyrmgus start
//		DrawFrameClipX(frame, x, y);
		DrawFrameClipX(frame, x, y, ignore_time_of_day);
		//Wyrmgus end
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	} else
#endif
	{
		//Wyrmgus start
		if (!ignore_time_of_day) {
			SetTimeOfDay(Map.TimeOfDay[CurrentMapLayer]);
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
												   int x, int y, bool ignore_time_of_day, int skin_color, int hair_color)
//Wyrmgus end
{
	//Wyrmgus start
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColors[i][0] == Players[player].Color) {
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
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorTextures[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, NoTimeOfDay, skin_color, hair_color);
			}
			DoDrawFrameClipX(PlayerColorTextures[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorTexturesDawn[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			DoDrawFrameClipX(PlayerColorTexturesDawn[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorTexturesDusk[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			DoDrawFrameClipX(PlayerColorTexturesDusk[player][skin_color][hair_color], frame, x, y);
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorTexturesNight[player][skin_color][hair_color]) {
				MakePlayerColorTexture(this, player, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			DoDrawFrameClipX(PlayerColorTexturesNight[player][skin_color][hair_color], frame, x, y);
		}
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
//		GraphicPlayerPixels(Players[player], *this);

		SDL_Surface *surface = NULL;
		if (ignore_time_of_day || !Map.TimeOfDay[CurrentMapLayer] || Map.TimeOfDay[CurrentMapLayer] == MorningTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MiddayTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == AfternoonTimeOfDay) {
			if (!PlayerColorSurfacesFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, NoTimeOfDay, skin_color, hair_color);
			}
			surface = PlayerColorSurfacesFlip[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DawnTimeOfDay) {
			if (!PlayerColorSurfacesDawnFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDawnFlip[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == DuskTimeOfDay) {
			if (!PlayerColorSurfacesDuskFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesDuskFlip[player][skin_color][hair_color];
		} else if (Map.TimeOfDay[CurrentMapLayer] == FirstWatchTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == MidnightTimeOfDay || Map.TimeOfDay[CurrentMapLayer] == SecondWatchTimeOfDay) {
			if (!PlayerColorSurfacesNightFlip[player][skin_color][hair_color]) {
				MakePlayerColorSurface(player, true, Map.TimeOfDay[CurrentMapLayer], skin_color, hair_color);
			}
			surface = PlayerColorSurfacesNightFlip[player][skin_color][hair_color];
		}
		
//		DrawFrameClipX(frame, x, y);
		DrawFrameClipX(frame, x, y, true, surface);
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
	if (g == NULL) {
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
	if (g == NULL) {
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
	CPlayerColorGraphic *g = CPlayerColorGraphic::ForceNew(this->File, this->Width, this->Height);

	if (this->IsLoaded()) {
		g->Load(grayscale);
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
		return NULL;
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
		return NULL;
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
		Assert(Surface != NULL);
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
static int map_terrains[8192][4096];

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
				map_terrains[i][j] = 0;
			} else {
				map_terrains[i][j] = 6;
			}
		}
	}
	SDL_UnlockSurface(Surface);
	
	FileWriter *fw = NULL;
	std::string map_filename = "scripts/map_templates/earth.map";

	try {
		fw = CreateFileWriter(map_filename);

		for (int y = 0; y < Height; ++y) {
			for (int x = 0; x < Width; ++x) {
				fw->printf("%d,", map_terrains[x][y]);
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
void CGraphic::Load(bool grayscale)
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
	
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		MakeTexture(this);
		Graphics.push_back(this);
	}
#endif
	GenFramesMap();
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

	unsigned char *pixels = NULL;

	if ((*surface)->flags & SDL_PREALLOC) {
		pixels = (unsigned char *)(*surface)->pixels;
	}

	SDL_FreeSurface(*surface);
	delete[] pixels;
	*surface = NULL;
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
			//Wyrmgus start
			if (g->TexturesDawn) {
				glDeleteTextures(g->NumTextures, g->TexturesDawn);
				delete[] g->TexturesDawn;
			}
			if (g->TexturesDusk) {
				glDeleteTextures(g->NumTextures, g->TexturesDusk);
				delete[] g->TexturesDusk;
			}
			if (g->TexturesNight) {
				glDeleteTextures(g->NumTextures, g->TexturesNight);
				delete[] g->TexturesNight;
			}
			//Wyrmgus end
			CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(g);
			if (cg) {
				//Wyrmgus start
//				for (int i = 0; i < PlayerMax; ++i) {
				for (int i = 0; i < PlayerColorMax; ++i) {
				//Wyrmgus end
					//Wyrmgus start
					/*
					if (cg->PlayerColorTextures[i]) {
						glDeleteTextures(cg->NumTextures, cg->PlayerColorTextures[i]);
						delete[] cg->PlayerColorTextures[i];
					}
					*/
					for (int j = 0; j < SkinColorMax; ++j) {
						if (SkinColorNames[j].empty()) {
							break;
						}
						for (int k = 0; k < HairColorMax; ++k) {
							if (HairColorNames[k].empty()) {
								break;
							}
							if (cg->PlayerColorTextures[i][j][k]) {
								glDeleteTextures(cg->NumTextures, cg->PlayerColorTextures[i][j][k]);
								delete[] cg->PlayerColorTextures[i][j][k];
							}
							if (cg->PlayerColorTexturesDawn[i][j][k]) {
								glDeleteTextures(cg->NumTextures, cg->PlayerColorTexturesDawn[i][j][k]);
								delete[] cg->PlayerColorTexturesDawn[i][j][k];
							}
							if (cg->PlayerColorTexturesDusk[i][j][k]) {
								glDeleteTextures(cg->NumTextures, cg->PlayerColorTexturesDusk[i][j][k]);
								delete[] cg->PlayerColorTexturesDusk[i][j][k];
							}
							if (cg->PlayerColorTexturesNight[i][j][k]) {
								glDeleteTextures(cg->NumTextures, cg->PlayerColorTexturesNight[i][j][k]);
								delete[] cg->PlayerColorTexturesNight[i][j][k];
							}
						}
					}
					//Wyrmgus end
				}
			}
			Graphics.remove(g);
		}
#endif

		FreeSurface(&g->Surface);
		delete[] g->frame_map;
		g->frame_map = NULL;

#if defined(USE_OPENGL) || defined(USE_GLES)
		if (!UseOpenGL)
#endif
		{
			FreeSurface(&g->SurfaceFlip);
			delete[] g->frameFlip_map;
			g->frameFlip_map = NULL;
			
			//Wyrmgus start
			CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(g);
			if (cg) {
				for (int i = 0; i < PlayerColorMax; ++i) {
					for (int j = 0; j < SkinColorMax; ++j) {
						if (SkinColorNames[j].empty()) {
							break;
						}
						for (int k = 0; k < HairColorMax; ++k) {
							if (HairColorNames[k].empty()) {
								break;
							}
							if (cg->PlayerColorSurfaces[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfaces[i][j][k]);
							}
							if (cg->PlayerColorSurfacesFlip[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesFlip[i][j][k]);
							}
							if (cg->PlayerColorSurfacesDawn[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesDawn[i][j][k]);
							}
							if (cg->PlayerColorSurfacesDawnFlip[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesDawnFlip[i][j][k]);
							}
							if (cg->PlayerColorSurfacesDusk[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesDusk[i][j][k]);
							}
							if (cg->PlayerColorSurfacesDuskFlip[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesDuskFlip[i][j][k]);
							}
							if (cg->PlayerColorSurfacesNight[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesNight[i][j][k]);
							}
							if (cg->PlayerColorSurfacesNightFlip[i][j][k]) {
								FreeSurface(&cg->PlayerColorSurfacesNightFlip[i][j][k]);
							}
						}
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
		//Wyrmgus start
		if ((*i)->TexturesDawn) {
			glDeleteTextures((*i)->NumTextures, (*i)->TexturesDawn);
		}
		if ((*i)->TexturesDusk) {
			glDeleteTextures((*i)->NumTextures, (*i)->TexturesDusk);
		}
		if ((*i)->TexturesNight) {
			glDeleteTextures((*i)->NumTextures, (*i)->TexturesNight);
		}
		//Wyrmgus end
		CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(*i);
		if (cg) {
			//Wyrmgus start
//			for (int j = 0; j < PlayerMax; ++j) {
			for (int j = 0; j < PlayerColorMax; ++j) {
			//Wyrmgus end
				//Wyrmgus start
				/*
				if (cg->PlayerColorTextures[j]) {
					glDeleteTextures(cg->NumTextures, cg->PlayerColorTextures[j]);
				}
				*/
				for (int k = 0; k < SkinColorMax; ++k) {
					if (SkinColorNames[k].empty()) {
						break;
					}
					for (int n = 0; n < HairColorMax; ++n) {
						if (HairColorNames[n].empty()) {
							break;
						}
						if (cg->PlayerColorTextures[j][k][n]) {
							glDeleteTextures(cg->NumTextures, cg->PlayerColorTextures[j][k][n]);
						}
						if (cg->PlayerColorTexturesDawn[j][k][n]) {
							glDeleteTextures(cg->NumTextures, cg->PlayerColorTexturesDawn[j][k][n]);
						}
						if (cg->PlayerColorTexturesDusk[j][k][n]) {
							glDeleteTextures(cg->NumTextures, cg->PlayerColorTexturesDusk[j][k][n]);
						}
						if (cg->PlayerColorTexturesNight[j][k][n]) {
							glDeleteTextures(cg->NumTextures, cg->PlayerColorTexturesNight[j][k][n]);
						}
					}
				}
				//Wyrmgus end
			}
		}
	}
}

/**
**  Reload OpenGL graphics
*/
void ReloadGraphics()
{
	std::list<CGraphic *>::iterator i;
	for (i = Graphics.begin(); i != Graphics.end(); ++i) {
		if ((*i)->Textures) {
			delete[](*i)->Textures;
			(*i)->Textures = NULL;
			MakeTexture(*i);
		}
		//Wyrmgus start
		if ((*i)->TexturesDawn) {
			delete[](*i)->TexturesDawn;
			(*i)->TexturesDawn = NULL;
			MakeTexture(*i, 1);
		}
		if ((*i)->TexturesDusk) {
			delete[](*i)->TexturesDusk;
			(*i)->TexturesDusk = NULL;
			MakeTexture(*i, 5);
		}
		if ((*i)->TexturesNight) {
			delete[](*i)->TexturesNight;
			(*i)->TexturesNight = NULL;
			MakeTexture(*i, 7);
		}
		//Wyrmgus end
		CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(*i);
		if (cg) {
			//Wyrmgus start
//			for (int j = 0; j < PlayerMax; ++j) {
			for (int j = 0; j < PlayerColorMax; ++j) {
			//Wyrmgus end
				//Wyrmgus start
				for (int k = 0; k < SkinColorMax; ++k) {
					if (SkinColorNames[k].empty()) {
						break;
					}
					for (int n = 0; n < HairColorMax; ++n) {
						if (HairColorNames[n].empty()) {
							break;
						}
						if (cg->PlayerColorTextures[j][k][n]) {
							delete[] cg->PlayerColorTextures[j][k][n];
							cg->PlayerColorTextures[j][k][n] = NULL;
							MakePlayerColorTexture(cg, j, NoTimeOfDay, k, n);
						}
						if (cg->PlayerColorTexturesDawn[j][k][n]) {
							delete[] cg->PlayerColorTexturesDawn[j][k][n];
							cg->PlayerColorTexturesDawn[j][k][n] = NULL;
							MakePlayerColorTexture(cg, j, 1, k, n);
						}
						if (cg->PlayerColorTexturesDusk[j][k][n]) {
							delete[] cg->PlayerColorTexturesDusk[j][k][n];
							cg->PlayerColorTexturesDusk[j][k][n] = NULL;
							MakePlayerColorTexture(cg, j, 5, k, n);
						}
						if (cg->PlayerColorTexturesNight[j][k][n]) {
							delete[] cg->PlayerColorTexturesNight[j][k][n];
							cg->PlayerColorTexturesNight[j][k][n] = NULL;
							MakePlayerColorTexture(cg, j, 7, k, n);
						}
					}
				}
				//Wyrmgus end
				//Wyrmgus start
				/*
				if (cg->PlayerColorTextures[j]) {
					delete[] cg->PlayerColorTextures[j];
					cg->PlayerColorTextures[j] = NULL;
					MakePlayerColorTexture(cg, j, skin_color, hair_color);
				}
				*/
				//Wyrmgus end
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
		//Wyrmgus start
		case 4: // doesn't work, but at least doesn't cause a crash
		//Wyrmgus end
			for (int i = 0; i < s->h; ++i) {
				for (int j = 0; j < s->w; ++j) {
					memcpy(&((char *)s->pixels)[j + i * s->pitch],
						   &((char *)Surface->pixels)[(s->w - j - 1) * 3 + i * Surface->pitch], 3);
				}
			}
			break;
		//Wyrmgus start
		/*
		case 4: {
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
//Wyrmgus start
//static void MakeTextures2(CGraphic *g, GLuint texture, CUnitColors *colors,
void MakeTextures2(CGraphic *g, GLuint texture, CUnitColors *colors,
//Wyrmgus end
						//Wyrmgus start
//						  int ow, int oh)
						  int ow, int oh, int time_of_day, int skin_color, int hair_color)
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
	int found_skin_color = -1;
	int found_hair_color = -1;
	//Wyrmgus end
	
	//Wyrmgus start
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (!g->Grayscale) { // don't alter the colors of grayscale graphics
		if (time_of_day == 1) { // dawn
			time_of_day_red = -20;
			time_of_day_green = -20;
			time_of_day_blue = 0;
		} else if (time_of_day == 2) { // morning
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (time_of_day == 3) { // midday
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (time_of_day == 4) { // afternoon
			time_of_day_red = 0;
			time_of_day_green = 0;
			time_of_day_blue = 0;
		} else if (time_of_day == 5) { // dusk
			time_of_day_red = 0;
			time_of_day_green = -20;
			time_of_day_blue = -20;
		} else if (time_of_day == 6) { // first watch
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
		} else if (time_of_day == 7) { // midnight
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
		} else if (time_of_day == 8) { // second watch
			time_of_day_red = -45;
			time_of_day_green = -35;
			time_of_day_blue = -10;
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
					
					if (skin_color != 0 && !g->Grayscale) {
						for (int k = 1; k < SkinColorMax; ++k) {
							if (SkinColorNames[k].empty()) {
								break;
							}
							if (k == skin_color) {
								continue;
							}
							
							for (size_t z = 0; z < SkinColorsRGB[k].size(); ++z) {
								if (p.r == SkinColorsRGB[k][z].R && p.g == SkinColorsRGB[k][z].G && p.b == SkinColorsRGB[k][z].B) {
									red = SkinColorsRGB[skin_color][z].R;
									green = SkinColorsRGB[skin_color][z].G;
									blue = SkinColorsRGB[skin_color][z].B;
									
									if (found_skin_color == -1) {
										found_skin_color = k;
									} else if (found_skin_color != k) {
										fprintf(stderr, "\"%s\" contains tones of both \"%s\" and \"%s\" skin colors.\n", g->File.c_str(), SkinColorNames[k].c_str(), SkinColorNames[found_skin_color].c_str());
									}
								}
							}
						}
					}
					
					if (hair_color != 0 && !g->Grayscale) {
						for (int k = 1; k < HairColorMax; ++k) {
							if (HairColorNames[k].empty()) {
								break;
							}
							if (k == hair_color) {
								continue;
							}
							
							for (size_t z = 0; z < HairColorsRGB[k].size(); ++z) {
								if (p.r == HairColorsRGB[k][z].R && p.g == HairColorsRGB[k][z].G && p.b == HairColorsRGB[k][z].B) {
									red = HairColorsRGB[hair_color][z].R;
									green = HairColorsRGB[hair_color][z].G;
									blue = HairColorsRGB[hair_color][z].B;
									
									if (found_hair_color == -1) {
										found_hair_color = k;
									} else if (found_hair_color != k) {
										fprintf(stderr, "\"%s\" contains tones of both \"%s\" and \"%s\" hair colors.\n", g->File.c_str(), HairColorNames[k].c_str(), HairColorNames[found_hair_color].c_str());
									}
								}
							}
						}
					}
					
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
static void MakeTextures(CGraphic *g, int player, CUnitColors *colors, int time_of_day, int skin_color, int hair_color)
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
		//Wyrmgus start
//		textures = g->Textures = new GLuint[g->NumTextures];
//		glGenTextures(g->NumTextures, g->Textures);
		if (time_of_day == 1) {
			textures = g->TexturesDawn = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->TexturesDawn);
		} else if (time_of_day == 5) {
			textures = g->TexturesDusk = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->TexturesDusk);
		} else if (time_of_day == 6 || time_of_day == 7 || time_of_day == 8) {
			textures = g->TexturesNight = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->TexturesNight);
		} else {
			textures = g->Textures = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->Textures);
		}
		//Wyrmgus end
	//Wyrmgus start
	} else if (time_of_day == 1) {
		textures = cg->PlayerColorTexturesDawn[player][skin_color][hair_color] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->PlayerColorTexturesDawn[player][skin_color][hair_color]);
	} else if (time_of_day == 5) {
		textures = cg->PlayerColorTexturesDusk[player][skin_color][hair_color] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->PlayerColorTexturesDusk[player][skin_color][hair_color]);
	} else if (time_of_day == 6 || time_of_day == 7 || time_of_day == 8) {
		textures = cg->PlayerColorTexturesNight[player][skin_color][hair_color] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->PlayerColorTexturesNight[player][skin_color][hair_color]);
	//Wyrmgus end
	} else {
		//Wyrmgus start
//		textures = cg->PlayerColorTextures[player] = new GLuint[cg->NumTextures];
//		glGenTextures(cg->NumTextures, cg->PlayerColorTextures[player]);
		textures = cg->PlayerColorTextures[player][skin_color][hair_color] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->PlayerColorTextures[player][skin_color][hair_color]);
		//Wyrmgus end
	}

	for (int j = 0; j < th; ++j) {
		for (int i = 0; i < tw; ++i) {
			//Wyrmgus start
//			MakeTextures2(g, textures[j * tw + i], colors, GLMaxTextureSize * i, GLMaxTextureSize * j);
			MakeTextures2(g, textures[j * tw + i], colors, GLMaxTextureSize * i, GLMaxTextureSize * j, time_of_day, skin_color, hair_color);
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
void MakeTexture(CGraphic *g, int time_of_day)
//Wyrmgus end
{
	//Wyrmgus start
//	if (g->Textures) {
//		return;
//	}
	if (time_of_day == 1) {
		if (g->TexturesDawn) {
			return;
		}
	} else if (time_of_day == 5) {
		if (g->TexturesDusk) {
			return;
		}
	} else if (time_of_day == 6 || time_of_day == 7 || time_of_day == 8) {
		if (g->TexturesNight) {
			return;
		}
	} else {
		if (g->Textures) {
			return;
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	MakeTextures(g, 0, NULL);
	MakeTextures(g, 0, NULL, time_of_day, 0, 0);
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
void MakePlayerColorTexture(CPlayerColorGraphic *g, int player, int time_of_day, int skin_color, int hair_color)
//Wyrmgus end
{
	//Wyrmgus start
	/*
	if (g->PlayerColorTextures[player]) {
		return;
	}
	*/
	if (time_of_day == 1) {
		if (g->PlayerColorTexturesDawn[player][skin_color][hair_color]) {
			return;
		}
	} else if (time_of_day == 5) {
		if (g->PlayerColorTexturesDusk[player][skin_color][hair_color]) {
			return;
		}
	} else if (time_of_day == 6 || time_of_day == 7 || time_of_day == 8) {
		if (g->PlayerColorTexturesNight[player][skin_color][hair_color]) {
			return;
		}
	} else {
		if (g->PlayerColorTextures[player][skin_color][hair_color]) {
			return;
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	MakeTextures(g, player, &Players[player].UnitColors);
	CUnitColors texture_unit_colors;
	texture_unit_colors.Colors = PlayerColorsRGB[player];
	MakeTextures(g, player, &texture_unit_colors, time_of_day, skin_color, hair_color);
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
				data[x] = pixels[(i * Height / h) * Surface->pitch + j * Width / w];
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
			float fy = (float)i * Height / h;
			int iy = (int)fy;
			fy -= iy;
			for (int j = 0; j < w; ++j) {
				float fx = (float)j * Width / w;
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
	Width = GraphicWidth = w;
	Height = GraphicHeight = h;

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL && Textures) {
		glDeleteTextures(NumTextures, Textures);
		delete[] Textures;
		Textures = NULL;
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
		Surface = NULL;
	}
	delete[] frame_map;
	frame_map = NULL;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		if (SurfaceFlip) {
			FreeSurface(&SurfaceFlip);
			SurfaceFlip = NULL;
		}
		delete[] frameFlip_map;
		frameFlip_map = NULL;
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL && Textures) {
		glDeleteTextures(NumTextures, Textures);
		delete[] Textures;
		Textures = NULL;
	}
#endif

	this->Width = this->Height = 0;
	this->Surface = NULL;
	this->Load();

	Resized = false;
}

//Wyrmgus start
/**
**  Set a graphic's time of day
**
**  @param time  New time of day of graphic.
*/
void CGraphic::SetTimeOfDay(int time)
{
	Assert(Surface);

	if (TimeOfDay == time || this->Grayscale) {
		return;
	}

	// If the image has already had a time of day change, get a clean copy first
	if (TimeOfDay) {
		this->ResetTimeOfDay();
		if (TimeOfDay == time) {
			return;
		}
	}

	TimeOfDay = time;
	
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (time == 1) { // dawn
		time_of_day_red = -20;
		time_of_day_green = -20;
		time_of_day_blue = 0;
	} else if (time == 2) { // morning
		time_of_day_red = 0;
		time_of_day_green = 0;
		time_of_day_blue = 0;
	} else if (time == 3) { // midday
		time_of_day_red = 0;
		time_of_day_green = 0;
		time_of_day_blue = 0;
	} else if (time == 4) { // afternoon
		time_of_day_red = 0;
		time_of_day_green = 0;
		time_of_day_blue = 0;
	} else if (time == 5) { // dusk
		time_of_day_red = 0;
		time_of_day_green = -20;
		time_of_day_blue = -20;
	} else if (time == 6) { // first watch
		time_of_day_red = -45;
		time_of_day_green = -35;
		time_of_day_blue = -10;
	} else if (time == 7) { // midnight
		time_of_day_red = -45;
		time_of_day_green = -35;
		time_of_day_blue = -10;
	} else if (time == 8) { // second watch
		time_of_day_red = -45;
		time_of_day_green = -35;
		time_of_day_blue = -10;
	}
	
	if (time && (time_of_day_red != 0 || time_of_day_green != 0 || time_of_day_blue != 0)) {
		const int bpp = Surface->format->BytesPerPixel;
		if (bpp == 1) {
			SDL_LockSurface(Surface);
			SDL_Color colors[256];
			SDL_Palette &pal = *Surface->format->palette;
			for (int i = 0; i < 256; ++i) {
				colors[i].r = std::max<int>(0,std::min<int>(255,int(pal.colors[i].r) + time_of_day_red));
				colors[i].g = std::max<int>(0,std::min<int>(255,int(pal.colors[i].g) + time_of_day_green));
				colors[i].b = std::max<int>(0,std::min<int>(255,int(pal.colors[i].b) + time_of_day_blue));
			}
			SDL_SetColors(Surface, &colors[0], 0, 256);
			if (SurfaceFlip) {
				SDL_SetColors(SurfaceFlip, &colors[0], 0, 256);
			}
			SDL_UnlockSurface(Surface);
		} else if (bpp == 4) {
		}
	}
}

/**
**  Resets time of day for a graphic
**
*/
void CGraphic::ResetTimeOfDay()
{
	Assert(Surface); // can't resize before it's been loaded

	if (!TimeOfDay) {
		return;
	}
	
	bool flip_surface = false;

	if (Surface) {
		FreeSurface(&Surface);
		Surface = NULL;
	}
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		if (SurfaceFlip) {
			flip_surface = true;
			FreeSurface(&SurfaceFlip);
			SurfaceFlip = NULL;
		}
	}

	this->Surface = NULL;
	this->Load();
	
	if (flip_surface) {
		this->Flip();
	}

	TimeOfDay = 0;	
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
			Textures = NULL;
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
		bstore = NULL;
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
		bstore = NULL;
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

//@}
