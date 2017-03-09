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
//      (c) Copyright 1999-2015 by Lutz Sammer, Vladi Belperchinov-Shabanski,
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "ui.h"

#include "actions.h"
//Wyrmgus start
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "depend.h"
#include "font.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "guichan/key.h"
#include "guichan/sdl/sdlinput.h"
#include "interface.h"
#include "map.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "player.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
#include "sound.h"
#include "spells.h"
#include "translate.h"
#include "trigger.h"
#include "ui/popup.h"
#include "unit.h"
//Wyrmgus start
#include "unit_manager.h"
//Wyrmgus end
#include "unittype.h"
#include "upgrade.h"
#include "video.h"

#include <ctype.h>
#include <vector>
#include <sstream>

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/// Last drawn popup : used to speed up drawing
ButtonAction *LastDrawnButtonPopup;
/// for unit buttons sub-menus etc.
int CurrentButtonLevel;
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
int AddButton(int pos, int level, const std::string &icon_ident,
			  ButtonCmd action, const std::string &value, void* actionCb, const ButtonCheckFunc func,
			  const std::string &allow, const int key, const std::string &hint, const std::string &descr,
			  const std::string &sound, const std::string &cursor, const std::string &umask,
			  //Wyrmgus start
//			  const std::string &popup, bool alwaysShow)
			  const std::string &popup, bool alwaysShow, const std::string &mod_file)
			  //Wyrmgus end
{
	char buf[2048];
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
			case ButtonSpellCast:
				ba->Value = SpellTypeByIdent(value)->Slot;
#ifdef DEBUG
				if (ba->Value < 0) {
					DebugPrint("Spell %s does not exist?\n" _C_ value.c_str());
					Assert(ba->Value >= 0);
				}
#endif
				break;
			case ButtonTrain:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			case ButtonResearch:
				ba->Value = UpgradeIdByIdent(value);
				break;
			//Wyrmgus start
			case ButtonLearnAbility:
				ba->Value = UpgradeIdByIdent(value);
				break;
			case ButtonExperienceUpgradeTo:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			//Wyrmgus end
			case ButtonUpgradeTo:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			case ButtonBuild:
				ba->Value = UnitTypeIdByIdent(value);
				break;
			//Wyrmgus start
			case ButtonSellResource:
				ba->Value = GetResourceIdByName(value.c_str());
				break;
			case ButtonBuyResource:
				ba->Value = GetResourceIdByName(value.c_str());
				break;
			//Wyrmgus end
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
		strcpy_s(buf, sizeof(buf), umask.c_str());
	} else {
		sprintf(buf, ",%s,", umask.c_str());
	}
	ba->UnitMask = buf;
	//Wyrmgus start
	ba->Mod = mod_file;
	//Wyrmgus end
	UnitButtonTable.push_back(ba);
	// FIXME: check if already initited
	//Assert(ba->Icon.Icon != NULL);// just checks, that's why at the end
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

	CurrentButtonLevel = 0;
	LastDrawnButtonPopup = NULL;
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
	//Wyrmgus end
	
	unsigned int action = UnitActionNone;
	switch (button.Action) {
		case ButtonStop:
			action = UnitActionStill;
			break;
		case ButtonStandGround:
			action = UnitActionStandGround;
			break;
		case ButtonAttack:
			action = UnitActionAttack;
			break;
		case ButtonAttackGround:
			action = UnitActionAttackGround;
			break;
		case ButtonPatrol:
			action = UnitActionPatrol;
			break;
		case ButtonHarvest:
		case ButtonReturn:
			action = UnitActionResource;
			break;
		default:
			break;
	}
	// Simple case.
	if (action != UnitActionNone) {
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
		case ButtonMove:
			for (i = 0; i < Selected.size(); ++i) {
				int saction = Selected[i]->CurrentAction();
				if (saction != UnitActionMove &&
					saction != UnitActionBuild &&
					saction != UnitActionFollow &&
					//Wyrmgus start
					saction != UnitActionPickUp &&
					//Wyrmgus end
					saction != UnitActionDefend) {
					break;
				}
			}
			if (i == Selected.size()) {
				res |= IconSelected;
			}
			break;
		case ButtonSpellCast:
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
		case ButtonRepair:
			for (i = 0; i < Selected.size(); ++i) {
				if (Selected[i]->CurrentAction() != UnitActionRepair) {
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
								CUnitType *type)
{
	if (!condition) {
		return true;
	}

	//Wyrmgus start
//	if (condition->HasHint && button.Hint.empty()) {
	if (condition->HasHint && button.GetHint().empty()) {
	//Wyrmgus end
		return false;
	}

	if (condition->HasDescription && button.Description.empty()) {
		return false;
	}

	if (condition->HasDependencies && PrintDependencies(*ThisPlayer, button).empty()) {
		return false;
	}
	
	//Wyrmgus start
	if (condition->Class && type && type->Class == -1 && !(type->BoolFlag[ITEM_INDEX].value && type->ItemClass != -1)) {
		return false;
	}
	
	if (condition->Description && type && type->Description.empty()) {
		return false;
	}
	
	if (condition->Quote && type && type->Quote.empty() && !((button.Action == ButtonUnit || button.Action == ButtonBuy) && UnitManager.GetSlotUnit(button.Value).Unique && !UnitManager.GetSlotUnit(button.Value).Unique->Quote.empty()) && !((button.Action == ButtonUnit || button.Action == ButtonBuy) && UnitManager.GetSlotUnit(button.Value).Work != NULL && !UnitManager.GetSlotUnit(button.Value).Work->Quote.empty() && UnitManager.GetSlotUnit(button.Value).Elixir != NULL && !UnitManager.GetSlotUnit(button.Value).Elixir->Quote.empty())) {
		return false;
	}
	
	if (condition->Encyclopedia && type && type->Description.empty() && type->Background.empty() && type->Quote.empty() && (!type->BoolFlag[ITEM_INDEX].value || type->ItemClass == -1)) {
		return false;
	}
	
	if (condition->SettlementName && !(button.Action == ButtonUnit && !UnitManager.GetSlotUnit(button.Value).SettlementName.empty())) {
		return false;
	}
	
	if (condition->FactionUpgrade != CONDITION_TRUE) {
		if ((condition->FactionUpgrade == CONDITION_ONLY) ^ ((button.Action == ButtonResearch || button.Action == ButtonLearnAbility) && !strncmp(AllUpgrades[button.Value]->Ident.c_str(), "upgrade-faction-", 16))) {
			return false;
		}
	}
	
	if (condition->ResearchedUpgrade != CONDITION_TRUE) {
		if ((condition->ResearchedUpgrade == CONDITION_ONLY) ^ (((button.Action == ButtonResearch && UpgradeIdentAllowed(*ThisPlayer, button.ValueStr) == 'R') || (button.Action == ButtonLearnAbility && Selected[0]->IndividualUpgrades[button.Value])))) {
			return false;
		}
	}
	
	if (condition->RequirementsString != CONDITION_TRUE) {
		if ((condition->RequirementsString == CONDITION_ONLY) ^ ((button.Action == ButtonResearch || button.Action == ButtonLearnAbility || button.Action == ButtonTrain || button.Action == ButtonBuild || button.Action == ButtonUpgradeTo || button.Action == ButtonExperienceUpgradeTo || button.Action == ButtonBuy) && !IsButtonUsable(*Selected[0], button) && ((type && !type->RequirementsString.empty()) ||  ((button.Action == ButtonResearch || button.Action == ButtonLearnAbility) && !AllUpgrades[button.Value]->RequirementsString.empty())))) {
			return false;
		}
	}
	//Wyrmgus end

	if (condition->ButtonAction != -1 && button.Action != condition->ButtonAction) {
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
	if (condition->Variables && type && button.Action != ButtonUnit && button.Action != ButtonBuy) {
	//Wyrmgus end
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
				if ((condition->Variables[i] == CONDITION_ONLY) ^ type->Stats[ThisPlayer->Index].Variables[i].Enable) {
					return false;
				}
			}
		}
	//Wyrmgus start
	} else if (condition->Variables && (button.Action == ButtonUnit || button.Action == ButtonBuy)) {
		for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
			if (condition->Variables[i] != CONDITION_TRUE) {
//				if ((condition->Variables[i] == CONDITION_ONLY) ^ UnitManager.GetSlotUnit(button.Value).Variable[i].Enable) {
				CUnit &unit = UnitManager.GetSlotUnit(button.Value);
				if (unit.Type->BoolFlag[ITEM_INDEX].value && unit.Container != NULL && unit.Container->HasInventory()) {
					if (i == BASICDAMAGE_INDEX) {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.Container->GetItemVariableChange(&unit, i) != 0 || unit.Container->GetItemVariableChange(&unit, PIERCINGDAMAGE_INDEX) != 0)) {
							return false;
						}
					} else {
						if ((condition->Variables[i] == CONDITION_ONLY) ^ (unit.Container->GetItemVariableChange(&unit, i) != 0)) { //the former for some reason wasn't working with negative values
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
	if (button.Action == ButtonSpellCast) {
		if (condition->AutoCast != CONDITION_TRUE) {
			if ((condition->AutoCast == CONDITION_ONLY) ^ (SpellTypeTable[button.Value]->AutoCast != NULL)) {
				return false;
			}
		}
	}
		
	if (button.Action == ButtonUnit || button.Action == ButtonBuy) {
		CUnit &unit = UnitManager.GetSlotUnit(button.Value);
		if (unit.Type->BoolFlag[ITEM_INDEX].value && unit.Container != NULL && unit.Container->HasInventory()) {
			if (condition->Equipped != CONDITION_TRUE) {
				if ((condition->Equipped == CONDITION_ONLY) ^ unit.Container->IsItemEquipped(&unit)) {
					return false;
				}
			}
			if (condition->Equippable != CONDITION_TRUE) {
				if ((condition->Equippable == CONDITION_ONLY) ^ unit.Container->CanEquipItem(&unit)) {
					return false;
				}
			}
			if (condition->Consumable != CONDITION_TRUE) {
				if ((condition->Consumable == CONDITION_ONLY) ^ IsItemClassConsumable(unit.Type->ItemClass)) {
					return false;
				}
			}
			if (condition->Spell != CONDITION_TRUE) {
				if ((condition->Spell == CONDITION_ONLY) ^ (unit.Spell != NULL)) {
					return false;
				}
			}
			if (condition->Work != CONDITION_TRUE) {
				if ((condition->Work == CONDITION_ONLY) ^ (unit.Work != NULL)) {
					return false;
				}
			}
			if (condition->ReadWork != CONDITION_TRUE) {
				if ((condition->ReadWork == CONDITION_ONLY) ^ (unit.Work != NULL && unit.Container->IndividualUpgrades[unit.Work->ID])) {
					return false;
				}
			}
			if (condition->Elixir != CONDITION_TRUE) {
				if ((condition->Elixir == CONDITION_ONLY) ^ (unit.Elixir != NULL)) {
					return false;
				}
			}
			if (condition->ConsumedElixir != CONDITION_TRUE) {
				if ((condition->ConsumedElixir == CONDITION_ONLY) ^ (unit.Elixir != NULL && unit.Container->IndividualUpgrades[unit.Elixir->ID])) {
					return false;
				}
			}
			if (condition->CanUse != CONDITION_TRUE) {
				if ((condition->CanUse == CONDITION_ONLY) ^ (unit.Container->CanUseItem(&unit))) {
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
				if (unit.Type->BoolFlag[ITEM_INDEX].value && unit.Container != NULL && unit.Container->HasInventory()) {
					if ((condition->Regeneration == CONDITION_ONLY) ^ (unit.Container->GetItemVariableChange(&unit, HITPOINTBONUS_INDEX, true) != 0)) {
						return false;
					}
				} else {
					if ((condition->Regeneration == CONDITION_ONLY) ^ (unit.Variable[HP_INDEX].Increase != 0 || unit.Variable[HITPOINTBONUS_INDEX].Increase != 0)) {
						return false;
					}
				}
			}
		}
		
		if (condition->Opponent != CONDITION_TRUE) {
			if ((condition->Opponent == CONDITION_ONLY) ^ ThisPlayer->IsEnemy(unit)) {
				return false;
			}
		}
		if (!(button.Action == ButtonBuy && unit.Character) && condition->Neutral != CONDITION_TRUE) {
			if ((condition->Neutral == CONDITION_ONLY) ^ (!ThisPlayer->IsEnemy(unit) && !ThisPlayer->IsAllied(unit) && ThisPlayer != unit.Player && (unit.Container == NULL || (!ThisPlayer->IsEnemy(*unit.Container) && !ThisPlayer->IsAllied(*unit.Container) && ThisPlayer != unit.Container->Player)))) {
				return false;
			}
		}
		
		if (condition->Affixed != CONDITION_TRUE) {
			if ((condition->Affixed == CONDITION_ONLY) ^ (unit.Prefix != NULL || unit.Suffix != NULL)) {
				return false;
			}
		}
		if (condition->Unique != CONDITION_TRUE) {
			if ((condition->Unique == CONDITION_ONLY) ^ (unit.Unique || unit.Character != NULL)) {
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
			(button.Action != ButtonUnit && button.Action != ButtonBuy && CanShowPopupContent(content.Condition, button, UnitTypes[button.Value]))
			|| ((button.Action == ButtonUnit || button.Action == ButtonBuy) && CanShowPopupContent(content.Condition, button, UnitTypes[UnitManager.GetSlotUnit(button.Value).Type->Slot]))
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


#if 0 // Fixme: need to remove soon
void DrawPopupUnitInfo(const CUnitType *type,
					   int player_index, CFont *font, Uint32 backgroundColor,
					   int buttonX, int buttonY)
{

	const CGraphic *G;
	const CUnitStats *stats = &type->Stats[player_index];

	//detect max Height
	int popupHeight = 85;//
	if (type->CanAttack) {
		popupHeight += 30;
	}

	//detect max Width
	int popupWidth = GetPopupCostsWidth(font, stats->Costs);
	if (stats->Variables[DEMAND_INDEX].Value) {
		if (UI.Resources[FoodCost].IconWidth != -1) {
			popupWidth += (UI.Resources[FoodCost].IconWidth + 5);
		} else {
			G = UI.Resources[FoodCost].G;
			if (G) {
				popupWidth += (G->Width + 5);
			}
		}
		popupWidth += (font->Width(stats->Variables[DEMAND_INDEX].Value) + 5);
	}
	popupWidth += 10;
	popupWidth = std::max<int>(popupWidth, font->Width(type->Name) + 10);

	if (popupWidth < 120) {
		popupWidth = 120;
	}

	int start_x = std::min<int>(buttonX, Video.Width - 1 - popupWidth);
	int y = buttonY - popupHeight - 10;
	int x = start_x;
	CLabel label(font, "white", "red");

	// Background
	Video.FillTransRectangle(backgroundColor, x, y,
							 popupWidth, popupHeight, 128);
	Video.DrawRectangle(ColorWhite, x, y, popupWidth, popupHeight);

	// Name
	label.Draw(x + 5, y + 5, type->Name);
	Video.DrawHLine(ColorWhite, x, y + 15, popupWidth - 1);

	y += 20;

	// Costs
	x = DrawPopupCosts(x + 5, y, label,  stats->Costs);

	if (stats->Variables[DEMAND_INDEX].Value) {
		int y_offset = 0;
		G = UI.Resources[FoodCost].G;
		if (G) {
			int x_offset = UI.Resources[FoodCost].IconWidth;
			G->DrawFrameClip(UI.Resources[FoodCost].IconFrame, x, y);
			x += ((x_offset != -1 ? x_offset : G->Width) + 5);
			y_offset = G->Height;
			y_offset -= font->Height();
			y_offset /= 2;
		}
		label.Draw(x, y + y_offset, stats->Variables[DEMAND_INDEX].Value);
		//x += 5;
	}

	y += 20;//15;
	x = start_x;

	// Hit Points
	{
		std::ostringstream hitPoints;
		hitPoints << "Hit Points: " << type->Variable[HP_INDEX].Value;
		label.Draw(x + 5, y, hitPoints.str());
		y += 15;
	}

	if (type->CanAttack) {
		// Damage
		int min_damage = std::max<int>(1, type->Variable[PIERCINGDAMAGE_INDEX].Value / 2);
		int max_damage = type->Variable[PIERCINGDAMAGE_INDEX].Value +
						 type->Variable[BASICDAMAGE_INDEX].Value;
		std::ostringstream damage;
		damage << "Damage: " << min_damage << "-" << max_damage;
		label.Draw(x + 5, y, damage.str());
		y += 15;

		// Attack Range
		std::ostringstream attackRange;
		attackRange << "Attack Range: " << type->Variable[ATTACKRANGE_INDEX].Value;
		label.Draw(x + 5, y, attackRange.str());
		y += 15;
	}

	// Armor
	{
		std::ostringstream armor;
		armor << "Armor: " << type->Variable[ARMOR_INDEX].Value;
		label.Draw(x + 5, y, armor.str());
		y += 15;
	}

	if (type->Variable[RADAR_INDEX].Value) {
		// Radar Range
		std::ostringstream radarRange;
		radarRange << "Radar Range: " << type->Variable[RADAR_INDEX].Value;
		label.Draw(x + 5, y, radarRange.str());
	} else {
		// Sight Range
		std::ostringstream sightRange;
		sightRange << "Sight Range: " << type->Variable[SIGHTRANGE_INDEX].Value;
		label.Draw(x + 5, y, sightRange.str());
	}
	//y += 15;
}
#endif

static struct PopupDrawCache {
	int popupWidth;
	int popupHeight;
} popupCache;

/**
**  Draw popup
*/
void DrawPopup(const ButtonAction &button, const CUIButton &uibutton, int x, int y)
{
	CPopup *popup = PopupByIdent(button.Popup);
	bool useCache = false;

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
		case ButtonResearch:
			//Wyrmgus start
//			memcpy(Costs, AllUpgrades[button.Value]->Costs, sizeof(AllUpgrades[button.Value]->Costs));
			ThisPlayer->GetUpgradeCosts(AllUpgrades[button.Value], Costs);
			//Wyrmgus end
			break;
		case ButtonSpellCast:
			memcpy(Costs, SpellTypeTable[button.Value]->Costs, sizeof(SpellTypeTable[button.Value]->Costs));
			Costs[ManaResCost] = SpellTypeTable[button.Value]->ManaCost;
			break;
		case ButtonBuild:
		case ButtonTrain:
		case ButtonUpgradeTo:
			memcpy(Costs, UnitTypes[button.Value]->Stats[ThisPlayer->Index].Costs,
				   sizeof(UnitTypes[button.Value]->Stats[ThisPlayer->Index].Costs));
			Costs[FoodCost] = UnitTypes[button.Value]->Stats[ThisPlayer->Index].Variables[DEMAND_INDEX].Value;
			//Wyrmgus start
			if (button.Action == ButtonTrain && UnitTypes[button.Value]->TrainQuantity > 1) { //if more than one unit is trained in a batch, multiply the costs
				for (int i = 1; i < MaxCosts; ++i) {
					Costs[i] *= UnitTypes[button.Value]->TrainQuantity;
				}
				Costs[FoodCost] *= UnitTypes[button.Value]->TrainQuantity;
			}
			//Wyrmgus end
			break;
		//Wyrmgus start
		case ButtonBuy:
			Costs[FoodCost] = UnitManager.GetSlotUnit(button.Value).Type->Stats[ThisPlayer->Index].Variables[DEMAND_INDEX].Value;
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
	y = y - popupHeight - 10;
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
			(button.Action != ButtonUnit && button.Action != ButtonBuy && CanShowPopupContent(content.Condition, button, UnitTypes[button.Value]))
			|| ((button.Action == ButtonUnit || button.Action == ButtonBuy) && CanShowPopupContent(content.Condition, button, UnitTypes[UnitManager.GetSlotUnit(button.Value).Type->Slot]))
		) {
		//Wyrmgus end
			content.Draw(x + content.pos.x, y + content.pos.y, *popup, popupWidth, button, Costs);
		}
	}

#if 0 // Fixme: need to remove soon
	switch (button.Action) {
		case ButtonResearch: {
			CLabel label(font, "white", "red");
			int *Costs = AllUpgrades[button->Value]->Costs;
			popupWidth = GetPopupCostsS(font, Costs);
			//Wyrmgus start
//			popupWidth = std::max<int>(popupWidth, font->Width(button->Hint) + 10);
			popupWidth = std::max<int>(popupWidth, font->Width(button->GetHint()) + 10);
			//Wyrmgus end

			popupHeight = 40;

			start_x = std::min<int>(uibutton->X, Video.Width - 1 - popupWidth);

			y = uibutton->Y - popupHeight - 10;
			x = start_x;

			// Background
			Video.FillTransRectangle(backgroundColor, x, y,
									 popupWidth, popupHeight, 128);
			Video.DrawRectangle(ColorWhite, x, y, popupWidth, popupHeight);

			// Name
			//Wyrmgus start
//			label.Draw(x + 5, y + 5, button->Hint);
			label.Draw(x + 5, y + 5, button->GetHint());
			//Wyrmgus end
			Video.DrawHLine(ColorWhite, x, y + 15, popupWidth - 1);

			y += 20;
			x = start_x;
			DrawPopupCosts(x + 5, y, label, Costs);
		}
		break;
		case ButtonSpellCast: {
			CLabel label(font, "white", "red");
			// FIXME: hardcoded image!!!
			//Wyrmgus start
//			const int IconID = GoldCost;
			const int IconID = CopperCost;
			//Wyrmgus end
			//SetCosts(SpellTypeTable[button->Value]->ManaCost, 0, NULL);
			const CGraphic *G = UI.Resources[IconID].G;
			const SpellType *spell = SpellTypeTable[button->Value];

			if (spell->ManaCost) {
				popupHeight = 40;
				popupWidth = 10;
				if (UI.Resources[IconID].IconWidth != -1) {
					popupWidth += (UI.Resources[IconID].IconWidth + 5);
				} else {
					if (G) {
						popupWidth += (G->Width + 5);
					}
				}
				popupWidth += font->Width(spell->ManaCost);
				popupWidth = std::max<int>(popupWidth, font->Width(spell->Name) + 10);
			} else {
				//Wyrmgus start
//				popupWidth = font->Width(button->Hint) + 10;
				popupWidth = font->Width(button->GetHint()) + 10;
				//Wyrmgus end
				popupHeight = font_height + 10;
			}

			popupWidth = std::max<int>(popupWidth, 100);

			x = std::min<int>(uibutton->X, Video.Width - 1 - popupWidth);
			y = uibutton->Y - popupHeight - 10;

			// Background
			Video.FillTransRectangle(backgroundColor, x, y,
									 popupWidth, popupHeight, 128);
			Video.DrawRectangle(ColorWhite, x, y, popupWidth, popupHeight);

			if (spell->ManaCost) {
				int y_offset = 0;
				// Name
				label.Draw(x + 5, y + 5, spell->Name);
				Video.DrawHLine(ColorWhite, x, y + 15, popupWidth - 1);
				y += 20;
				if (G) {
					int x_offset =  UI.Resources[IconID].IconWidth;
					x += 5;
					// FIXME: hardcoded image!!!
					G->DrawFrameClip(3, x, y);
					x += ((x_offset != -1 ? x_offset : G->Width) + 5);
					y_offset = G->Height;
					y_offset -= font_height;
					y_offset /= 2;
				}
				label.Draw(x, y + y_offset, spell->ManaCost);
			} else {
				// Only Hint
				//Wyrmgus start
//				label.Draw(x + 5, y + (popupHeight - font_height) / 2, button->Hint);
				label.Draw(x + 5, y + (popupHeight - font_height) / 2, button->GetHint());
				//Wyrmgus end
			}
		}
		break;

		case ButtonBuild:
		case ButtonTrain:
		case ButtonUpgradeTo:
			DrawPopupUnitInfo(UnitTypes[button->Value],
							  ThisPlayer->Index, font, backgroundColor,
							  uibutton->X, uibutton->Y);
			break;


		default:
			//Wyrmgus start
//			popupWidth = font->Width(button->Hint) + 10;
			popupWidth = font->Width(button->GetHint()) + 10;
			//Wyrmgus end
			popupHeight = font_height + 10;//19;
			x = std::min<int>(uibutton->X, Video.Width - 1 - popupWidth);
			y = uibutton->Y - popupHeight - 10;

			// Background
			Video.FillTransRectangle(backgroundColor, x, y, popupWidth, popupHeight, 128);
			Video.DrawRectangle(ColorWhite, x, y, popupWidth, popupHeight);

			// Hint
			CLabel(font, "white", "red").Draw(x + 5,
											  //Wyrmgus start
//											  y + (popupHeight - font_height) / 2, button->Hint);
											  y + (popupHeight - font_height) / 2, button->GetHint());
											  //Wyrmgus end
			break;

	}
#endif
}

//Wyrmgus start
/**
**  Draw popup
*/
void DrawGenericPopup(std::string popup_text, int x, int y, std::string text_color, std::string highlight_color)
{
	const CFont &font = GetGameFont();
	
	int MaxWidth = std::max(256, Video.Width / 5);

	int i;
		
	//calculate content width
	int content_width = 0;
	if (MaxWidth) {
		content_width = std::min(font.getWidth(popup_text), MaxWidth);
	} else {
		std::string content_width_sub;
		i = 1;
		while (!(content_width_sub = GetLineFont(i++, popup_text, 0, &font)).empty()) {
			int line_width = font.getWidth(content_width_sub);
			int cost_symbol_pos = content_width_sub.find("COST_", 0);
			if (cost_symbol_pos != std::string::npos) {
				int res = std::stoi(content_width_sub.substr(cost_symbol_pos + 5, content_width_sub.find(" ", cost_symbol_pos) - (cost_symbol_pos + 5) + 1));
				line_width -= font.getWidth("COST_" + std::to_string((long long) res));
				line_width += UI.Resources[res].G->Width;
			}
			content_width = std::max(content_width, line_width);
		}
	}
	
	//calculate content height
	int content_height = 0;
	i = 1;
	while ((GetLineFont(i++, popup_text, MaxWidth, &font)).length()) {
		content_height += font.Height() + 2;
	}
	
	int popupWidth, popupHeight;

	int contentWidth = MARGIN_X;
	int contentHeight = 0;
	int maxContentWidth = 0;
	int maxContentHeight = 0;
	popupWidth = MARGIN_X;
	popupHeight = MARGIN_Y;
	PixelPos pos(0, 0);
	
	bool wrap = true;

	// Automatically write the calculated coordinates.
	pos.x = contentWidth + MARGIN_X;
	pos.y = popupHeight + MARGIN_Y;

	contentWidth += std::max(0, 2 * MARGIN_X + content_width);
	contentHeight = std::max(0, 2 * MARGIN_Y + content_height);
	maxContentHeight = std::max(contentHeight, maxContentHeight);
	if (wrap) {
		popupWidth += std::max(0, contentWidth - maxContentWidth);
		popupHeight += maxContentHeight;
		maxContentWidth = std::max(maxContentWidth, contentWidth);
		contentWidth = MARGIN_X;
		maxContentHeight = 0;
	}

	popupWidth += MARGIN_X;
	popupHeight += MARGIN_Y;
	
	popupWidth = std::min(popupWidth, MaxWidth);
	popupHeight = std::max(popupHeight, 0);

	x = std::min<int>(x, Video.Width - 1 - popupWidth);
	clamp<int>(&x, 0, Video.Width - 1);
	y = y - popupHeight - 10;
	clamp<int>(&y, 0, Video.Height - 1);

	// Background
	IntColor BackgroundColor = Video.MapRGBA(TheScreen->format, 28, 28, 28, 208);
	IntColor BorderColor = Video.MapRGBA(TheScreen->format, 93, 93, 93, 160);
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		Video.FillTransRectangle(BackgroundColor, x, y, popupWidth, popupHeight, BackgroundColor >> ASHIFT);
	} else
#endif
	{
		Video.FillTransRectangle(BackgroundColor, x, y, popupWidth, popupHeight, 208);
	}
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
						 ? std::min(MaxWidth, popupWidth - 2 * MARGIN_X)
						 : 0;
	while ((sub = GetLineFont(++i, popup_text, width, &font)).length()) {
		if (sub.find("LINE", 0) != std::string::npos) {
			Video.FillRectangle(BorderColor, x - MARGIN_X + 1 - MARGIN_X,
								y_off, popupWidth - 2, 1);
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
			UI.Resources[res].G->DrawFrameClip(UI.Resources[res].IconFrame, x + x_offset, y + ((font.getHeight() - UI.Resources[res].G->Height) / 2), true);
			x_offset += UI.Resources[res].G->Width;
			label.Draw(x + x_offset, y_off, sub_second);
		} else {
			label.Draw(x, y_off, sub);
		}
		y_off += font.Height() + 2;
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
			Selected[0]->Player != ThisPlayer
			&& !ThisPlayer->IsTeamed(*Selected[0])
			&& ThisPlayer->HasBuildingAccess(*Selected[0]->Player)
			&& buttons[i].Action != ButtonTrain && buttons[i].Action != ButtonCancelTrain && buttons[i].Action != ButtonBuy && buttons[i].Action != ButtonSellResource && buttons[i].Action != ButtonBuyResource
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
			} else if (buttons[i].Action == ButtonSpellCast
					   && (*Selected[j]).SpellCoolDownTimers[SpellTypeTable[buttons[i].Value]->Slot]) {
				Assert(SpellTypeTable[buttons[i].Value]->CoolDown > 0);
				cooldownSpell = true;
				maxCooldown = std::max(maxCooldown, (*Selected[j]).SpellCoolDownTimers[SpellTypeTable[buttons[i].Value]->Slot]);
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
		CIcon *button_icon = buttons[i].Icon.Icon;
			
		// if there is a single unit selected, show the icon of its weapon/shield/boots/arrows equipped for the appropriate buttons
		if (buttons[i].Icon.Name.empty() && buttons[i].Action == ButtonAttack && Selected[0]->Type->CanTransport() && Selected[0]->Type->BoolFlag[ATTACKFROMTRANSPORTER_INDEX].value && Selected[0]->Type->CanAttack && Selected[0]->BoardCount > 0 && Selected[0]->UnitInside != NULL && Selected[0]->UnitInside->GetButtonIcon(buttons[i].Action) != NULL) {
			button_icon = Selected[0]->UnitInside->GetButtonIcon(buttons[i].Action);
		} else if (buttons[i].Icon.Name.empty() && Selected[0]->GetButtonIcon(buttons[i].Action) != NULL) {
			button_icon = Selected[0]->GetButtonIcon(buttons[i].Action);
		} else if (buttons[i].Action == ButtonExperienceUpgradeTo && Selected[0]->Type->VarInfo[Selected[0]->Variation] && UnitTypes[buttons[i].Value]->GetVariation(Selected[0]->Type->VarInfo[Selected[0]->Variation]->VariationId) != NULL && !UnitTypes[buttons[i].Value]->GetVariation(Selected[0]->Type->VarInfo[Selected[0]->Variation]->VariationId)->Icon.Name.empty()) {
			button_icon = UnitTypes[buttons[i].Value]->GetVariation(Selected[0]->Type->VarInfo[Selected[0]->Variation]->VariationId)->Icon.Icon;
		} else if ((buttons[i].Action == ButtonTrain || buttons[i].Action == ButtonBuild || buttons[i].Action == ButtonUpgradeTo || buttons[i].Action == ButtonExperienceUpgradeTo) && buttons[i].Icon.Name.empty() && UnitTypes[buttons[i].Value]->GetDefaultVariation(*ThisPlayer) != NULL && !UnitTypes[buttons[i].Value]->GetDefaultVariation(*ThisPlayer)->Icon.Name.empty()) {
			button_icon = UnitTypes[buttons[i].Value]->GetDefaultVariation(*ThisPlayer)->Icon.Icon;
		} else if ((buttons[i].Action == ButtonTrain || buttons[i].Action == ButtonBuild || buttons[i].Action == ButtonUpgradeTo || buttons[i].Action == ButtonExperienceUpgradeTo) && buttons[i].Icon.Name.empty() && !UnitTypes[buttons[i].Value]->Icon.Name.empty()) {
			button_icon = UnitTypes[buttons[i].Value]->Icon.Icon;
		} else if (buttons[i].Action == ButtonBuy) {
			button_icon = UnitManager.GetSlotUnit(buttons[i].Value).GetIcon().Icon;
		}
		//Wyrmgus end
		
		if (cooldownSpell) {
			//Wyrmgus start
//			buttons[i].Icon.Icon->DrawCooldownSpellIcon(pos,
			button_icon->DrawCooldownSpellIcon(pos,
			//Wyrmgus end
														maxCooldown * 100 / SpellTypeTable[buttons[i].Value]->CoolDown);
		} else if (gray) {
			//Wyrmgus start
//			buttons[i].Icon.Icon->DrawGrayscaleIcon(pos);
			button_icon->DrawGrayscaleIcon(pos);
			//Wyrmgus end
		} else {
			int player = -1;
			//Wyrmgus start
			int hair_color = 0;
			//Wyrmgus end
			if (Selected.empty() == false && Selected[0]->IsAlive()) {
				//Wyrmgus start
//				player = Selected[0]->RescuedFrom ? Selected[0]->RescuedFrom->Index : Selected[0]->Player->Index;
				player = Selected[0]->GetDisplayPlayer();

				//if is accessing a building of another player, set color to that of the person player (i.e. for training buttons)
				if (ThisPlayer->HasBuildingAccess(Players[player])) {
					player = ThisPlayer->Index;
				}
				
				if (buttons[i].Action == ButtonExperienceUpgradeTo || buttons[i].Action == ButtonUpgradeTo || buttons[i].Action == ButtonPatrol || buttons[i].Action == ButtonReturn) {
					hair_color = Selected[0]->GetHairColor();
				} else if (buttons[i].Action == ButtonTrain || buttons[i].Action == ButtonBuild) {
					hair_color = UnitTypes[buttons[i].Value]->GetDefaultHairColor(Players[player]);
				}
				//Wyrmgus end
			}
			//Wyrmgus start			
			/*
			buttons[i].Icon.Icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
											   GetButtonStatus(buttons[i], ButtonUnderCursor),
											   pos, buf, player);
			*/
			
			if (IsButtonUsable(*Selected[0], buttons[i])) {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(buttons[i], ButtonUnderCursor),
												   pos, buf, player, hair_color);
			} else if (buttons[i].Action == ButtonResearch && UpgradeIdentAllowed(*ThisPlayer, buttons[i].ValueStr) == 'R') { //draw researched technologies grayed
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(buttons[i], ButtonUnderCursor),
												   pos, buf, player, hair_color, false, true);
			} else {
				button_icon->DrawUnitIcon(*UI.ButtonPanel.Buttons[i].Style,
												   GetButtonStatus(buttons[i], ButtonUnderCursor),
												   pos, buf, player, hair_color, true);
			}
			
			//draw a number over the button in special circumstances
			if (
				(buttons[i].Action == ButtonTrain && Selected[0]->Type->Stats[Selected[0]->Player->Index].UnitStock[buttons[i].Value] != 0)
				|| buttons[i].Action == ButtonSellResource || buttons[i].Action == ButtonBuyResource
			) {
				std::string number_string;
				if (buttons[i].Action == ButtonTrain && Selected[0]->Type->Stats[Selected[0]->Player->Index].UnitStock[buttons[i].Value] != 0) { //draw the quantity in stock for unit "training" cases which have it
					number_string = std::to_string((long long) Selected[0]->GetUnitStock(buttons[i].Value)) + "/" + std::to_string((long long) Selected[0]->Type->Stats[Selected[0]->Player->Index].UnitStock[buttons[i].Value]);
				} else if (buttons[i].Action == ButtonSellResource) {
					number_string = std::to_string((long long) Selected[0]->Player->Prices[buttons[i].Value] * Selected[0]->Variable[TRADEEFFICIENCY_INDEX].Value / 100);
				} else if (buttons[i].Action == ButtonBuyResource) {
					number_string = std::to_string((long long) 100 * 100 / Selected[0]->Player->Prices[buttons[i].Value] * Selected[0]->Variable[TRADEEFFICIENCY_INDEX].Value / 100);
				}
				std::string oldnc;
				std::string oldrc;
				GetDefaultTextColors(oldnc, oldrc);
				CLabel label(GetGameFont(), oldnc, oldrc);

				label.Draw(pos.x + 46 - GetGameFont().Width(number_string), pos.y + 0, number_string);
			}
			//Wyrmgus end
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
				DrawPopup(buttons[i], UI.ButtonPanel.Buttons[i], UI.ButtonPanel.Buttons[i].X,
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
			if (!uins->Boarded || j >= UI.TransportingButtons.size() || (Selected[0]->Player != ThisPlayer && uins->Player != ThisPlayer)) {
				continue;
			}
			if (static_cast<size_t>(ButtonUnderCursor) == j) {
				std::string text_color;
				if (uins->Unique || uins->Character != NULL) {
					text_color = "fire";
				} else if (uins->Prefix != NULL || uins->Suffix != NULL) {
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
		case ButtonBuild:
		case ButtonTrain:
		case ButtonUpgradeTo: {
			// FIXME: store pointer in button table!
			//Wyrmgus start
//			const CUnitStats &stats = UnitTypes[button.Value]->Stats[ThisPlayer->Index];
//			UI.StatusLine.SetCosts(0, stats.Variables[DEMAND_INDEX].Value, stats.Costs);
			int modified_costs[MaxCosts];
			for (int i = 1; i < MaxCosts; ++i) {
				modified_costs[i] = UnitTypes[button.Value]->Stats[ThisPlayer->Index].Costs[i];
				if (UnitTypes[button.Value]->TrainQuantity) 
				{
					modified_costs[i] *= UnitTypes[button.Value]->TrainQuantity;
				}
			}
			UI.StatusLine.SetCosts(0, UnitTypes[button.Value]->Stats[ThisPlayer->Index].Variables[DEMAND_INDEX].Value * (UnitTypes[button.Value]->TrainQuantity ? UnitTypes[button.Value]->TrainQuantity : 1), modified_costs);
			//Wyrmgus end
			break;
		}
		case ButtonResearch:
			UI.StatusLine.SetCosts(0, 0, AllUpgrades[button.Value]->Costs);
			break;
		case ButtonSpellCast:
			UI.StatusLine.SetCosts(SpellTypeTable[button.Value]->ManaCost, 0, SpellTypeTable[button.Value]->Costs);
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
		case ButtonStop:
		case ButtonStandGround:
		case ButtonButton:
		case ButtonMove:
		case ButtonCallbackAction:
		//Wyrmgus start
		case ButtonRallyPoint:
		case ButtonUnit:
		case ButtonEditorUnit:
		case ButtonSellResource:
		case ButtonBuyResource:
		//Wyrmgus end
			res = true;
			break;
		case ButtonRepair:
			res = unit.Type->RepairRange > 0;
			break;
		case ButtonPatrol:
			res = unit.CanMove();
			break;
		case ButtonHarvest:
			if (!unit.CurrentResource
				|| !(unit.ResourcesHeld > 0 && !unit.Type->ResInfo[unit.CurrentResource]->LoseResources)
				|| (unit.ResourcesHeld != unit.Type->ResInfo[unit.CurrentResource]->ResourceCapacity
					&& unit.Type->ResInfo[unit.CurrentResource]->LoseResources)) {
				res = true;
			}
			break;
		case ButtonReturn:
			if (!(!unit.CurrentResource
				  || !(unit.ResourcesHeld > 0 && !unit.Type->ResInfo[unit.CurrentResource]->LoseResources)
				  || (unit.ResourcesHeld != unit.Type->ResInfo[unit.CurrentResource]->ResourceCapacity
					  && unit.Type->ResInfo[unit.CurrentResource]->LoseResources))) {
				res = true;
			}
			break;
		case ButtonAttack:
			res = ButtonCheckAttack(unit, buttonaction);
			break;
		case ButtonAttackGround:
			if (unit.Type->BoolFlag[GROUNDATTACK_INDEX].value) {
				res = true;
			}
			break;
		case ButtonTrain:
			// Check if building queue is enabled
			if (!EnableTrainingQueue && unit.CurrentAction() == UnitActionTrain) {
				break;
			}
			if (unit.Player->Index == PlayerNumNeutral && !unit.CanHireMercenary(UnitTypes[buttonaction.Value])) {
				break;
			}
		// FALL THROUGH
		case ButtonUpgradeTo:
		case ButtonResearch:
		case ButtonBuild:
			//Wyrmgus start
//			res = CheckDependByIdent(*unit.Player, buttonaction.ValueStr);
			res = CheckDependByIdent(*unit.Player, buttonaction.ValueStr, false, true);
			//Wyrmgus end
			if (res && !strncmp(buttonaction.ValueStr.c_str(), "upgrade-", 8)) {
				//Wyrmgus start
//				res = UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'A';
				res = UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'A' || UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'R';
				if (res && !strncmp(buttonaction.ValueStr.c_str(), "upgrade-faction-", 16)) {
					CFaction *upgrade_faction = PlayerRaces.GetFaction(-1, FindAndReplaceString(buttonaction.ValueStr, "upgrade-faction-", ""));
					if (upgrade_faction) {
						res = unit.Player->CanFoundFaction(upgrade_faction, true);
					}
				}
				//Wyrmgus end
			}
			break;
		//Wyrmgus start
		case ButtonExperienceUpgradeTo:
			res = CheckDependByIdent(*unit.Player, buttonaction.ValueStr, true, true);
			if (res && !strncmp(buttonaction.ValueStr.c_str(), "upgrade-", 8)) {
				res = UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'A' && unit.Variable[LEVELUP_INDEX].Value >= 1;
			}
			if (res && unit.Character != NULL) {
				res = !unit.Character->ForbiddenUpgrades[buttonaction.Value];
			}
			break;
		case ButtonLearnAbility:
			res = unit.CanLearnAbility(CUpgrade::Get(buttonaction.ValueStr)) && (unit.Variable[LEVELUP_INDEX].Value >= 1 || !CUpgrade::Get(buttonaction.ValueStr)->Ability);
			break;
		//Wyrmgus end
		case ButtonSpellCast:
			//Wyrmgus start
//			res = SpellIsAvailable(*unit.Player, buttonaction.Value);
			res = SpellIsAvailable(unit, buttonaction.Value);
			//Wyrmgus end
			break;
		case ButtonUnload:
			res = (Selected[0]->Type->CanTransport() && Selected[0]->BoardCount);
			break;
		case ButtonCancel:
			res = true;
			break;
		case ButtonCancelUpgrade:
			//Wyrmgus start
//			res = unit.CurrentAction() == UnitActionUpgradeTo
//				  || unit.CurrentAction() == UnitActionResearch;
			//don't show the cancel button for a quick moment if the time cost is 0
			res = (unit.CurrentAction() == UnitActionUpgradeTo && static_cast<COrder_UpgradeTo *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->Index].Costs[TimeCost] > 0)
				|| (unit.CurrentAction() == UnitActionResearch && static_cast<COrder_Research *>(unit.CurrentOrder())->GetUpgrade().Costs[TimeCost] > 0);
			//Wyrmgus end
			break;
		case ButtonCancelTrain:
			//Wyrmgus start
//			res = unit.CurrentAction() == UnitActionTrain;
			res = unit.CurrentAction() == UnitActionTrain && static_cast<COrder_Train *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->Index].Costs[TimeCost] > 0; //don't show the cancel button for a quick moment if the time cost is 0
			//Wyrmgus end
			break;
		case ButtonCancelBuild:
			res = unit.CurrentAction() == UnitActionBuilt;
			break;
		//Wyrmgus start
		case ButtonQuest:
			res = buttonaction.Value < (int) unit.Player->AvailableQuests.size() && unit.Player->CanAcceptQuest(unit.Player->AvailableQuests[buttonaction.Value]);
			break;
		case ButtonBuy:
			res = (buttonaction.Value != -1) && (&UnitManager.GetSlotUnit(buttonaction.Value) != NULL);
			if (res && UnitManager.GetSlotUnit(buttonaction.Value).Character != NULL) {
				res = ThisPlayer->Heroes.size() < PlayerHeroMax;
			}
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
**  Check if the button is usable for the unit.
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
		case ButtonStop:
		case ButtonStandGround:
		case ButtonButton:
		case ButtonMove:
		case ButtonCallbackAction:
		case ButtonRallyPoint:
		case ButtonUnit:
		case ButtonEditorUnit:
		case ButtonRepair:
		case ButtonPatrol:
		case ButtonHarvest:
		case ButtonReturn:
		case ButtonAttack:
		case ButtonAttackGround:
			res = true;
			break;
		case ButtonTrain:
		case ButtonUpgradeTo:
		case ButtonResearch:
		case ButtonBuild:
			res = CheckDependByIdent(*unit.Player, buttonaction.ValueStr, false, false);
			if (res && !strncmp(buttonaction.ValueStr.c_str(), "upgrade-", 8)) {
				res = UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'A';
				if (res && !strncmp(buttonaction.ValueStr.c_str(), "upgrade-faction-", 16)) {
					CFaction *upgrade_faction = PlayerRaces.GetFaction(-1, FindAndReplaceString(buttonaction.ValueStr, "upgrade-faction-", ""));
					if (upgrade_faction) {
						res = unit.Player->CanFoundFaction(upgrade_faction, false);
					}
				}
			}
			break;
		case ButtonExperienceUpgradeTo:
			res = CheckDependByIdent(*unit.Player, buttonaction.ValueStr, true, false);
			break;
		case ButtonLearnAbility:
			res = unit.CanLearnAbility(CUpgrade::Get(buttonaction.ValueStr));
			break;
		case ButtonSpellCast:
			res = SpellIsAvailable(unit, buttonaction.Value);
			break;
		case ButtonUnload:
		case ButtonCancel:
		case ButtonCancelUpgrade:
		case ButtonCancelTrain:
		case ButtonCancelBuild:
		case ButtonQuest:
		case ButtonBuy:
		case ButtonSellResource:
		case ButtonBuyResource:
			res = true;
			break;
	}

	return res;
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
	char individual_unit_ident[36][128]; // the 36 there is the max selectable quantity; not nice to hardcode it like this, should be changed in the future
	//Wyrmgus end

	sprintf(unit_ident, ",%s-group,", PlayerRaces.Name[ThisPlayer->Race].c_str());
	
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
	if (unit.CurrentAction() == UnitActionBuilt) {
		// Trick 17 to get the cancel-build button
		strcpy_s(unit_ident, sizeof(unit_ident), ",cancel-build,");
	} else if (unit.CurrentAction() == UnitActionUpgradeTo) {
		// Trick 17 to get the cancel-upgrade button
		strcpy_s(unit_ident, sizeof(unit_ident), ",cancel-upgrade,");
	} else if (unit.CurrentAction() == UnitActionResearch) {
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
		if (buttonaction.AlwaysShow && !allow && buttonaction.Action == ButtonResearch
			&& UpgradeIdentAllowed(*unit.Player, buttonaction.ValueStr) == 'R') {
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
	if (Selected.empty()) {
		CurrentButtons.clear();
		return;
	}

	CUnit &unit = *Selected[0];
	// foreign unit
	//Wyrmgus start
//	if (unit.Player != ThisPlayer && !ThisPlayer->IsTeamed(unit)) {
	if (unit.Player != ThisPlayer && !ThisPlayer->IsTeamed(unit) && !ThisPlayer->HasBuildingAccess(*unit.Player)) {
	//Wyrmgus end
		CurrentButtons.clear();
		return;
	}
	
	//Wyrmgus start
	//update the sold item buttons
	unsigned int sold_unit_count = 0;
	for (int i = 0; i < (int) UnitButtonTable.size(); ++i) {
		if (UnitButtonTable[i]->Action != ButtonBuy) {
			continue;
		}
		char unit_ident[128];
		sprintf(unit_ident, ",%s,", unit.Type->Ident.c_str());
		if (UnitButtonTable[i]->UnitMask[0] != '*' && !strstr(UnitButtonTable[i]->UnitMask.c_str(), unit_ident)) {
			continue;
		}
		
		if (sold_unit_count >= unit.SoldUnits.size()) {
			UnitButtonTable[i]->Value = -1;
		} else {
			UnitButtonTable[i]->Value = UnitNumber(*unit.SoldUnits[sold_unit_count]);
			if (unit.SoldUnits[sold_unit_count]->Character != NULL) {
				UnitButtonTable[i]->Hint = "Recruit " + unit.SoldUnits[sold_unit_count]->GetName();
			} else {
				UnitButtonTable[i]->Hint = "Buy " + unit.SoldUnits[sold_unit_count]->GetName();
			}
		}
		sold_unit_count += 1;
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
	CursorState = CursorStateSelect;
	if (CurrentButtons[button].ButtonCursor.length() && CursorByIdent(CurrentButtons[button].ButtonCursor)) {
		GameCursor = CursorByIdent(CurrentButtons[button].ButtonCursor);
		CustomCursor = CurrentButtons[button].ButtonCursor;
	} else {
		GameCursor = UI.YellowHair.Cursor;
	}
	CursorAction = CurrentButtons[button].Action;
	CursorValue = CurrentButtons[button].Value;
	CurrentButtonLevel = 9; // level 9 is cancel-only
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
	if ((Selected.size() == 1 && Selected[0]->CurrentAction() == UnitActionStill
		//Wyrmgus start
//		 && Selected[0]->Type->UnitType == UnitTypeNaval && Map.Field(Selected[0]->tilePos)->CoastOnMap())
		 && Selected[0]->Type->UnitType == UnitTypeNaval && Map.Field(Selected[0]->tilePos, Selected[0]->MapLayer)->CoastOnMap())
		//Wyrmgus end
		|| !Selected[0]->CanMove()) {
		//Wyrmgus start
//		SendCommandUnload(*Selected[0], Selected[0]->tilePos, NoUnitP, flush);
		SendCommandUnload(*Selected[0], Selected[0]->tilePos, NoUnitP, flush, Selected[0]->MapLayer);
		//Wyrmgus end
		return ;
	}
	DoClicked_SelectTarget(button);
}

void CButtonPanel::DoClicked_SpellCast(int button)
{
	const int spellId = CurrentButtons[button].Value;
	if (KeyModifiers & ModifierControl) {
		int autocast = 0;

		if (!SpellTypeTable[spellId]->AutoCast) {
			PlayGameSound(GameSounds.PlacementError[ThisPlayer->Race].Sound, MaxSampleVolume);
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
	if (SpellTypeTable[spellId]->IsCasterOnly()) {
		const int flush = !(KeyModifiers & ModifierShift);

		for (size_t i = 0; i != Selected.size(); ++i) {
			CUnit &unit = *Selected[i];
			// CursorValue here holds the spell type id
			//Wyrmgus start
//			SendCommandSpellCast(unit, unit.tilePos, &unit, spellId, flush);
			SendCommandSpellCast(unit, unit.tilePos, &unit, spellId, flush, unit.MapLayer);
			//Wyrmgus end
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
	CurrentButtonLevel = CurrentButtons[button].Value;
	LastDrawnButtonPopup = NULL;
	UI.ButtonPanel.Update();
}

void CButtonPanel::DoClicked_CancelUpgrade()
{
	if (Selected.size() == 1) {
		switch (Selected[0]->CurrentAction()) {
			case UnitActionUpgradeTo:
				SendCommandCancelUpgradeTo(*Selected[0]);
				break;
			case UnitActionResearch:
				SendCommandCancelResearch(*Selected[0]);
				break;
			default:
				break;
		}
	}
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	CurrentButtonLevel = 0;
	UI.ButtonPanel.Update();
	GameCursor = UI.Point.Cursor;
	CursorBuilding = NULL;
	CursorState = CursorStatePoint;
}

void CButtonPanel::DoClicked_CancelTrain()
{
	Assert(Selected[0]->CurrentAction() == UnitActionTrain);
	SendCommandCancelTraining(*Selected[0], -1, NULL);
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
}

void CButtonPanel::DoClicked_CancelBuild()
{
	// FIXME: johns is this not sure, only building should have this?
	Assert(Selected[0]->CurrentAction() == UnitActionBuilt);
	if (Selected.size() == 1) {
		SendCommandDismiss(*Selected[0]);
	}
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
}

void CButtonPanel::DoClicked_Build(int button)
{
	// FIXME: store pointer in button table!
	CUnitType &type = *UnitTypes[CurrentButtons[button].Value];
	if (!Selected[0]->Player->CheckUnitType(type)) {
		UI.StatusLine.Set(_("Select Location"));
		UI.StatusLine.ClearCosts();
		CursorBuilding = &type;
		// FIXME: check is this =9 necessary?
		CurrentButtonLevel = 9; // level 9 is cancel-only
		UI.ButtonPanel.Update();
	}
}

void CButtonPanel::DoClicked_Train(int button)
{
	// FIXME: store pointer in button table!
	CUnitType &type = *UnitTypes[CurrentButtons[button].Value];
	// FIXME: Johns: I want to place commands in queue, even if not
	// FIXME:        enough resources are available.
	// FIXME: training queue full check is not correct for network.
	// FIXME: this can be correct written, with a little more code.
	//Wyrmgus start
	/*
	if (Selected[0]->CurrentAction() == UnitActionTrain && !EnableTrainingQueue) {
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
				if (Selected[i]->Orders[j]->Action == UnitActionTrain) {
					selected_queue += 1;
				}
			}
			if (selected_queue < lowest_queue) {
				lowest_queue = selected_queue;
				best_training_place = i;
			}
		}
	}
	
	if (Selected[best_training_place]->CurrentAction() == UnitActionTrain && !EnableTrainingQueue) {
		ThisPlayer->Notify(NotifyYellow, Selected[best_training_place]->tilePos, Selected[best_training_place]->MapLayer, "%s", _("Unit training queue is full"));
	} else if (ThisPlayer->CheckLimits(type) >= 0 && !ThisPlayer->CheckUnitType(type)) {
		SendCommandTrainUnit(*Selected[best_training_place], type, ThisPlayer->Index, !(KeyModifiers & ModifierShift));
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
	} else if (ThisPlayer->CheckLimits(type) == -3) {
		if (GameSounds.NotEnoughFood[ThisPlayer->Race].Sound) {
			PlayGameSound(GameSounds.NotEnoughFood[ThisPlayer->Race].Sound, MaxSampleVolume);
		}
	}
	//Wyrmgus end
}

void CButtonPanel::DoClicked_UpgradeTo(int button)
{
	// FIXME: store pointer in button table!
	CUnitType &type = *UnitTypes[CurrentButtons[button].Value];
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[0]->Player->CheckLimits(type) != -6 && !Selected[i]->Player->CheckUnitType(type)) {
			if (Selected[i]->CurrentAction() != UnitActionUpgradeTo) {
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
	CUnitType &type = *UnitTypes[CurrentButtons[button].Value];
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (Selected[0]->Player->GetUnitTotalCount(type) < Selected[0]->Player->Allow.Units[type.Slot] || Selected[0]->Player->CheckLimits(type) != -6) { //ugly way to make the checklimits message only appear when it should
			if (Selected[i]->CurrentAction() != UnitActionUpgradeTo) {
				Selected[i]->Variable[LEVELUP_INDEX].Value -= 1;
				Selected[i]->Variable[LEVELUP_INDEX].Max = Selected[i]->Variable[LEVELUP_INDEX].Value;
				if (!IsNetworkGame() && Selected[i]->Character != NULL) {	//save the unit-type experience upgrade for persistent characters
					if (Selected[i]->Character->Type->Slot != type.Slot) {
						if (Selected[i]->Player->AiEnabled == false) {
							Selected[i]->Character->Type = UnitTypes[CurrentButtons[button].Value];
							SaveHero(Selected[i]->Character);
							CheckAchievements();
						}
						if (GrandStrategy) { //also update the corresponding grand strategy hero, if in grand strategy mode
							CGrandStrategyHero *hero = GrandStrategyGame.GetHero(Selected[i]->Character->GetFullName());
							if (hero) {
								hero->SetType(CurrentButtons[button].Value);
							}
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
	
	if (Selected[0]->Variable[LEVELUP_INDEX].Value == 0) {
		CurrentButtonLevel = 0;
		LastDrawnButtonPopup = NULL;
		UI.ButtonPanel.Update();
	}
}
//Wyrmgus end

void CButtonPanel::DoClicked_Research(int button)
{
	const int index = CurrentButtons[button].Value;
	//Wyrmgus start
	int upgrade_costs[MaxCosts];
	Selected[0]->Player->GetUpgradeCosts(AllUpgrades[index], upgrade_costs);
//	if (!Selected[0]->Player->CheckCosts(AllUpgrades[index]->Costs)) {
	if (!Selected[0]->Player->CheckCosts(upgrade_costs)) {
	//Wyrmgus end
		//PlayerSubCosts(player,Upgrades[i].Costs);
		SendCommandResearch(*Selected[0], *AllUpgrades[index], !(KeyModifiers & ModifierShift));
		UI.StatusLine.Clear();
		UI.StatusLine.ClearCosts();
		//Wyrmgus start
		LastDrawnButtonPopup = NULL;
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
	SendCommandLearnAbility(*Selected[0], *AllUpgrades[index]);
	UI.StatusLine.Clear();
	UI.StatusLine.ClearCosts();
	
	if (AllUpgrades[index]->Ability && Selected[0]->Variable[LEVELUP_INDEX].Value == 0) {
		CurrentButtonLevel = 0;
		LastDrawnButtonPopup = NULL;
		ButtonUnderCursor = -1;
		OldButtonUnderCursor = -1;
		UI.ButtonPanel.Update();
	}
}

void CButtonPanel::DoClicked_Quest(int button)
{
	const int index = CurrentButtons[button].Value;
	SendCommandQuest(*Selected[0], Selected[0]->Player->AvailableQuests[index]);
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;
	if (Selected[0]->Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

void CButtonPanel::DoClicked_Buy(int button)
{
	int buy_costs[MaxCosts];
	memset(buy_costs, 0, sizeof(buy_costs));
	buy_costs[CopperCost] = UnitManager.GetSlotUnit(CurrentButtons[button].Value).GetPrice();
	if (!ThisPlayer->CheckCosts(buy_costs) && ThisPlayer->CheckLimits(*UnitManager.GetSlotUnit(CurrentButtons[button].Value).Type) >= 0) {
		SendCommandBuy(*Selected[0], &UnitManager.GetSlotUnit(CurrentButtons[button].Value), ThisPlayer->Index);
		ButtonUnderCursor = -1;
		OldButtonUnderCursor = -1;
		if (IsOnlySelected(*Selected[0])) {
			SelectedUnitChanged();
		}
	} else if (ThisPlayer->CheckLimits(*UnitManager.GetSlotUnit(CurrentButtons[button].Value).Type) == -3) {
		if (GameSounds.NotEnoughFood[ThisPlayer->Race].Sound) {
			PlayGameSound(GameSounds.NotEnoughFood[ThisPlayer->Race].Sound, MaxSampleVolume);
		}
	}
}

void CButtonPanel::DoClicked_SellResource(int button)
{
	const int resource = CurrentButtons[button].Value;
	int sell_resource_costs[MaxCosts];
	memset(sell_resource_costs, 0, sizeof(sell_resource_costs));
	sell_resource_costs[resource] = 100;
	if (!ThisPlayer->CheckCosts(sell_resource_costs)) {
		SendCommandSellResource(*Selected[0], resource, ThisPlayer->Index);
	}
}

void CButtonPanel::DoClicked_BuyResource(int button)
{
	const int resource = CurrentButtons[button].Value;
	int buy_resource_costs[MaxCosts];
	memset(buy_resource_costs, 0, sizeof(buy_resource_costs));
	buy_resource_costs[CopperCost] = 100;
	if (!ThisPlayer->CheckCosts(buy_resource_costs)) {
		SendCommandBuyResource(*Selected[0], resource, ThisPlayer->Index);
	}
}
//Wyrmgus end

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
	if (CurrentButtons[button].Pos == -1 || (!ThisPlayer->IsTeamed(*Selected[0]) && !ThisPlayer->HasBuildingAccess(*Selected[0]->Player)) || (!ThisPlayer->IsTeamed(*Selected[0]) && ThisPlayer->HasBuildingAccess(*Selected[0]->Player) && CurrentButtons[button].Action != ButtonTrain && CurrentButtons[button].Action != ButtonCancelTrain && CurrentButtons[button].Action != ButtonBuy && CurrentButtons[button].Action != ButtonSellResource && CurrentButtons[button].Action != ButtonBuyResource)) { //allow neutral units to be used (but only for training or as transporters)
	//Wyrmgus end
		return;
	}
	PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
	if (CurrentButtons[button].CommentSound.Sound) {
		PlayGameSound(CurrentButtons[button].CommentSound.Sound, MaxSampleVolume);
	}
	
	//Wyrmgus start
	if (!IsButtonUsable(*Selected[0], CurrentButtons[button])) {
		if (CurrentButtons[button].Action == ButtonResearch && UpgradeIdentAllowed(*ThisPlayer, CurrentButtons[button].ValueStr) == 'R') {
			ThisPlayer->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer, "%s", _("The upgrade has already been acquired"));
		} else {
			ThisPlayer->Notify(NotifyYellow, Selected[0]->tilePos, Selected[0]->MapLayer, "%s", _("The requirements have not been fulfilled"));
		}
		PlayGameSound(GameSounds.PlacementError[ThisPlayer->Race].Sound, MaxSampleVolume);
		return;
	}
	//Wyrmgus end

	//  Handle action on button.
	switch (CurrentButtons[button].Action) {
		case ButtonUnload: { DoClicked_Unload(button); break; }
		case ButtonSpellCast: { DoClicked_SpellCast(button); break; }
		case ButtonRepair: { DoClicked_Repair(button); break; }
		case ButtonMove:    // Follow Next
		case ButtonPatrol:  // Follow Next
		case ButtonHarvest: // Follow Next
		case ButtonAttack:  // Follow Next
		//Wyrmgus start
		case ButtonRallyPoint:
		case ButtonUnit:
		case ButtonEditorUnit:
		//Wyrmgus end
		case ButtonAttackGround: { DoClicked_SelectTarget(button); break; }
		case ButtonReturn: { DoClicked_Return(); break; }
		case ButtonStop: { DoClicked_Stop(); break; }
		case ButtonStandGround: { DoClicked_StandGround(); break; }
		case ButtonButton: { DoClicked_Button(button); break; }
		case ButtonCancel: // Follow Next
		case ButtonCancelUpgrade: { DoClicked_CancelUpgrade(); break; }
		case ButtonCancelTrain: { DoClicked_CancelTrain(); break; }
		case ButtonCancelBuild: { DoClicked_CancelBuild(); break; }
		case ButtonBuild: { DoClicked_Build(button); break; }
		case ButtonTrain: { DoClicked_Train(button); break; }
		case ButtonUpgradeTo: { DoClicked_UpgradeTo(button); break; }
		case ButtonResearch: { DoClicked_Research(button); break; }
		case ButtonCallbackAction: { DoClicked_CallbackAction(button); break; }
		//Wyrmgus start
		case ButtonLearnAbility: { DoClicked_LearnAbility(button); break; }
		case ButtonExperienceUpgradeTo: { DoClicked_ExperienceUpgradeTo(button); break; }
		case ButtonQuest: { DoClicked_Quest(button); break; }
		case ButtonBuy: { DoClicked_Buy(button); break; }
		case ButtonSellResource: { DoClicked_SellResource(button); break; }
		case ButtonBuyResource: { DoClicked_BuyResource(button); break; }
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

//@}
