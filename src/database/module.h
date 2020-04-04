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
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <QObject>
#include <QVariantList>

#include <filesystem>
#include <set>
#include <string>

namespace stratagus {

class sml_data;
class sml_property;

class module final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring WRITE set_name_qstring)
	Q_PROPERTY(QVariantList dependencies READ get_dependencies_qvariant_list)

public:
	module(const std::string &identifier, const std::filesystem::path &path, module *parent_module)
		: identifier(identifier), path(path), parent_module(parent_module)
	{
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope) { Q_UNUSED(scope) }

	const std::string &get_identifier() const
	{
		return this->identifier;
	}

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	void set_name_qstring(const QString &name)
	{
		this->name = name.toStdString();
	}

	const std::filesystem::path &get_path() const
	{
		return this->path;
	}

	QVariantList get_dependencies_qvariant_list() const;

	Q_INVOKABLE void add_dependency(module *module)
	{
		if (module->depends_on(this)) {
			throw std::runtime_error("Cannot make module \"" + this->identifier + "\" depend on module \"" + module->identifier + "\", as that would create a circular dependency.");
		}

		this->dependencies.insert(module);
	}

	Q_INVOKABLE void remove_dependency(module *module)
	{
		this->dependencies.erase(module);
	}

	bool depends_on(module *module) const
	{
		if (module == this->parent_module) {
			return true;
		}

		if (this->dependencies.contains(module)) {
			return true;
		}

		for (const stratagus::module *dependency : this->dependencies) {
			if (dependency->depends_on(module)) {
				return true;
			}
		}

		if (this->parent_module != nullptr) {
			return this->parent_module->depends_on(module);
		}

		return false;
	}

	size_t get_dependency_count() const
	{
		size_t count = this->dependencies.size();

		for (const stratagus::module *dependency : this->dependencies) {
			count += dependency->get_dependency_count();
		}

		return count;
	}

private:
	std::string identifier;
	std::string name;
	std::filesystem::path path; //the module's path
	module *parent_module = nullptr;
	std::set<module *> dependencies; //modules on which this one is dependent
};

}
