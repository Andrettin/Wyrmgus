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
//      (c) Copyright 2019-2022 by Andrettin
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

namespace wyrmgus {

class gsml_data;
class gsml_property;

class data_module final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring)

public:
	explicit data_module(const std::string &identifier, const std::filesystem::path &path, const data_module *parent_module)
		: identifier(identifier), path(path), parent_module(parent_module)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const std::string &get_identifier() const
	{
		return this->identifier;
	}

	const std::string &get_name() const
	{
		return this->name;
	}

	Q_INVOKABLE void set_name(const std::string &name)
	{
		this->name = name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	const std::filesystem::path &get_path() const
	{
		return this->path;
	}

	void add_dependency(const data_module *data_module)
	{
		if (data_module->depends_on(this)) {
			throw std::runtime_error("Cannot make module \"" + this->identifier + "\" depend on module \"" + data_module->identifier + "\", as that would create a circular dependency.");
		}

		this->dependencies.insert(data_module);
	}

	bool depends_on(const data_module *data_module) const
	{
		if (data_module == this->parent_module) {
			return true;
		}

		if (this->dependencies.contains(data_module)) {
			return true;
		}

		for (const wyrmgus::data_module *dependency : this->dependencies) {
			if (dependency->depends_on(data_module)) {
				return true;
			}
		}

		if (this->parent_module != nullptr) {
			return this->parent_module->depends_on(data_module);
		}

		return false;
	}

	size_t get_dependency_count() const
	{
		size_t count = this->dependencies.size();

		for (const wyrmgus::data_module *dependency : this->dependencies) {
			count += dependency->get_dependency_count();
		}

		return count;
	}

private:
	std::string identifier;
	std::string name;
	std::filesystem::path path; //the module's path
	const data_module *parent_module = nullptr;
	std::set<const data_module *> dependencies; //modules on which this one is dependent
};

}
