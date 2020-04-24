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

#include "database/sml_data.h"
#include "util/qunique_ptr.h"
#include "util/singleton.h"
#include "util/type_traits.h"

namespace stratagus {

class data_entry;
class data_type_metadata;
class module;

class database final : public singleton<database>
{
public:
	static constexpr const char *data_folder = "data";
	static constexpr const char *graphics_folder = "graphics";
	static constexpr const char *maps_folder = "maps";
	static constexpr const char *sounds_folder = "sounds";

	template <typename T>
	static void process_sml_data(T *instance, const sml_data &data)
	{
		data.for_each_element([&](const sml_property &property) {
			instance->process_sml_property(property);
		}, [&](const sml_data &scope) {
			instance->process_sml_scope(scope);
		});
	}

	template <typename T>
	static void process_sml_data(T &instance, const sml_data &data)
	{
		if constexpr (is_specialization_of_v<T, std::unique_ptr>) {
			database::process_sml_data(instance.get(), data);
		} else {
			database::process_sml_data(&instance, data);
		}
	}

	template <typename T>
	static void process_sml_data(const std::unique_ptr<T> &instance, const sml_data &data)
	{
		database::process_sml_data(instance.get(), data);
	}

	template <typename T>
	static void process_sml_data(const qunique_ptr<T> &instance, const sml_data &data)
	{
		database::process_sml_data(instance.get(), data);
	}

	static void process_sml_property_for_object(QObject *object, const sml_property &property);
	static QVariant process_sml_property_value(const sml_property &property, const QMetaProperty &meta_property, const QObject *object);
	static void process_sml_scope_for_object(QObject *object, const sml_data &scope);
	static QVariant process_sml_scope_value(const sml_data &scope, const QMetaProperty &meta_property);
	static void modify_list_property_for_object(QObject *object, const std::string &property_name, const sml_operator sml_operator, const std::string &value);

	static std::filesystem::path get_root_path()
	{
		return std::filesystem::current_path();
	}

	static std::filesystem::path get_modules_path()
	{
		return database::get_root_path() / "modules";
	}

	static std::filesystem::path get_documents_modules_path()
	{
		return database::get_documents_path() / "modules";
	}

	static std::filesystem::path get_documents_path()
	{
		std::string documents_path_str = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toStdString();
		if (documents_path_str.empty()) {
			throw std::runtime_error("No writable documents path found.");
		}

		std::filesystem::path documents_path(documents_path_str +  "/Wyrmsun");
		return documents_path;
	}

	static std::filesystem::path get_base_path(const module *module);

	static std::filesystem::path get_graphics_path(const module *module)
	{
		return database::get_base_path(module) / database::graphics_folder;
	}

	static std::filesystem::path get_maps_path(const module *module)
	{
		return database::get_base_path(module) / database::maps_folder;
	}

	static std::filesystem::path get_sounds_path(const module *module)
	{
		return database::get_base_path(module) / database::sounds_folder;
	}

	static void parse_folder(const std::filesystem::path &path, std::vector<sml_data> &sml_data_list);

public:
	database();
	~database();

	void parse();
	void load(const bool initial_definition);
	void load_defines();
	void initialize();
	void clear();
	void register_metadata(std::unique_ptr<data_type_metadata> &&metadata);

	void process_modules();
	void process_modules_at_dir(const std::filesystem::path &path, module *parent_module = nullptr);
	std::vector<std::filesystem::path> get_module_paths() const;
	std::vector<std::pair<std::filesystem::path, const module *>> get_module_paths_with_module() const;

	module *get_module(const std::string &identifier) const
	{
		auto find_iterator = this->modules_by_identifier.find(identifier);
		if (find_iterator != this->modules_by_identifier.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No module found with identifier \"" + identifier + "\".");
	}

	std::vector<std::filesystem::path> get_base_paths() const
	{
		std::vector<std::filesystem::path> base_paths;
		base_paths.push_back(database::get_root_path());

		std::vector<std::filesystem::path> module_paths = this->get_module_paths();
		base_paths.insert(base_paths.end(), module_paths.begin(), module_paths.end());

		return base_paths;
	}

	std::vector<std::pair<std::filesystem::path, const module *>> get_base_paths_with_module() const
	{
		std::vector<std::pair<std::filesystem::path, const module *>> base_paths;
		base_paths.emplace_back(database::get_root_path(), nullptr);

		std::vector<std::pair<std::filesystem::path, const module *>> module_paths = this->get_module_paths_with_module();
		base_paths.insert(base_paths.end(), module_paths.begin(), module_paths.end());

		return base_paths;
	}

	std::vector<std::filesystem::path> get_data_paths() const
	{
		std::vector<std::filesystem::path> paths = this->get_base_paths();

		for (std::filesystem::path &path : paths) {
			path /= database::data_folder;
		}

		return paths;
	}

	std::vector<std::pair<std::filesystem::path, const module *>> get_data_paths_with_module() const
	{
		std::vector<std::pair<std::filesystem::path, const module *>> paths = this->get_base_paths_with_module();

		for (auto &kv_pair : paths) {
			std::filesystem::path &path = kv_pair.first;
			path /= database::data_folder;
		}

		return paths;
	}

	std::vector<std::filesystem::path> get_graphics_paths() const
	{
		std::vector<std::filesystem::path> paths = this->get_base_paths();

		for (std::filesystem::path &path : paths) {
			path /= database::graphics_folder;
		}

		return paths;
	}

private:
	std::vector<std::unique_ptr<data_type_metadata>> metadata;
	std::vector<qunique_ptr<module>> modules;
	std::map<std::string, module *> modules_by_identifier;
};

}
