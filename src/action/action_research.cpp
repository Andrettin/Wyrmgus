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
/**@name action_research.cpp - The research action. */
//
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_research.h"

#include "ai.h"
#include "animation.h"
#include "iolib.h"
#include "map/map_layer.h"
#include "player/civilization.h"
#include "player/player.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "translate.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_structs.h"
#include "upgrade/upgrade.h"

/// How many resources the player gets back if canceling research
static constexpr int CancelResearchCostsFactor = 100;

std::unique_ptr<COrder> COrder::NewActionResearch(const CUpgrade &upgrade, CPlayer *player)
{
	auto order = std::make_unique<COrder_Research>();

	// FIXME: if you give quick an other order, the resources are lost!
	//Wyrmgus start
	order->Player = player;
//	unit.Player->SubCosts(upgrade.Costs);
	const resource_map<int> upgrade_costs = player->GetUpgradeCosts(&upgrade);
	player->subtract_costs(upgrade_costs);
	//Wyrmgus end

	order->SetUpgrade(upgrade);
	return order;
}

void COrder_Research::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-research\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	//Wyrmgus start
	file.printf("\"player\", %d,", this->Player->get_index());
	//Wyrmgus end
	if (this->Upgrade) {
		file.printf(" \"upgrade\", \"%s\"", this->Upgrade->get_identifier().c_str());
	}
	file.printf("}");
}

bool COrder_Research::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp(value, "upgrade")) {
		++j;
		this->Upgrade = CUpgrade::get(LuaToString(l, -1, j + 1));
	//Wyrmgus start
	} else if (!strcmp(value, "player")) {
		++j;
		this->Player = CPlayer::Players[LuaToNumber(l, -1, j + 1)].get();
	//Wyrmgus end
	} else {
		return false;
	}
	return true;
}

bool COrder_Research::IsValid() const
{
	return true;
}

PixelPos COrder_Research::Show(const CViewport &, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	Q_UNUSED(render_commands)

	return lastScreenPos;
}

void COrder_Research::UpdateUnitVariables(CUnit &unit) const
{
	//Wyrmgus start
//	unit.Variable[RESEARCH_INDEX].Value = unit.Player->UpgradeTimers.Upgrades[this->Upgrade->ID];
	unit.Variable[RESEARCH_INDEX].Value = this->Player->UpgradeTimers.Upgrades[this->Upgrade->ID];
	//Wyrmgus end
	unit.Variable[RESEARCH_INDEX].Max = this->Upgrade->get_time_cost();
}

/**
**  Research upgrade.
**
**  @return true when finished.
*/
void COrder_Research::Execute(CUnit &unit)
{
	const CUpgrade &upgrade = this->GetUpgrade();
	const wyrmgus::unit_type &type = *unit.Type;

	UnitShowAnimation(unit, unit.get_animation_set()->Research ? unit.get_animation_set()->Research.get() : unit.get_animation_set()->Still.get());
	if (unit.Wait) {
		unit.Wait--;
		return;
	}

#if 0
	if (unit.Anim.Unbreakable) {
		return;
	}
#endif

	//Wyrmgus start
//	CPlayer &player = *unit.Player;
	CPlayer &player = *this->Player;
//	player.UpgradeTimers.Upgrades[upgrade.ID] += std::max(1, player.SpeedResearch / CPlayer::base_speed_factor);
	player.UpgradeTimers.Upgrades[upgrade.ID] += std::max(1, (player.SpeedResearch + unit.Variable[TIMEEFFICIENCYBONUS_INDEX].Value + unit.Variable[RESEARCHSPEEDBONUS_INDEX].Value) / CPlayer::base_speed_factor);
	//Wyrmgus end
	if (player.UpgradeTimers.Upgrades[upgrade.ID] >= upgrade.get_time_cost()) {
		if (upgrade.get_name().empty()) {
			//Wyrmgus start
//			player.Notify(NotifyGreen, unit.tilePos, _("%s: research complete"), type.Name.c_str());
			player.Notify(NotifyGreen, unit.tilePos, unit.MapLayer->ID, _("%s: research complete"), type.GetDefaultName(&player).c_str());
			//Wyrmgus end
		} else {
			player.Notify(NotifyGreen, unit.tilePos, unit.MapLayer->ID, _("%s: research complete"), upgrade.get_name().c_str());
		}
		if (&player == CPlayer::GetThisPlayer()) {
			const wyrmgus::civilization *civilization = unit.Player->get_civilization();
			const wyrmgus::sound *sound = nullptr;
			if (civilization != nullptr) {
				sound = civilization->get_research_complete_sound();
			}

			if (sound != nullptr) {
				PlayGameSound(sound, MaxSampleVolume);
			}
		}
		if (player.AiEnabled) {
			AiResearchComplete(unit, &upgrade);
		}
		UpgradeAcquire(player, &upgrade);
		this->Finished = true;
		return;
	}

	unit.Wait = CYCLES_PER_SECOND / 6;
}

void COrder_Research::Cancel(CUnit &unit)
{
	Q_UNUSED(unit)

	const CUpgrade &upgrade = this->GetUpgrade();
	//Wyrmgus start
//	unit.Player->UpgradeTimers.Upgrades[upgrade.ID] = 0;

//	unit.Player->AddCostsFactor(upgrade.Costs, CancelResearchCostsFactor);
	this->Player->UpgradeTimers.Upgrades[upgrade.ID] = 0;

	this->Player->AddCostsFactor(upgrade.get_costs(), CancelResearchCostsFactor);
	//Wyrmgus end
}
