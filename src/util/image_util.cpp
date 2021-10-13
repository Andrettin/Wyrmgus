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

#include "stratagus.h"

#include "util/image_util.h"

#include "util/angle_util.h"
#include "util/container_util.h"
#include "util/fractional_int.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/size_util.h"
#include "util/thread_pool.h"
#include "xbrz.h"

namespace wyrmgus::image {

static void copy_frame_data(const uint32_t *src_frame_data, uint32_t *dst_data, const QSize &frame_size, const int frame_x, const int frame_y, const int dst_width)
{
	//BPP is assumed to be 4, hence why uint32_t buffers are used

	const int frame_width = frame_size.width();
	const int frame_height = frame_size.height();

	for (int x = 0; x < frame_width; ++x) {
		for (int y = 0; y < frame_height; ++y) {
			const int frame_pixel_index = y * frame_width + x;
			const int pixel_x = frame_x * frame_width + x;
			const int pixel_y = frame_y * frame_height + y;
			const int pixel_index = pixel_y * dst_width + pixel_x;
			dst_data[pixel_index] = src_frame_data[frame_pixel_index];
		}
	}
}

QImage scale(const QImage &src_image, const centesimal_int &scale_factor)
{
	if (src_image.format() != QImage::Format_RGBA8888) {
		const QImage reformatted_src_image = src_image.convertToFormat(QImage::Format_RGBA8888);
		return image::scale(reformatted_src_image, scale_factor);
	}

	int scale_multiplier = scale_factor.to_int();
	if (scale_factor.get_fractional_value() != 0) {
		if (scale_factor.get_fractional_value() == 50) {
			scale_multiplier = (scale_factor * 2).to_int();
		} else {
			scale_multiplier += 1;
		}
	}

	QImage result_image;

	if (scale_multiplier > 1) {
		result_image = QImage(src_image.size() * scale_multiplier, QImage::Format_RGBA8888);
		const unsigned char *src_data = src_image.constBits();
		unsigned char *dst_data = result_image.bits();

		xbrz::scale(scale_multiplier, reinterpret_cast<const uint32_t *>(src_data), reinterpret_cast<uint32_t *>(dst_data), src_image.width(), src_image.height());
	} else {
		result_image = src_image;
	}

	if (scale_factor.get_fractional_value() != 0) {
		const QSize scaled_size = src_image.size() * scale_factor;
		result_image = result_image.scaled(scaled_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_RGBA8888);
	}

	return result_image;
}

QImage scale(const QImage &src_image, const centesimal_int &scale_factor, const QSize &old_frame_size)
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

	if (bpp != 4) {
		throw std::runtime_error("BPP must be 4 when scaling an image.");
	}

	const QSize result_size = src_image.size() * scale_factor;
	QImage result_image(result_size, QImage::Format_RGBA8888);

	if (result_image.isNull()) {
		throw std::runtime_error("Failed to allocate image to be scaled.");
	}

	uint32_t *dst_data = reinterpret_cast<uint32_t *>(result_image.bits());

	//if a simple scale factor is being used for the resizing, then use xBRZ for the rescaling
	const int horizontal_frame_count = src_image.width() / old_frame_size.width();
	const int vertical_frame_count = src_image.height() / old_frame_size.height();

	//scale each frame individually
	std::vector<std::future<void>> futures;
	const int result_width = result_size.width();

	for (int frame_x = 0; frame_x < horizontal_frame_count; ++frame_x) {
		for (int frame_y = 0; frame_y < vertical_frame_count; ++frame_y) {
			std::future<void> future = thread_pool::get()->async([&, frame_x, frame_y]() {
				const QImage result_frame_image = image::scale_frame(src_image, frame_x, frame_y, scale_factor, old_frame_size);

				const uint32_t *frame_data = reinterpret_cast<const uint32_t *>(result_frame_image.constBits());
				image::copy_frame_data(frame_data, dst_data, new_frame_size, frame_x, frame_y, result_width);
			});

			futures.push_back(std::move(future));
		}
	}

	for (std::future<void> &future : futures) {
		future.wait();
	}

	return result_image;
}

std::set<QRgb> get_rgbs(const QImage &image)
{
	if (!image.colorTable().empty()) {
		return container::to_set(image.colorTable());
	}

	std::set<QRgb> rgb_set;

	const unsigned char *image_data = image.constBits();
	const int pixel_count = image.width() * image.height();
	const int bpp = image.depth() / 8;

	static constexpr int red_index = 0;
	static constexpr int green_index = 1;
	static constexpr int blue_index = 2;
	static constexpr int alpha_index = 3;

	if (image.format() != QImage::Format_RGBA8888 && image.format() != QImage::Format_RGB888) {
		throw std::runtime_error("Invalid image format for image::get_rgbs: \"" + std::to_string(image.format()) + "\".");
	}

	if (bpp == 4) {
		for (int i = 0; i < pixel_count; ++i) {
			const int pixel_index = i * bpp;
			const unsigned char &red = image_data[pixel_index + red_index];
			const unsigned char &green = image_data[pixel_index + green_index];
			const unsigned char &blue = image_data[pixel_index + blue_index];
			const unsigned char &alpha = image_data[pixel_index + alpha_index];
			rgb_set.insert(qRgba(red, green, blue, alpha));
		}
	} else if (bpp == 3) {
		for (int i = 0; i < pixel_count; ++i) {
			const int pixel_index = i * bpp;
			const unsigned char &red = image_data[pixel_index + red_index];
			const unsigned char &green = image_data[pixel_index + green_index];
			const unsigned char &blue = image_data[pixel_index + blue_index];
			rgb_set.insert(qRgba(red, green, blue, 255));
		}
	}

	return rgb_set;
}

color_set get_colors(const QImage &image)
{
	color_set color_set;

	const std::set<QRgb> rgb_set = image::get_rgbs(image);
	for (const QRgb rgb : rgb_set) {
		QColor color;
		color.setRgba(rgb);
		color_set.insert(std::move(color));
	}

	return color_set;
}

int get_frame_index(const QImage &image, const QSize &frame_size, const QPoint &frame_pos)
{
	return wyrmgus::point::to_index(frame_pos, image::get_frames_per_row(image, frame_size.width()));
}

QPoint get_frame_pos(const QImage &image, const QSize &frame_size, const int frame_index, const frame_order frame_order)
{
	if (frame_order == frame_order::top_to_bottom) {
		const int frames_per_column = image::get_frames_per_column(image, frame_size.height());
		return QPoint(frame_index / frames_per_column, frame_index % frames_per_column);
	} else {
		//left to right
		return point::from_index(frame_index, image::get_frames_per_row(image, frame_size.width()));
	}
}

void pack_folder(const std::filesystem::path &dir_path, const frame_order frame_order, const int frames_per_row)
{
	const std::filesystem::directory_iterator dir_iterator(dir_path);

	int frame_count = 0;
	QSize frame_size;

	std::set<std::filesystem::path> frame_image_paths;

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_regular_file()) {
			continue;
		}

		frame_image_paths.insert(dir_entry.path());
	}

	for (const std::filesystem::path &frame_image_path : frame_image_paths) {
		try {
			const QImage frame_image(path::to_qstring(frame_image_path));

			if (frame_image.isNull()) {
				continue;
			}

			frame_count++;

			if (!frame_size.isValid()) {
				frame_size = frame_image.size();
			} else {
				if (frame_image.size() != frame_size) {
					throw std::runtime_error("Inconsistent frame size when packing image files in directory \"" + dir_path.string() + "\": the frame size of the first image file is " + size::to_string(frame_size) + ", but that of image file \"" + frame_image_path.string() + "\" is " + size::to_string(frame_image.size()) + ".");
				}
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to preprocess frame image \"" + frame_image_path.string() + "\"."));
		}
	}

	const int frames_per_column = frame_count / frames_per_row;
	QImage image(frame_size.width() * frames_per_row, frame_size.height() * frames_per_column, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	int frame_index = 0;
	for (const std::filesystem::path &frame_image_path : frame_image_paths) {
		try {
			QImage frame_image(path::to_qstring(frame_image_path));

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
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to process frame image \"" + frame_image_path.string() + "\"."));
		}
	}

	image.save(path::to_qstring(dir_path) + ".png");
}

QImage crop_frames(const QImage &src_image, const QSize &src_frame_size, const QSize &min_frame_size)
{
	if (src_image.format() != QImage::Format_RGBA8888) {
		const QImage reformatted_src_image = src_image.convertToFormat(QImage::Format_RGBA8888);
		return image::crop_frames(reformatted_src_image, src_frame_size, min_frame_size);
	}

	const int bpp = src_image.depth() / 8;

	if (bpp != 4) {
		throw std::runtime_error("BPP must be 4 when cropping the frames of an image.");
	}

	//crop empty space in the image's frames
	const int horizontal_frame_count = src_image.width() / src_frame_size.width();
	const int vertical_frame_count = src_image.height() / src_frame_size.height();

	QPoint min_pos(-1, -1);
	QPoint max_pos(-1, -1);

	const int frame_width = src_frame_size.width();
	const int frame_height = src_frame_size.height();

	int src_filled_pixel_count = 0;

	//scale each frame individually
	for (int frame_x = 0; frame_x < horizontal_frame_count; ++frame_x) {
		for (int frame_y = 0; frame_y < vertical_frame_count; ++frame_y) {
			const QImage frame_image = image::get_frame(src_image, frame_x, frame_y, src_frame_size);

			for (int x = 0; x < frame_width; ++x) {
				for (int y = 0; y < frame_height; ++y) {
					if (frame_image.pixelColor(x, y).alpha() == 0) {
						//empty pixel
						continue;
					}

					++src_filled_pixel_count;

					if (min_pos.x() == -1 || x < min_pos.x()) {
						min_pos.setX(x);
					}

					if (min_pos.y() == -1 || y < min_pos.y()) {
						min_pos.setY(y);
					}

					if (max_pos.x() == -1 || x > max_pos.x()) {
						max_pos.setX(x);
					}

					if (max_pos.y() == -1 || y > max_pos.y()) {
						max_pos.setY(y);
					}
				}
			}
		}
	}

	const int left_margin = min_pos.x();
	const int right_margin = frame_width - 1 - max_pos.x();
	const int top_margin = min_pos.y();
	const int bottom_margin = frame_height - 1 - max_pos.y();

	const int margin = std::min(std::min(left_margin, right_margin), std::min(top_margin, bottom_margin));

	if (margin == 0) {
		//nothing to do
		return src_image;
	}

	const int new_frame_width = std::max(src_frame_size.width() - margin * 2, min_frame_size.width());
	const int new_frame_height = std::max(src_frame_size.height() - margin * 2, min_frame_size.height());
	const QSize new_frame_size(new_frame_width, new_frame_height);

	const QSize result_size = src_image.size() * new_frame_size / src_frame_size;
	QImage result_image(result_size, QImage::Format_RGBA8888);

	if (result_image.isNull()) {
		throw std::runtime_error("Failed to allocate result image for frame cropping.");
	}

	uint32_t *dst_data = reinterpret_cast<uint32_t *>(result_image.bits());
	const int result_width = result_size.width();

	//scale each frame individually
	for (int frame_x = 0; frame_x < horizontal_frame_count; ++frame_x) {
		for (int frame_y = 0; frame_y < vertical_frame_count; ++frame_y) {
			const QImage frame_image = image::get_frame(src_image, frame_x, frame_y, src_frame_size);
			const QImage result_frame_image = frame_image.copy(margin, margin, new_frame_size.width(), new_frame_size.height());

			const uint32_t *frame_data = reinterpret_cast<const uint32_t *>(result_frame_image.constBits());
			image::copy_frame_data(frame_data, dst_data, new_frame_size, frame_x, frame_y, result_width);
		}
	}

	int result_filled_pixel_count = 0;

	image::for_each_pixel_pos(result_image, [&result_image, &result_filled_pixel_count](const int x, const int y) {
		if (result_image.pixelColor(x, y).alpha() == 0) {
			//empty pixel
			return;
		}

		++result_filled_pixel_count;
	});

	if (src_filled_pixel_count != result_filled_pixel_count) {
		throw std::runtime_error("The amount of filled pixels in the source image (" + std::to_string(src_filled_pixel_count) + ") is different than that in the result image (" + std::to_string(result_filled_pixel_count) + ") when cropping frames.");
	}

	return result_image;
}

void index_to_palette(QImage &image, const color_set &palette)
{
	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			const QColor pixel_color = image.pixelColor(x, y);

			if (palette.contains(pixel_color)) {
				continue;
			}

			//if the pixel's color is not present in the palette, pick the closest color in it, RGB-value-wise

			QColor best_color;
			int best_rgb_difference = -1;
			for (const QColor &palette_color : palette) {
				int rgb_difference = 0;
				rgb_difference += std::abs(pixel_color.red() - palette_color.red());
				rgb_difference += std::abs(pixel_color.green() - palette_color.green());
				rgb_difference += std::abs(pixel_color.blue() - palette_color.blue());

				if (best_rgb_difference == -1 || rgb_difference < best_rgb_difference) {
					best_rgb_difference = rgb_difference;
					best_color = palette_color;
				}
			}

			image.setPixelColor(x, y, best_color);
		}
	}
}

void rotate_hue(QImage &image, const double degrees, const color_set &ignored_colors)
{
	//rotate the RGB color cube for the image by a certain amount of degrees
	const double radians = angle::degrees_to_radians(degrees);
	const double cos = std::cos(radians);
	const double sin = std::sin(radians);

	static constexpr double third = 1. / 3.;
	const double c = cos * third;
	const double c2 = c * 2;
	const double s = sqrt(third) * sin;

	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			QColor pixel_color = image.pixelColor(x, y);

			if (ignored_colors.contains(pixel_color)) {
				continue;
			}

			const int old_red = pixel_color.red();
			const int old_green = pixel_color.green();
			const int old_blue = pixel_color.blue();

			const int red = static_cast<int>(old_red * (third + c2) + old_green * (third - c - s) + old_blue * (third - c + s));
			const int green = static_cast<int>(old_red * (third - c + s) + old_green * (third + c2) + old_blue * (third - c - s));
			const int blue = static_cast<int>(old_red * (third - c - s) + old_green * (third - c + s) + old_blue * (third + c2));

			pixel_color.setRed(std::clamp(red, 0, 255));
			pixel_color.setGreen(std::clamp(green, 0, 255));
			pixel_color.setBlue(std::clamp(blue, 0, 255));

			image.setPixelColor(x, y, pixel_color);
		}
	}
}

}
