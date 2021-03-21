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

#pragma once

#include <QOpenGLTexture>
#include <QOpenGLTextureBlitter>

namespace wyrmgus {

class frame_buffer_object;

//a singleton providing an OpenGL renderer to be used by QtQuick
class renderer final : public QQuickFramebufferObject::Renderer
{
public:
    explicit renderer(const frame_buffer_object *fbo) : fbo(fbo)
    {
        this->blitter.create();
    }

    virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    virtual void render() override;

	QSizeF get_target_sizef() const;

	QSize get_target_size() const
	{
		return this->get_target_sizef().toSize();
	}

	QPoint get_mirrored_pos(const QPoint &pos, const int element_height) const
	{
		const QSize target_size = this->get_target_size();
		return QPoint(pos.x(), target_size.height() - element_height - pos.y());
	}

	QPoint get_mirrored_pos(const QPoint &pos, const QSize &element_size) const
	{
		return this->get_mirrored_pos(pos, element_size.height());
	}

	void init_opengl()
    {
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

		glShadeModel(GL_FLAT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		//this is necessary so that the alpha channel of a texture does not overwrite an underlying texture
		glDisable(GL_DEPTH_TEST);
	}

	void blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QPoint &frame_pixel_pos, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent, const QSize &rendered_size)
	{
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
	}

	void blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const int frame_index, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent);

	void blit_texture(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const bool flip, const unsigned char opacity, const QSize &rendered_size)
	{
		this->blit_texture_frame(texture, pos, QPoint(0, 0), size, flip, opacity, 100, rendered_size);
	}

	void draw_pixel(const QPoint &pos, const QColor &color)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

		const QPoint mirrored_pos = this->get_mirrored_pos(pos, 1);

		glBegin(GL_POINTS);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y());
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	void draw_rect(const QPoint &pos, const QSize &size, const QColor &color)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

		const QPoint mirrored_pos = this->get_mirrored_pos(pos, size);

		glBegin(GL_LINES);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y());
		//Wyrmgus start
	//	glVertex2i(mirrored_pos.x() + size.width(), mirrored_pos.y());
		glVertex2i(mirrored_pos.x() + size.width() - 1, mirrored_pos.y());
		//Wyrmgus end

		glVertex2i(mirrored_pos.x() + size.width() - 1, mirrored_pos.y() + 1);
		//Wyrmgus start
	//	glVertex2i(mirrored_pos.x() + size.width() - 1, mirrored_pos.y() + size.height());
		glVertex2i(mirrored_pos.x() + size.width() - 1, mirrored_pos.y() + size.height() - 1);
		//Wyrmgus end

		glVertex2i(mirrored_pos.x() + size.width() - 1, mirrored_pos.y() + size.height() - 1);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y() + size.height() - 1);

		glVertex2i(mirrored_pos.x(), mirrored_pos.y() + size.height() - 1);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y() + 1);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	void fill_rect(const QRect &rect, const QColor &color)
	{
		const QSize target_size = this->get_target_size();

		glDisable(GL_TEXTURE_2D);
		glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

		const QPoint pos = rect.topLeft();
		const QSize size = rect.size();
		const QPoint mirrored_pos = this->get_mirrored_pos(pos, size);

		glBegin(GL_TRIANGLE_STRIP);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y());
		glVertex2i(mirrored_pos.x() + size.width(), mirrored_pos.y());
		glVertex2i(mirrored_pos.x(), mirrored_pos.y() + size.height());
		glVertex2i(mirrored_pos.x() + size.width(), mirrored_pos.y() + size.height());
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	void fill_rect(const QPoint &pixel_pos, const QSize &size, const QColor &color)
	{
		this->fill_rect(QRect(pixel_pos, size), color);
	}

	void draw_line(const QPoint &start_pos, const QPoint &end_pos, const QColor &color)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

		const QPoint mirrored_start_pos = this->get_mirrored_pos(start_pos, 0);
		const QPoint mirrored_end_pos = this->get_mirrored_pos(end_pos, 0);

		glBegin(GL_LINES);
		glVertex2f(mirrored_start_pos.x(), mirrored_start_pos.y());
		glVertex2f(mirrored_end_pos.x(), mirrored_end_pos.y());
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	void draw_horizontal_line(const QPoint &pos, const int width, const QColor &color)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

		const QPoint mirrored_pos = this->get_mirrored_pos(pos, 1);

		glBegin(GL_LINES);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y());
		glVertex2i(mirrored_pos.x() + width, mirrored_pos.y());
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	void draw_vertical_line(const QPoint &pos, const int height, const QColor &color)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4ub(color.red(), color.green(), color.blue(), color.alpha());

		const QPoint mirrored_pos = this->get_mirrored_pos(pos, height);

		glBegin(GL_LINES);
		glVertex2i(mirrored_pos.x(), mirrored_pos.y());
		glVertex2i(mirrored_pos.x(), mirrored_pos.y() + height);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

private:
    const frame_buffer_object *fbo = nullptr;
    QOpenGLTextureBlitter blitter;
};

}
