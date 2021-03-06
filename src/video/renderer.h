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

namespace wyrmgus {

class frame_buffer_object;

//a singleton providing an OpenGL renderer to be used by QtQuick
class renderer final : public QQuickFramebufferObject::Renderer
{
public:
    static renderer *get()
    {
        return renderer::instance;
    }

private:
    static inline renderer *instance = nullptr;

public:
    renderer(const frame_buffer_object *fbo) : fbo(fbo)
    {
        renderer::instance = this;
    }

    ~renderer()
    {
        if (renderer::instance == this) {
            renderer::instance = nullptr;
        }
    }

    virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    virtual void render() override;

private:
    const frame_buffer_object *fbo = nullptr;
};

}
