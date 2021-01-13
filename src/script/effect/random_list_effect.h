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
//      (c) Copyright 2020-2021 by Andrettin
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

#pragma once

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/factor_modifier.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

template <typename scope_type>
class random_list_entry
{
public:
	explicit random_list_entry(const sml_data &scope)
	{
		this->base_weight = std::stoi(scope.get_tag());
		this->effects = std::make_unique<effect_list<scope_type>>();

		scope.for_each_element([&](const sml_property &property) {
			this->effects->process_sml_property(property);
		}, [&](const sml_data &child_scope) {
			if (child_scope.get_tag() == "modifier") {
				auto modifier = std::make_unique<factor_modifier<scope_type>>();
				database::process_sml_data(modifier, child_scope);
				this->weight_modifiers.push_back(std::move(modifier));
			} else {
				this->effects->process_sml_scope(child_scope);
			}
		});
	}

	int get_weight(const scope_type *scope) const
	{
		int weight = this->base_weight;

		if (scope != nullptr) {
			for (const std::unique_ptr<factor_modifier<scope_type>> &modifier : this->weight_modifiers) {
				if (modifier->check_conditions(scope)) {
					weight *= modifier->get_factor();
					weight /= 100;
				}
			}
		}

		return weight;
	}

	void do_effects(scope_type *scope, const context &ctx) const
	{
		this->effects->do_effects(scope, ctx);
	}

	std::string get_effects_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		std::string effects_string = this->effects->get_effects_string(scope, ctx, indent, prefix);

		if (!effects_string.empty()) {
			return effects_string;
		}

		return std::string(indent, '\t') + no_effect_string;
	}

private:
	int base_weight = 0;
	std::vector<std::unique_ptr<factor_modifier<scope_type>>> weight_modifiers;
	std::unique_ptr<effect_list<scope_type>> effects;
};

template <typename scope_type>
class random_list_effect final : public effect<scope_type>
{
public:
	explicit random_list_effect(const sml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "random_list";
		return identifier;
	}

	virtual void process_sml_scope(const sml_data &scope) override
	{
		this->entries.emplace_back(scope);
	}

	virtual void do_assignment_effect(scope_type *scope, const context &ctx) const override
	{
		const std::vector<const random_list_entry<scope_type> *> weighted_entries = this->get_weighted_entries(scope);

		if (!weighted_entries.empty()) {
			const random_list_entry<scope_type> *chosen_entry = vector::get_random(weighted_entries);
			chosen_entry->do_effects(scope, ctx);
		}
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		int total_weight = 0;
		std::vector<std::pair<const random_list_entry<scope_type> *, int>> entry_weights;
		for (const random_list_entry<scope_type> &entry : this->entries) {
			const int weight = entry.get_weight(scope);
			if (weight > 0) {
				total_weight += weight;
				entry_weights.emplace_back(&entry, weight);
			}
		}

		if (total_weight == 0) {
			return std::string();
		} else if (entry_weights.size() == 1) {
			return (*entry_weights.begin()).first->get_effects_string(scope, ctx, indent, prefix);
		}

		std::string str = "One of these will occur:\n";

		bool first = true;
		for (const auto &entry_weight_pair : entry_weights) {
			const random_list_entry<scope_type> *entry = entry_weight_pair.first;
			const int weight = entry_weight_pair.second;

			if (first) {
				first = false;
			} else {
				str += "\n";
			}

			str += std::string(indent + 1, '\t');

			const int chance = weight * 100 / total_weight;
			const std::string effects_string = entry->get_effects_string(scope, ctx, indent + 2, prefix);
			str += std::to_string(chance) + "% chance of:\n" + effects_string;
		}

		return str;
	}

private:
	std::vector<const random_list_entry<scope_type> *> get_weighted_entries(const scope_type *scope) const
	{
		std::vector<const random_list_entry<scope_type> *> weighted_entries;

		for (const random_list_entry<scope_type> &entry : this->entries) {
			const int weight = entry.get_weight(scope);

			for (int i = 0; i < weight; ++i) {
				weighted_entries.push_back(&entry);
			}
		}

		return weighted_entries;
	}

private:
	std::vector<random_list_entry<scope_type>> entries;
};

}
