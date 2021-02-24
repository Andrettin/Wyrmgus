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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "stratagus.h"

#include "currency.h"

#include "config.h"

std::vector<CCurrency *> CCurrency::Currencies;
std::map<std::string, CCurrency *> CCurrency::CurrenciesByIdent;

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

void CCurrency::ClearCurrencies()
{
	for (size_t i = 0; i < Currencies.size(); ++i) {
		delete Currencies[i];
	}
	Currencies.clear();
}

void CCurrency::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid currency property: \"%s\".\n", key.c_str());
		}
	}
}
