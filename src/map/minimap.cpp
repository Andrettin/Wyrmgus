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
/**@name minimap.cpp - The minimap. */
//
//      (c) Copyright 1998-2011 by Lutz Sammer and Jimmy Salmon and Pali Rohár
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

#include <string.h>

#include "stratagus.h"

#include "minimap.h"

#include "editor.h"
#include "map.h"
#include "player.h"
//Wyrmgus start
#include "tileset.h"
//Wyrmgus end
#include "unit.h"
#include "unit_manager.h"
#include "ui.h"
#include "unittype.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define MINIMAP_FAC (16 * 3)  /// integer scale factor

/// unit attacked are shown red for at least this amount of cycles
#define ATTACK_RED_DURATION (1 * CYCLES_PER_SECOND)
/// unit attacked are shown blinking for this amount of cycles
#define ATTACK_BLINK_DURATION (7 * CYCLES_PER_SECOND)

#define SCALE_PRECISION 100


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
//SDL_Surface *MinimapSurface;        /// generated minimap
//SDL_Surface *MinimapTerrainSurface; /// generated minimap terrain
std::vector<SDL_Surface *> MinimapSurface;        /// generated minimap
std::vector<SDL_Surface *> MinimapTerrainSurface; /// generated minimap terrain
//Wyrmgus end

#if defined(USE_OPENGL) || defined(USE_GLES)
//Wyrmgus start
//unsigned char *MinimapSurfaceGL;
//unsigned char *MinimapTerrainSurfaceGL;
std::vector<unsigned char *> MinimapSurfaceGL;
std::vector<unsigned char *> MinimapTerrainSurfaceGL;
//Wyrmgus end

//Wyrmgus start
//static GLuint MinimapTexture;
static std::vector<GLuint> MinimapTexture;
//Wyrmgus end
static int MinimapTextureWidth;
static int MinimapTextureHeight;
#endif

//Wyrmgus start
//static int *Minimap2MapX;                  /// fast conversion table
//static int *Minimap2MapY;                  /// fast conversion table
//static int Map2MinimapX[MaxMapWidth];      /// fast conversion table
//static int Map2MinimapY[MaxMapHeight];     /// fast conversion table
static std::vector<int *> Minimap2MapX;                  /// fast conversion table
static std::vector<int *> Minimap2MapY;                  /// fast conversion table
static std::vector<int *> Map2MinimapX;      /// fast conversion table
static std::vector<int *> Map2MinimapY;     /// fast conversion table
//Wyrmgus end

// MinimapScale:
// 32x32 64x64 96x96 128x128 256x256 512x512 ...
// *4 *2 *4/3   *1 *1/2 *1/4
//Wyrmgus start
//static int MinimapScaleX;                  /// Minimap scale to fit into window
//static int MinimapScaleY;                  /// Minimap scale to fit into window
static std::vector<int> MinimapScaleX;                  /// Minimap scale to fit into window
static std::vector<int> MinimapScaleY;                  /// Minimap scale to fit into window
//Wyrmgus end

#define MAX_MINIMAP_EVENTS 8

struct MinimapEvent {
	PixelPos pos;
	int Size;
	Uint32 Color;
} MinimapEvents[MAX_MINIMAP_EVENTS];
int NumMinimapEvents;


/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/


#if defined(USE_OPENGL) || defined(USE_GLES)
/**
**  Create the minimap texture
*/
//Wyrmgus start
//static void CreateMinimapTexture()
static void CreateMinimapTexture(int z)
//Wyrmgus end
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//Wyrmgus start
//	glGenTextures(1, &MinimapTexture);
//	glBindTexture(GL_TEXTURE_2D, MinimapTexture);
	glGenTextures(1, &MinimapTexture[z]);
	glBindTexture(GL_TEXTURE_2D, MinimapTexture[z]);
	//Wyrmgus end
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MinimapTextureWidth,
				 MinimapTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
	//Wyrmgus start
//				 MinimapSurfaceGL);
				 MinimapSurfaceGL[z]);
	//Wyrmgus end
}
#endif

/**
**  Create a mini-map from the tiles of the map.
**
**  @todo Scaling and scrolling the minmap is currently not supported.
*/
void CMinimap::Create()
{
	//Wyrmgus start
	/*
	// Scale to biggest value.
	const int n = std::max(std::max(Map.Info.MapWidth, Map.Info.MapHeight), 32);

	MinimapScaleX = (W * MINIMAP_FAC + n - 1) / n;
	MinimapScaleY = (H * MINIMAP_FAC + n - 1) / n;

	XOffset = (W - (Map.Info.MapWidth * MinimapScaleX) / MINIMAP_FAC + 1) / 2;
	YOffset = (H - (Map.Info.MapHeight * MinimapScaleY) / MINIMAP_FAC + 1) / 2;

	DebugPrint("MinimapScale %d %d (%d %d), X off %d, Y off %d\n" _C_
			   MinimapScaleX / MINIMAP_FAC _C_ MinimapScaleY / MINIMAP_FAC _C_
			   MinimapScaleX _C_ MinimapScaleY _C_
			   XOffset _C_ YOffset);

	//
	// Calculate minimap fast lookup tables.
	//
	Minimap2MapX = new int[W * H];
	memset(Minimap2MapX, 0, W * H * sizeof(int));
	Minimap2MapY = new int[W * H];
	memset(Minimap2MapY, 0, W * H * sizeof(int));
	for (int i = XOffset; i < W - XOffset; ++i) {
		Minimap2MapX[i] = ((i - XOffset) * MINIMAP_FAC) / MinimapScaleX;
	}
	for (int i = YOffset; i < H - YOffset; ++i) {
		Minimap2MapY[i] = (((i - YOffset) * MINIMAP_FAC) / MinimapScaleY) * Map.Info.MapWidth;
	}
	for (int i = 0; i < Map.Info.MapWidth; ++i) {
		Map2MinimapX[i] = (i * MinimapScaleX) / MINIMAP_FAC;
	}
	for (int i = 0; i < Map.Info.MapHeight; ++i) {
		Map2MinimapY[i] = (i * MinimapScaleY) / MINIMAP_FAC;
	}

	// Palette updated from UpdateMinimapTerrain()
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		for (MinimapTextureWidth = 1; MinimapTextureWidth < W; MinimapTextureWidth <<= 1) {
		}
		for (MinimapTextureHeight = 1; MinimapTextureHeight < H; MinimapTextureHeight <<= 1) {
		}
		MinimapTerrainSurfaceGL = new unsigned char[MinimapTextureWidth * MinimapTextureHeight * 4];
		MinimapSurfaceGL = new unsigned char[MinimapTextureWidth * MinimapTextureHeight * 4];
		memset(MinimapSurfaceGL, 0, MinimapTextureWidth * MinimapTextureHeight * 4);
		CreateMinimapTexture();
	} else
#endif
	{
		SDL_PixelFormat *f = Map.TileGraphic->Surface->format;
		//Wyrmgus start
//		MinimapTerrainSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, f->BitsPerPixel, f->Rmask, f->Gmask, f->Bmask, f->Amask);
		MinimapTerrainSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, 32, TheScreen->format->Rmask, TheScreen->format->Gmask, TheScreen->format->Bmask, 0);
		//Wyrmgus end
		MinimapSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,  W, H, 32, TheScreen->format->Rmask, TheScreen->format->Gmask, TheScreen->format->Bmask, 0);
	}

	UpdateTerrain();
	*/
#if defined(USE_OPENGL) || defined(USE_GLES)
	MinimapTexture.resize(Map.Fields.size());
#endif
	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		// Scale to biggest value.
		const int n = std::max(std::max(Map.Info.MapWidths[z], Map.Info.MapHeights[z]), 32);

		MinimapScaleX.push_back((W * MINIMAP_FAC + n - 1) / n);
		MinimapScaleY.push_back((H * MINIMAP_FAC + n - 1) / n);

		XOffset.push_back((W - (Map.Info.MapWidths[z] * MinimapScaleX[z]) / MINIMAP_FAC + 1) / 2);
		YOffset.push_back((H - (Map.Info.MapHeights[z] * MinimapScaleY[z]) / MINIMAP_FAC + 1) / 2);

		DebugPrint("MinimapScale %d %d (%d %d), X off %d, Y off %d\n" _C_
				   MinimapScaleX[z] / MINIMAP_FAC _C_ MinimapScaleY[z] / MINIMAP_FAC _C_
				   MinimapScaleX[z] _C_ MinimapScaleY[z] _C_
				   XOffset[z] _C_ YOffset[z]);

		//
		// Calculate minimap fast lookup tables.
		//
		Minimap2MapX.push_back(new int[W * H]);
		memset(Minimap2MapX[z], 0, W * H * sizeof(int));
		Minimap2MapY.push_back(new int[W * H]);
		memset(Minimap2MapY[z], 0, W * H * sizeof(int));
		for (int i = XOffset[z]; i < W - XOffset[z]; ++i) {
			Minimap2MapX[z][i] = ((i - XOffset[z]) * MINIMAP_FAC) / MinimapScaleX[z];
		}
		for (int i = YOffset[z]; i < H - YOffset[z]; ++i) {
			Minimap2MapY[z][i] = (((i - YOffset[z]) * MINIMAP_FAC) / MinimapScaleY[z]) * Map.Info.MapWidths[z];
		}
		Map2MinimapX.push_back(new int[Map.Info.MapWidths[z]]);
		memset(Map2MinimapX[z], 0, Map.Info.MapWidths[z] * sizeof(int));
		Map2MinimapY.push_back(new int[Map.Info.MapHeights[z]]);
		memset(Map2MinimapY[z], 0, Map.Info.MapHeights[z] * sizeof(int));
		for (int i = 0; i < Map.Info.MapWidths[z]; ++i) {
			Map2MinimapX[z][i] = (i * MinimapScaleX[z]) / MINIMAP_FAC;
		}
		for (int i = 0; i < Map.Info.MapHeights[z]; ++i) {
			Map2MinimapY[z][i] = (i * MinimapScaleY[z]) / MINIMAP_FAC;
		}

		// Palette updated from UpdateMinimapTerrain()
	#if defined(USE_OPENGL) || defined(USE_GLES)
		if (UseOpenGL) {
			for (MinimapTextureWidth = 1; MinimapTextureWidth < W; MinimapTextureWidth <<= 1) {
			}
			for (MinimapTextureHeight = 1; MinimapTextureHeight < H; MinimapTextureHeight <<= 1) {
			}
			MinimapTerrainSurfaceGL.push_back(new unsigned char[MinimapTextureWidth * MinimapTextureHeight * 4]);
			MinimapSurfaceGL.push_back(new unsigned char[MinimapTextureWidth * MinimapTextureHeight * 4]);
			memset(MinimapSurfaceGL[z], 0, MinimapTextureWidth * MinimapTextureHeight * 4);
			CreateMinimapTexture(z);
		} else
	#endif
		{
			SDL_PixelFormat *f = TheScreen->format;
			MinimapTerrainSurface.push_back(SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, 32, RMASK, GMASK, BMASK, AMASK));
			MinimapSurface.push_back(SDL_CreateRGBSurface(SDL_SWSURFACE,  W, H, 32, f->Rmask, f->Gmask, f->Bmask, 0));
		}

		UpdateTerrain(z);
	}
	//Wyrmgus end

	NumMinimapEvents = 0;
}

#if defined(USE_OPENGL) || defined(USE_GLES)
/**
**  Free OpenGL minimap
*/
void CMinimap::FreeOpenGL()
{
	//Wyrmgus start
//	glDeleteTextures(1, &MinimapTexture);
	for (size_t z = 0; z < MinimapTexture.size(); ++z) {
		glDeleteTextures(1, &MinimapTexture[z]);
	}
	MinimapTexture.clear();
	//Wyrmgus end
}

/**
**  Reload OpenGL minimap
*/
void CMinimap::Reload()
{
	//Wyrmgus start
//	CreateMinimapTexture();
	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		CreateMinimapTexture(z);
	}
	//Wyrmgus end
}
#endif

/**
**  Calculate the tile graphic pixel
*/
//Wyrmgus start
//static inline Uint8 *GetTileGraphicPixel(int xofs, int yofs, int mx, int my, int scalex, int scaley, int bpp)
static inline Uint8 *GetTileGraphicPixel(int xofs, int yofs, int mx, int my, int scalex, int scaley, int bpp, int z, bool use_base = false)
//Wyrmgus end
{
	//Wyrmgus start
	const CMapField &mf = Map.Fields[z][Minimap2MapX[z][mx] + Minimap2MapY[z][my]];
	CTerrainType *terrain = (mf.OverlayTerrain && !use_base) ? mf.OverlayTerrain : mf.Terrain;
	//Wyrmgus end

	//Wyrmgus start
//	Uint8 *pixels = (Uint8 *)Map.TileGraphic->Surface->pixels;
	Uint8 *pixels = (Uint8 *)terrain->Graphics->Surface->pixels;
	//Wyrmgus end
	int x = (xofs + 7 + ((mx * SCALE_PRECISION) % scalex) / SCALE_PRECISION * 8);
	int y = (yofs + 6 + ((my * SCALE_PRECISION) % scaley) / SCALE_PRECISION * 8);
	//Wyrmgus start
//	return &pixels[x * bpp + y * Map.TileGraphic->Surface->pitch];
	return &pixels[x * bpp + y * terrain->Graphics->Surface->pitch];
	//Wyrmgus end
}

/**
**  Update a mini-map from the tiles of the map.
*/
//Wyrmgus start
//void CMinimap::UpdateTerrain()
void CMinimap::UpdateTerrain(int z)
//Wyrmgus end
{
	//Wyrmgus start
//	int scalex = MinimapScaleX * SCALE_PRECISION / MINIMAP_FAC;
	int scalex = MinimapScaleX[z] * SCALE_PRECISION / MINIMAP_FAC;
	//Wyrmgus end
	if (!scalex) {
		scalex = 1;
	}
	//Wyrmgus start
//	int scaley = MinimapScaleY * SCALE_PRECISION / MINIMAP_FAC;
	int scaley = MinimapScaleY[z] * SCALE_PRECISION / MINIMAP_FAC;
	//Wyrmgus end
	if (!scaley) {
		scaley = 1;
	}
	//Wyrmgus start
//	const int bpp = Map.TileGraphic->Surface->format->BytesPerPixel;
	//Wyrmgus end

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
		/*
		if (bpp == 1) {
			SDL_SetPalette(MinimapTerrainSurface, SDL_LOGPAL,
						   Map.TileGraphic->Surface->format->palette->colors, 0, 256);
		}
		*/
		//Wyrmgus end
	}

	//Wyrmgus start
//	const int tilepitch = Map.TileGraphic->Surface->w / PixelTileSize.x;
	//Wyrmgus end

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		SDL_LockSurface(Map.TileGraphic->Surface);
		//Wyrmgus start
		for (size_t i = 0; i != TerrainTypes.size(); ++i) {
			if (TerrainTypes[i]->Graphics) {
				SDL_LockSurface(TerrainTypes[i]->Graphics->Surface);
			}
		}
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
//		SDL_LockSurface(MinimapTerrainSurface);
		SDL_LockSurface(MinimapTerrainSurface[z]);
		//Wyrmgus end
	}

	//
	//  Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
	//
	//Wyrmgus start
//	for (int my = YOffset; my < H - YOffset; ++my) {
	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
	//Wyrmgus end
		//Wyrmgus start
//		for (int mx = XOffset; mx < W - XOffset; ++mx) {
		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
		//Wyrmgus end
			//Wyrmgus start
//			const int tile = Map.Fields[Minimap2MapX[mx] + Minimap2MapY[my]].getGraphicTile();
			const CMapField &mf = Map.Fields[z][Minimap2MapX[z][mx] + Minimap2MapY[z][my]];
			CTerrainType *terrain = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlayTerrain : mf.playerInfo.SeenTerrain;
			int tile = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlaySolidTile : mf.playerInfo.SeenSolidTile;
			if (!terrain) {
				terrain = mf.OverlayTerrain ? mf.OverlayTerrain : mf.Terrain;
				tile = mf.OverlayTerrain ? mf.OverlaySolidTile : mf.SolidTile;
			}
			
			CTerrainType *base_terrain = mf.playerInfo.SeenTerrain;
			int base_tile = mf.playerInfo.SeenSolidTile;
			if (!base_terrain) {
				base_terrain = mf.Terrain;
				base_tile = mf.SolidTile;
			}
			//Wyrmgus end
			
			//Wyrmgus start
			int tilepitch = terrain->Graphics->Surface->w / PixelTileSize.x;
			const int bpp = terrain->Graphics->Surface->format->BytesPerPixel;
			
			int base_tilepitch = base_terrain->Graphics->Surface->w / PixelTileSize.x;
			//assumes the BPP for the base terrain is the same as for the top terrain (which may be an overlay)
			//Wyrmgus end
	
			const int xofs = PixelTileSize.x * (tile % tilepitch);
			const int yofs = PixelTileSize.y * (tile / tilepitch);
			
			//Wyrmgus start
			const int base_xofs = PixelTileSize.x * (base_tile % base_tilepitch);
			const int base_yofs = PixelTileSize.y * (base_tile / base_tilepitch);
			//Wyrmgus end

#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				Uint32 c;

				if (bpp == 1) {
					//Wyrmgus start
//					SDL_Color color = Map.TileGraphic->Surface->format->palette->colors[
//										  *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp)];
					SDL_Color color = terrain->Graphics->Surface->format->palette->colors[
										  *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z)];
					if (color.r == 255 && color.g == 255 && color.b == 255) { //completely white pixel, presumed to be a transparent one; use base instead
						color = base_terrain->Graphics->Surface->format->palette->colors[
										  *GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, true)];
					}
					//Wyrmgus end
					c = Video.MapRGB(0, color.r, color.g, color.b);
				} else {
					//Wyrmgus start
//					SDL_PixelFormat *f = Map.TileGraphic->Surface->format;
					SDL_PixelFormat *f = terrain->Graphics->Surface->format;
					//Wyrmgus end
					c = *(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z);
					//Wyrmgus start
					if (((c & f->Amask) >> f->Ashift) == 0) { //transparent pixel, use base instead
						f = base_terrain->Graphics->Surface->format;
						c = *(Uint32 *)GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, true);
					}
					//Wyrmgus end
					c = Video.MapRGB(0,
									 ((c & f->Rmask) >> f->Rshift),
									 ((c & f->Gmask) >> f->Gshift),
									 ((c & f->Bmask) >> f->Bshift));
				}
				//Wyrmgus start
//				*(Uint32 *)&(MinimapTerrainSurfaceGL[(mx + my * MinimapTextureWidth) * 4]) = c;
				*(Uint32 *)&(MinimapTerrainSurfaceGL[z][(mx + my * MinimapTextureWidth) * 4]) = c;
				//Wyrmgus end
			} else
#endif
			{
				if (bpp == 1) {
					//Wyrmgus start
					/*
					((Uint8 *)MinimapTerrainSurface->pixels)[mx + my * MinimapTerrainSurface->pitch] =
						*GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
					*/
					SDL_Color original_color = terrain->Graphics->Surface->format->palette->colors[
										  *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z)];

					if (original_color.r == 255 && original_color.g == 255 && original_color.b == 255) { //completely white pixel, presumed to be a transparent one; use base instead
						original_color = base_terrain->Graphics->Surface->format->palette->colors[
										  *GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, true)];
					}

					Uint32 color;
					color = Video.MapRGB(TheScreen->format, original_color.r, original_color.g, original_color.b);
					SDL_Color c;
					SDL_GetRGB(color, TheScreen->format, &c.r, &c.g, &c.b);

					*(Uint32 *)&((Uint8 *)MinimapTerrainSurface[z]->pixels)[mx * MinimapSurface[z]->format->BytesPerPixel + my * MinimapTerrainSurface[z]->pitch] = color;
					//Wyrmgus end
				} else if (bpp == 3) {
					//Wyrmgus start
//					Uint8 *d = &((Uint8 *)MinimapTerrainSurface->pixels)[mx * bpp + my * MinimapTerrainSurface->pitch];
//					Uint8 *s = GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
					Uint8 *d = &((Uint8 *)MinimapTerrainSurface[z]->pixels)[mx * bpp + my * MinimapTerrainSurface[z]->pitch];
					Uint8 *s = GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z);
					//Wyrmgus end
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
				} else {
					//Wyrmgus start
//					*(Uint32 *)&((Uint8 *)MinimapTerrainSurface->pixels)[mx * bpp + my * MinimapTerrainSurface->pitch] =
//						*(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
					*(Uint32 *)&((Uint8 *)MinimapTerrainSurface[z]->pixels)[mx * bpp + my * MinimapTerrainSurface[z]->pitch] =
						*(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z);
					//Wyrmgus end
				}
			}

		}
	}
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
//		SDL_UnlockSurface(MinimapTerrainSurface);
		SDL_UnlockSurface(MinimapTerrainSurface[z]);
		//Wyrmgus end
	}
	SDL_UnlockSurface(Map.TileGraphic->Surface);
	//Wyrmgus start
	for (size_t i = 0; i != TerrainTypes.size(); ++i) {
		if (TerrainTypes[i]->Graphics) {
			SDL_UnlockSurface(TerrainTypes[i]->Graphics->Surface);
		}
	}
	//Wyrmgus end
}

/**
**  Update a single minimap tile after a change
**
**  @param pos  The map position to update in the minimap
*/
//Wyrmgus start
//void CMinimap::UpdateXY(const Vec2i &pos)
void CMinimap::UpdateXY(const Vec2i &pos, int z)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		//Wyrmgus start
//		if (!MinimapTerrainSurfaceGL) {
		if (z >= (int) MinimapTerrainSurfaceGL.size() || !MinimapTerrainSurfaceGL[z]) {
		//Wyrmgus end
			return;
		}
	} else
#endif
	{
		//Wyrmgus start
//		if (!MinimapTerrainSurface) {
		if (z >= (int) MinimapTerrainSurface.size() || !MinimapTerrainSurface[z]) {
		//Wyrmgus end
			return;
		}
	}

	//Wyrmgus start
//	int scalex = MinimapScaleX * SCALE_PRECISION / MINIMAP_FAC;
	int scalex = MinimapScaleX[z] * SCALE_PRECISION / MINIMAP_FAC;
	//Wyrmgus end
	if (scalex == 0) {
		scalex = 1;
	}
	//Wyrmgus start
//	int scaley = MinimapScaleY * SCALE_PRECISION / MINIMAP_FAC;
	int scaley = MinimapScaleY[z] * SCALE_PRECISION / MINIMAP_FAC;
	//Wyrmgus end
	if (scaley == 0) {
		scaley = 1;
	}

	//Wyrmgus start
//	const int tilepitch = Map.TileGraphic->Surface->w / PixelTileSize.x;
//	const int bpp = Map.TileGraphic->Surface->format->BytesPerPixel;
	//Wyrmgus end

	//
	//  Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
	//
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
//		SDL_LockSurface(MinimapTerrainSurface);
		SDL_LockSurface(MinimapTerrainSurface[z]);
		//Wyrmgus end
	}
	SDL_LockSurface(Map.TileGraphic->Surface);
	//Wyrmgus start
	for (size_t i = 0; i != TerrainTypes.size(); ++i) {
		if (TerrainTypes[i]->Graphics) {
			SDL_LockSurface(TerrainTypes[i]->Graphics->Surface);
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	const int ty = pos.y * Map.Info.MapWidth;
	const int ty = pos.y * Map.Info.MapWidths[z];
	//Wyrmgus end
	const int tx = pos.x;
	//Wyrmgus start
//	for (int my = YOffset; my < H - YOffset; ++my) {
	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
	//Wyrmgus end
		//Wyrmgus start
//		const int y = Minimap2MapY[my];
		const int y = Minimap2MapY[z][my];
		//Wyrmgus end
		if (y < ty) {
			continue;
		}
		if (y > ty) {
			break;
		}

		//Wyrmgus start
//		for (int mx = XOffset; mx < W - XOffset; ++mx) {
		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
		//Wyrmgus end
			//Wyrmgus start
//			const int x = Minimap2MapX[mx];
			const int x = Minimap2MapX[z][mx];
			//Wyrmgus end

			if (x < tx) {
				continue;
			}
			if (x > tx) {
				break;
			}

			//Wyrmgus start
			/*
			int tile = Map.Fields[x + y].playerInfo.SeenTile;
			if (!tile) {
				tile = Map.Fields[x + y].getGraphicTile();
			}
			*/
			CTerrainType *terrain = Map.Fields[z][x + y].playerInfo.SeenOverlayTerrain ? Map.Fields[z][x + y].playerInfo.SeenOverlayTerrain : Map.Fields[z][x + y].playerInfo.SeenTerrain;
			int tile = Map.Fields[z][x + y].playerInfo.SeenOverlayTerrain ? Map.Fields[z][x + y].playerInfo.SeenOverlaySolidTile : Map.Fields[z][x + y].playerInfo.SeenSolidTile;
			if (!terrain) {
				terrain = Map.Fields[z][x + y].OverlayTerrain ? Map.Fields[z][x + y].OverlayTerrain : Map.Fields[z][x + y].Terrain;
				tile = Map.Fields[z][x + y].OverlayTerrain ? Map.Fields[z][x + y].OverlaySolidTile : Map.Fields[z][x + y].SolidTile;
			}
			
			CTerrainType *base_terrain = Map.Fields[z][x + y].playerInfo.SeenTerrain;
			int base_tile = Map.Fields[z][x + y].playerInfo.SeenSolidTile;
			if (!base_terrain) {
				base_terrain = Map.Fields[z][x + y].Terrain;
				base_tile = Map.Fields[z][x + y].SolidTile;
			}
			//Wyrmgus end

			//Wyrmgus start
			int tilepitch = terrain->Graphics->Surface->w / PixelTileSize.x;
			const int bpp = terrain->Graphics->Surface->format->BytesPerPixel;
			
			int base_tilepitch = base_terrain->Graphics->Surface->w / PixelTileSize.x;
			//Wyrmgus end
	
			const int xofs = PixelTileSize.x * (tile % tilepitch);
			const int yofs = PixelTileSize.y * (tile / tilepitch);
			
			//Wyrmgus start
			const int base_xofs = PixelTileSize.x * (base_tile % base_tilepitch);
			const int base_yofs = PixelTileSize.y * (base_tile / base_tilepitch);
			//Wyrmgus end

#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				Uint32 c;

				if (bpp == 1) {
					//Wyrmgus start
//					const int colorIndex = *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
//					const SDL_Color color = Map.TileGraphic->Surface->format->palette->colors[colorIndex];
					int colorIndex = *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z);
					SDL_Color color = terrain->Graphics->Surface->format->palette->colors[colorIndex];
					if (color.r == 255 && color.g == 255 && color.b == 255) { //completely white pixel, presumed to be a transparent one; use base instead
						colorIndex = *GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, true);
						color = base_terrain->Graphics->Surface->format->palette->colors[colorIndex];
					}
					//Wyrmgus end

					c = Video.MapRGB(0, color.r, color.g, color.b);
				} else {
					//Wyrmgus start
//					SDL_PixelFormat *f = Map.TileGraphic->Surface->format;
					SDL_PixelFormat *f = terrain->Graphics->Surface->format;
					//Wyrmgus end

					//Wyrmgus start
//					c = *(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
					c = *(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z);
					
					if (((c & f->Amask) >> f->Ashift) == 0) { //transparent pixel, use base instead
						f = base_terrain->Graphics->Surface->format;
						c = *(Uint32 *)GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, true);
					}
					//Wyrmgus end
					c = Video.MapRGB(0,
									 ((c & f->Rmask) >> f->Rshift),
									 ((c & f->Gmask) >> f->Gshift),
									 ((c & f->Bmask) >> f->Bshift));
				}
				//Wyrmgus start
//				*(Uint32 *)&(MinimapTerrainSurfaceGL[(mx + my * MinimapTextureWidth) * 4]) = c;
				*(Uint32 *)&(MinimapTerrainSurfaceGL[z][(mx + my * MinimapTextureWidth) * 4]) = c;
				//Wyrmgus end
			} else
#endif
			{
				//Wyrmgus start
//				const int index = mx * bpp + my * MinimapTerrainSurface->pitch;
//				Uint8 *s = GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
				const int index = mx * bpp + my * MinimapTerrainSurface[z]->pitch;
				Uint8 *s = GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z);
				//Wyrmgus end
				if (bpp == 1) {
					//Wyrmgus start
					/*
					((Uint8 *)MinimapTerrainSurface->pixels)[index] = *s;
					*/
					SDL_Color original_color = terrain->Graphics->Surface->format->palette->colors[
										  *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z)];
										  
					if (original_color.r == 255 && original_color.g == 255 && original_color.b == 255) { //completely white pixel, presumed to be a transparent one; use base instead
						original_color = base_terrain->Graphics->Surface->format->palette->colors[
										  *GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, true)];
					}

					Uint32 color;
					color = Video.MapRGB(TheScreen->format, original_color.r, original_color.g, original_color.b);
					SDL_Color c;
					SDL_GetRGB(color, TheScreen->format, &c.r, &c.g, &c.b);

					*(Uint32 *)&((Uint8 *)MinimapTerrainSurface[z]->pixels)[mx * MinimapSurface[z]->format->BytesPerPixel + my * MinimapTerrainSurface[z]->pitch] = color;
					//Wyrmgus end
				} else if (bpp == 3) {
					//Wyrmgus start
//					Uint8 *d = &((Uint8 *)MinimapTerrainSurface->pixels)[index];
					Uint8 *d = &((Uint8 *)MinimapTerrainSurface[z]->pixels)[index];
					//Wyrmgus end

					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
				} else {
					//Wyrmgus start
//					*(Uint32 *)&((Uint8 *)MinimapTerrainSurface->pixels)[index] = *(Uint32 *)s;
					*(Uint32 *)&((Uint8 *)MinimapTerrainSurface[z]->pixels)[index] = *(Uint32 *)s;
					//Wyrmgus end
				}
			}
		}
	}
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
//		SDL_UnlockSurface(MinimapTerrainSurface);
		SDL_UnlockSurface(MinimapTerrainSurface[z]);
		//Wyrmgus end
	}
	SDL_UnlockSurface(Map.TileGraphic->Surface);
	//Wyrmgus start
	for (size_t i = 0; i != TerrainTypes.size(); ++i) {
		if (TerrainTypes[i]->Graphics) {
			SDL_UnlockSurface(TerrainTypes[i]->Graphics->Surface);
		}
	}
	//Wyrmgus end
}

/**
**  Draw a unit on the minimap.
*/
static void DrawUnitOn(CUnit &unit, int red_phase)
{
	const CUnitType *type;

	if (Editor.Running || ReplayRevealMap || unit.IsVisible(*ThisPlayer)) {
		type = unit.Type;
	} else {
		type = unit.Seen.Type;
		// This will happen for radar if the unit has not been seen and we
		// have it on radar.
		if (!type) {
			type = unit.Type;
		}
	}

	//Wyrmgus start
	//don't draw decorations or diminutive fauna units on the minimap
	if (type->BoolFlag[DECORATION_INDEX].value || (type->BoolFlag[DIMINUTIVE_INDEX].value && type->BoolFlag[FAUNA_INDEX].value)) {
		return;
	}
	//Wyrmgus end

	Uint32 color;
	//Wyrmgus start
//	if (unit.Player->Index == PlayerNumNeutral) {
	if (unit.GetDisplayPlayer() == PlayerNumNeutral) {
	//Wyrmgus end
		color = Video.MapRGB(TheScreen->format, type->NeutralMinimapColorRGB);
	} else if (unit.Player == ThisPlayer && !Editor.Running) {
		if (unit.Attacked && unit.Attacked + ATTACK_BLINK_DURATION > GameCycle &&
			(red_phase || unit.Attacked + ATTACK_RED_DURATION > GameCycle)) {
			color = ColorRed;
		} else if (UI.Minimap.ShowSelected && unit.Selected) {
			color = ColorWhite;
		} else {
			color = ColorGreen;
		}
	} else {
		color = unit.Player->Color;
	}

	//Wyrmgus start
//	int mx = 1 + UI.Minimap.XOffset + Map2MinimapX[unit.tilePos.x];
//	int my = 1 + UI.Minimap.YOffset + Map2MinimapY[unit.tilePos.y];
//	int w = Map2MinimapX[type->TileWidth];
	int mx = 1 + UI.Minimap.XOffset[CurrentMapLayer] + Map2MinimapX[CurrentMapLayer][unit.tilePos.x];
	int my = 1 + UI.Minimap.YOffset[CurrentMapLayer] + Map2MinimapY[CurrentMapLayer][unit.tilePos.y];
	int w = Map2MinimapX[CurrentMapLayer][type->TileWidth];
	//Wyrmgus end
	if (mx + w >= UI.Minimap.W) { // clip right side
		w = UI.Minimap.W - mx;
	}
	//Wyrmgus start
//	int h0 = Map2MinimapY[type->TileHeight];
	int h0 = Map2MinimapY[CurrentMapLayer][type->TileHeight];
	//Wyrmgus end
	if (my + h0 >= UI.Minimap.H) { // clip bottom side
		h0 = UI.Minimap.H - my;
	}
	int bpp = 0;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_Color c;
		//Wyrmgus start
//		bpp = MinimapSurface->format->BytesPerPixel;
		bpp = MinimapSurface[CurrentMapLayer]->format->BytesPerPixel;
		//Wyrmgus end
		SDL_GetRGB(color, TheScreen->format, &c.r, &c.g, &c.b);
	}
	while (w-- >= 0) {
		int h = h0;
		while (h-- >= 0) {
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				//Wyrmgus start
//				*(Uint32 *)&(MinimapSurfaceGL[((mx + w) + (my + h) * MinimapTextureWidth) * 4]) = color;
				*(Uint32 *)&(MinimapSurfaceGL[CurrentMapLayer][((mx + w) + (my + h) * MinimapTextureWidth) * 4]) = color;
				//Wyrmgus end
			} else
#endif
			{
				//Wyrmgus start
//				const unsigned int index = (mx + w) * bpp + (my + h) * MinimapSurface->pitch;
				const unsigned int index = (mx + w) * bpp + (my + h) * MinimapSurface[CurrentMapLayer]->pitch;
				//Wyrmgus end
				if (bpp == 2) {
					//Wyrmgus start
//					*(Uint16 *)&((Uint8 *)MinimapSurface->pixels)[index] = color;
					*(Uint16 *)&((Uint8 *)MinimapSurface[CurrentMapLayer]->pixels)[index] = color;
					//Wyrmgus end
				} else {
					//Wyrmgus start
//					*(Uint32 *)&((Uint8 *)MinimapSurface->pixels)[index] = color;
					*(Uint32 *)&((Uint8 *)MinimapSurface[CurrentMapLayer]->pixels)[index] = color;
					//Wyrmgus end
				}
			}
		}
	}
}

/**
**  Update the minimap with the current game information
*/
void CMinimap::Update()
{
	static int red_phase;

	int red_phase_changed = red_phase != (int)((FrameCounter / FRAMES_PER_SECOND) & 1);
	if (red_phase_changed) {
		red_phase = !red_phase;
	}

	// Clear Minimap background if not transparent
	if (!Transparent) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		if (UseOpenGL) {
			//Wyrmgus start
//			memset(MinimapSurfaceGL, 0, MinimapTextureWidth * MinimapTextureHeight * 4);
			memset(MinimapSurfaceGL[CurrentMapLayer], 0, MinimapTextureWidth * MinimapTextureHeight * 4);
			//Wyrmgus end
		} else
#endif
		{
			//Wyrmgus start
//			SDL_FillRect(MinimapSurface, NULL, SDL_MapRGB(MinimapSurface->format, 0, 0, 0));
			SDL_FillRect(MinimapSurface[CurrentMapLayer], NULL, SDL_MapRGB(MinimapSurface[CurrentMapLayer]->format, 0, 0, 0));
			//Wyrmgus end
		}
	}

	int bpp;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		bpp = 0;
	} else
#endif
	{
		//Wyrmgus start
//		bpp = MinimapSurface->format->BytesPerPixel;
		bpp = MinimapSurface[CurrentMapLayer]->format->BytesPerPixel;
		//Wyrmgus end
	}

	//
	// Draw the terrain
	//
	if (WithTerrain) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		if (UseOpenGL) {
			//Wyrmgus start
//			memcpy(MinimapSurfaceGL, MinimapTerrainSurfaceGL, MinimapTextureWidth * MinimapTextureHeight * 4);
			memcpy(MinimapSurfaceGL[CurrentMapLayer], MinimapTerrainSurfaceGL[CurrentMapLayer], MinimapTextureWidth * MinimapTextureHeight * 4);
			//Wyrmgus end
		} else
#endif
		{
			//Wyrmgus start
//			SDL_BlitSurface(MinimapTerrainSurface, NULL, MinimapSurface, NULL);
			SDL_BlitSurface(MinimapTerrainSurface[CurrentMapLayer], NULL, MinimapSurface[CurrentMapLayer], NULL);
			//Wyrmgus end
		}
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
//		SDL_LockSurface(MinimapSurface);
//		SDL_LockSurface(MinimapTerrainSurface);
		SDL_LockSurface(MinimapSurface[CurrentMapLayer]);
		SDL_LockSurface(MinimapTerrainSurface[CurrentMapLayer]);
		//Wyrmgus end
	}

	for (int my = 0; my < H; ++my) {
		for (int mx = 0; mx < W; ++mx) {
			//Wyrmgus start
			if (mx < XOffset[CurrentMapLayer] || mx >= W - XOffset[CurrentMapLayer] || my < YOffset[CurrentMapLayer] || my >= H - YOffset[CurrentMapLayer]) {
#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					*(Uint32 *)&(MinimapSurfaceGL[CurrentMapLayer][(mx + my * MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
				} else
#endif
				{
					//Wyrmgus start
//					const int index = mx * bpp + my * MinimapSurface->pitch;
					const int index = mx * bpp + my * MinimapSurface[CurrentMapLayer]->pitch;
					//Wyrmgus end
					if (bpp == 2) {
						//Wyrmgus start
//						*(Uint16 *)&((Uint8 *)MinimapSurface->pixels)[index] = ColorBlack;
						*(Uint16 *)&((Uint8 *)MinimapSurface[CurrentMapLayer]->pixels)[index] = ColorBlack;
						//Wyrmgus end
					} else {
						//Wyrmgus start
//						*(Uint32 *)&((Uint8 *)MinimapSurface->pixels)[index] = ColorBlack;
						*(Uint32 *)&((Uint8 *)MinimapSurface[CurrentMapLayer]->pixels)[index] = ColorBlack;
						//Wyrmgus end
					}
				}
				continue;
			}
			//Wyrmgus end
			
			int visiontype; // 0 unexplored, 1 explored, >1 visible.

			if (ReplayRevealMap) {
				visiontype = 2;
			} else {
				//Wyrmgus start
//				const Vec2i tilePos(Minimap2MapX[mx], Minimap2MapY[my] / Map.Info.MapWidth);
//				visiontype = Map.Field(tilePos)->playerInfo.TeamVisibilityState(*ThisPlayer);
				const Vec2i tilePos(Minimap2MapX[CurrentMapLayer][mx], Minimap2MapY[CurrentMapLayer][my] / Map.Info.MapWidths[CurrentMapLayer]);
				visiontype = Map.Field(tilePos, CurrentMapLayer)->playerInfo.TeamVisibilityState(*ThisPlayer);
				//Wyrmgus end
			}

			if (visiontype == 0 || (visiontype == 1 && ((mx & 1) != (my & 1)))) {
#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					//Wyrmgus start
//					*(Uint32 *)&(MinimapSurfaceGL[(mx + my * MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
					*(Uint32 *)&(MinimapSurfaceGL[CurrentMapLayer][(mx + my * MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
					//Wyrmgus end
				} else
#endif
				{
					//Wyrmgus start
//					const int index = mx * bpp + my * MinimapSurface->pitch;
					const int index = mx * bpp + my * MinimapSurface[CurrentMapLayer]->pitch;
					//Wyrmgus end
					if (bpp == 2) {
						//Wyrmgus start
//						*(Uint16 *)&((Uint8 *)MinimapSurface->pixels)[index] = ColorBlack;
						*(Uint16 *)&((Uint8 *)MinimapSurface[CurrentMapLayer]->pixels)[index] = ColorBlack;
						//Wyrmgus end
					} else {
						//Wyrmgus start
//						*(Uint32 *)&((Uint8 *)MinimapSurface->pixels)[index] = ColorBlack;
						*(Uint32 *)&((Uint8 *)MinimapSurface[CurrentMapLayer]->pixels)[index] = ColorBlack;
						//Wyrmgus end
					}
				}
			}
		}
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
//		SDL_UnlockSurface(MinimapTerrainSurface);
		SDL_UnlockSurface(MinimapTerrainSurface[CurrentMapLayer]);
		//Wyrmgus end
	}

	//
	// Draw units on map
	//
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (unit.IsVisibleOnMinimap()) {
			DrawUnitOn(unit, red_phase);
		}
	}
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//Wyrmgus start
//		SDL_UnlockSurface(MinimapSurface);
		SDL_UnlockSurface(MinimapSurface[CurrentMapLayer]);
		//Wyrmgus end
	}
}

/**
**  Draw the minimap events
*/
static void DrawEvents()
{
	const unsigned char alpha = 192;

	for (int i = 0; i < NumMinimapEvents; ++i) {
		Video.DrawTransCircleClip(MinimapEvents[i].Color,
								  MinimapEvents[i].pos.x, MinimapEvents[i].pos.y,
								  MinimapEvents[i].Size, alpha);
		MinimapEvents[i].Size -= 1;
		if (MinimapEvents[i].Size < 2) {
			MinimapEvents[i] = MinimapEvents[--NumMinimapEvents];
			--i;
		}
	}
}

/**
**  Draw the minimap on the screen
*/
void CMinimap::Draw() const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		//Wyrmgus start
//		glBindTexture(GL_TEXTURE_2D, MinimapTexture);
		glBindTexture(GL_TEXTURE_2D, MinimapTexture[CurrentMapLayer]);
		//Wyrmgus end
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MinimapTextureWidth, MinimapTextureHeight,
						//Wyrmgus start
//						GL_RGBA, GL_UNSIGNED_BYTE, MinimapSurfaceGL);
						GL_RGBA, GL_UNSIGNED_BYTE, MinimapSurfaceGL[CurrentMapLayer]);
						//Wyrmgus end

#ifdef USE_GLES
		float texCoord[] = {
			0.0f, 0.0f,
			(float)W / MinimapTextureWidth, 0.0f,
			0.0f, (float)H / MinimapTextureHeight,
			(float)W / MinimapTextureWidth, (float)H / MinimapTextureHeight
		};

		float vertex[] = {
			2.0f / (GLfloat)Video.Width *X - 1.0f, -2.0f / (GLfloat)Video.Height *Y + 1.0f,
			2.0f / (GLfloat)Video.Width *(X + W) - 1.0f, -2.0f / (GLfloat)Video.Height *Y + 1.0f,
			2.0f / (GLfloat)Video.Width *X - 1.0f, -2.0f / (GLfloat)Video.Height *(Y + H) + 1.0f,
			2.0f / (GLfloat)Video.Width *(X + W) - 1.0f, -2.0f / (GLfloat)Video.Height *(Y + H) + 1.0f
		};

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);

		glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(X, Y);
		glTexCoord2f(0.0f, (float)H / MinimapTextureHeight);
		glVertex2i(X, Y + H);
		glTexCoord2f((float)W / MinimapTextureWidth, (float)H / MinimapTextureHeight);
		glVertex2i(X + W, Y + H);
		glTexCoord2f((float)W / MinimapTextureWidth, 0.0f);
		glVertex2i(X + W, Y);
		glEnd();
#endif
	} else
#endif
	{
		SDL_Rect drect = {Sint16(X), Sint16(Y), 0, 0};
		//Wyrmgus start
//		SDL_BlitSurface(MinimapSurface, NULL, TheScreen, &drect);
		SDL_BlitSurface(MinimapSurface[CurrentMapLayer], NULL, TheScreen, &drect);
		//Wyrmgus end
	}

	DrawEvents();
}

/**
**  Convert screen position to tile map coordinate.
**
**  @param screenPos  Screen pixel coordinate.
**
**  @return   Tile coordinate.
*/
Vec2i CMinimap::ScreenToTilePos(const PixelPos &screenPos) const
{
	//Wyrmgus start
//	Vec2i tilePos((((screenPos.x - X - XOffset) * MINIMAP_FAC) / MinimapScaleX),
//				  (((screenPos.y - Y - YOffset) * MINIMAP_FAC) / MinimapScaleY));
	Vec2i tilePos((((screenPos.x - X - XOffset[CurrentMapLayer]) * MINIMAP_FAC) / MinimapScaleX[CurrentMapLayer]),
				  (((screenPos.y - Y - YOffset[CurrentMapLayer]) * MINIMAP_FAC) / MinimapScaleY[CurrentMapLayer]));
	//Wyrmgus end

	//Wyrmgus start
//	Map.Clamp(tilePos);
	Map.Clamp(tilePos, CurrentMapLayer);
	//Wyrmgus end
	return tilePos;
}

/**
**  Convert tile map coordinate to screen position.
**
**  @param tilePos  Tile coordinate.
**
**  @return   Screen pixel coordinate.
*/
PixelPos CMinimap::TilePosToScreenPos(const Vec2i &tilePos) const
{
	//Wyrmgus start
//	const PixelPos screenPos(X + XOffset + (tilePos.x * MinimapScaleX) / MINIMAP_FAC,
//							 Y + YOffset + (tilePos.y * MinimapScaleY) / MINIMAP_FAC);
	const PixelPos screenPos(X + XOffset[CurrentMapLayer] + (tilePos.x * MinimapScaleX[CurrentMapLayer]) / MINIMAP_FAC,
							 Y + YOffset[CurrentMapLayer] + (tilePos.y * MinimapScaleY[CurrentMapLayer]) / MINIMAP_FAC);
	//Wyrmgus end
	return screenPos;
}

/**
**  Destroy mini-map.
*/
void CMinimap::Destroy()
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		//Wyrmgus start
//		delete[] MinimapTerrainSurfaceGL;
//		MinimapTerrainSurfaceGL = NULL;
//		if (MinimapSurfaceGL) {
//			glDeleteTextures(1, &MinimapTexture);
//			delete[] MinimapSurfaceGL;
//			MinimapSurfaceGL = NULL;
//		}
		for (size_t z = 0; z < MinimapTerrainSurfaceGL.size(); ++z) {
			delete[] MinimapTerrainSurfaceGL[z];
			MinimapTerrainSurfaceGL[z] = NULL;
		}
		MinimapTerrainSurfaceGL.clear();
		for (size_t z = 0; z < MinimapSurfaceGL.size(); ++z) {
			if (MinimapSurfaceGL[z]) {
				//Wyrmgus start
//				glDeleteTextures(1, &MinimapTexture);
				glDeleteTextures(1, &MinimapTexture[z]);
				//Wyrmgus end
				delete[] MinimapSurfaceGL[z];
				MinimapSurfaceGL[z] = NULL;
			}
		}
		MinimapSurfaceGL.clear();
		MinimapTexture.clear();
		//Wyrmgus end
	} else
#endif
	{
		//Wyrmgus start
//		VideoPaletteListRemove(MinimapTerrainSurface);
//		SDL_FreeSurface(MinimapTerrainSurface);
//		MinimapTerrainSurface = NULL;
//		if (MinimapSurface) {
//			VideoPaletteListRemove(MinimapSurface);
//			SDL_FreeSurface(MinimapSurface);
//			MinimapSurface = NULL;
//		}
		for (size_t z = 0; z < MinimapTerrainSurface.size(); ++z) {
			VideoPaletteListRemove(MinimapTerrainSurface[z]);
			SDL_FreeSurface(MinimapTerrainSurface[z]);
			MinimapTerrainSurface[z] = NULL;
		}
		MinimapTerrainSurface.clear();
		for (size_t z = 0; z < MinimapSurface.size(); ++z) {
			if (MinimapSurface[z]) {
				VideoPaletteListRemove(MinimapSurface[z]);
				SDL_FreeSurface(MinimapSurface[z]);
				MinimapSurface[z] = NULL;
			}
		}
		MinimapSurface.clear();
		//Wyrmgus end
	}
	//Wyrmgus start
//	delete[] Minimap2MapX;
//	Minimap2MapX = NULL;
//	delete[] Minimap2MapY;
//	Minimap2MapY = NULL;
	for (size_t z = 0; z < Minimap2MapX.size(); ++z) {
		delete[] Minimap2MapX[z];
		Minimap2MapX[z] = NULL;
	}
	Minimap2MapX.clear();
	for (size_t z = 0; z < Minimap2MapY.size(); ++z) {
		delete[] Minimap2MapY[z];
		Minimap2MapY[z] = NULL;
	}
	Minimap2MapY.clear();
	for (size_t z = 0; z < Map2MinimapX.size(); ++z) {
		delete[] Map2MinimapX[z];
		Map2MinimapX[z] = NULL;
	}
	Map2MinimapX.clear();
	for (size_t z = 0; z < Map2MinimapY.size(); ++z) {
		delete[] Map2MinimapY[z];
		Map2MinimapY[z] = NULL;
	}
	Map2MinimapY.clear();
	MinimapScaleX.clear();
	MinimapScaleY.clear();
	XOffset.clear();
	YOffset.clear();
	//Wyrmgus end
}

/**
**  Draw viewport area contour.
*/
void CMinimap::DrawViewportArea(const CViewport &viewport) const
{
	// Determine and save region below minimap cursor
	const PixelPos screenPos = TilePosToScreenPos(viewport.MapPos);
	//Wyrmgus start
//	int w = (viewport.MapWidth * MinimapScaleX) / MINIMAP_FAC;
//	int h = (viewport.MapHeight * MinimapScaleY) / MINIMAP_FAC;
	int w = (viewport.MapWidth * MinimapScaleX[CurrentMapLayer]) / MINIMAP_FAC;
	int h = (viewport.MapHeight * MinimapScaleY[CurrentMapLayer]) / MINIMAP_FAC;
	//Wyrmgus end

	// Draw cursor as rectangle (Note: unclipped, as it is always visible)
	//Wyrmgus start
//	Video.DrawTransRectangle(UI.ViewportCursorColor, screenPos.x, screenPos.y, w, h, 128);
	Video.DrawTransRectangle(UI.ViewportCursorColor, screenPos.x, screenPos.y, w + 1, h + 1, 128);
	//Wyrmgus end
}

/**
**  Add a minimap event
**
**  @param pos  Map tile position
*/
//Wyrmgus start
//void CMinimap::AddEvent(const Vec2i &pos, Uint32 color)
void CMinimap::AddEvent(const Vec2i &pos, int z, Uint32 color)
//Wyrmgus end
{
	//Wyrmgus start
	if (z != CurrentMapLayer) {
		return;
	}
	//Wyrmgus end
	if (NumMinimapEvents == MAX_MINIMAP_EVENTS) {
		return;
	}
	MinimapEvents[NumMinimapEvents].pos = TilePosToScreenPos(pos);
	MinimapEvents[NumMinimapEvents].Size = (W < H) ? W / 3 : H / 3;
	MinimapEvents[NumMinimapEvents].Color = color;
	++NumMinimapEvents;
}

bool CMinimap::Contains(const PixelPos &screenPos) const
{
	return this->X <= screenPos.x && screenPos.x < this->X + this->W
		   && this->Y <= screenPos.y && screenPos.y < this->Y + this->H;
}

//@}
