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
/**@name button_action.cpp - The button action source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon, Pali Rohár and Andrettin
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

#include "ui/button_action.h"

#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "trigger/trigger.h"
#include "ui/button_level.h"
#include "ui/interface.h"
#include "ui/widgets.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void ButtonAction::ProcessConfigData(const CConfigData *config_data)
{
	ButtonAction ba;
	
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		std::string key = property.Key;
		std::string value = property.Value;
		
		if (key == "pos") {
			ba.Pos = std::stoi(value);
		} else if (key == "level") {
			value = FindAndReplaceString(value, "_", "-");
			ba.Level = CButtonLevel::GetButtonLevel(value);
		} else if (key == "always_show") {
			ba.AlwaysShow = StringToBool(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			ba.Icon.Name = value;
		} else if (key == "action") {
			value = FindAndReplaceString(value, "_", "-");
			const int button_action_id = GetButtonActionIdByName(value);
			if (button_action_id != -1) {
				ba.Action = ButtonCmd(button_action_id);
			} else {
				fprintf(stderr, "Invalid button action: \"%s\".\n", value.c_str());
			}
		} else if (key == "value") {
			value = FindAndReplaceString(value, "_", "-");
			ba.ValueStr = value;
		} else if (key == "allowed") {
			if (value == "check_true") {
				ba.Allowed = ButtonCheckTrue;
			} else if (value == "check_false") {
				ba.Allowed = ButtonCheckFalse;
			} else if (value == "check_upgrade") {
				ba.Allowed = ButtonCheckUpgrade;
			} else if (value == "check_upgrade_not") {
				ba.Allowed = ButtonCheckUpgradeNot;
			} else if (value == "check_upgrade_or") {
				ba.Allowed = ButtonCheckUpgradeOr;
			} else if (value == "check_individual_upgrade") {
				ba.Allowed = ButtonCheckIndividualUpgrade;
			} else if (value == "check_individual_upgrade_or") {
				ba.Allowed = ButtonCheckIndividualUpgradeOr;
			} else if (value == "check_unit_variable") {
				ba.Allowed = ButtonCheckUnitVariable;
			} else if (value == "check_units_or") {
				ba.Allowed = ButtonCheckUnitsOr;
			} else if (value == "check_units_and") {
				ba.Allowed = ButtonCheckUnitsAnd;
			} else if (value == "check_units_not") {
				ba.Allowed = ButtonCheckUnitsNot;
			} else if (value == "check_network") {
				ba.Allowed = ButtonCheckNetwork;
			} else if (value == "check_no_network") {
				ba.Allowed = ButtonCheckNoNetwork;
			} else if (value == "check_no_work") {
				ba.Allowed = ButtonCheckNoWork;
			} else if (value == "check_no_research") {
				ba.Allowed = ButtonCheckNoResearch;
			} else if (value == "check_attack") {
				ba.Allowed = ButtonCheckAttack;
			} else if (value == "check_upgrade_to") {
				ba.Allowed = ButtonCheckUpgradeTo;
			} else if (value == "check_research") {
				ba.Allowed = ButtonCheckResearch;
			} else if (value == "check_single_research") {
				ba.Allowed = ButtonCheckSingleResearch;
			} else if (value == "check_has_inventory") {
				ba.Allowed = ButtonCheckHasInventory;
			} else if (value == "check_has_sub_buttons") {
				ba.Allowed = ButtonCheckHasSubButtons;
			} else {
				fprintf(stderr, "Invalid button check: \"%s\".\n", value.c_str());
			}
		} else if (key == "allow_arg") {
			value = FindAndReplaceString(value, "_", "-");
			if (!ba.AllowStr.empty()) {
				ba.AllowStr += ",";
			}
			ba.AllowStr += value;
		} else if (key == "key") {
			ba.Key = GetHotKey(value);
		} else if (key == "hint") {
			ba.Hint = value;
		} else if (key == "description") {
			ba.Description = value;
		} else if (key == "comment_sound") {
			value = FindAndReplaceString(value, "_", "-");
			ba.CommentSound.Name = value;
		} else if (key == "button_cursor") {
			value = FindAndReplaceString(value, "_", "-");
			ba.ButtonCursor = value;
		} else if (key == "popup") {
			value = FindAndReplaceString(value, "_", "-");
			ba.Popup = value;
		} else if (key == "for_unit") {
			if (value == "*") {
				ba.UnitMask = value;
			} else {
				value = FindAndReplaceString(value, "_", "-");
				if (ba.UnitMask.empty()) {
					ba.UnitMask += ",";
				}
				ba.UnitMask += value;
				ba.UnitMask += ",";
			}
		} else {
			fprintf(stderr, "Invalid button property: \"%s\".\n", key.c_str());
		}
	}
	
	AddButton(ba.Pos, ba.Level, ba.Icon.Name, ba.Action, ba.ValueStr, ba.Allowed, ba.AllowStr, ba.Key, ba.Hint, ba.Description, ba.CommentSound.Name, ba.ButtonCursor, ba.UnitMask, ba.Popup, ba.AlwaysShow, ba.Mod);
}

void ButtonAction::SetTriggerData() const
{
	if (this->Action != ButtonUnit && this->Action != ButtonBuy) {
		TriggerData.Type = CUnitType::Get(this->Value);
	} else {
		TriggerData.Type = CUnitType::Get(UnitManager.GetSlotUnit(this->Value).Type->GetIndex());
		TriggerData.Unit = &UnitManager.GetSlotUnit(this->Value);
	}
	if (this->Action == ButtonResearch || this->Action == ButtonLearnAbility) {
		TriggerData.Upgrade = AllUpgrades[this->Value];
	} else if (this->Action == ButtonFaction) {
		TriggerData.Faction = CPlayer::GetThisPlayer()->GetFaction()->DevelopsTo[this->Value];
		if (!CPlayer::GetThisPlayer()->GetFaction()->DevelopsTo[this->Value]->FactionUpgrade.empty()) {
			TriggerData.Upgrade = CUpgrade::Get(CPlayer::GetThisPlayer()->GetFaction()->DevelopsTo[this->Value]->FactionUpgrade);
		}
	}
}

void ButtonAction::CleanTriggerData() const
{
	TriggerData.Type = nullptr;
	TriggerData.Unit = nullptr;
	TriggerData.Upgrade = nullptr;
	TriggerData.Resource = nullptr;
	TriggerData.Faction = nullptr;
}

int ButtonAction::GetLevelIndex() const
{
	if (this->Level) {
		return this->Level->GetIndex();
	} else {
		return 0;
	}
}

int ButtonAction::GetKey() const
{
	if ((this->Action == ButtonBuild || this->Action == ButtonTrain || this->Action == ButtonResearch || this->Action == ButtonLearnAbility || this->Action == ButtonExperienceUpgradeTo || this->Action == ButtonUpgradeTo) && !IsButtonUsable(*Selected[0], *this)) {
		return 0;
	}

	if (this->Key == gcn::Key::K_ESCAPE || this->Key == gcn::Key::K_DELETE || this->Key == gcn::Key::K_PAGE_DOWN || this->Key == gcn::Key::K_PAGE_UP) {
		return this->Key;
	}
	
	if ((Preference.HotkeySetup == 1 || (Preference.HotkeySetup == 2 && (this->Action == ButtonBuild || this->Action == ButtonTrain || this->Action == ButtonResearch || this->Action == ButtonLearnAbility || this->Action == ButtonExperienceUpgradeTo || this->Action == ButtonUpgradeTo || this->Action == ButtonRallyPoint || this->Action == ButtonSalvage || this->Action == ButtonEnterMapLayer))) && this->Key != 0) {
		if (this->Pos == 1) {
			return 'q';
		} else if (this->Pos == 2) {
			return 'w';
		} else if (this->Pos == 3) {
			return 'e';
		} else if (this->Pos == 4) {
			return 'r';
		} else if (this->Pos == 5) {
			return 'a';
		} else if (this->Pos == 6) {
			return 's';
		} else if (this->Pos == 7) {
			return 'd';
		} else if (this->Pos == 8) {
			return 'f';
		} else if (this->Pos == 9) {
			return 'z';
		} else if (this->Pos == 10) {
			return 'x';
		} else if (this->Pos == 11) {
			return 'c';
		} else if (this->Pos == 12) {
			return 'v';
		} else if (this->Pos == 13) {
			return 't';
		} else if (this->Pos == 14) {
			return 'g';
		} else if (this->Pos == 15) {
			return 'b';
		} else if (this->Pos == 16) {
			return 'y';
		}
	}
	return this->Key;
}

std::string ButtonAction::GetHint() const
{
	if ((this->Action == ButtonBuild || this->Action == ButtonTrain || this->Action == ButtonResearch || this->Action == ButtonLearnAbility || this->Action == ButtonExperienceUpgradeTo || this->Action == ButtonUpgradeTo) && !IsButtonUsable(*Selected[0], *this) && this->Key != 0 && !this->Hint.empty()) {
		std::string hint = this->Hint;
		hint = FindAndReplaceString(hint, "~!", "");
		return hint;
	}

	if (this->Key == gcn::Key::K_ESCAPE) {
		return this->Hint;
	}
	
	if ((Preference.HotkeySetup == 1 || (Preference.HotkeySetup == 2 && (this->Action == ButtonBuild || this->Action == ButtonTrain || this->Action == ButtonResearch || this->Action == ButtonLearnAbility || this->Action == ButtonExperienceUpgradeTo || this->Action == ButtonUpgradeTo || this->Action == ButtonRallyPoint || this->Action == ButtonSalvage || this->Action == ButtonEnterMapLayer))) && this->Key != 0 && !this->Hint.empty()) {
		std::string hint = this->Hint;
		hint = FindAndReplaceString(hint, "~!", "");
		hint += " (~!";
		hint += CapitalizeString(SdlKey2Str(this->GetKey()));
		hint += ")";
		return hint;
	}
	
	return this->Hint;
}

std::string GetButtonActionNameById(const int button_action)
{
	if (button_action == ButtonMove) {
		return "attack";
	} else if (button_action == ButtonStop) {
		return "stop";
	} else if (button_action == ButtonAttack) {
		return "attack";
	} else if (button_action == ButtonRepair) {
		return "repair";
	} else if (button_action == ButtonHarvest) {
		return "harvest";
	} else if (button_action == ButtonButton) {
		return "button";
	} else if (button_action == ButtonBuild) {
		return "build";
	} else if (button_action == ButtonTrain) {
		return "train-unit";
	} else if (button_action == ButtonPatrol) {
		return "patrol";
	} else if (button_action == ButtonStandGround) {
		return "stand-ground";
	} else if (button_action == ButtonAttackGround) {
		return "attack-ground";
	} else if (button_action == ButtonReturn) {
		return "return-goods";
	} else if (button_action == ButtonSpellCast) {
		return "cast-spell";
	} else if (button_action == ButtonResearch) {
		return "research";
	} else if (button_action == ButtonLearnAbility) {
		return "learn-ability";
	} else if (button_action == ButtonExperienceUpgradeTo) {
		return "experience-upgrade-to";
	} else if (button_action == ButtonUpgradeTo) {
		return "upgrade-to";
	} else if (button_action == ButtonUnload) {
		return "unload";
	} else if (button_action == ButtonRallyPoint) {
		return "rally-point";
	} else if (button_action == ButtonFaction) {
		return "faction";
	} else if (button_action == ButtonQuest) {
		return "quest";
	} else if (button_action == ButtonBuy) {
		return "buy";
	} else if (button_action == ButtonProduceResource) {
		return "produce-resource";
	} else if (button_action == ButtonSellResource) {
		return "sell-resource";
	} else if (button_action == ButtonBuyResource) {
		return "buy-resource";
	} else if (button_action == ButtonSalvage) {
		return "salvage";
	} else if (button_action == ButtonEnterMapLayer) {
		return "enter-map-layer";
	} else if (button_action == ButtonUnit) {
		return "unit";
	} else if (button_action == ButtonEditorUnit) {
		return "editor-unit";
	} else if (button_action == ButtonCancel) {
		return "cancel";
	} else if (button_action == ButtonCancelUpgrade) {
		return "cancel-upgrade";
	} else if (button_action == ButtonCancelTrain) {
		return "cancel-train-unit";
	} else if (button_action == ButtonCancelBuild) {
		return "cancel-build";
	}

	return "";
}

int GetButtonActionIdByName(const std::string &button_action)
{
	if (button_action == "move") {
		return ButtonMove;
	} else if (button_action == "stop") {
		return ButtonStop;
	} else if (button_action == "attack") {
		return ButtonAttack;
	} else if (button_action == "repair") {
		return ButtonRepair;
	} else if (button_action == "harvest") {
		return ButtonHarvest;
	} else if (button_action == "button") {
		return ButtonButton;
	} else if (button_action == "build") {
		return ButtonBuild;
	} else if (button_action == "train-unit") {
		return ButtonTrain;
	} else if (button_action == "patrol") {
		return ButtonPatrol;
	} else if (button_action == "stand-ground") {
		return ButtonStandGround;
	} else if (button_action == "attack-ground") {
		return ButtonAttackGround;
	} else if (button_action == "return-goods") {
		return ButtonReturn;
	} else if (button_action == "cast-spell") {
		return ButtonSpellCast;
	} else if (button_action == "research") {
		return ButtonResearch;
	} else if (button_action == "learn-ability") {
		return ButtonLearnAbility;
	} else if (button_action == "experience-upgrade-to") {
		return ButtonExperienceUpgradeTo;
	} else if (button_action == "upgrade-to") {
		return ButtonUpgradeTo;
	} else if (button_action == "unload") {
		return ButtonUnload;
	} else if (button_action == "rally-point") {
		return ButtonRallyPoint;
	} else if (button_action == "faction") {
		return ButtonFaction;
	} else if (button_action == "quest") {
		return ButtonQuest;
	} else if (button_action == "buy") {
		return ButtonBuy;
	} else if (button_action == "produce-resource") {
		return ButtonProduceResource;
	} else if (button_action == "sell-resource") {
		return ButtonSellResource;
	} else if (button_action == "buy-resource") {
		return ButtonBuyResource;
	} else if (button_action == "salvage") {
		return ButtonSalvage;
	} else if (button_action == "enter-map-layer") {
		return ButtonEnterMapLayer;
	} else if (button_action == "unit") {
		return ButtonUnit;
	} else if (button_action == "editor-unit") {
		return ButtonEditorUnit;
	} else if (button_action == "cancel") {
		return ButtonCancel;
	} else if (button_action == "cancel-upgrade") {
		return ButtonCancelUpgrade;
	} else if (button_action == "cancel-train-unit") {
		return ButtonCancelTrain;
	} else if (button_action == "cancel-build") {
		return ButtonCancelBuild;
	}

	return -1;
}

bool IsNeutralUsableButtonAction(const int button_action)
{
	return button_action == ButtonTrain || button_action == ButtonCancelTrain || button_action == ButtonBuy || button_action == ButtonSellResource || button_action == ButtonBuyResource || button_action == ButtonResearch;
}
