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
/**@name text.cpp - The texts. */
//
//      (c) Copyright 2016 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "text.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CText *> Texts;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CText::~CText()
{
	for (size_t i = 0; i < this->Chapters.size(); ++i) {
		delete this->Chapters[i];
	}
	this->Chapters.clear();
}
	
CChapter *CText::GetChapter(const std::string &chapter_name)
{
	for (size_t i = 0; i < this->Chapters.size(); ++i) {
		if (chapter_name == this->Chapters[i]->Name) {
			return this->Chapters[i];
		}
	}
	return nullptr;
}

void CleanTexts()
{
	for (size_t i = 0; i < Texts.size(); ++i) {
		delete Texts[i];
	}
	Texts.clear();
}

CText *GetText(const std::string &text_name)
{
	for (size_t i = 0; i < Texts.size(); ++i) {
		if (text_name == Texts[i]->Name) {
			return Texts[i];
		}
	}
	return nullptr;
}

//@}
