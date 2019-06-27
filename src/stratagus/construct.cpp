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
/**@name construct.cpp - The constructions. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer and Jimmy Salmon
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

#include "construct.h"

#include "script.h"
#include "translate.h"
#include "ui/ui.h"
#include "video/palette_image.h"
#include "video/video.h"

#include <vector>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/**
**  Constructions.
*/
static std::vector<CConstruction *> Constructions;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CConstruction::~CConstruction()
{
	this->Clean();
}

void CConstruction::Clean()
{
	CGraphic::Free(this->Sprite);
	this->Sprite = nullptr;
	CGraphic::Free(this->ShadowSprite);
	this->ShadowSprite = nullptr;
	CConstructionFrame *cframe = this->Frames;
	this->Frames = nullptr;
	while (cframe) {
		CConstructionFrame *next = cframe->Next;
		delete cframe;
		cframe = next;
	}
	this->Width = 0;
	this->Height = 0;
	this->ShadowWidth = 0;
	this->ShadowHeight = 0;
	this->ShadowFile.Width = 0;
	this->ShadowFile.Height = 0;
}

void CConstruction::Load()
{
	if (this->GetIdent().empty()) {
		return;
	}
	
	if (this->GetImage() != nullptr) {
		this->Width = this->GetImage()->GetFrameSize().width;
		this->Height = this->GetImage()->GetFrameSize().height;
		UpdateLoadProgress();
		this->Sprite = CPlayerColorGraphic::New(this->GetImage()->GetFile().utf8().get_data(), this->Width, this->Height);
		this->Sprite->Load();
		IncItemsLoaded();
	}
	std::string file = this->ShadowFile.File;
	this->ShadowWidth = this->ShadowFile.Width;
	this->ShadowHeight = this->ShadowFile.Height;
	if (!file.empty()) {
		UpdateLoadProgress();
		this->ShadowSprite = CGraphic::ForceNew(file, this->ShadowWidth, this->ShadowHeight);
		this->ShadowSprite->Load();
		this->ShadowSprite->MakeShadow();
		IncItemsLoaded();
	}
}


/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CConstruction::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "image") {
		PaletteImage *image = PaletteImage::GetOrAdd(this->Ident);
		image->ProcessConfigData(section);
		this->Image = image;
	} else {
		return false;
	}
	
	return true;
}

void CConstruction::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_image", "ident"), +[](CConstruction *construction, const String &ident){ construction->Image = PaletteImage::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_image"), +[](const CConstruction *construction){ return const_cast<PaletteImage *>(construction->GetImage()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "image"), "set_image", "get_image");
}

/**
**  Return the amount of constructions.
*/
int GetConstructionsCount()
{
	int count = 0;
	for (const CConstruction *construction : CConstruction::GetAll()) {
		if (construction->GetIdent().empty()) {
			continue;
		}

		if (construction->GetImage() != nullptr) count++;

		std::string file = construction->ShadowFile.File.c_str();
		if (!file.empty()) count++;

	}
	return count;
}

/**
**  Load the graphics for the constructions.
**
**  HELPME: who make this better terrain depended and extendable
**  HELPME: filename construction.
*/
void LoadConstructions()
{
	ShowLoadProgress("%s", _("Loading Construction Graphics"));
		
	for (CConstruction *construction : CConstruction::GetAll()) {
		construction->Load();
	}
}

/**
**  Parse the construction.
**
**  @param l  Lua state.
**
**  @note make this more flexible
*/
static int CclDefineConstruction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	// Slot identifier
	const std::string str = LuaToString(l, 1);
	CConstruction *construction = CConstruction::GetOrAdd(str);
	construction->Clean();

	//  Parse the arguments, in tagged format.
	lua_pushnil(l);
	while (lua_next(l, 2)) {
		const char *value = LuaToString(l, -2);
		bool files = !strcmp(value, "Files");

		if (files || !strcmp(value, "ShadowFiles")) {
			std::string file;
			int w = 0;
			int h = 0;

			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			lua_pushnil(l);
			while (lua_next(l, -2)) {
				const char *value = LuaToString(l, -2);

				if (!strcmp(value, "File")) {
					file = LuaToString(l, -1);
				} else if (!strcmp(value, "Size")) {
					CclGetPos(l, &w, &h);
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
				lua_pop(l, 1);
			}
			if (files) {
				String image_ident = construction->GetIdent();
				PaletteImage *image = PaletteImage::GetOrAdd(image_ident.utf8().get_data());
				image->File = file.c_str();
				image->FrameSize = Vector2i(w, h);
				construction->Image = image;
				image->Initialize();
			} else {
				construction->ShadowFile.File = file;
				construction->ShadowFile.Width = w;
				construction->ShadowFile.Height = h;
			}
		} else if (!strcmp(value, "Constructions")) {
			const unsigned int subargs = lua_rawlen(l, -1);

			for (unsigned int k = 0; k < subargs; ++k) {
				int percent = 0;
				ConstructionFileType file = ConstructionFileType::Construction;
				int frame = 0;

				lua_rawgeti(l, -1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				lua_pushnil(l);
				while (lua_next(l, -2)) {
					const char *value = LuaToString(l, -2);

					if (!strcmp(value, "Percent")) {
						percent = LuaToNumber(l, -1);
					} else if (!strcmp(value, "File")) {
						const char *value = LuaToString(l, -1);

						if (!strcmp(value, "construction")) {
							file = ConstructionFileType::Construction;
						} else if (!strcmp(value, "main")) {
							file = ConstructionFileType::Main;
						} else {
							LuaError(l, "Unsupported tag: %s" _C_ value);
						}
					} else if (!strcmp(value, "Frame")) {
						frame = LuaToNumber(l, -1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
					lua_pop(l, 1);
				}
				lua_pop(l, 1);
				CConstructionFrame **cframe = &construction->Frames;
				while (*cframe) {
					cframe = &((*cframe)->Next);
				}
				(*cframe) = new CConstructionFrame;
				(*cframe)->Percent = percent;
				(*cframe)->File = file;
				(*cframe)->Frame = frame;
				(*cframe)->Next = nullptr;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for construction.
*/
void ConstructionCclRegister()
{
	lua_register(Lua, "DefineConstruction", CclDefineConstruction);
}
