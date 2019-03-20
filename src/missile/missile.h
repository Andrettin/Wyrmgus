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
/**@name missile.h - The missile header file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer and Andrettin
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
//

#ifndef __MISSILE_H__
#define __MISSILE_H__

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class Missile missile.h
**
**  \#include "missile/missile.h"
**
**  This structure contains all information about a missile in game.
**  A missile could have different effects, based on its missile-type.
**  Missiles could be saved and stored with CCL. See (missile).
**  Currently only a tile, a unit or a missile could be placed on the map.
**
**
**  The missile structure members:
**
**  Missile::position
**
**    Missile current map position in pixels.
**
**  Missile::source
**
**    Missile original map position in pixels.
**
**  Missile::destination
**
**    Missile destination on the map in pixels.  If
**    Missile::X==Missile::DX and Missile::Y==Missile::DY the missile
**    stays at its position.  But the movement also depends on
**    MissileType::Class.
**
**  Missile::Type
**
**    ::MissileType pointer of the missile, contains the shared
**    information of all missiles of the same type.
**
**  Missile::SpriteFrame
**
**    Current sprite frame of the missile.  The range is from 0
**    to MissileType::SpriteFrames-1.  The topmost bit (128) is
**    used as flag to mirror the sprites in X direction.
**    Animation scripts aren't currently supported for missiles,
**    everything is handled by MissileType::Class
**    @note If wanted, we can add animation scripts support to the
**    engine.
**
**  Missile::State
**
**    Current state of the missile.  Used for a simple state machine.
**
**  Missile::AnimWait
**
**    Animation wait. Used internally by missile actions, to run the
**    animation in parallel with the rest.
**
**  Missile::Wait
**
**    Wait this number of game cycles until the next state or
**    animation of this missile is handled. This counts down from
**    MissileType::Sleep to 0.
**
**  Missile::Delay
**
**    Number of game cycles the missile isn't shown on the map.
**    This counts down from MissileType::StartDelay to 0, before this
**    happens the missile isn't shown and has no effects.
**    @note This can also be used by MissileType::Class
**    for temporary removement of the missile.
**
**  Missile::SourceUnit
**
**    The owner of the missile. Normally the one who fired the
**    missile.  Used to check units, to prevent hitting the owner
**    when field MissileType::CanHitOwner==true. Also used for kill
**    and experience points.
**
**  Missile::TargetUnit
**
**    The target of the missile.  Normally the unit which should be
**    hit by the missile.
**
**  Missile::Damage
**
**    Damage done by missile. See also MissileType::Range, which
**    denoted the 100% damage in center.
**
**  Missile::TTL
**
**    Time to live in game cycles of the missile, if it reaches zero
**    the missile is automatic removed from the map. If -1 the
**    missile lives for ever and the lifetime is handled by
**    Missile::Type:MissileType::Class
**
**  Missile::Hidden
**
**    When you set this to 1 the unit becomes hidden for a while.
**
**  Missile::CurrentStep
**
**    Movement step. Used for the different trajectories.
**
**  Missile::TotalStep
**
**    Maximum number of step. When CurrentStep >= TotalStep, the movement is finished.
**
**  Missile::Local
**
**    This is a local missile, which can be different on all
**    computer in play. Used for the user interface (fe the green
**    cross).
**
**  Missile::MissileSlot
**
**    Pointer to the slot of this missile. Used for faster freeing.
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "script.h"
#include "unit/unit_ptr.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFile;
class CUnit;
class CViewport;
class MissileType;

/*----------------------------------------------------------------------------
--  Missile
----------------------------------------------------------------------------*/

/// Missile on the map
class Missile
{
protected:
	Missile();

public:
	virtual ~Missile();

	//Wyrmgus start
//	static Missile *Init(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos);
	static Missile *Init(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos, int z);
	//Wyrmgus end

	virtual void Action() = 0;

	void DrawMissile(const CViewport &vp) const;
	void SaveMissile(CFile &file) const;
	void MissileHit(CUnit *unit = nullptr);
	bool NextMissileFrame(char sign, char longAnimation);
	void NextMissileFrameCycle();
	void MissileNewHeadingFromXY(const PixelPos &delta);


	//private:
	PixelPos source = PixelPos(0, 0); 	/// Missile source position
	PixelPos position = PixelPos(0, 0);	/// missile pixel position
	PixelPos destination = PixelPos(0, 0);	/// missile pixel destination
	const MissileType *Type = nullptr;	/// missile-type pointer
	int SpriteFrame = 0;	/// sprite frame counter
	int State = 0;			/// state
	int AnimWait = 0;		/// Animation wait.
	int Wait = 0;			/// delay between frames
	int Delay = 0;			/// delay to show up
	//Wyrmgus start
	int MapLayer;		/// map layer the missile is in
	//Wyrmgus end

	CUnitPtr SourceUnit;	/// unit that fires (could be killed)
	CUnitPtr TargetUnit;	/// target unit, used for spells

	std::vector<CUnit *> PiercedUnits;	/// Units which are already pierced by this missile

	int Damage = 0;				/// direct damage that the missile applies
	int LightningDamage = 0;	/// direct lightning damage that the missile applies

	int TTL = -1;		/// time to live (ticks) used for spells
	int Hidden = 0;		/// If this is 1 then the missile is invisible
	int DestroyMissile = 0;	/// this tells missile-class-straight-fly, that it's time to die

	// Internal use:
	int CurrentStep = 0;	/// Current step (0 <= x < TotalStep).
	int TotalStep = 0;		/// Total step.
	
	//Wyrmgus start
	bool AlwaysHits = false;		/// Missile always hits
	bool AlwaysCritical = false;	/// Whether the missile always results in a critical hit
	//Wyrmgus end

	unsigned Local : 1;		/// missile is a local missile
	unsigned int Slot;		/// unique number for draw level.

	static unsigned int Count;	/// slot number generator.
};

extern bool MissileInitMove(Missile &missile);
extern bool PointToPointMissile(Missile &missile);
extern bool IsPiercedUnit(const Missile &missile, const CUnit &unit);
extern void MissileHandlePierce(Missile &missile, const Vec2i &pos);
extern bool MissileHandleBlocking(Missile &missile, const PixelPos &position);

class MissileNone : public Missile
{
public:
	virtual void Action();
};
class MissilePointToPoint : public Missile
{
public:
	virtual void Action();
};
class MissilePointToPointWithHit : public Missile
{
public:
	virtual void Action();
};
class MissilePointToPointCycleOnce : public Missile
{
public:
	virtual void Action();
};
class MissilePointToPointBounce : public Missile
{
public:
	virtual void Action();
};
class MissileStay : public Missile
{
public:
	virtual void Action();
};
class MissileCycleOnce : public Missile
{
public:
	virtual void Action();
};
class MissileFire : public Missile
{
public:
	virtual void Action();
};
class MissileHit : public Missile
{
public:
	virtual void Action();
};
class MissileParabolic : public Missile
{
public:
	virtual void Action();
};
class MissileLandMine : public Missile
{
public:
	virtual void Action();
};
class MissileWhirlwind : public Missile
{
public:
	virtual void Action();
};
class MissileFlameShield : public Missile
{
public:
	virtual void Action();
};
class MissileDeathCoil : public Missile
{
public:
	virtual void Action();
};

class MissileTracer : public Missile
{
public:
	virtual void Action();
};

class MissileClipToTarget : public Missile
{
public:
	virtual void Action();
};

class MissileContinious : public Missile
{
public:
	virtual void Action();
};

class MissileStraightFly : public Missile
{
public:
	virtual void Action();
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

// In ccl_missile.c

/// register ccl features
extern void MissileCclRegister();

// In missile.c

/// create a missile
//Wyrmgus start
//extern Missile *MakeMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos);
extern Missile *MakeMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos, int z);
//Wyrmgus end
/// create a local missile
//Wyrmgus start
//extern Missile *MakeLocalMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos);
extern Missile *MakeLocalMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos, int z);
//Wyrmgus end

/// Calculates damage done to goal by attacker using formula
//Wyrmgus start
//extern int CalculateDamage(const CUnit &attacker, const CUnit &goal, const NumberDesc *formula);
extern int CalculateDamage(const CUnit &attacker, const CUnit &goal, const NumberDesc *formula, const Missile *missile = nullptr);
//Wyrmgus end
/// fire a missile
//Wyrmgus start
//extern void FireMissile(CUnit &unit, CUnit *goal, const Vec2i &goalPos);
extern void FireMissile(CUnit &unit, CUnit *goal, const Vec2i &goalPos, int z);
//Wyrmgus end

extern void FindAndSortMissiles(const CViewport &vp, std::vector<Missile *> &table);

/// handle all missiles
extern void MissileActions();
/// distance from view point to missile
extern int ViewPointDistanceToMissile(const Missile &missile);

/// Save missiles
extern void SaveMissiles(CFile &file);

/// Initialize missiles
extern void InitMissiles();
/// Clean missiles
extern void CleanMissiles();

#endif
