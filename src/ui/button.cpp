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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Pali Rohár and Andrettin
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

#include "ui/button.h"

#include "database/preferences.h"
#include "economy/resource.h"
#include "include/config.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/player.h"
#include "script/condition/and_condition.h"
#include "script/trigger.h"
#include "spell/spell.h"
#include "ui/button_cmd.h"
#include "ui/button_level.h"
#include "ui/hotkey_setup.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_manager.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"
#include "util/util.h"
#include "video/font.h"
#include "video/video.h"
#include "widgets.h"

#pragma warning(push, 0)
#include <QUuid>
#pragma warning(pop)

namespace wyrmgus {

void button::ProcessConfigData(const CConfigData *config_data)
{
	const QUuid uuid = QUuid::createUuid();
	const std::string identifier = uuid.toString(QUuid::WithoutBraces).toStdString();

	button *button = button::add(identifier, nullptr);
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "pos") {
			button->pos = std::stoi(value);
		} else if (key == "level") {
			button->level = button_level::get(value);
		} else if (key == "always_show") {
			button->always_show = string::to_bool(value);
		} else if (key == "icon") {
			button->Icon.Name = value;
		} else if (key == "action") {
			value = FindAndReplaceString(value, "_", "-");
			const ButtonCmd button_action_id = GetButtonActionIdByName(value);
			if (button_action_id != ButtonCmd::None) {
				button->Action = button_action_id;
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

void button::add_button_key_to_name(std::string &name, const std::string &key)
{
	std::string button_key = key;
	const size_t key_pos = string::ci_find(name, button_key);
	if (key_pos != std::string::npos) {
		name.insert(key_pos, "~!");
	} else {
		string::capitalize(button_key);
		name += " (~!" + button_key + ")";
	}
}

button::button(const std::string &identifier) : data_entry(identifier), Action(ButtonCmd::Move)
{
}

button::~button()
{
}

void button::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "action") {
		const ButtonCmd button_action_id = GetButtonActionIdByName(value);
		if (button_action_id != ButtonCmd::None) {
			this->Action = button_action_id;
		} else {
			throw std::runtime_error("Invalid button action: \"" + value + "\".");
		}
	} else if (key == "value") {
		this->ValueStr = value;
	} else if (key == "key") {
		this->Key = GetHotKey(value);
	} else if (key == "hint") {
		this->Hint = value;
	} else if (key == "description") {
		this->Description = value;
	} else if (key == "popup") {
		this->Popup = value;
	} else if (key == "icon") {
		this->Icon.Name = value;
	} else if (key == "allowed") {
		if (value == "check_true") {
			this->Allowed = ButtonCheckTrue;
		} else if (value == "check_false") {
			this->Allowed = ButtonCheckFalse;
		} else if (value == "check_upgrade") {
			this->Allowed = ButtonCheckUpgrade;
		} else if (value == "check_upgrade_not") {
			this->Allowed = ButtonCheckUpgradeNot;
		} else if (value == "check_upgrade_or") {
			this->Allowed = ButtonCheckUpgradeOr;
		} else if (value == "check_individual_upgrade") {
			this->Allowed = ButtonCheckIndividualUpgrade;
		} else if (value == "check_individual_upgrade_or") {
			this->Allowed = ButtonCheckIndividualUpgradeOr;
		} else if (value == "check_unit_variable") {
			this->Allowed = ButtonCheckUnitVariable;
		} else if (value == "check_units_or") {
			this->Allowed = ButtonCheckUnitsOr;
		} else if (value == "check_units_and") {
			this->Allowed = ButtonCheckUnitsAnd;
		} else if (value == "check_units_not") {
			this->Allowed = ButtonCheckUnitsNot;
		} else if (value == "check_network") {
			this->Allowed = ButtonCheckNetwork;
		} else if (value == "check_no_network") {
			this->Allowed = ButtonCheckNoNetwork;
		} else if (value == "check_no_work") {
			this->Allowed = ButtonCheckNoWork;
		} else if (value == "check_no_research") {
			this->Allowed = ButtonCheckNoResearch;
		} else if (value == "check_attack") {
			this->Allowed = ButtonCheckAttack;
		} else if (value == "check_upgrade_to") {
			this->Allowed = ButtonCheckUpgradeTo;
		} else if (value == "check_research") {
			this->Allowed = ButtonCheckResearch;
		} else if (value == "check_single_research") {
			this->Allowed = ButtonCheckSingleResearch;
		} else if (value == "check_has_inventory") {
			this->Allowed = ButtonCheckHasInventory;
		} else if (value == "check_has_sub_buttons") {
			this->Allowed = ButtonCheckHasSubButtons;
		} else {
			throw std::runtime_error("Invalid button check: \"" + value + "\".");
		}
	} else if (key == "allow_arg") {
		this->allow_strings.clear();
		this->allow_strings.push_back(value);
	} else {
		data_entry::process_gsml_property(property);
	}
}

void button::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "allow_arg") {
		this->allow_strings = values;
	} else if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition<CUnit>>();
		database::process_gsml_data(this->preconditions, scope);
		this->preconditions_ptr = this->preconditions.get();
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<CUnit>>();
		database::process_gsml_data(this->conditions, scope);
		this->conditions_ptr = this->conditions.get();
	} else if (tag == "unit_types") {
		for (const std::string &value : values) {
			if (this->UnitMask.empty()) {
				this->UnitMask += ",";
			}
			this->UnitMask += unit_type::get(value)->get_identifier();
			this->UnitMask += ",";
		}
	} else if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(unit_class::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void button::initialize()
{
	const button_level *level = nullptr;

	if (!this->ValueStr.empty()) {
		switch (this->Action) {
			case ButtonCmd::SpellCast:
				this->Value = spell::get(this->ValueStr)->Slot;
				break;
			case ButtonCmd::Train:
			case ButtonCmd::Build:
			case ButtonCmd::UpgradeTo:
			case ButtonCmd::ExperienceUpgradeTo:
				this->Value = UnitTypeIdByIdent(this->ValueStr);
				break;
			case ButtonCmd::TrainClass:
			case ButtonCmd::BuildClass:
			case ButtonCmd::UpgradeToClass:
				this->Value = unit_class::get(this->ValueStr)->get_index();
				break;
			case ButtonCmd::Research:
			case ButtonCmd::LearnAbility:
				this->Value = UpgradeIdByIdent(this->ValueStr);
				break;
			case ButtonCmd::ResearchClass:
				this->Value = upgrade_class::get(this->ValueStr)->get_index();
				break;
			case ButtonCmd::ProduceResource:
			case ButtonCmd::SellResource:
			case ButtonCmd::BuyResource:
				this->Value = resource::get(this->ValueStr)->get_index();
				break;
			case ButtonCmd::Button:
				if (!this->ValueStr.empty()) {
					level = button_level::get(this->ValueStr);
				}

				if (level != nullptr) {
					this->Value = level->get_index();
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

	if (this->Action == ButtonCmd::Faction) {
		++button::faction_button_count;
	}

	data_entry::initialize();
}

const CUnit *button::get_unit() const
{
	switch (this->Action) {
		case ButtonCmd::Buy:
		case ButtonCmd::Unit:
			return &wyrmgus::unit_manager::get()->GetSlotUnit(this->Value);
		default:
			if (!Selected.empty()) {
				return Selected[0];
			}
			return nullptr;
	}
}

const unit_type *button::get_unit_type(const CUnit *unit) const
{
	switch (this->Action) {
		case ButtonCmd::Train:
		case ButtonCmd::Build:
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::ExperienceUpgradeTo:
		case ButtonCmd::TrainClass:
		case ButtonCmd::BuildClass:
		case ButtonCmd::UpgradeToClass:
			return this->get_value_unit_type(unit);
		case ButtonCmd::Move:
		case ButtonCmd::Attack:
		case ButtonCmd::Repair:
		case ButtonCmd::Harvest:
		case ButtonCmd::Patrol:
		case ButtonCmd::AttackGround:
		case ButtonCmd::SpellCast:
		case ButtonCmd::Unload:
		case ButtonCmd::Stop:
		case ButtonCmd::StandGround:
		case ButtonCmd::Return:
		case ButtonCmd::RallyPoint:
		case ButtonCmd::Salvage:
		case ButtonCmd::Unit:
		case ButtonCmd::Buy:
			return unit->Type;
		default:
			return nullptr;
	}
}

const unit_type *button::get_value_unit_type(const CUnit *unit) const
{
	switch (this->Action) {
		case ButtonCmd::Train:
		case ButtonCmd::Build:
		case ButtonCmd::UpgradeTo:
		case ButtonCmd::ExperienceUpgradeTo:
			return unit_type::get_all()[this->Value];
		case ButtonCmd::TrainClass:
		case ButtonCmd::BuildClass:
		case ButtonCmd::UpgradeToClass: {
			const unit_class *unit_class = unit_class::get_all()[this->Value];
			if (unit->Player->get_faction() != nullptr) {
				return unit->Player->get_faction()->get_class_unit_type(unit_class);
			}
			break;
		}
		default:
			break;
	}

	return nullptr;
}

const CUpgrade *button::get_value_upgrade(const CUnit *unit) const
{
	switch (this->Action) {
		case ButtonCmd::Research:
		case ButtonCmd::LearnAbility:
			return CUpgrade::get_all()[this->Value];
		case ButtonCmd::ResearchClass: {
			const upgrade_class *upgrade_class = upgrade_class::get_all()[this->Value];
			if (unit->Player->get_faction() != nullptr) {
				return unit->Player->get_faction()->get_class_upgrade(upgrade_class);
			}
			break;
		}
		case ButtonCmd::Dynasty:
			if (this->Value != -1) {
				return CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[this->Value]->get_upgrade();
			}
		default:
			break;
	}

	return nullptr;
}

const resource *button::get_value_resource() const
{
	switch (this->Action) {
		case ButtonCmd::ProduceResource:
		case ButtonCmd::SellResource:
		case ButtonCmd::BuyResource:
			return resource::get_all()[this->Value];
		default:
			break;
	}

	return nullptr;
}

void button::SetTriggerData() const
{
	const unit_class *unit_class = nullptr;

	switch (this->Action) {
		case ButtonCmd::TrainClass:
		case ButtonCmd::BuildClass:
		case ButtonCmd::UpgradeToClass:
			unit_class = unit_class::get_all()[this->Value];
			if (Selected[0]->Player->get_faction() != nullptr) {
				TriggerData.Type = Selected[0]->Player->get_faction()->get_class_unit_type(unit_class);
			}
			break;
		case ButtonCmd::Research:
		case ButtonCmd::ResearchClass:
		case ButtonCmd::LearnAbility:
			TriggerData.Upgrade = this->get_value_upgrade(Selected[0]);
			break;
		case ButtonCmd::Unit:
		case ButtonCmd::Buy:
			TriggerData.Type = wyrmgus::unit_manager::get()->GetSlotUnit(this->Value).Type;
			TriggerData.Unit = &wyrmgus::unit_manager::get()->GetSlotUnit(this->Value);
			break;
		case ButtonCmd::Faction: {
			const faction *faction = CPlayer::GetThisPlayer()->get_faction()->get_develops_to().at(this->Value);
			TriggerData.faction = faction;
			if (faction->get_upgrade() != nullptr) {
				TriggerData.Upgrade = faction->get_upgrade();
			}
			break;
		}
		case ButtonCmd::PotentialNeutralFaction: {
			const faction *faction = Selected[0]->get_site()->get_neutral_factions().at(this->Value);
			TriggerData.faction = faction;
			if (faction->get_upgrade() != nullptr) {
				TriggerData.Upgrade = faction->get_upgrade();
			}
			break;
		}
		case ButtonCmd::Dynasty:
			TriggerData.dynasty = CPlayer::GetThisPlayer()->get_faction()->get_dynasties()[this->Value];
			TriggerData.Upgrade = TriggerData.dynasty->get_upgrade();
			break;
		case ButtonCmd::Player:
			TriggerData.player = CPlayer::Players.at(this->Value).get();
			break;
		case ButtonCmd::Tile:
			TriggerData.tile = CMap::get()->Field(this->Value, UI.CurrentMapLayer->ID);
			break;
		default:
			TriggerData.Type = unit_type::get_all()[this->Value];
			break;
	}
}

void button::CleanTriggerData() const
{
	TriggerData.Type = nullptr;
	TriggerData.Unit = nullptr;
	TriggerData.Upgrade = nullptr;
	TriggerData.resource = nullptr;
	TriggerData.faction = nullptr;
	TriggerData.dynasty = nullptr;
	TriggerData.player = nullptr;
	TriggerData.tile = nullptr;
}

int button::GetLevelID() const
{
	if (this->get_level() != nullptr) {
		return this->get_level()->get_index();
	} else {
		return 0;
	}
}

int button::get_key() const
{
	if ((this->Action == ButtonCmd::Build || this->Action == ButtonCmd::BuildClass || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::TrainClass || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::ResearchClass || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::UpgradeToClass) && !IsButtonUsable(*Selected[0], *this)) {
		return 0;
	}

	int key = this->Key;

	if (key == 0) {
		const CUnit *unit = this->get_unit();
		const unit_type *unit_type = this->get_value_unit_type(unit);
		const CUpgrade *upgrade = this->get_value_upgrade(unit);
		if (unit_type != nullptr) {
			key = GetHotKey(unit_type->get_default_button_key(unit->Player));
		} else if (upgrade != nullptr) {
			key = GetHotKey(upgrade->get_button_key());
		}
	}


	if (this->Key == gcn::Key::K_ESCAPE || this->Key == gcn::Key::K_DELETE || this->Key == gcn::Key::K_PAGE_DOWN || this->Key == gcn::Key::K_PAGE_UP) {
		return key;
	}
	
	if (this->has_position_based_hotkey() && this->Key != 0) {
		switch (this->get_pos()) {
			case 1:
				return 'q';
			case 2:
				return 'w';
			case 3:
				return 'e';
			case 4:
				return 'r';
			case 5:
				return 'a';
			case 6:
				return 's';
			case 7:
				return 'd';
			case 8:
				return 'f';
			case 9:
				return 'z';
			case 10:
				return 'x';
			case 11:
				return 'c';
			case 12:
				return 'v';
			case 13:
				return 't';
			case 14:
				return 'g';
			case 15:
				return 'b';
			case 16:
				return 'y';
			default:
				break;
		}
	}

	return key;
}

std::string button::get_hint() const
{
	std::string hint = this->Hint;

	bool show_key = true;
	if ((this->Action == ButtonCmd::Build || this->Action == ButtonCmd::BuildClass || this->Action == ButtonCmd::Train || this->Action == ButtonCmd::TrainClass || this->Action == ButtonCmd::Research || this->Action == ButtonCmd::ResearchClass || this->Action == ButtonCmd::LearnAbility || this->Action == ButtonCmd::ExperienceUpgradeTo || this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::UpgradeToClass) && !IsButtonUsable(*Selected[0], *this)) {
		if (!hint.empty()) {
			string::replace(hint, "~!", "");
			return hint;
		}
		show_key = false;
	}

	const CUnit *unit = this->get_unit();

	if (hint.empty()) {
		const unit_type *unit_type = this->get_value_unit_type(unit);
		const CUpgrade *upgrade = this->get_value_upgrade(unit);
		if (unit_type != nullptr) {
			if (this->Action == ButtonCmd::UpgradeTo || this->Action == ButtonCmd::UpgradeToClass) {
				hint = "Upgrade to ";
			} else {
				const bool hire = unit->Type->Stats[unit->Player->get_index()].has_hired_unit(unit_type);
				if (hire && !unit_type->BoolFlag[BUILDING_INDEX].value) {
					hint = "Hire ";
				} else {
					hint = unit_type->get_build_verb_string() + " ";
				}
			}

			std::string unit_type_name = unit_type->get_default_name(unit->Player);
			if (show_key && preferences::get()->get_hotkey_setup() == hotkey_setup::default_setup) {
				button::add_button_key_to_name(unit_type_name, unit_type->get_default_button_key(unit->Player));
			}
			hint += unit_type_name;
		} else if (upgrade != nullptr) {
			hint = "Research ";

			std::string upgrade_name = upgrade->get_name();
			if (show_key && preferences::get()->get_hotkey_setup() == hotkey_setup::default_setup) {
				button::add_button_key_to_name(upgrade_name, upgrade->get_button_key());
			}
			hint += upgrade_name;
		}
	}

	if (this->Key == gcn::Key::K_ESCAPE) {
		return hint;
	}
	
	if (this->has_position_based_hotkey() && this->Key != 0 && !hint.empty()) {
		string::replace(hint, "~!", "");
		hint += " (~!";
		hint += CapitalizeString(SdlKey2Str(this->get_key()));
		hint += ")";
		return hint;
	}
	
	return hint;
}

bool button::has_position_based_hotkey() const
{
	switch (preferences::get()->get_hotkey_setup()) {
		case hotkey_setup::position_based:
			return true;
		case hotkey_setup::position_based_except_commands:
			switch (this->Action) {
				case ButtonCmd::Build:
				case ButtonCmd::BuildClass:
				case ButtonCmd::Train:
				case ButtonCmd::TrainClass:
				case ButtonCmd::Research:
				case ButtonCmd::ResearchClass:
				case ButtonCmd::LearnAbility:
				case ButtonCmd::ExperienceUpgradeTo:
				case ButtonCmd::UpgradeTo:
				case ButtonCmd::UpgradeToClass:
				case ButtonCmd::RallyPoint:
				case ButtonCmd::Salvage:
				case ButtonCmd::EnterMapLayer:
					return true;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return false;
}

bool button::is_usable_when_paused() const
{
	switch (this->Action) {
		case ButtonCmd::Button:
		case ButtonCmd::ShowPopulation:
			return true;
		default:
			return false;
	}
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
		case ButtonCmd::BuildClass:
			return "build_class";
		case ButtonCmd::Train:
			return "train-unit";
		case ButtonCmd::TrainClass:
			return "train_unit_class";
		case ButtonCmd::Patrol:
			return "patrol";
		case ButtonCmd::StandGround:
			return "stand-ground";
		case ButtonCmd::AttackGround:
			return "attack-ground";
		case ButtonCmd::Return:
			return "return-goods";
		case ButtonCmd::SpellCast:
			return "cast_spell";
		case ButtonCmd::Research:
			return "research";
		case ButtonCmd::ResearchClass:
			return "research_class";
		case ButtonCmd::LearnAbility:
			return "learn_ability";
		case ButtonCmd::ExperienceUpgradeTo:
			return "experience_upgrade_to";
		case ButtonCmd::UpgradeTo:
			return "upgrade_to";
		case ButtonCmd::UpgradeToClass:
			return "upgrade_to_class";
		case ButtonCmd::Unload:
			return "unload";
		case ButtonCmd::RallyPoint:
			return "rally_point";
		case ButtonCmd::Faction:
			return "faction";
		case ButtonCmd::PotentialNeutralFaction:
			return "potential_neutral_faction";
		case ButtonCmd::Dynasty:
			return "dynasty";
		case ButtonCmd::Buy:
			return "buy";
		case ButtonCmd::ProduceResource:
			return "produce_resource";
		case ButtonCmd::SellResource:
			return "sell-resource";
		case ButtonCmd::BuyResource:
			return "buy-resource";
		case ButtonCmd::Salvage:
			return "salvage";
		case ButtonCmd::EnterMapLayer:
			return "enter_map_layer";
		case ButtonCmd::Unit:
			return "unit";
		case ButtonCmd::EditorUnit:
			return "editor-unit";
		case ButtonCmd::Tile:
			return "tile";
		case ButtonCmd::Cancel:
			return "cancel";
		case ButtonCmd::CancelUpgrade:
			return "cancel-upgrade";
		case ButtonCmd::CancelTrain:
			return "cancel-train-unit";
		case ButtonCmd::CancelBuild:
			return "cancel-build";
		case ButtonCmd::ShowPopulation:
			return "show_population";
		case ButtonCmd::Player:
			return "player";
		case ButtonCmd::CallbackAction:
			return "callback_action";
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
	} else if (button_action == "build_class") {
		return ButtonCmd::BuildClass;
	} else if (button_action == "train-unit") {
		return ButtonCmd::Train;
	} else if (button_action == "train_unit_class") {
		return ButtonCmd::TrainClass;
	} else if (button_action == "patrol") {
		return ButtonCmd::Patrol;
	} else if (button_action == "stand-ground" || button_action == "stand_ground") {
		return ButtonCmd::StandGround;
	} else if (button_action == "attack-ground") {
		return ButtonCmd::AttackGround;
	} else if (button_action == "return-goods" || button_action == "return_goods") {
		return ButtonCmd::Return;
	} else if (button_action == "cast_spell" || button_action == "cast-spell") {
		return ButtonCmd::SpellCast;
	} else if (button_action == "research") {
		return ButtonCmd::Research;
	} else if (button_action == "research_class") {
		return ButtonCmd::ResearchClass;
	} else if (button_action == "learn_ability" || button_action == "learn-ability") {
		return ButtonCmd::LearnAbility;
	} else if (button_action == "experience_upgrade_to") {
		return ButtonCmd::ExperienceUpgradeTo;
	} else if (button_action == "upgrade_to" || button_action == "upgrade-to") {
		return ButtonCmd::UpgradeTo;
	} else if (button_action == "upgrade_to_class") {
		return ButtonCmd::UpgradeToClass;
	} else if (button_action == "unload") {
		return ButtonCmd::Unload;
	} else if (button_action == "rally_point") {
		return ButtonCmd::RallyPoint;
	} else if (button_action == "faction") {
		return ButtonCmd::Faction;
	} else if (button_action == "potential_neutral_faction") {
		return ButtonCmd::PotentialNeutralFaction;
	} else if (button_action == "dynasty") {
		return ButtonCmd::Dynasty;
	} else if (button_action == "buy") {
		return ButtonCmd::Buy;
	} else if (button_action == "produce_resource") {
		return ButtonCmd::ProduceResource;
	} else if (button_action == "sell-resource") {
		return ButtonCmd::SellResource;
	} else if (button_action == "buy-resource") {
		return ButtonCmd::BuyResource;
	} else if (button_action == "salvage") {
		return ButtonCmd::Salvage;
	} else if (button_action == "enter_map_layer") {
		return ButtonCmd::EnterMapLayer;
	} else if (button_action == "unit") {
		return ButtonCmd::Unit;
	} else if (button_action == "editor-unit") {
		return ButtonCmd::EditorUnit;
	} else if (button_action == "tile") {
		return ButtonCmd::Tile;
	} else if (button_action == "cancel") {
		return ButtonCmd::Cancel;
	} else if (button_action == "cancel-upgrade") {
		return ButtonCmd::CancelUpgrade;
	} else if (button_action == "cancel-train-unit") {
		return ButtonCmd::CancelTrain;
	} else if (button_action == "cancel-build") {
		return ButtonCmd::CancelBuild;
	} else if (button_action == "show_population") {
		return ButtonCmd::ShowPopulation;
	}

	return ButtonCmd::None;
}

bool IsNeutralUsableButtonAction(const ButtonCmd button_action)
{
	switch (button_action) {
		case ButtonCmd::Train:
		case ButtonCmd::TrainClass:
		case ButtonCmd::CancelTrain:
		case ButtonCmd::Buy:
		case ButtonCmd::SellResource:
		case ButtonCmd::BuyResource:
		case ButtonCmd::Research:
		case ButtonCmd::ResearchClass:
		case ButtonCmd::PotentialNeutralFaction:
		case ButtonCmd::Button:
			return true;
		default:
			return false;
	}
}
