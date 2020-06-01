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
**
**  @param g  graphic to load.
**
**  @return   0 for success, -1 for error.
*/
int LoadGraphicPNG(CGraphic *g, const int scale_factor)
{
	std::filesystem::path filepath = LibraryFileName(g->get_filepath().string().c_str());

	//if the scale factor is greater than 1, see if there is a file in the same folder with e.g. the "_2x" suffix for the 2x scale factor, and if so, use that
	int suffix_scale_factor = scale_factor;
	while (suffix_scale_factor > 1) {
		std::filesystem::path scale_suffix_filepath = filepath;
		scale_suffix_filepath.replace_filename(filepath.stem().string() + "_" + std::to_string(suffix_scale_factor) + "x" + filepath.extension().string());
		if (std::filesystem::exists(scale_suffix_filepath)) {
			filepath = scale_suffix_filepath;
			g->custom_scale_factor = suffix_scale_factor;
			break;
		}
		suffix_scale_factor /= 2;
	}

	g->set_filepath(filepath);
	g->image = QImage(QString::fromStdString(filepath.string()));
	if (g->get_image().isNull()) {
		throw std::runtime_error("Failed to load the \"" + filepath.string() + "\" image file.");
	}

	if (g->get_image().width() == 0) {
		throw std::runtime_error("The \"" + filepath.string() + "\" image has no width.");
	}

	if (g->get_image().height() == 0) {
		throw std::runtime_error("The \"" + filepath.string() + "\" image has no width.");
	}

	const int bpp = g->image.depth() / 8;
	if (bpp == 4 && g->image.format() != QImage::Format_RGBA8888) {
		g->image = g->image.convertToFormat(QImage::Format_RGBA8888);
	} else if (bpp == 3 && g->image.format() != QImage::Format_RGB888) {
		g->image = g->image.convertToFormat(QImage::Format_RGB888);
	}

	g->GraphicWidth = g->get_image().width();
	g->GraphicHeight = g->get_image().height();
	g->original_size = g->get_image().size();

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

	pngw = Video.ViewportWidth;
	pngh = Video.ViewportHeight;

	png_set_IHDR(png_ptr, info_ptr, pngw, pngh, 8,
				 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

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

	png_write_end(png_ptr, info_ptr);

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}
