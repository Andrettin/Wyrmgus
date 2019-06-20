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
/**@name palette_image.h - The palette image header file. */
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

#ifndef __PALETTE_IMAGE_H
#define __PALETTE_IMAGE_H

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"

#include <core/math/vector2.h>
#include <scene/resources/texture.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CHairColor;
class CPlayerColor;
class CSkinColor;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class PaletteImage : public DataElement, public DataType<PaletteImage>
{
	GDCLASS(PaletteImage, DataElement)

public:
	static constexpr const char *ClassIdentifier = "palette_image";
	
	static constexpr Vector2i EmptyFrameSize = Vector2i(0, 0);
	
	virtual void Initialize() override;
	
	const String &GetFile() const
	{
		return this->File;
	}
	
	const Vector2i &GetFrameSize() const
	{
		return this->FrameSize;
	}
	
private:
	String File;							/// the image file
	Vector2i FrameSize = Vector2i(0, 0);	/// the size of a frame in the image, in pixels
	const CPlayerColor *SourcePrimaryPlayerColor = nullptr;		/// the source primary player color for the image
	const CPlayerColor *SourceSecondaryPlayerColor = nullptr;	/// the source secondary player color for the image
	const CSkinColor *SourceSkinColor = nullptr;				/// the source skin color for the image
	const CHairColor *SourceHairColor = nullptr;				/// the source hair color for the image
	Ref<Texture> Texture;										/// the image's texture

	friend int CclDefineIcon(lua_State *l);
	friend int CclDefineUnitType(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
