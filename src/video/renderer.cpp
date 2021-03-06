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
//      (c) Copyright 2021 by Andrettin
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

#include "video/renderer.h"

#include "util/point_util.h"
#include "video/frame_buffer_object.h"
#include "video/render_context.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QQuickWindow>

namespace wyrmgus {

renderer::~renderer()
{
	//run the OpenGL commands one more time, so that free texture commands are run
	render_context::get()->free_textures();
}

QOpenGLFramebufferObject *renderer::createFramebufferObject(const QSize &size)
{
	QOpenGLFramebufferObjectFormat format;
	format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	return new QOpenGLFramebufferObject(size, format);
}

void renderer::render()
{
	this->init_opengl();

	//run the posted OpenGL commands
	render_context::get()->run(this);

	this->fbo->window()->resetOpenGLState();
}

QSizeF renderer::get_target_sizef() const
{
	return this->fbo->size();
}

void renderer::blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const int frame_index, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent)
{
	const int frames_per_row = size.width() / frame_size.width();
	const QPoint frame_pos = point::from_index(frame_index, frames_per_row);
	const QPoint frame_pixel_pos(frame_pos.x() * frame_size.width(), frame_pos.y() * frame_size.height());
	this->blit_texture_frame(texture, pos, frame_pixel_pos, frame_size, flip, opacity, show_percent, frame_size);
}

}
