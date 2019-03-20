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
/**@name missile_type.cpp - The missile type source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "missile/missile_type.h"

#include "config.h"
#include "luacallback.h"
#include "mod.h"
#include "sound/sound.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/**
**  Missile class names, used to load/save the missiles.
*/
const char *MissileType::MissileClassNames[] = {
	"missile-class-none",
	"missile-class-point-to-point",
	"missile-class-point-to-point-with-hit",
	"missile-class-point-to-point-cycle-once",
	"missile-class-point-to-point-bounce",
	"missile-class-stay",
	"missile-class-cycle-once",
	"missile-class-fire",
	"missile-class-hit",
	"missile-class-parabolic",
	"missile-class-land-mine",
	"missile-class-whirlwind",
	"missile-class-flame-shield",
	"missile-class-death-coil",
	"missile-class-tracer",
	"missile-class-clip-to-target",
	"missile-class-continious",
	"missile-class-straight-fly",
	nullptr
};

/// lookup table for missile names
typedef std::map<std::string, MissileType *> MissileTypeMap;
static MissileTypeMap MissileTypes;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Constructor.
*/
MissileType::MissileType(const std::string &ident) :
	CDataType(ident), Class()
{
}

/**
**  Destructor.
*/
MissileType::~MissileType()
{
	CGraphic::Free(this->G);
	Impact.clear();
	delete OnImpact;
	FreeNumberDesc(this->Damage);
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool MissileType::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "frames") {
		this->SpriteFrames = std::stoi(value);
	} else if (key == "flip") {
		this->Flip = StringToBool(value);
	} else if (key == "num_directions") {
		this->NumDirections = std::stoi(value);
	} else if (key == "transparency") {
		this->Transparency = std::stoi(value);
	} else if (key == "fired_sound") {
		value = FindAndReplaceString(value, "_", "-");
		this->FiredSound = value;
	} else if (key == "impact_sound") {
		value = FindAndReplaceString(value, "_", "-");
		this->ImpactSound = value;
	} else if (key == "class") {
		value = FindAndReplaceString(value, "_", "-");
		const char *class_name = value.c_str();
		unsigned int i = 0;
		for (; MissileClassNames[i]; ++i) {
			if (!strcmp(class_name, MissileClassNames[i])) {
				this->Class = i;
				break;
			}
		}
		if (!MissileClassNames[i]) {
			fprintf(stderr, "Invalid missile class: \"%s\".\n", value.c_str());
		}
	} else if (key == "num_bounces") {
		this->NumBounces = std::stoi(value);
	} else if (key == "max_bounce_size") {
		this->MaxBounceSize = std::stoi(value);
	} else if (key == "parabol_coefficient") {
		this->ParabolCoefficient = std::stoi(value);
	} else if (key == "delay") {
		this->StartDelay = std::stoi(value);
	} else if (key == "sleep") {
		this->Sleep = std::stoi(value);
	} else if (key == "speed") {
		this->Speed = std::stoi(value);
	} else if (key == "blizzard_speed") {
		this->BlizzardSpeed = std::stoi(value);
	} else if (key == "attack_speed") {
		this->AttackSpeed = std::stoi(value);
	} else if (key == "ttl") {
		this->TTL = std::stoi(value);
	} else if (key == "reduce_factor") {
		this->ReduceFactor = std::stoi(value);
	} else if (key == "smoke_precision") {
		this->SmokePrecision = std::stoi(value);
	} else if (key == "missile_stop_flags") {
		this->MissileStopFlags = std::stoi(value);
	} else if (key == "draw_level") {
		this->DrawLevel = std::stoi(value);
	} else if (key == "range") {
		this->Range = std::stoi(value);
	} else if (key == "smoke_missile") {
		value = FindAndReplaceString(value, "_", "-");
		this->Smoke.Name = value;
	} else if (key == "can_hit_owner") {
		this->CanHitOwner = StringToBool(value);
	} else if (key == "always_fire") {
		this->AlwaysFire = StringToBool(value);
	} else if (key == "pierce") {
		this->Pierce = StringToBool(value);
	} else if (key == "pierce_once") {
		this->PierceOnce = StringToBool(value);
	} else if (key == "pierce_ignore_before_goal") {
		this->PierceIgnoreBeforeGoal = StringToBool(value);
	} else if (key == "ignore_walls") {
		this->IgnoreWalls = StringToBool(value);
	} else if (key == "kill_first_unit") {
		this->KillFirstUnit = StringToBool(value);
	} else if (key == "friendly_fire") {
		this->FriendlyFire = StringToBool(value);
	} else if (key == "always_hits") {
		this->AlwaysHits = StringToBool(value);
	} else if (key == "splash_factor") {
		this->SplashFactor = std::stoi(value);
	} else if (key == "correct_sphash_damage") {
		this->CorrectSphashDamage = StringToBool(value);
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool MissileType::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "image") {
		std::string file;
			
		for (size_t j = 0; j < section->Properties.size(); ++j) {
			std::string key = section->Properties[j].first;
			std::string value = section->Properties[j].second;
			
			if (key == "file") {
				file = CMod::GetCurrentModPath() + value;
			} else if (key == "width") {
				this->size.x = std::stoi(value);
				} else if (key == "height") {
				this->size.y = std::stoi(value);
			} else {
				fprintf(stderr, "Invalid image property: \"%s\".\n", key.c_str());
			}
		}
		
		if (file.empty()) {
			fprintf(stderr, "Image has no file.\n");
			return true;
		}
		
		if (this->size.x == 0) {
			fprintf(stderr, "Image has no width.\n");
			return true;
		}
		
		if (this->size.y == 0) {
			fprintf(stderr, "Image has no height.\n");
			return true;
		}
		
		this->G = CGraphic::New(file, this->Width(), this->Height());
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Initialize the missile type
*/
void MissileType::Initialize()
{
	if (!this->SmokePrecision) {
		this->SmokePrecision = this->Speed;
	}
	
	this->Initialized = true;
}

/**
**  Load the graphics for a missile type
*/
void MissileType::LoadMissileSprite()
{
	if (this->G && !this->G->IsLoaded()) {
		this->G->Load();
		if (this->Flip) {
			this->G->Flip();
		}

		// Correct the number of frames in graphic
		Assert(this->G->NumFrames >= this->SpriteFrames);
		this->G->NumFrames = this->SpriteFrames;
		// FIXME: Don't use NumFrames as number of frames.
	}
}

/**
**  Initialize missile type.
*/
void MissileType::Init()
{
	// Resolve impact missiles and sounds.
	this->FiredSound.MapSound();
	this->ImpactSound.MapSound();
	for (std::vector<MissileConfig *>::iterator it = this->Impact.begin(); it != this->Impact.end(); ++it) {
		MissileConfig &mc = **it;

		mc.MapMissile();
	}
	this->Smoke.MapMissile();
}

/**
**  Draw missile.
**
**  @param frame  Animation frame
**  @param pos    Screen pixel position
*/
void MissileType::DrawMissileType(int frame, const PixelPos &pos) const
{
#ifdef DYNAMIC_LOAD
	if (!this->G->IsLoaded()) {
		LoadMissileSprite(this);
	}
#endif

	if (this->Flip) {
		if (frame < 0) {
			if (this->Transparency > 0) {
				//Wyrmgus start
//				this->G->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * Transparency));
				this->G->DrawFrameClipTransX(-frame - 1, pos.x, pos.y, int(256 - 2.56 * Transparency), false);
				//Wyrmgus end
			} else {
				//Wyrmgus start
//				this->G->DrawFrameClipX(-frame - 1, pos.x, pos.y);
				this->G->DrawFrameClipX(-frame - 1, pos.x, pos.y, false);
				//Wyrmgus end
			}
		} else {
			if (this->Transparency > 0) {
				//Wyrmgus start
//				this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency));
				this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency), false);
				//Wyrmgus end
			} else {
				//Wyrmgus start
//				this->G->DrawFrameClip(frame, pos.x, pos.y);
				this->G->DrawFrameClip(frame, pos.x, pos.y, false);
				//Wyrmgus end
			}
		}
	} else {
		const int row = this->NumDirections / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * this->NumDirections + this->NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * this->NumDirections + frame % row;
		}
		if (this->Transparency > 0) {
			//Wyrmgus start
//			this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency));
			this->G->DrawFrameClipTrans(frame, pos.x, pos.y, int(256 - 2.56 * Transparency), false);
			//Wyrmgus end
		} else {
			//Wyrmgus start
//			this->G->DrawFrameClip(frame, pos.x, pos.y);
			this->G->DrawFrameClip(frame, pos.x, pos.y, false);
			//Wyrmgus end
		}
	}
}

int GetMissileSpritesCount()
{
#ifndef DYNAMIC_LOAD
	return MissileTypes.size();
#else
	return 0;
#endif
}

/**
**  Load the graphics for all missiles types
*/
void LoadMissileSprites()
{
#ifndef DYNAMIC_LOAD
	for (MissileTypeMap::iterator it = MissileTypes.begin(); it != MissileTypes.end(); ++it) {
		(*it).second->LoadMissileSprite();
	}
#endif
}

/**
**  Get Missile type by identifier.
**
**  @param ident  Identifier.
**
**  @return       Missile type pointer.
*/
MissileType *MissileTypeByIdent(const std::string &ident)
{
	if (ident.empty()) {
		return nullptr;
	}
	MissileTypeMap::iterator it = MissileTypes.find(ident);
	if (it != MissileTypes.end()) {
		return it->second;
	}
	return nullptr;
}

/**
**  Allocate an empty missile-type slot.
**
**  @param ident  Identifier to identify the slot.
**
**  @return       New allocated (zeroed) missile-type pointer.
*/
MissileType *NewMissileTypeSlot(const std::string &ident)
{
	MissileType *mtype = new MissileType(ident);

	MissileTypes[ident] = mtype;
	return mtype;
}

/**
**  Initialize missile-types.
*/
void InitMissileTypes()
{
	for (MissileTypeMap::iterator it = MissileTypes.begin(); it != MissileTypes.end(); ++it) {
		(*it).second->Init();
	}
}

/**
**  Clean up missile-types.
*/
void CleanMissileTypes()
{
	for (MissileTypeMap::iterator it = MissileTypes.begin(); it != MissileTypes.end(); ++it) {
		delete it->second;
	}
	MissileTypes.clear();
}
