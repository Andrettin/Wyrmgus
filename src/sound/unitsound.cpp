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
/**@name unitsound.cpp - The unit sounds. */
//
//      (c) Copyright 1999-2015 by Fabrice Rossi, Jimmy Salmon and Andrettin
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

#include "stratagus.h"

#include "sound/unitsound.h"

#include "animation/animation_randomsound.h"
#include "animation/animation_sound.h"
#include "civilization.h"
#include "map/map.h"
#include "player.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "util/string_util.h"
#include "video/video.h"

bool SoundConfig::MapSound()
{
	if (!this->Name.empty()) {
		this->Sound = stratagus::sound::get(this->Name);
	}
	return this->Sound != nullptr;
}

void SoundConfig::SetSoundRange(unsigned char range)
{
	if (this->Sound) {
		::SetSoundRange(this->Sound, range);
	}
}

namespace stratagus {

void unit_sound_set::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "selected") {
		this->Selected.Name = value;
	} else if (key == "acknowledge") {
		this->Acknowledgement.Name = value;
	} else if (key == "attack") {
		this->Attack.Name = value;
	} else if (key == "idle") {
		this->Idle.Name = value;
	} else if (key == "hit") {
		this->Hit.Name = value;
	} else if (key == "miss") {
		this->Miss.Name = value;
	} else if (key == "fire_missile") {
		this->FireMissile.Name = value;
	} else if (key == "step") {
		this->Step.Name = value;
	} else if (key == "step_dirt") {
		this->StepDirt.Name = value;
	} else if (key == "step_grass") {
		this->StepGrass.Name = value;
	} else if (key == "step_gravel") {
		this->StepGravel.Name = value;
	} else if (key == "step_mud") {
		this->StepMud.Name = value;
	} else if (key == "step_stone") {
		this->StepStone.Name = value;
	} else if (key == "used") {
		this->Used.Name = value;
	} else if (key == "build") {
		this->Build.Name = value;
	} else if (key == "ready") {
		this->Ready.Name = value;
	} else if (key == "repair") {
		this->Repair.Name = value;
	} else if (key.find("harvest_") != std::string::npos) {
		std::string resource_identifier = key;
		string::replace(resource_identifier, "harvest_", "");
		const resource *resource = resource::get(resource_identifier);
		this->Harvest[resource->ID].Name = value;
	} else if (key == "help") {
		this->Help.Name = value;
	} else if (key == "help_town") {
		this->HelpTown.Name = value;
	} else if (key == "dead") {
		this->Dead[ANIMATIONS_DEATHTYPES].Name = value;
	} else if (key.find("dead_") != std::string::npos) {
		std::string death_type_identifier = key;
		string::replace(death_type_identifier, "dead_", "");
		int death;
		for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
			if (death_type_identifier == ExtraDeathTypes[death]) {
				this->Dead[death].Name = value;
				break;
			}
		}
		if (death == ANIMATIONS_DEATHTYPES) {
			throw std::runtime_error("Invalid death type: \"" + death_type_identifier + "\".");
		}
	} else {
		throw std::runtime_error("Invalid unit sound set property: \"" + key + "\".");
	}
}

void unit_sound_set::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid unit sound set scope: \"" + scope.get_tag() + "\".");
}

void unit_sound_set::map_sounds()
{
	this->Selected.MapSound();
	this->Acknowledgement.MapSound();
	this->Attack.MapSound();
	this->Idle.MapSound();
	this->Build.MapSound();
	this->Ready.MapSound();
	this->Repair.MapSound();
	this->Hit.MapSound();
	this->Miss.MapSound();
	this->FireMissile.MapSound();
	this->Step.MapSound();
	this->StepDirt.MapSound();
	this->StepGrass.MapSound();
	this->StepGravel.MapSound();
	this->StepMud.MapSound();
	this->StepStone.MapSound();
	this->Used.MapSound();
	for (int i = 0; i < MaxCosts; ++i) {
		this->Harvest[i].MapSound();
	}
	this->Help.MapSound();
	this->HelpTown.MapSound();
	for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
		this->Dead[i].MapSound();
	}
}

}

static void MapAnimSound(CAnimation *anim)
{
	if (anim->Type == AnimationSound) {
		CAnimation_Sound *anim_sound = static_cast<CAnimation_Sound *>(anim);
		anim_sound->MapSound();
	} else if (anim->Type == AnimationRandomSound) {
		CAnimation_RandomSound *anim_rsound = static_cast<CAnimation_RandomSound *>(anim);
		anim_rsound->MapSound();
	}
}

/**
**  Map animation sounds
*/
static void MapAnimSounds2(CAnimation *anim)
{
	if (anim == nullptr) {
		return;
	}

	MapAnimSound(anim);
	for (CAnimation *it = anim->get_next(); it != anim; it = it->get_next()) {
		MapAnimSound(it);
	}
}

/**
**  Map animation sounds for a unit type
*/
static void MapAnimSounds(stratagus::unit_type &type)
{
	if (!type.get_animation_set()) {
		return;
	}
	MapAnimSounds2(type.get_animation_set()->Start.get());
	MapAnimSounds2(type.get_animation_set()->Still.get());
	MapAnimSounds2(type.get_animation_set()->Move.get());
	MapAnimSounds2(type.get_animation_set()->Attack.get());
	MapAnimSounds2(type.get_animation_set()->RangedAttack.get());
	MapAnimSounds2(type.get_animation_set()->SpellCast.get());
	for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
		MapAnimSounds2(type.get_animation_set()->Death[i].get());
	}
	MapAnimSounds2(type.get_animation_set()->Repair.get());
	MapAnimSounds2(type.get_animation_set()->Train.get());
	MapAnimSounds2(type.get_animation_set()->Research.get());
	MapAnimSounds2(type.get_animation_set()->Upgrade.get());
	MapAnimSounds2(type.get_animation_set()->Build.get());
	for (int i = 0; i < MaxCosts; ++i) {
		MapAnimSounds2(type.get_animation_set()->Harvest[i].get());
	}
	//Wyrmgus start
	for (const auto &variation : type.get_variations()) {
		if (!variation) {
			throw std::runtime_error("Unit type \"" + type.get_identifier() + "\" has an null variation.");
		}
		if (!variation->Animations) {
			continue;
		}
		MapAnimSounds2(variation->Animations->Start.get());
		MapAnimSounds2(variation->Animations->Still.get());
		MapAnimSounds2(variation->Animations->Move.get());
		MapAnimSounds2(variation->Animations->Attack.get());
		MapAnimSounds2(variation->Animations->RangedAttack.get());
		MapAnimSounds2(variation->Animations->SpellCast.get());
		for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
			MapAnimSounds2(variation->Animations->Death[i].get());
		}
		MapAnimSounds2(variation->Animations->Repair.get());
		MapAnimSounds2(variation->Animations->Train.get());
		MapAnimSounds2(variation->Animations->Research.get());
		MapAnimSounds2(variation->Animations->Upgrade.get());
		MapAnimSounds2(variation->Animations->Build.get());
		for (int i = 0; i < MaxCosts; ++i) {
			MapAnimSounds2(variation->Animations->Harvest[i].get());
		}
	}
	//Wyrmgus end
}

/**
**  Map the sounds of all unit-types to the correct sound id.
**  And overwrite the sound ranges.
**  @todo the sound ranges should be configurable by user with CCL.
*/
void MapUnitSounds()
{
	if (SoundEnabled() == false) {
		return;
	}

	for (stratagus::civilization *civilization : stratagus::civilization::get_all()) {
		civilization->UnitSounds.map_sounds();
	}

	// Parse all units sounds.
	for (stratagus::unit_type *unit_type : stratagus::unit_type::get_all()) {
		MapAnimSounds(*unit_type);

		unit_type->MapSound.map_sounds();
	}
}
