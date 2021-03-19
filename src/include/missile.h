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
//      (c) Copyright 1998-2021 by Lutz Sammer and Andrettin
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

/**
**  @class wyrmgus::missile_type missile.h
**
**  \#include "missile.h"
**
**  This structure defines the base type information of all missiles. It
**  contains all information that all missiles of the same type shares.
**  The fields are filled from the configuration files.
**
**
**  The missile-type structure members:
**
**  missile_type::DrawLevel
**
**    The Level/Order to draw the missile in, usually 0-255
**
**  missile_type::Width missile_type::Height
**
**    Size (width and height) of a frame in the image. All sprite
**    frames of the missile-type must have the same size.
**
**  missile_type::SpriteFrames
**
**    Total number of sprite frames in the graphic image.
**    @note If the image is smaller than the number of directions,
**    width/height and/or framecount suggest the engine crashes.
**    @note There is currently a limit of 127 sprite frames, which
**    can be lifted if needed.
**
**  missile_type::num_directions
**
**    Number of directions missile can face.
**
**  missile_type::Transparency
**
**    Set a missile transparency.
**
**  missile_type::FiredSound
**
**    Sound of the missile, if fired. @note currently not used.
**
**  missile_type::ImpactSound
**
**    Impact sound for this missile.
**
**  missile_type::CanHitOwner
**
**    Can hit the unit that have fired the missile.
**    @note Currently no missile can hurt its owner.
**
**  missile_type::FriendlyFire
**
**    Can't hit the units of the same player, that has the
**    missile fired.
**
**  missile_type::missile_class
**
**    Class of the missile-type, defines the basic effects of the
**    missile. Look at the different class identifiers for more
**    information (missile_class::none, ...).
**
**  missile_type::NumBounces
**
**    This is the number of bounces, and it is only valid with
**    missile_class::bounce. The missile will hit this many times in
**    a row.
**
**  missile_type::StartDelay
**
**    Delay in game cycles after the missile generation, until the
**    missile animation and effects starts. Delay denotes the number
**    of display cycles to skip before drawing the first sprite frame
**    and only happens once at start.
**
**  missile_type::Sleep
**
**    This are the number of game cycles to wait for the next
**    animation or the sleeping between the animation steps.
**    All animations steps use the same delay.  0 is the
**    fastest and 255 the slowest animation.
**    @note Perhaps we should later allow animation scripts for
**    more complex animations.
**
**  missile_type::Speed
**
**    The speed how fast the missile moves. 0 the missile didn't
**    move, 1 is the slowest speed and 32 s the fastest supported
**    speed. This is how many pixels the missiles moves with each
**    animation step.  The real use of this member depends on the
**    missile_type::Class
**    @note This is currently only used by the point-to-point
**    missiles (missile_class::point_to_point, ...).  Perhaps we should
**    later allow animation scripts for more complex animations.
**
**  missile_type::Range
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
**  missile_type::SplashFactor
**
**    Determines The Splash damage divisor, see Range
**
**  missile_type::Impact
**
**    The config of the next (other) missile to generate, when this
**    missile reached its end point or its life time is over.  So it
**    can be used to generate a chain of missiles.
**
**  missile_type::Smoke
**
**    The config of the next (other) missile to generate a trailing smoke.  So it
**    can be used to generate a chain of missiles.
**
**  missile_type::Sprite
**
**    Missile sprite image loaded from missile_type::File
**
**  missile_type::G
**
**    File containing the image (sprite) graphics of the missile.
**    The file can contain multiple sprite frames.  The sprite frames
**    for the different directions are placed in the row.
**    The different animations steps are placed in the column. But
**    the correct order depends on missile_type::Class. Missiles like fire
**    have no directions, missiles like arrows have a direction.
*/

/**
**  @class Missile missile.h
**
**  \#include "missile.h"
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
**    missile_type::missile_class.
**
**  Missile::Type
**
**    ::missile_type pointer of the missile, contains the shared
**    information of all missiles of the same type.
**
**  Missile::SpriteFrame
**
**    Current sprite frame of the missile.  The range is from 0
**    to missile_type::SpriteFrames-1.  The topmost bit (128) is
**    used as flag to mirror the sprites in X direction.
**    Animation scripts aren't currently supported for missiles,
**    everything is handled by missile_type::Class
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
**    missile_type::Sleep to 0.
**
**  Missile::Delay
**
**    Number of game cycles the missile isn't shown on the map.
**    This counts down from missile_type::StartDelay to 0, before this
**    happens the missile isn't shown and has no effects.
**    @note This can also be used by missile_type::Class
**    for temporary removement of the missile.
**
**  Missile::SourceUnit
**
**    The owner of the missile. Normally the one who fired the
**    missile.  Used to check units, to prevent hitting the owner
**    when field missile_type::CanHitOwner==true. Also used for kill
**    and experience points.
**
**  Missile::TargetUnit
**
**    The target of the missile.  Normally the unit which should be
**    hit by the missile.
**
**  Missile::Damage
**
**    Damage done by missile. See also missile_type::Range, which
**    denoted the 100% damage in center.
**
**  Missile::TTL
**
**    Time to live in game cycles of the missile, if it reaches zero
**    the missile is automatic removed from the map. If -1 the
**    missile lives for ever and the lifetime is handled by
**    Missile::Type:missile_type::Class
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

#include "database/data_entry.h"
#include "database/data_type.h"
#include "data_type.h"
#include "missileconfig.h"
#include "vec2i.h"

class CGraphic;
class CUnit;
class CViewport;
class CFile;
class LuaCallback;
struct lua_State;
struct NumberDesc;

/*----------------------------------------------------------------------------
--  Missile-type
----------------------------------------------------------------------------*/

namespace wyrmgus {

class renderer;
class sound;
class unit_ref;
enum class missile_class;
enum class tile_flag : uint32_t;

/// Base structure of missile-types
class missile_type final : public data_entry, public data_type<missile_type>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::missile_class missile_class MEMBER missile_class READ get_missile_class)
	Q_PROPERTY(QString image_file READ get_image_file_qstring)
	Q_PROPERTY(QSize frame_size MEMBER frame_size READ get_frame_size)
	Q_PROPERTY(int draw_level MEMBER draw_level READ get_draw_level)
	Q_PROPERTY(int frames MEMBER frames READ get_frames)
	Q_PROPERTY(int num_directions MEMBER num_directions READ get_num_directions)
	Q_PROPERTY(int sleep MEMBER sleep READ get_sleep)
	Q_PROPERTY(int speed MEMBER speed READ get_speed)
	Q_PROPERTY(int range MEMBER range READ get_range)
	Q_PROPERTY(wyrmgus::sound* fired_sound MEMBER fired_sound READ get_fired_sound)
	Q_PROPERTY(wyrmgus::sound* impact_sound MEMBER impact_sound READ get_impact_sound)

public:
	static constexpr const char *class_identifier = "missile_type";
	static constexpr const char *database_folder = "missile_types";

	explicit missile_type(const std::string &identifier);
	~missile_type();

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	
	/// load the graphics for a missile type
	void LoadMissileSprite();
	void Init();
	void DrawMissileType(int frame, const PixelPos &pos) const;

	void Load(lua_State *l);

	wyrmgus::missile_class get_missile_class() const
	{
		return this->missile_class;
	}

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	void set_image_file(const std::filesystem::path &filepath);

	QString get_image_file_qstring() const
	{
		return QString::fromStdString(this->get_image_file().string());
	}

	Q_INVOKABLE void set_image_file(const std::string &filepath)
	{
		this->set_image_file(std::filesystem::path(filepath));
	}

	const QSize &get_frame_size() const
	{
		return this->frame_size;
	}

	int get_frame_width() const
	{
		return this->get_frame_size().width();
	}

	int get_frame_height() const
	{
		return this->get_frame_size().height();
	}

	int get_draw_level() const
	{
		return this->draw_level;
	}

	int get_frames() const
	{
		return this->frames;
	}

	int get_num_directions() const
	{
		return this->num_directions;
	}

	int get_sleep() const
	{
		return this->sleep;
	}

	int get_speed() const
	{
		return this->speed;
	}

	int get_range() const
	{
		return this->range;
	}

	sound *get_fired_sound() const
	{
		return this->fired_sound;
	}

	sound *get_impact_sound() const
	{
		return this->impact_sound;
	}

private:
	std::filesystem::path image_file;
	QSize frame_size = QSize(0, 0); //the missile frame size in pixels
public:
	int Transparency = 0;          /// missile transparency
private:
	int draw_level = 0; //level to draw missile at
	int frames = 0; //number of sprite frames in the graphic
	int num_directions = 1; //number of directions missile can face
public:
	int ChangeVariable = -1;   /// variable to change
	int ChangeAmount = 0;      /// how many to change
	bool ChangeMax = false;    /// modify the max, if value will exceed it
private:
	sound *fired_sound = nullptr; //fired sound
	sound *impact_sound = nullptr; //impact sound for this missile-type
public:
	bool CorrectSphashDamage = false;  /// restricts the radius damage depending on land, air, naval
	bool Flip;                 /// flip image when facing left
	bool CanHitOwner = false;          /// missile can hit the owner
	bool FriendlyFire;         /// missile can't hit own units
	bool AlwaysFire = false;           /// missile will always fire (even if target is dead)
	bool Pierce = false;               /// missile will hit every unit on his way
	bool PierceOnce = false;           /// pierce every target only once
	bool PierceIgnoreBeforeGoal = false;	/// only pierce targets after the goal
	bool IgnoreWalls = true;          /// missile ignores Wall units on it's way
	bool KillFirstUnit = false;        /// missile kills first unit blocking it's way
	//Wyrmgus start
	bool AlwaysHits = false;		   /// missile never misses
	//Wyrmgus end

private:
	wyrmgus::missile_class missile_class; /// missile class
public:
	int NumBounces = 0;            /// number of bounces
	int MaxBounceSize = 0;		   /// if the unit has a size greater than this, the missile won't bounce further
	int ParabolCoefficient = 2048;    /// parabol coefficient in parabolic missile
	int StartDelay = 0;            /// missile start delay
private:
	int sleep = 0; //missile sleep
	int speed = 0; //missile speed
public:
	int BlizzardSpeed = 0;         /// speed for blizzard shards
	//Wyrmgus start
	int AttackSpeed = 10;		   /// attack speed; used by whirlwind missiles
	//Wyrmgus end
	int TTL = -1;                   /// missile time-to-live
	int ReduceFactor = 100;          /// Multiplier for reduce or increase damage dealt to the next unit
	int SmokePrecision = 0;        /// How frequently the smoke missile will generate itself
	tile_flag MissileStopFlags;      /// On which terrain types missile won't fly
	std::unique_ptr<NumberDesc> Damage;        /// missile damage (used for non-direct missiles, e.g. impacts)

private:
	int range = 0; //missile damage range
public:
	int SplashFactor = 100;             /// missile splash divisor
	std::vector<MissileConfig> Impact;  /// missile produces an impact
	MissileConfig Smoke;                   /// trailing missile
	std::unique_ptr<LuaCallback> ImpactParticle;           /// impact particle
	std::unique_ptr<LuaCallback> SmokeParticle;            /// smoke particle
	std::unique_ptr<LuaCallback> OnImpact;                 /// called when

	// --- FILLED UP ---
	std::shared_ptr<CGraphic> G;         /// missile graphic
};

}

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

	static std::unique_ptr<Missile> Init(const wyrmgus::missile_type &mtype, const PixelPos &startPos, const PixelPos &destPos, int z);

	virtual void Action() = 0;

	void DrawMissile(const CViewport &vp, std::vector<std::function<void(renderer *)>> &render_commands) const;
	void SaveMissile(CFile &file) const;
	void MissileHit(CUnit *unit = nullptr);
	bool NextMissileFrame(char sign, char longAnimation);
	void NextMissileFrameCycle();
	void MissileNewHeadingFromXY(const PixelPos &delta);

	CUnit *get_source_unit() const;
	CUnit *get_target_unit() const;

	//private:
	PixelPos source; /// Missile source position
	PixelPos position;   /// missile pixel position
	PixelPos destination;  /// missile pixel destination
	const wyrmgus::missile_type *Type;  /// missile-type pointer
	int SpriteFrame;  /// sprite frame counter
	int State;        /// state
	int AnimWait;     /// Animation wait.
	int Wait;         /// delay between frames
	int Delay;        /// delay to show up
	//Wyrmgus start
	int MapLayer;	  /// map layer the missile is in
	//Wyrmgus end

	std::shared_ptr<wyrmgus::unit_ref> SourceUnit;  /// unit that fires (could be killed)
	std::shared_ptr<wyrmgus::unit_ref> TargetUnit;  /// target unit, used for spells

	std::vector<CUnit *> PiercedUnits;	/// Units which are already pierced by this missile

	int Damage;				/// direct damage that the missile applies
	int LightningDamage;	/// direct lightning damage that the missile applies

	int TTL;             /// time to live (ticks) used for spells
	int Hidden;          /// If this is 1 then the missile is invisible
	int DestroyMissile;  /// this tells missile-class-straight-fly, that it's time to die

	// Internal use:
	int CurrentStep;  /// Current step (0 <= x < TotalStep).
	int TotalStep;    /// Total step.
	
	//Wyrmgus start
	bool AlwaysHits;		/// Missile always hits
	bool AlwaysCritical;	/// Whether the missile always results in a critical hit
	//Wyrmgus end

	unsigned  Local: 1;     /// missile is a local missile
	unsigned int Slot;      /// unique number for draw level.

	static unsigned int Count; /// slot number generator.
};

extern bool MissileInitMove(Missile &missile);
extern bool PointToPointMissile(Missile &missile);
extern bool IsPiercedUnit(const Missile &missile, const CUnit &unit);
extern void MissileHandlePierce(Missile &missile, const Vec2i &pos);
extern bool MissileHandleBlocking(Missile &missile, const PixelPos &position);

class MissileNone final : public Missile
{
public:
	virtual void Action() override;
};

class MissilePointToPoint final : public Missile
{
public:
	virtual void Action() override;
};

class MissilePointToPointWithHit final : public Missile
{
public:
	virtual void Action() override;
};

class MissilePointToPointCycleOnce final : public Missile
{
public:
	virtual void Action() override;
};

class MissilePointToPointBounce final : public Missile
{
public:
	virtual void Action() override;
};

class MissileStay final : public Missile
{
public:
	virtual void Action() override;
};

class MissileCycleOnce final : public Missile
{
public:
	virtual void Action() override;
};

class MissileFire final : public Missile
{
public:
	virtual void Action() override;
};

class MissileHit final : public Missile
{
public:
	virtual void Action() override;
};

class MissileParabolic final : public Missile
{
public:
	virtual void Action() override;
};

class MissileLandMine final : public Missile
{
public:
	virtual void Action() override;
};

class MissileWhirlwind final : public Missile
{
public:
	virtual void Action() override;
};

class MissileFlameShield final : public Missile
{
public:
	virtual void Action() override;
};

class MissileDeathCoil final : public Missile
{
public:
	virtual void Action() override;
};

class MissileTracer final : public Missile
{
public:
	virtual void Action() override;
};

class MissileClipToTarget final : public Missile
{
public:
	virtual void Action() override;
};

namespace wyrmgus {

class missile_continuous final : public Missile
{
public:
	virtual void Action() override;
};

}

class MissileStraightFly final : public Missile
{
public:
	virtual void Action() override;
};

class BurningBuildingFrame final
{
public:
	int Percent = 0;  /// HP percent
	const wyrmgus::missile_type *Missile = nullptr;  /// Missile to draw
} ;

extern std::vector<std::unique_ptr<BurningBuildingFrame>> BurningBuildingFrames;  /// Burning building frames

// In ccl_missile.c

/// register ccl features
extern void MissileCclRegister();

// In missile.c

/// load all missile sprites
extern void LoadMissileSprites();
/// count missile sprites
extern int GetMissileSpritesCount();
/// create a missile
extern Missile *MakeMissile(const wyrmgus::missile_type &mtype, const PixelPos &startPos, const PixelPos &destPos, int z);
/// create a local missile
extern Missile *MakeLocalMissile(const wyrmgus::missile_type &mtype, const PixelPos &startPos, const PixelPos &destPos, int z);

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

/// Get the burning building missile based on hp percent
extern const wyrmgus::missile_type *MissileBurningBuilding(const int percent);

/// Save missiles
extern void SaveMissiles(CFile &file);

/// Initialize missile-types
extern void InitMissileTypes();
/// Clean missiles
extern void CleanMissiles();

extern void FreeBurningBuildingFrames();
