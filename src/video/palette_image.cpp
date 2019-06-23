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
/**@name palette_image.cpp - The palette image source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "video/palette_image.h"

#include "hair_color.h"
#include "module.h"
#include "player_color.h"
#include "skin_color.h"
#include "wyrmgus.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Initialize the palette image
*/
void PaletteImage::Initialize()
{
	if (this->GetFile().empty()) {
		print_error("Palette image \"" + this->GetIdent() + "\" has no file.");
	}
	
	if (this->GetFrameSize().width == 0) {
		print_error("Palette image \"" + this->GetIdent() + "\" has no frame width.");
	}
	
	if (this->GetFrameSize().height == 0) {
		print_error("Palette image \"" + this->GetIdent() + "\" has no frame height.");
	}
	
	String filepath = this->GetFile();
	if (this->GetFile().find(Wyrmgus::GetInstance()->GetUserDirectory()) == -1) {
		if (this->GetFile().find("dlcs/") != -1 || this->GetFile().find("modules/") != -1) {
			filepath = "res://" + this->GetFile();
		} else {
			filepath = "res://graphics/" + this->GetFile();
		}
		this->Texture = ResourceLoader::load(filepath);
	} else {
		//for images that don't have import files, we need to do it a bit differently
		Ref<Image> image;
		image.instance();
		image->load(filepath);
		Ref<ImageTexture> image_texture;
		image_texture.instance();
		image_texture->create_from_image(image);
		this->Texture = image_texture;
	}
	
	this->Initialized = true;
}

void PaletteImage::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_file", "file"), +[](PaletteImage *image, const String &file){
		image->File = CModule::GetCurrentPath().c_str() + file;
	});
	ClassDB::bind_method(D_METHOD("get_file"), &PaletteImage::GetFile);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "file"), "set_file", "get_file");

	ClassDB::bind_method(D_METHOD("get_frame_size"), +[](const PaletteImage *image){ return Vector2(image->FrameSize); });
	
	ClassDB::bind_method(D_METHOD("set_frame_width", "frame_width"), +[](PaletteImage *image, const int frame_width){ image->FrameSize.x = frame_width; });
	ClassDB::bind_method(D_METHOD("get_frame_width"), +[](const PaletteImage *image){ return image->FrameSize.x; });
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frame_width"), "set_frame_width", "get_frame_width");
	
	ClassDB::bind_method(D_METHOD("set_frame_height", "frame_height"), +[](PaletteImage *image, const int frame_height){ image->FrameSize.y = frame_height; });
	ClassDB::bind_method(D_METHOD("get_frame_height"), +[](const PaletteImage *image){ return image->FrameSize.y; });
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frame_height"), "set_frame_height", "get_frame_height");
	
	ClassDB::bind_method(D_METHOD("set_source_primary_player_color", "ident"), +[](PaletteImage *image, const String &ident){
		image->SourcePrimaryPlayerColor = CPlayerColor::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_source_primary_player_color"), +[](const PaletteImage *image){ return const_cast<CPlayerColor *>(image->SourcePrimaryPlayerColor); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_primary_player_color"), "set_source_primary_player_color", "get_source_primary_player_color");
	
	ClassDB::bind_method(D_METHOD("set_source_secondary_player_color", "ident"), +[](PaletteImage *image, const String &ident){
		image->SourceSecondaryPlayerColor = CPlayerColor::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_source_secondary_player_color"), +[](const PaletteImage *image){ return const_cast<CPlayerColor *>(image->SourceSecondaryPlayerColor); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_secondary_player_color"), "set_source_secondary_player_color", "get_source_secondary_player_color");
	
	ClassDB::bind_method(D_METHOD("set_source_skin_color", "ident"), +[](PaletteImage *image, const String &ident){
		image->SourceSkinColor = CSkinColor::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_source_skin_color"), +[](const PaletteImage *image){ return const_cast<CSkinColor *>(image->SourceSkinColor); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_skin_color"), "set_source_skin_color", "get_source_skin_color");
	
	ClassDB::bind_method(D_METHOD("set_source_hair_color", "ident"), +[](PaletteImage *image, const String &ident){
		image->SourceHairColor = CHairColor::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_source_hair_color"), +[](const PaletteImage *image){ return const_cast<CHairColor *>(image->SourceHairColor); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_hair_color"), "set_source_hair_color", "get_source_hair_color");
	
	ClassDB::bind_method(D_METHOD("get_texture"), +[](const PaletteImage *image){ return image->Texture; });
}
