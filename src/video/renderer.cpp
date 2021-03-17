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

#include "video/frame_buffer_object.h"
#include "video/render_context.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QQuickWindow>

namespace wyrmgus {

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
	//render_context::get()->run();

	QImage image(512, 512, QImage::Format_RGBA8888);
	image.fill(Qt::black);

	static QOpenGLTexture *texture = new QOpenGLTexture(image.mirrored());

	this->blitter.bind();

	const QRect target_rect(QPoint(0, 0), image.size());
	const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(target_rect, QRect(QPoint(0, 0), this->fbo->size().toSize()));

	this->blitter.blit(texture->textureId(), target, QOpenGLTextureBlitter::OriginTopLeft);

	this->blitter.release();

	this->fbo->window()->resetOpenGLState();
}

void renderer::init_opengl() const
{
	glViewport(0, 0, (GLsizei) this->fbo->size().width(), (GLsizei) this->fbo->size().height());

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, this->fbo->size().width(), this->fbo->size().height(), 0, -1, 1);

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
}

}
