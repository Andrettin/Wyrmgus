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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Pali Rohár and Andrettin
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
#include "util/image_util.h"
#include "util/path_util.h"
#include "video/video.h"

#pragma warning(push, 0)
#include <QPixmap>
#include <QScreen>
#include <QWindow>
#pragma warning(pop)

/**
**  Load a png graphic file.
**
**  @param g  graphic to load.
**
**  @return   0 for success, -1 for error.
*/
int LoadGraphicPNG(CGraphic *g, const centesimal_int &scale_factor)
{
	std::filesystem::path filepath = LibraryFileName(g->get_filepath().string().c_str());

	//load the image without scaling to get the original size
	g->image = QImage(path::to_qstring(filepath));
	g->original_size = g->get_image().size();

	//if the scale factor is greater than 1, see if there is a file in the same folder with e.g. the "_2x" suffix for the 2x scale factor, and if so, use that
	const std::pair<std::filesystem::path, centesimal_int> suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

	if (!suffix_result.first.empty()) {
		filepath = suffix_result.first;
		g->custom_scale_factor = suffix_result.second;
	}

	if (g->custom_scale_factor != 1) {
		g->image = QImage(path::to_qstring(filepath));
	}

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

	return 0;
}

/**
**  Save a screenshot to a PNG file.
**
**  @param name  PNG filename to save.
*/
void SaveScreenshotPNG(const char *name)
{
	const QWindow *window = QApplication::focusWindow();

	if (window == nullptr) {
		return;
	}

	const QPixmap screen_pixmap = window->screen()->grabWindow(window->winId());
	screen_pixmap.save(name);
}
