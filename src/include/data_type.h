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

#include <core/typedefs.h>
#include <core/ustring.h>

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
	static _FORCE_INLINE_ class_name *Get(const std::string &ident, const bool should_find = true) \
	{ \
		std::map<std::string, class_name *>::const_iterator find_iterator = class_name::InstancesByIdent.find(ident); \
		\
		if (find_iterator != class_name::InstancesByIdent.end()) { \
			return find_iterator->second; \
		} \
		\
		if (should_find) { \
			fprintf(stderr, "Invalid class_name instance: \"%s\".\n", ident.c_str()); \
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
	static _FORCE_INLINE_ class_name *GetOrAdd(const std::string &ident) \
	{ \
		class_name *instance = class_name::Get(ident, false); \
		\
		if (!instance) { \
			instance = new class_name; \
			instance->Ident = ident; \
			class_name::Instances.push_back(instance); \
			InstancesByIdent[ident] = instance; \
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
	static _FORCE_INLINE_ const std::vector<class_name *> &GetAll() \
	{ \
		return class_name::Instances; \
	} \
	\
	/**
	**	@brief	Remove the existing class instances
	*/ \
	static _FORCE_INLINE_ void Clear() \
	{ \
		for (class_name *instance : class_name::Instances) { \
			delete instance; \
		} \
		class_name::Instances.clear(); \
	} \
	\
private: \
	static inline std::vector<class_name *> Instances; \
	static inline std::map<std::string, class_name *> InstancesByIdent;

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;

class CDataType
{
public:
	CDataType()
	{
	}
	
	CDataType(const std::string &ident) : Ident(ident)
	{
	}
	
	virtual void ProcessConfigData(const CConfigData *config_data) = 0;
	
	String GetIdent() const
	{
		return this->Ident.c_str();
	}
	
	std::string Ident;	/// String identifier of the data type instance
};

#endif
