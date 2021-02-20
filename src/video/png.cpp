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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon, Pali Rohár and Andrettin
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

#include "iolib.h"
#include "iocompat.h"
#include "video/video.h"

#ifdef USE_OPENGL
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <SDL_opengl.h>
#endif

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
	QImage image(Video.ViewportWidth, Video.ViewportHeight, QImage::Format_RGB888);
	image.fill(Qt::transparent);

	glPixelStorei(GL_PACK_ALIGNMENT, 1); //allows screenshots of resolution widths that aren't multiples of 4
#ifdef USE_OPENGL
	glReadBuffer(GL_FRONT);
#endif
	glReadPixels(0, 0, image.width(), image.height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());

	//we need to flip the image vertically, as glReadPixels returns a vertically-inverted image
	const QImage screenshot_image = image.mirrored(false, true);

	screenshot_image.save(name);
}
