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
/**@name currency.cpp - The currency source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "economy/currency.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CCurrency *> CCurrency::Currencies;
std::map<std::string, CCurrency *> CCurrency::CurrenciesByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a currency
**
**	@param	ident		The currency's string identifier
**	@param	should_find	Whether it is an error if the currency couldn't be found
**
**	@return	The currency if found, or null otherwise
*/
CCurrency *CCurrency::GetCurrency(const std::string &ident, const bool should_find)
{
	std::map<std::string, CCurrency *>::const_iterator find_iterator = CurrenciesByIdent.find(ident);
	
	if (find_iterator != CurrenciesByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid currency: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a currency
**
**	@param	ident	The currency's string identifier
**
**	@return	The currency if found, otherwise a new currency is created and returned
*/
CCurrency *CCurrency::GetOrAddCurrency(const std::string &ident)
{
	CCurrency *currency = GetCurrency(ident, false);
	
	if (!currency) {
		currency = new CCurrency;
		currency->Ident = ident;
		Currencies.push_back(currency);
		CurrenciesByIdent[ident] = currency;
	}
	
	return currency;
}

/**
**	@brief	Remove the existing currencies
*/
void CCurrency::ClearCurrencies()
{
	for (size_t i = 0; i < Currencies.size(); ++i) {
		delete Currencies[i];
	}
	Currencies.clear();
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CCurrency::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "name") {
		this->Name = value;
	} else {
		return false;
	}
	
	return true;
}
