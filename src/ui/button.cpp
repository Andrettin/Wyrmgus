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

#include "stratagus.h"

#include "ui/button.h"

#include "config.h"
#include "faction.h"
#include "script/trigger.h"
#include "spells.h"
#include "ui/button_level.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "upgrade/upgrade.h"
#include "util/string_util.h"
#include "video.h"
#include "widgets.h"

#include <QUuid>

namespace stratagus {

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void button::ProcessConfigData(const CConfigData *config_data)
{
	const QUuid uuid = QUuid::createUuid();
	const std::string identifier = uuid.toString(QUuid::WithoutBraces).toStdString();

	stratagus::button *button = stratagus::button::add(identifier, nullptr);
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "pos") {
			button->pos = std::stoi(value);
		} else if (key == "level") {
			value = FindAndReplaceString(value, "_", "-");
			button->Level = CButtonLevel::GetButtonLevel(value);
		} else if (key == "always_show") {
			button->AlwaysShow = string::to_bool(value);
		} else if (key == "icon") {
			button->Icon.Name = value;
		} else if (key == "action") {
			value = FindAndReplaceString(value, "_", "-");
			const ButtonCmd button_action_id = GetButtonActionIdByName(value);
			if (button_action_id != ButtonCmd::None) {
				button->Action = ButtonCmd(button_action_id);
			} else {
				fprintf(stderr, "Invalid button action: \"%s\".\n", value.c_str());
			}
		} else if (key == "value") {
			value = FindAndReplaceString(value, "_", "-");
			button->ValueStr = value;
		} else if (key == "allowed") {
			if (value == "check_true") {
				button->Allowed = ButtonCheckTrue;
			} else if (value == "check_false") {
				button->Allowed = ButtonCheckFalse;
			} else if (value == "check_upgrade") {
				button->Allowed = ButtonCheckUpgrade;
			} else if (value == "check_upgrade_not") {
				button->Allowed = ButtonCheckUpgradeNot;
			} else if (value == "check_upgrade_or") {
				button->Allowed = ButtonCheckUpgradeOr;
			} else if (value == "check_individual_upgrade") {
				button->Allowed = ButtonCheckIndividualUpgrade;
			} else if (value == "check_individual_upgrade_or") {
				button->Allowed = ButtonCheckIndividualUpgradeOr;
			} else if (value == "check_unit_variable") {
				button->Allowed = ButtonCheckUnitVariable;
			} else if (value == "check_units_or") {
				button->Allowed = ButtonCheckUnitsOr;
			} else if (value == "check_units_and") {
				button->Allowed = ButtonCheckUnitsAnd;
			} else if (value == "check_units_not") {
				button->Allowed = ButtonCheckUnitsNot;
			} else if (value == "check_network") {
				button->Allowed = ButtonCheckNetwork;
			} else if (value == "check_no_network") {
				button->Allowed = ButtonCheckNoNetwork;
			} else if (value == "check_no_work") {
				button->Allowed = ButtonCheckNoWork;
			} else if (value == "check_no_research") {
				button->Allowed = ButtonCheckNoResearch;
			} else if (value == "check_attack") {
				button->Allowed = ButtonCheckAttack;
			} else if (value == "check_upgrade_to") {
				button->Allowed = ButtonCheckUpgradeTo;
			} else if (value == "check_research") {
				button->Allowed = ButtonCheckResearch;
			} else if (value == "check_single_research") {
				button->Allowed = ButtonCheckSingleResearch;
			} else if (value == "check_has_inventory") {
				button->Allowed = ButtonCheckHasInventory;
			} else if (value == "check_has_sub_buttons") {
				button->Allowed = ButtonCheckHasSubButtons;
			} else {
				fprintf(stderr, "Invalid button check: \"%s\".\n", value.c_str());
			}
		} else if (key == "allow_arg") {
			value = FindAndReplaceString(value, "_", "-");
			if (!button->AllowStr.empty()) {
				button->AllowStr += ",";
			}
			button->AllowStr += value;
		} else if (key == "key") {
			button->Key = GetHotKey(value);
		} else if (key == "hint") {
			button->Hint = value;
		} else if (key == "description") {
			button->Description = value;
		} else if (key == "comment_sound") {
			value = FindAndReplaceString(value, "_", "-");
			button->CommentSound.Name = value;
		} else if (key == "popup") {
			value = FindAndReplaceString(value, "_", "-");
			button->Popup = value;
		} else if (key == "for_unit") {
			if (value == "*") {
				button->UnitMask = value;
			} else {
				value = FindAndReplaceString(value, "_", "-");
				if (button->UnitMask.empty()) {
					button->UnitMask += ",";
				}
				button->UnitMask += value;
				button->UnitMask += ",";
			}
		} else {
			fprintf(stderr, "Invalid button property: \"%s\".\n", key.c_str());
		}
	}
}

void button::initialize()
{
	if (!this->ValueStr.empty()) {
		switch (this->Action) {
			case ButtonCmd::SpellCast:
				this->Value = CSpell::GetSpell(this->ValueStr)->Slot;
#ifdef DEBUG
				if (ba->Value < 0) {
					DebugPrint("Spell %s does not exist?\n" _C_ value.c_str());
					Assert(ba->Value >= 0);
				}
#endif
				break;
			case ButtonCmd::Train:
				this->Value = UnitTypeIdByIdent(this->ValueStr);
				break;
			case ButtonCmd::Research:
				this->Value = UpgradeIdByIdent(this->ValueStr);
				break;
				//Wyrmgus start
			case ButtonCmd::LearnAbility:
				this->Value = UpgradeIdByIdent(this->ValueStr);
				break;
			case ButtonCmd::ExperienceUpgradeTo:
				this->Value = UnitTypeIdByIdent(this->ValueStr);
				break;
				//Wyrmgus end
			case ButtonCmd::UpgradeTo:
				this->Value = UnitTypeIdByIdent(this->ValueStr);
				break;
			case ButtonCmd::Build:
				this->Value = UnitTypeIdByIdent(this->ValueStr);
				break;
				//Wyrmgus start
			case ButtonCmd::ProduceResource:
				this->Value = GetResourceIdByName(this->ValueStr.c_str());
				break;
			case ButtonCmd::SellResource:
				this->Value = GetResourceIdByName(this->ValueStr.c_str());
				break;
			case ButtonCmd::BuyResource:
				this->Value = GetResourceIdByName(this->ValueStr.c_str());
				break;
				//Wyrmgus end
			case ButtonCmd::Button:
				if (CButtonLevel::GetButtonLevel(this->ValueStr)) {
					this->Value = CButtonLevel::GetButtonLevel(this->ValueStr)->ID;
				} else {
					this->Value = 0;
				}
				break;
			default:
				this->Value = atoi(this->ValueStr.c_str());
				break;
		}
	} else {
		this->Value = 0;
	}

	if (!this->CommentSound.Name.empty()) {
		this->CommentSound.MapSound();
	}

	if (!this->Popup.empty()) {
		CPopup *popup = PopupByIdent(this->Popup);
		if (!popup) {
			throw std::runtime_error("Popup \"" + this->Popup + "\" hasn't defined.");
		}
	}

	// FIXME: here should be added costs to the hint
	// FIXME: johns: show should be nice done?
	if (this->UnitMask[0] != '*') {
		this->UnitMask = "," + this->UnitMask + ",";
	}
}

void button::SetTriggerData() const
{
	if (this->Action != ButtonCmd::Unit && this->Action != ButtonCmd::Buy) {
		TriggerData.Type = unit_type::get_all()[this->Value];
	} else {
		TriggerData.Type = UnitManager.GetSlotUnit(this->Value).Type;
		TriggerData.Unit = &UnitManager.GetSlotUnit(this->Value);
	}
	if (this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility) {
		TriggerData.Upgrade = CUpgrade::get_all()[this->Value];
	} else if (this->Action == ButtonCmd::Faction) {
		TriggerData.Faction = faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[this->Value];
		if (!faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[this->Value]->FactionUpgrade.empty()) {
			TriggerData.Upgrade = CUpgrade::try_get(faction::get_all()[CPlayer::GetThisPlayer()->Faction]->DevelopsTo[this->Value]->FactionUpgrade);
		}
	}
}

void button::CleanTriggerData() const
{
	TriggerData.Type = nullptr;
	TriggerData.Unit = nullptr;
	TriggerData.Upgrade = nullptr;
	TriggerData.Resource = nullptr;
	TriggerData.Faction = nullptr;
}

int button::GetLevelID() const
{
	if (this->Level) {
		return this->Level->ID;
	} else {
		return 0;
	}
}

int button::GetKey() const
{
	if ((this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo) && !IsButtonUsable(*Selected[0], *this)) {
		return 0;
	}

	if (this->Key == gcn::Key::K_ESCAPE || this->Key == gcn::Key::K_DELETE || this->Key == gcn::Key::K_PAGE_DOWN || this->Key == gcn::Key::K_PAGE_UP) {
		return this->Key;
	}
	
	if ((Preference.HotkeySetup == 1 || (Preference.HotkeySetup == 2 && (this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::RallyPoint || this->Action == ButtonCmd::Salvage || this->Action == ButtonCmd::EnterMapLayer))) && this->Key != 0) {
		if (this->get_pos() == 1) {
			return 'q';
		} else if (this->get_pos() == 2) {
			return 'w';
		} else if (this->get_pos() == 3) {
			return 'e';
		} else if (this->get_pos() == 4) {
			return 'r';
		} else if (this->get_pos() == 5) {
			return 'a';
		} else if (this->get_pos() == 6) {
			return 's';
		} else if (this->get_pos() == 7) {
			return 'd';
		} else if (this->get_pos() == 8) {
			return 'f';
		} else if (this->get_pos() == 9) {
			return 'z';
		} else if (this->get_pos() == 10) {
			return 'x';
		} else if (this->get_pos() == 11) {
			return 'c';
		} else if (this->get_pos() == 12) {
			return 'v';
		} else if (this->get_pos() == 13) {
			return 't';
		} else if (this->get_pos() == 14) {
			return 'g';
		} else if (this->get_pos() == 15) {
			return 'b';
		} else if (this->get_pos() == 16) {
			return 'y';
		}
	}
	return this->Key;
}

std::string button::GetHint() const
{
	if ((this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo) && !IsButtonUsable(*Selected[0], *this) && this->Key != 0 && !this->Hint.empty()) {
		std::string hint = this->Hint;
		string::replace(hint, "~!", "");
		return hint;
	}

	if (this->Key == gcn::Key::K_ESCAPE) {
		return this->Hint;
	}
	
	if ((Preference.HotkeySetup == 1 || (Preference.HotkeySetup == 2 && (this->Action == ButtonCmd::Build || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::RallyPoint || this->Action == ButtonCmd::Salvage || this->Action == ButtonCmd::EnterMapLayer))) && this->Key != 0 && !this->Hint.empty()) {
		std::string hint = this->Hint;
		string::replace(hint, "~!", "");
		hint += " (~!";
		hint += CapitalizeString(SdlKey2Str(this->GetKey()));
		hint += ")";
		return hint;
	}
	
	return this->Hint;
}

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
	} else if (button_action == "stand-ground" || button_action == "stand_ground") {
		return ButtonCmd::StandGround;
	} else if (button_action == "attack-ground") {
		return ButtonCmd::AttackGround;
	} else if (button_action == "return-goods" || button_action == "return_goods") {
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
