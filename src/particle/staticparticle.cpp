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
/**@name staticparticle.cpp - The static particle. */
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

#include "util/assert_util.h"

//Wyrmgus start
//StaticParticle::StaticParticle(CPosition position, GraphicAnimation *animation, int drawlevel) :
StaticParticle::StaticParticle(CPosition position, int z, const GraphicAnimation *animation, int drawlevel) :
//Wyrmgus end
	CParticle(position, z, drawlevel)
{
	assert_throw(animation != nullptr);
	this->animation = animation->clone();
}

StaticParticle::~StaticParticle()
{
}

bool StaticParticle::isVisible(const CViewport &vp) const
{
	//Wyrmgus start
//	return animation && animation->isVisible(vp, pos);
	return animation && animation->isVisible(vp, pos, MapLayer);
	//Wyrmgus end
}

void StaticParticle::draw(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	CPosition screenPos = ParticleManager.getScreenPos(pos);
	animation->draw(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), render_commands);
}

void StaticParticle::update(int ticks)
{
	animation->update(ticks);
	if (animation->isFinished()) {
		destroy();
	}
}

std::unique_ptr<CParticle> StaticParticle::clone() const
{
	//Wyrmgus start
//	return std::make_unique<StaticParticle>(pos, animation, drawLevel);
	return std::make_unique<StaticParticle>(pos, MapLayer, animation.get(), drawLevel);
	//Wyrmgus end
}
