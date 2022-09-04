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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "character.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "player/player.h"
#include "script/effect/effect.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_container.h"
#include "upgrade/upgrade_structs.h"
#include "util/string_util.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

class create_unit_effect final : public effect<CPlayer>
{
public:
	explicit create_unit_effect(const gsml_operator effect_operator)
		: effect(effect_operator)
	{
	}

	explicit create_unit_effect(const std::string &unit_type_identifier, const gsml_operator effect_operator)
		: create_unit_effect(effect_operator)
	{
		this->unit_type = unit_type::get(unit_type_identifier);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "create_unit";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_type") {
			this->unit_type = unit_type::get(value);
		} else if (key == "unit_class") {
			this->unit_class = unit_class::get(value);
		} else if (key == "character") {
			this->character = character::get(value);
		} else if (key == "site") {
			this->site = site::get(value);
		} else if (key == "map_template") {
			this->map_template = map_template::get(value);
		} else if (key == "ttl") {
			this->ttl = std::stoi(value);
		} else if (key == "use_current_unit_pos") {
			this->use_current_unit_pos = string::to_bool(value);
		} else if (key == "use_source_unit_pos") {
			this->use_source_unit_pos = string::to_bool(value);
		} else if (key == "count") {
			this->count = std::stoi(value);
		} else if (key == "level") {
			this->level = std::stoi(value);
		} else {
			effect::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();
		const std::vector<std::string> &values = scope.get_values();

		if (tag == "pos") {
			this->pos = scope.to_point();
		} else if (tag == "abilities") {
			for (const std::string &value : values) {
				const CUpgrade *ability = CUpgrade::get(value);
				++this->ability_counts[ability];
			}
		} else if (tag == "bonus_abilities") {
			for (const std::string &value : values) {
				const CUpgrade *ability = CUpgrade::get(value);
				++this->bonus_ability_counts[ability];
			}
		} else {
			effect::process_gsml_scope(scope);
		}
	}

	virtual void check() const override
	{
		if (this->unit_type == nullptr && this->unit_class == nullptr && this->character == nullptr) {
			throw std::runtime_error("\"create_unit\" effect has neither a unit type, nor a unit class nor a character.");
		}

		if (this->site != nullptr && this->map_template != nullptr) {
			throw std::runtime_error("\"create_unit\" effect has both a site and a map template.");
		}

		if (this->map_template != nullptr && this->pos == QPoint(-1, -1)) {
			throw std::runtime_error("\"create_unit\" effect has a map template but no position.");
		}

		if (this->character != nullptr) {
			if (this->count > 1) {
				throw std::runtime_error("\"create_unit\" effect has a character and a count greater than 1 at the same time.");
			}

			assert_throw(this->ability_counts.empty());
			assert_throw(this->bonus_ability_counts.empty());
		}

		if (this->count < 1) {
			throw std::runtime_error("\"create_unit\" effect has a count smaller than 1.");
		}
	}

	virtual void do_assignment_effect(CPlayer *player, context &ctx) const override
	{
		const wyrmgus::unit_type *unit_type = this->get_unit_type();

		if (unit_type == nullptr) {
			if (this->unit_class != nullptr) {
				unit_type = player->get_class_unit_type(this->unit_class);

				if (unit_type == nullptr) {
					return;
				}
			}
		}

		const QPoint unit_top_left_pos = this->get_tile_pos(player, ctx) - unit_type->get_tile_center_pos_offset();
		const int map_layer = this->get_map_layer_index(player);

		for (int i = 0; i < this->count; ++i) {
			CUnit *unit = CreateUnit(unit_top_left_pos, *unit_type, player, map_layer);

			if (this->level != 0 && this->level > unit->Variable[LEVEL_INDEX].Value) {
				const int level_increase = this->level - unit->Variable[LEVEL_INDEX].Value;
				unit->IncreaseLevel(level_increase, false);
			}

			if (this->character != nullptr) {
				unit->set_character(this->character);
			}

			for (const auto &[ability, ability_count] : this->ability_counts) {
				for (int j = 0; j < ability_count; ++j) {
					AbilityAcquire(*unit, ability);

					if (unit->Variable[LEVELUP_INDEX].Value <= 0) {
						break;
					}
				}

				if (unit->Variable[LEVELUP_INDEX].Value <= 0) {
					break;
				}
			}

			for (const auto &[ability, ability_count] : this->bonus_ability_counts) {
				for (int j = 0; j < ability_count; ++j) {
					unit->add_bonus_ability(ability);
				}
			}

			if (this->ttl != 0) {
				unit->TTL = GameCycle + this->ttl;
			}
		}
	}

	virtual std::string get_assignment_string(const CPlayer *player, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(prefix);

		std::string str = "Receive ";
		if (this->character != nullptr) {
			str += "the " + string::highlight(this->character->get_full_name()) + " character";
		} else {
			std::string unit_name;
			if (this->unit_class != nullptr) {
				unit_name = this->unit_class->get_name();
			} else {
				unit_name = this->unit_type->get_name_for_player(player);
			}

			if (this->count > 1) {
				str += std::to_string(this->count);
			} else {
				str += string::get_indefinite_article(unit_name);
			}
			str += " ";

			if (this->level != 0) {
				str += "level " + std::to_string(this->level) + " ";
			}

			str += string::highlight(unit_name);

			str += " unit";
			if (this->count > 1) {
				str += "s";
			}
		}

		const wyrmgus::site *site = this->get_site(ctx);
		if (site != nullptr) {
			str += " at " + site->get_name();
		}

		if (this->ttl != 0) {
			str += " for " + std::to_string(this->ttl) + " cycles";
		}

		if (!this->ability_counts.empty()) {
			str += "\n" + std::string(indent + 1, '\t') + "Abilities:";

			for (const auto &[ability, ability_count] : this->ability_counts) {
				str += "\n" + std::string(indent + 2, '\t');
				str += ability->get_name();
				if (ability_count > 1) {
					str += " (x" + std::to_string(ability_count) + ")";
				}
			}
		}

		if (!this->bonus_ability_counts.empty()) {
			str += "\n" + std::string(indent + 1, '\t') + "Bonus Abilities:";

			for (const auto &[ability, ability_count] : this->bonus_ability_counts) {
				str += "\n" + std::string(indent + 2, '\t');
				str += ability->get_name();
				if (ability_count > 1) {
					str += " (x" + std::to_string(ability_count) + ")";
				}
			}
		}

		return str;
	}

	const wyrmgus::unit_type *get_unit_type() const
	{
		if (this->character != nullptr) {
			return this->character->get_unit_type();
		}

		return this->unit_type;
	}

	const wyrmgus::site *get_site(const read_only_context &ctx) const
	{
		if (this->site != nullptr) {
			return this->site;
		}

		if (this->use_current_unit_pos && ctx.current_unit != nullptr && ctx.current_unit->get()->get_site() != nullptr) {
			return ctx.current_unit->get()->get_site();
		}

		if (this->use_source_unit_pos && ctx.source_unit != nullptr && ctx.source_unit->get()->get_site() != nullptr) {
			return ctx.source_unit->get()->get_site();
		}

		return nullptr;
	}

	QPoint get_tile_pos(const CPlayer *player, const context &ctx) const
	{
		if (this->site != nullptr) {
			const site_game_data *site_game_data = this->site->get_game_data();

			if (site_game_data->get_site_unit() != nullptr) {
				return site_game_data->get_site_unit()->get_center_tile_pos();
			} else {
				return site_game_data->get_map_pos();
			}
		}

		if (this->map_template != nullptr && CMap::get()->is_subtemplate_on_map(this->map_template)) {
			return this->map_template->pos_to_map_pos(this->pos);
		}

		if (this->use_current_unit_pos && ctx.current_unit != nullptr) {
			return ctx.current_unit->get()->get_center_tile_pos();
		}

		if (this->use_source_unit_pos && ctx.source_unit != nullptr) {
			return ctx.source_unit->get()->get_center_tile_pos();
		}

		return player->get_main_pos();
	}

	int get_map_layer_index(const CPlayer *player) const
	{
		if (this->site != nullptr) {
			const site_game_data *site_game_data = this->site->get_game_data();

			if (site_game_data->get_site_unit() != nullptr) {
				return site_game_data->get_site_unit()->MapLayer->ID;
			} else if (site_game_data->get_map_layer() != nullptr) {
				return site_game_data->get_map_layer()->ID;
			}
		}

		if (this->map_template != nullptr && CMap::get()->is_subtemplate_on_map(this->map_template)) {
			return CMap::get()->get_subtemplate_map_layer(this->map_template)->ID;
		}

		return player->get_main_map_layer_index();
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr; //the unit type to be created
	const wyrmgus::unit_class *unit_class = nullptr; //the unit class to be created
	wyrmgus::character *character = nullptr; //the character to be created
	const wyrmgus::site *site = nullptr; //the site where the unit type is to be located, if any
	const wyrmgus::map_template *map_template = nullptr; //the map template where the unit type is to be located, if any
	QPoint pos = QPoint(-1, -1); //the unit's position in the map template
	int ttl = 0; //the time to live (in cycles) of the created unit, if any; useful for revealers
	bool use_current_unit_pos = false;
	bool use_source_unit_pos = false;
	int count = 1;
	int level = 0;
	upgrade_map<int> ability_counts;
	upgrade_map<int> bonus_ability_counts;
};

}
