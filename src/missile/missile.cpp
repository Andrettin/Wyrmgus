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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "missile.h"

#include "action/action_spellcast.h"
#include "actions.h"
#include "animation/animation_set.h"
#include "config.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "missile/missile_class.h"
#include "mod.h"
#include "player/player.h"
#include "script.h"
#include "script/trigger.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "sound/unit_sound_type.h"
#include "spell/spell.h"
#include "spell/spell_target_type.h"
#include "spell/status_effect.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/point_util.h"
#include "util/string_conversion_util.h"
#include "util/util.h"
#include "video/font.h"
#include "video/video.h"

#ifdef __MORPHOS__
#undef Wait
#endif

unsigned int Missile::Count = 0;

static std::vector<std::unique_ptr<Missile>> GlobalMissiles;    /// all global missiles on map
static std::vector<std::unique_ptr<Missile>> LocalMissiles;     /// all local missiles on map

std::vector<std::unique_ptr<BurningBuildingFrame>> BurningBuildingFrames; /// Burning building frames

namespace wyrmgus {

void missile_type::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "flip") {
			this->Flip = string::to_bool(value);
		} else if (key == "transparency") {
			this->Transparency = std::stoi(value);
		} else if (key == "num_bounces") {
			this->NumBounces = std::stoi(value);
		} else if (key == "max_bounce_size") {
			this->MaxBounceSize = std::stoi(value);
		} else if (key == "parabol_coefficient") {
			this->ParabolCoefficient = std::stoi(value);
		} else if (key == "delay") {
			this->StartDelay = std::stoi(value);
		} else if (key == "blizzard_speed") {
			this->BlizzardSpeed = std::stoi(value);
		} else if (key == "attack_speed") {
			this->AttackSpeed = std::stoi(value);
		} else if (key == "ttl") {
			this->TTL = std::stoi(value);
		} else if (key == "reduce_factor") {
			this->ReduceFactor = std::stoi(value);
		} else if (key == "smoke_precision") {
			this->SmokePrecision = std::stoi(value);
		} else if (key == "missile_stop_flags") {
			this->MissileStopFlags = string_to_tile_flag(value);
		} else if (key == "smoke_missile") {
			value = FindAndReplaceString(value, "_", "-");
			this->Smoke.Name = value;
		} else if (key == "can_hit_owner") {
			this->CanHitOwner = string::to_bool(value);
		} else if (key == "always_fire") {
			this->AlwaysFire = string::to_bool(value);
		} else if (key == "pierce") {
			this->Pierce = string::to_bool(value);
		} else if (key == "pierce_once") {
			this->PierceOnce = string::to_bool(value);
		} else if (key == "pierce_ignore_before_goal") {
			this->PierceIgnoreBeforeGoal = string::to_bool(value);
		} else if (key == "ignore_walls") {
			this->IgnoreWalls = string::to_bool(value);
		} else if (key == "kill_first_unit") {
			this->KillFirstUnit = string::to_bool(value);
		} else if (key == "friendly_fire") {
			this->FriendlyFire = string::to_bool(value);
		} else if (key == "always_hits") {
			this->AlwaysHits = string::to_bool(value);
		} else if (key == "splash_factor") {
			this->SplashFactor = std::stoi(value);
		} else if (key == "correct_sphash_damage") {
			this->CorrectSphashDamage = string::to_bool(value);
		} else {
			fprintf(stderr, "Invalid missile type property: \"%s\".\n", key.c_str());
		}
	}
}

void missile_type::initialize()
{
	if (!this->SmokePrecision) {
		this->SmokePrecision = this->get_speed();
	}

	if (!this->get_image_file().empty()) {
		this->G = CGraphic::New(this->get_image_file().string(), this->get_frame_size());
	}

	data_entry::initialize();
}

void missile_type::set_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_file()) {
		return;
	}

	this->image_file = database::get()->get_graphics_filepath(filepath);
}

/**
**  Load the graphics for a missile type
*/
void missile_type::LoadMissileSprite()
{
	if (this->G && !this->G->IsLoaded()) {
		this->G->Load(preferences::get()->get_scale_factor());

		// Correct the number of frames in graphic
		assert_throw(this->G->NumFrames >= this->get_frames());
		this->G->NumFrames = this->get_frames();
		// FIXME: Don't use NumFrames as number of frames.
	}
}

}

int GetMissileSpritesCount()
{
#ifndef DYNAMIC_LOAD
	return wyrmgus::missile_type::get_all().size();
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
	for (wyrmgus::missile_type *missile_type : wyrmgus::missile_type::get_all()) {
		missile_type->LoadMissileSprite();
	}
#endif
}

/**
**  Constructor
*/
Missile::Missile()
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
std::unique_ptr<Missile> Missile::Init(const wyrmgus::missile_type &mtype, const PixelPos &startPos, const PixelPos &destPos, int z)
{
	std::unique_ptr<Missile> missile;

	switch (mtype.get_missile_class()) {
		case wyrmgus::missile_class::none:
			missile = std::make_unique<MissileNone>();
			break;
		case wyrmgus::missile_class::point_to_point:
			missile = std::make_unique<MissilePointToPoint>();
			break;
		case wyrmgus::missile_class::point_to_point_with_hit:
			missile = std::make_unique<MissilePointToPointWithHit>();
			break;
		case wyrmgus::missile_class::point_to_point_cycle_once:
			missile = std::make_unique<MissilePointToPointCycleOnce>();
			break;
		case wyrmgus::missile_class::point_to_point_bounce:
			missile = std::make_unique<MissilePointToPointBounce>();
			break;
		case wyrmgus::missile_class::stay:
			missile = std::make_unique<MissileStay>();
			break;
		case wyrmgus::missile_class::cycle_once:
			missile = std::make_unique<MissileCycleOnce>();
			break;
		case wyrmgus::missile_class::fire:
			missile = std::make_unique<MissileFire>();
			break;
		case wyrmgus::missile_class::hit:
			missile = std::make_unique<::MissileHit>();
			break;
		case wyrmgus::missile_class::parabolic:
			missile = std::make_unique<MissileParabolic>();
			break;
		case wyrmgus::missile_class::land_mine:
			missile = std::make_unique<MissileLandMine>();
			break;
		case wyrmgus::missile_class::whirlwind:
			missile = std::make_unique<MissileWhirlwind>();
			break;
		case wyrmgus::missile_class::flame_shield:
			missile = std::make_unique<MissileFlameShield>();
			break;
		case wyrmgus::missile_class::death_coil:
			missile = std::make_unique<MissileDeathCoil>();
			break;
		case wyrmgus::missile_class::tracer:
			missile = std::make_unique<MissileTracer>();
			break;
		case wyrmgus::missile_class::clip_to_target:
			missile = std::make_unique<MissileClipToTarget>();
			break;
		case wyrmgus::missile_class::continuous:
			missile = std::make_unique<wyrmgus::missile_continuous>();
			break;
		case wyrmgus::missile_class::straight_fly:
			missile = std::make_unique<MissileStraightFly>();
			break;
	}
	const PixelPos halfSize = mtype.get_frame_size() / 2;
	missile->position = startPos - halfSize;
	missile->destination = destPos - halfSize;
	missile->source = missile->position;
	//Wyrmgus start
	missile->MapLayer = z;
	//Wyrmgus end
	missile->Type = &mtype;
	missile->Wait = mtype.get_sleep();
	missile->Delay = mtype.StartDelay;
	missile->TTL = mtype.TTL;
	//Wyrmgus start
	missile->AlwaysHits = mtype.AlwaysHits;
	//Wyrmgus end
	if (mtype.get_fired_sound() != nullptr) {
		PlayMissileSound(*missile, mtype.get_fired_sound());
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
Missile *MakeMissile(const wyrmgus::missile_type &mtype, const PixelPos &startPos, const PixelPos &destPos, int z)
{
	std::unique_ptr<Missile> missile = Missile::Init(mtype, startPos, destPos, z);
	Missile *missile_ptr = missile.get();
	GlobalMissiles.push_back(std::move(missile));
	return missile_ptr;
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
Missile *MakeLocalMissile(const wyrmgus::missile_type &mtype, const PixelPos &startPos, const PixelPos &destPos, int z)
{
	std::unique_ptr<Missile> missile = Missile::Init(mtype, startPos, destPos, z);
	missile->Local = 1;
	Missile *missile_ptr = missile.get();
	LocalMissiles.push_back(std::move(missile));
	return missile_ptr;
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
//static int CalculateDamageStats(const unit_stats &attacker_stats,
//								const unit_stats &goal_stats, int bloodlust)
static int CalculateDamageStats(const CUnit &attacker, const unit_stats &goal_stats, const CUnit *goal, const Missile *missile = nullptr)
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
	if (attacker.has_status_effect(status_effect::infusion)) {
		arcane_damage += 4; //+4 arcane damage bonus from Infusion
	}
	
	int lightning_damage = attacker.Variable[LIGHTNINGDAMAGE_INDEX].Value;
	int air_damage = attacker.Variable[AIRDAMAGE_INDEX].Value;
	int earth_damage = attacker.Variable[EARTHDAMAGE_INDEX].Value;
	int water_damage = attacker.Variable[WATERDAMAGE_INDEX].Value;
	int acid_damage = attacker.Variable[ACIDDAMAGE_INDEX].Value;
	int shadow_damage = attacker.Variable[SHADOW_DAMAGE_INDEX].Value;
	//Wyrmgus end

	//Wyrmgus start
	/*
	if (bloodlust) {
		basic_damage *= 2;
		piercing_damage *= 2;
	}
	*/
	//apply damage modifiers, but don't stack the effects of Blessing, Inspire, Bloodlust and Leadership
	int damage_modifier = 100;
	if (attacker.has_status_effect(status_effect::bloodlust)) {
		damage_modifier += 100;
	} else if (attacker.has_status_effect(status_effect::inspire) || attacker.has_status_effect(status_effect::blessing)) {
		damage_modifier += 50;
	} else if (attacker.has_status_effect(status_effect::leadership)) {
		damage_modifier += 10;
	} else if (attacker.has_status_effect(status_effect::wither)) {
		damage_modifier -= 50;
	}
	
	if (attacker.Variable[CHARGEBONUS_INDEX].Value != 0) {
		damage_modifier += attacker.Variable[CHARGEBONUS_INDEX].Value * attacker.get_step_count();
	}
	
	int armor = 0;
	if (goal != nullptr) {
		armor = goal->Variable[ARMOR_INDEX].Value;
		
		if (goal->has_status_effect(status_effect::barkskin)) {
			armor += 4; //+4 armor bonus from Barkskin
		}
	} else {
		armor = goal_stats.Variables[ARMOR_INDEX].Value;
	}
	
	int critical_strike_chance = attacker.Variable[CRITICALSTRIKECHANCE_INDEX].Value;
	if (missile && missile->AlwaysCritical) {
		critical_strike_chance = 100;
	}

	if (critical_strike_chance > 0) {
		if (SyncRand(100) < critical_strike_chance) {
			damage_modifier += 100;
		}
	}

	if (goal != nullptr) {
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
		acid_damage *= 100 - goal->Variable[ACIDRESISTANCE_INDEX].Value;
		acid_damage /= 100;
		shadow_damage *= 100 - goal->Variable[SHADOW_RESISTANCE_INDEX].Value;
		shadow_damage /= 100;
		
		// extra backstab damage (only works against units (that are organic and non-building, and that have 8 facing directions) facing opposite to the attacker
		if (attacker.Variable[BACKSTAB_INDEX].Value > 0 && goal->Type->BoolFlag[ORGANIC_INDEX].value && !goal->Type->BoolFlag[BUILDING_INDEX].value && goal->Type->get_num_directions() == 8) {
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
		acid_damage *= 100 - goal_stats.Variables[ACIDRESISTANCE_INDEX].Value;
		acid_damage /= 100;
		shadow_damage *= 100 - goal_stats.Variables[SHADOW_RESISTANCE_INDEX].Value;
		shadow_damage /= 100;
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
	piercing_damage += acid_damage;
	piercing_damage += shadow_damage;
	piercing_damage *= damage_modifier;
	piercing_damage /= 100;
	//Wyrmgus end

	int damage = std::max<int>(basic_damage - armor, 1);
	damage += piercing_damage;
	
	//Wyrmgus start
	//apply hack/pierce/blunt resistances
	if (goal != nullptr) {
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

	damage -= SyncRand((damage + 2) / 2);

	assert_throw(damage >= 0);

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
	assert_throw(formula != nullptr);

	UpdateUnitVariables(const_cast<CUnit &>(attacker));
	UpdateUnitVariables(const_cast<CUnit &>(goal));
	TriggerData.Attacker = const_cast<CUnit *>(&attacker);
	TriggerData.Defender = const_cast<CUnit *>(&goal);
	const int res = EvalNumber(formula);
	TriggerData.Attacker = nullptr;
	TriggerData.Defender = nullptr;
	return res;
}

//Wyrmgus start
/**
**  Calculate hit.
**
**  @return                whether the target was hit or not.
*/
static bool CalculateHit(const CUnit &attacker, const unit_stats &goal_stats, const CUnit *goal)
{
	if (GodMode && attacker.Player == CPlayer::GetThisPlayer() && (!goal || goal->Player != CPlayer::GetThisPlayer())) {
		return true; //always hit if in god mode
	}

	if (attacker.Type->BoolFlag[TRAP_INDEX].value) { // traps always hit
		return true;
	}
	
	int accuracy_modifier = 100;
	if (attacker.has_status_effect(status_effect::precision)) {
		accuracy_modifier += 100;
	}
	
	int evasion_modifier = 100;
	if (goal && goal->has_status_effect(status_effect::blessing)) {
		evasion_modifier += 50;
	}
	
	int accuracy = attacker.Variable[ACCURACY_INDEX].Value;
	accuracy *= accuracy_modifier;
	accuracy /= 100;
	
	if (accuracy == 0) {
		return false;
	} else {
		int evasion = 0;
		if (goal != nullptr) {
			if (goal->Variable[EVASION_INDEX].Value && !goal->has_status_effect(status_effect::stun)) {
				//stunned targets cannot evade
				evasion = goal->Variable[EVASION_INDEX].Value;
			}
			if (goal->Type->BoolFlag[ORGANIC_INDEX].value && !goal->Type->BoolFlag[BUILDING_INDEX].value && goal->Type->get_num_directions() == 8) { //flanking
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
		
		evasion *= evasion_modifier;
		evasion /= 100;
		
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
		assert_throw(!unit.GetMissile().Missile->AlwaysFire || unit.GetMissile().Missile->get_range());
		if (goal->Destroyed) {
			DebugPrint("destroyed unit\n");
			return;
		}
		if (goal->Removed) {
			return;
		}
		if (goal->CurrentAction() == UnitAction::Die) {
			//Wyrmgus start
//			if (unit.Type->Missile.Missile->AlwaysFire) {
			if (unit.GetMissile().Missile->AlwaysFire) {
			//Wyrmgus end
				newgoalPos = goal->tilePos;
				new_z = goal->MapLayer->ID;
				goal = nullptr;
			} else {
				return;
			}
		}
	}

	// No missile hits immediately!
	if (
		unit.GetMissile().Missile->get_missile_class() == wyrmgus::missile_class::none
		//Wyrmgus start
//		|| (unit.Type->Animations && unit.Type->Animations->Attack && unit.Type->Animations->RangedAttack && !unit.IsAttackRanged(goal, goalPos)) // treat melee attacks from units that have both attack and ranged attack animations as having missile class none
		|| (unit.get_animation_set() && unit.get_animation_set()->Attack && unit.get_animation_set()->RangedAttack && !unit.IsAttackRanged(goal, goalPos, z)) // treat melee attacks from units that have both attack and ranged attack animations as having missile class none
		//Wyrmgus end
	) {
		//Wyrmgus start
		int damage = 0;
		//Wyrmgus end
		// No goal, take target coordinates
		if (!goal) {
			if (CMap::get()->WallOnMap(goalPos, z)) {
				//Wyrmgus start
//				if (CMap::get()->HumanWallOnMap(goalPos)) {
				if (CMap::get()->Field(goalPos, z)->get_overlay_terrain()->get_unit_type() && CalculateHit(unit, *CMap::get()->Field(goalPos, z)->get_overlay_terrain()->get_unit_type()->Stats, nullptr) == true) {
				//Wyrmgus end
					//Wyrmgus start
					PlayUnitSound(&unit, wyrmgus::unit_sound_type::hit);
					damage = CalculateDamageStats(unit, *CMap::get()->Field(goalPos, z)->get_overlay_terrain()->get_unit_type()->Stats, nullptr);
					//Wyrmgus end
					CMap::get()->HitWall(goalPos,
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
			damage = CalculateDamage(unit, *goal, Damage.get());
			HitUnit(&unit, *goal, damage);
			if (goal->IsAlive()) {
				HitUnit_NormalHitSpecialDamageEffects(unit, *goal);
			}
			PlayUnitSound(&unit, unit_sound_type::hit);
			
			//apply thorns damage if attacker is at melee range
			if (goal && goal->Variable[THORNSDAMAGE_INDEX].Value && unit.MapDistanceTo(*goal) <= 1) {
				int thorns_damage = std::max<int>(goal->Variable[THORNSDAMAGE_INDEX].Value - unit.Variable[ARMOR_INDEX].Value, 1);
				thorns_damage -= SyncRand((thorns_damage + 2) / 2);
				HitUnit(goal, unit, thorns_damage);
			}
		} else {
			PlayUnitSound(&unit, unit_sound_type::miss);
		}
		//Wyrmgus end
		return;
	}

	// If Firing from inside a Bunker
	CUnit *from = unit.GetFirstContainer();
	const int dir = ((unit.Direction + NextDirection / 2) & 0xFF) / NextDirection;
	const PixelPos startPixelPos = CMap::get()->tile_pos_to_map_pixel_pos_top_left(from->tilePos) + PixelSize(from->Type->get_half_tile_pixel_size().x, from->Type->get_half_tile_pixel_size().y)
								   + unit.Type->MissileOffsets[dir][0];

	Vec2i dpos;
	if (goal) {
		assert_throw(goal->Type != nullptr);  // Target invalid?
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
			NearestOfUnit(*goal, unit.GetFirstContainer()->tilePos, &dpos);
		} else {
			dpos = goal->tilePos + goal->GetHalfTileSize();
			z = goal->MapLayer->ID;
		}
	} else {
		dpos = newgoalPos;
		// FIXME: Can this be too near??
		//Wyrmgus start
		z = new_z;
		//Wyrmgus end
	}

	PixelPos destPixelPos = CMap::get()->tile_pos_to_map_pixel_pos_center(dpos);
	//Wyrmgus start
//	Missile *missile = MakeMissile(*unit.Type->Missile.Missile, startPixelPos, destPixelPos);
	Missile *missile = MakeMissile(*unit.GetMissile().Missile, startPixelPos, destPixelPos, z);
	//Wyrmgus end
	//
	// Damage of missile
	//
	if (goal) {
		missile->TargetUnit = goal->acquire_ref();
	}
	missile->SourceUnit = unit.acquire_ref();

	//for pierce missiles, make them continue up to the limits of the attacker's range
	if (missile->Type->Pierce) {
		for (int i = 0; i < (unit.GetModifiedVariable(ATTACKRANGE_INDEX) - unit.MapDistanceTo(dpos, z)); ++i) {
			const PixelPos diff(missile->destination - missile->source);
			missile->destination += diff * ((defines::get()->get_tile_width() + defines::get()->get_tile_height()) * 3) / 4 / point::distance_to(missile->source, missile->destination);
		}
	}
	
	PlayUnitSound(&unit, wyrmgus::unit_sound_type::fire_missile);
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
static void GetMissileMapArea(const Missile &missile, QPoint &box_min, QPoint &box_max)
{
	PixelSize missileSize(missile.Type->get_frame_size());
	PixelDiff margin(defines::get()->get_tile_width() - 1, defines::get()->get_tile_height() - 1);
	box_min = CMap::get()->map_pixel_pos_to_tile_pos(missile.position);
	box_max = CMap::get()->map_pixel_pos_to_tile_pos(missile.position + missileSize + margin);

	CMap::get()->clamp(box_min, missile.MapLayer);
	CMap::get()->clamp(box_max, missile.MapLayer);
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
	QPoint boxmin;
	QPoint boxmax;

	GetMissileMapArea(missile, boxmin, boxmax);

	if (!vp.AnyMapAreaVisibleInViewport(boxmin, boxmax)) {
		return 0;
	}

	Vec2i pos;
	for (pos.x = boxmin.x(); pos.x <= boxmax.x(); ++pos.x) {
		for (pos.y = boxmin.y(); pos.y <= boxmax.y(); ++pos.y) {
			//Wyrmgus start
//			if (ReplayRevealMap || CMap::get()->Field(pos)->player_info->IsTeamVisible(*ThisPlayer)) {
			if (ReplayRevealMap || CMap::get()->Field(pos, missile.MapLayer)->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
			//Wyrmgus end
				return 1;
			}
		}
	}
	return 0;
}

namespace wyrmgus {

/**
**  Draw missile.
**
**  @param frame  Animation frame
**  @param pos    Screen pixel position
*/
void missile_type::DrawMissileType(int frame, const PixelPos &pos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
#ifdef DYNAMIC_LOAD
	if (!this->G->IsLoaded()) {
		LoadMissileSprite(this);
	}
#endif

	if (this->Flip) {
		if (frame < 0) {
			if (this->Transparency > 0) {
				this->G->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * Transparency), render_commands);
			} else {
				this->G->DrawFrameClipX(-frame - 1, pos.x, pos.y, render_commands);
			}
		} else {
			if (this->Transparency > 0) {
				this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency), render_commands);
			} else {
				this->G->DrawFrameClip(frame, pos.x, pos.y, render_commands);
			}
		}
	} else {
		const int row = this->get_num_directions() / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * this->get_num_directions() + this->get_num_directions() - (-frame - 1) % row;
		} else {
			frame = (frame / row) * this->get_num_directions() + frame % row;
		}
		if (this->Transparency > 0) {
			this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency), render_commands);
		} else {
			this->G->DrawFrameClip(frame, pos.x, pos.y, render_commands);
		}
	}
}

}

/**
**  Draw missile.
*/
void Missile::DrawMissile(const CViewport &vp, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	assert_throw(this->Type != nullptr);
	CUnit *sunit = this->get_source_unit();
	// FIXME: I should copy SourcePlayer for second level missiles.
	if (sunit && sunit->Player) {
#ifdef DYNAMIC_LOAD
		if (!this->Type->Sprite) {
			LoadMissileSprite(this->Type);
		}
#endif
	}
	const PixelPos screenPixelPos = vp.map_to_screen_pixel_pos(this->position);

	switch (this->Type->get_missile_class()) {
		case missile_class::hit:
			CLabel(defines::get()->get_game_font()).DrawClip(screenPixelPos.x, screenPixelPos.y, this->Damage, render_commands);
			break;
		default:
			if (Type->G) {
				this->Type->DrawMissileType(this->SpriteFrame, screenPixelPos, render_commands);
			}
			break;
	}
}

static bool MissileDrawLevelCompare(const Missile *const l, const Missile *const r)
{
	if (l->Type->get_draw_level() == r->Type->get_draw_level()) {
		return l->Slot < r->Slot;
	} else {
		return l->Type->get_draw_level() < r->Type->get_draw_level();
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
	typedef std::vector<std::unique_ptr<Missile>>::const_iterator MissilePtrConstiterator;

	// Loop through global missiles, then through locals.
	for (MissilePtrConstiterator i = GlobalMissiles.begin(); i != GlobalMissiles.end(); ++i) {
		Missile &missile = *(*i);
		//Wyrmgus start
//		if (missile.Delay || missile.Hidden) {
		if (missile.Delay || missile.Hidden || missile.MapLayer != UI.CurrentMapLayer->ID) {
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
		if (missile.Delay || missile.Hidden || missile.MapLayer != UI.CurrentMapLayer->ID) {
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
	if (this->Type->get_num_directions() == 1 || (delta.x == 0 && delta.y == 0)) {
		return;
	}

	if (this->SpriteFrame < 0) {
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
	this->SpriteFrame /= this->Type->get_num_directions() / 2 + 1;
	this->SpriteFrame *= this->Type->get_num_directions() / 2 + 1;

	const int nextdir = 256 / this->Type->get_num_directions();
	assert_throw(nextdir != 0);
	const int dir = ((DirectionToHeading(delta) + nextdir / 2) & 0xFF) / nextdir;
	if (dir <= LookingS / nextdir) { // north->east->south
		this->SpriteFrame += dir;
	} else {
		this->SpriteFrame += 256 / nextdir - dir;
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
}

CUnit *Missile::get_source_unit() const
{
	if (this->SourceUnit == nullptr) {
		return nullptr;
	}

	return this->SourceUnit->get();
}

CUnit *Missile::get_target_unit() const
{
	if (this->TargetUnit == nullptr) {
		return nullptr;
	}

	return this->TargetUnit->get();
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
		missile.TotalStep = point::distance_to(missile.source, missile.destination);
		missile.State++;
		return false;
	}
	assert_throw(missile.TotalStep != 0);
	missile.CurrentStep += missile.Type->get_speed();
	if (missile.CurrentStep >= missile.TotalStep) {
		missile.CurrentStep = missile.TotalStep;
		return true;
	}
	return false;
}

void MissileHandlePierce(Missile &missile, const Vec2i &pos)
{
	if (CMap::get()->Info->IsPointOnMap(pos, missile.MapLayer) == false) {
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
			&& (missile.Type->FriendlyFire == true || unit.is_enemy_of(*missile.get_source_unit()))
			&& missile.get_source_unit() != &unit //don't hit the source unit, otherwise it will be hit by pierce as soon as it fires the missile
			&& (!missile.Type->PierceOnce || !IsPiercedUnit(missile, unit))
			&& missile.get_source_unit()->Type->can_target(&unit)
			&& !unit.Type->BoolFlag[DECORATION_INDEX].value
			&& (!missile.Type->PierceIgnoreBeforeGoal || !missile.get_target_unit() || IsPiercedUnit(missile, *missile.get_target_unit()) || missile.get_target_unit() == &unit)
		) {
			missile.MissileHit(&unit);
		}
	}
}

bool MissileHandleBlocking(Missile &missile, const PixelPos &position)
{
	const wyrmgus::missile_type &mtype = *missile.Type;
	if (missile.get_source_unit() != nullptr) {
		bool shouldHit = false;
		if (missile.get_target_unit() && missile.get_source_unit()->Type->get_domain() == missile.get_target_unit()->Type->get_domain()) {
			shouldHit = true;
		}
		if (mtype.get_range() && mtype.CorrectSphashDamage) {
			shouldHit = true;
		}
		if (shouldHit) {
			// search for blocking units
			std::vector<CUnit *> blockingUnits;
			const Vec2i missilePos = CMap::get()->map_pixel_pos_to_tile_pos(position);
			Select(missilePos, missilePos, blockingUnits, missile.MapLayer);
			for (std::vector<CUnit *>::iterator it = blockingUnits.begin();	it != blockingUnits.end(); ++it) {
				CUnit &unit = **it;
				// If land unit shoots at land unit, missile can be blocked by Wall units
				if (!missile.Type->IgnoreWalls && missile.get_source_unit()->Type->get_domain() == unit_domain::land) {
					if (!missile.get_target_unit() || missile.get_target_unit()->Type->get_domain() == unit_domain::land) {
						if (&unit != missile.get_source_unit() && unit.Type->BoolFlag[WALL_INDEX].value
							&& unit.Player != missile.get_source_unit()->Player && unit.is_allied_with(*missile.get_source_unit()) == false) {
							if (missile.get_target_unit()) {
								missile.TargetUnit = unit.acquire_ref();
								if (unit.Type->get_tile_width() == 1 || unit.Type->get_tile_height() == 1) {
									missile.position = CMap::get()->tile_pos_to_map_pixel_pos_top_left(unit.tilePos);
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
				if (missile.Type->KillFirstUnit && &unit != missile.get_source_unit()) {
					// can't kill non-solid or dead units
					if (unit.IsAliveOnMap() == false || unit.Type->BoolFlag[NONSOLID_INDEX].value) {
						continue;
					}
					//Wyrmgus start
//					if (missile.Type->FriendlyFire == false || unit.is_enemy_of(*missile.get_source_unit()->Player)) {
					if (missile.Type->FriendlyFire == true || unit.is_enemy_of(*missile.get_source_unit())) {
					//Wyrmgus end
						missile.TargetUnit = unit.acquire_ref();
						if (unit.Type->get_tile_width() == 1 || unit.Type->get_tile_height() == 1) {
							missile.position = CMap::get()->tile_pos_to_map_pixel_pos_top_left(unit.tilePos);
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
	assert_throw(missile.Type != nullptr);
	assert_throw(missile.TotalStep != 0);

	const PixelPos diff = (missile.destination - missile.source);
	const PixelPrecise sign(diff.x >= 0 ? 1.0 : -1.0, diff.y >= 0 ? 1.0 : -1.0); // Remember sign to move into correct direction
	const PixelPrecise oldPos((double)missile.position.x, (double)missile.position.y); // Remember old position
	PixelPrecise pos(oldPos);
	missile.position = missile.source + diff * missile.CurrentStep / missile.TotalStep;

	for (; pos.x * sign.x <= missile.position.x * sign.x
		 && pos.y * sign.y <= missile.position.y * sign.y;
		 pos.x += (double)diff.x * missile.Type->SmokePrecision / missile.TotalStep,
		 pos.y += (double)diff.y * missile.Type->SmokePrecision / missile.TotalStep) {
		const PixelPos position((int)pos.x + missile.Type->get_frame_width() / 2,
								(int)pos.y + missile.Type->get_frame_height() / 2);

		if (missile.Type->Smoke.Missile && (missile.CurrentStep || missile.State > 1)) {
			//Wyrmgus start
//			Missile *smoke = MakeMissile(*missile.Type->Smoke.Missile, position, position);
			Missile *smoke = MakeMissile(*missile.Type->Smoke.Missile, position, position, missile.MapLayer);
			//Wyrmgus end
			if (smoke && smoke->Type->get_num_directions() > 1) {
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
			const PixelPos posInt((int)pos.x + missile.Type->get_frame_width() / 2, (int)pos.y + missile.Type->get_frame_height() / 2);
			MissileHandlePierce(missile, CMap::get()->map_pixel_pos_to_tile_pos(posInt));
		}
	}

	// Handle wall blocking and kill first enemy
	for (pos = oldPos; pos.x * sign.x <= missile.position.x * sign.x
		 && pos.y * sign.y <= missile.position.y * sign.y;
		 pos.x += (double)diff.x / missile.TotalStep,
		 pos.y += (double)diff.y / missile.TotalStep) {
		const PixelPos position((int)pos.x + missile.Type->get_frame_width() / 2,
								(int)pos.y + missile.Type->get_frame_height() / 2);
		const Vec2i tilePos(CMap::get()->map_pixel_pos_to_tile_pos(position));

		if (CMap::get()->Info->IsPointOnMap(tilePos, missile.MapLayer) && MissileHandleBlocking(missile, position)) {
			return true;
		}
		if (missile.Type->MissileStopFlags != tile_flag::none) {
			if (!CMap::get()->Info->IsPointOnMap(tilePos, missile.MapLayer)) { // gone outside
				missile.TTL = 0;
				return false;
			}
			//Wyrmgus start
//			const wyrmgus::tile &mf = *CMap::get()->Field(tilePos);
			const wyrmgus::tile &mf = *CMap::get()->Field(tilePos, missile.MapLayer);
			//Wyrmgus end
			if ((missile.Type->MissileStopFlags & mf.Flags) != tile_flag::none) { // incompatible terrain
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
	if (!missile.Type->CanHitOwner && missile.get_source_unit() == &goal) {
		return;
	}
	
	if (goal.CurrentAction() != UnitAction::Die) {
		//Wyrmgus start
		if (goal.Type->BoolFlag[ITEM_INDEX].value && splash != 1) { //don't damage items with splash damage
			return;
		}
		
		if (!missile.AlwaysHits && CalculateHit(*missile.get_source_unit(), *goal.Stats, &goal) == false) {
			if (splash == 1 && missile.Type->SplashFactor <= 0) {
				return;
			} else if (splash == 1 && missile.Type->SplashFactor > 0) {
				splash = missile.Type->SplashFactor; //if missile has splash but missed, apply splash damage
			}
		}
		//Wyrmgus end

		int damage;

		if (missile.Type->Damage) {   // custom formula
			assert_throw(missile.get_source_unit() != nullptr);
			//Wyrmgus start
//			damage = CalculateDamage(*missile.get_source_unit(), goal, missile.Type->Damage) / splash;
			damage = CalculateDamage(*missile.get_source_unit(), goal, missile.Type->Damage.get(), &missile) / splash;
			//Wyrmgus end
		} else if (missile.Damage || missile.LightningDamage) {  // direct damage, spells mostly
			damage = missile.Damage / splash;
			damage += missile.LightningDamage * (100 - goal.Variable[LIGHTNINGRESISTANCE_INDEX].Value) / 100 / splash;
		} else {
			assert_throw(missile.get_source_unit() != nullptr);
			//Wyrmgus start
//			damage = CalculateDamage(*missile.get_source_unit(), goal, Damage) / splash;
			damage = CalculateDamage(*missile.get_source_unit(), goal, Damage.get(), &missile) / splash;
			//Wyrmgus end
		}
		if (missile.Type->Pierce && !missile.PiercedUnits.empty()) {  // Handle pierce factor
			for (size_t i = 0; i < (missile.PiercedUnits.size() - 1); ++i) {
				damage *= (double)missile.Type->ReduceFactor / 100;
			}
		}

		HitUnit(missile.get_source_unit(), goal, damage, &missile);
		//Wyrmgus start
		if (missile.Type->Damage == 0 && missile.Damage == 0 && missile.LightningDamage == 0 && goal.IsAlive()) {
			HitUnit_NormalHitSpecialDamageEffects(*missile.get_source_unit(), goal);
		}
		//Wyrmgus end
		
		//Wyrmgus start
		//apply Thorns damage if attacker is at melee range
		if (goal.Variable[THORNSDAMAGE_INDEX].Value && missile.get_source_unit()->MapDistanceTo(goal) <= 1) {
			int thorns_damage = std::max<int>(goal.Variable[THORNSDAMAGE_INDEX].Value - missile.get_source_unit()->Variable[ARMOR_INDEX].Value, 1);
			thorns_damage -= SyncRand((thorns_damage + 2) / 2);
			HitUnit(&goal, *missile.get_source_unit(), thorns_damage);
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
	//Wyrmgus start
//	if (!CMap::get()->WallOnMap(tilePos)) {
	if (!CMap::get()->WallOnMap(tilePos, missile.MapLayer)) {
	//Wyrmgus end
		return;
	}
	
	const unit_stats *stats = CMap::get()->Field(tilePos, missile.MapLayer)->get_overlay_terrain()->get_unit_type()->Stats;
	
	if (missile.Damage || missile.LightningDamage) {  // direct damage, spells mostly
		int damage = missile.Damage / splash;
		damage += missile.LightningDamage * (100 - stats->Variables[LIGHTNINGRESISTANCE_INDEX].Value) / 100 / splash;
		CMap::get()->HitWall(tilePos, damage, missile.MapLayer);
		return;
	}

	assert_throw(missile.get_source_unit() != nullptr);

	//Wyrmgus start
	if (!missile.AlwaysHits && CalculateHit(*missile.get_source_unit(), *stats, nullptr) == false) {
		if (splash == 1 && missile.Type->SplashFactor <= 0) {
			return;
		} else if (splash == 1 && missile.Type->SplashFactor > 0) {
			splash = missile.Type->SplashFactor; // if missile has splash but missed, apply splash damage
		}
	}
	//Wyrmgus end

	//Wyrmgus start
//	CMap::get()->HitWall(tilePos, CalculateDamageStats(*missile.get_source_unit()->Stats, *stats, 0) / splash);
	CMap::get()->HitWall(tilePos, CalculateDamageStats(*missile.get_source_unit(), *stats, nullptr, &missile) / splash, missile.MapLayer);
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

bool IsPiercedUnit(const Missile &missile, const CUnit &unit)
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
	const wyrmgus::missile_type &mtype = *this->Type;

	if (mtype.get_impact_sound() != nullptr) {
		PlayMissileSound(*this, mtype.get_impact_sound());
	}
	const PixelPos pixelPos = this->position + mtype.get_frame_size() / 2;

	//
	// The impact generates a new missile.
	//
	if (mtype.Impact.empty() == false) {
		for (const MissileConfig &mc : mtype.Impact) {
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

	if (!this->get_source_unit()) {  // no owner - green-cross ...
		return;
	}

	const Vec2i pos = CMap::get()->map_pixel_pos_to_tile_pos(pixelPos);

	if (!CMap::get()->Info->IsPointOnMap(pos, this->MapLayer)) {
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
		if (mtype.get_missile_class() == wyrmgus::missile_class::point_to_point_bounce && (unit->Type->get_tile_width() > mtype.MaxBounceSize || unit->Type->get_tile_height() > mtype.MaxBounceSize)) {
			this->TTL = 0;
		}
		return;
	}
	if (!mtype.get_range()) {
		//Wyrmgus start
//		if (this->get_target_unit() && (mtype.FriendlyFire == false
		if (this->get_target_unit() && (mtype.FriendlyFire == true
		//Wyrmgus end
								//Wyrmgus start
//								 || this->get_target_unit()->Player != this->get_source_unit()->Player)) {
								 || this->get_target_unit()->is_enemy_of(*this->get_source_unit()))) {
								//Wyrmgus end
			//
			// Missiles without range only hits the goal always.
			//
			CUnit &goal = *this->get_target_unit();
			if (mtype.Pierce && mtype.PierceOnce) {
				if (IsPiercedUnit(*this, goal)) {
					return;
				} else {
					PiercedUnits.insert(this->PiercedUnits.begin(), &goal);
				}
			}
			if (goal.Destroyed) {
				this->TargetUnit.reset();
				return;
			}
			int splash = 1;
			if (mtype.get_missile_class() == wyrmgus::missile_class::point_to_point_bounce && this->State > 3) {
				splash = mtype.SplashFactor;
			}
			MissileHitsGoal(*this, goal, splash);
			if (mtype.get_missile_class() == wyrmgus::missile_class::point_to_point_bounce && (goal.Type->get_tile_width() > mtype.MaxBounceSize || goal.Type->get_tile_height() > mtype.MaxBounceSize)) {
				this->TTL = 0;
			}
			return;
		}
		MissileHitsWall(*this, pos, 1);
		return;
	}

	{
		//
		// Hits all units in range.
		//
		const Vec2i range(mtype.get_range() - 1, mtype.get_range() - 1);
		std::vector<CUnit *> table;
		//Wyrmgus start
//		Select(pos - range, pos + range, table);
		Select(pos - range, pos + range, table, this->MapLayer);
		//Wyrmgus end
		assert_throw(this->get_source_unit() != nullptr);
		for (size_t i = 0; i != table.size(); ++i) {
			CUnit &goal = *table[i];
			//
			// Can the unit attack this unit-type?
			// NOTE: perhaps this should be come a property of the missile.
			// Also check CorrectSphashDamage so land explosions can't hit the air units
			//
			if (this->get_source_unit()->Type->can_target(&goal)
				//Wyrmgus start
//				&& (mtype.FriendlyFire == false || goal.Player != this->get_source_unit()->Player)) {
				&& (mtype.FriendlyFire == true || goal.is_enemy_of(*this->get_source_unit()))) {
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
					if (this->get_target_unit() == nullptr) {
						if (this->get_source_unit()->CurrentAction() == UnitAction::SpellCast) {
							const COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(this->get_source_unit()->CurrentOrder());
							if (order.GetSpell().get_target() == wyrmgus::spell_target_type::position) {
								isPosition = true;
							}
						} else {
							isPosition = true;
						}
					}
					if (isPosition || this->get_source_unit()->CurrentAction() == UnitAction::AttackGround) {
						if (goal.Type->get_domain() != this->get_source_unit()->Type->get_domain()) {
							shouldHit = false;
						}
					} else {
						if (this->get_target_unit() == nullptr || goal.Type->get_domain() != this->get_target_unit()->Type->get_domain()) {
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
						if (mtype.get_missile_class() == wyrmgus::missile_class::point_to_point_bounce && this->State > 3) {
							splash = mtype.SplashFactor;
						}
					}
					MissileHitsGoal(*this, goal, splash);
					if (mtype.get_missile_class() == wyrmgus::missile_class::point_to_point_bounce && (goal.Type->get_tile_width() > mtype.MaxBounceSize || goal.Type->get_tile_height() > mtype.MaxBounceSize)) {
						this->TTL = 0;
					}
				}
			}
		}
	}

	// Missile hits ground.
	const Vec2i offset(mtype.get_range(), mtype.get_range());
	const Vec2i posmin = pos - offset;
	for (int i = mtype.get_range() * 2; --i;) {
		for (int j = mtype.get_range() * 2; --j;) {
			const Vec2i posIt(posmin.x + i, posmin.y + j);

			if (CMap::get()->Info->IsPointOnMap(posIt, this->MapLayer)) {
				int d = point::distance_to(pos, posIt);
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
	int num_directions = this->Type->get_num_directions() / 2 + 1;
	if (this->SpriteFrame < 0) {
		neg = 1;
		this->SpriteFrame = -this->SpriteFrame - 1;
	}
	if (longAnimation) {
		// Total distance to cover.
		const int totalx = point::distance_to(this->destination, this->source);
		// Covered distance.
		const int dx = point::distance_to(this->position, this->source);
		// Total number of frame (for one direction).
		const int totalf = this->Type->get_frames() / num_directions;
		// Current frame (for one direction).
		const int df = this->SpriteFrame / num_directions;

		if ((sign == 1 && dx * totalf <= df * totalx)
			|| (sign == -1 && dx * totalf > df * totalx)) {
			return animationIsFinished;
		}
	}
	this->SpriteFrame += sign * num_directions;
	if (sign > 0) {
		if (this->SpriteFrame >= this->Type->get_frames()) {
			this->SpriteFrame -= this->Type->get_frames();
			animationIsFinished = true;
		}
	} else {
		if (this->SpriteFrame < 0) {
			this->SpriteFrame += this->Type->get_frames();
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
	int f = this->Type->get_frames() / (this->Type->get_num_directions() / 2 + 1);
	f = 2 * f - 1;
	for (int i = 1, j = 1; i <= f; ++i) {
		if (dx * f / i < totalx) {
			if ((i - 1) * 2 < f) {
				j = i - 1;
			} else {
				j = f - i;
			}
			this->SpriteFrame = this->SpriteFrame % (this->Type->get_num_directions() / 2 + 1) +
								j * (this->Type->get_num_directions() / 2 + 1);
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
static void MissilesActionLoop(std::vector<std::unique_ptr<Missile>> &missiles)
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
			missiles.erase(missiles.begin() + i);
			continue;
		}
		assert_throw(missile.Wait != 0);
		if (--missile.Wait) {  // wait until time is over
			++i;
			continue;
		}
		missile.Action(); // may create other missiles, and so modifies the array
		if (missile.TTL == 0) {
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
	try {
		MissilesActionLoop(GlobalMissiles);
		MissilesActionLoop(LocalMissiles);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error executing actions for missiles."));
	}
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
	const PixelPos pixelPos = missile.position + missile.Type->get_frame_size() / 2;
	const Vec2i tilePos = CMap::get()->map_pixel_pos_to_tile_pos(pixelPos);

	return ViewPointDistance(tilePos);
}

/**
**  Get the burning building missile based on hp percent.
**
**  @param percent  HP percent
**
**  @return  the missile used for burning.
*/
const wyrmgus::missile_type *MissileBurningBuilding(const int percent)
{
	for (const std::unique_ptr<BurningBuildingFrame> &frame : BurningBuildingFrames) {
		if (percent >= frame->Percent) {
			return frame->Missile;
		}
	}
	return nullptr;
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
	file.printf("Missile(\"type\", \"%s\",", this->Type->get_identifier().c_str());
	file.printf(" \"%s\",", this->Local ? "local" : "global");
	file.printf(" \"pos\", ");
	SavePixelPos(file, this->position);
	file.printf(", \"origin-pos\", ");
	SavePixelPos(file, this->source);
	file.printf(", \"goal\", ");
	SavePixelPos(file, this->destination);
	file.printf(",\n  \"frame\", %d, \"state\", %d, \"anim-wait\", %d, \"wait\", %d, \"delay\", %d,\n ",
				this->SpriteFrame, this->State, this->AnimWait, this->Wait, this->Delay);
	if (this->get_source_unit() != nullptr) {
		file.printf(" \"source\", \"%s\",", UnitReference(this->get_source_unit()).c_str());
	}
	if (this->get_target_unit() != nullptr) {
		file.printf(" \"target\", \"%s\",", UnitReference(this->get_target_unit()).c_str());
	}
	file.printf(" \"damage\", %d,", this->Damage);
	file.printf(" \"lightning-damage\", %d,", this->LightningDamage);
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

	for (const std::unique_ptr<Missile> &missile : GlobalMissiles) {
		missile->SaveMissile(file);
	}
	for (const std::unique_ptr<Missile> &missile : LocalMissiles) {
		missile->SaveMissile(file);
	}
}

namespace wyrmgus {

void missile_type::Init()
{
	// Resolve impact missiles
	for (MissileConfig &mc : this->Impact) {
		mc.MapMissile();
	}
	this->Smoke.MapMissile();
}

}

/**
**  Initialize missile-types.
*/
void InitMissileTypes()
{
	for (wyrmgus::missile_type *missile_type : wyrmgus::missile_type::get_all()) {
		missile_type->Init();
	}
}

namespace wyrmgus {

missile_type::missile_type(const std::string &identifier)
	: data_entry(identifier),
	missile_class(missile_class::none),
	MissileStopFlags(tile_flag::none)
{
}

missile_type::~missile_type()
{
}

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
	GlobalMissiles.clear();
	LocalMissiles.clear();
}

void FreeBurningBuildingFrames()
{
	BurningBuildingFrames.clear();
}
