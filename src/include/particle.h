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
/**@name particle.h - The base particle headerfile. */
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

#pragma once

class CGraphic;
class CViewport;

namespace wyrmgus {
	class renderer;
}

struct CPosition {
	CPosition(float x, float y) : x(x), y(y) {}
	float x;
	float y;
};

class GraphicAnimation final
{
	CGraphic *g;
	int ticksPerFrame;
	int currentFrame = 0;
	int currTicks = 0;
public:
	explicit GraphicAnimation(CGraphic *g, int ticksPerFrame);
	~GraphicAnimation() {}

	/**
	**  Draw the current frame of the animation.
	**  @param x x screen coordinate where to draw the animation.
	**  @param y y screen coordinate where to draw the animation.
	*/
	void draw(int x, int y, std::vector<std::function<void(renderer *)>> &render_commands) const;

	/**
	**  Update the animation.
	**  @param ticks the number of ticks elapsed since the last call.
	*/
	void update(int ticks);

	bool isFinished() const;
	//Wyrmgus start
//	bool isVisible(const CViewport &vp, const CPosition &pos);
	bool isVisible(const CViewport &vp, const CPosition &pos, int z) const;
	//Wyrmgus end
	std::unique_ptr<GraphicAnimation> clone() const;
};

// Base particle class
class CParticle
{
public:
	//Wyrmgus start
//	CParticle(CPosition position, int drawlevel = 0) :
	explicit CParticle(CPosition position, int z, int drawlevel = 0) :
	//Wyrmgus end
		//Wyrmgus start
//		pos(position), destroyed(false), drawLevel(drawlevel)
		pos(position), destroyed(false), drawLevel(drawlevel), MapLayer(z)
		//Wyrmgus end
	{}
	virtual ~CParticle() {}

	virtual bool isVisible(const CViewport &vp) const = 0;
	virtual void draw(std::vector<std::function<void(renderer *)>> &render_commands) const = 0;
	virtual void update(int) = 0;

	void destroy() { destroyed = true; }
	bool isDestroyed() const { return destroyed; }

	virtual std::unique_ptr<CParticle> clone() const = 0;

	int getDrawLevel() const { return drawLevel; }
	void setDrawLevel(int value) { drawLevel = value; }

protected:
	CPosition pos;
	bool destroyed;
	int drawLevel;
	//Wyrmgus start
	int MapLayer;
	//Wyrmgus end
};

class StaticParticle final : public CParticle
{
public:
	//Wyrmgus start
//	StaticParticle(CPosition position, GraphicAnimation *flame, int drawlevel = 0);
	explicit StaticParticle(CPosition position, int z, const GraphicAnimation *flame, int drawlevel = 0);
	//Wyrmgus end
	virtual ~StaticParticle();

	virtual bool isVisible(const CViewport &vp) const;
	virtual void draw(std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void update(int ticks);
	virtual std::unique_ptr<CParticle> clone() const;

protected:
	std::unique_ptr<GraphicAnimation> animation;
};

// Chunk particle
class CChunkParticle : public CParticle
{
public:
	//Wyrmgus start
//	CChunkParticle(CPosition position, GraphicAnimation *smokeAnimation, GraphicAnimation *debrisAnimation,
	explicit CChunkParticle(CPosition position, int z, GraphicAnimation *smokeAnimation, GraphicAnimation *debrisAnimation,
	//Wyrmgus end
				   GraphicAnimation *destroyAnimation,
				   int minVelocity = 0, int maxVelocity = 400,
				   int minTrajectoryAngle = 77, int maxTTL = 0, int drawlevel = 0);
	virtual ~CChunkParticle();

	virtual bool isVisible(const CViewport &vp) const;
	virtual void draw(std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void update(int ticks);
	virtual std::unique_ptr<CParticle> clone() const;
	int getSmokeDrawLevel() const { return smokeDrawLevel; }
	int getDestroyDrawLevel() const { return destroyDrawLevel; }
	void setSmokeDrawLevel(int value) { smokeDrawLevel = value; }
	void setDestroyDrawLevel(int value) { destroyDrawLevel = value; }

protected:
	CPosition initialPos;
	int initialVelocity;
	float trajectoryAngle;
	int maxTTL;
	int nextSmokeTicks;
	int lifetime;
	int age;
	int minVelocity;
	int maxVelocity;
	int minTrajectoryAngle;
	float height;
	int smokeDrawLevel;
	int destroyDrawLevel;
	std::unique_ptr<GraphicAnimation> debrisAnimation;
	std::unique_ptr<GraphicAnimation> smokeAnimation;
	std::unique_ptr<GraphicAnimation> destroyAnimation;

	struct {
		float x;
		float y;
	} direction;
};

// Smoke particle
class CSmokeParticle : public CParticle
{
public:
	//Wyrmgus start
//	CSmokeParticle(CPosition position, GraphicAnimation *animation, float speedx = 0, float speedy = -22.0f, int drawlevel = 0);
	explicit CSmokeParticle(CPosition position, int z, GraphicAnimation *animation, float speedx = 0, float speedy = -22.0f, int drawlevel = 0);
	//Wyrmgus end
	virtual ~CSmokeParticle();

	virtual bool isVisible(const CViewport &vp) const;
	virtual void draw(std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void update(int ticks);
	virtual std::unique_ptr<CParticle> clone() const;

protected:
	std::unique_ptr<GraphicAnimation> puff;
	struct {
		float x;
		float y;
	} speedVector;
};

class CRadialParticle : public CParticle
{
public:
	//Wyrmgus start
//	explicit CRadialParticle(CPosition position, GraphicAnimation *animation, int maxSpeed, int drawlevel = 0);
	explicit CRadialParticle(CPosition position, int z, GraphicAnimation *animation, int maxSpeed, int drawlevel = 0);
	//Wyrmgus end
	virtual ~CRadialParticle();

	virtual bool isVisible(const CViewport &vp) const;
	virtual void draw(std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void update(int ticks);
	virtual std::unique_ptr<CParticle> clone() const;

protected:
	std::unique_ptr<GraphicAnimation> animation;
	float direction;
	int speed;
	int maxSpeed;
};

class CParticleManager final
{
public:
	CParticleManager();
	~CParticleManager();

	static void init();
	static void exit();

	void prepareToDraw(const CViewport &vp, std::vector<CParticle *> &table);
	void endDraw();

	void update();

	void add(std::unique_ptr<CParticle> &&particle);
	void clear();

	CPosition getScreenPos(const CPosition &pos) const;

	inline void setLowDetail(bool detail) { lowDetail = detail; }
	inline bool getLowDetail() const { return lowDetail; }

private:
	std::vector<std::unique_ptr<CParticle>> particles;
	std::vector<std::unique_ptr<CParticle>> new_particles;
	const CViewport *vp = nullptr;
	unsigned long lastTicks = 0;
	bool lowDetail;
};

extern CParticleManager ParticleManager;
