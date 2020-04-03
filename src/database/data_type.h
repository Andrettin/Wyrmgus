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

#include "util/qunique_ptr.h"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace stratagus {

template <typename T>
class data_type
{
public:
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

		auto find_iterator = data_type::instances_by_identifier.find(identifier);
		if (find_iterator != data_type::instances_by_identifier.end()) {
			return find_iterator->second.get();
		}

		auto alias_find_iterator = data_type::instances_by_alias.find(identifier);
		if (alias_find_iterator != data_type::instances_by_alias.end()) {
			return alias_find_iterator->second;
		}

		return nullptr;
	}

	static T *get_or_add(const std::string &identifier)
	{
		T *instance = T::try_get(identifier);
		if (instance != nullptr) {
			return instance;
		}

		return T::add(identifier);
	}

	static const std::vector<T *> &get_all()
	{
		return data_type::instances;
	}

	static bool exists(const std::string &identifier)
	{
		return data_type::instances_by_identifier.contains(identifier) || data_type::instances_by_alias.contains(identifier);
	}

	static T *add(const std::string &identifier)
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

private:
	static inline std::vector<T *> instances;
	static inline std::map<std::string, qunique_ptr<T>> instances_by_identifier;
	static inline std::map<std::string, T *> instances_by_alias;
};

}
