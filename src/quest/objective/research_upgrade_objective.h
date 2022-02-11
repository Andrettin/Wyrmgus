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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "ai/ai_local.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "quest/quest.h"
#include "script/condition/condition.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_modifier.h"
#include "util/vector_util.h"

namespace wyrmgus {

class research_upgrade_objective final : public quest_objective
{
public:
	explicit research_upgrade_objective(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::research_upgrade;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "upgrade_class") {
			this->upgrade_class = upgrade_class::get(value);
		} else {
			quest_objective::process_gsml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->get_upgrade() == nullptr && this->upgrade_class == nullptr) {
			throw std::runtime_error("Research upgrade quest objective has neither an upgrade nor an upgrade class set for it.");
		}
	}

	const CUpgrade *get_player_upgrade(const CPlayer *player) const
	{
		if (this->get_upgrade() != nullptr) {
			return this->get_upgrade();
		} else if (this->upgrade_class != nullptr) {
			return player->get_class_upgrade(this->upgrade_class);
		}

		return nullptr;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		const CUpgrade *upgrade = this->get_player_upgrade(player);

		std::string objective_str;

		if (upgrade != nullptr) {
			objective_str = upgrade->get_research_verb_string() + " ";
			objective_str += upgrade->get_name();
		} else {
			objective_str = "Research ";
			objective_str += this->upgrade_class->get_name();
		}

		return objective_str;
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		const CUpgrade *upgrade = this->get_player_upgrade(player);

		if (upgrade == nullptr) {
			return false;
		}

		bool has_researcher = player->HasUpgradeResearcher(upgrade);

		if (!has_researcher) {
			//check if the quest includes an objective to build a researcher of the upgrade
			for (const auto &other_objective : this->get_quest()->get_objectives()) {
				if (other_objective.get() == this) {
					continue;
				}

				if (other_objective->get_objective_type() == objective_type::build_units) {
					std::vector<const unit_type *> unit_types = other_objective->get_unit_types();

					for (const unit_class *unit_class : other_objective->get_unit_classes()) {
						const unit_type *unit_type = player->get_faction()->get_class_unit_type(unit_class);
						if (unit_type == nullptr) {
							continue;
						}
						unit_types.push_back(unit_type);
					}

					if (unit_types.empty()) {
						continue;
					}

					for (const unit_type *unit_type : unit_types) {
						if (vector::contains(AiHelpers.get_researchers(upgrade), unit_type) || vector::contains(AiHelpers.get_researcher_classes(upgrade->get_upgrade_class()), unit_type->get_unit_class())) {
							//if the unit type of the other objective is a researcher of this upgrade
							has_researcher = true;
							break;
						}
					}

					if (has_researcher) {
						break;
					}
				}
			}
		}

		if (!has_researcher || player->Allow.Upgrades[upgrade->ID] != 'A' || !check_conditions(upgrade, player)) {
			return false;
		}

		//do not allow accepting the quest if it requires researching an upgrade which would remove the research objective of another quest
		for (const auto &objective : player->get_quest_objectives()) {
			const quest_objective *quest_objective = objective->get_quest_objective();

			if (quest_objective->get_objective_type() != objective_type::research_upgrade) {
				continue;
			}

			const research_upgrade_objective *research_quest_objective = static_cast<const research_upgrade_objective *>(quest_objective);

			const CUpgrade *other_upgrade = research_quest_objective->get_player_upgrade(player);

			if (other_upgrade == nullptr) {
				continue;
			}

			for (const auto &modifier : upgrade->get_modifiers()) {
				if (vector::contains(modifier->get_removed_upgrades(), other_upgrade)) {
					return false;
				}
			}

			for (const auto &modifier : other_upgrade->get_modifiers()) {
				if (vector::contains(modifier->get_removed_upgrades(), upgrade)) {
					return false;
				}
			}
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		const CUpgrade *upgrade = this->get_player_upgrade(player);

		if (upgrade == nullptr) {
			return std::make_pair(true, "You can no longer research the required upgrade.");
		}

		if (player->Allow.Upgrades[upgrade->ID] != 'R') {
			bool has_researcher = player->HasUpgradeResearcher(upgrade);

			if (!has_researcher) { //check if the quest includes an objective to build a researcher of the upgrade
				for (const auto &other_objective : player->get_quest_objectives()) {
					const quest_objective *other_quest_objective = other_objective->get_quest_objective();
					if (other_quest_objective->get_quest() != this->get_quest() || other_quest_objective == this || other_objective->get_counter() >= other_quest_objective->get_quantity()) { //if the objective has been fulfilled, then there should be a researcher, if there isn't it is due to i.e. the researcher having been destroyed later on, or upgraded to another type, and then the quest should fail if the upgrade can no longer be researched
						continue;
					}

					if (other_quest_objective->get_objective_type() == objective_type::build_units) {
						std::vector<const unit_type *> unit_types = other_quest_objective->get_unit_types();

						for (const unit_class *unit_class : other_quest_objective->get_unit_classes()) {
							const unit_type *unit_type = player->get_faction()->get_class_unit_type(unit_class);
							if (unit_type == nullptr) {
								continue;
							}
							unit_types.push_back(unit_type);
						}

						if (unit_types.empty()) {
							continue;
						}

						for (const unit_type *unit_type : unit_types) {
							if (vector::contains(AiHelpers.get_researchers(upgrade), unit_type) || vector::contains(AiHelpers.get_researcher_classes(upgrade->get_upgrade_class()), unit_type->get_unit_class())) { //if the unit type of the other objective is a researcher of this upgrade
								has_researcher = true;
								break;
							}
						}

						if (has_researcher) {
							break;
						}
					}
				}
			}

			if (!has_researcher || player->Allow.Upgrades[upgrade->ID] != 'A' || !check_conditions(upgrade, player)) {
				return std::make_pair(true, "You can no longer research the required upgrade.");
			}
		}

		return quest_objective::check_failure(player);
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const override
	{
		const CUpgrade *upgrade = this->get_player_upgrade(player_quest_objective->get_player());

		if (upgrade == nullptr) {
			player_quest_objective->set_counter(0);
			return;
		}

		const int count = UpgradeIdAllowed(*player_quest_objective->get_player(), upgrade->ID) == 'R' ? 1 : 0;
		player_quest_objective->set_counter(count);
	}

	virtual void check_ai(PlayerAi *ai_player, const player_quest_objective *player_quest_objective) const override
	{
		const CUpgrade *upgrade = this->get_player_upgrade(player_quest_objective->get_player());

		if (upgrade == nullptr) {
			return;
		}

		if (!AiRequestedUpgradeAllowed(*ai_player->Player, upgrade)) {
			return;
		}

		if (AiHasUpgrade(*ai_player, upgrade, true)) {
			return;
		}

		ai_player->add_research_request(upgrade);
	}

private:
	wyrmgus::upgrade_class *upgrade_class = nullptr;
};

}
