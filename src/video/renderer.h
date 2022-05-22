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

#pragma once

#pragma warning(push, 0)
#include <QOpenGLTexture>
#include <QOpenGLTextureBlitter>
#pragma warning(pop)

class QOpenGLPaintDevice;
class QPainter;

namespace wyrmgus {

class frame_buffer_object;

//a singleton providing an OpenGL renderer to be used by QtQuick
class renderer final : public QQuickFramebufferObject::Renderer
{
public:
	explicit renderer(const frame_buffer_object *fbo);
	~renderer();

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

	void init_opengl();
	void reset_opengl();

	void setup_native_opengl_state()
	{
		glShadeModel(GL_FLAT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		//this is necessary so that the alpha channel of a texture does not overwrite an underlying texture
		glDisable(GL_DEPTH_TEST);
	}

	void blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QPoint &frame_pixel_pos, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent, const QSize &rendered_size);
	void blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const int frame_index, const QSize &frame_size, const bool flip, const unsigned char opacity, const int show_percent);

	void blit_texture(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const bool flip, const unsigned char opacity, const QSize &rendered_size)
	{
		this->blit_texture_frame(texture, pos, QPoint(0, 0), size, flip, opacity, 100, rendered_size);
	}

	void draw_image(const QImage &image, const QPoint &pos);

	void draw_pixel(const QPoint &pos, const QColor &color);
	void draw_rect(const QPoint &pos, const QSize &size, const QColor &color, const double line_width = 1.0);
	void fill_rect(const QRect &rect, const QColor &color);

	void fill_rect(const QPoint &pixel_pos, const QSize &size, const QColor &color)
	{
		this->fill_rect(QRect(pixel_pos, size), color);
	}

	void draw_line(const QPoint &start_pos, const QPoint &end_pos, const QColor &color, const double line_width = 1.0);

	void draw_horizontal_line(const QPoint &pos, const int width, const QColor &color, const double line_width = 1.0)
	{
		if (width == 0) {
			return;
		}

		QPoint end_pos = pos;
		if (width > 0) {
			end_pos += QPoint(width - 1, 0);
		} else {
			end_pos += QPoint(width + 1, 0);
		}

		this->draw_line(pos, end_pos, color, line_width);
	}

	void draw_vertical_line(const QPoint &pos, const int height, const QColor &color, const double line_width = 1.0)
	{
		if (height == 0) {
			return;
		}

		QPoint end_pos = pos;
		if (height > 0) {
			end_pos += QPoint(0, height - 1);
		} else {
			end_pos += QPoint(0, height + 1);
		}

		this->draw_line(pos, end_pos, color, line_width);
	}

	void draw_circle(const QPoint &pos, const int radius, const QColor &color, const double line_width = 1.0);
	void fill_circle(const QPoint &pos, const int radius, const QColor &color);

private:
	const frame_buffer_object *fbo = nullptr;
	QOpenGLTextureBlitter blitter;
	std::unique_ptr<QOpenGLPaintDevice> paint_device;
	std::unique_ptr<QPainter> painter;
};

}
