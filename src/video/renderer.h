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
        blitter.create();
    }

    virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    virtual void render() override;

	QSizeF get_target_sizef() const;

	QSize get_target_size() const
	{
		return this->get_target_sizef().toSize();
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

	void blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QPoint &frame_pixel_pos, const QSize &frame_size, const bool flip)
	{
		const QSize target_size = this->get_target_size();

		const QRect source_rect(frame_pixel_pos, QSize(flip ? -frame_size.width() : frame_size.width(), frame_size.height()));
		const QSize texture_size(texture->width(), texture->height());
		const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(source_rect, texture_size, QOpenGLTextureBlitter::OriginBottomLeft);

		const QRect target_rect(QPoint(pos.x(), target_size.height() - frame_size.height() - pos.y()), frame_size);
		const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(target_rect, QRect(QPoint(0, 0), target_size));

		this->blitter.blit(texture->textureId(), target, source);
	}

	void blit_texture_frame(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const int frame_index, const QSize &frame_size, const bool flip);

	void blit_texture(const QOpenGLTexture *texture, const QPoint &pos, const QSize &size, const bool flip)
	{
		this->blit_texture_frame(texture, pos, QPoint(0, 0), size, flip);
	}

private:
    const frame_buffer_object *fbo = nullptr;
    QOpenGLTextureBlitter blitter;
};

}
