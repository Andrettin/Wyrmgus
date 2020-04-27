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
//      (c) Copyright 2019-2020 by Andrettin
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

class CConfigData;
class CPlayer;
class CUnitType;

namespace stratagus {

class dialogue;
class sml_data;
class sml_property;

//a scripted effect
class effect
{
public:
	static std::unique_ptr<effect> from_sml_property(const sml_property &property);
	static std::unique_ptr<effect> from_sml_scope(const sml_data &scope);

	virtual const std::string &get_class_identifier() const = 0;

	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);
	virtual void do_effect(CPlayer *player) const = 0;
	virtual std::string get_string(const CPlayer *player) const = 0;

	virtual bool is_hidden() const
	{
		return false;
	}
};

class call_dialogue_effect final : public effect
{
public:
	call_dialogue_effect() {}
	explicit call_dialogue_effect(const std::string &dialogue_identifier);

	virtual const std::string &get_class_identifier() const override
	{
		static std::string class_identifier = "call_dialogue";
		return class_identifier;
	}

	virtual void do_effect(CPlayer *player) const override;
	virtual std::string get_string(const CPlayer *player) const override;

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const dialogue *dialogue = nullptr;
};

class create_unit_effect final : public effect
{
public:
	explicit create_unit_effect(const std::string &unit_type_identifier);

	virtual const std::string &get_class_identifier() const override
	{
		static std::string class_identifier = "create_unit";
		return class_identifier;
	}

	virtual void do_effect(CPlayer *player) const override;
	virtual std::string get_string(const CPlayer *player) const override;

private:
	const CUnitType *unit_type = nullptr;	/// Unit type to be created
};

}
