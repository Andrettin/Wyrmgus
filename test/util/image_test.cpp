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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/fractional_int.h"
#include "util/thread_pool.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(scale_image_test)
{
	QImage image(128, 128, QImage::Format_RGBA8888);
	image.fill(Qt::black);

	for (int scale_factor = 2; scale_factor <= 5; ++scale_factor) {
		const QImage scaled_image = image::scale(image, centesimal_int(scale_factor));

		BOOST_CHECK(scaled_image.width() == image.width() * scale_factor);
		BOOST_CHECK(scaled_image.height() == image.height() * scale_factor);
	}
}

BOOST_AUTO_TEST_CASE(scale_frame_image_test)
{
	thread_pool::get()->co_spawn_sync([]() -> boost::asio::awaitable<void> {
		QImage image(360, 864, QImage::Format_RGBA8888);
		image.fill(Qt::black);

		const QSize frame_size(72, 72);

		for (int scale_factor = 2; scale_factor <= 5; ++scale_factor) {
			const QImage scaled_image = co_await image::scale(image, centesimal_int(scale_factor), frame_size);

			BOOST_CHECK(scaled_image.width() == image.width() * scale_factor);
			BOOST_CHECK(scaled_image.height() == image.height() * scale_factor);
		}
	});
}
