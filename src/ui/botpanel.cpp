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
/**@name botpanel.cpp - The bottom panel. */
//
//      (c) Copyright 1999-2022 by Lutz Sammer, Vladi Belperchinov-Shabanski,
//		Jimmy Salmon, cybermind and Andrettin
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

#include "ui/ui.h"

#include "actions.h"
//Wyrmgus start
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/resource.h"
#include "game/game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "guichan/key.h"
#include "guichan/sdl/sdlinput.h"
#include "item/unique_item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/tile.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "player/civilization.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/player.h"
#include "quest/achievement.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/trigger.h"
#include "sound/game_sound_set.h"
#include "sound/sound.h"
#include "sound/unit_sound_type.h"
#include "spell/spell.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/popup.h"
#include "ui/resource_icon.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_manager.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "util/assert_util.h"
#include "util/util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/video.h"

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define RSHIFT  0
#define GSHIFT  8
#define BSHIFT  16
#define ASHIFT  24
#define RMASK   0x000000ff
#define GMASK   0x0000ff00
#define BMASK   0x00ff0000
#define AMASK   0xff000000
#else
#define RSHIFT  24
#define GSHIFT  16
#define BSHIFT  8
#define ASHIFT  0
#define RMASK   0xff000000
#define GMASK   0x00ff0000
#define BMASK   0x0000ff00
#define AMASK   0x000000ff
#endif

/// Last drawn popup : used to speed up drawing
const wyrmgus::button *LastDrawnButtonPopup = nullptr;
/// for unit buttons sub-menus etc.
const wyrmgus::button_level *CurrentButtonLevel = nullptr;
/// Pointer to current buttons
std::vector<std::unique_ptr<button>> CurrentButtons;

void InitButtons()
{
	// Resolve the icon names.
	for (wyrmgus::button *button : wyrmgus::button::get_all()) {
		if (!button->Icon.Name.empty()) {
			button->Icon.Load();
		}
	}
	CurrentButtons.clear();
}

/*----------------------------------------------------------------------------
--  Buttons structures
----------------------------------------------------------------------------*/

/**
**  Cleanup buttons.
*/
void CleanButtons()
{
	CurrentButtonLevel = nullptr;
	LastDrawnButtonPopup = nullptr;
	CurrentButtons.clear();
}

/**
**  Return Status of button.
**
**  @param button  button to check status
**  @param UnderCursor  Current Button Under Cursor
**
**  @return status of button
**  @return Icon(Active | Selected | Clicked | AutoCast | Disabled).
**
**  @todo FIXME : add IconDisabled when needed.
**  @todo FIXME : Should show the rally action for training unit ? (NewOrder)
*/
static int GetButtonStatus(const wyrmgus::button &button, int UnderCursor)
{
	unsigned int res = 0;

	/* parallel drawing */
	if (Selected.empty()) {
		return res;
	}

	// cursor is on that button
	if (ButtonAreaUnderCursor == ButtonAreaButton && UnderCursor == button.get_pos() - 1) {
		res |= IconActive;
		if (MouseButtons & LeftButton) {
			// Overwrite IconActive.
			res = IconClicked;
		}
	}

	//Wyrmgus start
	res |= IconCommandButton;
	
	if (button.Action == ButtonCmd::ProduceResource) {
		size_t i;
		for (i = 0; i < Selected.size(); ++i) {
			if (Selected[i]->GivesResource != button.Value) {
				break;
			}
		}
		if (i == Selected.size()) {
			res |= IconSelected;
		}
	}
	//Wyrmgus end
	
	UnitAction action = UnitAction::None;
	switch (button.Action) {
		case ButtonCmd::Stop:
			action = UnitAction::Still;
			break;
		case ButtonCmd::StandGround:
			action = UnitAction::StandGround;
			break;
		case ButtonCmd::Attack:
			action = UnitAction::Attack;
			break;
		case ButtonCmd::AttackGround:
			action = UnitAction::AttackGround;
			break;
		case ButtonCmd::Patrol:
			action = UnitAction::Patrol;
			break;
		case ButtonCmd::Harvest:
		case ButtonCmd::Return:
			action = UnitAction::Resource;
			break;
		default:
			break;
	}
	// Simple case.
	if (action != UnitAction::None) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (Selected[i]->CurrentAction() != action) {
				return res;
			}
		}
		res |= IconSelected;
		return res;
	}
	// other cases : manage AutoCast and different possible action.
	size_t i;
	switch (button.Action) {
		case ButtonCmd::Move:
			for (i = 0; i < Selected.size(); ++i) {
				const UnitAction saction = Selected[i]->CurrentAction();
				if (saction != UnitAction::Move &&
					saction != UnitAction::Build &&
					saction != UnitAction::Follow &&
					//Wyrmgus start
					saction != UnitAction::PickUp &&
					//Wyrmgus end
					saction != UnitAction::Defend) {
					break;
				}
			}
			if (i == Selected.size()) {
				res |= IconSelected;
			}
			break;
		case ButtonCmd::SpellCast:
			// FIXME : and IconSelected ?

			// Autocast
			for (i = 0; i < Selected.size(); ++i) {
				if (!Selected[i]->is_autocast_spell(wyrmgus::spell::get_all()[button.Value])) {
					break;
				}
			}
			if (i == Selected.size()) {
				res |= IconAutoCast;
			}
			break;
		case ButtonCmd::Repair:
			for (i = 0; i < Selected.size(); ++i) {
				if (Selected[i]->CurrentAction() != UnitAction::Repair) {
					break;
				}
			}
			if (i == Selected.size()) {
				res |= IconSelected;
			}
			// Auto repair
			for (i = 0; i < Selected.size(); ++i) {
				if (Selected[i]->AutoRepair != 1) {
					break;
				}
			}
			if (i == Selected.size()) {
				res |= IconAutoCast;
			}
			break;
		// FIXME: must handle more actions
		case ButtonCmd::SellResource:
			if (std::find(CPlayer::GetThisPlayer()->AutosellResources.begin(), CPlayer::GetThisPlayer()->AutosellResources.end(), button.Value) != CPlayer::GetThisPlayer()->AutosellResources.end()) {
				res |= IconAutoCast;
			}
		default:
			break;
	}
	return res;
}

/**
**  Tell if we can show the popup content.
**  verify each sub condition for that.
**
**  @param condition   condition to verify.
**  @param unit        unit that certain condition can refer.
**
**  @return            0 if we can't show the content, else 1.
*/
static bool CanShowPopupContent(const PopupConditionPanel *condition,
								const wyrmgus::button &button,
								const wyrmgus::unit_type *type)
{
	if (!condition) {
		return true;
	}

	if (condition->HasHint && button.get_hint().empty()) {
		return false;
	}

	if (condition->HasDescription && button.Description.empty()) {
		return false;
	}

	if (condition->HasConditions && PrintConditions(button).empty()) {
		return false;
	}
	
	//Wyrmgus start
	if (condition->Class && type && type->get_unit_class() == nullptr && !(type->BoolFlag[ITEM_INDEX].value && type->get_item_class() != item_class::none)) {
		return false;
	}
	
	if (condition->item_usable != CONDITION_TRUE) {
		if ((condition->item_usable == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Unit && Selected.size() == 1 && type->BoolFlag[ITEM_INDEX].value && Selected[0]->CanUseItem(&unit_manager::get()->GetSlotUnit(button.Value)))) {
			return false;
		}
	}
	
	if (condition->item_equippable != CONDITION_TRUE) {
		if ((condition->item_equippable == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Unit && Selected.size() == 1 && type->BoolFlag[ITEM_INDEX].value && Selected[0]->can_equip_item_class(type->get_item_class()))) {
			return false;
		}
	}

	if (condition->unit_class != nullptr) {
		if (!type || condition->unit_class != type->get_unit_class()) {
			return false;
		}
	}

	if (condition->unit_domain != unit_domain::none) {
		if (!type || condition->unit_domain != type->get_domain()) {
			return false;
		}
	}

	if (condition->CanStore != -1) {
		if (!type || !type->can_store(resource::get_all()[condition->CanStore])) {
			return false;
		}
	}

	if (condition->ImproveIncome != -1) {
		const resource *resource = resource::get_all()[condition->ImproveIncome];
		if (!type || type->Stats[CPlayer::GetThisPlayer()->get_index()].get_improve_income(resource) <= resource->get_default_income()) {
			return false;
		}
	}

	if (condition->ImproveIncomes != CONDITION_TRUE) {
		bool improve_incomes = false;

		if (!type) {
			return false;
		}

		for (const auto &[resource, quantity] : type->Stats[CPlayer::GetThisPlayer()->get_index()].get_improve_incomes()) {
			if (resource->get_index() == TimeCost) {
				continue;
			}

			if (quantity > resource->get_default_income()) {
				improve_incomes = true;
				break;
			}
		}

		if ((condition->ImproveIncomes == CONDITION_ONLY) ^ improve_incomes) {
			return false;
		}
	}

	if (condition->Description && type && type->get_description().empty()) {
		return false;
	}
	
	if (condition->Quote && type && type->get_quote().empty() && !((button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) && wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).get_unique() != nullptr && !wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).get_unique()->get_quote().empty()) && !((button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) && wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Work != nullptr && !wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Work->get_quote().empty() && wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Elixir != nullptr && !wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Elixir->get_quote().empty())) {
		return false;
	}
	
	if (condition->Encyclopedia && type && type->get_description().empty() && type->get_background().empty() && type->get_quote().empty() && (!type->BoolFlag[ITEM_INDEX].value || type->get_item_class() == wyrmgus::item_class::none)) {
		return false;
	}
	
	if (condition->site_name != CONDITION_TRUE) {
		if ((condition->site_name == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Unit && unit_manager::get()->GetSlotUnit(button.Value).get_site() != nullptr && !unit_manager::get()->GetSlotUnit(button.Value).get_site()->is_settlement() && !unit_manager::get()->GetSlotUnit(button.Value).get_site()->can_use_name_for_site_unit())) {
			return false;
		}
	}

	if (condition->settlement_name != CONDITION_TRUE) {
		if ((condition->settlement_name == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Unit && unit_manager::get()->GetSlotUnit(button.Value).settlement != nullptr)) {
			return false;
		}
	}
	
	if (condition->CanActiveHarvest && !(button.Action == ButtonCmd::Unit && Selected.size() > 0 && Selected[0]->can_harvest(&unit_manager::get()->GetSlotUnit(button.Value), false))) {
		return false;
	}

	if (condition->FactionUpgrade != CONDITION_TRUE) {
		if ((condition->FactionUpgrade == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Faction)) {
			return false;
		}
	}
	
	if (condition->DynastyUpgrade != CONDITION_TRUE) {
		if ((condition->DynastyUpgrade == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Dynasty)) {
			return false;
		}
	}
	
	if (condition->FactionCoreSettlements != CONDITION_TRUE) {
		if ((condition->FactionCoreSettlements == CONDITION_ONLY) ^ (game::get()->get_current_campaign() != nullptr && button.Action == ButtonCmd::Faction && button.Value != -1 && CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(button.Value)->get_core_settlements().size() > 0)) {
			return false;
		}
	}
	
	const CUpgrade *upgrade = nullptr;
	if (button.Action == ButtonCmd::Research || button.Action == ButtonCmd::ResearchClass || button.Action == ButtonCmd::LearnAbility) {
		upgrade = button.get_value_upgrade(Selected[0]);
	} else if (button.Action == ButtonCmd::Faction && CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(button.Value)->get_upgrade() != nullptr) {
		upgrade = CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(button.Value)->get_upgrade();
	} else if (button.Action == ButtonCmd::Dynasty && CPlayer::GetThisPlayer()->get_faction()->get_dynasties().at(button.Value)->get_upgrade() != nullptr) {
		upgrade = CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[button.Value]->get_upgrade();
	}
	
	if (condition->UpgradeResearched != CONDITION_TRUE) {
		if ((condition->UpgradeResearched == CONDITION_ONLY) ^ ((((button.Action == ButtonCmd::Research || button.Action == ButtonCmd::ResearchClass || button.Action == ButtonCmd::Faction || button.Action == ButtonCmd::Dynasty) && UpgradeIdAllowed(*CPlayer::GetThisPlayer(), upgrade->ID) == 'R') || (button.Action == ButtonCmd::LearnAbility && Selected[0]->GetIndividualUpgrade(upgrade) >= upgrade->MaxLimit)))) {
			return false;
		}
	}
	
	if (condition->ResearchedUpgrade) {
		if (UpgradeIdAllowed(*CPlayer::GetThisPlayer(), condition->ResearchedUpgrade->ID) != 'R') {
			return false;
		}
	}
	
	if (condition->researched_upgrade_class != nullptr) {
		if (!CPlayer::GetThisPlayer()->has_upgrade_class(condition->researched_upgrade_class)) {
			return false;
		}
	}
	
	if (condition->Ability != CONDITION_TRUE) {
		if ((condition->Ability == CONDITION_ONLY) ^ (upgrade && upgrade->is_ability())) {
			return false;
		}
	}
	
	if (condition->LuxuryResource != CONDITION_TRUE) {
		if ((condition->LuxuryResource == CONDITION_ONLY) ^ (button.Action == ButtonCmd::ProduceResource && button.get_value_resource()->is_luxury())) {
			return false;
		}
	}
	
	if (condition->RequirementsString != CONDITION_TRUE) {
		bool has_requirements_string = !IsButtonUsable(*Selected[0], button) && Selected[0]->Player == CPlayer::GetThisPlayer();

		switch (button.Action) {
			case ButtonCmd::Train:
			case ButtonCmd::TrainClass:
			case ButtonCmd::Build:
			case ButtonCmd::BuildClass:
			case ButtonCmd::UpgradeTo:
			case ButtonCmd::UpgradeToClass:
			case ButtonCmd::Buy:
				has_requirements_string = has_requirements_string && type != nullptr && !type->RequirementsString.empty();
				break;
			case ButtonCmd::Research:
			case ButtonCmd::ResearchClass:
			case ButtonCmd::LearnAbility:
			case ButtonCmd::Dynasty:
				has_requirements_string = has_requirements_string && upgrade != nullptr && (!upgrade->get_requirements_string().empty() || upgrade->get_conditions() != nullptr);
				break;
			case ButtonCmd::Faction:
				has_requirements_string = has_requirements_string && button.Value != -1 && !CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(button.Value)->get_requirements_string().empty();
				break;
			default:
				has_requirements_string = false;
				break;
		}

		if ((condition->RequirementsString == CONDITION_ONLY) ^ has_requirements_string) {
			return false;
		}
	}
	
	if (condition->ExperienceRequirementsString != CONDITION_TRUE) {
		if ((condition->ExperienceRequirementsString == CONDITION_ONLY) ^ (button.Action == ButtonCmd::ExperienceUpgradeTo && !IsButtonUsable(*Selected[0], button) && type && !type->ExperienceRequirementsString.empty())) {
			return false;
		}
	}
	
	if (condition->BuildingRulesString != CONDITION_TRUE) {
		if ((condition->BuildingRulesString == CONDITION_ONLY) ^ ((button.Action == ButtonCmd::Build || button.Action == ButtonCmd::BuildClass) && type && !type->BuildingRulesString.empty())) {
			return false;
		}
	}
	//Wyrmgus end

	if (condition->Overlord != CONDITION_TRUE) {
		if ((condition->Overlord == CONDITION_ONLY) ^ (CPlayer::Players[button.Value]->get_overlord() != nullptr)) {
			return false;
		}
	}

	if (condition->TopOverlord != CONDITION_TRUE) {
		if ((condition->TopOverlord == CONDITION_ONLY) ^ (CPlayer::Players[button.Value]->get_overlord() != nullptr && CPlayer::Players[button.Value]->get_top_overlord() != CPlayer::Players[button.Value]->get_overlord())) {
			return false;
		}
	}

	if (button.Action == ButtonCmd::Tile) {
		if (condition->terrain_feature != CONDITION_TRUE) {
			if ((condition->terrain_feature == CONDITION_ONLY) ^ (CMap::get()->Field(button.Value, UI.CurrentMapLayer->ID)->get_terrain_feature() != nullptr)) {
				return false;
			}
		}

		if (condition->world != CONDITION_TRUE) {
			if ((condition->world == CONDITION_ONLY) ^ (CMap::get()->Field(button.Value, UI.CurrentMapLayer->ID)->get_world() != nullptr)) {
				return false;
			}
		}
	}

	if (condition->ButtonAction != ButtonCmd::None && button.Action != condition->ButtonAction) {
		return false;
	}

	if (condition->ButtonValue.empty() == false && button.ValueStr != condition->ButtonValue) {
		return false;
	}

	if (type && condition->BoolFlags && !type->CheckUserBoolFlags(condition->BoolFlags.get())) {
		return false;
	}

	//Wyrmgus start
//	if (condition->Variables && type) {
	if (condition->Variables && type && button.Action != ButtonCmd::Unit && button.Action != ButtonCmd::Buy) {
	//Wyrmgus end
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
				if ((condition->Variables[i] == CONDITION_ONLY) ^ type->Stats[CPlayer::GetThisPlayer()->get_index()].Variables[i].Enable) {
					return false;
				}
			}
		}
	//Wyrmgus start
	} else if (condition->Variables && (button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy)) {
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
//				if ((condition->Variables[i] == CONDITION_ONLY) ^ wyrmgus::unit_manager::get()->GetSlotUnit(button.Value).Variable[i].Enable) {
				CUnit &unit = wyrmgus::unit_manager::get()->GetSlotUnit(button.Value);
				if (unit.Type->BoolFlag[ITEM_INDEX].value && unit.Container != nullptr && unit.Container->HasInventory()) {
					if (i == BASICDAMAGE_INDEX) {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.Container->GetItemVariableChange(&unit, i) != 0 || unit.Container->GetItemVariableChange(&unit, PIERCINGDAMAGE_INDEX) != 0)) {
							return false;
						}
					} else {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.Container->GetItemVariableChange(&unit, i) != 0)) { //the former for some reason wasn't working with negative values
							return false;
						}
					}
				} else if (unit.Work || unit.Elixir) { //special case for literary works and elixirs that aren't in an inventory
					if (i == BASICDAMAGE_INDEX) {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.GetItemVariableChange(&unit, i) != 0 || unit.GetItemVariableChange(&unit, PIERCINGDAMAGE_INDEX) != 0)) {
							return false;
						}
					} else {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.GetItemVariableChange(&unit, i) != 0)) { //the former for some reason wasn't working with negative values
							return false;
						}
					}
				} else {
					if (i == BASICDAMAGE_INDEX) {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.Variable[i].Value != 0 || unit.Variable[PIERCINGDAMAGE_INDEX].Value != 0)) {
							return false;
						}
					} else {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.Variable[i].Value != 0)) { //the former for some reason wasn't working with negative values
							return false;
						}
					}
				}
			}
		}
	//Wyrmgus end
	}
	
	//Wyrmgus start
	if (button.Action == ButtonCmd::SpellCast) {
		if (condition->AutoCast != CONDITION_TRUE) {
			if ((condition->AutoCast == CONDITION_ONLY) ^ (wyrmgus::spell::get_all()[button.Value]->get_autocast_info() != nullptr)) {
				return false;
			}
		}
	}
		
	if (button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) {
		CUnit &unit = wyrmgus::unit_manager::get()->GetSlotUnit(button.Value);
		if (unit.Type->BoolFlag[ITEM_INDEX].value) {
			if (condition->Equipped != CONDITION_TRUE) {
				if ((condition->Equipped == CONDITION_ONLY) ^ (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->IsItemEquipped(&unit))) {
					return false;
				}
			}
			if (condition->Equippable != CONDITION_TRUE) {
				if ((condition->Equippable == CONDITION_ONLY) ^ (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->can_equip_item(&unit))) {
					return false;
				}
			}
			if (condition->Consumable != CONDITION_TRUE) {
				if ((condition->Consumable == CONDITION_ONLY) ^ wyrmgus::is_consumable_item_class(unit.Type->get_item_class())) {
					return false;
				}
			}
			if (condition->Spell != CONDITION_TRUE) {
				if ((condition->Spell == CONDITION_ONLY) ^ (unit.Spell != nullptr)) {
					return false;
				}
			}
			if (condition->Work != CONDITION_TRUE) {
				if ((condition->Work == CONDITION_ONLY) ^ (unit.Work != nullptr)) {
					return false;
				}
			}
			if (condition->ReadWork != CONDITION_TRUE) {
				if ((condition->ReadWork == CONDITION_ONLY) ^ (unit.Work != nullptr && (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->GetIndividualUpgrade(unit.Work)))) {
					return false;
				}
			}
			if (condition->Elixir != CONDITION_TRUE) {
				if ((condition->Elixir == CONDITION_ONLY) ^ (unit.Elixir != nullptr)) {
					return false;
				}
			}
			if (condition->ConsumedElixir != CONDITION_TRUE) {
				if ((condition->ConsumedElixir == CONDITION_ONLY) ^ (unit.Elixir != nullptr && (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->GetIndividualUpgrade(unit.Elixir)))) {
					return false;
				}
			}
			if (condition->CanUse != CONDITION_TRUE) {
				if ((condition->CanUse == CONDITION_ONLY) ^ (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->CanUseItem(&unit))) {
					return false;
				}
			}
			if (condition->Bound != CONDITION_TRUE) {
				if ((condition->Bound == CONDITION_ONLY) ^ unit.Bound) {
					return false;
				}
			}
			if (condition->Identified != CONDITION_TRUE) {
				if ((condition->Identified == CONDITION_ONLY) ^ unit.Identified) {
					return false;
				}
			}
			if (condition->Weapon != CONDITION_TRUE) {
				if ((condition->Weapon == CONDITION_ONLY) ^ (wyrmgus::get_item_class_slot(unit.Type->get_item_class()) == wyrmgus::item_slot::weapon)) {
					return false;
				}
			}
			if (condition->Shield != CONDITION_TRUE) {
				if ((condition->Shield == CONDITION_ONLY) ^ (wyrmgus::get_item_class_slot(unit.Type->get_item_class()) == wyrmgus::item_slot::shield)) {
					return false;
				}
			}
			if (condition->Boots != CONDITION_TRUE) {
				if ((condition->Boots == CONDITION_ONLY) ^ (wyrmgus::get_item_class_slot(unit.Type->get_item_class()) == wyrmgus::item_slot::boots)) {
					return false;
				}
			}
			if (condition->Arrows != CONDITION_TRUE) {
				if ((condition->Arrows == CONDITION_ONLY) ^ (wyrmgus::get_item_class_slot(unit.Type->get_item_class()) == wyrmgus::item_slot::arrows)) {
					return false;
				}
			}
			if (condition->item_class != wyrmgus::item_class::none) {
				if (condition->item_class != unit.Type->get_item_class()) {
					return false;
				}
			}
			if (condition->Regeneration != CONDITION_TRUE) {
				if (unit.Container != nullptr && unit.Container->HasInventory()) {
					if ((condition->Regeneration == CONDITION_ONLY) ^ (unit.Container->GetItemVariableChange(&unit, HITPOINTBONUS_INDEX, true) != 0)) {
						return false;
					}
				} else if (unit.Work || unit.Elixir) { //special case for literary works and elixirs that aren't in an inventory
					if ((condition->Regeneration == CONDITION_ONLY) ^ (unit.GetItemVariableChange(&unit, HP_INDEX, true) != 0 || unit.GetItemVariableChange(&unit, HITPOINTBONUS_INDEX, true) != 0)) {
						return false;
					}
				} else {
					if ((condition->Regeneration == CONDITION_ONLY) ^ (unit.Variable[HP_INDEX].Increase != 0 || unit.Variable[HITPOINTBONUS_INDEX].Increase != 0)) {
						return false;
					}
				}
			}
		}
		
		if (!(button.Action == ButtonCmd::Buy && unit.get_character()) && condition->Opponent != CONDITION_TRUE) {
			if ((condition->Opponent == CONDITION_ONLY) ^ CPlayer::GetThisPlayer()->is_enemy_of(unit)) {
				return false;
			}
		}
		if (!(button.Action == ButtonCmd::Buy && unit.get_character()) && condition->Neutral != CONDITION_TRUE) {
			if ((condition->Neutral == CONDITION_ONLY) ^ (!CPlayer::GetThisPlayer()->is_enemy_of(unit) && !CPlayer::GetThisPlayer()->is_allied_with(unit) && CPlayer::GetThisPlayer() != unit.Player && (unit.Container == nullptr || (!CPlayer::GetThisPlayer()->is_enemy_of(*unit.Container) && !CPlayer::GetThisPlayer()->is_allied_with(*unit.Container) && CPlayer::GetThisPlayer() != unit.Container->Player)))) {
				return false;
			}
		}
		
		if (condition->Affixed != CONDITION_TRUE) {
			if ((condition->Affixed == CONDITION_ONLY) ^ (unit.Prefix != nullptr || unit.Suffix != nullptr)) {
				return false;
			}
		}
		if (condition->Unique != CONDITION_TRUE) {
			if ((condition->Unique == CONDITION_ONLY) ^ (unit.get_unique() != nullptr || unit.get_character() != nullptr)) {
				return false;
			}
		}
		if (condition->UniqueSet != CONDITION_TRUE) {
			if ((condition->UniqueSet == CONDITION_ONLY) ^ (unit.get_unique() != nullptr && unit.get_unique()->get_set() != nullptr)) {
				return false;
			}
		}
	} else { // always return false for "Affixed" and "Unique" for buttons that aren't individual unit buttons
		if (condition->Affixed != CONDITION_TRUE) {
			if (condition->Affixed == CONDITION_ONLY) {
				return false;
			}
		}
		if (condition->Unique != CONDITION_TRUE) {
			if (condition->Unique == CONDITION_ONLY) {
				return false;
			}
		}
	}
	//Wyrmgus end
	
	return true;
}

static void GetPopupSize(const CPopup &popup, const wyrmgus::button &button,
						 int &popupWidth, int &popupHeight, int *Costs)
{
	int contentWidth = popup.MarginX;
	int contentHeight = 0;
	int maxContentWidth = 0;
	int maxContentHeight = 0;
	popupWidth = popup.MarginX;
	popupHeight = popup.MarginY;

	const wyrmgus::unit_type *unit_type = button.get_unit_type();

	for (const std::unique_ptr<CPopupContentType> &content : popup.Contents) {
		if (CanShowPopupContent(content->Condition.get(), button, unit_type)) {
			// Automatically write the calculated coordinates.
			content->pos.x = contentWidth + content->MarginX;
			content->pos.y = popupHeight + content->MarginY;

			contentWidth += std::max(content->minSize.x, 2 * content->MarginX + content->GetWidth(button, Costs));
			contentHeight = std::max(content->minSize.y, 2 * content->MarginY + content->GetHeight(button, Costs));
			maxContentHeight = std::max(contentHeight, maxContentHeight);
			if (content->Wrap) {
				popupWidth += std::max(0, contentWidth - maxContentWidth);
				popupHeight += maxContentHeight;
				maxContentWidth = std::max(maxContentWidth, contentWidth);
				contentWidth = popup.MarginX;
				maxContentHeight = 0;
			}
		}
	}

	popupWidth += popup.MarginX;
	popupHeight += popup.MarginY;
}

static struct PopupDrawCache {
	int popupWidth;
	int popupHeight;
} popupCache;

/**
**  Draw popup
*/
void DrawPopup(const wyrmgus::button &button, int x, int y, bool above, std::vector<std::function<void(renderer *)>> &render_commands)
{
	CPopup *popup = PopupByIdent(button.Popup);
	bool useCache = false;
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	if (!popup) {
		return;
	} else if (&button == LastDrawnButtonPopup) {
		useCache = true;
	} else {
		LastDrawnButtonPopup = &button;
	}

	int popupWidth, popupHeight;
	std::array<int, ManaResCost + 1> Costs{};
	Costs.fill(0);

	const unit_type *unit_type = button.get_unit_type();
	const CUpgrade *upgrade = button.get_value_upgrade();

	switch (button.Action) {
		case ButtonCmd::Research:
		case ButtonCmd::ResearchClass:
		case ButtonCmd::Dynasty: {
			const resource_map<int> upgrade_costs = CPlayer::GetThisPlayer()->GetUpgradeCosts(upgrade);
			for (const auto &[resource, cost] : upgrade_costs) {
				Costs[resource->get_index()] = cost;
			}
			break;
		}
		case ButtonCmd::SpellCast:
			for (const auto &[resource, cost] : spell::get_all()[button.Value]->get_costs()) {
				Costs[resource->get_index()] = cost;
			}
			Costs[ManaResCost] = spell::get_all()[button.Value]->get_mana_cost();
			break;
		case ButtonCmd::Build:
		case ButtonCmd::BuildClass:
		case ButtonCmd::Train:
		case ButtonCmd::TrainClass:
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::UpgradeToClass: {
			const resource_map<int> type_costs = CPlayer::GetThisPlayer()->GetUnitTypeCosts(unit_type, Selected[0]->Type->Stats[Selected[0]->Player->get_index()].get_unit_stock(unit_type) != 0);

			for (const auto &[resource, cost] : type_costs) {
				Costs[resource->get_index()] = cost;
			}

			Costs[FoodCost] = unit_type->Stats[CPlayer::GetThisPlayer()->get_index()].Variables[DEMAND_INDEX].Value;
			break;
		}
		case ButtonCmd::Buy:
			Costs[FoodCost] = unit_manager::get()->GetSlotUnit(button.Value).Variable[DEMAND_INDEX].Value;
			Costs[CopperCost] = unit_manager::get()->GetSlotUnit(button.Value).GetPrice();
			break;
		default:
			break;
	}

	if (useCache) {
		popupWidth = popupCache.popupWidth;
		popupHeight = popupCache.popupHeight;
	} else {
		GetPopupSize(*popup, button, popupWidth, popupHeight, Costs.data());
		popupWidth = std::max(popupWidth, popup->MinWidth);
		popupHeight = std::max(popupHeight, popup->MinHeight);
		popupCache.popupWidth = popupWidth;
		popupCache.popupHeight = popupHeight;
	}

	x = std::min<int>(x, Video.Width - 1 - popupWidth);
	x = std::clamp(x, 0, Video.Width - 1);
	if (above) {
		y = y - popupHeight - (10 * scale_factor).to_int();
	} else { //below
		y = y + (10 * scale_factor).to_int();
	}

	//ensure the popup is drawn below the top bar, so that it doesn't appear below it
	const int min_y = (16 * defines::get()->get_scale_factor()).to_int();

	y = std::clamp(y, min_y, Video.Height - 1);

	// Background
	Video.FillTransRectangle(popup->BackgroundColor, x, y, popupWidth, popupHeight, popup->BackgroundColor >> ASHIFT, render_commands);
	Video.DrawRectangle(popup->BorderColor, x, y, popupWidth, popupHeight, render_commands);

	// Contents
	for (const std::unique_ptr<CPopupContentType> &content : popup->Contents) {
		if (CanShowPopupContent(content->Condition.get(), button, unit_type)) {
			content->Draw(x + content->pos.x, y + content->pos.y, *popup, popupWidth, button, Costs.data(), render_commands);
		}
	}
}

//Wyrmgus start
/**
**  Draw popup
*/
void DrawGenericPopup(const std::string &popup_text, int x, int y, const font_color *text_color, const font_color *highlight_color, bool above, std::vector<std::function<void(renderer *)>> &render_commands)
{
	wyrmgus::font *font = wyrmgus::defines::get()->get_game_font();
	
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	int MaxWidth = std::max((512 * scale_factor).to_int(), Video.Width / 5);

	int i;
		
	//calculate content width
	int content_width = 0;
	std::string content_width_sub;
	i = 1;
	while (!(content_width_sub = GetLineFont(i++, popup_text, 0, font)).empty()) {
		int line_width = font->getWidth(content_width_sub);
		int cost_symbol_pos = content_width_sub.find("COST_", 0);
		if (cost_symbol_pos != std::string::npos) {
			const int res = std::stoi(content_width_sub.substr(cost_symbol_pos + 5, content_width_sub.find(" ", cost_symbol_pos) - (cost_symbol_pos + 5) + 1));
			line_width -= font->getWidth("COST_" + std::to_string(res));
			const wyrmgus::resource *resource = wyrmgus::resource::get_all()[res];
			line_width += resource->get_icon()->get_graphics()->Width;
		}
		content_width = std::max(content_width, line_width);
	}
	
	if (MaxWidth) {
		content_width = std::min(content_width, MaxWidth);
	}
	
	//calculate content height
	int content_height = 0;
	i = 1;
	while ((GetLineFont(i++, popup_text, MaxWidth, font)).length()) {
		content_height += font->Height() + (2 * scale_factor).to_int();
	}
	
	int popupWidth, popupHeight;

	int contentWidth = (MARGIN_X * scale_factor).to_int();
	int contentHeight = 0;
	int maxContentWidth = 0;
	int maxContentHeight = 0;
	popupWidth = (MARGIN_X * scale_factor).to_int();
	popupHeight = (MARGIN_Y * scale_factor).to_int();
	PixelPos pos(0, 0);
	
	bool wrap = true;

	// Automatically write the calculated coordinates.
	pos.x = contentWidth + (MARGIN_X * scale_factor).to_int();
	pos.y = popupHeight + (MARGIN_Y * scale_factor).to_int();

	contentWidth += std::max(0, 2 * (MARGIN_X * scale_factor).to_int() + content_width);
	contentHeight = std::max(0, 2 * (MARGIN_Y * scale_factor).to_int() + content_height);
	maxContentHeight = std::max(contentHeight, maxContentHeight);
	if (wrap) {
		popupWidth += std::max(0, contentWidth - maxContentWidth);
		popupHeight += maxContentHeight;
		maxContentWidth = std::max(maxContentWidth, contentWidth);
		contentWidth = (MARGIN_X * scale_factor).to_int();
		maxContentHeight = 0;
	}

	popupWidth += (MARGIN_X * scale_factor).to_int();
	popupHeight += (MARGIN_Y * scale_factor).to_int();
	
	popupWidth = std::min(popupWidth, MaxWidth);
	popupHeight = std::max(popupHeight, 0);

	x = std::min<int>(x, Video.Width - 1 - popupWidth);
	x = std::clamp(x, 0, Video.Width - 1);
	if (above) {
		y = y - popupHeight - (10 * scale_factor).to_int();
	} else { //below
		y = y + (10 * scale_factor).to_int();
	}

	//ensure the popup is drawn below the top bar, so that it doesn't appear below it
	const int min_y = (16 * defines::get()->get_scale_factor()).to_int();
	y = std::clamp(y, min_y, Video.Height - 1);

	// Background
	const IntColor BackgroundColor = CVideo::MapRGBA(28, 28, 28, 208);
	const IntColor BorderColor = CVideo::MapRGBA(93, 93, 93, 160);

	Video.FillTransRectangle(BackgroundColor, x, y, popupWidth, popupHeight, BackgroundColor >> ASHIFT, render_commands);
	Video.DrawRectangle(BorderColor, x, y, popupWidth, popupHeight, render_commands);

	if (text_color == nullptr) {
		text_color = wyrmgus::defines::get()->get_default_font_color();
	}
	if (highlight_color == nullptr) {
		highlight_color = wyrmgus::defines::get()->get_default_highlight_font_color();
	}
	
	// Contents
	x += pos.x;
	y += pos.y;
	CLabel label(font, text_color, highlight_color);
	std::string sub;
	i = 0;
	int y_off = y;
	unsigned int width = MaxWidth
						 ? std::min(MaxWidth, popupWidth - 2 * (MARGIN_X * scale_factor).to_int())
						 : 0;
	while ((sub = GetLineFont(++i, popup_text, width, font)).length()) {
		if (sub.find("LINE", 0) != std::string::npos) {
			Video.FillRectangle(BorderColor, x + ((-MARGIN_X + 1 - MARGIN_X) * scale_factor).to_int(),
								y_off, popupWidth - (2 * scale_factor).to_int(), 1, render_commands);
			sub = sub.substr(sub.find("LINE", 0) + 4, sub.length());
		}
		int cost_symbol_pos = sub.find("COST_", 0);
		if (cost_symbol_pos != std::string::npos) {
			int x_offset = 0;
			const int res = std::stoi(sub.substr(cost_symbol_pos + 5, sub.find(" ", cost_symbol_pos) - (cost_symbol_pos + 5) + 1));
			std::string sub_first = sub.substr(0, cost_symbol_pos);
			std::string sub_second = sub.substr(cost_symbol_pos + 5 + std::to_string(res).length(), sub.length() - cost_symbol_pos - (5 + std::to_string(res).length()));
			label.Draw(x, y_off, sub_first, render_commands);
			x_offset += font->getWidth(sub_first);
			const wyrmgus::resource *resource = wyrmgus::resource::get_all()[res];
			const wyrmgus::resource_icon *icon = resource->get_icon();
			const std::shared_ptr<CGraphic> &icon_graphics = icon->get_graphics();
			icon_graphics->DrawFrameClip(icon->get_frame(), x + x_offset, y + ((font->getHeight() - icon_graphics->Height) / 2), nullptr, render_commands);
			x_offset += icon_graphics->Width;
			label.Draw(x + x_offset, y_off, sub_second, render_commands);
		} else {
			label.Draw(x, y_off, sub, render_commands);
		}
		y_off += font->Height() + (2 * scale_factor).to_int();
	}
}
//Wyrmgus end

/**
**  Draw button panel.
**
**  Draw all action buttons.
*/
void CButtonPanel::Draw(std::vector<std::function<void(renderer *)>> &render_commands)
{
	//  Draw background
	if (UI.ButtonPanel.G) {
		UI.ButtonPanel.G->DrawSubClip(0, 0,
									  UI.ButtonPanel.G->Width, UI.ButtonPanel.G->Height,
									  UI.ButtonPanel.X, UI.ButtonPanel.Y, render_commands);
	}

	// No buttons
	if (CurrentButtons.empty()) {
		return;
	}
	const std::vector<std::unique_ptr<wyrmgus::button>> &buttons(CurrentButtons);

	assert_throw(!Selected.empty());
	std::string str;

	//  Draw all buttons.
	for (size_t i = 0; i < buttons.size(); ++i) {
		const std::unique_ptr<wyrmgus::button> &button = buttons[i];

		if (button->get_pos() == -1) {
			continue;
		}

		assert_throw(button->get_pos() == static_cast<int>(i + 1));

		//Wyrmgus start
		//for neutral units, don't draw buttons that aren't training buttons (in other words, only draw buttons which are usable by neutral buildings)
		if (
			!button->is_always_shown()
			&& Selected[0]->Player != CPlayer::GetThisPlayer()
			&& !CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])
			&& CPlayer::GetThisPlayer()->has_building_access(Selected[0], button->Action)
			&& !IsNeutralUsableButtonAction(button->Action)
		) {
			continue;
		}
		//Wyrmgus end
		bool gray = false;
		bool cooldownSpell = false;
		int maxCooldown = 0;
		for (size_t j = 0; j != Selected.size(); ++j) {
			if (!IsButtonAllowed(*Selected[j], *button)) {
				gray = true;
				break;
			} else if (button->Action == ButtonCmd::SpellCast
				&& (*Selected[j]).get_spell_cooldown_timer(wyrmgus::spell::get_all()[button->Value]) > 0) {
				assert_throw(spell::get_all()[button->Value]->get_cooldown() > 0);
				cooldownSpell = true;
				maxCooldown = std::max(maxCooldown, (*Selected[j]).get_spell_cooldown_timer(wyrmgus::spell::get_all()[button->Value]));
			}
		}
		//
		//  Tutorial show command key in icons
		//
		if (preferences::get()->is_show_hotkeys_enabled()) {
			const int key = button->get_key();
			if (key == gcn::Key::K_ESCAPE) {
				str = "Esc";
			} else if (key == gcn::Key::K_PAGE_UP) {
				str = "PgUp";
			} else if (key == gcn::Key::K_PAGE_DOWN) {
				str = "PgDwn";
			} else if (key == gcn::Key::K_DELETE) {
				str = "Del";
			} else {
				str = static_cast<char>(toupper(key));
			}
		}

		//
		// Draw main Icon.
		//
		const PixelPos pos(UI.ButtonPanel.Buttons[i].X, UI.ButtonPanel.Buttons[i].Y);

		//Wyrmgus start
		const wyrmgus::icon *button_icon = button->Icon.Icon;

		const wyrmgus::unit_type *button_unit_type = button->get_value_unit_type(Selected[0]);
		const CUpgrade *button_upgrade = button->get_value_upgrade(Selected[0]);
		QColor border_color;
			
		// if there is a single unit selected, show the icon of its weapon/shield/boots/arrows equipped for the appropriate buttons
		if (button->Icon.Name.empty() && button->Action == ButtonCmd::Attack && Selected[0]->Type->CanTransport() && Selected[0]->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && Selected[0]->BoardCount > 0 && Selected[0]->has_units_inside() && Selected[0]->get_units_inside().at(0)->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && Selected[0]->get_units_inside().at(0)->GetButtonIcon(button->Action) != nullptr) {
			button_icon = Selected[0]->get_units_inside().at(0)->GetButtonIcon(button->Action);
		} else if (button->Icon.Name.empty() && Selected[0]->GetButtonIcon(button->Action) != nullptr) {
			button_icon = Selected[0]->GetButtonIcon(button->Action);
		} else if (button->Action == ButtonCmd::ExperienceUpgradeTo && Selected[0]->GetVariation() && button_unit_type->get_variation(Selected[0]->GetVariation()->get_tags()) != nullptr && !button_unit_type->get_variation(Selected[0]->GetVariation()->get_tags())->Icon.Name.empty()) {
			button_icon = button_unit_type->get_variation(Selected[0]->GetVariation()->get_tags())->Icon.Icon;
		} else if ((button->Action == ButtonCmd::Train || button->Action == ButtonCmd::TrainClass || button->Action == ButtonCmd::Build || button->Action == ButtonCmd::BuildClass || button->Action == ButtonCmd::UpgradeTo || button->Action == ButtonCmd::UpgradeToClass || button->Action == ButtonCmd::ExperienceUpgradeTo) && button->Icon.Name.empty() && button_unit_type->GetDefaultVariation(CPlayer::GetThisPlayer()) != nullptr && !button_unit_type->GetDefaultVariation(CPlayer::GetThisPlayer())->Icon.Name.empty()) {
			button_icon = button_unit_type->GetDefaultVariation(CPlayer::GetThisPlayer())->Icon.Icon;
		} else if ((button->Action == ButtonCmd::Train || button->Action == ButtonCmd::TrainClass || button->Action == ButtonCmd::Build || button->Action == ButtonCmd::BuildClass || button->Action == ButtonCmd::UpgradeTo || button->Action == ButtonCmd::UpgradeToClass || button->Action == ButtonCmd::ExperienceUpgradeTo) && button->Icon.Name.empty() && button_unit_type->get_icon() != nullptr) {
			button_icon = button_unit_type->get_icon();
		} else if (button->Action == ButtonCmd::Buy) {
			const CUnit &sold_unit = unit_manager::get()->GetSlotUnit(button->Value);
			button_icon = sold_unit.get_icon();

			if (sold_unit.get_unique() != nullptr) {
				border_color = defines::get()->get_unique_item_border_color();
			} else if (sold_unit.Prefix != nullptr || sold_unit.Suffix != nullptr) {
				border_color = defines::get()->get_magic_item_border_color();
			}
		} else if ((button->Action == ButtonCmd::Research || button->Action == ButtonCmd::ResearchClass) && button->Icon.Name.empty() && button_upgrade->get_icon()) {
			button_icon = button_upgrade->get_icon();
		} else if (button->Action == ButtonCmd::Faction && button->Icon.Name.empty() && CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(button->Value)->get_icon() != nullptr) {
			button_icon = CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(button->Value)->get_icon();
		} else if (button->Action == ButtonCmd::Dynasty && button->Icon.Name.empty() && CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[button->Value]->get_icon() != nullptr) {
			button_icon = CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[button->Value]->get_icon();
		}
		//Wyrmgus end
		
		if (cooldownSpell) {
			//Wyrmgus start
//			button->Icon.Icon->DrawCooldownSpellIcon(pos,
			button_icon->DrawCooldownSpellIcon(pos,
			//Wyrmgus end
				maxCooldown * 100 / wyrmgus::spell::get_all()[button->Value]->get_cooldown(), render_commands);
		} else if (gray) {
			//Wyrmgus start
//			button->Icon.Icon->DrawGrayscaleIcon(pos);
//			button_icon->DrawGrayscaleIcon(pos); //better to not show it
			//Wyrmgus end
		} else {
			const wyrmgus::player_color *player_color = nullptr;
			if (Selected.empty() == false && Selected[0]->IsAlive()) {
				player_color = Selected[0]->get_player_color();

				//Wyrmgus start
				//if is accessing a building of another player, set color to that of the person player (i.e. for training buttons)
				if (CPlayer::GetThisPlayer()->has_building_access(Selected[0]->Player, button->Action)) {
					player_color = CPlayer::GetThisPlayer()->get_player_color();
				}
				//Wyrmgus end
			}
			
			if (IsButtonUsable(*Selected[0], *button)) {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(*button, ButtonUnderCursor),
												   pos, str, player_color, border_color, false, false, 100 - GetButtonCooldownPercent(*Selected[0], *button), render_commands);
												   
				if (
					((button->Action == ButtonCmd::Train || button->Action == ButtonCmd::TrainClass) && Selected[0]->Type->Stats[Selected[0]->Player->get_index()].get_unit_stock(button_unit_type) != 0)
					|| button->Action == ButtonCmd::SellResource || button->Action == ButtonCmd::BuyResource
				) {
					std::string number_string;
					if ((button->Action == ButtonCmd::Train || button->Action == ButtonCmd::TrainClass) && Selected[0]->Type->Stats[Selected[0]->Player->get_index()].get_unit_stock(button_unit_type) != 0) { //draw the quantity in stock for unit "training" cases which have it
						number_string = std::to_string(Selected[0]->GetUnitStock(button_unit_type)) + "/" + std::to_string(Selected[0]->Type->Stats[Selected[0]->Player->get_index()].get_unit_stock(button_unit_type));
					} else if (button->Action == ButtonCmd::SellResource) {
						number_string = std::to_string(Selected[0]->Player->get_effective_resource_sell_price(button->get_value_resource()));
					} else if (button->Action == ButtonCmd::BuyResource) {
						number_string = std::to_string(Selected[0]->Player->get_effective_resource_buy_price(button->get_value_resource()));
					}
					CLabel label(wyrmgus::defines::get()->get_game_font());

					label.Draw(pos.x + 46 - wyrmgus::defines::get()->get_game_font()->Width(number_string), pos.y + 0, number_string, render_commands);
				}
			} else if ( //draw researched technologies (or acquired abilities) grayed
				((button->Action == ButtonCmd::Research || button->Action == ButtonCmd::ResearchClass || button->Action == ButtonCmd::Dynasty) && UpgradeIdAllowed(*CPlayer::GetThisPlayer(), button_upgrade->ID) == 'R')
				|| (button->Action == ButtonCmd::LearnAbility && Selected[0]->GetIndividualUpgrade(CUpgrade::get(button->ValueStr)) == CUpgrade::get(button->ValueStr)->MaxLimit)
			) {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(*button, ButtonUnderCursor),
												   pos, str, player_color, border_color, false, true, 100, render_commands);
			} else {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(*button, ButtonUnderCursor),
												   pos, str, player_color, border_color, true, false, 100, render_commands);
			}
		}
	}
	
	//Wyrmgus start
	if (ButtonAreaUnderCursor == ButtonAreaTransporting) {
		size_t i = 0;

		for (const CUnit *uins : Selected[0]->get_units_inside()) {
			if (!uins->Boarded || i >= UI.TransportingButtons.size() || (Selected[0]->Player != CPlayer::GetThisPlayer() && uins->Player != CPlayer::GetThisPlayer())) {
				continue;
			}
			if (static_cast<size_t>(ButtonUnderCursor) == i) {
				const wyrmgus::font_color *text_color = nullptr;
				if (uins->get_unique() != nullptr || uins->get_character() != nullptr) {
					text_color = wyrmgus::defines::get()->get_unique_font_color();
				} else if (uins->Prefix != nullptr || uins->Suffix != nullptr) {
					text_color = wyrmgus::defines::get()->get_magic_font_color();
				}
				DrawGenericPopup(uins->GetMessageName(), UI.TransportingButtons[i].X, UI.TransportingButtons[i].Y, text_color, nullptr, render_commands);
			}

			++i;
		}
	}
	//Wyrmgus end
}

/**
**  Check if the button is allowed for the unit.
**
**  @param unit          unit which checks for allow.
**  @param buttonaction  button to check if it is allowed.
**
**  @return 1 if button is allowed, 0 else.
**
**  @todo FIXME: better check. (dependency, resource, ...)
**  @todo FIXME: make difference with impossible and not yet researched.
*/
bool IsButtonAllowed(const CUnit &unit, const wyrmgus::button &buttonaction)
{
	if (buttonaction.is_always_shown()) {
		return true;
	}
	
	bool res = false;
	if (buttonaction.Allowed) {
		res = buttonaction.Allowed(unit, buttonaction);
		if (!res) {
			return false;
		} else {
			res = false;
		}
	}

	if (buttonaction.get_preconditions() != nullptr && !buttonaction.get_preconditions()->check(&unit, read_only_context::from_scope(&unit))) {
		return false;
	}
	
	//Wyrmgus start
	if (!CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) && (!CPlayer::GetThisPlayer()->has_building_access(Selected[0], buttonaction.Action) || !IsNeutralUsableButtonAction(buttonaction.Action))) {
		return false;
	}
	//Wyrmgus end

	const wyrmgus::unit_type *unit_type = buttonaction.get_value_unit_type(&unit);
	const CUpgrade *upgrade = buttonaction.get_value_upgrade(&unit);

	// Check button-specific cases
	switch (buttonaction.Action) {
		case ButtonCmd::Stop:
		case ButtonCmd::StandGround:
		case ButtonCmd::Button:
		case ButtonCmd::Move:
		case ButtonCmd::CallbackAction:
		//Wyrmgus start
		case ButtonCmd::RallyPoint:
		case ButtonCmd::Unit:
		case ButtonCmd::EditorUnit:
		case ButtonCmd::ProduceResource:
		case ButtonCmd::SellResource:
		case ButtonCmd::BuyResource:
		case ButtonCmd::Salvage:
		case ButtonCmd::EnterMapLayer:
		//Wyrmgus end
			res = true;
			break;
		case ButtonCmd::Repair:
			res = unit.can_repair();
			break;
		case ButtonCmd::Patrol:
			res = unit.CanMove();
			break;
		case ButtonCmd::Harvest:
			if (!unit.CurrentResource
				|| !(unit.ResourcesHeld > 0 && !unit.Type->get_resource_info(unit.get_current_resource())->LoseResources)
				|| (unit.ResourcesHeld != unit.Type->get_resource_info(unit.get_current_resource())->ResourceCapacity
					&& unit.Type->get_resource_info(unit.get_current_resource())->LoseResources)) {
				res = true;
			}
			//Wyrmgus start
			if (unit.BoardCount) {
				res = false;
			}
			//Wyrmgus end
			break;
		case ButtonCmd::Return:
			if (!(!unit.CurrentResource
				  || !(unit.ResourcesHeld > 0 && !unit.Type->get_resource_info(unit.get_current_resource())->LoseResources)
				  || (unit.ResourcesHeld != unit.Type->get_resource_info(unit.get_current_resource())->ResourceCapacity
					  && unit.Type->get_resource_info(unit.get_current_resource())->LoseResources))) {
				res = true;
			}
			break;
		case ButtonCmd::Attack:
			res = ButtonCheckAttack(unit, buttonaction);
			break;
		case ButtonCmd::AttackGround:
			if (unit.Type->BoolFlag[GROUNDATTACK_INDEX].value) {
				res = true;
			}
			break;
		case ButtonCmd::Train:
		case ButtonCmd::TrainClass:
			if (unit_type == nullptr) {
				break;
			}

			// Check if building queue is enabled
			if (!EnableTrainingQueue && unit.CurrentAction() == UnitAction::Train) {
				break;
			}
		// FALL THROUGH
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::UpgradeToClass:
		case ButtonCmd::Build:
		case ButtonCmd::BuildClass:
			if (unit_type == nullptr) {
				break;
			}

			res = check_conditions<true>(unit_type, unit.Player, false, !CPlayer::GetThisPlayer()->IsTeamed(unit));
			break;
		case ButtonCmd::Research:
		case ButtonCmd::ResearchClass:
			res = check_conditions<true>(upgrade, unit.Player, false, !CPlayer::GetThisPlayer()->IsTeamed(unit));
			if (res) {
				res = (UpgradeIdAllowed(*CPlayer::GetThisPlayer(), upgrade->ID) == 'A' || UpgradeIdAllowed(*CPlayer::GetThisPlayer(), upgrade->ID) == 'R') && check_conditions<true>(upgrade, CPlayer::GetThisPlayer(), false); //also check for the conditions for this player (rather than the unit) as an extra for researches, so that the player doesn't research too advanced technologies at neutral buildings
				res = res && (!unit.Player->UpgradeTimers.Upgrades[upgrade->ID] || unit.Player->UpgradeTimers.Upgrades[upgrade->ID] == upgrade->get_time_cost()); //don't show if is being researched elsewhere
			}
			break;
		case ButtonCmd::ExperienceUpgradeTo:
			res = check_conditions<true>(unit_type, &unit, true);
			if (res && unit.get_character() != nullptr) {
				res = !wyrmgus::vector::contains(unit.get_character()->ForbiddenUpgrades, unit_type);
			}
			break;
		case ButtonCmd::LearnAbility:
			res = unit.can_learn_ability<true>(upgrade);
			break;
		case ButtonCmd::SpellCast:
			res = wyrmgus::spell::get_all()[buttonaction.Value]->IsAvailableForUnit(unit);
			break;
		case ButtonCmd::Unload:
			res = (Selected[0]->Type->CanTransport() && Selected[0]->BoardCount);
			break;
		case ButtonCmd::Cancel:
			res = true;
			break;
		case ButtonCmd::CancelUpgrade:
			//Wyrmgus start
//			res = unit.CurrentAction() == UnitAction::UpgradeTo
//				  || unit.CurrentAction() == UnitAction::Research;
			//don't show the cancel button for a quick moment if the time cost is 0
			res = (unit.CurrentAction() == UnitAction::UpgradeTo && static_cast<COrder_UpgradeTo *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->get_index()].get_time_cost() > 0)
				|| (unit.CurrentAction() == UnitAction::Research && static_cast<COrder_Research *>(unit.CurrentOrder())->GetUpgrade().get_time_cost() > 0);
			//Wyrmgus end
			break;
		case ButtonCmd::CancelTrain:
			//Wyrmgus start
//			res = unit.CurrentAction() == UnitAction::Train;
			res = unit.CurrentAction() == UnitAction::Train && static_cast<COrder_Train *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->get_index()].get_time_cost() > 0; //don't show the cancel button for a quick moment if the time cost is 0
			//Wyrmgus end
			break;
		case ButtonCmd::CancelBuild:
			res = unit.CurrentAction() == UnitAction::Built;
			break;
		case ButtonCmd::Faction:
			res = CPlayer::GetThisPlayer()->get_faction() != nullptr && buttonaction.Value != -1 && buttonaction.Value < static_cast<int>(CPlayer::GetThisPlayer()->get_potentially_foundable_factions().size()) && CPlayer::GetThisPlayer()->can_found_faction<true>(CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(buttonaction.Value));
			break;
		case ButtonCmd::Dynasty:
			res = CPlayer::GetThisPlayer()->get_faction() != nullptr && buttonaction.Value != -1 && buttonaction.Value < static_cast<int>(CPlayer::GetThisPlayer()->get_faction()->get_dynasties().size()) && CPlayer::GetThisPlayer()->can_choose_dynasty<true>(CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[buttonaction.Value]) && CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[buttonaction.Value]->get_icon() != nullptr;
			if (res) {
				upgrade = CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[buttonaction.Value]->get_upgrade();
				res = (!unit.Player->UpgradeTimers.Upgrades[upgrade->ID] || unit.Player->UpgradeTimers.Upgrades[upgrade->ID] == upgrade->get_time_cost());
			}
			break;
		case ButtonCmd::Quest:
			res = buttonaction.Value < static_cast<int>(unit.Player->get_available_quests().size()) && unit.Player->get_current_quests().size() < CPlayer::max_current_quests && unit.Player->can_accept_quest(unit.Player->get_available_quests().at(buttonaction.Value));
			break;
		case ButtonCmd::Buy:
			res = (buttonaction.Value != -1) && (&wyrmgus::unit_manager::get()->GetSlotUnit(buttonaction.Value) != nullptr);
			if (res && wyrmgus::unit_manager::get()->GetSlotUnit(buttonaction.Value).get_character() != nullptr) {
				//check whether the character's conditions are still valid
				res = res && Selected[0]->Player->is_character_available_for_recruitment(wyrmgus::unit_manager::get()->GetSlotUnit(buttonaction.Value).get_character(), true);
			}
			break;
	}
#if 0
	// there is a additional check function -- call it
	if (res && buttonaction.Disabled) {
		return buttonaction.Disabled(unit, buttonaction);
	}
#endif
	return res;
}

//Wyrmgus start
/**
**	Check if the button is usable for the unit.
**
**  @param unit          unit which checks for allow.
**  @param buttonaction  button to check if it is usable.
**
**  @return 1 if button is usable, 0 else.
*/
bool IsButtonUsable(const CUnit &unit, const wyrmgus::button &buttonaction)
{
	if (!IsButtonAllowed(unit, buttonaction)) {
		return false;
	}
	
	if (buttonaction.get_conditions() != nullptr && !buttonaction.get_conditions()->check(&unit, read_only_context::from_scope(&unit))) {
		return false;
	}

	bool res = false;

	const unit_type *unit_type = buttonaction.get_value_unit_type(&unit);
	const CUpgrade *upgrade = buttonaction.get_value_upgrade(&unit);

	// Check button-specific cases
	switch (buttonaction.Action) {
		case ButtonCmd::Stop:
		case ButtonCmd::StandGround:
		case ButtonCmd::Button:
		case ButtonCmd::Move:
		case ButtonCmd::CallbackAction:
		case ButtonCmd::RallyPoint:
		case ButtonCmd::Unit:
		case ButtonCmd::EditorUnit:
		case ButtonCmd::Repair:
		case ButtonCmd::Patrol:
		case ButtonCmd::Harvest:
		case ButtonCmd::Return:
		case ButtonCmd::Attack:
		case ButtonCmd::AttackGround:
		case ButtonCmd::Salvage:
		case ButtonCmd::EnterMapLayer:
			res = true;
			break;
		case ButtonCmd::Train:
		case ButtonCmd::TrainClass:
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::UpgradeToClass:
		case ButtonCmd::Build:
		case ButtonCmd::BuildClass:
			res = check_conditions<false>(unit_type, unit.Player, false, !CPlayer::GetThisPlayer()->IsTeamed(unit));
			break;
		case ButtonCmd::Research:
		case ButtonCmd::ResearchClass:
			res = check_conditions<false>(upgrade, unit.Player, false, !CPlayer::GetThisPlayer()->IsTeamed(unit));
			if (res) {
				res = UpgradeIdAllowed(*CPlayer::GetThisPlayer(), upgrade->ID) == 'A' && check_conditions<false>(upgrade, CPlayer::GetThisPlayer(), false); //also check for the conditions of this player extra for researches, so that the player doesn't research too advanced technologies at neutral buildings
			}
			break;
		case ButtonCmd::ExperienceUpgradeTo:
			res = check_conditions<false>(unit_type, &unit, true) && unit.Variable[LEVELUP_INDEX].Value >= 1;
			break;
		case ButtonCmd::LearnAbility:
			res = unit.can_learn_ability(upgrade);
			break;
		case ButtonCmd::SpellCast:
			res = spell::get_all()[buttonaction.Value]->IsAvailableForUnit(unit);
			break;
		case ButtonCmd::Faction:
			res = CPlayer::GetThisPlayer()->can_found_faction(CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(buttonaction.Value));
			break;
		case ButtonCmd::Dynasty:
			res = CPlayer::GetThisPlayer()->can_choose_dynasty(CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[buttonaction.Value]);
			break;
		case ButtonCmd::Buy:
			res = true;
			if (unit_manager::get()->GetSlotUnit(buttonaction.Value).get_character() != nullptr) {
				res = CPlayer::GetThisPlayer()->Heroes.size() < CPlayer::max_heroes;
			}
			break;
		case ButtonCmd::Unload:
		case ButtonCmd::Cancel:
		case ButtonCmd::CancelUpgrade:
		case ButtonCmd::CancelTrain:
		case ButtonCmd::CancelBuild:
		case ButtonCmd::Quest:
		case ButtonCmd::ProduceResource:
		case ButtonCmd::SellResource:
		case ButtonCmd::BuyResource:
			res = true;
			break;
	}

	return res;
}

/**
**  Get the cooldown timer for the button (if any).
**
**  @param unit          unit which checks for allow.
**  @param buttonaction  button to check if it is usable.
**
**  @return the cooldown for using the button.
*/
int GetButtonCooldown(const CUnit &unit, const wyrmgus::button &buttonaction)
{
	int cooldown = 0;

	// Check button-specific cases
	switch (buttonaction.Action) {
		case ButtonCmd::Buy:
			if (buttonaction.Value != -1 && wyrmgus::unit_manager::get()->GetSlotUnit(buttonaction.Value).get_character() != nullptr) {
				cooldown = CPlayer::GetThisPlayer()->HeroCooldownTimer;
			}
			break;
	}

	return cooldown;
}

/**
**  Get the cooldown timer for the button, in percent.
**
**  @param unit          unit which checks for allow.
**  @param buttonaction  button to check if it is usable.
**
**  @return the cooldown for using the button.
*/
int GetButtonCooldownPercent(const CUnit &unit, const wyrmgus::button &buttonaction)
{
	int cooldown = 0;

	// Check button-specific cases
	switch (buttonaction.Action) {
		case ButtonCmd::Buy:
			if (buttonaction.Value != -1 && wyrmgus::unit_manager::get()->GetSlotUnit(buttonaction.Value).get_character() != nullptr) {
				cooldown = CPlayer::GetThisPlayer()->HeroCooldownTimer * 100 / HeroCooldownCycles;
			}
			break;
	}

	return cooldown;
}
//Wyrmgus end

/**
**  Update bottom panel for multiple units.
**
**  @return array of UI.ButtonPanel.NumButtons buttons to show.
**
**  @todo FIXME : make UpdateButtonPanelMultipleUnits more configurable.
**  @todo show all possible buttons or just same button...
*/
static void UpdateButtonPanelMultipleUnits(const std::vector<std::unique_ptr<button>> &buttonActions)
{
	std::array<char, 128> unit_ident{};
	//Wyrmgus start
	std::vector<std::array<char, 128>> individual_unit_ident;
	//Wyrmgus end

	sprintf(unit_ident.data(), ",%s-group,", CPlayer::GetThisPlayer()->get_civilization()->get_identifier().c_str());
	
	//Wyrmgus start
	for (size_t i = 0; i != Selected.size(); ++i) {
		std::array<char, 128> ident_array{};
		sprintf(ident_array.data(), ",%s,", Selected[i]->Type->Ident.c_str());
		individual_unit_ident.push_back(std::move(ident_array));
	}
	//Wyrmgus end

	for (const button *button : button::get_all()) {
		if (button->get_level() != CurrentButtonLevel) {
			continue;
		}

		//Wyrmgus start
		bool used_by_all = true;
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (!strstr(button->UnitMask.c_str(), individual_unit_ident[i].data()) && !vector::contains(button->get_unit_classes(), Selected[i]->Type->get_unit_class())) {
				used_by_all = false;
				break;
			}
		}
		//Wyrmgus end
		
		// any unit or unit in list
		if (button->UnitMask[0] != '*'
			&& !strstr(button->UnitMask.c_str(), unit_ident.data()) && !used_by_all) {
			continue;
		}

		bool allow = true;
		if (button->is_always_shown() == false) {
			for (size_t i = 0; i != Selected.size(); ++i) {
				if (!IsButtonAllowed(*Selected[i], *button)) {
					allow = false;
					break;
				}
			}
		}

		assert_throw(1 <= button->get_pos());
		assert_throw(button->get_pos() <= (int)UI.ButtonPanel.Buttons.size());

		// is button allowed after all?
		if (allow) {
			// OverWrite, So take last valid button.
			*buttonActions[button->get_pos() - 1] = *button;
		}
	}
}

/**
**  Update bottom panel for single unit.
**  or unit group with the same type.
**
**  @param unit  unit which has actions shown with buttons.
**
**  @return array of UI.ButtonPanel.NumButtons buttons to show.
**
**  @todo FIXME : Remove Hack for cancel button.
*/
static void UpdateButtonPanelSingleUnit(const CUnit &unit, const std::vector<std::unique_ptr<button>> &buttonActions)
{
	std::array<char, 128> unit_ident{};

	//
	//  FIXME: johns: some hacks for cancel buttons
	//
	bool only_cancel_allowed = true;
	if (unit.CurrentAction() == UnitAction::Built) {
		// Trick 17 to get the cancel-build button
		strcpy_s(unit_ident.data(), sizeof(unit_ident), ",cancel-build,");
	} else if (unit.CurrentAction() == UnitAction::UpgradeTo) {
		// Trick 17 to get the cancel-upgrade button
		strcpy_s(unit_ident.data(), sizeof(unit_ident), ",cancel-upgrade,");
	} else if (unit.CurrentAction() == UnitAction::Research) {
		if (CurrentButtonLevel != nullptr) {
			CurrentButtonLevel = nullptr;
		}
		// Trick 17 to get the cancel-upgrade button
		strcpy_s(unit_ident.data(), sizeof(unit_ident), ",cancel-upgrade,");
	} else {
		sprintf(unit_ident.data(), ",%s,", unit.Type->Ident.c_str());
		only_cancel_allowed = false;
	}
	for (const button *button : button::get_all()) {
		assert_throw(0 < button->get_pos() && button->get_pos() <= (int)UI.ButtonPanel.Buttons.size());

		// Same level
		if (button->get_level() != CurrentButtonLevel) {
			continue;
		}

		// any unit or unit in list
		if (button->UnitMask[0] != '*'
			&& !strstr(button->UnitMask.c_str(), unit_ident.data()) && (only_cancel_allowed || !vector::contains(button->get_unit_classes(), unit.Type->get_unit_class()))) {
			continue;
		}
		//Wyrmgus start
//		int allow = IsButtonAllowed(unit, buttonaction);
		bool allow = true; // check all selected units, as different units of the same type may have different allowed buttons
		if (button->is_always_shown() == false) {
			for (size_t j = 0; j != Selected.size(); ++j) {
				if (!IsButtonAllowed(*Selected[j], *button)) {
					allow = false;
					break;
				}
			}
		}
		//Wyrmgus end
		int pos = button->get_pos();

		// Special case for researches
		int researchCheck = true;
		if (button->is_always_shown() && !allow && (button->Action == ButtonCmd::Research || button->Action == ButtonCmd::ResearchClass || button->Action == ButtonCmd::Dynasty)) {
			const CUpgrade *upgrade = button->get_value_upgrade(Selected[0]);

			if (UpgradeIdAllowed(*CPlayer::GetThisPlayer(), upgrade->ID) == 'R') {
				researchCheck = false;
			}
		}
		
		// is button allowed after all?
		if ((button->is_always_shown() && buttonActions[pos - 1]->get_pos() == -1 && researchCheck) || allow) {
			// OverWrite, So take last valid button.
			*buttonActions[pos - 1] = *button;
		}
	}
}

/**
**  Update button panel.
**
**  @internal Affect CurrentButtons with buttons to show.
*/
void CButtonPanel::Update()
{
	//Wyrmgus start
//	if (Selected.empty()) {
	if (Selected.empty() || (!GameRunning && !GameEstablishing)) {
	//Wyrmgus end
		CurrentButtons.clear();
		return;
	}

	CUnit &unit = *Selected[0];
	// foreign unit
	//Wyrmgus start
//	if (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->IsTeamed(unit)) {
	if (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->IsTeamed(unit) && !CPlayer::GetThisPlayer()->has_building_access(&unit)) {
	//Wyrmgus end
		CurrentButtons.clear();
		return;
	}
	
	//Wyrmgus start
	//update the sold item buttons
	if (GameRunning || GameEstablishing) {
		unsigned int sold_unit_count = 0;
		unsigned int potential_faction_count = 0;
		unsigned int potential_dynasty_count = 0;

		for (button *button : button::get_all()) {
			if (button->Action != ButtonCmd::Faction && button->Action != ButtonCmd::Dynasty && button->Action != ButtonCmd::Buy) {
				continue;
			}

			std::array<char, 128> unit_ident{};
			sprintf(unit_ident.data(), ",%s,", unit.Type->Ident.c_str());
			if (button->UnitMask[0] != '*' && !strstr(button->UnitMask.c_str(), unit_ident.data()) && !vector::contains(button->get_unit_classes(), unit.Type->get_unit_class())) {
				continue;
			}

			if (button->Action == ButtonCmd::Faction) {
				if (CPlayer::GetThisPlayer()->get_faction() == nullptr || potential_faction_count >= CPlayer::GetThisPlayer()->get_potentially_foundable_factions().size()) {
					button->Value = -1;
				} else {
					const faction *faction = CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(potential_faction_count);
					button->Value = potential_faction_count;

					const government_type player_government_type = CPlayer::GetThisPlayer()->get_government_type();
					const government_type government_type = faction->is_government_type_valid(player_government_type) ? player_government_type : faction->get_default_government_type();
					const faction_tier faction_tier = faction->get_nearest_valid_tier(CPlayer::GetThisPlayer()->get_faction_tier());

					const std::string faction_name = faction->get_name(government_type, faction_tier);
					const bool uses_definite_article = faction->uses_definite_article(government_type);

					button->Hint = "Found ";
					if (uses_definite_article) {
						button->Hint += "the ";
					}
					button->Hint += faction_name;
					button->Description = "Changes your faction to ";
					if (uses_definite_article) {
						button->Description += "the ";
					}
					button->Description += faction_name;
				}
				potential_faction_count += 1;
			} else if (button->Action == ButtonCmd::Dynasty) {
				if (CPlayer::GetThisPlayer()->get_faction() == nullptr || potential_dynasty_count >= CPlayer::GetThisPlayer()->get_faction()->get_dynasties().size()) {
					button->Value = -1;
				} else {
					const dynasty *dynasty = CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[potential_dynasty_count];
					button->Value = potential_dynasty_count;
					button->Hint = "Choose the ";
					button->Hint += dynasty->get_name();
					button->Hint += " Dynasty";
					button->Description = "Changes your dynasty to the ";
					button->Description += dynasty->get_name();
					button->Description += " dynasty";
				}
				potential_dynasty_count += 1;
			} else if (button->Action == ButtonCmd::Buy) {
				if (sold_unit_count >= unit.SoldUnits.size()) {
					button->Value = -1;
				} else {
					button->Value = UnitNumber(*unit.SoldUnits[sold_unit_count]);
					if (unit.SoldUnits[sold_unit_count]->get_character() != nullptr) {
						button->Hint = "Recruit " + unit.SoldUnits[sold_unit_count]->get_full_name();
					} else {
						if (!unit.SoldUnits[sold_unit_count]->Name.empty()) {
							button->Hint = "Buy " + unit.SoldUnits[sold_unit_count]->get_full_name();
						} else {
							button->Hint = "Buy " + unit.SoldUnits[sold_unit_count]->get_type_name();
						}
					}
				}
				sold_unit_count += 1;
			}
		}
	}
	//Wyrmgus end

	bool sameType = true;
	// multiple selected
	for (size_t i = 1; i < Selected.size(); ++i) {
		if (Selected[i]->Type != unit.Type) {
			sameType = false;
			break;
		}
	}

	for (size_t i = CurrentButtons.size(); i != UI.ButtonPanel.Buttons.size(); ++i) {
		CurrentButtons.push_back(std::make_unique<button>());
	}

	for (const std::unique_ptr<button> &button : CurrentButtons) {
		button->pos = -1;
	}

	// We have selected different units types
	if (!sameType) {
		UpdateButtonPanelMultipleUnits(CurrentButtons);
	} else {
		// We have same type units selected
		// -- continue with setting buttons as for the first unit
		UpdateButtonPanelSingleUnit(unit, CurrentButtons);
	}
}

void CButtonPanel::DoClicked_SelectTarget(int button)
{
	// Select target.
	CurrentCursorState = CursorState::Select;
	cursor::set_current_cursor(UI.get_cursor(cursor_type::yellow_hair), true);
	CursorAction = CurrentButtons[button]->Action;
	CursorValue = CurrentButtons[button]->Value;
	CurrentButtonLevel = defines::get()->get_cancel_button_level(); // the cancel-only button level
	UI.ButtonPanel.Update();
	UI.StatusLine.Set(_("Select Target"));
}

void CButtonPanel::DoClicked_Unload(int button, const Qt::KeyboardModifiers key_modifiers)
{
	const int flush = !(key_modifiers & Qt::ShiftModifier);
	//
	//  Unload on coast, transporter standing, unload all units right now.
	//  That or a bunker.
	//  Unload on coast valid only for sea units
	//
	if ((Selected.size() == 1 && Selected[0]->CurrentAction() == UnitAction::Still
		 && Selected[0]->Type->get_domain() == unit_domain::water && Selected[0]->MapLayer->Field(Selected[0]->tilePos)->CoastOnMap())
		|| !Selected[0]->CanMove()) {
		SendCommandUnload(*Selected[0], Selected[0]->tilePos, NoUnitP, flush, Selected[0]->MapLayer->ID);
		return;
	}

	DoClicked_SelectTarget(button);
}

void CButtonPanel::DoClicked_SpellCast(int button, const Qt::KeyboardModifiers key_modifiers)
{
	const int spell_id = CurrentButtons[button]->Value;
	const wyrmgus::spell *spell = wyrmgus::spell::get_all()[spell_id];
	if (key_modifiers & Qt::ControlModifier) {
		if (spell->get_autocast_info() == nullptr) {
			PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_error_sound(), MaxSampleVolume);
			return;
		}

		bool autocast = false;

		// If any selected unit doesn't have autocast on turn it on
		// for everyone
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (!Selected[i]->is_autocast_spell(spell)) {
				autocast = true;
				break;
			}
		}
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (Selected[i]->is_autocast_spell(spell) != autocast) {
				SendCommandAutoSpellCast(*Selected[i], spell, autocast);
			}
		}
		return;
	}

	if (spell->is_caster_only()) {
		const int flush = !(key_modifiers & Qt::ShiftModifier);

		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit &unit = *Selected[i];
			// CursorValue here holds the spell type id
			SendCommandSpellCast(unit, unit.tilePos, &unit, spell_id, flush, unit.MapLayer->ID);
		}
		return;
	}
	DoClicked_SelectTarget(button);
}

void CButtonPanel::DoClicked_Repair(int button, const Qt::KeyboardModifiers key_modifiers)
{
	if (key_modifiers & Qt::ControlModifier) {
		unsigned autorepair = 0;
		// If any selected unit doesn't have autocast on turn it on
		// for everyone
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (Selected[i]->AutoRepair == 0) {
				autorepair = 1;
				break;
			}
		}
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (Selected[i]->AutoRepair != autorepair) {
				SendCommandAutoRepair(*Selected[i], autorepair);
			}
		}
		return;
	}
	DoClicked_SelectTarget(button);
}

void CButtonPanel::DoClicked_Return(const Qt::KeyboardModifiers key_modifiers)
{
	for (size_t i = 0; i != Selected.size(); ++i) {
		SendCommandReturnGoods(*Selected[i], NoUnitP, !(key_modifiers & Qt::ShiftModifier));
	}
}

void CButtonPanel::DoClicked_Stop()
{
	for (size_t i = 0; i != Selected.size(); ++i) {
		SendCommandStopUnit(*Selected[i]);
	}
}

void CButtonPanel::DoClicked_StandGround(const Qt::KeyboardModifiers key_modifiers)
{
	for (size_t i = 0; i != Selected.size(); ++i) {
		SendCommandStandGround(*Selected[i], !(key_modifiers & Qt::ShiftModifier));
	}
}

void CButtonPanel::DoClicked_Button(const int button)
{
	CurrentButtonLevel = button_level::try_get(CurrentButtons[button]->ValueStr);
	LastDrawnButtonPopup = nullptr;
	UI.ButtonPanel.Update();
}

void CButtonPanel::DoClicked_CancelUpgrade()
{
	if (Selected.size() == 1) {
		switch (Selected[0]->CurrentAction()) {
			case UnitAction::UpgradeTo:
				SendCommandCancelUpgradeTo(*Selected[0]);
				break;
			case UnitAction::Research:
				SendCommandCancelResearch(*Selected[0]);
				break;
			default:
				break;
		}
	}
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CurrentButtonLevel = nullptr;
	UI.ButtonPanel.Update();
	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
	CursorBuilding = nullptr;
	CurrentCursorState = CursorState::Point;
}

void CButtonPanel::DoClicked_CancelTrain()
{
	assert_throw(Selected[0]->CurrentAction() == UnitAction::Train);
	SendCommandCancelTraining(*Selected[0], -1, nullptr);
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
}

void CButtonPanel::DoClicked_CancelBuild()
{
	// FIXME: johns is this not sure, only building should have this?
	assert_throw(Selected[0]->CurrentAction() == UnitAction::Built);
	if (Selected.size() == 1) {
		SendCommandDismiss(*Selected[0]);
	}
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
}

void CButtonPanel::DoClicked_Build(const std::unique_ptr<wyrmgus::button> &button)
{
	// FIXME: store pointer in button table!
	const wyrmgus::unit_type *unit_type = button->get_value_unit_type(Selected[0]);

	if (!CPlayer::GetThisPlayer()->CheckUnitType(*unit_type)) {
		UI.StatusLine.Set(_("Select Location"));
		UI.StatusLine.ClearCosts();
		CursorBuilding = unit_type;
		// FIXME: check is this check necessary?
		CurrentButtonLevel = wyrmgus::defines::get()->get_cancel_button_level(); // the cancel-only button level
		UI.ButtonPanel.Update();
	}
}

void CButtonPanel::DoClicked_Train(const std::unique_ptr<wyrmgus::button> &button, const Qt::KeyboardModifiers key_modifiers)
{
	// FIXME: store pointer in button table!
	const wyrmgus::unit_type *unit_type = button->get_value_unit_type(Selected[0]);

	// FIXME: training queue full check is not correct for network.
	// FIXME: this can be correct written, with a little more code.

	int best_training_place = 0;
	int lowest_queue = Selected[0]->Orders.size();
	
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[i]->Type == Selected[0]->Type) {
			int selected_queue = 0;
			for (size_t j = 0; j < Selected[i]->Orders.size(); ++j) {
				if (Selected[i]->Orders[j]->Action == UnitAction::Train) {
					selected_queue += 1;
				}
			}
			if (selected_queue < lowest_queue) {
				lowest_queue = selected_queue;
				best_training_place = i;
			}
		}
	}
	
	int unit_count = 1;
	if (key_modifiers & Qt::ShiftModifier) {
		unit_count = 5;
	}
	
	for (int i = 0; i < unit_count; ++i) {
		if (Selected[best_training_place]->CurrentAction() == UnitAction::Train && !EnableTrainingQueue) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[best_training_place]->tilePos, Selected[best_training_place]->MapLayer->ID, "%s", _("Unit training queue is full"));
			return;
		} else if (CPlayer::GetThisPlayer()->CheckLimits(*unit_type) >= 0 && !CPlayer::GetThisPlayer()->CheckUnitType(*unit_type, Selected[best_training_place]->Type->Stats[Selected[best_training_place]->Player->get_index()].get_unit_stock(unit_type) != 0)) {
			SendCommandTrainUnit(*Selected[best_training_place], *unit_type, CPlayer::GetThisPlayer()->get_index(), FlushCommands);
			UI.StatusLine.Clear();
			UI.StatusLine.ClearCosts();
		} else if (CPlayer::GetThisPlayer()->CheckLimits(*unit_type) == -3) {
			if (CPlayer::GetThisPlayer()->get_civilization() != nullptr && CPlayer::GetThisPlayer()->get_civilization()->get_not_enough_food_sound() != nullptr) {
				PlayGameSound(CPlayer::GetThisPlayer()->get_civilization()->get_not_enough_food_sound(), MaxSampleVolume);
			}
			return;
		}
	}
	//Wyrmgus end
}

void CButtonPanel::DoClicked_UpgradeTo(const std::unique_ptr<wyrmgus::button> &button, const Qt::KeyboardModifiers key_modifiers)
{
	const wyrmgus::unit_type *unit_type = button->get_value_unit_type(Selected[0]);

	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[i]->Player->CheckLimits(*unit_type) != -6 && !Selected[i]->Player->CheckUnitType(*unit_type)) {
			if (Selected[i]->CurrentAction() != UnitAction::UpgradeTo) {
				SendCommandUpgradeTo(*Selected[i], *unit_type, !(key_modifiers & Qt::ShiftModifier));
				UI.StatusLine.Clear();
				UI.StatusLine.ClearCosts();
			}
		} else {
			break;
		}
	}
}

void CButtonPanel::DoClicked_ExperienceUpgradeTo(int button, const Qt::KeyboardModifiers key_modifiers)
{
	// FIXME: store pointer in button table!
	wyrmgus::unit_type &type = *wyrmgus::unit_type::get_all()[CurrentButtons[button]->Value];
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[0]->Player->GetUnitTotalCount(type) < Selected[0]->Player->Allow.Units[type.Slot] || Selected[0]->Player->CheckLimits(type) != -6) { //ugly way to make the checklimits message only appear when it should
			if (Selected[i]->CurrentAction() != UnitAction::UpgradeTo) {
				Selected[i]->Variable[LEVELUP_INDEX].Value -= 1;
				Selected[i]->Variable[LEVELUP_INDEX].Max = Selected[i]->Variable[LEVELUP_INDEX].Value;
				if (game::get()->is_persistency_enabled() && Selected[i]->get_character() != nullptr) {	//save the unit-type experience upgrade for persistent characters
					if (Selected[i]->get_character()->get_unit_type()->Slot != type.Slot) {
						if (Selected[i]->Player == CPlayer::GetThisPlayer()) {
							Selected[i]->get_character()->set_unit_type(wyrmgus::unit_type::get_all()[CurrentButtons[button]->Value]);
							Selected[i]->get_character()->save();
							achievement::check_achievements();
						}
					}
				}
				SendCommandTransformInto(*Selected[i], type, !(key_modifiers & Qt::ShiftModifier));
				UI.StatusLine.Clear();
				UI.StatusLine.ClearCosts();
				Selected[i]->Player->UpdateLevelUpUnits();
			}
		} else {
			break;
		}
	}
	
	LastDrawnButtonPopup = nullptr;
	UI.ButtonPanel.Update();
	
	if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Research(const std::unique_ptr<wyrmgus::button> &button, const Qt::KeyboardModifiers key_modifiers)
{
	const CUpgrade *button_upgrade = button->get_value_upgrade(Selected[0]);

	const resource_map<int> upgrade_costs = CPlayer::GetThisPlayer()->GetUpgradeCosts(button_upgrade);
	if (!CPlayer::GetThisPlayer()->CheckCosts(upgrade_costs)) {
		//PlayerSubCosts(player,Upgrades[i].Costs);
		SendCommandResearch(*Selected[0], *button_upgrade, CPlayer::GetThisPlayer()->get_index(), !(key_modifiers & Qt::ShiftModifier));
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
		LastDrawnButtonPopup = nullptr;
		ButtonUnderCursor = -1;
		OldButtonUnderCursor = -1;
		UI.ButtonPanel.Update();
	}
}

//Wyrmgus start
void CButtonPanel::DoClicked_LearnAbility(int button)
{
	const int index = CurrentButtons[button]->Value;
	SendCommandLearnAbility(*Selected[0], *CUpgrade::get_all()[index]);
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	LastDrawnButtonPopup = nullptr;
	
	if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Faction(int button)
{
	const int index = CurrentButtons[button]->Value;
	SendCommandSetFaction(CPlayer::GetThisPlayer(), CPlayer::GetThisPlayer()->get_potentially_foundable_factions().at(index));
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	LastDrawnButtonPopup = nullptr;
	if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Quest(int button)
{
	const int index = CurrentButtons[button]->Value;
	SendCommandQuest(*Selected[0], Selected[0]->Player->get_available_quests().at(index));
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	LastDrawnButtonPopup = nullptr;
	if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Buy(int button)
{
	resource_map<int> buy_costs;
	buy_costs[defines::get()->get_wealth_resource()] = unit_manager::get()->GetSlotUnit(CurrentButtons[button]->Value).GetPrice();

	if (!CPlayer::GetThisPlayer()->CheckCosts(buy_costs) && CPlayer::GetThisPlayer()->CheckLimits(*wyrmgus::unit_manager::get()->GetSlotUnit(CurrentButtons[button]->Value).Type) >= 0) {
		SendCommandBuy(*Selected[0], &wyrmgus::unit_manager::get()->GetSlotUnit(CurrentButtons[button]->Value), CPlayer::GetThisPlayer()->get_index());
		ButtonUnderCursor = -1;
		OldButtonUnderCursor = -1;
		LastDrawnButtonPopup = nullptr;
		if (IsOnlySelected(*Selected[0])) {
			SelectedUnitChanged();
		}
	} else if (CPlayer::GetThisPlayer()->CheckLimits(*wyrmgus::unit_manager::get()->GetSlotUnit(CurrentButtons[button]->Value).Type) == -3) {
		if (CPlayer::GetThisPlayer()->get_civilization() != nullptr && CPlayer::GetThisPlayer()->get_civilization()->get_not_enough_food_sound() != nullptr) {
			PlayGameSound(CPlayer::GetThisPlayer()->get_civilization()->get_not_enough_food_sound(), MaxSampleVolume);
		}
	}
}

void CButtonPanel::DoClicked_ProduceResource(int button)
{
	const int resource = CurrentButtons[button]->Value;
	if (resource != Selected[0]->GivesResource) {
		SendCommandProduceResource(*Selected[0], resource != 0 ? wyrmgus::resource::get_all()[resource] : nullptr);
	} else if (Selected[0]->Type->get_given_resource() == nullptr) {
		//if resource production button was clicked when it was already active, then this means it should be toggled off; only do this if the building's type doesn't have a default produced resource, though, since in those cases it should always produce a resource
		SendCommandProduceResource(*Selected[0], nullptr);
	}
}

void CButtonPanel::DoClicked_SellResource(int button, const Qt::KeyboardModifiers key_modifiers)
{
	const bool toggle_autosell = (key_modifiers & Qt::ControlModifier) != 0;
	const int resource = CurrentButtons[button]->Value;
	
	if (toggle_autosell && Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SendCommandAutosellResource(CPlayer::GetThisPlayer()->get_index(), resource);
	} else {
		resource_map<int> sell_resource_costs;
		sell_resource_costs[resource::get_all()[resource]] = 100;
		if (!CPlayer::GetThisPlayer()->CheckCosts(sell_resource_costs)) {
			SendCommandSellResource(*Selected[0], resource, CPlayer::GetThisPlayer()->get_index());
		}
	}
}

void CButtonPanel::DoClicked_BuyResource(int button)
{
	const resource *resource = CurrentButtons[button]->get_value_resource();
	resource_map<int> buy_resource_costs;
	buy_resource_costs[defines::get()->get_wealth_resource()] = Selected[0]->Player->get_effective_resource_buy_price(resource);
	if (!CPlayer::GetThisPlayer()->CheckCosts(buy_resource_costs)) {
		SendCommandBuyResource(*Selected[0], resource->get_index(), CPlayer::GetThisPlayer()->get_index());
	}
}

void CButtonPanel::DoClicked_Salvage()
{
	for (int i = (Selected.size()  - 1); i >= 0; --i) {
		SendCommandDismiss(*Selected[i], true);
	}
}

/**
**	@brief	Enter the map layer that the selected unit leads to.
*/
void CButtonPanel::DoClicked_EnterMapLayer()
{
	for (size_t i = 0; i < Selected.size(); ++i) {
		CUnit *connection_destination = Selected[i]->ConnectingDestination;
		if (connection_destination != nullptr) {
			PlayUnitSound(*connection_destination, wyrmgus::unit_sound_type::used);
			Selected[i]->Blink = 4;
			connection_destination->Blink = 4;
			UnSelectUnit(*Selected[i]);
			if (connection_destination->IsVisible(*CPlayer::GetThisPlayer())) {
				SelectUnit(*connection_destination);
			}
			SelectionChanged();
			ChangeCurrentMapLayer(connection_destination->MapLayer->ID);
			UI.SelectedViewport->Center(connection_destination->get_scaled_map_pixel_pos_center());
			break;
		}
	}
}

void CButtonPanel::DoClicked_CallbackAction(int button)
{
	LuaCallback* callback = (LuaCallback*)(CurrentButtons[button]->Payload);
	callback->pushPreamble();
	callback->pushInteger(UnitNumber(*Selected[0]));
	callback->run();
}

/**
**  Handle bottom button clicked.
**
**  @param button  Button that was clicked.
*/
void CButtonPanel::DoClicked(int button, const Qt::KeyboardModifiers key_modifiers)
{
	assert_throw(0 <= button && button < (int)UI.ButtonPanel.Buttons.size());
	// no buttons
	if (CurrentButtons.empty()) {
		return;
	}
	if (IsButtonAllowed(*Selected[0], *CurrentButtons[button]) == false) {
		return;
	}
	//
	//  Button not available.
	//  or Not Teamed
	//
	//Wyrmgus start
//	if (CurrentButtons[button]->get_pos() == -1 || !ThisPlayer->IsTeamed(*Selected[0])) {
	if (CurrentButtons[button]->get_pos() == -1 || (!CurrentButtons[button]->is_always_shown() && !CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) && !CPlayer::GetThisPlayer()->has_building_access(Selected[0], CurrentButtons[button]->Action)) || (!CurrentButtons[button]->is_always_shown() && !CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) && CPlayer::GetThisPlayer()->has_building_access(Selected[0], CurrentButtons[button]->Action) && !IsNeutralUsableButtonAction(CurrentButtons[button]->Action))) { //allow neutral units to be used (but only for training or as transporters)
	//Wyrmgus end
		return;
	}

	//Wyrmgus start
	if (!IsButtonUsable(*Selected[0], *CurrentButtons[button])) {
		const CUpgrade *button_upgrade = CurrentButtons[button]->get_value_upgrade(Selected[0]);

		if (
			((CurrentButtons[button]->Action == ButtonCmd::Research || CurrentButtons[button]->Action == ButtonCmd::ResearchClass || CurrentButtons[button]->Action == ButtonCmd::Dynasty) && UpgradeIdAllowed(*CPlayer::GetThisPlayer(), button_upgrade->ID) == 'R')
			|| (CurrentButtons[button]->Action == ButtonCmd::LearnAbility && Selected[0]->GetIndividualUpgrade(button_upgrade) == button_upgrade->MaxLimit)
		) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The upgrade has already been acquired"));
		} else if (CurrentButtons[button]->Action == ButtonCmd::Buy && CPlayer::GetThisPlayer()->Heroes.size() >= CPlayer::max_heroes && CurrentButtons[button]->Value != -1 && unit_manager::get()->GetSlotUnit(CurrentButtons[button]->Value).get_character() != nullptr) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The hero limit has been reached"));
		} else {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The requirements have not been fulfilled"));
		}
		PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_error_sound(), MaxSampleVolume);
		return;
	}

	if (GetButtonCooldown(*Selected[0], *CurrentButtons[button]) > 0) {
		CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The cooldown is active"));
		PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_error_sound(), MaxSampleVolume);
		return;
	}
	//Wyrmgus end

	PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
	if (CurrentButtons[button]->CommentSound.Sound) {
		PlayGameSound(CurrentButtons[button]->CommentSound.Sound, MaxSampleVolume);
	}
	
	//  Handle action on button.
	switch (CurrentButtons[button]->Action) {
		case ButtonCmd::Unload: { DoClicked_Unload(button, key_modifiers); break; }
		case ButtonCmd::SpellCast: { DoClicked_SpellCast(button, key_modifiers); break; }
		case ButtonCmd::Repair: { DoClicked_Repair(button, key_modifiers); break; }
		case ButtonCmd::Move:    // Follow Next
		case ButtonCmd::Patrol:  // Follow Next
		case ButtonCmd::Harvest: // Follow Next
		case ButtonCmd::Attack:  // Follow Next
		//Wyrmgus start
		case ButtonCmd::RallyPoint:
		case ButtonCmd::Unit:
		case ButtonCmd::EditorUnit:
		//Wyrmgus end
		case ButtonCmd::AttackGround: { DoClicked_SelectTarget(button); break; }
		case ButtonCmd::Return: { DoClicked_Return(key_modifiers); break; }
		case ButtonCmd::Stop: { DoClicked_Stop(); break; }
		case ButtonCmd::StandGround: { DoClicked_StandGround(key_modifiers); break; }
		case ButtonCmd::Button: { DoClicked_Button(button); break; }
		case ButtonCmd::Cancel: // Follow Next
		case ButtonCmd::CancelUpgrade: { DoClicked_CancelUpgrade(); break; }
		case ButtonCmd::CancelTrain: { DoClicked_CancelTrain(); break; }
		case ButtonCmd::CancelBuild: { DoClicked_CancelBuild(); break; }
		case ButtonCmd::Build:
		case ButtonCmd::BuildClass:
			DoClicked_Build(CurrentButtons[button]);
			break;
		case ButtonCmd::Train:
		case ButtonCmd::TrainClass:
			DoClicked_Train(CurrentButtons[button], key_modifiers);
			break;
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::UpgradeToClass:
			DoClicked_UpgradeTo(CurrentButtons[button], key_modifiers);
			break;
		case ButtonCmd::Research:
		case ButtonCmd::ResearchClass:
		case ButtonCmd::Dynasty:
			DoClicked_Research(CurrentButtons[button], key_modifiers);
			break;
		case ButtonCmd::CallbackAction: { DoClicked_CallbackAction(button); break; }
		//Wyrmgus start
		case ButtonCmd::LearnAbility: { DoClicked_LearnAbility(button); break; }
		case ButtonCmd::ExperienceUpgradeTo: { DoClicked_ExperienceUpgradeTo(button, key_modifiers); break; }
		case ButtonCmd::Faction: { DoClicked_Faction(button); break; }
		case ButtonCmd::Quest: { DoClicked_Quest(button); break; }
		case ButtonCmd::Buy: { DoClicked_Buy(button); break; }
		case ButtonCmd::ProduceResource: { DoClicked_ProduceResource(button); break; }
		case ButtonCmd::SellResource: { DoClicked_SellResource(button, key_modifiers); break; }
		case ButtonCmd::BuyResource: { DoClicked_BuyResource(button); break; }
		case ButtonCmd::Salvage: { DoClicked_Salvage(); break; }
		case ButtonCmd::EnterMapLayer: { DoClicked_EnterMapLayer(); break; }
		//Wyrmgus end
	}
}

/**
**  Lookup key for bottom panel buttons.
**
**  @param key  Internal key symbol for pressed key.
**
**  @return     True, if button is handled (consumed).
*/
int CButtonPanel::DoKey(int key, const Qt::KeyboardModifiers key_modifiers)
{
	SDL_Keysym keysym;
	memset(&keysym, 0, sizeof(keysym));
	keysym.sym = (SDL_Keycode) key;
	gcn::Key k = gcn::SDLInput::convertKeyCharacter(keysym);
	key = k.getValue();

	if (!CurrentButtons.empty()) {
		// This is required for action queues SHIFT+M should be `m'
		if (isascii(key) && isupper(key)) {
			key = tolower(key);
		}

		for (int i = 0; i < (int)UI.ButtonPanel.Buttons.size(); ++i) {
			if (CurrentButtons[i]->get_pos() != -1 && key == CurrentButtons[i]->get_key()) {
				UI.ButtonPanel.DoClicked(i, key_modifiers);
				return 1;
			}
		}
	}
	return 0;
}
