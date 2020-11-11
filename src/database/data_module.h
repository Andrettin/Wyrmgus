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

namespace wyrmgus {

class sml_data;
class sml_property;

class data_module final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring)

public:
	explicit data_module(const std::string &identifier, const std::filesystem::path &path, const data_module *parent_module)
		: identifier(identifier), path(path), parent_module(parent_module)
	{
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

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
