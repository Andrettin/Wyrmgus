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
//      (c) Copyright 2019-2021 by Andrettin
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

class CPlayer;
class CUnit;

namespace wyrmgus {

class dialogue;
class sml_data;
class sml_property;
enum class sml_operator;
struct context;
struct read_only_context;

static constexpr const char *no_effect_string = "No effect";

//a scripted effect
template <typename scope_type>
class effect
{
public:
	static std::unique_ptr<effect> from_sml_property(const sml_property &property);
	static std::unique_ptr<effect> from_sml_scope(const sml_data &scope);

	explicit effect(const sml_operator effect_operator);

	virtual ~effect()
	{
	}

	virtual const std::string &get_class_identifier() const = 0;

	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);

	virtual void check() const
	{
	}

	void do_effect(scope_type *scope, const context &ctx) const;

	virtual void do_assignment_effect(scope_type *scope) const
	{
		Q_UNUSED(scope)

		throw std::runtime_error("The assignment operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual void do_assignment_effect(scope_type *scope, const context &ctx) const
	{
		Q_UNUSED(ctx)

		this->do_assignment_effect(scope);
	}

	virtual void do_addition_effect(scope_type *scope) const
	{
		Q_UNUSED(scope)

		throw std::runtime_error("The addition operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual void do_subtraction_effect(scope_type *scope) const
	{
		Q_UNUSED(scope)

		throw std::runtime_error("The subtraction operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	std::string get_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const;

	virtual std::string get_assignment_string() const
	{
		throw std::runtime_error("The assignment operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		Q_UNUSED(scope)
		Q_UNUSED(ctx)
		Q_UNUSED(indent)
		Q_UNUSED(prefix)

		return this->get_assignment_string();
	}

	virtual std::string get_addition_string() const
	{
		throw std::runtime_error("The addition operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual std::string get_subtraction_string() const
	{
		throw std::runtime_error("The subtraction operator is not supported for \"" + this->get_class_identifier() + "\" effects.");
	}

	virtual bool is_hidden() const
	{
		return false;
	}

private:
	sml_operator effect_operator;
};

extern template class effect<CPlayer>;
extern template class effect<CUnit>;

}
