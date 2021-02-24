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
/**@name particlemanager.cpp - The particle manager. */
//
//      (c) Copyright 2007-2008 by Jimmy Salmon and Francois Beerten
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

#include "particle.h"

#include "ui/ui.h"
#include "util/vector_util.h"
#include "video/video.h"

CParticleManager ParticleManager;

CParticleManager::CParticleManager()
{
}

CParticleManager::~CParticleManager()
{
}

void CParticleManager::init()
{
}

void CParticleManager::exit()
{
	ParticleManager.clear();
}

void CParticleManager::clear()
{
	this->particles.clear();
	this->new_particles.clear();
}

static inline bool DrawLevelCompare(const CParticle *lhs, const CParticle *rhs)
{
	return lhs->getDrawLevel() < rhs->getDrawLevel();
}

void CParticleManager::prepareToDraw(const CViewport &vp, std::vector<CParticle *> &table)
{
	this->vp = &vp;

	for (const std::unique_ptr<CParticle> &particle : this->particles) {
		if (particle->isVisible(vp)) {
			table.push_back(particle.get());
		}
	}

	std::sort(table.begin(), table.end(), DrawLevelCompare);
}

void CParticleManager::endDraw()
{
	this->vp = nullptr;
}

void CParticleManager::update()
{
	unsigned long ticks = GameCycle - lastTicks;
	std::vector<std::unique_ptr<CParticle>>::iterator i;

	wyrmgus::vector::merge(this->particles, std::move(this->new_particles));
	this->new_particles = std::vector<std::unique_ptr<CParticle>>();

	i = particles.begin();
	while (i != particles.end()) {
		(*i)->update(1000.0f / CYCLES_PER_SECOND * ticks);
		if ((*i)->isDestroyed()) {
			i = particles.erase(i);
		} else {
			++i;
		}
	}

	lastTicks += ticks;
}

void CParticleManager::add(std::unique_ptr<CParticle> &&particle)
{
	new_particles.push_back(std::move(particle));
}

CPosition CParticleManager::getScreenPos(const CPosition &pos) const
{
	const PixelPos mapPixelPos((int)pos.x, (int)pos.y);
	const PixelPos screenPixelPos = vp->map_to_screen_pixel_pos(mapPixelPos);

	return CPosition(screenPixelPos.x, screenPixelPos.y);
}
