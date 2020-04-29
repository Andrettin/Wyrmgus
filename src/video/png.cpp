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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon, Pali Rohár and Andrettin
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

#include "stratagus.h"

#include "database/defines.h"
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

	g->image = QImage(name.c_str());
	if (g->image.isNull()) {
		throw std::runtime_error("Failed to load the \"" + name + "\" image file.");
	}

	g->GraphicWidth = g->get_image().width();
	g->GraphicHeight = g->get_image().height();

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

//Wyrmgus start
/**
**  Convert a map template's terrain file to a PNG one.
**
**  @param name  PNG filename to save.
*/
void save_map_template_png(const char *name, const stratagus::map_template *map_template, const bool overlay)
{
	bool use_terrain_file = true;
	std::filesystem::path terrain_file;
	if (overlay) {
		terrain_file = map_template->get_overlay_terrain_file();
	} else {
		terrain_file = map_template->get_terrain_file();
	}
	
	if (terrain_file.empty()) {
		use_terrain_file = false;
	}
	
	QImage image(map_template->get_size(), QImage::Format_RGBA8888);

	const size_t imageWidth = map_template->get_width();
	const size_t imageHeight = map_template->get_height();

	if (use_terrain_file) {
		const std::string terrain_filename = LibraryFileName(terrain_file.string().c_str());
			
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
				const char terrain_character = line_str.at(i);
				stratagus::terrain_type *terrain = nullptr;
				if (stratagus::terrain_type::TerrainTypesByCharacter.find(terrain_character) != stratagus::terrain_type::TerrainTypesByCharacter.end()) {
					terrain = stratagus::terrain_type::TerrainTypesByCharacter.find(terrain_character)->second;
				}
				Uint8 red = 0;
				Uint8 green = 0;
				Uint8 blue = 0;
				if (terrain) {
					red = terrain->Color.R;
					green = terrain->Color.G;
					blue = terrain->Color.B;
				}
				
				const QColor color(red, green, blue);
				image.setPixelColor(QPoint(x, y), color);

				x += 1;
			}
			
			y += 1;
		}
	} else {
		std::map<int, std::map<int, stratagus::terrain_type *>> terrain_map;
		for (size_t i = 0; i < map_template->HistoricalTerrains.size(); ++i) {
			if (std::get<2>(map_template->HistoricalTerrains[i]).Year == 0) {
				Vec2i terrain_pos = std::get<0>(map_template->HistoricalTerrains[i]);
				stratagus::terrain_type *terrain_type = std::get<1>(map_template->HistoricalTerrains[i]);
				if (terrain_type->Overlay == overlay) {
					terrain_map[terrain_pos.y][terrain_pos.x] = terrain_type;
				}
			}
		}
		
		for (int y = 0; y < map_template->get_height(); ++y) {
			for (int x = 0; x < map_template->get_width(); ++x) {
				image.setPixelColor(QPoint(x, y), QColor(0, 0, 0));
			}
			
			for (std::map<int, stratagus::terrain_type *>::iterator iterator = terrain_map[y].begin(); iterator != terrain_map[y].end(); ++iterator) {
				int x = iterator->first;
				stratagus::terrain_type *terrain_type = iterator->second;
				
				Uint8 red = 0;
				Uint8 green = 0;
				Uint8 blue = 0;
				if (terrain_type) {
					red = terrain_type->Color.R;
					green = terrain_type->Color.G;
					blue = terrain_type->Color.B;
				}
				
				const QColor color(red, green, blue);
				image.setPixelColor(QPoint(x, y), color);
			}
		}
	}

	image.save(name);
}
//Wyrmgus end
