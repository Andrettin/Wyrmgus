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
/**@name data_type.h - The data type header file. */
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

#include <core/object.h>
#include <core/ustring.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

//macro for data type classes
#define DATA_TYPE_CLASS(class_name) \
public: \
	/**
	**	@brief	Get an instance of the class by its string identifier
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The instance if found, or null otherwise
	*/ \
	static inline class_name *Get(const std::string &ident, const bool should_find = true) \
	{ \
		std::map<std::string, class_name *>::const_iterator find_iterator = class_name::InstancesByIdent.find(ident); \
		\
		if (find_iterator != class_name::InstancesByIdent.end()) { \
			return find_iterator->second; \
		} \
		\
		if (should_find) { \
			fprintf(stderr, "Invalid %s instance: \"%s\".\n", #class_name, ident.c_str()); \
		} \
		\
		return nullptr; \
	} \
	\
	/**
	**	@brief	Get an instance of the class by its index
	**
	**	@param	index	The instance's index
	**
	**	@return	The instance if found, or null otherwise
	*/ \
	static inline class_name *Get(const int index, const bool should_find = true) \
	{ \
		if (index == -1) { \
			return nullptr; \
		} \
		\
		if (index < static_cast<int>(class_name::Instances.size())) { \
			return class_name::Instances[index]; \
		} \
		\
		if (should_find) { \
			fprintf(stderr, "Invalid %s instance index: %i.\n", #class_name, index); \
		} \
		\
		return nullptr; \
	} \
	\
	/**
	**	@brief	Get or add an instance of the class
	**
	**	@param	ident	The instance's string identifier
	**
	**	@return	The instance if found, otherwise a new instance is created and returned
	*/ \
	static inline class_name *GetOrAdd(const std::string &ident) \
	{ \
		class_name *instance = class_name::Get(ident, false); \
		\
		if (!instance) { \
			instance = new class_name; \
			instance->Ident = ident; \
			instance->Index = class_name::Instances.size(); \
			class_name::Instances.push_back(instance); \
			class_name::InstancesByIdent[ident] = instance; \
		} \
		\
		return instance; \
	} \
	\
	/**
	**	@brief	Gets all instances of the class
	**
	**	@return	All existing instances of the class
	*/ \
	static inline const std::vector<class_name *> &GetAll() \
	{ \
		return class_name::Instances; \
	} \
	\
	/**
	**	@brief	Remove an instance of the class
	**
	**	@param	instance	The instance
	*/ \
	static inline void Remove(const class_name *instance) \
	{ \
		class_name::InstancesByIdent.erase(instance->Ident); \
		class_name::Instances.erase(std::remove(class_name::Instances.begin(), class_name::Instances.end(), instance), class_name::Instances.end()); \
		delete instance; \
	} \
	\
	/**
	**	@brief	Remove the existing class instances
	*/ \
	static inline void Clear() \
	{ \
		for (class_name *instance : class_name::Instances) { \
			delete instance; \
		} \
		class_name::Instances.clear(); \
		class_name::InstancesByIdent.clear(); \
	} \
	\
	static inline bool AreAllInitialized() \
	{ \
		for (class_name *instance : class_name::Instances) { \
			if (!instance->IsInitialized()) { \
				return false; \
			} \
		} \
		\
		return true; \
	} \
	\
private: \
	static inline std::vector<class_name *> Instances; \
	static inline std::map<std::string, class_name *> InstancesByIdent;

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CDataType : public Object
{
	GDCLASS(CDataType, Object)
	
public:
	CDataType(const std::string &ident = "", const int index = -1) : Ident(ident), Index(index)
	{
	}
	
	virtual void ProcessConfigData(const CConfigData *config_data);
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) { return false; }
	virtual bool ProcessConfigDataSection(const CConfigData *section) { return false; }
	
	/**
	**	@brief	Initialize the data type instance
	*/
	virtual void Initialize()
	{
		this->Initialized = true;
	}
	
	/**
	**	@brief	Get the data type instance's string identifier
	**
	**	@return	The data type instance's string identifier
	*/
	String GetIdent() const
	{
		return this->Ident.c_str();
	}
	
	/**
	**	@brief	Get the data type instance's index
	**
	**	@return	The data type instance's index
	*/
	int GetIndex() const
	{
		return this->Index;
	}
	
	/**
	**	@brief	Get whether the data type instance has been initialized
	**
	**	@return	True if the data type instance has been initialized, or false otherwise
	*/
	bool IsInitialized() const
	{
		return this->Initialized;
	}
	
	std::string Ident;	/// string identifier of the data type instance
protected:
	int Index = -1;		/// index of the data type instance
	bool Initialized = false;	/// whether the data type instance has been initialized

protected:
	static void _bind_methods();
};

#endif
