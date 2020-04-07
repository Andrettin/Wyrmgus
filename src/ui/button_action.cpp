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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon, Pali Rohár and Andrettin
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
#include "trigger.h"
#include "ui/button_level.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "util/string_util.h"
#include "video.h"
#include "widgets.h"

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
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "pos") {
			ba.Pos = std::stoi(value);
		} else if (key == "level") {
			value = FindAndReplaceString(value, "_", "-");
			ba.Level = CButtonLevel::GetButtonLevel(value);
		} else if (key == "always_show") {
			ba.AlwaysShow = string::to_bool(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			ba.Icon.Name = value;
		} else if (key == "action") {
			value = FindAndReplaceString(value, "_", "-");
			const ButtonCmd button_action_id = GetButtonActionIdByName(value);
			if (button_action_id != ButtonCmd::None) {
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
	
	AddButton(ba.Pos, ba.Level, ba.Icon.Name, ba.Action, ba.ValueStr, ba.Payload, ba.Allowed, ba.AllowStr, ba.Key, ba.Hint, ba.Description, ba.CommentSound.Name, ba.ButtonCursor, ba.UnitMask, ba.Popup, ba.AlwaysShow, ba.Mod);
}

void ButtonAction::SetTriggerData() const
{
	if (this->Action != ButtonCmd::Unit && this->Action != ButtonCmd::Buy) {
		TriggerData.Type = UnitTypes[this->Value];
	} else {
		TriggerData.Type = UnitTypes[UnitManager.GetSlotUnit(this->Value).Type->Slot];
		TriggerData.Unit = &UnitManager.GetSlotUnit(this->Value);
	}
	if (this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility) {
		TriggerData.Upgrade = CUpgrade::get_all()[this->Value];
	} else if (this->Action == ButtonCmd::Faction) {
		TriggerData.Faction = PlayerRaces.Factions[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[this->Value];
		if (!PlayerRaces.Factions[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[this->Value]->FactionUpgrade.empty()) {
			TriggerData.Upgrade = CUpgrade::try_get(PlayerRaces.Factions[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[this->Value]->FactionUpgrade);
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

int ButtonAction::GetLevelID() const
{
	if (this->Level) {
		return this->Level->ID;
	} else {
		return 0;
	}
}

int ButtonAction::GetKey() const
{
	if ((this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo) && !IsButtonUsable(*Selected[0], *this)) {
		return 0;
	}

	if (this->Key == gcn::Key::K_ESCAPE || this->Key == gcn::Key::K_DELETE || this->Key == gcn::Key::K_PAGE_DOWN || this->Key == gcn::Key::K_PAGE_UP) {
		return this->Key;
	}
	
	if ((Preference.HotkeySetup == 1 || (Preference.HotkeySetup == 2 && (this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::RallyPoint || this->Action == ButtonCmd::Salvage || this->Action == ButtonCmd::EnterMapLayer))) && this->Key != 0) {
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
	if ((this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo) && !IsButtonUsable(*Selected[0], *this) && this->Key != 0 && !this->Hint.empty()) {
		std::string hint = this->Hint;
		hint = FindAndReplaceString(hint, "~!", "");
		return hint;
	}

	if (this->Key == gcn::Key::K_ESCAPE) {
		return this->Hint;
	}
	
	if ((Preference.HotkeySetup == 1 || (Preference.HotkeySetup == 2 && (this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::RallyPoint || this->Action == ButtonCmd::Salvage || this->Action == ButtonCmd::EnterMapLayer))) && this->Key != 0 && !this->Hint.empty()) {
		std::string hint = this->Hint;
		hint = FindAndReplaceString(hint, "~!", "");
		hint += " (~!";
		hint += CapitalizeString(SdlKey2Str(this->GetKey()));
		hint += ")";
		return hint;
	}
	
	return this->Hint;
}

std::string GetButtonActionNameById(const ButtonCmd button_action)
{
	switch (button_action) {
		case ButtonCmd::None:
			return std::string();
		case ButtonCmd::Move:
			return "attack";
		case ButtonCmd::Stop:
			return "stop";
		case ButtonCmd::Attack:
			return "attack";
		case ButtonCmd::Repair:
			return "repair";
		case ButtonCmd::Harvest:
			return "harvest";
		case ButtonCmd::Button:
			return "button";
		case ButtonCmd::Build:
			return "build";
		case ButtonCmd::Train:
			return "train-unit";
		case ButtonCmd::Patrol:
			return "patrol";
		case ButtonCmd::StandGround:
			return "stand-ground";
		case ButtonCmd::AttackGround:
			return "attack-ground";
		case ButtonCmd::Return:
			return "return-goods";
		case ButtonCmd::SpellCast:
			return "cast-spell";
		case ButtonCmd::Research:
			return "research";
		case ButtonCmd::LearnAbility:
			return "learn-ability";
		case ButtonCmd::ExperienceUpgradeTo:
			return "experience-upgrade-to";
		case ButtonCmd::UpgradeTo:
			return "upgrade-to";
		case ButtonCmd::Unload:
			return "unload";
		case ButtonCmd::RallyPoint:
			return "rally-point";
		case ButtonCmd::Faction:
			return "faction";
		case ButtonCmd::Quest:
			return "quest";
		case ButtonCmd::Buy:
			return "buy";
		case ButtonCmd::ProduceResource:
			return "produce-resource";
		case ButtonCmd::SellResource:
			return "sell-resource";
		case ButtonCmd::BuyResource:
			return "buy-resource";
		case ButtonCmd::Salvage:
			return "salvage";
		case ButtonCmd::EnterMapLayer:
			return "enter-map-layer";
		case ButtonCmd::Unit:
			return "unit";
		case ButtonCmd::EditorUnit:
			return "editor-unit";
		case ButtonCmd::Cancel:
			return "cancel";
		case ButtonCmd::CancelUpgrade:
			return "cancel-upgrade";
		case ButtonCmd::CancelTrain:
			return "cancel-train-unit";
		case ButtonCmd::CancelBuild:
			return "cancel-build";
	}

	throw std::runtime_error("Invalid button action enum value: " + std::to_string(static_cast<int>(button_action)));
}

ButtonCmd GetButtonActionIdByName(const std::string &button_action)
{
	if (button_action == "move") {
		return ButtonCmd::Move;
	} else if (button_action == "stop") {
		return ButtonCmd::Stop;
	} else if (button_action == "attack") {
		return ButtonCmd::Attack;
	} else if (button_action == "repair") {
		return ButtonCmd::Repair;
	} else if (button_action == "harvest") {
		return ButtonCmd::Harvest;
	} else if (button_action == "button") {
		return ButtonCmd::Button;
	} else if (button_action == "build") {
		return ButtonCmd::Build;
	} else if (button_action == "train-unit") {
		return ButtonCmd::Train;
	} else if (button_action == "patrol") {
		return ButtonCmd::Patrol;
	} else if (button_action == "stand-ground") {
		return ButtonCmd::StandGround;
	} else if (button_action == "attack-ground") {
		return ButtonCmd::AttackGround;
	} else if (button_action == "return-goods") {
		return ButtonCmd::Return;
	} else if (button_action == "cast-spell") {
		return ButtonCmd::SpellCast;
	} else if (button_action == "research") {
		return ButtonCmd::Research;
	} else if (button_action == "learn-ability") {
		return ButtonCmd::LearnAbility;
	} else if (button_action == "experience-upgrade-to") {
		return ButtonCmd::ExperienceUpgradeTo;
	} else if (button_action == "upgrade-to") {
		return ButtonCmd::UpgradeTo;
	} else if (button_action == "unload") {
		return ButtonCmd::Unload;
	} else if (button_action == "rally-point") {
		return ButtonCmd::RallyPoint;
	} else if (button_action == "faction") {
		return ButtonCmd::Faction;
	} else if (button_action == "quest") {
		return ButtonCmd::Quest;
	} else if (button_action == "buy") {
		return ButtonCmd::Buy;
	} else if (button_action == "produce-resource") {
		return ButtonCmd::ProduceResource;
	} else if (button_action == "sell-resource") {
		return ButtonCmd::SellResource;
	} else if (button_action == "buy-resource") {
		return ButtonCmd::BuyResource;
	} else if (button_action == "salvage") {
		return ButtonCmd::Salvage;
	} else if (button_action == "enter-map-layer") {
		return ButtonCmd::EnterMapLayer;
	} else if (button_action == "unit") {
		return ButtonCmd::Unit;
	} else if (button_action == "editor-unit") {
		return ButtonCmd::EditorUnit;
	} else if (button_action == "cancel") {
		return ButtonCmd::Cancel;
	} else if (button_action == "cancel-upgrade") {
		return ButtonCmd::CancelUpgrade;
	} else if (button_action == "cancel-train-unit") {
		return ButtonCmd::CancelTrain;
	} else if (button_action == "cancel-build") {
		return ButtonCmd::CancelBuild;
	}

	return ButtonCmd::None;
}

bool IsNeutralUsableButtonAction(const ButtonCmd button_action)
{
	return button_action == ButtonCmd::Train || button_action == ButtonCmd::CancelTrain || button_action == ButtonCmd::Buy || button_action == ButtonCmd::SellResource || button_action == ButtonCmd::BuyResource || button_action == ButtonCmd::Research;
}
