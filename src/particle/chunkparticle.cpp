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
/**@name chunkparticle.cpp - The chunk particle. */
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

#include "util/random.h"
#include "util/util.h"
#include "video/video.h"

static constexpr int gravity = 32 * 12;

static inline float deg2rad(int degrees)
{
	return degrees * (3.1415926535f / 180);
}

//Wyrmgus start
//CChunkParticle::CChunkParticle(CPosition position, GraphicAnimation *smokeAnimation, GraphicAnimation *debrisAnimation,
CChunkParticle::CChunkParticle(CPosition position, int z, GraphicAnimation *smokeAnimation, GraphicAnimation *debrisAnimation,
//Wyrmgus end
							   GraphicAnimation *destroyAnimation,
							   int minVelocity, int maxVelocity, int minTrajectoryAngle, int maxTTL, int drawlevel) :
	//Wyrmgus start
//	CParticle(position, drawlevel), initialPos(position), maxTTL(maxTTL), nextSmokeTicks(0),
	CParticle(position, z, drawlevel), initialPos(position), maxTTL(maxTTL), nextSmokeTicks(0),
	//Wyrmgus end
	age(0), height(0.f)
{
	float radians = deg2rad(random::get()->generate_async(360));
	direction.x = cos(radians);
	direction.y = sin(radians);

	this->minVelocity = minVelocity;
	this->maxVelocity = maxVelocity;
	this->minTrajectoryAngle = minTrajectoryAngle;
	this->initialVelocity = this->minVelocity + random::get()->generate_async(this->maxVelocity - this->minVelocity + 1);
	this->trajectoryAngle = deg2rad(random::get()->generate_async(90 - this->minTrajectoryAngle) + this->minTrajectoryAngle);
	this->lifetime = (int)(1000 * (initialVelocity * sin(trajectoryAngle) / gravity) * 2);
	if (maxTTL) {
		this->lifetime = std::min(maxTTL, this->lifetime);
	}

	this->smokeAnimation = smokeAnimation->clone();
	this->debrisAnimation = debrisAnimation->clone();
	this->destroyAnimation = destroyAnimation->clone();
}

CChunkParticle::~CChunkParticle()
{
}

static float calculateScreenPos(float posy, float height)
{
	return posy - height * 0.2f;
}


bool CChunkParticle::isVisible(const CViewport &vp) const
{
	//Wyrmgus start
//	return debrisAnimation && debrisAnimation->isVisible(vp, pos);
	return debrisAnimation && debrisAnimation->isVisible(vp, pos, MapLayer);
	//Wyrmgus end
}

void CChunkParticle::draw(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	CPosition screenPos = ParticleManager.getScreenPos(pos);
	screenPos.y = calculateScreenPos(screenPos.y, height);
	debrisAnimation->draw(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), render_commands);
}

static float getHorizontalPosition(int initialVelocity, float trajectoryAngle, float time)
{
	return (initialVelocity * cos(trajectoryAngle)) * time;
}

static float getVerticalPosition(int initialVelocity, float trajectoryAngle, float time)
{
	return (initialVelocity * sin(trajectoryAngle)) * time -
		   (gravity / 2.0f) * (time * time);
}

void CChunkParticle::update(int ticks)
{
	age += ticks;
	if (age >= lifetime) {
		if (destroyAnimation) {
			CPosition p(pos.x, calculateScreenPos(pos.y, height));
			std::unique_ptr<GraphicAnimation> destroyanimation = destroyAnimation->clone();
			//Wyrmgus start
//			StaticParticle *destroy = new StaticParticle(p, destroyanimation, destroyDrawLevel);
			auto destroy = std::make_unique<StaticParticle>(p, this->MapLayer, destroyanimation.get(), destroyDrawLevel);
			//Wyrmgus end
			ParticleManager.add(std::move(destroy));
		}

		destroy();
		return;
	}

	const int minSmokeTicks = 150;
	const int randSmokeTicks = 50;

	if (age > nextSmokeTicks) {
		CPosition p(pos.x, calculateScreenPos(pos.y, height));
		std::unique_ptr<GraphicAnimation> smokeanimation = smokeAnimation->clone();
		//Wyrmgus start
//		CSmokeParticle *smoke = new CSmokeParticle(p, smokeanimation, 0, -22.0f, smokeDrawLevel);
		auto smoke = std::make_unique<CSmokeParticle>(p, MapLayer, smokeanimation.get(), 0, -22.0f, smokeDrawLevel);
		//Wyrmgus end
		ParticleManager.add(std::move(smoke));

		nextSmokeTicks += random::get()->generate_async(randSmokeTicks) + minSmokeTicks;
	}

	debrisAnimation->update(ticks);
	if (debrisAnimation->isFinished()) {
		std::unique_ptr<GraphicAnimation> debrisanimation = debrisAnimation->clone();
		debrisAnimation = std::move(debrisanimation);
	}

	float time = age / 1000.f;

	float distance = getHorizontalPosition(initialVelocity, trajectoryAngle, time);
	pos.x = initialPos.x + distance * direction.x;
	pos.y = initialPos.y + distance * direction.y;

	height = getVerticalPosition(initialVelocity, trajectoryAngle, time);
}

std::unique_ptr<CParticle> CChunkParticle::clone() const
{
	//Wyrmgus start
//	auto particle = std::make_unique<CChunkParticle>(pos, smokeAnimation, debrisAnimation, destroyAnimation, minVelocity, maxVelocity, minTrajectoryAngle, maxTTL, drawLevel);
	auto particle = std::make_unique<CChunkParticle>(pos, MapLayer, smokeAnimation.get(), debrisAnimation.get(), destroyAnimation.get(), minVelocity, maxVelocity, minTrajectoryAngle, maxTTL, drawLevel);
	//Wyrmgus end
	particle->smokeDrawLevel = smokeDrawLevel;
	particle->destroyDrawLevel = destroyDrawLevel;
	return particle;
}
