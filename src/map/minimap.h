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
/**@name minimap.h - The minimap header file. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
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

#ifndef __MINIMAP_H__
#define __MINIMAP_H__

#include "vec2i.h"
#include "video/color.h"

class CViewport;

struct SDL_Surface;

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CMinimap
{
	template <const int BPP>
	void UpdateMapTerrain(void *const mpixels, const int mpitch,
						  const void *const tpixels, const int tpitch);

	void UpdateTerrain(int z);

	template <const int BPP>
	void UpdateSeen(void *const pixels, const int pitch);

public:
	void UpdateXY(const Vec2i &pos, int z);
	void UpdateSeenXY(const Vec2i &) {}
	void Update();
	void Create();

	void FreeOpenGL();
	void Reload();

	void Destroy();
	void Draw() const;
	void DrawViewportArea(const CViewport &viewport) const;
	//Wyrmgus start
//	void AddEvent(const Vec2i &pos, IntColor color);
	void AddEvent(const Vec2i &pos, int z, IntColor color);
	//Wyrmgus end

	Vec2i ScreenToTilePos(const PixelPos &screenPos) const;
	PixelPos TilePosToScreenPos(const Vec2i &tilePos) const;

	bool Contains(const PixelPos &screenPos) const;
public:
	int X = 0;
	int Y = 0;
	int W = 0;
	int H = 0;
	//Wyrmgus start
//	int XOffset = 0;
//	int YOffset = 0;
	std::vector<int> XOffset;
	std::vector<int> YOffset;
	//Wyrmgus end
	bool WithTerrain = false;
	bool ShowSelected = false;
	bool Transparent = false;
	bool UpdateCache = false;
};

// Minimap surface with units (for OpenGL)
//Wyrmgus start
//extern unsigned char *MinimapSurfaceGL;
extern std::vector<unsigned char *> MinimapSurfaceGL;
//Wyrmgus end
// Minimap surface with terrain only (for OpenGL)
//Wyrmgus start
//extern unsigned char *MinimapTerrainSurfaceGL;
extern std::vector<unsigned char *> MinimapTerrainSurfaceGL;
//Wyrmgus end

// Minimap surface with units (for software)
//Wyrmgus start
//extern SDL_Surface *MinimapSurface;
extern std::vector<SDL_Surface *> MinimapSurface;
//Wyrmgus end
// Minimap surface with terrain only (for software)
//Wyrmgus start
//extern SDL_Surface *MinimapTerrainSurface;
extern std::vector<SDL_Surface *> MinimapTerrainSurface;
//Wyrmgus end

#endif
