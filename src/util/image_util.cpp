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

#include "xbrz.h"

namespace stratagus::image {

QImage scale(const QImage &src_image, const int scale_factor)
{
	if (src_image.format() != QImage::Format_RGBA8888) {
		const QImage reformatted_src_image = src_image.convertToFormat(QImage::Format_RGBA8888);
		return image::scale(reformatted_src_image, scale_factor);
	}

	QImage result_image(src_image.size() * scale_factor, QImage::Format_ARGB32);

	const unsigned char *src_data = src_image.constBits();
	unsigned char *dst_data = result_image.bits();
	xbrz::scale(scale_factor, reinterpret_cast<const uint32_t *>(src_data), reinterpret_cast<uint32_t *>(dst_data), src_image.width(), src_image.height());

	return result_image;
}

QImage scale(const QImage &src_image, const int scale_factor, const QSize &old_frame_size)
{
	if (src_image.format() != QImage::Format_RGBA8888) {
		const QImage reformatted_src_image = src_image.convertToFormat(QImage::Format_RGBA8888);
		return image::scale(reformatted_src_image, scale_factor, old_frame_size);
	}

	if (src_image.size() == old_frame_size) {
		return image::scale(src_image, scale_factor); //image has only one frame
	}

	//scale an image with xBRZ
	const QSize new_frame_size = old_frame_size * scale_factor;

	const int bpp = src_image.depth() / 8;
	QImage result_image(src_image.size() * scale_factor, QImage::Format_ARGB32);
	unsigned char *dst_data = result_image.bits();

	//if a simple scale factor is being used for the resizing, then use xBRZ for the rescaling
	const int horizontal_frame_count = src_image.width() / old_frame_size.width();
	const int vertical_frame_count = src_image.height() / old_frame_size.height();

	//scale each frame individually
	for (int frame_x = 0; frame_x < horizontal_frame_count; ++frame_x) {
		for (int frame_y = 0; frame_y < vertical_frame_count; ++frame_y) {
			const QImage src_frame_image = src_image.copy(frame_x * old_frame_size.width(), frame_y * old_frame_size.height(), old_frame_size.width(), old_frame_size.height());
			QImage result_frame_image = image::scale(src_frame_image, scale_factor);

			const unsigned char *frame_data = result_frame_image.constBits();

			for (int x = 0; x < new_frame_size.width(); ++x) {
				for (int y = 0; y < new_frame_size.height(); ++y) {
					const int frame_pixel_index = y * new_frame_size.width() + x;
					const int pixel_x = frame_x * new_frame_size.width() + x;
					const int pixel_y = frame_y * new_frame_size.height() + y;
					const int pixel_index = pixel_y * result_image.width() + pixel_x;
					for (int i = 0; i < bpp; ++i) {
						dst_data[pixel_index * bpp + i] = frame_data[frame_pixel_index * bpp + i];
					}
				}
			}
		}
	}

	return result_image;
}

}
