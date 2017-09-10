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
/**@name missile.cpp - The missiles. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer and Jimmy Salmon
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <math.h>

#include "stratagus.h"

#include "missile.h"

#include "action/action_spellcast.h"
#include "actions.h"
#include "animation.h"
#include "font.h"
#include "iolib.h"
#include "luacallback.h"
#include "map.h"
#include "player.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound.h"
#include "spells.h"
//Wyrmgus start
#include "tileset.h"
//Wyrmgus end
#include "trigger.h"
#include "ui.h"
#include "unit.h"
#include "unit_find.h"
#include "unitsound.h"
#include "unittype.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

unsigned int Missile::Count = 0;

static std::vector<Missile *> GlobalMissiles;    /// all global missiles on map
static std::vector<Missile *> LocalMissiles;     /// all local missiles on map

/// lookup table for missile names
typedef std::map<std::string, MissileType *> MissileTypeMap;
static MissileTypeMap MissileTypes;

std::vector<BurningBuildingFrame *> BurningBuildingFrames; /// Burning building frames

extern NumberDesc *Damage;                   /// Damage calculation for missile.

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Load the graphics for a missile type
*/
void MissileType::LoadMissileSprite()
{
	if (this->G && !this->G->IsLoaded()) {
		this->G->Load();
		if (this->Flip) {
			this->G->Flip();
		}

		// Correct the number of frames in graphic
		Assert(this->G->NumFrames >= this->SpriteFrames);
		this->G->NumFrames = this->SpriteFrames;
		// FIXME: Don't use NumFrames as number of frames.
	}
}

int GetMissileSpritesCount()
{
#ifndef DYNAMIC_LOAD
	return MissileTypes.size();
#else
	return 0;
#endif
}

/**
**  Load the graphics for all missiles types
*/
void LoadMissileSprites()
{
#ifndef DYNAMIC_LOAD
	for (MissileTypeMap::iterator it = MissileTypes.begin(); it != MissileTypes.end(); ++it) {
		(*it).second->LoadMissileSprite();
	}
#endif
}
/**
**  Get Missile type by identifier.
**
**  @param ident  Identifier.
**
**  @return       Missile type pointer.
*/
MissileType *MissileTypeByIdent(const std::string &ident)
{
	if (ident.empty()) {
		return NULL;
	}
	MissileTypeMap::iterator it = MissileTypes.find(ident);
	if (it != MissileTypes.end()) {
		return it->second;
	}
	return NULL;
}

/**
**  Allocate an empty missile-type slot.
**
**  @param ident  Identifier to identify the slot.
**
**  @return       New allocated (zeroed) missile-type pointer.
*/
MissileType *NewMissileTypeSlot(const std::string &ident)
{
	MissileType *mtype = new MissileType(ident);

	MissileTypes[ident] = mtype;
	return mtype;
}

/**
**  Constructor
*/
Missile::Missile() :
	Type(NULL), SpriteFrame(0), State(0), AnimWait(0), Wait(0),
	Delay(0), SourceUnit(), TargetUnit(), Damage(0),
	TTL(-1), Hidden(0), DestroyMissile(0),
	CurrentStep(0), TotalStep(0),
	//Wyrmgus start
//	Local(0)
	Local(0),
	AlwaysHits(false), AlwaysCritical(false)
	//Wyrmgus end
{
	position.x = 0;
	position.y = 0;
	destination.x = 0;
	destination.y = 0;
	source.x = 0;
	source.y = 0;
	this->Slot = Missile::Count++;
}

/**
**  Initialize a new made missile.
**
**  @param mtype      Type pointer of missile.
**  @param sourcePos  Missile start point in pixel.
**  @param destPos    Missile destination point in pixel.
**
**  @return       created missile.
*/
//Wyrmgus start
///* static */ Missile *Missile::Init(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos)
/* static */ Missile *Missile::Init(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos, int z)
//Wyrmgus end
{
	Missile *missile = NULL;

	switch (mtype.Class) {
		case MissileClassNone :
			missile = new MissileNone;
			break;
		case MissileClassPointToPoint :
			missile = new MissilePointToPoint;
			break;
		case MissileClassPointToPointWithHit :
			missile = new MissilePointToPointWithHit;
			break;
		case MissileClassPointToPointCycleOnce :
			missile = new MissilePointToPointCycleOnce;
			break;
		case MissileClassPointToPointBounce :
			missile = new MissilePointToPointBounce;
			break;
		case MissileClassStay :
			missile = new MissileStay;
			break;
		case MissileClassCycleOnce :
			missile = new MissileCycleOnce;
			break;
		case MissileClassFire :
			missile = new MissileFire;
			break;
		case MissileClassHit :
			missile = new ::MissileHit;
			break;
		case MissileClassParabolic :
			missile = new MissileParabolic;
			break;
		case MissileClassLandMine :
			missile = new MissileLandMine;
			break;
		case MissileClassWhirlwind :
			missile = new MissileWhirlwind;
			break;
		case MissileClassFlameShield :
			missile = new MissileFlameShield;
			break;
		case MissileClassDeathCoil :
			missile = new MissileDeathCoil;
			break;
		case MissileClassTracer :
			missile = new MissileTracer;
			break;
		case MissileClassClipToTarget :
			missile = new MissileClipToTarget;
			break;
		case MissileClassContinious :
			missile = new MissileContinious;
			break;
		case MissileClassStraightFly :
			missile = new MissileStraightFly;
			break;
	}
	const PixelPos halfSize = mtype.size / 2;
	missile->position = startPos - halfSize;
	missile->destination = destPos - halfSize;
	missile->source = missile->position;
	//Wyrmgus start
	missile->MapLayer = z;
	//Wyrmgus end
	missile->Type = &mtype;
	missile->Wait = mtype.Sleep;
	missile->Delay = mtype.StartDelay;
	missile->TTL = mtype.TTL;
	//Wyrmgus start
	missile->AlwaysHits = mtype.AlwaysHits;
	//Wyrmgus end
	if (mtype.FiredSound.Sound) {
		PlayMissileSound(*missile, mtype.FiredSound.Sound);
	}

	return missile;
}

/**
**  Create a new global missile at (x,y).
**
**  @param mtype    Type pointer of missile.
**  @param startPos Missile start point in pixel.
**  @param destPos  Missile destination point in pixel.
**
**  @return       created missile.
*/
//Wyrmgus start
//Missile *MakeMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos)
Missile *MakeMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Missile *missile = Missile::Init(mtype, startPos, destPos);
	Missile *missile = Missile::Init(mtype, startPos, destPos, z);
	//Wyrmgus end

	GlobalMissiles.push_back(missile);
	return missile;
}

/**
**  Create a new local missile at (x,y).
**
**  @param mtype     Type pointer of missile.
**  @param startPos  Missile start point in pixel.
**  @param destPos   Missile destination point in pixel.
**
**  @return       created missile.
*/
//Wyrmgus start
//Missile *MakeLocalMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos)
Missile *MakeLocalMissile(const MissileType &mtype, const PixelPos &startPos, const PixelPos &destPos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Missile *missile = Missile::Init(mtype, startPos, destPos);
	Missile *missile = Missile::Init(mtype, startPos, destPos, z);
	//Wyrmgus end

	missile->Local = 1;
	LocalMissiles.push_back(missile);
	return missile;
}

/**
**  Calculate damage.
**
**  @todo NOTE: different targets (big are hit by some missiles better)
**  @todo NOTE: lower damage for hidden targets.
**  @todo NOTE: lower damage for targets on higher ground.
**
**  @param attacker_stats  Attacker attributes.
**  @param goal_stats      Goal attributes.
**  @param bloodlust       If attacker has bloodlust
**  @param xp              Experience of attacker.
**
**  @return                damage inflicted to goal.
*/
//Wyrmgus start
//static int CalculateDamageStats(const CUnitStats &attacker_stats,
//								const CUnitStats &goal_stats, int bloodlust)
static int CalculateDamageStats(const CUnit &attacker, const CUnitStats &goal_stats, const CUnit *goal, const Missile *missile = NULL)
//Wyrmgus end
{
	//Wyrmgus start
//	int basic_damage = attacker_stats.Variables[BASICDAMAGE_INDEX].Value;
//	int piercing_damage = attacker_stats.Variables[PIERCINGDAMAGE_INDEX].Value;
	int basic_damage = attacker.Variable[BASICDAMAGE_INDEX].Value;
	int piercing_damage = attacker.Variable[PIERCINGDAMAGE_INDEX].Value;
	int fire_damage = attacker.Variable[FIREDAMAGE_INDEX].Value;
	int cold_damage = attacker.Variable[COLDDAMAGE_INDEX].Value;
	int arcane_damage = attacker.Variable[ARCANEDAMAGE_INDEX].Value;
	int lightning_damage = attacker.Variable[LIGHTNINGDAMAGE_INDEX].Value;
	int air_damage = attacker.Variable[AIRDAMAGE_INDEX].Value;
	int earth_damage = attacker.Variable[EARTHDAMAGE_INDEX].Value;
	int water_damage = attacker.Variable[WATERDAMAGE_INDEX].Value;
	//Wyrmgus end

	//Wyrmgus start
	/*
	if (bloodlust) {
		basic_damage *= 2;
		piercing_damage *= 2;
	}
	*/
	int damage_modifier = 100;
	if (attacker.Variable[BLOODLUST_INDEX].Value > 0) {
		damage_modifier += 100;
	} else if (attacker.Variable[INSPIRE_INDEX].Value > 0) { // don't stack the effects of Inspire, Bloodlust and Leadership
		damage_modifier += 50;
	} else if (attacker.Variable[LEADERSHIP_INDEX].Value > 0) {
		damage_modifier += 10;
	}
	
	int accuracy_modifier = 100;
	if (attacker.Variable[PRECISION_INDEX].Value > 0) {
		accuracy_modifier += 100;
	}
	
	int critical_strike_chance = attacker.Variable[CRITICALSTRIKECHANCE_INDEX].Value;
	if (missile && missile->AlwaysCritical) {
		critical_strike_chance = 100;
	}
	if (critical_strike_chance > 0) {
		if (GameSettings.NoRandomness) {
			damage_modifier += critical_strike_chance;	//if no randomness setting is used, then critical strike chance will be used as a constant damage modifier, instead of being a chance of doubling the damage
		} else {
			if (SyncRand(100) < critical_strike_chance) {
				damage_modifier += 100;
			}
		}
	}
	if (goal != NULL) {
		// apply resistances to fire/cold damage
		fire_damage *= 100 - goal->Variable[FIRERESISTANCE_INDEX].Value;
		fire_damage /= 100;
		cold_damage *= 100 - goal->Variable[COLDRESISTANCE_INDEX].Value;
		cold_damage /= 100;
		arcane_damage *= 100 - goal->Variable[ARCANERESISTANCE_INDEX].Value;
		arcane_damage /= 100;
		lightning_damage *= 100 - goal->Variable[LIGHTNINGRESISTANCE_INDEX].Value;
		lightning_damage /= 100;
		air_damage *= 100 - goal->Variable[AIRRESISTANCE_INDEX].Value;
		air_damage /= 100;
		earth_damage *= 100 - goal->Variable[EARTHRESISTANCE_INDEX].Value;
		earth_damage /= 100;
		water_damage *= 100 - goal->Variable[WATERRESISTANCE_INDEX].Value;
		water_damage /= 100;
		
		// extra backstab damage (only works against units (that are organic and non-building, and that have 8 facing directions) facing opposite to the attacker
		if (attacker.Variable[BACKSTAB_INDEX].Value > 0 && goal->Type->BoolFlag[ORGANIC_INDEX].value && !goal->Type->Building && goal->Type->NumDirections == 8) {
			if (attacker.Direction == goal->Direction) {
				damage_modifier += attacker.Variable[BACKSTAB_INDEX].Value;
			} else if (goal->Direction == (attacker.Direction - 32) || goal->Direction == (attacker.Direction + 32) || (attacker.Direction == 0 && goal->Direction == 224) || (attacker.Direction == 224 && goal->Direction == 0)) {
				damage_modifier += attacker.Variable[BACKSTAB_INDEX].Value / 2;
			}
		}
		
		//add bonus against mounted, if applicable
		if (attacker.Variable[BONUSAGAINSTMOUNTED_INDEX].Value > 0 && goal->Type->BoolFlag[MOUNTED_INDEX].value) {
			damage_modifier += attacker.Variable[BONUSAGAINSTMOUNTED_INDEX].Value;
		}
		
		//add bonus against buildings, if applicable
		if (attacker.Variable[BONUSAGAINSTBUILDINGS_INDEX].Value > 0 && goal->Type->BoolFlag[BUILDING_INDEX].value) {
			damage_modifier += attacker.Variable[BONUSAGAINSTBUILDINGS_INDEX].Value;
		}
		
		//add bonus against air, if applicable
		if (attacker.Variable[BONUSAGAINSTAIR_INDEX].Value > 0 && goal->Type->BoolFlag[AIRUNIT_INDEX].value) {
			damage_modifier += attacker.Variable[BONUSAGAINSTAIR_INDEX].Value;
		}
		
		//add bonus against giants, if applicable
		if (attacker.Variable[BONUSAGAINSTGIANTS_INDEX].Value > 0 && goal->Type->BoolFlag[GIANT_INDEX].value) {
			damage_modifier += attacker.Variable[BONUSAGAINSTGIANTS_INDEX].Value;
		}
		
		//add bonus against dragons, if applicable
		if (attacker.Variable[BONUSAGAINSTDRAGONS_INDEX].Value > 0 && goal->Type->BoolFlag[DRAGON_INDEX].value) {
			damage_modifier += attacker.Variable[BONUSAGAINSTDRAGONS_INDEX].Value;
		}
	} else {
		// apply resistances to fire/cold damage
		fire_damage *= 100 - goal_stats.Variables[FIRERESISTANCE_INDEX].Value;
		fire_damage /= 100;
		cold_damage *= 100 - goal_stats.Variables[COLDRESISTANCE_INDEX].Value;
		cold_damage /= 100;
		arcane_damage *= 100 - goal_stats.Variables[ARCANERESISTANCE_INDEX].Value;
		arcane_damage /= 100;
		lightning_damage *= 100 - goal_stats.Variables[LIGHTNINGRESISTANCE_INDEX].Value;
		lightning_damage /= 100;
		air_damage *= 100 - goal_stats.Variables[AIRRESISTANCE_INDEX].Value;
		air_damage /= 100;
		earth_damage *= 100 - goal_stats.Variables[EARTHRESISTANCE_INDEX].Value;
		earth_damage /= 100;
		water_damage *= 100 - goal_stats.Variables[WATERRESISTANCE_INDEX].Value;
		water_damage /= 100;
	}
	
	basic_damage *= damage_modifier;
	basic_damage /= 100;
	
	piercing_damage += fire_damage;
	piercing_damage += cold_damage;
	piercing_damage += arcane_damage;
	piercing_damage += lightning_damage;
	piercing_damage += air_damage;
	piercing_damage += earth_damage;
	piercing_damage += water_damage;
	piercing_damage *= damage_modifier;
	piercing_damage /= 100;
	//Wyrmgus end

	//Wyrmgus start
//	int damage = std::max<int>(basic_damage - goal_stats.Variables[ARMOR_INDEX].Value, 1);
	int damage = 0;
	if (goal != NULL) {
		damage = std::max<int>(basic_damage - goal->Variable[ARMOR_INDEX].Value, 1);
	} else {
		damage = std::max<int>(basic_damage - goal_stats.Variables[ARMOR_INDEX].Value, 1);
	}
	//Wyrmgus end
	damage += piercing_damage;
	
	//Wyrmgus start
	int accuracy = attacker.Variable[ACCURACY_INDEX].Value;
	accuracy *= accuracy_modifier;
	accuracy /= 100;
	//Wyrmgus end
	
	if (GameSettings.NoRandomness) {
		if (accuracy > 0) { //if no randomness setting is used, and the attacker's accuracy and is greater than 0, then apply accuracy as a damage bonus and evasion as a damage malus
			if (goal != NULL) {
				if (goal->Variable[EVASION_INDEX].Value > 0) {
					damage += accuracy;
					if (goal->Variable[STUN_INDEX].Value == 0) { //stunned targets cannot evade
						damage -= goal->Variable[EVASION_INDEX].Value;
					}
					
					if (goal->Type->BoolFlag[ORGANIC_INDEX].value && !goal->Type->Building && goal->Type->NumDirections == 8) { //flanking
						if (attacker.Direction == goal->Direction) {
							damage += 4;
						} else if (goal->Direction == (attacker.Direction - 32) || goal->Direction == (attacker.Direction + 32) || (attacker.Direction == 0 && goal->Direction == 224) || (attacker.Direction == 224 && goal->Direction == 0)) {
							damage += 3;
						} else if (goal->Direction == (attacker.Direction - 64) || goal->Direction == (attacker.Direction + 64) || (attacker.Direction == 0 && goal->Direction == 192) || (attacker.Direction == 192 && goal->Direction == 0)) {
							damage += 2;
						} else if (goal->Direction == (attacker.Direction - 96) || goal->Direction == (attacker.Direction + 96) || (attacker.Direction == 0 && goal->Direction == 160) || (attacker.Direction == 160 && goal->Direction == 0)) {
							damage += 1;
						}
					}					
				}
			} else {
				if (goal_stats.Variables[EVASION_INDEX].Value > 0) {
					damage += accuracy;
					damage -= goal_stats.Variables[EVASION_INDEX].Value;
				}
			}
		}
		
		//Wyrmgus start
		//apply hack/pierce/blunt resistances
		if (goal != NULL) {
			if (attacker.Type->BoolFlag[HACKDAMAGE_INDEX].value) {
				damage *= 100 - goal->Variable[HACKRESISTANCE_INDEX].Value;
				damage /= 100;
			} else if (attacker.Type->BoolFlag[PIERCEDAMAGE_INDEX].value) {
				damage *= 100 - goal->Variable[PIERCERESISTANCE_INDEX].Value;
				damage /= 100;
			} else if (attacker.Type->BoolFlag[BLUNTDAMAGE_INDEX].value) {
				damage *= 100 - goal->Variable[BLUNTRESISTANCE_INDEX].Value;
				damage /= 100;
			}
		}
		//Wyrmgus end
		
		damage -= ((damage + 2) / 2) / 2; //if no randomness setting is used, then the damage will always return what would have been the average damage with randomness
	} else {
		//Wyrmgus start
		//apply hack/pierce/blunt resistances
		if (goal != NULL) {
			if (attacker.Type->BoolFlag[HACKDAMAGE_INDEX].value) {
				damage *= 100 - goal->Variable[HACKRESISTANCE_INDEX].Value;
				damage /= 100;
			} else if (attacker.Type->BoolFlag[PIERCEDAMAGE_INDEX].value) {
				damage *= 100 - goal->Variable[PIERCERESISTANCE_INDEX].Value;
				damage /= 100;
			} else if (attacker.Type->BoolFlag[BLUNTDAMAGE_INDEX].value) {
				damage *= 100 - goal->Variable[BLUNTRESISTANCE_INDEX].Value;
				damage /= 100;
			}
		}
		//Wyrmgus end
		
		damage -= SyncRand() % ((damage + 2) / 2);
	}
	
	Assert(damage >= 0);

	return damage;
}

/**
**  Calculate damage.
**
**  @param attacker  Attacker.
**  @param goal      Goal unit.
**  @param formula   Formula used to calculate damage.
**
**  @return          damage produces on goal.
*/
//Wyrmgus start
//int CalculateDamage(const CUnit &attacker, const CUnit &goal, const NumberDesc *formula)
int CalculateDamage(const CUnit &attacker, const CUnit &goal, const NumberDesc *formula, const Missile *missile)
//Wyrmgus end
{
	if (!formula) { // Use old method.
		//Wyrmgus start
//		return CalculateDamageStats(*attacker.Stats, *goal.Stats,
//									attacker.Variable[BLOODLUST_INDEX].Value);
		return CalculateDamageStats(attacker, *goal.Stats, &goal, missile);
		//Wyrmgus end
	}
	Assert(formula);

	UpdateUnitVariables(const_cast<CUnit &>(attacker));
	UpdateUnitVariables(const_cast<CUnit &>(goal));
	TriggerData.Attacker = const_cast<CUnit *>(&attacker);
	TriggerData.Defender = const_cast<CUnit *>(&goal);
	const int res = EvalNumber(formula);
	TriggerData.Attacker = NULL;
	TriggerData.Defender = NULL;
	return res;
}

//Wyrmgus start
/**
**  Calculate hit.
**
**  @return                whether the target was hit or not.
*/
static bool CalculateHit(const CUnit &attacker, const CUnitStats &goal_stats, const CUnit *goal)
{
	if (GameSettings.NoRandomness) {
		return true;
	}
	
	if (GodMode && attacker.Player == ThisPlayer && (!goal || goal->Player != ThisPlayer)) {
		return true; //always hit if in god mode
	}

	if (attacker.Type->BoolFlag[TRAP_INDEX].value) { // traps always hit
		return true;
	}
	
	int accuracy_modifier = 100;
	if (attacker.Variable[PRECISION_INDEX].Value > 0) {
		accuracy_modifier += 100;
	}
	
	int accuracy = attacker.Variable[ACCURACY_INDEX].Value;
	accuracy *= accuracy_modifier;
	accuracy /= 100;
	
	if (accuracy == 0) {
		return false;
	} else {
		int evasion = 0;
		if (goal != NULL) {
			if (goal->Variable[EVASION_INDEX].Value && goal->Variable[STUN_INDEX].Value == 0) { //stunned targets cannot evade
				evasion = goal->Variable[EVASION_INDEX].Value;
			}
			if (goal->Type->BoolFlag[ORGANIC_INDEX].value && !goal->Type->Building && goal->Type->NumDirections == 8) { //flanking
				if (attacker.Direction == goal->Direction) {
					evasion -= 4;
				} else if (goal->Direction == (attacker.Direction - 32) || goal->Direction == (attacker.Direction + 32) || (attacker.Direction == 0 && goal->Direction == 224) || (attacker.Direction == 224 && goal->Direction == 0)) {
					evasion -= 3;
				} else if (goal->Direction == (attacker.Direction - 64) || goal->Direction == (attacker.Direction + 64) || (attacker.Direction == 0 && goal->Direction == 192) || (attacker.Direction == 192 && goal->Direction == 0)) {
					evasion -= 2;
				} else if (goal->Direction == (attacker.Direction - 96) || goal->Direction == (attacker.Direction + 96) || (attacker.Direction == 0 && goal->Direction == 160) || (attacker.Direction == 160 && goal->Direction == 0)) {
					evasion -= 1;
				}
			}
		} else {
			if (goal_stats.Variables[EVASION_INDEX].Value > 0) {
				evasion = goal_stats.Variables[EVASION_INDEX].Value;
			}
		}
		if (accuracy > 0) {
			accuracy = SyncRand(accuracy);
		}
		if (evasion > 0) {
			evasion = SyncRand(evasion);
		}
		if (evasion > 0 && (accuracy < evasion || accuracy == 0)) {
			return false;
		}
	}

	return true;
}
//Wyrmgus end

/**
**  Fire missile.
**
**  @param unit  Unit that fires the missile.
*/
//Wyrmgus start
//void FireMissile(CUnit &unit, CUnit *goal, const Vec2i &goalPos)
void FireMissile(CUnit &unit, CUnit *goal, const Vec2i &goalPos, int z)
//Wyrmgus end
{
	Vec2i newgoalPos = goalPos;
	//Wyrmgus start
	int new_z = z;
	//Wyrmgus end
	// Goal dead?
	if (goal) {
		//Wyrmgus start
//		Assert(!unit.Type->Missile.Missile->AlwaysFire || unit.Type->Missile.Missile->Range);
		Assert(!unit.GetMissile().Missile->AlwaysFire || unit.GetMissile().Missile->Range);
		//Wyrmgus end
		if (goal->Destroyed) {
			DebugPrint("destroyed unit\n");
			return;
		}
		if (goal->Removed) {
			return;
		}
		if (goal->CurrentAction() == UnitActionDie) {
			//Wyrmgus start
//			if (unit.Type->Missile.Missile->AlwaysFire) {
			if (unit.GetMissile().Missile->AlwaysFire) {
			//Wyrmgus end
				newgoalPos = goal->tilePos;
				//Wyrmgus start
				new_z = goal->MapLayer;
				//Wyrmgus end
				goal = NULL;
			} else {
				return;
			}
		}
	}

	// No missile hits immediately!
	if (
		//Wyrmgus start
//		unit.Type->Missile.Missile->Class == MissileClassNone
		unit.GetMissile().Missile->Class == MissileClassNone
//		|| (unit.Type->Animations && unit.Type->Animations->Attack && unit.Type->Animations->RangedAttack && !unit.IsAttackRanged(goal, goalPos)) // treat melee attacks from units that have both attack and ranged attack animations as having missile class none
		|| (unit.GetAnimations() && unit.GetAnimations()->Attack && unit.GetAnimations()->RangedAttack && !unit.IsAttackRanged(goal, goalPos, z)) // treat melee attacks from units that have both attack and ranged attack animations as having missile class none
		//Wyrmgus end
	) {
		//Wyrmgus start
		int damage = 0;
		//Wyrmgus end
		// No goal, take target coordinates
		if (!goal) {
			//Wyrmgus start
//			if (Map.WallOnMap(goalPos)) {
			if (Map.WallOnMap(goalPos, z)) {
			//Wyrmgus end
				//Wyrmgus start
//				if (Map.HumanWallOnMap(goalPos)) {
				if (Map.Field(goalPos, z)->OverlayTerrain->UnitType && CalculateHit(unit, *Map.Field(goalPos, z)->OverlayTerrain->UnitType->Stats, NULL) == true) {
				//Wyrmgus end
					//Wyrmgus start
					PlayUnitSound(unit, VoiceHit);
					damage = CalculateDamageStats(unit, *Map.Field(goalPos, z)->OverlayTerrain->UnitType->Stats, NULL);
					//Wyrmgus end
					Map.HitWall(goalPos,
								//Wyrmgus start
//								CalculateDamageStats(*unit.Stats,
//													 *Map.Field(goalPos)->OverlayTerrain->UnitType->Stats, unit.Variable[BLOODLUST_INDEX].Value));
								damage, z);
								//Wyrmgus end
				//Wyrmgus start
				/*
				} else {
					Map.HitWall(goalPos,
								CalculateDamageStats(*unit.Stats,
													 *UnitTypeOrcWall->Stats, unit.Variable[BLOODLUST_INDEX].Value));
				*/
				//Wyrmgus end
				}
				return;
			}
			DebugPrint("Missile-none hits no unit, shouldn't happen!\n");
			return;
		}
		//Wyrmgus start
//		HitUnit(&unit, *goal, CalculateDamage(unit, *goal, Damage));
		if (CalculateHit(unit, *goal->Stats, goal) == true) {
			damage = CalculateDamage(unit, *goal, Damage);
			HitUnit(&unit, *goal, damage);
			if (goal->IsAlive()) {
				HitUnit_NormalHitSpecialDamageEffects(unit, *goal);
			}
			PlayUnitSound(unit, VoiceHit);
			
			//apply Thorns damage if attacker is at melee range
			if (goal && goal->Variable[THORNSDAMAGE_INDEX].Value && unit.MapDistanceTo(*goal) <= 1) {
				int thorns_damage = std::max<int>(goal->Variable[THORNSDAMAGE_INDEX].Value - unit.Variable[ARMOR_INDEX].Value, 1);
				if (GameSettings.NoRandomness) {
					thorns_damage -= ((thorns_damage + 2) / 2) / 2; //if no randomness setting is used, then the damage will always return what would have been the average damage with randomness
				} else {
					thorns_damage -= SyncRand() % ((thorns_damage + 2) / 2);
				}
				HitUnit(goal, unit, thorns_damage);
			}
		} else {
			PlayUnitSound(unit, VoiceMiss);
		}
		//Wyrmgus end
		return;
	}

	// If Firing from inside a Bunker
	CUnit *from = GetFirstContainer(unit);
	const int dir = ((unit.Direction + NextDirection / 2) & 0xFF) / NextDirection;
	//Wyrmgus start
//	const PixelPos startPixelPos = Map.TilePosToMapPixelPos_Center(from->tilePos)
	const PixelPos startPixelPos = Map.TilePosToMapPixelPos_TopLeft(from->tilePos) + PixelSize(from->Type->GetHalfTilePixelSize().x, from->Type->GetHalfTilePixelSize().y)
	//Wyrmgus end
								   + unit.Type->MissileOffsets[dir][0];

	Vec2i dpos;
	if (goal) {
		Assert(goal->Type);  // Target invalid?
		// Moved out of attack range?

		if (unit.MapDistanceTo(*goal) < unit.Type->MinAttackRange) {
			DebugPrint("Missile target too near %d,%d\n" _C_
					   unit.MapDistanceTo(*goal) _C_ unit.Type->MinAttackRange);
			// FIXME: do something other?
			return;
		}
		// Fire to nearest point of the unit!
		// If Firing from inside a Bunker
		if (unit.Container) {
			NearestOfUnit(*goal, GetFirstContainer(unit)->tilePos, &dpos);
		} else {
			dpos = goal->tilePos + goal->Type->GetHalfTileSize();
			//Wyrmgus start
			z = goal->MapLayer;
			//Wyrmgus end
		}
	} else {
		dpos = newgoalPos;
		// FIXME: Can this be too near??
		//Wyrmgus start
		z = new_z;
		//Wyrmgus end
	}

	PixelPos destPixelPos = Map.TilePosToMapPixelPos_Center(dpos);
	//Wyrmgus start
//	Missile *missile = MakeMissile(*unit.Type->Missile.Missile, startPixelPos, destPixelPos);
	Missile *missile = MakeMissile(*unit.GetMissile().Missile, startPixelPos, destPixelPos, z);
	//Wyrmgus end
	//
	// Damage of missile
	//
	if (goal) {
		missile->TargetUnit = goal;
	}
	missile->SourceUnit = &unit;
}

/**
**  Get area of tiles covered by missile
**
**  @param missile  Missile to be checked and set.
**  @param boxMin       OUT: Pointer to top left corner in map tiles.
**  @param boxMax       OUT: Pointer to bottom right corner in map tiles.
**
**  @return         sx,sy,ex,ey defining area in Map
*/
static void GetMissileMapArea(const Missile &missile, Vec2i &boxMin, Vec2i &boxMax)
{
	PixelSize missileSize(missile.Type->Width(), missile.Type->Height());
	PixelDiff margin(PixelTileSize.x - 1, PixelTileSize.y - 1);
	boxMin = Map.MapPixelPosToTilePos(missile.position);
	boxMax = Map.MapPixelPosToTilePos(missile.position + missileSize + margin);
	//Wyrmgus start
//	Map.Clamp(boxMin);
//	Map.Clamp(boxMax);
	Map.Clamp(boxMin, missile.MapLayer);
	Map.Clamp(boxMax, missile.MapLayer);
	//Wyrmgus end
}

/**
**  Check missile visibility in a given viewport.
**
**  @param vp       Viewport to be checked.
**  @param missile  Missile pointer to check if visible.
**
**  @return         Returns true if visible, false otherwise.
*/
static int MissileVisibleInViewport(const CViewport &vp, const Missile &missile)
{
	Vec2i boxmin;
	Vec2i boxmax;

	GetMissileMapArea(missile, boxmin, boxmax);
	if (!vp.AnyMapAreaVisibleInViewport(boxmin, boxmax)) {
		return 0;
	}
	Vec2i pos;
	for (pos.x = boxmin.x; pos.x <= boxmax.x; ++pos.x) {
		for (pos.y = boxmin.y; pos.y <= boxmax.y; ++pos.y) {
			//Wyrmgus start
//			if (ReplayRevealMap || Map.Field(pos)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			if (ReplayRevealMap || Map.Field(pos, missile.MapLayer)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			//Wyrmgus end
				return 1;
			}
		}
	}
	return 0;
}

/**
**  Draw missile.
**
**  @param frame  Animation frame
**  @param pos    Screen pixel position
*/
void MissileType::DrawMissileType(int frame, const PixelPos &pos) const
{
#ifdef DYNAMIC_LOAD
	if (!this->G->IsLoaded()) {
		LoadMissileSprite(this);
	}
#endif

	if (this->Flip) {
		if (frame < 0) {
			if (this->Transparency > 0) {
				//Wyrmgus start
//				this->G->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * Transparency));
				this->G->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * Transparency), false);
				//Wyrmgus end
			} else {
				//Wyrmgus start
//				this->G->DrawFrameClipX(-frame - 1, pos.x, pos.y);
				this->G->DrawFrameClipX(-frame - 1, pos.x, pos.y, false);
				//Wyrmgus end
			}
		} else {
			if (this->Transparency > 0) {
				//Wyrmgus start
//				this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency));
				this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency), false);
				//Wyrmgus end
			} else {
				//Wyrmgus start
//				this->G->DrawFrameClip(frame, pos.x, pos.y);
				this->G->DrawFrameClip(frame, pos.x, pos.y, false);
				//Wyrmgus end
			}
		}
	} else {
		const int row = this->NumDirections / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * this->NumDirections + this->NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * this->NumDirections + frame % row;
		}
		if (this->Transparency > 0) {
			//Wyrmgus start
//			this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency));
			this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency), false);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			this->G->DrawFrameClip(frame, pos.x, pos.y);
			this->G->DrawFrameClip(frame, pos.x, pos.y, false);
			//Wyrmgus end
		}
	}
}

/**
**  Draw missile.
*/
void Missile::DrawMissile(const CViewport &vp) const
{
	Assert(this->Type);
	CUnit *sunit = this->SourceUnit;
	// FIXME: I should copy SourcePlayer for second level missiles.
	if (sunit && sunit->Player) {
#ifdef DYNAMIC_LOAD
		if (!this->Type->Sprite) {
			LoadMissileSprite(this->Type);
		}
#endif
	}
	const PixelPos screenPixelPos = vp.MapToScreenPixelPos(this->position);

	switch (this->Type->Class) {
		case MissileClassHit:
			CLabel(GetGameFont()).DrawClip(screenPixelPos.x, screenPixelPos.y, this->Damage);
			break;
		default:
			if (Type->G) {
				this->Type->DrawMissileType(this->SpriteFrame, screenPixelPos);
			}
			break;
	}
}

static bool MissileDrawLevelCompare(const Missile *const l, const Missile *const r)
{
	if (l->Type->DrawLevel == r->Type->DrawLevel) {
		return l->Slot < r->Slot;
	} else {
		return l->Type->DrawLevel < r->Type->DrawLevel;
	}
}

/**
**  Sort visible missiles on map for display.
**
**  @param vp         Viewport pointer.
**  @param table      OUT : array of missile to display sorted by DrawLevel.
*/
void FindAndSortMissiles(const CViewport &vp, std::vector<Missile *> &table)
{
	typedef std::vector<Missile *>::const_iterator MissilePtrConstiterator;

	// Loop through global missiles, then through locals.
	for (MissilePtrConstiterator i = GlobalMissiles.begin(); i != GlobalMissiles.end(); ++i) {
		Missile &missile = *(*i);
		//Wyrmgus start
//		if (missile.Delay || missile.Hidden) {
		if (missile.Delay || missile.Hidden || missile.MapLayer != CurrentMapLayer) {
		//Wyrmgus end
			continue;  // delayed or hidden -> aren't shown
		}
		// Draw only visible missiles
		if (MissileVisibleInViewport(vp, missile)) {
			table.push_back(&missile);
		}
	}

	for (MissilePtrConstiterator i = LocalMissiles.begin(); i != LocalMissiles.end(); ++i) {
		Missile &missile = *(*i);
		//Wyrmgus start
//		if (missile.Delay || missile.Hidden) {
		if (missile.Delay || missile.Hidden || missile.MapLayer != CurrentMapLayer) {
		//Wyrmgus end
			continue;  // delayed or hidden -> aren't shown
		}
		// Local missile are visible.
		table.push_back(&missile);
	}
	std::sort(table.begin(), table.end(), MissileDrawLevelCompare);
}

/**
**  Change missile heading from x,y.
**
**  @param delta    Delta movement
**
**  @internal We have : SpriteFrame / (2 * (Numdirection - 1)) == DirectionToHeading / 256.
*/
void Missile::MissileNewHeadingFromXY(const PixelPos &delta)
{
	if (this->Type->NumDirections == 1 || (delta.x == 0 && delta.y == 0)) {
		return;
	}

	if (this->SpriteFrame < 0) {
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
	this->SpriteFrame /= this->Type->NumDirections / 2 + 1;
	this->SpriteFrame *= this->Type->NumDirections / 2 + 1;

	const int nextdir = 256 / this->Type->NumDirections;
	Assert(nextdir != 0);
	const int dir = ((DirectionToHeading(delta) + nextdir / 2) & 0xFF) / nextdir;
	if (dir <= LookingS / nextdir) { // north->east->south
		this->SpriteFrame += dir;
	} else {
		this->SpriteFrame += 256 / nextdir - dir;
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
}

/**
**  Init the move.
**
**  @param missile  missile to initialise for movement.
**
**  @return         true if goal is reached, false else.
*/
bool MissileInitMove(Missile &missile)
{
	const PixelPos heading = missile.destination - missile.position;

	missile.MissileNewHeadingFromXY(heading);
	if (!(missile.State & 1)) {
		missile.CurrentStep = 0;
		missile.TotalStep = 0;
		if (heading.x == 0 && heading.y == 0) {
			return true;
		}
		// initialize
		missile.TotalStep = Distance(missile.source, missile.destination);
		missile.State++;
		return false;
	}
	Assert(missile.TotalStep != 0);
	missile.CurrentStep += missile.Type->Speed;
	if (missile.CurrentStep >= missile.TotalStep) {
		missile.CurrentStep = missile.TotalStep;
		return true;
	}
	return false;
}

void MissileHandlePierce(Missile &missile, const Vec2i &pos)
{
	//Wyrmgus start
//	if (Map.Info.IsPointOnMap(pos) == false) {
	if (Map.Info.IsPointOnMap(pos, missile.MapLayer) == false) {
	//Wyrmgus end
		return;
	}
	std::vector<CUnit *> units;
	//Wyrmgus start
//	Select(pos, pos, units);
	Select(pos, pos, units, missile.MapLayer);
	//Wyrmgus end
	for (std::vector<CUnit *>::iterator it = units.begin(); it != units.end(); ++it) {
		CUnit &unit = **it;

		if (unit.IsAliveOnMap()
			//Wyrmgus start
//			&& (missile.Type->FriendlyFire == false || unit.IsEnemy(*missile.SourceUnit->Player))) {
			&& (missile.Type->FriendlyFire == true || unit.IsEnemy(*missile.SourceUnit))) {
			//Wyrmgus end
			missile.MissileHit(&unit);
		}
	}
}

bool MissileHandleBlocking(Missile &missile, const PixelPos &position)
{
	const MissileType &mtype = *missile.Type;
	if (missile.SourceUnit) {
		bool shouldHit = false;
		if (missile.TargetUnit && missile.SourceUnit->Type->UnitType == missile.TargetUnit->Type->UnitType) {
			shouldHit = true;
		}
		if (mtype.Range && mtype.CorrectSphashDamage) {
			shouldHit = true;
		}
		if (shouldHit) {
			// search for blocking units
			std::vector<CUnit *> blockingUnits;
			const Vec2i missilePos = Map.MapPixelPosToTilePos(position);
			//Wyrmgus start
//			Select(missilePos, missilePos, blockingUnits);
			Select(missilePos, missilePos, blockingUnits, missile.MapLayer);
			//Wyrmgus end
			for (std::vector<CUnit *>::iterator it = blockingUnits.begin();	it != blockingUnits.end(); ++it) {
				CUnit &unit = **it;
				// If land unit shoots at land unit, missile can be blocked by Wall units
				if (!missile.Type->IgnoreWalls && missile.SourceUnit->Type->UnitType == UnitTypeLand) {
					if (!missile.TargetUnit || missile.TargetUnit->Type->UnitType == UnitTypeLand) {
						if (&unit != missile.SourceUnit && unit.Type->BoolFlag[WALL_INDEX].value
							&& unit.Player != missile.SourceUnit->Player && unit.IsAllied(*missile.SourceUnit) == false) {
							if (missile.TargetUnit) {
								missile.TargetUnit = &unit;
								if (unit.Type->TileSize.x == 1 || unit.Type->TileSize.y == 1) {
									missile.position = Map.TilePosToMapPixelPos_TopLeft(unit.tilePos);
								}
							} else {
								missile.position = position;
							}
							missile.DestroyMissile = 1;
							return true;
						}
					}
				}
				// missile can kill any unit on it's way
				if (missile.Type->KillFirstUnit && &unit != missile.SourceUnit) {
					// can't kill non-solid or dead units
					if (unit.IsAliveOnMap() == false || unit.Type->BoolFlag[NONSOLID_INDEX].value) {
						continue;
					}
					//Wyrmgus start
//					if (missile.Type->FriendlyFire == false || unit.IsEnemy(*missile.SourceUnit->Player)) {
					if (missile.Type->FriendlyFire == true || unit.IsEnemy(*missile.SourceUnit)) {
					//Wyrmgus end
						missile.TargetUnit = &unit;
						if (unit.Type->TileSize.x == 1 || unit.Type->TileSize.y == 1) {
							missile.position = Map.TilePosToMapPixelPos_TopLeft(unit.tilePos);
						}
						missile.DestroyMissile = 1;
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
**  Handle point to point missile.
**
**  @param missile  Missile pointer.
**
**  @return         true if goal is reached, false else.
*/
bool PointToPointMissile(Missile &missile)
{
	MissileInitMove(missile);
	if (missile.TotalStep == 0) {
		return true;
	}
	Assert(missile.Type != NULL);
	Assert(missile.TotalStep != 0);

	const PixelPos diff = (missile.destination - missile.source);
	const PixelPrecise sign(diff.x >= 0 ? 1.0 : -1.0, diff.y >= 0 ? 1.0 : -1.0); // Remember sign to move into correct direction
	const PixelPrecise oldPos((double)missile.position.x, (double)missile.position.y); // Remember old position
	PixelPrecise pos(oldPos);
	missile.position = missile.source + diff * missile.CurrentStep / missile.TotalStep;

	for (; pos.x * sign.x <= missile.position.x * sign.x
		 && pos.y * sign.y <= missile.position.y * sign.y;
		 pos.x += (double)diff.x * missile.Type->SmokePrecision / missile.TotalStep,
		 pos.y += (double)diff.y * missile.Type->SmokePrecision / missile.TotalStep) {
		const PixelPos position((int)pos.x + missile.Type->size.x / 2,
								(int)pos.y + missile.Type->size.y / 2);

		if (missile.Type->Smoke.Missile && (missile.CurrentStep || missile.State > 1)) {
			//Wyrmgus start
//			Missile *smoke = MakeMissile(*missile.Type->Smoke.Missile, position, position);
			Missile *smoke = MakeMissile(*missile.Type->Smoke.Missile, position, position, missile.MapLayer);
			//Wyrmgus end
			if (smoke && smoke->Type->NumDirections > 1) {
				smoke->MissileNewHeadingFromXY(diff);
			}
		}

		if (missile.Type->SmokeParticle && (missile.CurrentStep || missile.State > 1)) {
			missile.Type->SmokeParticle->pushPreamble();
			missile.Type->SmokeParticle->pushInteger(position.x);
			missile.Type->SmokeParticle->pushInteger(position.y);
			missile.Type->SmokeParticle->run();
		}

		if (missile.Type->Pierce) {
			const PixelPos posInt((int)pos.x, (int)pos.y);
			MissileHandlePierce(missile, Map.MapPixelPosToTilePos(posInt));
		}
	}

	// Handle wall blocking and kill first enemy
	for (pos = oldPos; pos.x * sign.x <= missile.position.x * sign.x
		 && pos.y * sign.y <= missile.position.y * sign.y;
		 pos.x += (double)diff.x / missile.TotalStep,
		 pos.y += (double)diff.y / missile.TotalStep) {
		const PixelPos position((int)pos.x + missile.Type->size.x / 2,
								(int)pos.y + missile.Type->size.y / 2);
		const Vec2i tilePos(Map.MapPixelPosToTilePos(position));

		//Wyrmgus start
//		if (Map.Info.IsPointOnMap(tilePos) && MissileHandleBlocking(missile, position)) {
		if (Map.Info.IsPointOnMap(tilePos, missile.MapLayer) && MissileHandleBlocking(missile, position)) {
		//Wyrmugs end
			return true;
		}
		if (missile.Type->MissileStopFlags) {
			//Wyrmgus start
//			if (!Map.Info.IsPointOnMap(tilePos)) { // gone outside
			if (!Map.Info.IsPointOnMap(tilePos, missile.MapLayer)) { // gone outside
			//Wyrmgus end
				missile.TTL = 0;
				return false;
			}
			//Wyrmgus start
//			const CMapField &mf = *Map.Field(tilePos);
			const CMapField &mf = *Map.Field(tilePos, missile.MapLayer);
			//Wyrmgus end
			if (missile.Type->MissileStopFlags & mf.Flags) { // incompatible terrain
				missile.position = position;
				missile.MissileHit();
				missile.TTL = 0;
				return false;
			}
		}
	}

	if (missile.CurrentStep == missile.TotalStep) {
		missile.position = missile.destination;
		return true;
	}
	return false;
}

/**
**  Missile hits the goal.
**
**  @param missile  Missile hitting the goal.
**  @param goal     Goal of the missile.
**  @param splash   Splash damage divisor.
*/
static void MissileHitsGoal(const Missile &missile, CUnit &goal, int splash)
{
	if (!missile.Type->CanHitOwner && missile.SourceUnit == &goal) {
		return;
	}
	
	if (goal.CurrentAction() != UnitActionDie) {
		//Wyrmgus start
		if (goal.Type->BoolFlag[ITEM_INDEX].value && splash != 1) { //don't damage items with splash damage
			return;
		}
		
		if (!missile.AlwaysHits && CalculateHit(*missile.SourceUnit, *goal.Stats, &goal) == false) {
			if (splash == 1 && missile.Type->Range <= 1) {
				return;
			} else if (splash == 1 && missile.Type->Range > 1) {
				splash = missile.Type->SplashFactor; // if missile has splash but missed, apply splash damage
				if (missile.Type->SplashFactor == 0) {
					splash = 1;
				}
			}
		}
		//Wyrmgus end

		int damage;

		if (missile.Type->Damage) {   // custom formula
			Assert(missile.SourceUnit != NULL);
			//Wyrmgus start
//			damage = CalculateDamage(*missile.SourceUnit, goal, missile.Type->Damage) / splash;
			damage = CalculateDamage(*missile.SourceUnit, goal, missile.Type->Damage, &missile) / splash;
			//Wyrmgus end
		} else if (missile.Damage) {  // direct damage, spells mostly
			damage = missile.Damage / splash;
		} else {
			Assert(missile.SourceUnit != NULL);
			//Wyrmgus start
//			damage = CalculateDamage(*missile.SourceUnit, goal, Damage) / splash;
			damage = CalculateDamage(*missile.SourceUnit, goal, Damage, &missile) / splash;
			//Wyrmgus end
		}
		if (missile.Type->Pierce) {  // Handle pierce factor
			for (size_t i = 0; i < (missile.PiercedUnits.size() - 1); ++i) {
				damage *= (double)missile.Type->ReduceFactor / 100;
			}
		}

		HitUnit(missile.SourceUnit, goal, damage, &missile);
		//Wyrmgus start
		if (missile.Type->Damage == 0 && missile.Damage == 00 && goal.IsAlive()) {
			HitUnit_NormalHitSpecialDamageEffects(*missile.SourceUnit, goal);
		}
		//Wyrmgus end
		
		//Wyrmgus start
		//apply Thorns damage if attacker is at melee range
		if (goal.Variable[THORNSDAMAGE_INDEX].Value && missile.SourceUnit->MapDistanceTo(goal) <= 1) {
			int thorns_damage = std::max<int>(goal.Variable[THORNSDAMAGE_INDEX].Value - missile.SourceUnit->Variable[ARMOR_INDEX].Value, 1);
			if (GameSettings.NoRandomness) {
				thorns_damage -= ((thorns_damage + 2) / 2) / 2; //if no randomness setting is used, then the damage will always return what would have been the average damage with randomness
			} else {
				thorns_damage -= SyncRand() % ((thorns_damage + 2) / 2);
			}
			HitUnit(&goal, *missile.SourceUnit, thorns_damage);
		}
		//Wyrmgus end
	}
}

/**
**  Missile hits wall.
**
**  @param missile  Missile hitting the goal.
**  @param tilePos  Wall map tile position.
**  @param splash   Splash damage divisor.
**
**  @todo FIXME: Support for more races.
*/
static void MissileHitsWall(const Missile &missile, const Vec2i &tilePos, int splash)
{
	CUnitStats *stats; // stat of the wall.

	//Wyrmgus start
//	if (!Map.WallOnMap(tilePos)) {
	if (!Map.WallOnMap(tilePos, missile.MapLayer)) {
	//Wyrmgus end
		return;
	}
	if (missile.Damage) {  // direct damage, spells mostly
		//Wyrmgus start
//		Map.HitWall(tilePos, missile.Damage / splash);
		Map.HitWall(tilePos, missile.Damage / splash, missile.MapLayer);
		//Wyrmgus end
		return;
	}

	Assert(missile.SourceUnit != NULL);
	//Wyrmgus start
	/*
	if (Map.HumanWallOnMap(tilePos)) {
		stats = UnitTypeHumanWall->Stats;
	} else {
		Assert(Map.OrcWallOnMap(tilePos));
		stats = UnitTypeOrcWall->Stats;
	}
	*/
	stats = Map.Field(tilePos, missile.MapLayer)->OverlayTerrain->UnitType->Stats;
	//Wyrmgus end

	//Wyrmgus start
	if (!missile.AlwaysHits && CalculateHit(*missile.SourceUnit, *stats, NULL) == false) {
		if (splash == 1 && missile.Type->Range <= 1) {
			return;
		} else if (splash == 1 && missile.Type->Range > 1) {
			splash = missile.Type->SplashFactor; // if missile has splash but missed, apply splash damage
			if (missile.Type->SplashFactor == 0) {
				splash = 1;
			}
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	Map.HitWall(tilePos, CalculateDamageStats(*missile.SourceUnit->Stats, *stats, 0) / splash);
	Map.HitWall(tilePos, CalculateDamageStats(*missile.SourceUnit, *stats, NULL, &missile) / splash, missile.MapLayer);
	//Wyrmgus end
}

/**
**  Check if missile has already pierced that unit
**
**  @param missile  Current missile.
**  @param unit     Target unit.
**
**  @return         true if goal is pierced, false else.
*/

static bool IsPiercedUnit(const Missile &missile, const CUnit &unit)
{
	for (std::vector<CUnit *>::const_iterator it = missile.PiercedUnits.begin();
		 it != missile.PiercedUnits.end(); ++it) {
		CUnit &punit = **it;
		if (UnitNumber(unit) == UnitNumber(punit)) {
			return true;
		}
	}
	return false;
}

/**
**  Work for missile hit.
*/
void Missile::MissileHit(CUnit *unit)
{
	const MissileType &mtype = *this->Type;

	if (mtype.ImpactSound.Sound) {
		PlayMissileSound(*this, mtype.ImpactSound.Sound);
	}
	const PixelPos pixelPos = this->position + this->Type->size / 2;

	//
	// The impact generates a new missile.
	//
	if (mtype.Impact.empty() == false) {
		for (std::vector<MissileConfig *>::const_iterator it = mtype.Impact.begin(); it != mtype.Impact.end(); ++it) {
			const MissileConfig &mc = **it;
			//Wyrmgus start
//			Missile *impact = MakeMissile(*mc.Missile, pixelPos, pixelPos);
			Missile *impact = MakeMissile(*mc.Missile, pixelPos, pixelPos, this->MapLayer);
			//Wyrmgus end
			if (impact && impact->Type->Damage) {
				impact->SourceUnit = this->SourceUnit;
			}
		}
	}
	if (mtype.ImpactParticle) {
		mtype.ImpactParticle->pushPreamble();
		mtype.ImpactParticle->pushInteger(pixelPos.x);
		mtype.ImpactParticle->pushInteger(pixelPos.y);
		mtype.ImpactParticle->run();
	}

	if (!this->SourceUnit) {  // no owner - green-cross ...
		return;
	}

	const Vec2i pos = Map.MapPixelPosToTilePos(pixelPos);

	//Wyrmgus start
//	if (!Map.Info.IsPointOnMap(pos)) {
	if (!Map.Info.IsPointOnMap(pos, this->MapLayer)) {
	//Wyrmgus end
		// FIXME: this should handled by caller?
		DebugPrint("Missile gone outside of map!\n");
		return;  // outside the map.
	}

	//
	// Choose correct goal.
	//
	if (unit) {
		if (unit->Destroyed) {
			return;
		}
		if (mtype.Pierce && mtype.PierceOnce) {
			if (IsPiercedUnit(*this, *unit)) {
				return;
			} else {
				PiercedUnits.insert(this->PiercedUnits.begin(), unit);
			}
		}
		MissileHitsGoal(*this, *unit, 1);
		return;
	}
	if (!mtype.Range) {
		//Wyrmgus start
//		if (this->TargetUnit && (mtype.FriendlyFire == false
		if (this->TargetUnit && (mtype.FriendlyFire == true
		//Wyrmgus end
								//Wyrmgus start
//								 || this->TargetUnit->Player->Index != this->SourceUnit->Player->Index)) {
								 || this->TargetUnit->IsEnemy(*this->SourceUnit))) {
								//Wyrmgus end
			//
			// Missiles without range only hits the goal always.
			//
			CUnit &goal = *this->TargetUnit;
			if (mtype.Pierce && mtype.PierceOnce) {
				if (IsPiercedUnit(*this, goal)) {
					return;
				} else {
					PiercedUnits.insert(this->PiercedUnits.begin(), &goal);
				}
			}
			if (goal.Destroyed) {
				this->TargetUnit = NULL;
				return;
			}
			MissileHitsGoal(*this, goal, 1);
			return;
		}
		MissileHitsWall(*this, pos, 1);
		return;
	}

	{
		//
		// Hits all units in range.
		//
		const Vec2i range(mtype.Range - 1, mtype.Range - 1);
		std::vector<CUnit *> table;
		//Wyrmgus start
//		Select(pos - range, pos + range, table);
		Select(pos - range, pos + range, table, this->MapLayer);
		//Wyrmgus end
		Assert(this->SourceUnit != NULL);
		for (size_t i = 0; i != table.size(); ++i) {
			CUnit &goal = *table[i];
			//
			// Can the unit attack this unit-type?
			// NOTE: perhaps this should be come a property of the missile.
			// Also check CorrectSphashDamage so land explosions can't hit the air units
			//
			if (CanTarget(*this->SourceUnit->Type, *goal.Type)
				//Wyrmgus start
//				&& (mtype.FriendlyFire == false || goal.Player->Index != this->SourceUnit->Player->Index)) {
				&& (mtype.FriendlyFire == true || goal.IsEnemy(*this->SourceUnit))) {
				//Wyrmgus end
				bool shouldHit = true;

				if (mtype.Pierce && mtype.PierceOnce) {
					if (IsPiercedUnit(*this, goal)) {
						shouldHit = false;
					} else {
						PiercedUnits.insert(this->PiercedUnits.begin(), &goal);
					}
				}

				if (mtype.CorrectSphashDamage == true) {
					bool isPosition = false;
					if (this->TargetUnit == NULL) {
						if (this->SourceUnit->CurrentAction() == UnitActionSpellCast) {
							const COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(this->SourceUnit->CurrentOrder());
							if (order.GetSpell().Target == TargetPosition) {
								isPosition = true;
							}
						} else {
							isPosition = true;
						}
					}
					if (isPosition || this->SourceUnit->CurrentAction() == UnitActionAttackGround) {
						if (goal.Type->UnitType != this->SourceUnit->Type->UnitType) {
							shouldHit = false;
						}
					} else {
						if (this->TargetUnit == NULL || goal.Type->UnitType != this->TargetUnit->Type->UnitType) {
							shouldHit = false;
						}
					}
				}
				if (shouldHit) {
					//Wyrmgus start
//					int splash = goal.MapDistanceTo(pos);
					int splash = goal.MapDistanceTo(pos, this->MapLayer);
					//Wyrmgus end

					if (splash) {
						splash *= mtype.SplashFactor;
						//Wyrmgus start
						if (splash == 0) { // if splash factor is set to 0, cause constant splash damage, regardless of distance
							splash = 1;
						}
						//Wyrmgus end
					} else {
						splash = 1;
					}
					MissileHitsGoal(*this, goal, splash);
				}
			}
		}
	}

	// Missile hits ground.
	const Vec2i offset(mtype.Range, mtype.Range);
	const Vec2i posmin = pos - offset;
	for (int i = mtype.Range * 2; --i;) {
		for (int j = mtype.Range * 2; --j;) {
			const Vec2i posIt(posmin.x + i, posmin.y + j);

			//Wyrmgus start
//			if (Map.Info.IsPointOnMap(posIt)) {
			if (Map.Info.IsPointOnMap(posIt, this->MapLayer)) {
			//Wyrmgus end
				int d = Distance(pos, posIt);
				d *= mtype.SplashFactor;
				if (d == 0) {
					d = 1;
				}
				MissileHitsWall(*this, posIt, d);
			}
		}
	}
}

/**
**  Pass to the next frame for animation.
**
**  @param sign           1 for next frame, -1 for previous frame.
**  @param longAnimation  1 if Frame is conditioned by covered distance, 0 else.
**
**  @return               true if animation is finished, false else.
*/
bool Missile::NextMissileFrame(char sign, char longAnimation)
{
	int neg = 0; // True for mirroring sprite.
	bool animationIsFinished = false;
	int numDirections = this->Type->NumDirections / 2 + 1;
	if (this->SpriteFrame < 0) {
		neg = 1;
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
	if (longAnimation) {
		// Total distance to cover.
		const int totalx = Distance(this->destination, this->source);
		// Covered distance.
		const int dx = Distance(this->position, this->source);
		// Total number of frame (for one direction).
		const int totalf = this->Type->SpriteFrames / numDirections;
		// Current frame (for one direction).
		const int df = this->SpriteFrame / numDirections;

		if ((sign == 1 && dx * totalf <= df * totalx)
			|| (sign == -1 && dx * totalf > df * totalx)) {
			return animationIsFinished;
		}
	}
	this->SpriteFrame += sign * numDirections;
	if (sign > 0) {
		if (this->SpriteFrame >= this->Type->SpriteFrames) {
			this->SpriteFrame -= this->Type->SpriteFrames;
			animationIsFinished = true;
		}
	} else {
		if (this->SpriteFrame < 0) {
			this->SpriteFrame += this->Type->SpriteFrames;
			animationIsFinished = true;
		}
	}
	if (neg) {
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
	return animationIsFinished;
}

/**
**  Pass the next frame of the animation.
**  This animation goes from start to finish ONCE on the way
*/
void Missile::NextMissileFrameCycle()
{
	int neg = 0;

	if (this->SpriteFrame < 0) {
		neg = 1;
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
	const int totalx = abs(this->destination.x - this->source.x);
	const int dx = abs(this->position.x - this->source.x);
	int f = this->Type->SpriteFrames / (this->Type->NumDirections / 2 + 1);
	f = 2 * f - 1;
	for (int i = 1, j = 1; i <= f; ++i) {
		if (dx * f / i < totalx) {
			if ((i - 1) * 2 < f) {
				j = i - 1;
			} else {
				j = f - i;
			}
			this->SpriteFrame = this->SpriteFrame % (this->Type->NumDirections / 2 + 1) +
								j * (this->Type->NumDirections / 2 + 1);
			break;
		}
	}
	if (neg) {
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
}

/**
**  Handle all missile actions of global/local missiles.
**
**  @param missiles  Table of missiles.
*/
static void MissilesActionLoop(std::vector<Missile *> &missiles)
{
	for (size_t i = 0; i != missiles.size(); /* empty */) {
		Missile &missile = *missiles[i];

		if (missile.Delay) {
			missile.Delay--;
			++i;
			continue;  // delay start of missile
		}
		if (missile.TTL > 0) {
			missile.TTL--;  // overall time to live if specified
		}
		if (missile.TTL == 0) {
			delete &missile;
			missiles.erase(missiles.begin() + i);
			continue;
		}
		Assert(missile.Wait);
		if (--missile.Wait) {  // wait until time is over
			++i;
			continue;
		}
		missile.Action(); // may create other missiles, and so modifies the array
		if (missile.TTL == 0) {
			delete &missile;
			missiles.erase(missiles.begin() + i);
			continue;
		}
		++i;
	}
}

/**
**  Handle all missile actions.
*/
void MissileActions()
{
	MissilesActionLoop(GlobalMissiles);
	MissilesActionLoop(LocalMissiles);
}

/**
**  Calculate distance from view-point to missile.
**
**  @param missile  Missile pointer for distance.
**
**  @return the computed value.
*/
int ViewPointDistanceToMissile(const Missile &missile)
{
	const PixelPos pixelPos = missile.position + missile.Type->size / 2;
	const Vec2i tilePos = Map.MapPixelPosToTilePos(pixelPos);

	return ViewPointDistance(tilePos);
}

/**
**  Get the burning building missile based on hp percent.
**
**  @param percent  HP percent
**
**  @return  the missile used for burning.
*/
MissileType *MissileBurningBuilding(int percent)
{
	for (std::vector<BurningBuildingFrame *>::iterator i = BurningBuildingFrames.begin();
		 i != BurningBuildingFrames.end(); ++i) {
		//Wyrmgus start
//		if (percent > (*i)->Percent) {
		if (percent >= (*i)->Percent) {
		//Wyrmgus end
			return (*i)->Missile;
		}
	}
	return NULL;
}

/**
**  Save a specific pos.
*/
static void SavePixelPos(CFile &file, const PixelPos &pos)
{
	file.printf("{%d, %d}", pos.x, pos.y);
}


/**
**  Save the state of a missile to file.
**
**  @param file  Output file.
*/
void Missile::SaveMissile(CFile &file) const
{
	file.printf("Missile(\"type\", \"%s\",", this->Type->Ident.c_str());
	file.printf(" \"%s\",", this->Local ? "local" : "global");
	file.printf(" \"pos\", ");
	SavePixelPos(file, this->position);
	file.printf(", \"origin-pos\", ");
	SavePixelPos(file, this->source);
	file.printf(", \"goal\", ");
	SavePixelPos(file, this->destination);
	file.printf(",\n  \"frame\", %d, \"state\", %d, \"anim-wait\", %d, \"wait\", %d, \"delay\", %d,\n ",
				this->SpriteFrame, this->State, this->AnimWait, this->Wait, this->Delay);
	if (this->SourceUnit != NULL) {
		file.printf(" \"source\", \"%s\",", UnitReference(this->SourceUnit).c_str());
	}
	if (this->TargetUnit != NULL) {
		file.printf(" \"target\", \"%s\",", UnitReference(this->TargetUnit).c_str());
	}
	file.printf(" \"damage\", %d,", this->Damage);
	file.printf(" \"ttl\", %d,", this->TTL);
	if (this->Hidden) {
		file.printf(" \"hidden\", ");
	}
	file.printf(" \"step\", {%d, %d}", this->CurrentStep, this->TotalStep);

	// Slot filled in during init
	file.printf(")\n");
}

/**
**  Save the state missiles to file.
**
**  @param file  Output file.
*/
void SaveMissiles(CFile &file)
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: missiles\n\n");

	std::vector<Missile *>::const_iterator i;

	for (i = GlobalMissiles.begin(); i != GlobalMissiles.end(); ++i) {
		(*i)->SaveMissile(file);
	}
	for (i = LocalMissiles.begin(); i != LocalMissiles.end(); ++i) {
		(*i)->SaveMissile(file);
	}
}

/**
**  Initialize missile type.
*/
void MissileType::Init()
{
	// Resolve impact missiles and sounds.
	this->FiredSound.MapSound();
	this->ImpactSound.MapSound();
	for (std::vector<MissileConfig *>::iterator it = this->Impact.begin(); it != this->Impact.end(); ++it) {
		MissileConfig &mc = **it;

		mc.MapMissile();
	}
	this->Smoke.MapMissile();
}

/**
**  Initialize missile-types.
*/
void InitMissileTypes()
{
	for (MissileTypeMap::iterator it = MissileTypes.begin(); it != MissileTypes.end(); ++it) {
		(*it).second->Init();
	}
}

/**
**  Constructor.
*/
MissileType::MissileType(const std::string &ident) :
	Ident(ident), Transparency(0), DrawLevel(0),
	SpriteFrames(0), NumDirections(0), ChangeVariable(-1), ChangeAmount(0), ChangeMax(false),
	//Wyrmgus start
//	CorrectSphashDamage(false), Flip(false), CanHitOwner(false), FriendlyFire(false),
	CorrectSphashDamage(false), Flip(false), CanHitOwner(false), FriendlyFire(true),
	//Wyrmgus end
	AlwaysFire(false), Pierce(false), PierceOnce(false), IgnoreWalls(true), KillFirstUnit(false),
	//Wyrmgus start
	AlwaysHits(false),
	//Wyrmgus end
	Class(), NumBounces(0),	ParabolCoefficient(2048), StartDelay(0),
	//Wyrmgus start
//	Sleep(0), Speed(0), BlizzardSpeed(0), TTL(-1), ReduceFactor(100), SmokePrecision(0),
	Sleep(0), Speed(0), BlizzardSpeed(0), AttackSpeed(0), TTL(-1), ReduceFactor(100), SmokePrecision(0),
	//Wyrmgus end
	MissileStopFlags(0), Damage(NULL), Range(0), SplashFactor(0),
	ImpactParticle(NULL), SmokeParticle(NULL), OnImpact(NULL),
	G(NULL)
{
	size.x = 0;
	size.y = 0;
}

/**
**  Destructor.
*/
MissileType::~MissileType()
{
	CGraphic::Free(this->G);
	Impact.clear();
	delete ImpactParticle;
	delete SmokeParticle;
	delete OnImpact;
	FreeNumberDesc(this->Damage);
}

/**
**  Clean up missile-types.
*/
void CleanMissileTypes()
{
	for (MissileTypeMap::iterator it = MissileTypes.begin(); it != MissileTypes.end(); ++it) {
		delete it->second;
	}
	MissileTypes.clear();
}

/**
**  Initialize missiles.
*/
void InitMissiles()
{
}

/**
**  Missile destructior.
*/
Missile::~Missile()
{
	PiercedUnits.clear();
}

/**
**  Clean up missiles.
*/
void CleanMissiles()
{
	std::vector<Missile *>::const_iterator i;

	for (i = GlobalMissiles.begin(); i != GlobalMissiles.end(); ++i) {
		delete *i;
	}
	GlobalMissiles.clear();
	for (i = LocalMissiles.begin(); i != LocalMissiles.end(); ++i) {
		delete *i;
	}
	LocalMissiles.clear();
}

void FreeBurningBuildingFrames()
{
	for (std::vector<BurningBuildingFrame *>::iterator i = BurningBuildingFrames.begin();
		 i != BurningBuildingFrames.end(); ++i) {
		delete *i;
	}
	BurningBuildingFrames.clear();
}

//@}
