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
//      (c) Copyright 2020-2021 by Andrettin
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

#pragma once

#include "util/color_container.h"

namespace wyrmgus {
	template <int N>
	class fractional_int;

	using centesimal_int = fractional_int<2>;
}

namespace wyrmgus::image {

enum class frame_order {
	left_to_right,
	top_to_bottom
};

inline QImage get_frame(const QImage &image, const int frame_x, const int frame_y, const QSize &frame_size)
{
	const int pixel_x = frame_x * frame_size.width();
	const int pixel_y = frame_y * frame_size.height();
	return image.copy(pixel_x, pixel_y, frame_size.width(), frame_size.height());
}

extern QImage scale(const QImage &src_image, const centesimal_int &scale_factor);
extern QImage scale(const QImage &src_image, const centesimal_int &scale_factor, const QSize &old_frame_size);

inline QImage scale_frame(const QImage &image, const int frame_x, const int frame_y, const centesimal_int &scale_factor, const QSize &old_frame_size)
{
	const QImage frame_image = image::get_frame(image, frame_x, frame_y, old_frame_size);

	return image::scale(frame_image, scale_factor);
}

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
extern QPoint get_frame_pos(const QImage &image, const QSize &frame_size, const int frame_index, const frame_order frame_order = frame_order::left_to_right);

inline std::vector<QImage> to_frames(const QImage &image, const QSize &frame_size)
{
	std::vector<QImage> frames;

	const int horizontal_frame_count = image::get_frames_per_row(image, frame_size.width());
	const int vertical_frame_count = image::get_frames_per_column(image, frame_size.height());

	for (int frame_y = 0; frame_y < vertical_frame_count; ++frame_y) {
		for (int frame_x = 0; frame_x < horizontal_frame_count; ++frame_x) {
			frames.push_back(image::get_frame(image, frame_x, frame_y, frame_size));
		}
	}

	return frames;
}

extern void pack_folder(const std::filesystem::path &dir_path, const frame_order frame_order, const int frames_per_row);
extern QImage crop_frames(const QImage &src_image, const QSize &src_frame_size, const QSize &min_size);

extern void index_to_palette(QImage &image, const color_set &palette);

inline void index_to_image_palette(QImage &image, const QImage &other_image)
{
	image::index_to_palette(image, image::get_colors(other_image));
}

extern void rotate_hue(QImage &image, const double degrees, const color_set &ignored_colors);

template <typename function_type>
inline void for_each_pixel_pos(const QImage &image, const function_type &function)
{
	for (int y = 0; y < image.height(); ++y) {
		for (int x = 0; x < image.width(); ++x) {
			function(x, y);
		}
	}
}

}
