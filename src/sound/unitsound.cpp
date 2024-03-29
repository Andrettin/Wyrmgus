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
//      (c) Copyright 1999-2022 by Fabrice Rossi, Jimmy Salmon and Andrettin
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

#include "sound/unitsound.h"

#include "action/action_resource.h"
#include "actions.h"
#include "animation/animation_randomsound.h"
#include "animation/animation_sound.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "player/civilization.h"
#include "player/player.h"
#include "sound/game_sound_set.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "sound/unit_sound_type.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "util/string_util.h"

bool SoundConfig::MapSound()
{
	if (!this->Name.empty()) {
		this->Sound = sound::get(this->Name);
	}
	return this->Sound != nullptr;
}

void SoundConfig::SetSoundRange(unsigned char range)
{
	if (this->Sound) {
		::SetSoundRange(this->Sound, range);
	}
}

namespace wyrmgus {

void unit_sound_set::process_gsml_property(const gsml_property &property)
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
		this->Harvest[resource->get_index()].Name = value;
	} else if (key == "help") {
		this->Help.Name = value;
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

void unit_sound_set::process_gsml_scope(const gsml_data &scope)
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
	for (int i = 0; i <= ANIMATIONS_DEATHTYPES; ++i) {
		this->Dead[i].MapSound();
	}
}

const sound *unit_sound_set::get_sound_for_unit(const unit_sound_type unit_sound_type, const CUnit *unit) const
{
	switch (unit_sound_type) {
		case unit_sound_type::acknowledging:
			return this->Acknowledgement.Sound;
		case unit_sound_type::attack:
			if (this->Attack.Sound != nullptr) {
				return this->Attack.Sound;
			}

			return this->get_sound_for_unit(unit_sound_type::acknowledging, unit);
		case unit_sound_type::idle:
			return this->Idle.Sound;
		case unit_sound_type::hit:
			return this->Hit.Sound;
		case unit_sound_type::miss:
			if (this->Miss.Sound != nullptr) {
				return this->Miss.Sound;
			}

			return this->get_sound_for_unit(unit_sound_type::hit, unit);
		case unit_sound_type::fire_missile:
			return this->FireMissile.Sound;
		case unit_sound_type::step: {
			const tile *tile = unit->MapLayer->Field(unit->tilePos);

			if (this->StepMud.Sound && (tile->has_flag(tile_flag::mud) || tile->has_flag(tile_flag::snow))) {
				return this->StepMud.Sound;
			} else if (this->StepDirt.Sound && (tile->has_flag(tile_flag::dirt) || tile->has_flag(tile_flag::ice))) {
				return this->StepDirt.Sound;
			} else if (this->StepGravel.Sound && tile->has_flag(tile_flag::gravel)) {
				return this->StepGravel.Sound;
			} else if (this->StepGrass.Sound && (tile->has_flag(tile_flag::grass) || tile->has_flag(tile_flag::stumps))) {
				return this->StepGrass.Sound;
			} else if (this->StepStone.Sound && tile->has_flag(tile_flag::stone_floor)) {
				return this->StepStone.Sound;
			} else {
				return this->Step.Sound;
			}
		}
		case unit_sound_type::used:
			return this->Used.Sound;
		case unit_sound_type::build:
			if (this->Build.Sound != nullptr) {
				return this->Build.Sound;
			}

			return this->get_sound_for_unit(unit_sound_type::acknowledging, unit);
		case unit_sound_type::ready:
			return this->Ready.Sound;
		case unit_sound_type::selected:
			return this->Selected.Sound;
		case unit_sound_type::help:
			return this->Help.Sound;
		case unit_sound_type::dying:
			if (this->Dead[unit->DamagedType].Sound != nullptr) {
				return this->Dead[unit->DamagedType].Sound;
			} else {
				return this->Dead[ANIMATIONS_DEATHTYPES].Sound;
			}
		case unit_sound_type::work_completed:
			if (CPlayer::GetThisPlayer()->get_civilization() != nullptr) {
				return CPlayer::GetThisPlayer()->get_civilization()->get_work_complete_sound();
			}
			break;
		case unit_sound_type::construction:
			return game_sound_set::get()->get_building_construction_sound();
		case unit_sound_type::docking:
			return game_sound_set::get()->get_docking_sound();
		case unit_sound_type::repairing:
			if (this->Repair.Sound != nullptr) {
				return this->Repair.Sound;
			}

			return this->get_sound_for_unit(unit_sound_type::acknowledging, unit);
		case unit_sound_type::harvesting:
			for (const std::unique_ptr<COrder> &order : unit->Orders) {
				if (order->Action == UnitAction::Resource) {
					const COrder_Resource *resource_order = dynamic_cast<const COrder_Resource *>(order.get());
					if (this->Harvest[resource_order->GetCurrentResource()].Sound != nullptr) {
						return this->Harvest[resource_order->GetCurrentResource()].Sound;
					}

					return this->get_sound_for_unit(unit_sound_type::acknowledging, unit);
				}
			}
			break;
		default:
			break;
	}

	return nullptr;
}

}
