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
/**@name data_type.h - The data_type header file. */
//
//      (c) Copyright 2019 by Andrettin
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

#ifndef __DATA_TYPE_H__
#define __DATA_TYPE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "config.h"
#include "util.h"

#include <core/ustring.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class DataElement;

extern std::vector<std::function<void()>> ClassClearFunctions;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

template <typename T>
class DataType
{
public:
	/**
	**	@brief	Get an instance of the class by its string identifier
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The instance if found, or null otherwise
	*/
	static inline T *Get(const std::string &ident, const bool should_find = true)
	{
		if (ident.empty()) {
			return nullptr;
		}
		
		std::string processed_ident = FindAndReplaceString(ident, "_", "-"); //remove this when data elements are no longer used in Lua
		
		std::map<std::string, T *>::const_iterator find_iterator = DataType<T>::InstancesByIdent.find(processed_ident);
		
		if (find_iterator != DataType<T>::InstancesByIdent.end()) {
			return find_iterator->second;
		}
		
		if (should_find) {
			print_error("Invalid \"" + String(T::ClassIdentifier) + "\" instance: \"" + processed_ident.c_str() + "\".");
		}
		
		return nullptr;
	}
	
	/**
	**	@brief	Get an instance of the class by its string identifier
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The instance if found, or null otherwise
	*/
	static inline T *Get(const char *ident, const bool should_find = true)
	{
		return T::Get(std::string(ident), should_find);
	}
	
	/**
	**	@brief	Get an instance of the class by its string identifier
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The instance if found, or null otherwise
	*/
	static inline T *Get(const String &ident, const bool should_find = true)
	{
		return T::Get(ident.utf8().get_data(), should_find);
	}
	
	/**
	**	@brief	Get an instance of the class by its index
	**
	**	@param	index	The instance's index
	**
	**	@return	The instance if found, or null otherwise
	*/
	static inline T *Get(const int index, const bool should_find = true)
	{
		if (index == -1) {
			return nullptr;
		}
		
		if (index < static_cast<int>(DataType<T>::Instances.size())) {
			return DataType<T>::Instances[index];
		}
		
		if (should_find) {
			print_error("Invalid \"" + String(T::ClassIdentifier) + "\" instance index: " + String::num_int64(index) + ".");
		}
		
		return nullptr;
	}
	
	/**
	**	@brief	Get or add an instance of the class
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The instance if found, otherwise a new instance is created and returned
	*/
	static inline T *GetOrAdd(const std::string &ident)
	{
		T *instance = T::Get(ident, false);
		
		if (!instance) {
			instance = T::Add(ident);
		}
		
		return instance;
	}
	
	/**
	**	@brief	Gets all instances of the class
	**
	**	@return	All existing instances of the class
	*/
	static inline const std::vector<T *> &GetAll()
	{
		return DataType<T>::Instances;
	}
	
	/**
	**	@brief	Add a new instance of the class
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The new instance
	*/
	static inline T *Add(const std::string &ident)
	{
		std::string processed_ident = FindAndReplaceString(ident, "_", "-"); //remove this when data elements are no longer used in Lua
		
		if (ident.empty()) {
			throw std::runtime_error("Tried to add a \"" + std::string(T::ClassIdentifier) + "\" instance with an empty string identifier.");
		}
		
		T *instance = new T;
		instance->Ident = processed_ident;
		instance->Index = DataType<T>::Instances.size();
		DataType<T>::Instances.push_back(instance);
		DataType<T>::InstancesByIdent[processed_ident] = instance;
		
		return instance;
	}
	
	/**
	**	@brief	Add a string identifier alias for an instance of the class
	**
	**	@param	ident	The instance's string identifier
	**	@param	alias	The string identifier alias for the instance
	*/
	static inline void AddAlias(const std::string &ident, const std::string &alias)
	{
		std::string processed_alias = FindAndReplaceString(alias, "_", "-"); //remove this when data elements are no longer used in Lua
		
		T *instance = T::Get(ident);
		
		DataType<T>::InstancesByIdent[processed_alias] = instance;
	}
	
	/**
	**	@brief	Remove an instance of the class
	**
	**	@param	instance	The instance
	*/
	static inline void Remove(T *instance)
	{
		DataType<T>::InstancesByIdent.erase(instance->Ident);
		
		//remove aliases as well
		std::vector<std::string> aliases;
		for (const auto &element : DataType<T>::InstancesByIdent) {
			if (element.second == instance && element.first != instance->Ident) {
				aliases.push_back(element.first);
			}
		}
		for (const std::string &alias : aliases) {
			DataType<T>::InstancesByIdent.erase(alias);
		}
		
		DataType<T>::Instances.erase(std::remove(DataType<T>::Instances.begin(), DataType<T>::Instances.end(), instance), DataType<T>::Instances.end());
		delete instance;
	}
	
	/**
	**	@brief	Remove the existing class instances
	*/
	static inline void Clear()
	{
		for (T *instance : DataType<T>::Instances) {
			delete instance;
		}
		DataType<T>::Instances.clear();
		DataType<T>::InstancesByIdent.clear();
	}
	
	/**
	**	@brief	Get whether all instances of the data type have been initialized
	**
	**	@return	True if all instances have been initialized, or false otherwise
	*/
	static inline bool AreAllInitialized()
	{
		for (T *instance : DataType<T>::Instances) {
			if (!instance->IsInitialized()) {
				return false;
			}
		}
		
		return true;
	}
	
private:
	/**
	**	@brief	Update the indexes for the instances
	*/
	static inline void UpdateIndexes()
	{
		for (size_t i = 0; i < DataType<T>::Instances.size(); ++i) {
			DataType<T>::Instances[i]->Index = static_cast<int>(i);
		}
	}
	
	/**
	**	@brief	Initialize the class
	*/
	static inline bool InitializeDataTypeClass()
	{
		//initialize the class factory function
		CConfigData::DataTypeGetFunctions[T::ClassIdentifier] = [](const std::string &ident) -> DataElement * { return T::Get(ident); };
		CConfigData::DataTypeGetOrAddFunctions[T::ClassIdentifier] = std::function<DataElement *(const std::string &)>(T::GetOrAdd);
		CConfigData::DataTypeAddAliasFunctions[T::ClassIdentifier] = std::function<void(const std::string &, const std::string &)>(T::AddAlias);
		
		//register the clear function of the class
		ClassClearFunctions.push_back(std::function<void()>(T::Clear));
		
		return true;
	}
	
	static inline std::vector<T *> Instances;
	static inline std::map<std::string, T *> InstancesByIdent;
	static inline bool DataTypeClassInitialized = DataType<T>::InitializeDataTypeClass();
	
	friend T;
};

#endif
