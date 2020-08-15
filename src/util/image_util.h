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
//      (c) Copyright 2020 by Andrettin
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

#pragma once

#include "util/color_container.h"

namespace wyrmgus::image {

enum class frame_order {
	left_to_right,
	top_to_bottom
};

extern QImage scale(const QImage &src_image, const int scale_factor);
extern QImage scale(const QImage &src_image, const int scale_factor, const QSize &old_frame_size);
extern std::set<QRgb> get_rgbs(const QImage &image);
extern color_set get_colors(const QImage &image);

inline int get_frames_per_row(const QImage &image, const int frame_width)
{
	return image.width() / frame_width;
}

inline int get_frames_per_column(const QImage &image, const int frame_height)
{
	return image.height() / frame_height;
}

extern int get_frame_index(const QImage &image, const QSize &frame_size, const QPoint &frame_pos);
extern QPoint get_frame_pos(const QImage &image, const QSize &frame_size, const int frame_index, const frame_order frame_order);

inline void pack_folder(const std::filesystem::path &dir_path, const frame_order frame_order)
{
	static constexpr int frames_per_row = 5;

	std::filesystem::directory_iterator dir_iterator(dir_path);

	int frame_count = 0;
	QSize frame_size;

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_regular_file()) {
			continue;
		}

		const QImage frame_image(QString::fromStdString(dir_entry.path().string()));

		if (frame_image.isNull()) {
			continue;
		}

		frame_count++;

		if (!frame_size.isValid()) {
			frame_size = frame_image.size();
		} else {
			if (frame_image.size() != frame_size) {
				throw std::runtime_error("Inconsistent frame size when packing image files in directory \"" + dir_path.string() + "\": the frame size of the first image file is (" + std::to_string(frame_size.width()) + ", " + std::to_string(frame_size.height()) + "), but of image file \"" + dir_entry.path().string() + "\" is (" + std::to_string(frame_image.width()) + ", " + std::to_string(frame_image.height()) + ").");
			}
		}
	}

	const int frames_per_column = frame_count / frames_per_row;
	QImage image(frame_size.width() * frames_per_row, frame_size.height() * frames_per_column, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	dir_iterator = std::filesystem::directory_iterator(dir_path);

	int frame_index = 0;
	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_regular_file()) {
			continue;
		}

		QImage frame_image(QString::fromStdString(dir_entry.path().string()));

		if (frame_image.isNull()) {
			continue;
		}

		const QPoint frame_pos = image::get_frame_pos(image, frame_size, frame_index, frame_order);
		const QPoint frame_pixel_pos(frame_pos.x() * frame_size.width(), frame_pos.y() * frame_size.height());

		for (int x = 0; x < frame_size.width(); ++x) {
			for (int y = 0; y < frame_size.height(); ++y) {
				const QColor pixel_color = frame_image.pixelColor(x, y);
				const int pixel_x = frame_pixel_pos.x() + x;
				const int pixel_y = frame_pixel_pos.y() + y;
				image.setPixelColor(pixel_x, pixel_y, pixel_color);
			}
		}

		frame_index++;
	}

	image.save(QString::fromStdString(dir_path.string() + ".png"));
}

extern void index_to_palette(QImage &image, const color_set &palette);

inline void index_to_image_palette(QImage &image, const QImage &other_image)
{
	image::index_to_palette(image, image::get_colors(other_image));
}

}
