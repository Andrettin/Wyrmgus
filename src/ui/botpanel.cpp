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
//      (c) Copyright 1999-2020 by Lutz Sammer, Vladi Belperchinov-Shabanski,
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
//

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "ui/ui.h"

#include "achievement.h"
#include "actions.h"
//Wyrmgus start
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
//Wyrmgus end
#include "campaign.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "civilization.h"
#include "commands.h"
#include "database/defines.h"
#include "faction.h"
#include "font.h"
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "guichan/key.h"
#include "guichan/sdl/sdlinput.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "player.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
#include "script/trigger.h"
#include "sound/sound.h"
#include "spells.h"
#include "translate.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "ui/interface.h"
#include "ui/popup.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_manager.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/// Last drawn popup : used to speed up drawing
ButtonAction *LastDrawnButtonPopup;
/// for unit buttons sub-menus etc.
CButtonLevel *CurrentButtonLevel = nullptr;
/// All buttons for units
std::vector<ButtonAction *> UnitButtonTable;
/// Pointer to current buttons
std::vector<ButtonAction> CurrentButtons;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Initialize the buttons.
*/
void InitButtons()
{
	// Resolve the icon names.
	for (size_t i = 0; i != UnitButtonTable.size(); ++i) {
		//Wyrmgus start
//		UnitButtonTable[i]->Icon.Load();
		if (!UnitButtonTable[i]->Icon.Name.empty()) {
			UnitButtonTable[i]->Icon.Load();
		}
		//Wyrmgus end
	}
	CurrentButtons.clear();
}

/*----------------------------------------------------------------------------
--  Buttons structures
----------------------------------------------------------------------------*/

/**
**  FIXME: docu
*/
int AddButton(int pos, CButtonLevel *level, const std::string &icon_ident,
			  ButtonCmd action, const std::string &value, void* actionCb, const ButtonCheckFunc func,
			  const std::string &allow, const int key, const std::string &hint, const std::string &descr,
			  const std::string &sound, const std::string &cursor, const std::string &umask,
			  //Wyrmgus start
//			  const std::string &popup, bool alwaysShow)
			  const std::string &popup, bool alwaysShow, const std::string &mod_file)
			  //Wyrmgus end
{
	std::string buf;
	ButtonAction *ba = new ButtonAction;
	Assert(ba);

	ba->Pos = pos;
	ba->Level = level;
	ba->AlwaysShow = alwaysShow;
	ba->Icon.Name = icon_ident;
	ba->Payload = actionCb;
	// FIXME: check if already initited
	//ba->Icon.Load();
	ba->Action = action;
	if (!value.empty()) {
		ba->ValueStr = value;
		switch (action) {
			case ButtonCmd::SpellCast:
				ba->Value = CSpell::GetSpell(value)->Slot;
#ifdef DEBUG
				if (ba->Value < 0) {
					DebugPrint("Spell %s does not exist?\n" _C_ value.c_str());
					Assert(ba->Value >= 0);
				}
#endif
				break;
			case ButtonCmd::Train:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			case ButtonCmd::Research:
				ba->Value = UpgradeIdByIdent(value);
				break;
			//Wyrmgus start
			case ButtonCmd::LearnAbility:
				ba->Value = UpgradeIdByIdent(value);
				break;
			case ButtonCmd::ExperienceUpgradeTo:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			//Wyrmgus end
			case ButtonCmd::UpgradeTo:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			case ButtonCmd::Build:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			//Wyrmgus start
			case ButtonCmd::ProduceResource:
				ba->Value = GetResourceIdByName(value.c_str());
				break;
			case ButtonCmd::SellResource:
				ba->Value = GetResourceIdByName(value.c_str());
				break;
			case ButtonCmd::BuyResource:
				ba->Value = GetResourceIdByName(value.c_str());
				break;
			//Wyrmgus end
			case ButtonCmd::Button:
				if (CButtonLevel::GetButtonLevel(value)) {
					ba->Value = CButtonLevel::GetButtonLevel(value)->ID;
				} else {
					ba->Value = 0;
				}
				break;
			default:
				ba->Value = atoi(value.c_str());
				break;
		}
	} else {
		ba->ValueStr.clear();
		ba->Value = 0;
	}

	ba->Allowed = func;
	ba->AllowStr = allow;
	ba->Key = key;
	ba->Hint = hint;
	ba->Description = descr;
	ba->CommentSound.Name = sound;
	if (!ba->CommentSound.Name.empty()) {
		ba->CommentSound.MapSound();
	}
	if (!ba->Popup.empty()) {
		CPopup *popup = PopupByIdent(ba->Popup);
		if (!popup) {
			fprintf(stderr, "Popup \"%s\" hasn't defined.\n ", ba->Popup.c_str());
			Exit(1);
		}
	}
	ba->ButtonCursor = cursor;
	ba->Popup = popup;
	// FIXME: here should be added costs to the hint
	// FIXME: johns: show should be nice done?
	if (umask[0] == '*') {
		buf = umask;
	} else {
		buf = "," + umask + ",";
	}
	ba->UnitMask = buf;
	//Wyrmgus start
	ba->Mod = mod_file;
	//Wyrmgus end
	UnitButtonTable.push_back(ba);
	// FIXME: check if already initited
	//Assert(ba->Icon.Icon != nullptr);// just checks, that's why at the end
	return 1;
}


/**
**  Cleanup buttons.
*/
void CleanButtons()
{
	// Free the allocated buttons.
	for (size_t i = 0; i != UnitButtonTable.size(); ++i) {
		delete UnitButtonTable[i];
	}
	UnitButtonTable.clear();

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
static int GetButtonStatus(const ButtonAction &button, int UnderCursor)
{
	unsigned int res = 0;

	/* parallel drawing */
	if (Selected.empty()) {
		return res;
	}

	// cursor is on that button
	if (ButtonAreaUnderCursor == ButtonAreaButton && UnderCursor == button.Pos - 1) {
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
				Assert(Selected[i]->AutoCastSpell);
				if (Selected[i]->AutoCastSpell[button.Value] != 1) {
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
								const ButtonAction &button,
								const stratagus::unit_type *type)
{
	if (!condition) {
		return true;
	}

	if (condition->HasHint && button.GetHint().empty()) {
		return false;
	}

	if (condition->HasDescription && button.Description.empty()) {
		return false;
	}

	if (condition->HasDependencies && PrintDependencies(*CPlayer::GetThisPlayer(), button).empty()) {
		return false;
	}
	
	//Wyrmgus start
	if (condition->Class && type && type->get_unit_class() == nullptr && !(type->BoolFlag[ITEM_INDEX].value && type->ItemClass != -1)) {
		return false;
	}
	
	if (condition->unit_class != nullptr) {
		if (!type || condition->unit_class != type->get_unit_class()) {
			return false;
		}
	}

	if (condition->UnitTypeType != UnitTypeType::None) {
		if (!type || condition->UnitTypeType != type->UnitType) {
			return false;
		}
	}

	if (condition->CanStore != -1) {
		if (!type || !type->CanStore[condition->CanStore]) {
			return false;
		}
	}

	if (condition->ImproveIncome != -1) {
		if (!type || type->Stats[CPlayer::GetThisPlayer()->Index].ImproveIncomes[condition->ImproveIncome] <= stratagus::resource::get_all()[condition->ImproveIncome]->DefaultIncome) {
			return false;
		}
	}

	if (condition->ChildResources != CONDITION_TRUE) {
		if ((condition->ChildResources == CONDITION_ONLY) ^ (stratagus::resource::get_all()[button.Value]->ChildResources.size() > 0)) {
			return false;
		}
	}

	if (condition->ImproveIncomes != CONDITION_TRUE) {
		bool improve_incomes = false;
		if (button.Action == ButtonCmd::ProduceResource) {
			if (CPlayer::GetThisPlayer()->Incomes[button.Value] > stratagus::resource::get_all()[button.Value]->DefaultIncome) {
				improve_incomes = true;
			}
			for (const stratagus::resource *child_resource : stratagus::resource::get_all()[button.Value]->ChildResources) {
				if (CPlayer::GetThisPlayer()->Incomes[child_resource->ID] > child_resource->DefaultIncome) {
					improve_incomes = true;
					break;
				}
			}
		} else {
			if (!type) {
				return false;
			}
			for (int i = 1; i < MaxCosts; ++i) {
				if (type->Stats[CPlayer::GetThisPlayer()->Index].ImproveIncomes[i] > stratagus::resource::get_all()[i]->DefaultIncome) {
					improve_incomes = true;
					break;
				}
			}
		}
		if ((condition->ImproveIncomes == CONDITION_ONLY) ^ improve_incomes) {
			return false;
		}
	}

	if (condition->Description && type && type->Description.empty()) {
		return false;
	}
	
	if (condition->Quote && type && type->Quote.empty() && !((button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) && UnitManager.GetSlotUnit(button.Value).Unique && !UnitManager.GetSlotUnit(button.Value).Unique->Quote.empty()) && !((button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) && UnitManager.GetSlotUnit(button.Value).Work != nullptr && !UnitManager.GetSlotUnit(button.Value).Work->get_quote().empty() && UnitManager.GetSlotUnit(button.Value).Elixir != nullptr && !UnitManager.GetSlotUnit(button.Value).Elixir->get_quote().empty())) {
		return false;
	}
	
	if (condition->Encyclopedia && type && type->Description.empty() && type->Background.empty() && type->Quote.empty() && (!type->BoolFlag[ITEM_INDEX].value || type->ItemClass == -1)) {
		return false;
	}
	
	if (condition->settlement_name && !(button.Action == ButtonCmd::Unit && UnitManager.GetSlotUnit(button.Value).settlement)) {
		return false;
	}
	
	if (condition->CanActiveHarvest && !(button.Action == ButtonCmd::Unit && Selected.size() > 0 && Selected[0]->CanHarvest(&UnitManager.GetSlotUnit(button.Value), false))) {
		return false;
	}

	if (condition->FactionUpgrade != CONDITION_TRUE) {
		if ((condition->FactionUpgrade == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Faction)) {
			return false;
		}
	}
	
	if (condition->FactionCoreSettlements != CONDITION_TRUE) {
		if ((condition->FactionCoreSettlements == CONDITION_ONLY) ^ (stratagus::game::get()->get_current_campaign() != nullptr && button.Action == ButtonCmd::Faction && button.Value != -1 && stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[button.Value]->Cores.size() > 0)) {
			return false;
		}
	}
	
	CUpgrade *upgrade = nullptr;
	if (button.Action == ButtonCmd::Research || button.Action == ButtonCmd::LearnAbility) {
		upgrade = CUpgrade::get_all()[button.Value];
	} else if (button.Action == ButtonCmd::Faction && !stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[button.Value]->FactionUpgrade.empty()) {
		upgrade = CUpgrade::get(stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[button.Value]->FactionUpgrade);
	}
	
	if (condition->UpgradeResearched != CONDITION_TRUE) {
		if ((condition->UpgradeResearched == CONDITION_ONLY) ^ ((((button.Action == ButtonCmd::Research || button.Action == ButtonCmd::Faction) && UpgradeIdAllowed(*CPlayer::GetThisPlayer(), upgrade->ID) == 'R') || (button.Action == ButtonCmd::LearnAbility && Selected[0]->GetIndividualUpgrade(upgrade) >= upgrade->MaxLimit)))) {
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
		if ((condition->LuxuryResource == CONDITION_ONLY) ^ (button.Action == ButtonCmd::ProduceResource && stratagus::resource::get_all()[button.Value]->LuxuryResource)) {
			return false;
		}
	}
	
	if (condition->RequirementsString != CONDITION_TRUE) {
		if ((condition->RequirementsString == CONDITION_ONLY) ^ ((button.Action == ButtonCmd::Research || button.Action == ButtonCmd::LearnAbility || button.Action == ButtonCmd::Faction || button.Action == ButtonCmd::Train || button.Action == ButtonCmd::Build || button.Action == ButtonCmd::UpgradeTo || button.Action == ButtonCmd::Buy) && !IsButtonUsable(*Selected[0], button) && Selected[0]->Player == CPlayer::GetThisPlayer() && ((type && !type->RequirementsString.empty()) ||  ((button.Action == ButtonCmd::Research || button.Action == ButtonCmd::LearnAbility || button.Action == ButtonCmd::Faction) && !upgrade->get_requirements_string().empty())))) {
			return false;
		}
	}
	
	if (condition->ExperienceRequirementsString != CONDITION_TRUE) {
		if ((condition->ExperienceRequirementsString == CONDITION_ONLY) ^ (button.Action == ButtonCmd::ExperienceUpgradeTo && !IsButtonUsable(*Selected[0], button) && type && !type->ExperienceRequirementsString.empty())) {
			return false;
		}
	}
	
	if (condition->BuildingRulesString != CONDITION_TRUE) {
		if ((condition->BuildingRulesString == CONDITION_ONLY) ^ (button.Action == ButtonCmd::Build && type && !type->BuildingRulesString.empty())) {
			return false;
		}
	}
	//Wyrmgus end

	if (condition->ButtonAction != ButtonCmd::None && button.Action != condition->ButtonAction) {
		return false;
	}

	if (condition->ButtonValue.empty() == false && button.ValueStr != condition->ButtonValue) {
		return false;
	}

	if (type && condition->BoolFlags && !type->CheckUserBoolFlags(condition->BoolFlags)) {
		return false;
	}

	//Wyrmgus start
//	if (condition->Variables && type) {
	if (condition->Variables && type && button.Action != ButtonCmd::Unit && button.Action != ButtonCmd::Buy) {
	//Wyrmgus end
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
				if ((condition->Variables[i] == CONDITION_ONLY) ^ type->Stats[CPlayer::GetThisPlayer()->Index].Variables[i].Enable) {
					return false;
				}
			}
		}
	//Wyrmgus start
	} else if (condition->Variables && (button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy)) {
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
//				if ((condition->Variables[i] == CONDITION_ONLY) ^ UnitManager.GetSlotUnit(button.Value).Variable[i].Enable) {
				CUnit &unit = UnitManager.GetSlotUnit(button.Value);
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
			if ((condition->AutoCast == CONDITION_ONLY) ^ (CSpell::Spells[button.Value]->AutoCast != nullptr)) {
				return false;
			}
		}
	}
		
	if (button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) {
		CUnit &unit = UnitManager.GetSlotUnit(button.Value);
		if (unit.Type->BoolFlag[ITEM_INDEX].value) {
			if (condition->Equipped != CONDITION_TRUE) {
				if ((condition->Equipped == CONDITION_ONLY) ^ (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->IsItemEquipped(&unit))) {
					return false;
				}
			}
			if (condition->Equippable != CONDITION_TRUE) {
				if ((condition->Equippable == CONDITION_ONLY) ^ (unit.Container != nullptr && unit.Container->HasInventory() && unit.Container->CanEquipItem(&unit))) {
					return false;
				}
			}
			if (condition->Consumable != CONDITION_TRUE) {
				if ((condition->Consumable == CONDITION_ONLY) ^ IsItemClassConsumable(unit.Type->ItemClass)) {
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
				if ((condition->Weapon == CONDITION_ONLY) ^ (GetItemClassSlot(unit.Type->ItemClass) == WeaponItemSlot)) {
					return false;
				}
			}
			if (condition->Shield != CONDITION_TRUE) {
				if ((condition->Shield == CONDITION_ONLY) ^ (GetItemClassSlot(unit.Type->ItemClass) == ShieldItemSlot)) {
					return false;
				}
			}
			if (condition->Boots != CONDITION_TRUE) {
				if ((condition->Boots == CONDITION_ONLY) ^ (GetItemClassSlot(unit.Type->ItemClass) == BootsItemSlot)) {
					return false;
				}
			}
			if (condition->Arrows != CONDITION_TRUE) {
				if ((condition->Arrows == CONDITION_ONLY) ^ (GetItemClassSlot(unit.Type->ItemClass) == ArrowsItemSlot)) {
					return false;
				}
			}
			if (condition->ItemClass != -1) {
				if (condition->ItemClass != unit.Type->ItemClass) {
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
		
		if (!(button.Action == ButtonCmd::Buy && unit.Character) && condition->Opponent != CONDITION_TRUE) {
			if ((condition->Opponent == CONDITION_ONLY) ^ CPlayer::GetThisPlayer()->IsEnemy(unit)) {
				return false;
			}
		}
		if (!(button.Action == ButtonCmd::Buy && unit.Character) && condition->Neutral != CONDITION_TRUE) {
			if ((condition->Neutral == CONDITION_ONLY) ^ (!CPlayer::GetThisPlayer()->IsEnemy(unit) && !CPlayer::GetThisPlayer()->IsAllied(unit) && CPlayer::GetThisPlayer() != unit.Player && (unit.Container == nullptr || (!CPlayer::GetThisPlayer()->IsEnemy(*unit.Container) && !CPlayer::GetThisPlayer()->IsAllied(*unit.Container) && CPlayer::GetThisPlayer() != unit.Container->Player)))) {
				return false;
			}
		}
		
		if (condition->Affixed != CONDITION_TRUE) {
			if ((condition->Affixed == CONDITION_ONLY) ^ (unit.Prefix != nullptr || unit.Suffix != nullptr)) {
				return false;
			}
		}
		if (condition->Unique != CONDITION_TRUE) {
			if ((condition->Unique == CONDITION_ONLY) ^ (unit.Unique || unit.Character != nullptr)) {
				return false;
			}
		}
		if (condition->UniqueSet != CONDITION_TRUE) {
			if ((condition->UniqueSet == CONDITION_ONLY) ^ (unit.Unique && unit.Unique->Set)) {
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

static void GetPopupSize(const CPopup &popup, const ButtonAction &button,
						 int &popupWidth, int &popupHeight, int *Costs)
{
	int contentWidth = popup.MarginX;
	int contentHeight = 0;
	int maxContentWidth = 0;
	int maxContentHeight = 0;
	popupWidth = popup.MarginX;
	popupHeight = popup.MarginY;

	for (std::vector<CPopupContentType *>::const_iterator it = popup.Contents.begin();
		 it != popup.Contents.end();
		 ++it) {
		CPopupContentType &content = **it;

		//Wyrmgus start
//		if (CanShowPopupContent(content.Condition, button, UnitTypes[button.Value])) {
		if (
			(button.Action != ButtonCmd::Unit && button.Action != ButtonCmd::Buy && CanShowPopupContent(content.Condition, button, stratagus::unit_type::get_all()[button.Value]))
			|| ((button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) && CanShowPopupContent(content.Condition, button, UnitManager.GetSlotUnit(button.Value).Type))
		) {
		//Wyrmgus end
			// Automatically write the calculated coordinates.
			content.pos.x = contentWidth + content.MarginX;
			content.pos.y = popupHeight + content.MarginY;

			contentWidth += std::max(content.minSize.x, 2 * content.MarginX + content.GetWidth(button, Costs));
			contentHeight = std::max(content.minSize.y, 2 * content.MarginY + content.GetHeight(button, Costs));
			maxContentHeight = std::max(contentHeight, maxContentHeight);
			if (content.Wrap) {
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
void DrawPopup(const ButtonAction &button, int x, int y, bool above)
{
	CPopup *popup = PopupByIdent(button.Popup);
	bool useCache = false;
	const int scale_factor = stratagus::defines::get()->get_scale_factor();

	if (!popup) {
		return;
	} else if (&button == LastDrawnButtonPopup) {
		useCache = true;
	} else {
		LastDrawnButtonPopup = const_cast<ButtonAction *>(&button);
	}

	int popupWidth, popupHeight;
	int Costs[ManaResCost + 1];
	memset(Costs, 0, sizeof(Costs));

	switch (button.Action) {
		case ButtonCmd::Research:
			//Wyrmgus start
//			memcpy(Costs, CUpgrade::get_all()[button.Value]->Costs, sizeof(CUpgrade::get_all()[button.Value]->Costs));
			CPlayer::GetThisPlayer()->GetUpgradeCosts(CUpgrade::get_all()[button.Value], Costs);
			//Wyrmgus end
			break;
		case ButtonCmd::SpellCast:
			memcpy(Costs, CSpell::Spells[button.Value]->Costs, sizeof(CSpell::Spells[button.Value]->Costs));
			Costs[ManaResCost] = CSpell::Spells[button.Value]->ManaCost;
			break;
		case ButtonCmd::Build:
		case ButtonCmd::Train:
		case ButtonCmd::UpgradeTo:
			//Wyrmgus start
//			memcpy(Costs, UnitTypes[button.Value]->Stats[ThisPlayer->Index].Costs,
//				   sizeof(UnitTypes[button.Value]->Stats[ThisPlayer->Index].Costs));
//			Costs[FoodCost] = UnitTypes[button.Value]->Stats[ThisPlayer->Index].Variables[DEMAND_INDEX].Value;
			int type_costs[MaxCosts];
			CPlayer::GetThisPlayer()->GetUnitTypeCosts(stratagus::unit_type::get_all()[button.Value], type_costs, Selected[0]->Type->Stats[Selected[0]->Player->Index].GetUnitStock(stratagus::unit_type::get_all()[button.Value]) != 0);
			memcpy(Costs, type_costs, sizeof(type_costs));
			Costs[FoodCost] = stratagus::unit_type::get_all()[button.Value]->Stats[CPlayer::GetThisPlayer()->Index].Variables[DEMAND_INDEX].Value;
			//Wyrmgus end
			break;
		//Wyrmgus start
		case ButtonCmd::Buy:
			Costs[FoodCost] = UnitManager.GetSlotUnit(button.Value).Variable[DEMAND_INDEX].Value;
			Costs[CopperCost] = UnitManager.GetSlotUnit(button.Value).GetPrice();
			break;
		//Wyrmgus end
		default:
			break;
	}

	if (useCache) {
		popupWidth = popupCache.popupWidth;
		popupHeight = popupCache.popupHeight;
	} else {
		GetPopupSize(*popup, button, popupWidth, popupHeight, Costs);
		popupWidth = std::max(popupWidth, popup->MinWidth);
		popupHeight = std::max(popupHeight, popup->MinHeight);
		popupCache.popupWidth = popupWidth;
		popupCache.popupHeight = popupHeight;
	}

	x = std::min<int>(x, Video.Width - 1 - popupWidth);
	clamp<int>(&x, 0, Video.Width - 1);
	if (above) {
		y = y - popupHeight - 10 * scale_factor;
	} else { //below
		y = y + 10 * scale_factor;
	}
	clamp<int>(&y, 0, Video.Height - 1);

	// Background
	Video.FillTransRectangle(popup->BackgroundColor, x, y, popupWidth, popupHeight, popup->BackgroundColor >> ASHIFT);
	Video.DrawRectangle(popup->BorderColor, x, y, popupWidth, popupHeight);

	// Contents
	for (std::vector<CPopupContentType *>::const_iterator it = popup->Contents.begin();
		 it != popup->Contents.end(); ++it) {
		const CPopupContentType &content = **it;

		//Wyrmgus start
//		if (CanShowPopupContent(content.Condition, button, UnitTypes[button.Value])) {
		if (
			(button.Action != ButtonCmd::Unit && button.Action != ButtonCmd::Buy && CanShowPopupContent(content.Condition, button, stratagus::unit_type::get_all()[button.Value]))
			|| ((button.Action == ButtonCmd::Unit || button.Action == ButtonCmd::Buy) && CanShowPopupContent(content.Condition, button, UnitManager.GetSlotUnit(button.Value).Type))
		) {
		//Wyrmgus end
			content.Draw(x + content.pos.x, y + content.pos.y, *popup, popupWidth, button, Costs);
		}
	}
}

//Wyrmgus start
/**
**  Draw popup
*/
void DrawGenericPopup(const std::string &popup_text, int x, int y, std::string text_color, std::string highlight_color, bool above)
{
	const CFont &font = GetGameFont();
	
	const int scale_factor = stratagus::defines::get()->get_scale_factor();
	int MaxWidth = std::max(512 * scale_factor, Video.Width / 5);

	int i;
		
	//calculate content width
	int content_width = 0;
	std::string content_width_sub;
	i = 1;
	while (!(content_width_sub = GetLineFont(i++, popup_text, 0, &font)).empty()) {
		int line_width = font.getWidth(content_width_sub);
		int cost_symbol_pos = content_width_sub.find("COST_", 0);
		if (cost_symbol_pos != std::string::npos) {
			int res = std::stoi(content_width_sub.substr(cost_symbol_pos + 5, content_width_sub.find(" ", cost_symbol_pos) - (cost_symbol_pos + 5) + 1));
			line_width -= font.getWidth("COST_" + std::to_string(res));
			line_width += UI.Resources[res].G->Width;
		}
		content_width = std::max(content_width, line_width);
	}
	
	if (MaxWidth) {
		content_width = std::min(content_width, MaxWidth);
	}
	
	//calculate content height
	int content_height = 0;
	i = 1;
	while ((GetLineFont(i++, popup_text, MaxWidth, &font)).length()) {
		content_height += font.Height() + 2 * scale_factor;
	}
	
	int popupWidth, popupHeight;

	int contentWidth = MARGIN_X * scale_factor;
	int contentHeight = 0;
	int maxContentWidth = 0;
	int maxContentHeight = 0;
	popupWidth = MARGIN_X * scale_factor;
	popupHeight = MARGIN_Y * scale_factor;
	PixelPos pos(0, 0);
	
	bool wrap = true;

	// Automatically write the calculated coordinates.
	pos.x = contentWidth + MARGIN_X * scale_factor;
	pos.y = popupHeight + MARGIN_Y * scale_factor;

	contentWidth += std::max(0, 2 * MARGIN_X * scale_factor + content_width);
	contentHeight = std::max(0, 2 * MARGIN_Y * scale_factor + content_height);
	maxContentHeight = std::max(contentHeight, maxContentHeight);
	if (wrap) {
		popupWidth += std::max(0, contentWidth - maxContentWidth);
		popupHeight += maxContentHeight;
		maxContentWidth = std::max(maxContentWidth, contentWidth);
		contentWidth = MARGIN_X * scale_factor;
		maxContentHeight = 0;
	}

	popupWidth += MARGIN_X * scale_factor;
	popupHeight += MARGIN_Y * scale_factor;
	
	popupWidth = std::min(popupWidth, MaxWidth);
	popupHeight = std::max(popupHeight, 0);

	x = std::min<int>(x, Video.Width - 1 - popupWidth);
	clamp<int>(&x, 0, Video.Width - 1);
	if (above) {
		y = y - popupHeight - 10 * scale_factor;
	} else { //below
		y = y + 10 * scale_factor;
	}
	clamp<int>(&y, 0, Video.Height - 1);

	// Background
	IntColor BackgroundColor = Video.MapRGBA(TheScreen->format, 28, 28, 28, 208);
	IntColor BorderColor = Video.MapRGBA(TheScreen->format, 93, 93, 93, 160);

	Video.FillTransRectangle(BackgroundColor, x, y, popupWidth, popupHeight, BackgroundColor >> ASHIFT);
	Video.DrawRectangle(BorderColor, x, y, popupWidth, popupHeight);

	if (text_color.empty()) {
		text_color = "white";
	}
	if (highlight_color.empty()) {
		highlight_color = "yellow";
	}
	
	// Contents
	x += pos.x;
	y += pos.y;
	CLabel label(font, text_color, highlight_color);
	std::string sub;
	i = 0;
	int y_off = y;
	unsigned int width = MaxWidth
						 ? std::min(MaxWidth, popupWidth - 2 * MARGIN_X * scale_factor)
						 : 0;
	while ((sub = GetLineFont(++i, popup_text, width, &font)).length()) {
		if (sub.find("LINE", 0) != std::string::npos) {
			Video.FillRectangle(BorderColor, x + (-MARGIN_X + 1 - MARGIN_X) * scale_factor,
								y_off, popupWidth - 2 * scale_factor, 1);
			sub = sub.substr(sub.find("LINE", 0) + 4, sub.length());
		}
		int cost_symbol_pos = sub.find("COST_", 0);
		if (cost_symbol_pos != std::string::npos) {
			int x_offset = 0;
			int res = std::stoi(sub.substr(cost_symbol_pos + 5, sub.find(" ", cost_symbol_pos) - (cost_symbol_pos + 5) + 1));
			std::string sub_first = sub.substr(0, cost_symbol_pos);
			std::string sub_second = sub.substr(cost_symbol_pos + 5 + std::to_string((long long) res).length(), sub.length() - cost_symbol_pos - (5 + std::to_string((long long) res).length()));
			label.Draw(x, y_off, sub_first);
			x_offset += font.getWidth(sub_first);
			UI.Resources[res].G->DrawFrameClip(UI.Resources[res].IconFrame, x + x_offset, y + ((font.getHeight() - UI.Resources[res].G->Height) / 2), nullptr);
			x_offset += UI.Resources[res].G->Width;
			label.Draw(x + x_offset, y_off, sub_second);
		} else {
			label.Draw(x, y_off, sub);
		}
		y_off += font.Height() + 2 * scale_factor;
	}
}
//Wyrmgus end

/**
**  Draw button panel.
**
**  Draw all action buttons.
*/
void CButtonPanel::Draw()
{
	//  Draw background
	if (UI.ButtonPanel.G) {
		UI.ButtonPanel.G->DrawSubClip(0, 0,
									  UI.ButtonPanel.G->Width, UI.ButtonPanel.G->Height,
									  UI.ButtonPanel.X, UI.ButtonPanel.Y);
	}

	// No buttons
	if (CurrentButtons.empty()) {
		return;
	}
	std::vector<ButtonAction> &buttons(CurrentButtons);

	Assert(!Selected.empty());
	char buf[8];

	//  Draw all buttons.
	for (int i = 0; i < (int) UI.ButtonPanel.Buttons.size(); ++i) {
		if (buttons[i].Pos == -1) {
			continue;
		}
		Assert(buttons[i].Pos == i + 1);
		//Wyrmgus start
		//for neutral units, don't draw buttons that aren't training buttons (in other words, only draw buttons which are usable by neutral buildings)
		if (
			!buttons[i].AlwaysShow
			&& Selected[0]->Player != CPlayer::GetThisPlayer()
			&& !CPlayer::GetThisPlayer()->IsTeamed(*Selected[0])
			&& CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->Player, buttons[i].Action)
			&& !IsNeutralUsableButtonAction(buttons[i].Action)
		) {
			continue;
		}
		//Wyrmgus end
		bool gray = false;
		bool cooldownSpell = false;
		int maxCooldown = 0;
		for (size_t j = 0; j != Selected.size(); ++j) {
			if (!IsButtonAllowed(*Selected[j], buttons[i])) {
				gray = true;
				break;
			} else if (buttons[i].Action == ButtonCmd::SpellCast
					   && (*Selected[j]).SpellCoolDownTimers[CSpell::Spells[buttons[i].Value]->Slot]) {
				Assert(CSpell::Spells[buttons[i].Value]->CoolDown > 0);
				cooldownSpell = true;
				maxCooldown = std::max(maxCooldown, (*Selected[j]).SpellCoolDownTimers[CSpell::Spells[buttons[i].Value]->Slot]);
			}
		}
		//
		//  Tutorial show command key in icons
		//
		if (ShowCommandKey) {
			//Wyrmgus start
//			if (buttons[i].Key == gcn::Key::K_ESCAPE) {
			if (buttons[i].GetKey() == gcn::Key::K_ESCAPE) {
			//Wyrmgus end
				//Wyrmgus start
//				strcpy_s(buf, sizeof(buf), "ESC");
				strcpy_s(buf, sizeof(buf), "Esc");
				//Wyrmgus end
			//Wyrmgus start
			} else if (buttons[i].GetKey() == gcn::Key::K_PAGE_UP) {
				strcpy_s(buf, sizeof(buf), "PgUp");
			} else if (buttons[i].GetKey() == gcn::Key::K_PAGE_DOWN) {
				strcpy_s(buf, sizeof(buf), "PgDwn");
			} else if (buttons[i].GetKey() == gcn::Key::K_DELETE) {
				strcpy_s(buf, sizeof(buf), "Del");
			//Wyrmgus end
			} else {
				//Wyrmgus start
//				buf[0] = toupper(buttons[i].Key);
				buf[0] = toupper(buttons[i].GetKey());
				//Wyrmgus end
				buf[1] = '\0';
			}
		} else {
			buf[0] = '\0';
		}

		//
		// Draw main Icon.
		//
		const PixelPos pos(UI.ButtonPanel.Buttons[i].X, UI.ButtonPanel.Buttons[i].Y);

		//Wyrmgus start
		stratagus::icon *button_icon = buttons[i].Icon.Icon;
			
		// if there is a single unit selected, show the icon of its weapon/shield/boots/arrows equipped for the appropriate buttons
		if (buttons[i].Icon.Name.empty() && buttons[i].Action == ButtonCmd::Attack && Selected[0]->Type->CanTransport() && Selected[0]->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && Selected[0]->BoardCount > 0 && Selected[0]->UnitInside != nullptr && Selected[0]->UnitInside->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && Selected[0]->UnitInside->GetButtonIcon(buttons[i].Action) != nullptr) {
			button_icon = Selected[0]->UnitInside->GetButtonIcon(buttons[i].Action);
		} else if (buttons[i].Icon.Name.empty() && Selected[0]->GetButtonIcon(buttons[i].Action) != nullptr) {
			button_icon = Selected[0]->GetButtonIcon(buttons[i].Action);
		} else if (buttons[i].Action == ButtonCmd::ExperienceUpgradeTo && Selected[0]->GetVariation() && stratagus::unit_type::get_all()[buttons[i].Value]->GetVariation(Selected[0]->GetVariation()->VariationId) != nullptr && !stratagus::unit_type::get_all()[buttons[i].Value]->GetVariation(Selected[0]->GetVariation()->VariationId)->Icon.Name.empty()) {
			button_icon = stratagus::unit_type::get_all()[buttons[i].Value]->GetVariation(Selected[0]->GetVariation()->VariationId)->Icon.Icon;
		} else if ((buttons[i].Action == ButtonCmd::Train || buttons[i].Action == ButtonCmd::Build || buttons[i].Action == ButtonCmd::UpgradeTo || buttons[i].Action == ButtonCmd::ExperienceUpgradeTo) && buttons[i].Icon.Name.empty() && stratagus::unit_type::get_all()[buttons[i].Value]->GetDefaultVariation(CPlayer::GetThisPlayer()) != nullptr && !stratagus::unit_type::get_all()[buttons[i].Value]->GetDefaultVariation(CPlayer::GetThisPlayer())->Icon.Name.empty()) {
			button_icon = stratagus::unit_type::get_all()[buttons[i].Value]->GetDefaultVariation(CPlayer::GetThisPlayer())->Icon.Icon;
		} else if ((buttons[i].Action == ButtonCmd::Train || buttons[i].Action == ButtonCmd::Build || buttons[i].Action == ButtonCmd::UpgradeTo || buttons[i].Action == ButtonCmd::ExperienceUpgradeTo) && buttons[i].Icon.Name.empty() && !stratagus::unit_type::get_all()[buttons[i].Value]->Icon.Name.empty()) {
			button_icon = stratagus::unit_type::get_all()[buttons[i].Value]->Icon.Icon;
		} else if (buttons[i].Action == ButtonCmd::Buy) {
			button_icon = UnitManager.GetSlotUnit(buttons[i].Value).GetIcon().Icon;
		} else if (buttons[i].Action == ButtonCmd::Research && buttons[i].Icon.Name.empty() && CUpgrade::get_all()[buttons[i].Value]->get_icon()) {
			button_icon = CUpgrade::get_all()[buttons[i].Value]->get_icon();
		} else if (buttons[i].Action == ButtonCmd::Faction && buttons[i].Icon.Name.empty() && stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[buttons[i].Value]->get_icon() != nullptr) {
			button_icon = stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[buttons[i].Value]->get_icon();
		}
		//Wyrmgus end
		
		if (cooldownSpell) {
			//Wyrmgus start
//			buttons[i].Icon.Icon->DrawCooldownSpellIcon(pos,
			button_icon->DrawCooldownSpellIcon(pos,
			//Wyrmgus end
														maxCooldown * 100 / CSpell::Spells[buttons[i].Value]->CoolDown);
		} else if (gray) {
			//Wyrmgus start
//			buttons[i].Icon.Icon->DrawGrayscaleIcon(pos);
//			button_icon->DrawGrayscaleIcon(pos); //better to not show it
			//Wyrmgus end
		} else {
			const stratagus::player_color *player_color = nullptr;
			if (Selected.empty() == false && Selected[0]->IsAlive()) {
				player_color = Selected[0]->get_player_color();

				//Wyrmgus start
				//if is accessing a building of another player, set color to that of the person player (i.e. for training buttons)
				if (CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->Player, buttons[i].Action)) {
					player_color = CPlayer::GetThisPlayer()->get_player_color();
				}
				//Wyrmgus end
			}
			
			if (IsButtonUsable(*Selected[0], buttons[i])) {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(buttons[i], ButtonUnderCursor),
												   pos, buf, player_color, false, false, 100 - GetButtonCooldownPercent(*Selected[0], buttons[i]));
												   
				if (
					(buttons[i].Action == ButtonCmd::Train && Selected[0]->Type->Stats[Selected[0]->Player->Index].GetUnitStock(stratagus::unit_type::get_all()[buttons[i].Value]) != 0)
					|| buttons[i].Action == ButtonCmd::SellResource || buttons[i].Action == ButtonCmd::BuyResource
				) {
					std::string number_string;
					if (buttons[i].Action == ButtonCmd::Train && Selected[0]->Type->Stats[Selected[0]->Player->Index].GetUnitStock(stratagus::unit_type::get_all()[buttons[i].Value]) != 0) { //draw the quantity in stock for unit "training" cases which have it
						number_string = std::to_string((long long) Selected[0]->GetUnitStock(stratagus::unit_type::get_all()[buttons[i].Value])) + "/" + std::to_string((long long) Selected[0]->Type->Stats[Selected[0]->Player->Index].GetUnitStock(stratagus::unit_type::get_all()[buttons[i].Value]));
					} else if (buttons[i].Action == ButtonCmd::SellResource) {
						number_string = std::to_string((long long) Selected[0]->Player->GetEffectiveResourceSellPrice(buttons[i].Value));
					} else if (buttons[i].Action == ButtonCmd::BuyResource) {
						number_string = std::to_string((long long) Selected[0]->Player->GetEffectiveResourceBuyPrice(buttons[i].Value));
					}
					std::string oldnc;
					std::string oldrc;
					GetDefaultTextColors(oldnc, oldrc);
					CLabel label(GetGameFont(), oldnc, oldrc);

					label.Draw(pos.x + 46 - GetGameFont().Width(number_string), pos.y + 0, number_string);
				}
			} else if ( //draw researched technologies (or acquired abilities) grayed
				buttons[i].Action == ButtonCmd::Research && UpgradeIdentAllowed(*CPlayer::GetThisPlayer(), buttons[i].ValueStr) == 'R'
				|| (buttons[i].Action == ButtonCmd::LearnAbility && Selected[0]->GetIndividualUpgrade(CUpgrade::get(buttons[i].ValueStr)) == CUpgrade::get(buttons[i].ValueStr)->MaxLimit)
			) {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(buttons[i], ButtonUnderCursor),
												   pos, buf, player_color, false, true);
			} else {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(buttons[i], ButtonUnderCursor),
												   pos, buf, player_color, true);
			}
		}
	}
	//
	//  Update status line for this button and draw popups
	//
	//Wyrmgus start
	/*
	for (int i = 0; i < (int) UI.ButtonPanel.Buttons.size(); ++i) {
		if (ButtonAreaUnderCursor == ButtonAreaButton &&
			//Wyrmgus start
//			ButtonUnderCursor == i && KeyState != KeyStateInput) {
			ButtonUnderCursor == i && KeyState != KeyStateInput
			&& buttons[i].Level == CurrentButtonLevel) {
			//Wyrmgus end
				if (!Preference.NoStatusLineTooltips) {
					UpdateStatusLineForButton(buttons[i]);
				}
				DrawPopup(buttons[i], UI.ButtonPanel.Buttons[i].X,
					UI.ButtonPanel.Buttons[i].Y);
		}
	}
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	if (ButtonAreaUnderCursor == ButtonAreaTransporting) {
		CUnit *uins = Selected[0]->UnitInside;
		size_t j = 0;

		for (int i = 0; i < Selected[0]->InsideCount; ++i, uins = uins->NextContained) {
			if (!uins->Boarded || j >= UI.TransportingButtons.size() || (Selected[0]->Player != CPlayer::GetThisPlayer() && uins->Player != CPlayer::GetThisPlayer())) {
				continue;
			}
			if (static_cast<size_t>(ButtonUnderCursor) == j) {
				std::string text_color;
				if (uins->Unique || uins->Character != nullptr) {
					text_color = "fire";
				} else if (uins->Prefix != nullptr || uins->Suffix != nullptr) {
					text_color = "light-blue";
				}
				DrawGenericPopup(uins->GetMessageName(), UI.TransportingButtons[j].X, UI.TransportingButtons[j].Y, text_color);
			}
			++j;
		}
	}
	//Wyrmgus end
}

/**
**  Update the status line with hints from the button
**
**  @param button  Button
*/
void UpdateStatusLineForButton(const ButtonAction &button)
{
	//Wyrmgus start
//	UI.StatusLine.Set(button.Hint);
	UI.StatusLine.Set(button.GetHint());
	//Wyrmgus end
	switch (button.Action) {
		case ButtonCmd::Build:
		case ButtonCmd::Train:
		case ButtonCmd::UpgradeTo: {
			// FIXME: store pointer in button table!
			//Wyrmgus start
//			const CUnitStats &stats = UnitTypes[button.Value]->Stats[CPlayer::GetThisPlayer()->Index];
//			UI.StatusLine.SetCosts(0, stats.Variables[DEMAND_INDEX].Value, stats.Costs);
			int type_costs[MaxCosts];
			CPlayer::GetThisPlayer()->GetUnitTypeCosts(stratagus::unit_type::get_all()[button.Value], type_costs, Selected[0]->Type->Stats[Selected[0]->Player->Index].GetUnitStock(stratagus::unit_type::get_all()[button.Value]) != 0);
			UI.StatusLine.SetCosts(0, stratagus::unit_type::get_all()[button.Value]->Stats[CPlayer::GetThisPlayer()->Index].Variables[DEMAND_INDEX].Value * (stratagus::unit_type::get_all()[button.Value]->TrainQuantity ? stratagus::unit_type::get_all()[button.Value]->TrainQuantity : 1), type_costs);
			//Wyrmgus end
			break;
		}
		case ButtonCmd::Research:
			UI.StatusLine.SetCosts(0, 0, CUpgrade::get_all()[button.Value]->Costs);
			break;
		case ButtonCmd::SpellCast:
			UI.StatusLine.SetCosts(CSpell::Spells[button.Value]->ManaCost, 0, CSpell::Spells[button.Value]->Costs);
			break;
		default:
			UI.StatusLine.ClearCosts();
			break;
	}
}
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
bool IsButtonAllowed(const CUnit &unit, const ButtonAction &buttonaction)
{
	if (buttonaction.AlwaysShow) {
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
	
	//Wyrmgus start
	if (!CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) && (!CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->Player, buttonaction.Action) || !IsNeutralUsableButtonAction(buttonaction.Action))) {
		return false;
	}
	//Wyrmgus end

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
			res = unit.Type->RepairRange > 0;
			break;
		case ButtonCmd::Patrol:
			res = unit.CanMove();
			break;
		case ButtonCmd::Harvest:
			if (!unit.CurrentResource
				|| !(unit.ResourcesHeld > 0 && !unit.Type->ResInfo[unit.CurrentResource]->LoseResources)
				|| (unit.ResourcesHeld != unit.Type->ResInfo[unit.CurrentResource]->ResourceCapacity
					&& unit.Type->ResInfo[unit.CurrentResource]->LoseResources)) {
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
				  || !(unit.ResourcesHeld > 0 && !unit.Type->ResInfo[unit.CurrentResource]->LoseResources)
				  || (unit.ResourcesHeld != unit.Type->ResInfo[unit.CurrentResource]->ResourceCapacity
					  && unit.Type->ResInfo[unit.CurrentResource]->LoseResources))) {
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
			// Check if building queue is enabled
			if (!EnableTrainingQueue && unit.CurrentAction() == UnitAction::Train) {
				break;
			}
			if (unit.Player->Index == PlayerNumNeutral && !unit.CanHireMercenary(stratagus::unit_type::get_all()[buttonaction.Value])) {
				break;
			}
		// FALL THROUGH
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::Research:
		case ButtonCmd::Build:
			if (buttonaction.Action == ButtonCmd::Research) {
				res = CheckDependencies(CUpgrade::get_all()[buttonaction.Value], unit.Player, false, true, !CPlayer::GetThisPlayer()->IsTeamed(unit));
				if (res) {
					//Wyrmgus start
	//				res = UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'A';
					res = (UpgradeIdentAllowed(*CPlayer::GetThisPlayer(), buttonaction.ValueStr) == 'A' || UpgradeIdentAllowed(*CPlayer::GetThisPlayer(), buttonaction.ValueStr) == 'R') && CheckDependencies(CUpgrade::get_all()[buttonaction.Value], CPlayer::GetThisPlayer(), false, true); //also check for the dependencies for this player (rather than the unit) as an extra for researches, so that the player doesn't research too advanced technologies at neutral buildings
					res = res && (!unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(buttonaction.ValueStr)] || unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(buttonaction.ValueStr)] == CUpgrade::get_all()[UpgradeIdByIdent(buttonaction.ValueStr)]->Costs[TimeCost]); //don't show if is being researched elsewhere
					//Wyrmgus end
				}
			} else {
				res = CheckDependencies(stratagus::unit_type::get_all()[buttonaction.Value], unit.Player, false, true, !CPlayer::GetThisPlayer()->IsTeamed(unit));
			}
			break;
		case ButtonCmd::ExperienceUpgradeTo:
			res = CheckDependencies(stratagus::unit_type::get_all()[buttonaction.Value], &unit, true, true);
			if (res && unit.Character != nullptr) {
				res = std::find(unit.Character->ForbiddenUpgrades.begin(), unit.Character->ForbiddenUpgrades.end(), stratagus::unit_type::get_all()[buttonaction.Value]) == unit.Character->ForbiddenUpgrades.end();
			}
			break;
		case ButtonCmd::LearnAbility:
			res = unit.CanLearnAbility(CUpgrade::get(buttonaction.ValueStr), true);
			break;
		case ButtonCmd::SpellCast:
			res = CSpell::Spells[buttonaction.Value]->IsAvailableForUnit(unit);
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
			res = (unit.CurrentAction() == UnitAction::UpgradeTo && static_cast<COrder_UpgradeTo *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->Index].Costs[TimeCost] > 0)
				|| (unit.CurrentAction() == UnitAction::Research && static_cast<COrder_Research *>(unit.CurrentOrder())->GetUpgrade().Costs[TimeCost] > 0);
			//Wyrmgus end
			break;
		case ButtonCmd::CancelTrain:
			//Wyrmgus start
//			res = unit.CurrentAction() == UnitAction::Train;
			res = unit.CurrentAction() == UnitAction::Train && static_cast<COrder_Train *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->Index].Costs[TimeCost] > 0; //don't show the cancel button for a quick moment if the time cost is 0
			//Wyrmgus end
			break;
		case ButtonCmd::CancelBuild:
			res = unit.CurrentAction() == UnitAction::Built;
			break;
		//Wyrmgus start
		case ButtonCmd::Faction:
			res = CPlayer::GetThisPlayer()->Faction != -1 && buttonaction.Value != -1 && buttonaction.Value < (int) stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo.size() && CPlayer::GetThisPlayer()->CanFoundFaction(stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[buttonaction.Value], true);
			break;
		case ButtonCmd::Quest:
			res = buttonaction.Value < (int) unit.Player->AvailableQuests.size() && unit.Player->can_accept_quest(unit.Player->AvailableQuests[buttonaction.Value]);
			break;
		case ButtonCmd::Buy:
			res = (buttonaction.Value != -1) && (&UnitManager.GetSlotUnit(buttonaction.Value) != nullptr);
			break;
		//Wyrmgus end
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
bool IsButtonUsable(const CUnit &unit, const ButtonAction &buttonaction)
{
	if (!IsButtonAllowed(unit, buttonaction)) {
		return false;
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
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::Research:
		case ButtonCmd::Build:
			if (buttonaction.Action == ButtonCmd::Research) {
				res = CheckDependencies(CUpgrade::get_all()[buttonaction.Value], unit.Player, false, false, !CPlayer::GetThisPlayer()->IsTeamed(unit));
				if (res) {
					res = UpgradeIdentAllowed(*CPlayer::GetThisPlayer(), buttonaction.ValueStr) == 'A' && CheckDependencies(CUpgrade::get_all()[buttonaction.Value], CPlayer::GetThisPlayer(), false, false); //also check for the dependencies of this player extra for researches, so that the player doesn't research too advanced technologies at neutral buildings
				}
			} else {
				res = CheckDependencies(stratagus::unit_type::get_all()[buttonaction.Value], unit.Player, false, false, !CPlayer::GetThisPlayer()->IsTeamed(unit));
			}
			break;
		case ButtonCmd::ExperienceUpgradeTo:
			res = CheckDependencies(stratagus::unit_type::get_all()[buttonaction.Value], &unit, true, false) && unit.Variable[LEVELUP_INDEX].Value >= 1;
			break;
		case ButtonCmd::LearnAbility:
			res = unit.CanLearnAbility(CUpgrade::get(buttonaction.ValueStr));
			break;
		case ButtonCmd::SpellCast:
			res = CSpell::Spells[buttonaction.Value]->IsAvailableForUnit(unit);
			break;
		case ButtonCmd::Faction:
			res = CPlayer::GetThisPlayer()->CanFoundFaction(stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[buttonaction.Value]);
			break;
		case ButtonCmd::Buy:
			res = true;
			if (UnitManager.GetSlotUnit(buttonaction.Value).Character != nullptr) {
				res = CPlayer::GetThisPlayer()->Heroes.size() < PlayerHeroMax;
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
int GetButtonCooldown(const CUnit &unit, const ButtonAction &buttonaction)
{
	int cooldown = 0;

	// Check button-specific cases
	switch (buttonaction.Action) {
		case ButtonCmd::Buy:
			if (buttonaction.Value != -1 && UnitManager.GetSlotUnit(buttonaction.Value).Character != nullptr) {
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
int GetButtonCooldownPercent(const CUnit &unit, const ButtonAction &buttonaction)
{
	int cooldown = 0;

	// Check button-specific cases
	switch (buttonaction.Action) {
		case ButtonCmd::Buy:
			if (buttonaction.Value != -1 && UnitManager.GetSlotUnit(buttonaction.Value).Character != nullptr) {
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
static void UpdateButtonPanelMultipleUnits(std::vector<ButtonAction> *buttonActions)
{
	buttonActions->resize(UI.ButtonPanel.Buttons.size());
	for (size_t z = 0; z < UI.ButtonPanel.Buttons.size(); ++z) {
		(*buttonActions)[z].Pos = -1;
	}
	char unit_ident[128];
	//Wyrmgus start
	char individual_unit_ident[200][128]; // the 200 there is the max selectable quantity; not nice to hardcode it like this, should be changed in the future
	//Wyrmgus end

	sprintf(unit_ident, ",%s-group,", stratagus::civilization::get_all()[CPlayer::GetThisPlayer()->Race]->get_identifier().c_str());
	
	//Wyrmgus start
	for (size_t i = 0; i != Selected.size(); ++i) {
		sprintf(individual_unit_ident[i], ",%s,", Selected[i]->Type->Ident.c_str());
	}
	//Wyrmgus end

	for (size_t z = 0; z < UnitButtonTable.size(); ++z) {
		if (UnitButtonTable[z]->Level != CurrentButtonLevel) {
			continue;
		}

		//Wyrmgus start
		bool used_by_all = true;
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (!strstr(UnitButtonTable[z]->UnitMask.c_str(), individual_unit_ident[i])) {
				used_by_all = false;
				break;
			}
		}
		//Wyrmgus end
		
		// any unit or unit in list
		if (UnitButtonTable[z]->UnitMask[0] != '*'
			//Wyrmgus start
//			&& !strstr(UnitButtonTable[z]->UnitMask.c_str(), unit_ident)) {
			&& !strstr(UnitButtonTable[z]->UnitMask.c_str(), unit_ident) && !used_by_all) {
			//Wyrmgus end
			continue;
		}

		bool allow = true;
		if (UnitButtonTable[z]->AlwaysShow == false) {
			for (size_t i = 0; i != Selected.size(); ++i) {
				if (!IsButtonAllowed(*Selected[i], *UnitButtonTable[z])) {
					allow = false;
					break;
				}
			}
		}

		Assert(1 <= UnitButtonTable[z]->Pos);
		Assert(UnitButtonTable[z]->Pos <= (int)UI.ButtonPanel.Buttons.size());

		// is button allowed after all?
		if (allow) {
			// OverWrite, So take last valid button.
			(*buttonActions)[UnitButtonTable[z]->Pos - 1] = *UnitButtonTable[z];
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
static void UpdateButtonPanelSingleUnit(const CUnit &unit, std::vector<ButtonAction> *buttonActions)
{
	buttonActions->resize(UI.ButtonPanel.Buttons.size());

	for (size_t i = 0; i != UI.ButtonPanel.Buttons.size(); ++i) {
		(*buttonActions)[i].Pos = -1;
	}
	char unit_ident[128];

	//
	//  FIXME: johns: some hacks for cancel buttons
	//
	if (unit.CurrentAction() == UnitAction::Built) {
		// Trick 17 to get the cancel-build button
		strcpy_s(unit_ident, sizeof(unit_ident), ",cancel-build,");
	} else if (unit.CurrentAction() == UnitAction::UpgradeTo) {
		// Trick 17 to get the cancel-upgrade button
		strcpy_s(unit_ident, sizeof(unit_ident), ",cancel-upgrade,");
	} else if (unit.CurrentAction() == UnitAction::Research) {
		// Trick 17 to get the cancel-upgrade button
		strcpy_s(unit_ident, sizeof(unit_ident), ",cancel-upgrade,");
	} else {
		sprintf(unit_ident, ",%s,", unit.Type->Ident.c_str());
	}
	for (size_t i = 0; i != UnitButtonTable.size(); ++i) {
		ButtonAction &buttonaction = *UnitButtonTable[i];
		Assert(0 < buttonaction.Pos && buttonaction.Pos <= (int)UI.ButtonPanel.Buttons.size());

		// Same level
		if (buttonaction.Level != CurrentButtonLevel) {
			continue;
		}

		// any unit or unit in list
		if (buttonaction.UnitMask[0] != '*'
			&& !strstr(buttonaction.UnitMask.c_str(), unit_ident)) {
			continue;
		}
		//Wyrmgus start
//		int allow = IsButtonAllowed(unit, buttonaction);
		bool allow = true; // check all selected units, as different units of the same type may have different allowed buttons
		if (UnitButtonTable[i]->AlwaysShow == false) {
			for (size_t j = 0; j != Selected.size(); ++j) {
				if (!IsButtonAllowed(*Selected[j], *UnitButtonTable[i])) {
					allow = false;
					break;
				}
			}
		}
		//Wyrmgus end
		int pos = buttonaction.Pos;

		// Special case for researches
		int researchCheck = true;
		if (buttonaction.AlwaysShow && !allow && buttonaction.Action == ButtonCmd::Research
			//Wyrmgus start
//			&& UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'R') {
			&& UpgradeIdentAllowed(*CPlayer::GetThisPlayer(), buttonaction.ValueStr) == 'R') {
			//Wyrmgus end
			researchCheck = false;
		}
		
		// is button allowed after all?
		if ((buttonaction.AlwaysShow && (*buttonActions)[pos - 1].Pos == -1 && researchCheck) || allow) {
			// OverWrite, So take last valid button.
			(*buttonActions)[pos - 1] = buttonaction;
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
	if (unit.Player != CPlayer::GetThisPlayer() && !CPlayer::GetThisPlayer()->IsTeamed(unit) && !CPlayer::GetThisPlayer()->HasBuildingAccess(*unit.Player)) {
	//Wyrmgus end
		CurrentButtons.clear();
		return;
	}
	
	//Wyrmgus start
	//update the sold item buttons
	if (GameRunning || GameEstablishing) {
		unsigned int sold_unit_count = 0;
		unsigned int potential_faction_count = 0;
		for (int i = 0; i < (int) UnitButtonTable.size(); ++i) {
			if (UnitButtonTable[i]->Action != ButtonCmd::Faction && UnitButtonTable[i]->Action != ButtonCmd::Buy) {
				continue;
			}
			char unit_ident[128];
			sprintf(unit_ident, ",%s,", unit.Type->Ident.c_str());
			if (UnitButtonTable[i]->UnitMask[0] != '*' && !strstr(UnitButtonTable[i]->UnitMask.c_str(), unit_ident)) {
				continue;
			}

			if (UnitButtonTable[i]->Action == ButtonCmd::Faction) {
				if (CPlayer::GetThisPlayer()->Faction == -1 || potential_faction_count >= stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo.size()) {
					UnitButtonTable[i]->Value = -1;
				} else {
					UnitButtonTable[i]->Value = potential_faction_count;
					UnitButtonTable[i]->Hint = "Found ";
					if (stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[potential_faction_count]->DefiniteArticle) {
						UnitButtonTable[i]->Hint += "the ";
					}
					UnitButtonTable[i]->Hint += stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[potential_faction_count]->get_name();
					UnitButtonTable[i]->Description = "Changes your faction to ";
					if (stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[potential_faction_count]->DefiniteArticle) {
						UnitButtonTable[i]->Description += "the ";
					}
					UnitButtonTable[i]->Description += stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[potential_faction_count]->get_name();
				}
				potential_faction_count += 1;
			} else if (UnitButtonTable[i]->Action == ButtonCmd::Buy) {
				if (sold_unit_count >= unit.SoldUnits.size()) {
					UnitButtonTable[i]->Value = -1;
				} else {
					UnitButtonTable[i]->Value = UnitNumber(*unit.SoldUnits[sold_unit_count]);
					if (unit.SoldUnits[sold_unit_count]->Character != nullptr) {
						UnitButtonTable[i]->Hint = "Recruit " + unit.SoldUnits[sold_unit_count]->GetName();
					} else {
						if (!unit.SoldUnits[sold_unit_count]->Name.empty()) {
							UnitButtonTable[i]->Hint = "Buy " + unit.SoldUnits[sold_unit_count]->GetName();
						} else {
							UnitButtonTable[i]->Hint = "Buy " + unit.SoldUnits[sold_unit_count]->GetTypeName();
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

	// We have selected different units types
	if (!sameType) {
		UpdateButtonPanelMultipleUnits(&CurrentButtons);
	} else {
		// We have same type units selected
		// -- continue with setting buttons as for the first unit
		UpdateButtonPanelSingleUnit(unit, &CurrentButtons);
	}
}

void CButtonPanel::DoClicked_SelectTarget(int button)
{
	// Select target.
	CurrentCursorState = CursorState::Select;
	if (CurrentButtons[button].ButtonCursor.length() && CursorByIdent(CurrentButtons[button].ButtonCursor)) {
		GameCursor = CursorByIdent(CurrentButtons[button].ButtonCursor);
		CustomCursor = CurrentButtons[button].ButtonCursor;
	} else {
		GameCursor = UI.YellowHair.Cursor;
	}
	CursorAction = CurrentButtons[button].Action;
	CursorValue = CurrentButtons[button].Value;
	CurrentButtonLevel = CButtonLevel::CancelButtonLevel; // the cancel-only button level
	UI.ButtonPanel.Update();
	UI.StatusLine.Set(_("Select Target"));
}

void CButtonPanel::DoClicked_Unload(int button)
{
	const int flush = !(KeyModifiers & ModifierShift);
	//
	//  Unload on coast, transporter standing, unload all units right now.
	//  That or a bunker.
	//  Unload on coast valid only for sea units
	//
	if ((Selected.size() == 1 && Selected[0]->CurrentAction() == UnitAction::Still
		 && Selected[0]->Type->UnitType == UnitTypeType::Naval && Selected[0]->MapLayer->Field(Selected[0]->tilePos)->CoastOnMap())
		|| !Selected[0]->CanMove()) {
		SendCommandUnload(*Selected[0], Selected[0]->tilePos, NoUnitP, flush, Selected[0]->MapLayer->ID);
		return ;
	}
	DoClicked_SelectTarget(button);
}

void CButtonPanel::DoClicked_SpellCast(int button)
{
	const int spellId = CurrentButtons[button].Value;
	if (KeyModifiers & ModifierControl) {
		int autocast = 0;

		if (!CSpell::Spells[spellId]->AutoCast) {
			PlayGameSound(GameSounds.PlacementError[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
			return;
		}

		//autocast = 0;
		// If any selected unit doesn't have autocast on turn it on
		// for everyone
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (Selected[i]->AutoCastSpell[spellId] == 0) {
				autocast = 1;
				break;
			}
		}
		for (size_t i = 0; i != Selected.size(); ++i) {
			if (Selected[i]->AutoCastSpell[spellId] != autocast) {
				SendCommandAutoSpellCast(*Selected[i], spellId, autocast);
			}
		}
		return;
	}
	if (CSpell::Spells[spellId]->IsCasterOnly()) {
		const int flush = !(KeyModifiers & ModifierShift);

		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit &unit = *Selected[i];
			// CursorValue here holds the spell type id
			SendCommandSpellCast(unit, unit.tilePos, &unit, spellId, flush, unit.MapLayer->ID);
		}
		return;
	}
	DoClicked_SelectTarget(button);
}

void CButtonPanel::DoClicked_Repair(int button)
{
	if (KeyModifiers & ModifierControl) {
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

void CButtonPanel::DoClicked_Return()
{
	for (size_t i = 0; i != Selected.size(); ++i) {
		SendCommandReturnGoods(*Selected[i], NoUnitP, !(KeyModifiers & ModifierShift));
	}
}

void CButtonPanel::DoClicked_Stop()
{
	for (size_t i = 0; i != Selected.size(); ++i) {
		SendCommandStopUnit(*Selected[i]);
	}
}

void CButtonPanel::DoClicked_StandGround()
{
	for (size_t i = 0; i != Selected.size(); ++i) {
		SendCommandStandGround(*Selected[i], !(KeyModifiers & ModifierShift));
	}
}

void CButtonPanel::DoClicked_Button(int button)
{
	CurrentButtonLevel = CButtonLevel::GetButtonLevel(CurrentButtons[button].ValueStr);
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
	GameCursor = UI.Point.Cursor;
	CursorBuilding = nullptr;
	CurrentCursorState = CursorState::Point;
}

void CButtonPanel::DoClicked_CancelTrain()
{
	Assert(Selected[0]->CurrentAction() == UnitAction::Train);
	SendCommandCancelTraining(*Selected[0], -1, nullptr);
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
}

void CButtonPanel::DoClicked_CancelBuild()
{
	// FIXME: johns is this not sure, only building should have this?
	Assert(Selected[0]->CurrentAction() == UnitAction::Built);
	if (Selected.size() == 1) {
		SendCommandDismiss(*Selected[0]);
	}
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
}

void CButtonPanel::DoClicked_Build(int button)
{
	// FIXME: store pointer in button table!
	stratagus::unit_type &type = *stratagus::unit_type::get_all()[CurrentButtons[button].Value];
	if (!CPlayer::GetThisPlayer()->CheckUnitType(type)) {
		UI.StatusLine.Set(_("Select Location"));
		UI.StatusLine.ClearCosts();
		CursorBuilding = &type;
		// FIXME: check is this check necessary?
		CurrentButtonLevel = CButtonLevel::CancelButtonLevel; // the cancel-only button level
		UI.ButtonPanel.Update();
	}
}

void CButtonPanel::DoClicked_Train(int button)
{
	// FIXME: store pointer in button table!
	stratagus::unit_type &type = *stratagus::unit_type::get_all()[CurrentButtons[button].Value];
	// FIXME: Johns: I want to place commands in queue, even if not
	// FIXME:        enough resources are available.
	// FIXME: training queue full check is not correct for network.
	// FIXME: this can be correct written, with a little more code.
	//Wyrmgus start
	/*
	if (Selected[0]->CurrentAction() == UnitAction::Train && !EnableTrainingQueue) {
		//Wyrmgus start
//		Selected[0]->Player->Notify(NotifyYellow, Selected[0]->tilePos, "%s", _("Unit training queue is full"));
		ThisPlayer->Notify(NotifyYellow, Selected[0]->tilePos, "%s", _("Unit training queue is full"));
		//Wyrmgus end
	//Wyrmgus start
//	} else if (Selected[0]->Player->CheckLimits(type) >= 0 && !Selected[0]->Player->CheckUnitType(type)) {
	} else if (ThisPlayer->CheckLimits(type) >= 0 && !ThisPlayer->CheckUnitType(type)) {
	//Wyrmgus end
		//PlayerSubUnitType(player,type);
		//Wyrmgus start
//		SendCommandTrainUnit(*Selected[0], type, !(KeyModifiers & ModifierShift));
		SendCommandTrainUnit(*Selected[0], type, ThisPlayer->Index, !(KeyModifiers & ModifierShift));
		//Wyrmgus end
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
	//Wyrmgus start
//	} else if (Selected[0]->Player->CheckLimits(type) == -3) {
//		if (GameSounds.NotEnoughFood[Selected[0]->Player->Race].Sound) {
//			PlayGameSound(GameSounds.NotEnoughFood[Selected[0]->Player->Race].Sound, MaxSampleVolume);
//		}
	} else if (ThisPlayer->CheckLimits(type) == -3) {
		if (GameSounds.NotEnoughFood[ThisPlayer->Race].Sound) {
			PlayGameSound(GameSounds.NotEnoughFood[ThisPlayer->Race].Sound, MaxSampleVolume);
		}
	//Wyrmgus end
	}
	*/
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
	
	if (type.BoolFlag[RAIL_INDEX].value) {
		bool has_adjacent_rail = Selected[best_training_place]->HasAdjacentRailForUnitType(&type);
		if (!has_adjacent_rail) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[best_training_place]->tilePos, Selected[best_training_place]->MapLayer->ID, "%s", _("The unit requires railroads to be placed on"));
			PlayGameSound(GameSounds.PlacementError[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
			return;
		}
	}
	
	int unit_count = 1;
	if (KeyModifiers & ModifierShift) {
		unit_count = 5;
	}
	
	for (int i = 0; i < unit_count; ++i) {
		if (Selected[best_training_place]->CurrentAction() == UnitAction::Train && !EnableTrainingQueue) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[best_training_place]->tilePos, Selected[best_training_place]->MapLayer->ID, "%s", _("Unit training queue is full"));
			return;
		} else if (CPlayer::GetThisPlayer()->CheckLimits(type) >= 0 && !CPlayer::GetThisPlayer()->CheckUnitType(type, Selected[best_training_place]->Type->Stats[Selected[best_training_place]->Player->Index].GetUnitStock(&type) != 0)) {
			SendCommandTrainUnit(*Selected[best_training_place], type, CPlayer::GetThisPlayer()->Index, FlushCommands);
			UI.StatusLine.Clear();
			UI.StatusLine.ClearCosts();
		} else if (CPlayer::GetThisPlayer()->CheckLimits(type) == -3) {
			if (GameSounds.NotEnoughFood[CPlayer::GetThisPlayer()->Race].Sound) {
				PlayGameSound(GameSounds.NotEnoughFood[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
			}
			return;
		}
	}
	//Wyrmgus end
}

void CButtonPanel::DoClicked_UpgradeTo(int button)
{
	// FIXME: store pointer in button table!
	stratagus::unit_type &type = *stratagus::unit_type::get_all()[CurrentButtons[button].Value];
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[i]->Player->CheckLimits(type) != -6 && !Selected[i]->Player->CheckUnitType(type)) {
			if (Selected[i]->CurrentAction() != UnitAction::UpgradeTo) {
				SendCommandUpgradeTo(*Selected[i], type, !(KeyModifiers & ModifierShift));
				UI.StatusLine.Clear();
				UI.StatusLine.ClearCosts();
			}
		} else {
			break;
		}
	}
}

//Wyrmgus start
void CButtonPanel::DoClicked_ExperienceUpgradeTo(int button)
{
	// FIXME: store pointer in button table!
	stratagus::unit_type &type = *stratagus::unit_type::get_all()[CurrentButtons[button].Value];
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[0]->Player->GetUnitTotalCount(type) < Selected[0]->Player->Allow.Units[type.Slot] || Selected[0]->Player->CheckLimits(type) != -6) { //ugly way to make the checklimits message only appear when it should
			if (Selected[i]->CurrentAction() != UnitAction::UpgradeTo) {
				Selected[i]->Variable[LEVELUP_INDEX].Value -= 1;
				Selected[i]->Variable[LEVELUP_INDEX].Max = Selected[i]->Variable[LEVELUP_INDEX].Value;
				if (!IsNetworkGame() && Selected[i]->Character != nullptr) {	//save the unit-type experience upgrade for persistent characters
					if (Selected[i]->Character->get_unit_type()->Slot != type.Slot) {
						if (Selected[i]->Player->AiEnabled == false) {
							Selected[i]->Character->set_unit_type(stratagus::unit_type::get_all()[CurrentButtons[button].Value]);
							SaveHero(Selected[i]->Character);
							CAchievement::CheckAchievements();
						}
					}
				}
				SendCommandTransformInto(*Selected[i], type, !(KeyModifiers & ModifierShift));
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
//Wyrmgus end

void CButtonPanel::DoClicked_Research(int button)
{
	const int index = CurrentButtons[button].Value;
	//Wyrmgus start
	int upgrade_costs[MaxCosts];
	CPlayer::GetThisPlayer()->GetUpgradeCosts(CUpgrade::get_all()[index], upgrade_costs);
//	if (!Selected[0]->Player->CheckCosts(CUpgrade::get_all()[index]->Costs)) {
	if (!CPlayer::GetThisPlayer()->CheckCosts(upgrade_costs)) {
	//Wyrmgus end
		//PlayerSubCosts(player,Upgrades[i].Costs);
		//Wyrmgus start
//		SendCommandResearch(*Selected[0], *CUpgrade::get_all()[index], !(KeyModifiers & ModifierShift));
		SendCommandResearch(*Selected[0], *CUpgrade::get_all()[index], CPlayer::GetThisPlayer()->Index, !(KeyModifiers & ModifierShift));
		//Wyrmgus end
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
		//Wyrmgus start
		LastDrawnButtonPopup = nullptr;
		ButtonUnderCursor = -1;
		OldButtonUnderCursor = -1;
		UI.ButtonPanel.Update();
		//Wyrmgus end
	}
}

//Wyrmgus start
void CButtonPanel::DoClicked_LearnAbility(int button)
{
	const int index = CurrentButtons[button].Value;
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
	const int index = CurrentButtons[button].Value;
	SendCommandSetFaction(CPlayer::GetThisPlayer()->Index, stratagus::faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[index]->ID);
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	LastDrawnButtonPopup = nullptr;
	if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Quest(int button)
{
	const int index = CurrentButtons[button].Value;
	SendCommandQuest(*Selected[0], Selected[0]->Player->AvailableQuests[index]);
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	LastDrawnButtonPopup = nullptr;
	if (Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Buy(int button)
{
	int buy_costs[MaxCosts];
	memset(buy_costs, 0, sizeof(buy_costs));
	buy_costs[CopperCost] = UnitManager.GetSlotUnit(CurrentButtons[button].Value).GetPrice();
	if (!CPlayer::GetThisPlayer()->CheckCosts(buy_costs) && CPlayer::GetThisPlayer()->CheckLimits(*UnitManager.GetSlotUnit(CurrentButtons[button].Value).Type) >= 0) {
		SendCommandBuy(*Selected[0], &UnitManager.GetSlotUnit(CurrentButtons[button].Value), CPlayer::GetThisPlayer()->Index);
		ButtonUnderCursor = -1;
		OldButtonUnderCursor = -1;
		LastDrawnButtonPopup = nullptr;
		if (IsOnlySelected(*Selected[0])) {
			SelectedUnitChanged();
		}
	} else if (CPlayer::GetThisPlayer()->CheckLimits(*UnitManager.GetSlotUnit(CurrentButtons[button].Value).Type) == -3) {
		if (GameSounds.NotEnoughFood[CPlayer::GetThisPlayer()->Race].Sound) {
			PlayGameSound(GameSounds.NotEnoughFood[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
		}
	}
}

void CButtonPanel::DoClicked_ProduceResource(int button)
{
	const int resource = CurrentButtons[button].Value;
	if (resource != Selected[0]->GivesResource) {
		SendCommandProduceResource(*Selected[0], resource);
	} else if (!Selected[0]->Type->GivesResource) { //if resource production button was clicked when it was already active, then this means it should be toggled off; only do this if the building's type doesn't have a default produced resource, though, since in those cases it should always produce a resource
		SendCommandProduceResource(*Selected[0], 0);
	}
}

void CButtonPanel::DoClicked_SellResource(int button)
{
	const bool toggle_autosell = (KeyModifiers & ModifierControl) != 0;
	const int resource = CurrentButtons[button].Value;
	
	if (toggle_autosell && Selected[0]->Player == CPlayer::GetThisPlayer()) {
		SendCommandAutosellResource(CPlayer::GetThisPlayer()->Index, resource);
	} else {
		int sell_resource_costs[MaxCosts];
		memset(sell_resource_costs, 0, sizeof(sell_resource_costs));
		sell_resource_costs[resource] = 100;
		if (!CPlayer::GetThisPlayer()->CheckCosts(sell_resource_costs)) {
			SendCommandSellResource(*Selected[0], resource, CPlayer::GetThisPlayer()->Index);
		}
	}
}

void CButtonPanel::DoClicked_BuyResource(int button)
{
	const int resource = CurrentButtons[button].Value;
	int buy_resource_costs[MaxCosts];
	memset(buy_resource_costs, 0, sizeof(buy_resource_costs));
	buy_resource_costs[CopperCost] = Selected[0]->Player->GetEffectiveResourceBuyPrice(resource);
	if (!CPlayer::GetThisPlayer()->CheckCosts(buy_resource_costs)) {
		SendCommandBuyResource(*Selected[0], resource, CPlayer::GetThisPlayer()->Index);
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
			PlayUnitSound(*connection_destination, UnitVoiceGroup::Used);
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
	LuaCallback* callback = (LuaCallback*)(CurrentButtons[button].Payload);
	callback->pushPreamble();
	callback->pushInteger(UnitNumber(*Selected[0]));
	callback->run();
}

/**
**  Handle bottom button clicked.
**
**  @param button  Button that was clicked.
*/
void CButtonPanel::DoClicked(int button)
{
	Assert(0 <= button && button < (int)UI.ButtonPanel.Buttons.size());
	// no buttons
	if (CurrentButtons.empty()) {
		return;
	}
	if (IsButtonAllowed(*Selected[0], CurrentButtons[button]) == false) {
		return;
	}
	//
	//  Button not available.
	//  or Not Teamed
	//
	//Wyrmgus start
//	if (CurrentButtons[button].Pos == -1 || !ThisPlayer->IsTeamed(*Selected[0])) {
	if (CurrentButtons[button].Pos == -1 || (!CurrentButtons[button].AlwaysShow && !CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) && !CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->Player, CurrentButtons[button].Action)) || (!CurrentButtons[button].AlwaysShow && !CPlayer::GetThisPlayer()->IsTeamed(*Selected[0]) && CPlayer::GetThisPlayer()->HasBuildingAccess(*Selected[0]->Player, CurrentButtons[button].Action) && !IsNeutralUsableButtonAction(CurrentButtons[button].Action))) { //allow neutral units to be used (but only for training or as transporters)
	//Wyrmgus end
		return;
	}

	//Wyrmgus start
	if (!IsButtonUsable(*Selected[0], CurrentButtons[button])) {
		if (
			(CurrentButtons[button].Action == ButtonCmd::Research && UpgradeIdentAllowed(*CPlayer::GetThisPlayer(), CurrentButtons[button].ValueStr) == 'R')
			|| (CurrentButtons[button].Action == ButtonCmd::LearnAbility && Selected[0]->GetIndividualUpgrade(CUpgrade::get(CurrentButtons[button].ValueStr)) == CUpgrade::get(CurrentButtons[button].ValueStr)->MaxLimit)
		) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The upgrade has already been acquired"));
		} else if (CurrentButtons[button].Action == ButtonCmd::Buy && CPlayer::GetThisPlayer()->Heroes.size() >= PlayerHeroMax && CurrentButtons[button].Value != -1 && UnitManager.GetSlotUnit(CurrentButtons[button].Value).Character != nullptr) {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The hero limit has been reached"));
		} else {
			CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The requirements have not been fulfilled"));
		}
		PlayGameSound(GameSounds.PlacementError[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
		return;
	}

	if (GetButtonCooldown(*Selected[0], CurrentButtons[button]) > 0) {
		CPlayer::GetThisPlayer()->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer->ID, "%s", _("The cooldown is active"));
		PlayGameSound(GameSounds.PlacementError[CPlayer::GetThisPlayer()->Race].Sound, MaxSampleVolume);
		return;
	}
	//Wyrmgus end

	PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
	if (CurrentButtons[button].CommentSound.Sound) {
		PlayGameSound(CurrentButtons[button].CommentSound.Sound, MaxSampleVolume);
	}
	
	//  Handle action on button.
	switch (CurrentButtons[button].Action) {
		case ButtonCmd::Unload: { DoClicked_Unload(button); break; }
		case ButtonCmd::SpellCast: { DoClicked_SpellCast(button); break; }
		case ButtonCmd::Repair: { DoClicked_Repair(button); break; }
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
		case ButtonCmd::Return: { DoClicked_Return(); break; }
		case ButtonCmd::Stop: { DoClicked_Stop(); break; }
		case ButtonCmd::StandGround: { DoClicked_StandGround(); break; }
		case ButtonCmd::Button: { DoClicked_Button(button); break; }
		case ButtonCmd::Cancel: // Follow Next
		case ButtonCmd::CancelUpgrade: { DoClicked_CancelUpgrade(); break; }
		case ButtonCmd::CancelTrain: { DoClicked_CancelTrain(); break; }
		case ButtonCmd::CancelBuild: { DoClicked_CancelBuild(); break; }
		case ButtonCmd::Build: { DoClicked_Build(button); break; }
		case ButtonCmd::Train: { DoClicked_Train(button); break; }
		case ButtonCmd::UpgradeTo: { DoClicked_UpgradeTo(button); break; }
		case ButtonCmd::Research: { DoClicked_Research(button); break; }
		case ButtonCmd::CallbackAction: { DoClicked_CallbackAction(button); break; }
		//Wyrmgus start
		case ButtonCmd::LearnAbility: { DoClicked_LearnAbility(button); break; }
		case ButtonCmd::ExperienceUpgradeTo: { DoClicked_ExperienceUpgradeTo(button); break; }
		case ButtonCmd::Faction: { DoClicked_Faction(button); break; }
		case ButtonCmd::Quest: { DoClicked_Quest(button); break; }
		case ButtonCmd::Buy: { DoClicked_Buy(button); break; }
		case ButtonCmd::ProduceResource: { DoClicked_ProduceResource(button); break; }
		case ButtonCmd::SellResource: { DoClicked_SellResource(button); break; }
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
int CButtonPanel::DoKey(int key)
{
	SDL_keysym keysym;
	memset(&keysym, 0, sizeof(keysym));
	keysym.sym = (SDLKey)key;
	gcn::Key k = gcn::SDLInput::convertKeyCharacter(keysym);
	key = k.getValue();

	if (!CurrentButtons.empty()) {
		// This is required for action queues SHIFT+M should be `m'
		if (isascii(key) && isupper(key)) {
			key = tolower(key);
		}

		for (int i = 0; i < (int)UI.ButtonPanel.Buttons.size(); ++i) {
			//Wyrmgus start
//			if (CurrentButtons[i].Pos != -1 && key == CurrentButtons[i].Key) {
			if (CurrentButtons[i].Pos != -1 && key == CurrentButtons[i].GetKey()) {
			//Wyrmgus end
				UI.ButtonPanel.DoClicked(i);
				return 1;
			}
		}
	}
	return 0;
}
