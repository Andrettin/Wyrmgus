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
/**@name png.cpp - The png graphic file loader. */
//
//      (c) Copyright 1998-2011 by Lutz Sammer, Jimmy Salmon and Pali Rohár
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

#include <png.h>

//Wyrmgus start
#include <fstream>
//Wyrmgus end

#include "stratagus.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
//Wyrmgus start
#include "map/terrain_type.h"
#include "map/tileset.h"
//Wyrmgus end
#include "ui/ui.h"
#include "video.h"
#include "iolib.h"
#include "iocompat.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  png read callback for CL-IO.
**
**  @param png_ptr  png struct pointer.
**  @param data     byte address to read to.
**  @param length   number of bytes to read.
*/
static void CL_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	CFile *f = (CFile *)png_get_io_ptr(png_ptr);
	png_size_t check = (png_size_t)f->read(data, (size_t)length);

	if (check != length) {
		png_error(png_ptr, "Read Error");
	}
}

class AutoPng_read_structp
{
public:
	explicit AutoPng_read_structp(png_structp png_ptr) : png_ptr(png_ptr), info_ptr(nullptr) {}
	~AutoPng_read_structp() { png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp)0, (png_infopp)0); }
	void setInfo(png_infop info_ptr) { this->info_ptr = info_ptr; }
private:
	png_structp png_ptr;
	png_infop info_ptr;
};

/**
**  Load a png graphic file.
**  Modified function from SDL_Image
**
**  @param g  graphic to load.
**
**  @return   0 for success, -1 for error.
*/
int LoadGraphicPNG(CGraphic *g)
{
	if (g->File.empty()) {
		return -1;
	}
	const std::string name = LibraryFileName(g->File.c_str());
	if (name.empty()) {
		return -1;
	}
	CFile fp;

	if (fp.open(name.c_str(), CL_OPEN_READ) == -1) {
		perror("Can't open file");
		return -1;
	}

	// Create the PNG loading context structure
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr) {
		fprintf(stderr, "Couldn't allocate memory for PNG file");
		return -1;
	}
	// Clean png_ptr on exit
	AutoPng_read_structp pngRaii(png_ptr);

	// Allocate/initialize the memory for image information.  REQUIRED.
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr) {
		fprintf(stderr, "Couldn't create image information for PNG file");
		return -1;
	}
	pngRaii.setInfo(info_ptr);

	/* Set error handling if you are using setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in png_create_read_struct() earlier.
	 */
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error reading the PNG file.\n");
		return -1;
	}

	/* Set up the input control */
	png_set_read_fn(png_ptr, &fp, CL_png_read_data);

	/* Read PNG header info */
	png_uint_32 width;
	png_uint_32 height;
	int bit_depth;
	int color_type;
	int interlace_type;

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
				 &color_type, &interlace_type, nullptr, nullptr);

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr) ;

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* scale greyscale values to the range 0..255 */
	if (color_type == PNG_COLOR_TYPE_GRAY) {
		png_set_expand(png_ptr);
	}

	/* For images with a single "transparent colour", set colour key;
	 if more than one index has transparency, or if partially transparent
	 entries exist, use full alpha channel */
	png_color_16 *transv = nullptr;
	volatile int ckey = -1;

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		int num_trans;
		png_bytep trans;

		png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &transv);
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			/* Check if all tRNS entries are opaque except one */
			int i;
			int t = -1;
			for (i = 0; i < num_trans; ++i) {
				if (trans[i] == 0) {
					if (t >= 0) {
						break;
					}
					t = i;
				} else if (trans[i] != 255) {
					break;
				}
			}
			if (i == num_trans) {
				/* exactly one transparent index */
				ckey = t;
			} else {
				/* more than one transparent index, or translucency */
				png_set_expand(png_ptr);
			}
		} else {
			ckey = 0; /* actual value will be set later */
		}
	}

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
				 &color_type, &interlace_type, nullptr, nullptr);

	/* Allocate the SDL surface to hold the image */
	Uint32 Rmask = 0;
	Uint32 Gmask = 0;
	Uint32 Bmask = 0;
	Uint32 Amask = 0;

	if (color_type != PNG_COLOR_TYPE_PALETTE) {
		if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
			Rmask = 0x000000FF;
			Gmask = 0x0000FF00;
			Bmask = 0x00FF0000;
			Amask = (png_get_channels(png_ptr, info_ptr) == 4) ? 0xFF000000 : 0;
		} else {
			const int s = (png_get_channels(png_ptr, info_ptr) == 4) ? 0 : 8;
			Rmask = 0xFF000000 >> s;
			Gmask = 0x00FF0000 >> s;
			Bmask = 0x0000FF00 >> s;
			Amask = 0x000000FF >> s;
		}
	}
	SDL_Surface *surface =
		SDL_AllocSurface(SDL_SWSURFACE, width, height,
						 bit_depth * png_get_channels(png_ptr, info_ptr), Rmask, Gmask, Bmask, Amask);
	if (surface == nullptr) {
		fprintf(stderr, "Out of memory");
		return -1;
	}

	if (ckey != -1) {
		if (color_type != PNG_COLOR_TYPE_PALETTE) {
			/* FIXME: Should these be truncated or shifted down? */
			ckey = SDL_MapRGB(surface->format, (Uint8)transv->red, (Uint8)transv->green, (Uint8)transv->blue);
		}
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY, ckey);
	}

	/* Create the array of pointers to image data */
	std::vector<png_bytep> row_pointers;
	row_pointers.resize(height);

	for (int i = 0; i < (int)height; ++i) {
		row_pointers[i] = (png_bytep)(Uint8 *)surface->pixels + i * surface->pitch;
	}

	/* Read the entire image in one go */
	png_read_image(png_ptr, &row_pointers[0]);

	/* read rest of file, get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* Load the palette, if any */
	SDL_Palette *palette = surface->format->palette;
	if (palette) {
		if (color_type == PNG_COLOR_TYPE_GRAY) {
			palette->ncolors = 256;
			for (int i = 0; i < 256; ++i) {
				palette->colors[i].r = i;
				palette->colors[i].g = i;
				palette->colors[i].b = i;
			}
		} else {
			png_colorp pngpalette;
			int num_palette;
			png_get_PLTE(png_ptr, info_ptr, &pngpalette, &num_palette);
			if (num_palette > 0) {
				palette->ncolors = num_palette;
				for (int i = 0; i < num_palette; ++i) {
					palette->colors[i].b = pngpalette[i].blue;
					palette->colors[i].g = pngpalette[i].green;
					palette->colors[i].r = pngpalette[i].red;
				}
			}
		}
	}

	g->Surface = surface;
	g->GraphicWidth = surface->w;
	g->GraphicHeight = surface->h;

	fp.close();
	return 0;
}

/**
**  Save a screenshot to a PNG file.
**
**  @param name  PNG filename to save.
*/
void SaveScreenshotPNG(const char *name)
{
	FILE *fp = fopen(name, "wb");
	if (fp == nullptr) {
		return;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr) {
		fclose(fp);
		return;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, nullptr);
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return;
	}

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	int pngw, pngh;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		pngw = Video.ViewportWidth;
		pngh = Video.ViewportHeight;
	}
	else
#endif
	{
		pngw = Video.Width;
		pngh = Video.Height;
	}
	png_set_IHDR(png_ptr, info_ptr, pngw, pngh, 8,
				 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	Video.LockScreen();

	png_write_info(png_ptr, info_ptr);

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glPixelStorei(GL_PACK_ALIGNMENT, 1); //allows screenshots of resolution widths that aren't multiples of 4
		unsigned char* pixels = new unsigned char[Video.ViewportWidth * Video.ViewportHeight * 3];
		if (GLShaderPipelineSupported) {
			// switch to real display
			glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		}
#ifdef USE_OPENGL
		glReadBuffer(GL_FRONT);
#endif
		glReadPixels(0, 0, Video.ViewportWidth, Video.ViewportHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		if (GLShaderPipelineSupported) {
			// switch back to framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER_EXT, fullscreenFramebuffer);
		}
		for (int i = 0; i < Video.ViewportHeight; ++i) {
			png_write_row(png_ptr, &pixels[(Video.ViewportHeight - 1 - i) * Video.ViewportWidth * 3]);
		}
	} else
#endif
	{
		std::vector<unsigned char> row;
		SDL_PixelFormat *fmt = TheScreen->format;

		row.resize(Video.Width * 3);
		for (int i = 0; i < Video.Height; ++i) {
			switch (Video.Depth) {
				case 15:
				case 16: {
					for (int j = 0; j < Video.Width; ++j) {
						Uint16 c = ((Uint16 *)TheScreen->pixels)[j + i * Video.Width];
						row[j * 3 + 0] = ((c & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss;
						row[j * 3 + 1] = ((c & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss;
						row[j * 3 + 2] = ((c & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss;
					}
					break;
				}
				case 24: {
					memcpy(&row[0], (char *)TheScreen->pixels + i * Video.Width, Video.Width * 3);
					break;
				}
				case 32: {
					for (int j = 0; j < Video.Width; ++j) {
						Uint32 c = ((Uint32 *)TheScreen->pixels)[j + i * Video.Width];
						row[j * 3 + 0] = ((c & fmt->Rmask) >> fmt->Rshift);
						row[j * 3 + 1] = ((c & fmt->Gmask) >> fmt->Gshift);
						row[j * 3 + 2] = ((c & fmt->Bmask) >> fmt->Bshift);
					}
					break;
				}
			}
			png_write_row(png_ptr, &row[0]);
		}
	}

	png_write_end(png_ptr, info_ptr);

	Video.UnlockScreen();

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}

/**
**  Save a whole map to a PNG file.
**
**  @param name  PNG filename to save.
*/
void SaveMapPNG(const char *name)
{
	FILE *fp = fopen(name, "wb");
	if (fp == nullptr) {
		return;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr) {
		fclose(fp);
		return;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, nullptr);
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return;
	}

	const size_t imageWidth = UI.CurrentMapLayer->Width * Map.GetCurrentPixelTileSize().x;
	const size_t imageHeight = UI.CurrentMapLayer->Height * Map.GetCurrentPixelTileSize().y;

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, imageWidth, imageHeight, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	SDL_Surface *mapImage = SDL_CreateRGBSurface(SDL_SWSURFACE,
		imageWidth, imageHeight, 32, RMASK, GMASK, BMASK, 0);

	for (int i = 0; i < UI.CurrentMapLayer->Height; ++i) {
		for (int j = 0; j < UI.CurrentMapLayer->Width; ++j) {
			const CMapField &mf = *UI.CurrentMapLayer->Field(i, j);
			SDL_Rect srcRect, dstRect;
			//Wyrmgus start
			/*
			unsigned short int tile = mf.getGraphicTile();

			srcRect.x = Map.TileGraphic->frame_map[tile].x;
			srcRect.y = Map.TileGraphic->frame_map[tile].y;
			*/
			const CTerrainType *terrain = mf.OverlayTerrain ? mf.OverlayTerrain : mf.Terrain;
			unsigned short int tile = mf.OverlayTerrain ? mf.OverlaySolidTile : mf.SolidTile;

			srcRect.x = terrain->GetGraphics()->frame_map[tile].x;
			srcRect.y = terrain->GetGraphics()->frame_map[tile].y;
			//Wyrmgus end
			dstRect.x = i * Map.GetCurrentPixelTileSize().x;
			dstRect.y = j * Map.GetCurrentPixelTileSize().y;
			srcRect.w = dstRect.w = Map.GetCurrentPixelTileSize().x;
			srcRect.h = dstRect.h = Map.GetCurrentPixelTileSize().y;
			//Wyrmgus start
//			SDL_BlitSurface(Map.TileGraphic->Surface, &srcRect, mapImage, &dstRect);
			SDL_BlitSurface(terrain->GetGraphics()->Surface, &srcRect, mapImage, &dstRect);
			//Wyrmgus end
		}
	}

	SDL_LockSurface(mapImage);
	unsigned char *row = new unsigned char[imageWidth * 3];
	for (size_t i = 0; i < imageHeight; ++i) {
		for (size_t j = 0; j < imageWidth; ++j) {
			Uint32 c = ((Uint32 *)mapImage->pixels)[j + i * imageWidth];
			row[j * 3 + 0] = ((c & RMASK) >> RSHIFT);
			row[j * 3 + 1] = ((c & GMASK) >> GSHIFT);
			row[j * 3 + 2] = ((c & BMASK) >> BSHIFT);
		}
		png_write_row(png_ptr, row);
	}
	delete[] row;

	png_write_end(png_ptr, info_ptr);

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);
	SDL_UnlockSurface(mapImage);
	SDL_FreeSurface(mapImage);

	fclose(fp);
}

//Wyrmgus start
/**
**  Convert a map template's terrain file to a PNG one.
**
**  @param name  PNG filename to save.
*/
void SaveMapTemplatePNG(const char *name, const CMapTemplate *map_template, const bool overlay)
{
	bool use_terrain_file = true;
	std::string terrain_file;
	if (overlay) {
		terrain_file = map_template->OverlayTerrainFile;
	} else {
		terrain_file = map_template->TerrainFile;
	}
	
	if (terrain_file.empty()) {
		use_terrain_file = false;
	}
	
	FILE *fp = fopen(name, "wb");
	if (fp == nullptr) {
		return;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr) {
		fclose(fp);
		return;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, nullptr);
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return;
	}

	const size_t imageWidth = map_template->Width;
	const size_t imageHeight = map_template->Height;

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, imageWidth, imageHeight, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	unsigned char *row = new unsigned char[imageWidth * 3];
	
	if (use_terrain_file) {
		const std::string terrain_filename = LibraryFileName(terrain_file.c_str());
			
		if (!CanAccessFile(terrain_filename.c_str())) {
			fprintf(stderr, "File \"%s\" not found.\n", terrain_filename.c_str());
		}
		
		std::ifstream is_map(terrain_filename);
		
		std::string line_str;
		int y = 0;
		while (std::getline(is_map, line_str))
		{
			int x = 0;
			
			for (unsigned int i = 0; i < line_str.length(); ++i) {
				std::string terrain_character = line_str.substr(i, 1);
				CTerrainType *terrain = nullptr;
				if (CTerrainType::TerrainTypesByCharacter.find(terrain_character) != CTerrainType::TerrainTypesByCharacter.end()) {
					terrain = CTerrainType::TerrainTypesByCharacter.find(terrain_character)->second;
				}
				Uint8 red = 0;
				Uint8 green = 0;
				Uint8 blue = 0;
				if (terrain) {
					red = terrain->Color.R;
					green = terrain->Color.G;
					blue = terrain->Color.B;
				}
				
				row[x * 3 + 0] = red;
				row[x * 3 + 1] = green;
				row[x * 3 + 2] = blue;

				x += 1;
			}
			
			png_write_row(png_ptr, row);
			
			y += 1;
		}
	} else {
		std::map<int, std::map<int, CTerrainType *>> terrain_map;
		for (size_t i = 0; i < map_template->HistoricalTerrains.size(); ++i) {
			if (std::get<2>(map_template->HistoricalTerrains[i]).Year == 0) {
				Vec2i terrain_pos = std::get<0>(map_template->HistoricalTerrains[i]);
				CTerrainType *terrain_type = std::get<1>(map_template->HistoricalTerrains[i]);
				if (terrain_type->Overlay == overlay) {
					terrain_map[terrain_pos.y][terrain_pos.x] = terrain_type;
				}
			}
		}
		
		for (int y = 0; y < map_template->Height; ++y) {
			for (int x = 0; x < map_template->Width; ++x) {
				row[x * 3 + 0] = 0;
				row[x * 3 + 1] = 0;
				row[x * 3 + 2] = 0;
			}
			
			for (std::map<int, CTerrainType *>::iterator iterator = terrain_map[y].begin(); iterator != terrain_map[y].end(); ++iterator) {
				int x = iterator->first;
				CTerrainType *terrain_type = iterator->second;
				
				Uint8 red = 0;
				Uint8 green = 0;
				Uint8 blue = 0;
				if (terrain_type) {
					red = terrain_type->Color.R;
					green = terrain_type->Color.G;
					blue = terrain_type->Color.B;
				}
				
				row[x * 3 + 0] = red;
				row[x * 3 + 1] = green;
				row[x * 3 + 2] = blue;
			}
			
			png_write_row(png_ptr, row);
		}
	}
	
	delete[] row;

	png_write_end(png_ptr, info_ptr);

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}
//Wyrmgus end
