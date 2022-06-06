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

#include "database/gsml_data.h"
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
	static constexpr const char *campaign_folder = "campaign";
	static constexpr const char *campaign_map_filename = "campaign.smp";
	static constexpr const char *data_folder = "data";
	static constexpr const char *graphics_folder = "graphics";
	static constexpr const char *maps_folder = "maps";
	static constexpr const char *music_folder = "music";
	static constexpr const char *save_folder = "save";
	static constexpr const char *shaders_folder = "shaders";
	static constexpr const char *sounds_folder = "sounds";

	template <typename T>
	static void process_gsml_data(T *instance, const gsml_data &data)
	{
		data.for_each_element([&](const gsml_property &property) {
			instance->process_gsml_property(property);
		}, [&](const gsml_data &scope) {
			instance->process_gsml_scope(scope);
		});
	}

	template <typename T>
	static void process_gsml_data(T &instance, const gsml_data &data)
	{
		if constexpr (is_specialization_of_v<T, std::unique_ptr>) {
			database::process_gsml_data(instance.get(), data);
		} else {
			database::process_gsml_data(&instance, data);
		}
	}

	template <typename T>
	static void process_gsml_data(const std::unique_ptr<T> &instance, const gsml_data &data)
	{
		database::process_gsml_data(instance.get(), data);
	}

	template <typename T>
	static void process_gsml_data(const qunique_ptr<T> &instance, const gsml_data &data)
	{
		database::process_gsml_data(instance.get(), data);
	}

	static void process_gsml_property_for_object(QObject *object, const gsml_property &property);
	static QVariant process_gsml_property_value(const gsml_property &property, const QMetaProperty &meta_property, const QObject *object);
	static void process_gsml_scope_for_object(QObject *object, const gsml_data &scope);
	static QVariant process_gsml_scope_value(const gsml_data &scope, const QMetaProperty &meta_property);
	static void modify_list_property_for_object(QObject *object, const std::string &property_name, const gsml_operator gsml_operator, const std::string &value);
	static void modify_list_property_for_object(QObject *object, const std::string &property_name, const gsml_operator gsml_operator, const gsml_data &scope);

	static std::filesystem::path get_documents_modules_path()
	{
		return database::get_documents_path() / "modules";
	}

	static std::filesystem::path get_documents_path();
	static std::filesystem::path get_user_data_path();

	static std::filesystem::path get_user_maps_path()
	{
		std::filesystem::path path = database::get_user_data_path() / database::maps_folder;
		database::ensure_path_exists(path);
		return path;
	}

	static std::filesystem::path get_save_path()
	{
		std::filesystem::path path = database::get_user_data_path() / database::save_folder;
		database::ensure_path_exists(path);
		return path;
	}

	static void ensure_path_exists(const std::filesystem::path &path);

	[[nodiscard]]
	static boost::asio::awaitable<void> parse_folder(const std::filesystem::path &path, std::vector<gsml_data> &gsml_data_list);

public:
	database();
	~database();

	[[nodiscard]]
	boost::asio::awaitable<void> parse();

	[[nodiscard]]
	boost::asio::awaitable<void> load(const bool initial_definition);

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

	const std::vector<qunique_ptr<data_module>> &get_modules() const
	{
		return this->modules;
	}

	data_module *get_module(const std::string &identifier) const
	{
		const auto find_iterator = this->modules_by_identifier.find(identifier);
		if (find_iterator != this->modules_by_identifier.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No module found with identifier \"" + identifier + "\".");
	}

	bool has_module(const std::string &identifier) const
	{
		return this->modules_by_identifier.contains(identifier);
	}

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

	std::filesystem::path get_workshop_path() const
	{
		return this->get_root_path().parent_path().parent_path() / "workshop" / "content" / "370070";
	}

	const std::filesystem::path &get_base_path(const data_module *data_module) const;

	std::filesystem::path get_graphics_path(const data_module *data_module) const
	{
		return this->get_base_path(data_module) / database::graphics_folder;
	}

	std::filesystem::path get_maps_path(const data_module *data_module) const
	{
		return this->get_base_path(data_module) / database::maps_folder;
	}

	std::filesystem::path get_campaign_maps_path() const
	{
		return this->get_maps_path(nullptr) / database::campaign_folder;
	}

	std::filesystem::path get_campaign_map_filepath() const
	{
		std::filesystem::path filepath = this->get_campaign_maps_path() / campaign_map_filename;
		filepath.make_preferred();
		return filepath;
	}

	std::filesystem::path get_music_path(const data_module *data_module)
	{
		return this->get_base_path(data_module) / database::music_folder;
	}

	std::filesystem::path get_sounds_path(const data_module *data_module)
	{
		return this->get_base_path(data_module) / database::sounds_folder;
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

		paths.push_back(database::get_user_maps_path());

		return paths;
	}

	std::filesystem::path get_graphics_filepath(const std::filesystem::path &relative_filepath) const
	{
		const data_module *preferred_module = this->get_current_module();

		const std::filesystem::path preferred_path = this->get_graphics_path(preferred_module) / relative_filepath;

		if (std::filesystem::exists(preferred_path)) {
			return preferred_path;
		}

		const std::vector<std::filesystem::path> graphics_paths = this->get_graphics_paths();

		//iterate the graphics paths in reverse other, so that modules loaded later can override graphics of those loaded earlier
		for (auto iterator = graphics_paths.rbegin(); iterator != graphics_paths.rend(); ++iterator) {
			std::filesystem::path filepath = *iterator / relative_filepath;

			if (std::filesystem::exists(filepath)) {
				return filepath;
			}
		}

		throw std::runtime_error("Graphics file \"" + relative_filepath.string() + "\" not found.");
	}

	std::filesystem::path get_shaders_path() const
	{
		return this->get_root_path() / database::shaders_folder;
	}

	const data_module *get_current_module() const
	{
		return this->current_module;
	}

	void set_current_module(const data_module *data_module)
	{
		this->current_module = data_module;
	}

private:
	std::filesystem::path root_path = std::filesystem::current_path();
	std::vector<std::unique_ptr<data_type_metadata>> metadata;
	std::vector<qunique_ptr<data_module>> modules;
	std::map<std::string, data_module *> modules_by_identifier;
	const data_module *current_module = nullptr; //the module currently being processed
	bool initialized = false;
};

}
