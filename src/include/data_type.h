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

#include <functional>
#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Macros
----------------------------------------------------------------------------*/

#define DATA_TYPE(class_name, base_class_name) \
	typedef class_name ThisClass; \
	\
	GDCLASS(class_name, base_class_name) \
	\
protected: \
	/**
	**	@brief	Get a property of the data type by its key
	**
	**	@param	property_key	The key of the property
	**
	**	@return	The property if it exists, or null otherwise
	*/ \
	virtual PropertyCommonBase *GetProperty(const std::string &property_key) override \
	{ \
		std::map<std::string, std::function<PropertyCommonBase *(class_name *)>>::iterator find_iterator = class_name::Properties.find(property_key); \
		if (find_iterator != class_name::Properties.end()) { \
			return find_iterator->second(this); \
		} else { \
			return base_class_name::GetProperty(property_key); \
		} \
	} \
	\
private: \
	static inline std::map<std::string, std::function<PropertyCommonBase *(class_name *)>> Properties; \
	static inline std::map<std::string, String> PropertyGetterPrefixes;
	
#define REGISTER_PROPERTY(property_variable) \
	ThisClass::Properties.insert({PascalCaseToSnakeCase(#property_variable), std::function<PropertyCommonBase *(ThisClass *)>([](ThisClass *class_instance) -> PropertyCommonBase* { return &class_instance->property_variable; })});
	
#define BIND_PROPERTIES() \
	for (std::map<std::string, std::function<PropertyCommonBase *(ThisClass *)>>::iterator iterator = ThisClass::Properties.begin(); iterator != ThisClass::Properties.end(); ++iterator) { \
		const std::string &property_key = iterator->first; \
		std::function<PropertyCommonBase *(ThisClass *)> property_function = iterator->second; \
		/* temporary instance used to check the contained value in the property */ \
		ThisClass temp_instance; \
		PropertyCommonBase *property = property_function(&temp_instance); \
		String method_name; \
		\
		std::map<std::string, String>::iterator getter_prefix_find_iterator = ThisClass::PropertyGetterPrefixes.find(property_key); \
		if (getter_prefix_find_iterator != ThisClass::PropertyGetterPrefixes.end()) { \
			method_name += getter_prefix_find_iterator->second + "_"; \
		} else { \
			if (dynamic_cast<PropertyBase<bool> *>(property)) { \
				method_name += "is_"; \
			} else { \
				method_name += "get_"; \
			} \
		} \
		\
		method_name += property_key.c_str(); \
		ClassDB::bind_method(MethodDefinition(method_name), [property_function](ThisClass *instance){ return property_function(instance)->ToVariant(); }); \
	}

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCivilization;
class CFaction;
class DataElement;

template <typename T, typename O>
class ExposedProperty;

template <typename T, typename O>
class Property;

class PropertyCommonBase;

extern std::string PascalCaseToSnakeCase(const std::string &str);

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

template <typename T>
class DataType
{
	template <typename T2>
	using Property = Property<T2, T>; //to reduce redundancy from the property declarations
	
	template <typename T2>
	using ExposedProperty = ExposedProperty<T2, T>;
	
	using PropertyMap = std::map<std::string, PropertyCommonBase *T::*>;
	
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
		std::map<std::string, T *>::const_iterator find_iterator = DataType<T>::InstancesByIdent.find(ident);
		
		if (find_iterator != DataType<T>::InstancesByIdent.end()) {
			return find_iterator->second;
		}
		
		if (should_find) {
			fprintf(stderr, "Invalid \"%s\" instance: \"%s\".\n", T::ClassIdentifier, ident.c_str());
		}
		
		return nullptr;
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
			fprintf(stderr, "Invalid \"%s\" instance index: %i.\n", T::ClassIdentifier, index);
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
		T *instance = new T;
		instance->Ident = ident;
		instance->Index = DataType<T>::Instances.size();
		DataType<T>::Instances.push_back(instance);
		DataType<T>::InstancesByIdent[ident] = instance;
		
		return instance;
	}
	
	/**
	**	@brief	Remove an instance of the class
	**
	**	@param	instance	The instance
	*/
	static inline void Remove(T *instance)
	{
		DataType<T>::InstancesByIdent.erase(instance->Ident);
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
	**	@brief	Initialize the factory function for the class
	*/
	static inline bool InitializeClassFactoryFunction()
	{
		if constexpr(!std::is_same_v<T, CCivilization> && !std::is_same_v<T, CFaction>) {
			//FIXME: this conditional is only temporarily needed while the civilization and faction classes don't inherit from DataElement
			CConfigData::DataTypeGetOrAddFunctions[T::ClassIdentifier] = std::function<DataElement *(const std::string &)>(DataType<T>::GetOrAdd);
		}
		
		return true;
	}
	
	static inline std::vector<T *> Instances;
	static inline std::map<std::string, T *> InstancesByIdent;
	static inline bool ClassFactoryFunctionInitialized = DataType<T>::InitializeClassFactoryFunction();
	
	friend T;
};

#endif
