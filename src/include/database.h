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
/**@name database.h - The database header file. */
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

#ifndef __DATABASE_H__
#define __DATABASE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

template <typename T>
class Database
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
		std::map<std::string, T *>::const_iterator find_iterator = Database<T>::InstancesByIdent.find(ident);
		
		if (find_iterator != Database<T>::InstancesByIdent.end()) {
			return find_iterator->second;
		}
		
		if (should_find) {
			fprintf(stderr, "Invalid %s instance: \"%s\".\n", T::GetClassIdentifier(), ident.c_str());
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
		
		if (index < static_cast<int>(Database<T>::Instances.size())) {
			return Database<T>::Instances[index];
		}
		
		if (should_find) {
			fprintf(stderr, "Invalid %s instance index: %i.\n", T::GetClassIdentifier(), index);
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
		T *instance = Database<T>::Get(ident, false);
		
		if (!instance) {
			instance = new T;
			instance->Ident = ident;
			instance->Index = Database<T>::Instances.size();
			Database<T>::Instances.push_back(instance);
			Database<T>::InstancesByIdent[ident] = instance;
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
		return Database<T>::Instances;
	}
	
	/**
	**	@brief	Remove an instance of the class
	**
	**	@param	instance	The instance
	*/
	static inline void Remove(const T *instance)
	{
		Database<T>::InstancesByIdent.erase(instance->Ident);
		Database<T>::Instances.erase(std::remove(Database<T>::Instances.begin(), Database<T>::Instances.end(), instance), Database<T>::Instances.end());
		delete instance;
	}
	
	/**
	**	@brief	Remove the existing class instances
	*/
	static inline void Clear()
	{
		for (T *instance : Database<T>::Instances) {
			delete instance;
		}
		Database<T>::Instances.clear();
		Database<T>::InstancesByIdent.clear();
	}
	
	static inline bool AreAllInitialized()
	{
		for (T *instance : Database<T>::Instances) {
			if (!instance->IsInitialized()) {
				return false;
			}
		}
		
		return true;
	}
	
private:
	static inline std::vector<T *> Instances;
	static inline std::map<std::string, T *> InstancesByIdent;
};

#endif
