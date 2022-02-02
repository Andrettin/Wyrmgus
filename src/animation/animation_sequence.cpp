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
//      (c) Copyright 1998-2022 by Lutz Sammer, Russell Smith, Jimmy Salmon
//		and Andrettin
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

#include "animation/animation_sequence.h"

#include "animation/animation.h"
#include "animation/animation_attack.h"
#include "animation/animation_die.h"
#include "animation/animation_exactframe.h"
#include "animation/animation_frame.h"
#include "animation/animation_goto.h"
#include "animation/animation_ifvar.h"
#include "animation/animation_label.h"
#include "animation/animation_move.h"
#include "animation/animation_randomgoto.h"
#include "animation/animation_randomrotate.h"
#include "animation/animation_randomsound.h"
#include "animation/animation_randomwait.h"
#include "animation/animation_rotate.h"
#include "animation/animation_setvar.h"
#include "animation/animation_sound.h"
#include "animation/animation_spawnmissile.h"
#include "animation/animation_unbreakable.h"
#include "animation/animation_wait.h"
#include "util/assert_util.h"

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

namespace wyrmgus {

void animation_sequence::AddAnimationToArray(CAnimation *anim)
{
	if (anim == nullptr) {
		return;
	}

	CAnimation::animation_list.push_back(anim);
}

animation_sequence::animation_sequence(const std::string &identifier) : data_entry(identifier)
{
}

animation_sequence::~animation_sequence()
{
}

void animation_sequence::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	std::unique_ptr<CAnimation> anim;

	if (key == "frame") {
		anim = std::make_unique<CAnimation_Frame>();
	} else if (key == "exact_frame") {
		anim = std::make_unique<CAnimation_ExactFrame>();
	} else if (key == "wait") {
		anim = std::make_unique<CAnimation_Wait>();
	} else if (key == "random_wait") {
		anim = std::make_unique<CAnimation_RandomWait>();
	} else if (key == "sound") {
		anim = std::make_unique<CAnimation_Sound>();
	} else if (key == "random_sound") {
		anim = std::make_unique<CAnimation_RandomSound>();
	} else if (key == "attack") {
		anim = std::make_unique<CAnimation_Attack>();
	} else if (key == "spawn_missile") {
		anim = std::make_unique<CAnimation_SpawnMissile>();
	} else if (key == "if_var") {
		anim = std::make_unique<CAnimation_IfVar>();
	} else if (key == "set_var") {
		anim = std::make_unique<CAnimation_SetVar>();
	} else if (key == "die") {
		anim = std::make_unique<CAnimation_Die>();
	} else if (key == "rotate") {
		anim = std::make_unique<CAnimation_Rotate>();
	} else if (key == "random_rotate") {
		anim = std::make_unique<CAnimation_RandomRotate>();
	} else if (key == "move") {
		anim = std::make_unique<CAnimation_Move>();
	} else if (key == "unbreakable") {
		anim = std::make_unique<CAnimation_Unbreakable>();
	} else if (key == "label") {
		anim = std::make_unique<CAnimation_Label>();
		AddLabel(anim.get(), value);
	} else if (key == "goto") {
		anim = std::make_unique<CAnimation_Goto>();
	} else if (key == "random_goto") {
		anim = std::make_unique<CAnimation_RandomGoto>();
	} else {
		data_entry::process_sml_property(property);
		return;
	}

	assert_throw(anim != nullptr);

	anim->Init(value.c_str(), nullptr);

	if (!this->animations.empty()) {
		this->animations.back()->set_next(anim.get());
	}

	this->animations.push_back(std::move(anim));
}

void animation_sequence::initialize()
{
	assert_throw(!this->animations.empty());

	this->animations.back()->set_next(this->animations.front().get());

	//must add to array in a fixed order for save games
	animation_sequence::AddAnimationToArray(this->animations.front().get());

	for (const std::unique_ptr<CAnimation> &anim : this->animations) {
		MapAnimSound(anim.get());
	}

	data_entry::initialize();
}

void animation_sequence::check() const
{
	for (const std::unique_ptr<CAnimation> &anim : this->animations) {
		assert_throw(anim->get_next() != nullptr);
	}
}

void animation_sequence::set_animations(std::vector<std::unique_ptr<CAnimation>> &&animations)
{
	this->animations = std::move(animations);
}

}
