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

#include "database/data_module_container.h"
#include "database/data_type_metadata.h"
#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "util/qunique_ptr.h"

namespace wyrmgus {

class data_module;

class data_type_base
{
public:
	static inline const std::set<std::string> database_dependencies; //the other classes on which this one depends, i.e. after which this class' database can be processed
};

template <typename T>
class data_type : public data_type_base
{
public:
	data_type()
	{
		//this check is required for class_initialized variable and, correspondingly,
		//the data_type::initialize_class() call to not to be initialized away
		if (!data_type::class_initialized) {
			throw std::runtime_error("Never reached.");
		}
	}

	static T *get(const std::string &identifier)
	{
		if (identifier == "none") {
			return nullptr;
		}

		T *instance = T::try_get(identifier);

		if (instance == nullptr) {
			throw std::runtime_error("Invalid " + std::string(T::class_identifier) + " instance: \"" + identifier + "\".");
		}

		return instance;
	}

	static T *try_get(const std::string &identifier)
	{
		if (identifier == "none") {
			return nullptr;
		}

		const auto find_iterator = data_type::instances_by_identifier.find(identifier);
		if (find_iterator != data_type::instances_by_identifier.end()) {
			return find_iterator->second.get();
		}

		const auto alias_find_iterator = data_type::instances_by_alias.find(identifier);
		if (alias_find_iterator != data_type::instances_by_alias.end()) {
			return alias_find_iterator->second;
		}

		return nullptr;
	}

	static T *get_or_add(const std::string &identifier, const data_module *data_module)
	{
		T *instance = T::try_get(identifier);
		if (instance != nullptr) {
			return instance;
		}

		return T::add(identifier, data_module);
	}

	static const std::vector<T *> &get_all()
	{
		return data_type::instances;
	}

	static bool exists(const std::string &identifier)
	{
		return data_type::instances_by_identifier.contains(identifier) || data_type::instances_by_alias.contains(identifier);
	}

	static T *add(const std::string &identifier, const data_module *data_module)
	{
		if (identifier.empty()) {
			throw std::runtime_error("Tried to add a " + std::string(T::class_identifier) + " instance with an empty string identifier.");
		}

		if (T::exists(identifier)) {
			throw std::runtime_error("Tried to add a " + std::string(T::class_identifier) + " instance with the already-used \"" + identifier + "\" string identifier.");
		}

		data_type::instances_by_identifier[identifier] = make_qunique<T>(identifier);

		T *instance = data_type::instances_by_identifier.find(identifier)->second.get();
		data_type::instances.push_back(instance);
		instance->moveToThread(QApplication::instance()->thread());
		instance->set_module(data_module);

		//for backwards compatibility, change instances of "_" in the identifier with "-" and add that as an alias, and do the opposite as well
		if (identifier.find("_") != std::string::npos) {
			std::string alias = identifier;
			std::replace(alias.begin(), alias.end(), '_', '-');
			T::add_instance_alias(instance, alias);
		}
		
		if (identifier.find("-") != std::string::npos) {
			std::string alias = identifier;
			std::replace(alias.begin(), alias.end(), '-', '_');
			T::add_instance_alias(instance, alias);
		}

		return instance;
	}

	static void add_instance_alias(T *instance, const std::string &alias)
	{
		if (alias.empty()) {
			throw std::runtime_error("Tried to add a " + std::string(T::class_identifier) + " instance empty alias.");
		}

		if (T::exists(alias)) {
			throw std::runtime_error("Tried to add a " + std::string(T::class_identifier) + " alias with the already-used \"" + alias + "\" string identifier.");
		}

		data_type::instances_by_alias[alias] = instance;
		instance->add_alias(alias);
	}

	static void remove(T *instance)
	{
		data_type::instances.erase(std::remove(data_type::instances.begin(), data_type::instances.end(), instance), data_type::instances.end());

		data_type::instances_by_identifier.erase(instance->get_identifier());
	}

	static void remove(const std::string &identifier)
	{
		T::remove(T::get(identifier));
	}

	static void clear()
	{
		data_type::instances.clear();
		data_type::instances_by_alias.clear();
		data_type::instances_by_identifier.clear();
	}

	template <typename function_type>
	static void sort_instances(const function_type &function)
	{
		std::sort(data_type::instances.begin(), data_type::instances.end(), function);
	}

	[[nodiscard]]
	static boost::asio::awaitable<void> parse_database(const std::filesystem::path &data_path, const data_module *data_module)
	{
		if (std::string(T::database_folder).empty()) {
			co_return;
		}

		const std::filesystem::path database_path(data_path / T::database_folder);

		if (!std::filesystem::exists(database_path)) {
			co_return;
		}

		co_await database::parse_folder(database_path, data_type::gsml_data_to_process[data_module]);
	}

	static void process_database(const bool definition)
	{
		if (std::string(T::database_folder).empty()) {
			return;
		}

		for (const auto &kv_pair : data_type::gsml_data_to_process) {
			const data_module *data_module = kv_pair.first;

			database::get()->set_current_module(data_module);

			const std::vector<gsml_data> &gsml_data_list = kv_pair.second;
			for (const gsml_data &data : gsml_data_list) {
				data.for_each_child([&](const gsml_data &data_entry) {
					const std::string &identifier = data_entry.get_tag();

					T *instance = nullptr;
					if (definition) {
						if (data_entry.get_operator() != gsml_operator::addition) {
							//addition operators for data entry scopes mean modifying already-defined entries
							instance = T::add(identifier, data_module);
						} else {
							instance = T::get(identifier);
						}

						for (const gsml_property *alias_property : data_entry.try_get_properties("aliases")) {
							if (alias_property->get_operator() != gsml_operator::addition) {
								throw std::runtime_error("Only the addition operator is supported for data entry aliases.");
							}

							const std::string &alias = alias_property->get_value();
							T::add_instance_alias(instance, alias);

							//for backwards compatibility, change instances of "_" in the identifier with "-" and add that as a further alias, and do the opposite as well
							if (alias.find("_") != std::string::npos) {
								std::string other_alias = alias;
								std::replace(other_alias.begin(), other_alias.end(), '_', '-');
								T::add_instance_alias(instance, other_alias);
							} else if (alias.find("-") != std::string::npos) {
								std::string other_alias = alias;
								std::replace(other_alias.begin(), other_alias.end(), '-', '_');
								T::add_instance_alias(instance, other_alias);
							}
						}
					} else {
						try {
							instance = T::get(identifier);
							database::process_gsml_data<T>(instance, data_entry);
							instance->set_defined(true);
						} catch (...) {
							std::throw_with_nested(std::runtime_error("Error processing or loading data for " + std::string(T::class_identifier) + " instance \"" + identifier + "\"."));
						}
					}
				});
			}
		}

		database::get()->set_current_module(nullptr);

		if (!definition) {
			data_type::gsml_data_to_process.clear();
		}
	}

	static void load_history_database()
	{
		try {
			for (T *instance : T::get_all()) {
				try {
					instance->load_history();
				} catch (...) {
					std::throw_with_nested(std::runtime_error("Error loading history for the " + std::string(T::class_identifier) + " instance \"" + instance->get_identifier() + "\"."));
				}
			}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error loading history for the " + std::string(T::class_identifier) + " class."));
		}
	}

	static void initialize_all()
	{
		for (T *instance : T::get_all()) {
			if (instance->is_initialized()) {
				continue; //the instance might have been initialized already, e.g. in the initialization function of another instance which needs it to be initialized
			}

			try {
				instance->initialize();
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to initialize the " + std::string(T::class_identifier) + " instance \"" + instance->get_identifier() + "\"."));
			}

			if (!instance->is_initialized()) {
				throw std::runtime_error("The " + std::string(T::class_identifier) + " instance \"" + instance->get_identifier() + "\" is not marked as initialized despite the initialization function having been called for it.");
			}
		}
	}

	static void process_all_text()
	{
		for (T *instance : T::get_all()) {
			try {
				instance->process_text();
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to process text for the " + std::string(T::class_identifier) + " instance \"" + instance->get_identifier() + "\"."));
			}
		}
	}

	static void check_all()
	{
		for (const T *instance : T::get_all()) {
			try {
				instance->check();
			} catch (...) {
				std::throw_with_nested(std::runtime_error("The validity check for the " + std::string(T::class_identifier) + " instance \"" + instance->get_identifier() + "\" failed."));
			}
		}
	}

	static std::vector<T *> get_encyclopedia_entries()
	{
		std::vector<T *> entries;

		for (T *instance : T::get_all()) {
			if (instance->has_encyclopedia_entry()) {
				entries.push_back(instance);
			}
		}

		T::sort_encyclopedia_entries(entries);

		return entries;
	}

	static void sort_encyclopedia_entries(std::vector<T *> &entries)
	{
		std::sort(entries.begin(), entries.end(), T::compare_encyclopedia_entries);
	}

private:
	static inline bool initialize_class()
	{
		//initialize the metadata (including database parsing/processing functions) for this data type
		auto metadata = std::make_unique<data_type_metadata>(T::class_identifier, T::database_dependencies, T::parse_database, T::process_database, T::initialize_all, T::process_all_text, T::check_all, T::clear);
		database::get()->register_metadata(std::move(metadata));

		database::get()->register_string_to_qvariant_conversion(T::property_class_identifier, [](const std::string &value) {
			return QVariant::fromValue(T::get(value));
		});

		return true;
	}

	static inline std::vector<T *> instances;
	static inline std::map<std::string, qunique_ptr<T>> instances_by_identifier;
	static inline std::map<std::string, T *> instances_by_alias;
	static inline data_module_map<std::vector<gsml_data>> gsml_data_to_process;
	static inline bool class_initialized = data_type::initialize_class();

public:
	virtual std::string get_link_string(const std::string &link_text = "", const bool highlight_as_fallback = false) const
	{
		const T *underlying = this->to_underlying();

		std::string link_name;
		if (!link_text.empty()) {
			link_name = link_text;
		} else {
			link_name = underlying->get_link_name();
		}

		if (!underlying->has_encyclopedia_entry()) {
			//don't write a link if the entry cannot have an encyclopedia entry
			if (highlight_as_fallback) {
				return "~<" + link_name + "~>";
			} else {
				return link_name;
			}
		}

		std::string link = "<a href='";
		link += T::class_identifier;
		link += ":";
		link += underlying->get_identifier();
		link += "'>";
		link += link_name;
		link += "</a>";
		return link;
	}

private:
	const T *to_underlying() const
	{
		return static_cast<const T *>(this);
	}
};

}
