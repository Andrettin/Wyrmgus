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
//

#pragma once

#include "database/sml_data.h"
#include "util/qunique_ptr.h"
#include "util/singleton.h"
#include "util/type_traits.h"

namespace wyrmgus {

class data_entry;
class data_module;
class data_type_metadata;

class database final : public singleton<database>
{
public:
	static constexpr const char *data_folder = "data";
	static constexpr const char *graphics_folder = "graphics";
	static constexpr const char *maps_folder = "maps";
	static constexpr const char *music_folder = "music";
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
	static void modify_list_property_for_object(QObject *object, const std::string &property_name, const sml_operator sml_operator, const sml_data &scope);

	const std::filesystem::path &get_root_path() const
	{
		return this->root_path;
	}

	void set_root_path(const std::filesystem::path &path)
	{
		this->root_path = path;
	}

	std::filesystem::path get_modules_path() const
	{
		return this->get_root_path() / "modules";
	}

	std::filesystem::path get_dlcs_path() const
	{
		return this->get_root_path() / "dlcs";
	}

	static std::filesystem::path get_documents_modules_path()
	{
		return database::get_documents_path() / "modules";
	}

	static std::filesystem::path get_documents_path();
	static std::filesystem::path get_user_data_path();
	static void ensure_path_exists(const std::filesystem::path &path);

	const std::filesystem::path &get_base_path(const data_module *data_module) const;

	std::filesystem::path get_graphics_path(const data_module *data_module) const
	{
		return this->get_base_path(data_module) / database::graphics_folder;
	}

	std::filesystem::path get_maps_path(const data_module *data_module) const
	{
		return this->get_base_path(data_module) / database::maps_folder;
	}

	std::filesystem::path get_music_path(const data_module *data_module)
	{
		return this->get_base_path(data_module) / database::music_folder;
	}

	std::filesystem::path get_sounds_path(const data_module *data_module)
	{
		return this->get_base_path(data_module) / database::sounds_folder;
	}

	static void parse_folder(const std::filesystem::path &path, std::vector<sml_data> &sml_data_list);

public:
	database();
	~database();

	void parse();
	void load(const bool initial_definition);
	void load_predefines();
	void load_defines();
	static void load_history();

	bool is_initialized() const
	{
		return this->initialized;
	}

	void initialize();

	void clear();
	void register_metadata(std::unique_ptr<data_type_metadata> &&metadata);

	void process_modules();
	void process_modules_at_dir(const std::filesystem::path &path, data_module *parent_module = nullptr);
	std::vector<std::filesystem::path> get_module_paths() const;
	std::vector<std::pair<std::filesystem::path, const data_module *>> get_module_paths_with_module() const;

	data_module *get_module(const std::string &identifier) const
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
		base_paths.push_back(this->get_root_path());

		std::vector<std::filesystem::path> module_paths = this->get_module_paths();
		base_paths.insert(base_paths.end(), module_paths.begin(), module_paths.end());

		return base_paths;
	}

	std::vector<std::pair<std::filesystem::path, const data_module *>> get_base_paths_with_module() const
	{
		std::vector<std::pair<std::filesystem::path, const data_module *>> base_paths;
		base_paths.emplace_back(this->get_root_path(), nullptr);

		std::vector<std::pair<std::filesystem::path, const data_module *>> module_paths = this->get_module_paths_with_module();
		base_paths.insert(base_paths.end(), module_paths.begin(), module_paths.end());

		return base_paths;
	}

	std::filesystem::path get_data_path() const
	{
		return this->get_root_path() / database::data_folder;
	}

	std::vector<std::filesystem::path> get_data_paths() const
	{
		std::vector<std::filesystem::path> paths = this->get_base_paths();

		for (std::filesystem::path &path : paths) {
			path /= database::data_folder;
		}

		return paths;
	}

	std::vector<std::pair<std::filesystem::path, const data_module *>> get_data_paths_with_module() const
	{
		std::vector<std::pair<std::filesystem::path, const data_module *>> paths = this->get_base_paths_with_module();

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

	std::vector<std::filesystem::path> get_maps_paths() const
	{
		std::vector<std::filesystem::path> paths = this->get_base_paths();

		for (std::filesystem::path &path : paths) {
			path /= database::maps_folder;
		}

		return paths;
	}

private:
	std::filesystem::path root_path = std::filesystem::current_path();
	std::vector<std::unique_ptr<data_type_metadata>> metadata;
	std::vector<qunique_ptr<data_module>> modules;
	std::map<std::string, data_module *> modules_by_identifier;
	bool initialized = false;
};

}
