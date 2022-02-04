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

#include "video/renderer.h"

#include "util/point_util.h"
#include "video/frame_buffer_object.h"
#include "video/render_context.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QQuickWindow>

namespace wyrmgus {

renderer::renderer(const frame_buffer_object *fbo) : fbo(fbo)
{
	this->blitter.create();
}

renderer::~renderer()
{
	//run the OpenGL commands one more time, so that free texture commands are run
	render_context::get()->run_free_texture_commands();
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

	this->reset_opengl();
}

QSizeF renderer::get_target_sizef() const
{
	return this->fbo->size();
}

void renderer::init_opengl()
{
	this->paint_device = std::make_unique<QOpenGLPaintDevice>(this->get_target_size());
	this->paint_device->setPaintFlipped(true);
	this->painter = std::make_unique<QPainter>(this->paint_device.get());

	const QSizeF target_sizef = this->get_target_sizef();
	const QSize target_size = target_sizef.toSize();

	glViewport(0, 0, static_cast<GLsizei>(target_size.width()), static_cast<GLsizei>(target_size.height()));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, target_sizef.width(), target_sizef.height(), 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.375, 0.375, 0.);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glClearDepth(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->setup_native_opengl_state();
}

void renderer::reset_opengl()
{
	this->painter.reset();
	this->paint_device.reset();

	this->fbo->window()->resetOpenGLState();
}

void renderer::blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QPoint &frame_pixel_pos, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent, const QSize &rendered_size)
{
	this->painter->beginNativePainting();
	this->setup_native_opengl_state();

	const QSize target_size = this->get_target_size();

	QRect source_rect;

	QSize source_frame_size = frame_size;
	QSize source_rendered_size = rendered_size;
	if (show_percent < 100) {
		source_frame_size = QSize(frame_size.width(), frame_size.height() * show_percent / 100);
		source_rendered_size = QSize(rendered_size.width(), rendered_size.height() * show_percent / 100);
	}

	if (flip) {
		source_rect = QRect(QPoint(frame_pixel_pos.x() + source_frame_size.width(), frame_pixel_pos.y()), QSize(-source_frame_size.width(), source_frame_size.height()));
	} else {
		source_rect = QRect(frame_pixel_pos, source_frame_size);
	}

	const QSize texture_size(texture->width(), texture->height());
	const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(source_rect, texture_size, QOpenGLTextureBlitter::OriginBottomLeft);

	const QRect target_rect(this->get_mirrored_pos(pos, source_rendered_size), source_rendered_size);
	const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(target_rect, QRect(QPoint(0, 0), target_size));

	this->blitter.bind();

	if (opacity != 255) {
		this->blitter.setOpacity(opacity / 255.0);
	}

	this->blitter.blit(texture->textureId(), target, source);

	if (opacity != 255) {
		this->blitter.setOpacity(1.0);
	}

	this->blitter.release();

	this->painter->endNativePainting();
}

void renderer::blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const int frame_index, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent)
{
	const int frames_per_row = size.width() / frame_size.width();
	const QPoint frame_pos = point::from_index(frame_index, frames_per_row);
	const QPoint frame_pixel_pos(frame_pos.x() * frame_size.width(), frame_pos.y() * frame_size.height());
	this->blit_texture_frame(texture, pos, frame_pixel_pos, frame_size, flip, opacity, show_percent, frame_size);
}

void renderer::draw_pixel(const QPoint &pos, const QColor &color)
{
	this->painter->beginNativePainting();
	this->setup_native_opengl_state();

	glDisable(GL_TEXTURE_2D);
	glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

	const QPoint mirrored_pos = this->get_mirrored_pos(pos, 1);

	glBegin(GL_POINTS);
	glVertex2i(mirrored_pos.x(), mirrored_pos.y());
	glEnd();

	glEnable(GL_TEXTURE_2D);

	this->painter->endNativePainting();
}

void renderer::draw_rect(const QPoint &pos, const QSize &size, const QColor &color, const double line_width)
{
	QPen pen(color);
	pen.setWidthF(line_width);

	this->painter->setPen(pen);

	this->draw_vertical_line(pos, size.height(), color, line_width);
	this->draw_vertical_line(pos + QPoint(size.width() - 1, 0), size.height(), color, line_width);

	this->draw_horizontal_line(pos, size.width(), color, line_width);
	this->draw_horizontal_line(pos + QPoint(0, size.height() - 1), size.width(), color, line_width);
}

void renderer::fill_rect(const QRect &rect, const QColor &color)
{
	this->painter->fillRect(rect, color);
}

void renderer::draw_line(const QPoint &start_pos, const QPoint &end_pos, const QColor &color, const double line_width)
{
	QPen pen(color);
	pen.setWidthF(line_width);

	this->painter->setPen(pen);
	this->painter->drawLine(start_pos, end_pos);
}

}
