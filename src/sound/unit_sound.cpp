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

/*----------------------------------------------------------------------------
--  Include
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "sound/unit_sound.h"

#include "animation/animation_randomsound.h"
#include "animation/animation_sound.h"
#include "civilization.h"
#include "map/map.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

bool SoundConfig::MapSound()
{
	if (!this->Name.empty()) {
		this->Sound = SoundForName(this->Name);
	}
	return this->Sound != nullptr;
}

void SoundConfig::SetSoundRange(unsigned char range)
{
	if (this->Sound) {
		::SetSoundRange(this->Sound, range);
	}
}

/**
**  Load all sounds for units.
*/
void LoadUnitSounds()
{
}

static void MapAnimSound(CAnimation &anim)
{
	if (anim.Type == AnimationSound) {
		CAnimation_Sound &anim_sound = *static_cast<CAnimation_Sound *>(&anim);

		anim_sound.MapSound();
	} else if (anim.Type == AnimationRandomSound) {
		CAnimation_RandomSound &anim_rsound = *static_cast<CAnimation_RandomSound *>(&anim);

		anim_rsound.MapSound();
	}
}

/**
**  Map animation sounds
*/
static void MapAnimSounds2(CAnimation *anim)
{
	if (anim == nullptr) {
		return ;
	}
	MapAnimSound(*anim);
	for (CAnimation *it = anim->Next; it != anim; it = it->Next) {
		MapAnimSound(*it);
	}
}

/**
**  Map animation sounds for a unit type
*/
static void MapAnimSounds(CUnitType &type)
{
	if (!type.Animations) {
		return;
	}
	MapAnimSounds2(type.Animations->Start);
	MapAnimSounds2(type.Animations->Still);
	MapAnimSounds2(type.Animations->Move);
	MapAnimSounds2(type.Animations->Attack);
	MapAnimSounds2(type.Animations->RangedAttack);
	MapAnimSounds2(type.Animations->SpellCast);
	for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
		MapAnimSounds2(type.Animations->Death[i]);
	}
	MapAnimSounds2(type.Animations->Repair);
	MapAnimSounds2(type.Animations->Train);
	MapAnimSounds2(type.Animations->Research);
	MapAnimSounds2(type.Animations->Upgrade);
	MapAnimSounds2(type.Animations->Build);
	for (int i = 0; i < MaxCosts; ++i) {
		MapAnimSounds2(type.Animations->Harvest[i]);
	}
	//Wyrmgus start
	for (CUnitTypeVariation *variation : type.Variations) {
		if (!variation) {
			continue;
		}
		if (!variation->Animations) {
			continue;
		}
		MapAnimSounds2(variation->Animations->Start);
		MapAnimSounds2(variation->Animations->Still);
		MapAnimSounds2(variation->Animations->Move);
		MapAnimSounds2(variation->Animations->Attack);
		MapAnimSounds2(variation->Animations->RangedAttack);
		MapAnimSounds2(variation->Animations->SpellCast);
		for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
			MapAnimSounds2(variation->Animations->Death[i]);
		}
		MapAnimSounds2(variation->Animations->Repair);
		MapAnimSounds2(variation->Animations->Train);
		MapAnimSounds2(variation->Animations->Research);
		MapAnimSounds2(variation->Animations->Upgrade);
		MapAnimSounds2(variation->Animations->Build);
		for (int i = 0; i < MaxCosts; ++i) {
			MapAnimSounds2(variation->Animations->Harvest[i]);
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

	for (CCivilization *civilization : CCivilization::GetAll()) {
		civilization->UnitSounds.Selected.MapSound();
		civilization->UnitSounds.Acknowledgement.MapSound();
		civilization->UnitSounds.Attack.MapSound();
		civilization->UnitSounds.Idle.MapSound();
		civilization->UnitSounds.Build.MapSound();
		civilization->UnitSounds.Ready.MapSound();
		civilization->UnitSounds.Ready.SetSoundRange(INFINITE_SOUND_RANGE);
		civilization->UnitSounds.Repair.MapSound();
		for (int j = 0; j < MaxCosts; ++j) {
			civilization->UnitSounds.Harvest[j].MapSound();
		}
		civilization->UnitSounds.Help.MapSound();
		civilization->UnitSounds.Help.SetSoundRange(INFINITE_SOUND_RANGE);
		civilization->UnitSounds.HelpTown.MapSound();
		civilization->UnitSounds.HelpTown.SetSoundRange(INFINITE_SOUND_RANGE);
	}

	// Parse all units sounds.
	for (CUnitType *unit_type : CUnitType::GetAll()) {
		MapAnimSounds(*unit_type);

		unit_type->MapSound.Selected.MapSound();
		unit_type->MapSound.Acknowledgement.MapSound();
		//unit_type->Sound.Acknowledgement.SetSoundRange(INFINITE_SOUND_RANGE);
		unit_type->MapSound.Attack.MapSound();
		//Wyrmgus start
		unit_type->MapSound.Idle.MapSound();
		unit_type->MapSound.Hit.MapSound();
		unit_type->MapSound.Miss.MapSound();
		unit_type->MapSound.FireMissile.MapSound();
		unit_type->MapSound.Step.MapSound();
		unit_type->MapSound.StepDirt.MapSound();
		unit_type->MapSound.StepGrass.MapSound();
		unit_type->MapSound.StepGravel.MapSound();
		unit_type->MapSound.StepMud.MapSound();
		unit_type->MapSound.StepStone.MapSound();
		unit_type->MapSound.Used.MapSound();
		//Wyrmgus end
		unit_type->MapSound.Build.MapSound();
		unit_type->MapSound.Ready.MapSound();
		unit_type->MapSound.Ready.SetSoundRange(INFINITE_SOUND_RANGE);
		unit_type->MapSound.Repair.MapSound();
		for (int i = 0; i < MaxCosts; ++i) {
			unit_type->MapSound.Harvest[i].MapSound();
		}
		unit_type->MapSound.Help.MapSound();
		unit_type->MapSound.Help.SetSoundRange(INFINITE_SOUND_RANGE);

		for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
			unit_type->MapSound.Dead[i].MapSound();
		}
	}
}
