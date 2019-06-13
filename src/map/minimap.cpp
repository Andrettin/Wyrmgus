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
/**@name minimap.cpp - The minimap source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer and Jimmy Salmon, Pali Rohár and Andrettin
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

#include "map/minimap.h"

#include "editor/editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "player.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "video/video.h"
#include "world/plane.h"
#include "world/province.h"
#include "world/world.h"

#include <string.h>

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

constexpr int MINIMAP_FAC = (16 * 3);  /// integer scale factor

/// unit attacked are shown red for at least this amount of cycles
constexpr int ATTACK_RED_DURATION = (1 * CYCLES_PER_SECOND);
/// unit attacked are shown blinking for this amount of cycles
constexpr int ATTACK_BLINK_DURATION = (7 * CYCLES_PER_SECOND);

constexpr int SCALE_PRECISION = 100;

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
//SDL_Surface *MinimapSurface;        /// generated minimap
//SDL_Surface *MinimapTerrainSurface; /// generated minimap terrain
std::vector<SDL_Surface *> MinimapSurface;        /// generated minimap
std::vector<SDL_Surface *> MinimapTerrainSurface; /// generated minimap terrain
//Wyrmgus end

//Wyrmgus start
//unsigned char *MinimapSurfaceGL;
//unsigned char *MinimapTerrainSurfaceGL;
std::vector<unsigned char *> MinimapSurfaceGL;
std::vector<unsigned char *> MinimapTerrainSurfaceGL;
//Wyrmgus end

//Wyrmgus start
//static GLuint MinimapTexture;
//static int MinimapTextureWidth;
//static int MinimapTextureHeight;
static std::vector<GLuint> MinimapTexture;
static std::vector<int> MinimapTextureWidth;
static std::vector<int> MinimapTextureHeight;
//Wyrmgus end

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

constexpr int MAX_MINIMAP_EVENTS = 8;

struct MinimapEvent {
	PixelPos pos;
	int Size;
	Uint32 Color;
} MinimapEvents[MAX_MINIMAP_EVENTS];
int NumMinimapEvents;


/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/


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
	//Wyrmgus start
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MinimapTextureWidth,
//				 MinimapTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
//				 MinimapSurfaceGL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MinimapTextureWidth[z],
				 MinimapTextureHeight[z], 0, GL_RGBA, GL_UNSIGNED_BYTE,
				 MinimapSurfaceGL[z]);
	//Wyrmgus end
}

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
	const int n = std::max(std::max(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight), 32);

	MinimapScaleX = (W * MINIMAP_FAC + n - 1) / n;
	MinimapScaleY = (H * MINIMAP_FAC + n - 1) / n;

	XOffset = (W - (CMap::Map.Info.MapWidth * MinimapScaleX) / MINIMAP_FAC + 1) / 2;
	YOffset = (H - (CMap::Map.Info.MapHeight * MinimapScaleY) / MINIMAP_FAC + 1) / 2;

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
		Minimap2MapY[i] = (((i - YOffset) * MINIMAP_FAC) / MinimapScaleY) * CMap::Map.Info.MapWidth;
	}
	for (int i = 0; i < CMap::Map.Info.MapWidth; ++i) {
		Map2MinimapX[i] = (i * MinimapScaleX) / MINIMAP_FAC;
	}
	for (int i = 0; i < CMap::Map.Info.MapHeight; ++i) {
		Map2MinimapY[i] = (i * MinimapScaleY) / MINIMAP_FAC;
	}

	// Palette updated from UpdateMinimapTerrain()
	for (MinimapTextureWidth = 1; MinimapTextureWidth < W; MinimapTextureWidth <<= 1) {
	}
	for (MinimapTextureHeight = 1; MinimapTextureHeight < H; MinimapTextureHeight <<= 1) {
	}
	MinimapTerrainSurfaceGL = new unsigned char[MinimapTextureWidth * MinimapTextureHeight * 4];
	MinimapSurfaceGL = new unsigned char[MinimapTextureWidth * MinimapTextureHeight * 4];
	memset(MinimapSurfaceGL, 0, MinimapTextureWidth * MinimapTextureHeight * 4);
	CreateMinimapTexture();

	UpdateTerrain();
	*/

	MinimapTexture.resize(CMap::Map.MapLayers.size());
	MinimapTextureWidth.resize(CMap::Map.MapLayers.size());
	MinimapTextureHeight.resize(CMap::Map.MapLayers.size());

	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		// Scale to biggest value.
		const int n = std::max(std::max(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]), 32);

		MinimapScaleX.push_back((W * MINIMAP_FAC + n - 1) / n);
		MinimapScaleY.push_back((H * MINIMAP_FAC + n - 1) / n);

		XOffset.push_back((W - (CMap::Map.Info.MapWidths[z] * MinimapScaleX[z]) / MINIMAP_FAC + 1) / 2);
		YOffset.push_back((H - (CMap::Map.Info.MapHeights[z] * MinimapScaleY[z]) / MINIMAP_FAC + 1) / 2);

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
			Minimap2MapY[z][i] = (((i - YOffset[z]) * MINIMAP_FAC) / MinimapScaleY[z]) * CMap::Map.Info.MapWidths[z];
		}
		Map2MinimapX.push_back(new int[CMap::Map.Info.MapWidths[z]]);
		memset(Map2MinimapX[z], 0, CMap::Map.Info.MapWidths[z] * sizeof(int));
		Map2MinimapY.push_back(new int[CMap::Map.Info.MapHeights[z]]);
		memset(Map2MinimapY[z], 0, CMap::Map.Info.MapHeights[z] * sizeof(int));
		for (int i = 0; i < CMap::Map.Info.MapWidths[z]; ++i) {
			Map2MinimapX[z][i] = (i * MinimapScaleX[z]) / MINIMAP_FAC;
		}
		for (int i = 0; i < CMap::Map.Info.MapHeights[z]; ++i) {
			Map2MinimapY[z][i] = (i * MinimapScaleY[z]) / MINIMAP_FAC;
		}

		// Palette updated from UpdateMinimapTerrain()
		for (MinimapTextureWidth[z] = 1; MinimapTextureWidth[z] < W; MinimapTextureWidth[z] <<= 1) {
		}
		for (MinimapTextureHeight[z] = 1; MinimapTextureHeight[z] < H; MinimapTextureHeight[z] <<= 1) {
		}
		MinimapTerrainSurfaceGL.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		MinimapSurfaceGL.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		memset(MinimapSurfaceGL[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);
		CreateMinimapTexture(z);

		UpdateTerrain(z);
	}
	//Wyrmgus end

	NumMinimapEvents = 0;
}

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
	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		CreateMinimapTexture(z);
	}
	//Wyrmgus end
}

/**
**  Calculate the tile graphic pixel
*/
//Wyrmgus start
//static inline Uint8 *GetTileGraphicPixel(int xofs, int yofs, int mx, int my, int scalex, int scaley, int bpp)
static inline Uint8 *GetTileGraphicPixel(int xofs, int yofs, int mx, int my, int scalex, int scaley, int bpp, int z, const CTerrainType *terrain, const CSeason *season)
//Wyrmgus end
{
	//Wyrmgus start
//	Uint8 *pixels = (Uint8 *)CMap::Map.TileGraphic->Surface->pixels;
	Uint8 *pixels = (Uint8 *)terrain->GetGraphics(season)->Surface->pixels;
	//Wyrmgus end
	int x = (xofs + 7 + ((mx * SCALE_PRECISION) % scalex) / SCALE_PRECISION * 8);
	int y = (yofs + 6 + ((my * SCALE_PRECISION) % scaley) / SCALE_PRECISION * 8);
	//Wyrmgus start
//	return &pixels[x * bpp + y * CMap::Map.TileGraphic->Surface->pitch];
	return &pixels[x * bpp + y * terrain->GetGraphics(season)->Surface->pitch];
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
//	const int bpp = CMap::Map.TileGraphic->Surface->format->BytesPerPixel;
	//Wyrmgus end
	
	const CSeason *season = CMap::Map.MapLayers[z]->GetSeason();

	//Wyrmgus start
//	const int tilepitch = CMap::Map.TileGraphic->Surface->w / CMap::Map.GetCurrentPixelTileSize().x;
	//Wyrmgus end

	SDL_LockSurface(CMap::Map.TileGraphic->Surface);
	//Wyrmgus start
	for (const CTerrainType *terrain_type : CTerrainType::GetAll()) {
		if (terrain_type->GetGraphics(season)) {
			SDL_LockSurface(terrain_type->GetGraphics(season)->Surface);
		}
	}
	//Wyrmgus end

	const CMapLayer *map_layer = CMap::Map.MapLayers[z];
	
	//
	//  Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
	//
	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
			//Wyrmgus start
//			const int tile = CMap::Map.Fields[Minimap2MapX[mx] + Minimap2MapY[my]].getGraphicTile();
			const CMapField &mf = *map_layer->Field(Minimap2MapX[z][mx] + Minimap2MapY[z][my]);
			const CTerrainType *terrain = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlayTerrain : mf.playerInfo.SeenTerrain;
			int tile = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlaySolidTile : mf.playerInfo.SeenSolidTile;
			if (!terrain) {
				terrain = mf.OverlayTerrain ? mf.OverlayTerrain : mf.Terrain;
				tile = mf.OverlayTerrain ? mf.OverlaySolidTile : mf.SolidTile;
			}
			
			const CTerrainType *base_terrain = mf.playerInfo.SeenTerrain;
			int base_tile = mf.playerInfo.SeenSolidTile;
			if (!base_terrain) {
				base_terrain = mf.Terrain;
				base_tile = mf.SolidTile;
			}
			//Wyrmgus end

			if (!terrain) {
				continue;
			}
			
			//Wyrmgus start
			int tilepitch = terrain->GetGraphics(season)->Surface->w / CMap::Map.GetCurrentPixelTileSize().x;
			const int bpp = terrain->GetGraphics(season)->Surface->format->BytesPerPixel;
			
			int base_tilepitch = base_terrain->GetGraphics(season)->Surface->w / CMap::Map.GetCurrentPixelTileSize().x;
			//assumes the BPP for the base terrain is the same as for the top terrain (which may be an overlay)
			//Wyrmgus end
	
			const int xofs = CMap::Map.GetCurrentPixelTileSize().x * (tile % tilepitch);
			const int yofs = CMap::Map.GetCurrentPixelTileSize().y * (tile / tilepitch);
			
			//Wyrmgus start
			const int base_xofs = CMap::Map.GetCurrentPixelTileSize().x * (base_tile % base_tilepitch);
			const int base_yofs = CMap::Map.GetCurrentPixelTileSize().y * (base_tile / base_tilepitch);
			//Wyrmgus end

			Uint32 c;

			if (bpp == 1) {
				//Wyrmgus start
//				SDL_Color color = CMap::Map.TileGraphic->Surface->format->palette->colors[
//									  *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp)];
				SDL_Color color = terrain->GetGraphics(season)->Surface->format->palette->colors[
									  *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z, terrain, season)];
				if (color.r == 255 && color.g == 255 && color.b == 255) { //completely white pixel, presumed to be a transparent one; use base instead
					color = base_terrain->GetGraphics(season)->Surface->format->palette->colors[
									  *GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, base_terrain, season)];
				}
				//Wyrmgus end
				
				c = Video.MapRGB(0, color.r, color.g, color.b);
			} else {
				//Wyrmgus start
//				SDL_PixelFormat *f = CMap::Map.TileGraphic->Surface->format;
				SDL_PixelFormat *f = terrain->GetGraphics(season)->Surface->format;
				//Wyrmgus end
				c = *(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z, terrain, season);
				//Wyrmgus start
				if (((c & f->Amask) >> f->Ashift) == 0) { //transparent pixel, use base instead
					f = base_terrain->GetGraphics(season)->Surface->format;
					c = *(Uint32 *)GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, base_terrain, season);
				}
				//Wyrmgus end
				c = Video.MapRGB(0,
								 ((c & f->Rmask) >> f->Rshift),
								 ((c & f->Gmask) >> f->Gshift),
								 ((c & f->Bmask) >> f->Bshift));
			}
			//Wyrmgus start
//			*(Uint32 *)&(MinimapTerrainSurfaceGL[(mx + my * MinimapTextureWidth) * 4]) = c;
			*(Uint32 *)&(MinimapTerrainSurfaceGL[z][(mx + my * MinimapTextureWidth[z]) * 4]) = c;
			//Wyrmgus end
		}
	}
	SDL_UnlockSurface(CMap::Map.TileGraphic->Surface);
	//Wyrmgus start
	for (const CTerrainType *terrain_type : CTerrainType::GetAll()) {
		if (terrain_type->GetGraphics(season)) {
			SDL_UnlockSurface(terrain_type->GetGraphics(season)->Surface);
		}
	}
	//Wyrmgus end
}

/**
**	@brief	Update a single minimap tile after a change
**
**	@param	pos	The map position to update in the minimap
**	@param	z	The map layer of the tile to update
*/
void CMinimap::UpdateXY(const Vec2i &pos, int z)
{
	//Wyrmgus start
//	if (!MinimapTerrainSurfaceGL) {
	if (z >= (int) MinimapTerrainSurfaceGL.size() || !MinimapTerrainSurfaceGL[z]) {
	//Wyrmgus end
		return;
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
//	const int tilepitch = CMap::Map.TileGraphic->Surface->w / CMap::Map.GetCurrentPixelTileSize().x;
//	const int bpp = CMap::Map.TileGraphic->Surface->format->BytesPerPixel;
	//Wyrmgus end

	const CSeason *season = CMap::Map.MapLayers[z]->GetSeason();

	//
	//  Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
	//
	SDL_LockSurface(CMap::Map.TileGraphic->Surface);
	//Wyrmgus start
	for (const CTerrainType *terrain_type : CTerrainType::GetAll()) {
		if (terrain_type->GetGraphics(season)) {
			SDL_LockSurface(terrain_type->GetGraphics(season)->Surface);
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	const int ty = pos.y * CMap::Map.Info.MapWidth;
	const int ty = pos.y * CMap::Map.Info.MapWidths[z];
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
			int tile = CMap::Map.Fields[x + y].playerInfo.SeenTile;
			if (!tile) {
				tile = CMap::Map.Fields[x + y].getGraphicTile();
			}
			*/
			const CMapField &mf = *CMap::Map.MapLayers[z]->Field(x + y);
			const CTerrainType *terrain = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlayTerrain : mf.playerInfo.SeenTerrain;
			int tile = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlaySolidTile : mf.playerInfo.SeenSolidTile;
			if (!terrain) {
				terrain = mf.OverlayTerrain ? mf.OverlayTerrain : mf.Terrain;
				tile = mf.OverlayTerrain ? mf.OverlaySolidTile : mf.SolidTile;
			}
			
			const CTerrainType *base_terrain = mf.playerInfo.SeenTerrain;
			int base_tile = mf.playerInfo.SeenSolidTile;
			if (!base_terrain) {
				base_terrain = mf.Terrain;
				base_tile = mf.SolidTile;
			}
			//Wyrmgus end

			//Wyrmgus start
			int tilepitch = terrain->GetGraphics(season)->Surface->w / CMap::Map.GetCurrentPixelTileSize().x;
			const int bpp = terrain->GetGraphics(season)->Surface->format->BytesPerPixel;
			
			int base_tilepitch = base_terrain->GetGraphics(season)->Surface->w / CMap::Map.GetCurrentPixelTileSize().x;
			//Wyrmgus end
	
			const int xofs = CMap::Map.GetCurrentPixelTileSize().x * (tile % tilepitch);
			const int yofs = CMap::Map.GetCurrentPixelTileSize().y * (tile / tilepitch);
			
			//Wyrmgus start
			const int base_xofs = CMap::Map.GetCurrentPixelTileSize().x * (base_tile % base_tilepitch);
			const int base_yofs = CMap::Map.GetCurrentPixelTileSize().y * (base_tile / base_tilepitch);
			//Wyrmgus end

			Uint32 c;

			if (bpp == 1) {
				//Wyrmgus start
//				const int colorIndex = *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
//				const SDL_Color color = CMap::Map.TileGraphic->Surface->format->palette->colors[colorIndex];
				int colorIndex = *GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z, terrain, season);
				SDL_Color color = terrain->GetGraphics(season)->Surface->format->palette->colors[colorIndex];
				if (color.r == 255 && color.g == 255 && color.b == 255) { //completely white pixel, presumed to be a transparent one; use base instead
					colorIndex = *GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, base_terrain, season);
					color = base_terrain->GetGraphics(season)->Surface->format->palette->colors[colorIndex];
				}
				//Wyrmgus end

				c = Video.MapRGB(0, color.r, color.g, color.b);
			} else {
				//Wyrmgus start
//				SDL_PixelFormat *f = CMap::Map.TileGraphic->Surface->format;
				SDL_PixelFormat *f = terrain->GetGraphics(season)->Surface->format;
				//Wyrmgus end

				//Wyrmgus start
//				c = *(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp);
				c = *(Uint32 *)GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, bpp, z, terrain, season);
				
				if (((c & f->Amask) >> f->Ashift) == 0) { //transparent pixel, use base instead
					f = base_terrain->GetGraphics(season)->Surface->format;
					c = *(Uint32 *)GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, bpp, z, base_terrain, season);
				}
				//Wyrmgus end
				c = Video.MapRGB(0,
								 ((c & f->Rmask) >> f->Rshift),
								 ((c & f->Gmask) >> f->Gshift),
								 ((c & f->Bmask) >> f->Bshift));
			}
			//Wyrmgus start
//			*(Uint32 *)&(MinimapTerrainSurfaceGL[(mx + my * MinimapTextureWidth) * 4]) = c;
			*(Uint32 *)&(MinimapTerrainSurfaceGL[z][(mx + my * MinimapTextureWidth[z]) * 4]) = c;
			//Wyrmgus end
		}
	}
	SDL_UnlockSurface(CMap::Map.TileGraphic->Surface);
	//Wyrmgus start
	for (const CTerrainType *terrain_type : CTerrainType::GetAll()) {
		if (terrain_type->GetGraphics(season)) {
			SDL_UnlockSurface(terrain_type->GetGraphics(season)->Surface);
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

	if (Editor.Running || ReplayRevealMap || unit.IsVisible(*CPlayer::GetThisPlayer())) {
		type = unit.GetType();
	} else {
		type = unit.Seen.Type;
		// This will happen for radar if the unit has not been seen and we
		// have it on radar.
		if (!type) {
			type = unit.GetType();
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
//	if (unit.GetPlayer()->GetIndex() == PlayerNumNeutral) {
	if (unit.GetDisplayPlayer() == PlayerNumNeutral) {
	//Wyrmgus end
		color = Video.MapRGB(TheScreen->format, type->NeutralMinimapColorRGB);
	} else if (unit.GetPlayer() == CPlayer::GetThisPlayer() && !Editor.Running) {
		if (unit.Attacked && unit.Attacked + ATTACK_BLINK_DURATION > GameCycle &&
			(red_phase || unit.Attacked + ATTACK_RED_DURATION > GameCycle)) {
			color = ColorRed;
		} else if (UI.Minimap.ShowSelected && unit.Selected) {
			color = ColorWhite;
		} else {
			color = ColorGreen;
		}
	} else {
		color = unit.GetPlayer()->Color;
	}

	//Wyrmgus start
//	int mx = 1 + UI.Minimap.XOffset + Map2MinimapX[unit.GetTilePos().x];
//	int my = 1 + UI.Minimap.YOffset + Map2MinimapY[unit.GetTilePos().y];
//	int w = Map2MinimapX[type->TileSize.x];
	int mx = 1 + UI.Minimap.XOffset[UI.CurrentMapLayer->ID] + Map2MinimapX[UI.CurrentMapLayer->ID][unit.GetTilePos().x];
	int my = 1 + UI.Minimap.YOffset[UI.CurrentMapLayer->ID] + Map2MinimapY[UI.CurrentMapLayer->ID][unit.GetTilePos().y];
	int w = Map2MinimapX[UI.CurrentMapLayer->ID][type->TileSize.x];
	//Wyrmgus end
	if (mx + w >= UI.Minimap.W) { // clip right side
		w = UI.Minimap.W - mx;
	}
	//Wyrmgus start
//	int h0 = Map2MinimapY[type->TileSize.y];
	int h0 = Map2MinimapY[UI.CurrentMapLayer->ID][type->TileSize.y];
	//Wyrmgus end
	if (my + h0 >= UI.Minimap.H) { // clip bottom side
		h0 = UI.Minimap.H - my;
	}
	int bpp = 0;
	while (w-- >= 0) {
		int h = h0;
		while (h-- >= 0) {
			//Wyrmgus start
//			*(Uint32 *)&(MinimapSurfaceGL[((mx + w) + (my + h) * MinimapTextureWidth) * 4]) = color;
			*(Uint32 *)&(MinimapSurfaceGL[UI.CurrentMapLayer->ID][((mx + w) + (my + h) * MinimapTextureWidth[UI.CurrentMapLayer->ID]) * 4]) = color;
			//Wyrmgus end
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
		//Wyrmgus start
//		memset(MinimapSurfaceGL, 0, MinimapTextureWidth * MinimapTextureHeight * 4);
		memset(MinimapSurfaceGL[UI.CurrentMapLayer->ID], 0, MinimapTextureWidth[UI.CurrentMapLayer->ID] * MinimapTextureHeight[UI.CurrentMapLayer->ID] * 4);
		//Wyrmgus end
	}

	int bpp = 0;


	//
	// Draw the terrain
	//
	if (WithTerrain) {
		//Wyrmgus start
//		memcpy(MinimapSurfaceGL, MinimapTerrainSurfaceGL, MinimapTextureWidth * MinimapTextureHeight * 4);
		memcpy(MinimapSurfaceGL[UI.CurrentMapLayer->ID], MinimapTerrainSurfaceGL[UI.CurrentMapLayer->ID], MinimapTextureWidth[UI.CurrentMapLayer->ID] * MinimapTextureHeight[UI.CurrentMapLayer->ID] * 4);
		//Wyrmgus end
	}

	for (int my = 0; my < H; ++my) {
		for (int mx = 0; mx < W; ++mx) {
			//Wyrmgus start
			if (mx < XOffset[UI.CurrentMapLayer->ID] || mx >= W - XOffset[UI.CurrentMapLayer->ID] || my < YOffset[UI.CurrentMapLayer->ID] || my >= H - YOffset[UI.CurrentMapLayer->ID]) {
				*(Uint32 *)&(MinimapSurfaceGL[UI.CurrentMapLayer->ID][(mx + my * MinimapTextureWidth[UI.CurrentMapLayer->ID]) * 4]) = Video.MapRGB(0, 0, 0, 0);

				continue;
			}
			//Wyrmgus end
			
			int visiontype; // 0 unexplored, 1 explored, >1 visible.

			if (ReplayRevealMap) {
				visiontype = 2;
			} else {
				//Wyrmgus start
//				const Vec2i tilePos(Minimap2MapX[mx], Minimap2MapY[my] / CMap::Map.Info.MapWidth);
//				visiontype = CMap::Map.Field(tilePos)->playerInfo.TeamVisibilityState(*CPlayer::GetThisPlayer());
				const Vec2i tilePos(Minimap2MapX[UI.CurrentMapLayer->ID][mx], Minimap2MapY[UI.CurrentMapLayer->ID][my] / UI.CurrentMapLayer->GetWidth());
				visiontype = CMap::Map.Field(tilePos, UI.CurrentMapLayer->ID)->playerInfo.TeamVisibilityState(*CPlayer::GetThisPlayer());
				//Wyrmgus end
			}

			if (visiontype == 0 || (visiontype == 1 && ((mx & 1) != (my & 1)))) {
				//Wyrmgus start
//				*(Uint32 *)&(MinimapSurfaceGL[(mx + my * MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
				*(Uint32 *)&(MinimapSurfaceGL[UI.CurrentMapLayer->ID][(mx + my * MinimapTextureWidth[UI.CurrentMapLayer->ID]) * 4]) = Video.MapRGB(0, 0, 0, 0);
				//Wyrmgus end
			}
		}
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
	//Wyrmgus start
//	glBindTexture(GL_TEXTURE_2D, MinimapTexture);
	glBindTexture(GL_TEXTURE_2D, MinimapTexture[UI.CurrentMapLayer->ID]);
	//Wyrmgus end
	//Wyrmgus start
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MinimapTextureWidth, MinimapTextureHeight,
//					GL_RGBA, GL_UNSIGNED_BYTE, MinimapSurfaceGL);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MinimapTextureWidth[UI.CurrentMapLayer->ID], MinimapTextureHeight[UI.CurrentMapLayer->ID],
					GL_RGBA, GL_UNSIGNED_BYTE, MinimapSurfaceGL[UI.CurrentMapLayer->ID]);

#ifdef USE_GLES
	float texCoord[] = {
		0.0f, 0.0f,
		//Wyrmgus start
//		(float)W / MinimapTextureWidth, 0.0f,
//		0.0f, (float)H / MinimapTextureHeight,
//		(float)W / MinimapTextureWidth, (float)H / MinimapTextureHeight
		(float)W / MinimapTextureWidth[UI.CurrentMapLayer->ID], 0.0f,
		0.0f, (float)H / MinimapTextureHeight[UI.CurrentMapLayer->ID],
		(float)W / MinimapTextureWidth[UI.CurrentMapLayer->ID], (float)H / MinimapTextureHeight[UI.CurrentMapLayer->ID]
		//Wyrmgus end
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

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(X, Y);
	//Wyrmgus start
//	glTexCoord2f(0.0f, (float)H / MinimapTextureHeight);
	glTexCoord2f(0.0f, (float)H / MinimapTextureHeight[UI.CurrentMapLayer->ID]);
	//Wyrmgus end
	glVertex2i(X, Y + H);
	//Wyrmgus start
//	glTexCoord2f((float)W / MinimapTextureWidth, (float)H / MinimapTextureHeight);
	glTexCoord2f((float)W / MinimapTextureWidth[UI.CurrentMapLayer->ID], (float)H / MinimapTextureHeight[UI.CurrentMapLayer->ID]);
	//Wyrmgus end
	glVertex2i(X + W, Y + H);
	//Wyrmgus start
//	glTexCoord2f((float)W / MinimapTextureWidth, 0.0f);
	glTexCoord2f((float)W / MinimapTextureWidth[UI.CurrentMapLayer->ID], 0.0f);
	//Wyrmgus end
	glVertex2i(X + W, Y);
	glEnd();

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
	Vec2i tilePos((((screenPos.x - X - XOffset[UI.CurrentMapLayer->ID]) * MINIMAP_FAC) / MinimapScaleX[UI.CurrentMapLayer->ID]),
				  (((screenPos.y - Y - YOffset[UI.CurrentMapLayer->ID]) * MINIMAP_FAC) / MinimapScaleY[UI.CurrentMapLayer->ID]));
	//Wyrmgus end

	CMap::Map.Clamp(tilePos, UI.CurrentMapLayer->ID);
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
	const PixelPos screenPos(X + XOffset[UI.CurrentMapLayer->ID] + (tilePos.x * MinimapScaleX[UI.CurrentMapLayer->ID]) / MINIMAP_FAC,
							 Y + YOffset[UI.CurrentMapLayer->ID] + (tilePos.y * MinimapScaleY[UI.CurrentMapLayer->ID]) / MINIMAP_FAC);
	//Wyrmgus end
	return screenPos;
}

/**
**  Destroy mini-map.
*/
void CMinimap::Destroy()
{
	//Wyrmgus start
//	delete[] MinimapTerrainSurfaceGL;
//	MinimapTerrainSurfaceGL = nullptr;
//	if (MinimapSurfaceGL) {
//		glDeleteTextures(1, &MinimapTexture);
//		delete[] MinimapSurfaceGL;
//		MinimapSurfaceGL = nullptr;
//	}
	for (size_t z = 0; z < MinimapTerrainSurfaceGL.size(); ++z) {
		delete[] MinimapTerrainSurfaceGL[z];
		MinimapTerrainSurfaceGL[z] = nullptr;
	}
	MinimapTerrainSurfaceGL.clear();
	for (size_t z = 0; z < MinimapSurfaceGL.size(); ++z) {
		if (MinimapSurfaceGL[z]) {
			//Wyrmgus start
//			glDeleteTextures(1, &MinimapTexture);
			glDeleteTextures(1, &MinimapTexture[z]);
			//Wyrmgus end
			delete[] MinimapSurfaceGL[z];
			MinimapSurfaceGL[z] = nullptr;
		}
	}
	MinimapSurfaceGL.clear();
	MinimapTexture.clear();
	//Wyrmgus end
	
	//Wyrmgus start
//	delete[] Minimap2MapX;
//	Minimap2MapX = nullptr;
//	delete[] Minimap2MapY;
//	Minimap2MapY = nullptr;
	for (size_t z = 0; z < Minimap2MapX.size(); ++z) {
		delete[] Minimap2MapX[z];
		Minimap2MapX[z] = nullptr;
	}
	Minimap2MapX.clear();
	for (size_t z = 0; z < Minimap2MapY.size(); ++z) {
		delete[] Minimap2MapY[z];
		Minimap2MapY[z] = nullptr;
	}
	Minimap2MapY.clear();
	for (size_t z = 0; z < Map2MinimapX.size(); ++z) {
		delete[] Map2MinimapX[z];
		Map2MinimapX[z] = nullptr;
	}
	Map2MinimapX.clear();
	for (size_t z = 0; z < Map2MinimapY.size(); ++z) {
		delete[] Map2MinimapY[z];
		Map2MinimapY[z] = nullptr;
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
	int w = (viewport.MapWidth * MinimapScaleX[UI.CurrentMapLayer->ID]) / MINIMAP_FAC;
	int h = (viewport.MapHeight * MinimapScaleY[UI.CurrentMapLayer->ID]) / MINIMAP_FAC;
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
	if (NumMinimapEvents == MAX_MINIMAP_EVENTS) {
		return;
	}
	if (z == UI.CurrentMapLayer->ID) {
		MinimapEvents[NumMinimapEvents].pos = TilePosToScreenPos(pos);
		MinimapEvents[NumMinimapEvents].Size = (W < H) ? W / 3 : H / 3;
		MinimapEvents[NumMinimapEvents].Color = color;
		++NumMinimapEvents;
	} else {
		CMapLayer *event_map_layer = CMap::Map.MapLayers[z];
		if (event_map_layer->GetPlane() != nullptr && CMap::Map.GetCurrentPlane() != event_map_layer->GetPlane() && UI.PlaneButtons[event_map_layer->GetPlane()->GetIndex()].X != -1) {
			MinimapEvents[NumMinimapEvents].pos.x = UI.PlaneButtons[event_map_layer->GetPlane()->GetIndex()].X + (UI.PlaneButtons[event_map_layer->GetPlane()->GetIndex()].Style->Width / 2);
			MinimapEvents[NumMinimapEvents].pos.y = UI.PlaneButtons[event_map_layer->GetPlane()->GetIndex()].Y + (UI.PlaneButtons[event_map_layer->GetPlane()->GetIndex()].Style->Height / 2);
		} else if (event_map_layer->GetWorld() != nullptr && CMap::Map.GetCurrentWorld() != event_map_layer->GetWorld() && UI.WorldButtons[event_map_layer->GetWorld()->GetIndex()].X != -1) {
			MinimapEvents[NumMinimapEvents].pos.x = UI.WorldButtons[event_map_layer->GetWorld()->GetIndex()].X + (UI.WorldButtons[event_map_layer->GetWorld()->GetIndex()].Style->Width / 2);
			MinimapEvents[NumMinimapEvents].pos.y = UI.WorldButtons[event_map_layer->GetWorld()->GetIndex()].Y + (UI.WorldButtons[event_map_layer->GetWorld()->GetIndex()].Style->Height / 2);
		} else if (CMap::Map.GetCurrentSurfaceLayer() != event_map_layer->GetSurfaceLayer() && UI.SurfaceLayerButtons[event_map_layer->GetSurfaceLayer()].X != -1) {
			MinimapEvents[NumMinimapEvents].pos.x = UI.SurfaceLayerButtons[event_map_layer->GetSurfaceLayer()].X + (UI.SurfaceLayerButtons[event_map_layer->GetSurfaceLayer()].Style->Width / 2);
			MinimapEvents[NumMinimapEvents].pos.y = UI.SurfaceLayerButtons[event_map_layer->GetSurfaceLayer()].Y + (UI.SurfaceLayerButtons[event_map_layer->GetSurfaceLayer()].Style->Height / 2);
		} else {
			return;
		}
		MinimapEvents[NumMinimapEvents].Size = (W < H) ? W / 3 : H / 3;
		MinimapEvents[NumMinimapEvents].Color = color;
		++NumMinimapEvents;
	}
}

bool CMinimap::Contains(const PixelPos &screenPos) const
{
	return this->X <= screenPos.x && screenPos.x < this->X + this->W
		   && this->Y <= screenPos.y && screenPos.y < this->Y + this->H;
}
