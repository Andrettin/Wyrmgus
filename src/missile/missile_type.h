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
/**@name missile_type.h - The missile type header file. */
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

#ifndef __MISSILE_TYPE_H__
#define __MISSILE_TYPE_H__

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class MissileType missile.h
**
**  \#include "missile/missile.h"
**
**  This structure defines the base type information of all missiles. It
**  contains all information that all missiles of the same type shares.
**  The fields are filled from the configuration files.
**
**
**  The missile-type structure members:
**
**  MissileType::Ident
**
**    Unique identifier of the missile-type, used to reference it in
**    config files and during startup.
**    @note Don't use this member in game, use instead the pointer
**    to this structure. See MissileTypeByIdent().
**
**  MissileType::DrawLevel
**
**    The Level/Order to draw the missile in, usually 0-255
**
**  MissileType::Width MissileType::Height
**
**    Size (width and height) of a frame in the image. All sprite
**    frames of the missile-type must have the same size.
**
**  MissileType::SpriteFrames
**
**    Total number of sprite frames in the graphic image.
**    @note If the image is smaller than the number of directions,
**    width/height and/or framecount suggest the engine crashes.
**    @note There is currently a limit of 127 sprite frames, which
**    can be lifted if needed.
**
**  MissileType::NumDirections
**
**    Number of directions missile can face.
**
**  MissileType::Transparency
**
**    Set a missile transparency.
**
**  MissileType::FiredSound
**
**    Sound of the missile, if fired. @note currently not used.
**
**  MissileType::ImpactSound
**
**    Impact sound for this missile.
**
**  MissileType::CanHitOwner
**
**    Can hit the unit that have fired the missile.
**    @note Currently no missile can hurt its owner.
**
**  MissileType::FriendlyFire
**
**    Can't hit the units of the same player, that has the
**    missile fired.
**
**  MissileType::Class
**
**    Class of the missile-type, defines the basic effects of the
**    missile. Look at the different class identifiers for more
**    information (::MissileClassNone, ...).
**
**  MissileType::NumBounces
**
**    This is the number of bounces, and it is only valid with
**    MissileClassBounce. The missile will hit this many times in
**    a row.
**
**  MissileType::StartDelay
**
**    Delay in game cycles after the missile generation, until the
**    missile animation and effects starts. Delay denotes the number
**    of display cycles to skip before drawing the first sprite frame
**    and only happens once at start.
**
**  MissileType::Sleep
**
**    This are the number of game cycles to wait for the next
**    animation or the sleeping between the animation steps.
**    All animations steps use the same delay.  0 is the
**    fastest and 255 the slowest animation.
**    @note Perhaps we should later allow animation scripts for
**    more complex animations.
**
**  MissileType::Speed
**
**    The speed how fast the missile moves. 0 the missile didn't
**    move, 1 is the slowest speed and 32 s the fastest supported
**    speed. This is how many pixels the missiles moves with each
**    animation step.  The real use of this member depends on the
**    MissileType::Class
**    @note This is currently only used by the point-to-point
**    missiles (::MissileClassPointToPoint, ...).  Perhaps we should
**    later allow animation scripts for more complex animations.
**
**  MissileType::Range
**
**    Determines the range in which a projectile will deal its damage.
**    A range of 0 will mean that the damage will be limited to the
**    targeted unit only.  So if you shot a missile at a unit, it
**    would only damage that unit.  A value of 1 only affects the
**    field where the missile hits.  A value of 2  would mean that
**    the damage for that particular missile would be dealt for a range
**    of 1 around the impact spot. All fields that aren't the center
**    get only 1/SpashFactor of the damage. Fields 2 away get
**    1/(SplashFactor*2), and following...
**
**  MissileType::SplashFactor
**
**    Determines The Splash damage divisor, see Range
**
**  MissileType::Impact
**
**    The config of the next (other) missile to generate, when this
**    missile reached its end point or its life time is over.  So it
**    can be used to generate a chain of missiles.
**
**  MissileType::Smoke
**
**    The config of the next (other) missile to generate a trailing smoke.  So it
**    can be used to generate a chain of missiles.
**
**  MissileType::Sprite
**
**    Missile sprite image loaded from MissileType::File
**
**  MissileType::G
**
**    File containing the image (sprite) graphics of the missile.
**    The file can contain multiple sprite frames.  The sprite frames
**    for the different directions are placed in the row.
**    The different animations steps are placed in the column. But
**    the correct order depends on MissileType::Class. Missiles like fire
**    have no directions, missiles like arrows have a direction.
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "missile/missile_config.h"
#include "script.h"
#include "sound/unit_sound.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGraphic;
class LuaCallback;

/**
**  Missile-class this defines how a missile-type reacts.
*/
enum {
	MissileClassNone,                     /// Missile does nothing
	MissileClassPointToPoint,             /// Missile flies from x,y to x1,y1
	MissileClassPointToPointWithHit,      /// Missile flies from x,y to x1,y1 than shows hit animation.
	MissileClassPointToPointCycleOnce,    /// Missile flies from x,y to x1,y1 and animates ONCE from start to finish and back
	MissileClassPointToPointBounce,       /// Missile flies from x,y to x1,y1 than bounces three times.
	MissileClassStay,                     /// Missile appears at x,y, does it's anim and vanishes.
	MissileClassCycleOnce,                /// Missile appears at x,y, then cycle through the frames once.
	MissileClassFire,                     /// Missile doesn't move, than checks the source unit for HP.
	MissileClassHit,                      /// Missile shows the hit points.
	MissileClassParabolic,                /// Missile flies from x,y to x1,y1 using a parabolic path
	MissileClassLandMine,                 /// Missile wait on x,y until a non-air unit comes by, the explodes.
	MissileClassWhirlwind,                /// Missile appears at x,y, is whirlwind
	MissileClassFlameShield,              /// Missile surround x,y
	MissileClassDeathCoil,                /// Missile is death coil.
	MissileClassTracer,                   /// Missile seeks towards to target unit
	MissileClassClipToTarget,             /// Missile remains clipped to target's current goal and plays his animation once
	MissileClassContinious,               /// Missile stays and plays it's animation several times
	MissileClassStraightFly               /// Missile flies from x,y to x1,y1 then continues to fly, until incompatible terrain is detected
};

/// Base structure of missile-types
class MissileType : public DataElement
{
	GDCLASS(MissileType, DataElement)

public:
	// so that the class can be exposed to Godot
	MissileType()
	{
	}
	
	explicit MissileType(const std::string &ident);
	~MissileType();

	static const char *MissileClassNames[];
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	/// load the graphics for a missile type
	void LoadMissileSprite();
	void Init();
	void DrawMissileType(int frame, const PixelPos &pos) const;

	void Load(lua_State *l);

	int Width() const { return size.x; }
	int Height() const { return size.y; }

	//private:
	int Transparency = 0;		/// missile transparency
	PixelSize size = PixelSize(0, 0);	/// missile size in pixels
	int DrawLevel = 0;			/// Level to draw missile at
	int SpriteFrames = 0;		/// number of sprite frames in graphic
	int NumDirections = 1;		/// number of directions missile can face
	int ChangeVariable = -1;		/// variable to change
	int ChangeAmount = 0;		/// how many to change
	bool ChangeMax = false;		/// modify the max, if value will exceed it

	SoundConfig FiredSound;	/// fired sound
	SoundConfig ImpactSound;	/// impact sound for this missile-type

	bool CorrectSphashDamage = false;	/// restricts the radius damage depending on land, air, naval
	bool Flip = true;			/// flip image when facing left
	bool CanHitOwner = false;	/// missile can hit the owner
	bool FriendlyFire = true;	/// missile can't hit own units
	bool AlwaysFire = false;	/// missile will always fire (even if target is dead)
	bool Pierce = false;		/// missile will hit every unit on his way
	bool PierceOnce = false;	/// pierce every target only once
	bool PierceIgnoreBeforeGoal = false;	/// only pierce targets after the goal
	bool IgnoreWalls = true;	/// missile ignores Wall units on it's way
	bool KillFirstUnit = false;	/// missile kills first unit blocking it's way
	//Wyrmgus start
	bool AlwaysHits = false;	/// missile never misses
	//Wyrmgus end

	int Class;					/// missile class
	int NumBounces = 0;			/// number of bounces
	int MaxBounceSize = 0;		/// if the unit has a size greater than this, the missile won't bounce further
	int ParabolCoefficient = 2048;	/// parabol coefficient in parabolic missile
	int StartDelay = 0;			/// missile start delay
	int Sleep = 0;				/// missile sleep
	int Speed = 0;				/// missile speed
	int BlizzardSpeed = 0;		/// speed for blizzard shards
	//Wyrmgus start
	int AttackSpeed = 10;		/// attack speed; used by whirlwind missiles
	//Wyrmgus end
	int TTL = -1;				/// missile time-to-live
	int ReduceFactor = 100;		/// Multiplier for reduce or increase damage dealt to the next unit
	int SmokePrecision = 0;		/// How frequently the smoke missile will generate itself
	int MissileStopFlags = 0;	/// On which terrain types missile won't fly
	NumberDesc *Damage = nullptr;	/// missile damage (used for non-direct missiles, e.g. impacts)

	int Range = 0;				/// missile damage range
	int SplashFactor = 100;		/// missile splash divisor
	std::vector <MissileConfig *> Impact;	/// missile produces an impact
	MissileConfig Smoke;		/// trailing missile
	LuaCallback *OnImpact = nullptr;	/// called when

	// --- FILLED UP ---
	CGraphic *G = nullptr;		/// missile graphic

protected:
	static inline void _bind_methods() {}
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// load all missile sprites
extern void LoadMissileSprites();
/// count missile sprites
extern int GetMissileSpritesCount();

/// allocate an empty missile-type slot
extern MissileType *NewMissileTypeSlot(const std::string &ident);
/// Get missile-type by ident
extern MissileType *MissileTypeByIdent(const std::string &ident);

/// Initialize missile-types
extern void InitMissileTypes();
/// Clean missile-types
extern void CleanMissileTypes();

#endif
