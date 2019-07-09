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
/**@name time_of_day.cpp - The time of day source file. */
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

#include "time/time_of_day.h"

#include "config.h"
#include "config_operator.h"
#include "module.h"
#include "video/palette_image.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CTimeOfDay::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "color_modification") {
		this->ColorModification.ProcessConfigData(section);
	} else if (section->Tag == "image") {
		String image_ident = "time_of_day_" + this->GetIdent();
		image_ident = image_ident.replace("_", "-");
		PaletteImage *image = PaletteImage::GetOrAdd(image_ident.utf8().get_data());
		image->ProcessConfigData(section);
		this->Image = image;
		
		this->G = CGraphic::New(image->GetFile().utf8().get_data(), image->GetFrameSize().width, image->GetFrameSize().height);
		this->G->Load();
	} else {
		return false;
	}
	
	return true;
}

void CTimeOfDay::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_dawn", "dawn"), +[](CTimeOfDay *time_of_day, const bool dawn){ time_of_day->Dawn = dawn; });
	ClassDB::bind_method(D_METHOD("is_dawn"), &CTimeOfDay::IsDawn);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dawn"), "set_dawn", "is_dawn");
	
	ClassDB::bind_method(D_METHOD("set_day", "day"), +[](CTimeOfDay *time_of_day, const bool day){ time_of_day->Day = day; });
	ClassDB::bind_method(D_METHOD("is_day"), &CTimeOfDay::IsDay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "day"), "set_day", "is_day");
	
	ClassDB::bind_method(D_METHOD("set_dusk", "dusk"), +[](CTimeOfDay *time_of_day, const bool dusk){ time_of_day->Dusk = dusk; });
	ClassDB::bind_method(D_METHOD("is_dusk"), &CTimeOfDay::IsDusk);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dusk"), "set_dusk", "is_dusk");
	
	ClassDB::bind_method(D_METHOD("set_night", "night"), +[](CTimeOfDay *time_of_day, const bool night){ time_of_day->Night = night; });
	ClassDB::bind_method(D_METHOD("is_night"), &CTimeOfDay::IsNight);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "night"), "set_night", "is_night");
	
	ClassDB::bind_method(D_METHOD("set_image", "ident"), +[](CTimeOfDay *time_of_day, const String &ident){ time_of_day->Image = PaletteImage::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_image"), +[](const CTimeOfDay *time_of_day){ return const_cast<PaletteImage *>(time_of_day->GetImage()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "image"), "set_image", "get_image");
}
