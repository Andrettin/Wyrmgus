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
/**@name script_sound.cpp - The sound ccl functions. */
//
//      (c) Copyright 1999-2007 by Lutz Sammer, Fabrice Rossi, and Jimmy Salmon
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
-- Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "sound/sound.h"

#include "civilization.h"
#include "script.h"
#include "sound/sound_server.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Glue between c and scheme. Ask the sound system to associate a
**  sound id to a sound name.
**
**  @param l  Lua state.
*/
static int CclSoundForName(lua_State *l)
{
	CSound *id;
	const char *sound_name;
	LuaUserData *data;

	sound_name = LuaToString(l, -1);
	id = SoundForName(sound_name);

	data = (LuaUserData *)lua_newuserdata(l, sizeof(LuaUserData));
	data->Type = LuaSoundType;
	data->Data = id;
	return 1;
}

/**
**  Get a Game Sound Id from either a lua sound id or a sound name
**
**  @param l  Lua state.
**
**  @return   The C sound id.
*/
static CSound *CclGetSound(lua_State *l)
{
	LuaUserData *data;
	int pop;

	pop = 0;
	if (lua_isstring(l, -1)) {
		CclSoundForName(l);
		pop = 1;
	}
	if (lua_isuserdata(l, -1)) {
		data = (LuaUserData *)lua_touserdata(l, -1);
		if (data->Type == LuaSoundType) {
			if (pop) {
				lua_pop(l, 1);
			}
			return (CSound *)data->Data;
		}
	}
	LuaError(l, "CclGetSound: not a sound");
	return nullptr;
}

/**
**  Create a sound.
**
**  Glue between c and scheme. This function asks the sound system to
**  register a sound under a given name, with an associated list of files
**  (the list can be replaced by only one file).
**
**  @param l  Lua state.
**
**  @return   the sound id of the created sound
*/
static int CclMakeSound(lua_State *l)
{
	LuaCheckArgs(l, 2);

	std::string c_name = LuaToString(l, 1);
	std::vector<std::string> files;
	CSound *id;
	if (lua_isstring(l, 2)) {
		// only one file
		files.push_back(LuaToString(l, 2));
		id = MakeSound(c_name, files);
	} else if (lua_istable(l, 2)) {
		// several files
		const int args = lua_rawlen(l, 2);
		files.reserve(args);
		for (int j = 0; j < args; ++j) {
			files.push_back(LuaToString(l, 2, j + 1));
		}
		id = MakeSound(c_name, files);
	} else {
		LuaError(l, "string or table expected");
		return 0;
	}
	LuaUserData *data = (LuaUserData *)lua_newuserdata(l, sizeof(LuaUserData));
	data->Type = LuaSoundType;
	data->Data = id;
	return 1;
}

/**
**  Glue between c and scheme. This function asks the sound system to
**  build a special sound group.
**
**  @param l  Lua state.
**
**  @return   The sound id of the created sound
*/
static int CclMakeSoundGroup(lua_State *l)
{
	CSound *id;
	std::string c_name;
	CSound *first;
	CSound *second;
	LuaUserData *data;

	LuaCheckArgs(l, 3);

	c_name = LuaToString(l, 1);

	lua_pushvalue(l, 2);
	first = CclGetSound(l);
	lua_pop(l, 1);
	second = CclGetSound(l);
	id = MakeSoundGroup(c_name, first, second);
	data = (LuaUserData *)lua_newuserdata(l, sizeof(LuaUserData));
	data->Type = LuaSoundType;
	data->Data = id;
	return 1;
}

/**
**  Glue between c and scheme. Ask to the sound system to remap a sound id
**  to a given name.
**
**  @param l  Lua state.
**
**  @return   the sound object
*/
static int CclMapSound(lua_State *l)
{
	const char *sound_name;

	LuaCheckArgs(l, 2);
	sound_name = LuaToString(l, 1);
	MapSound(sound_name, CclGetSound(l));
	lua_pushvalue(l, 2);
	return 1;
}

/**
**  Ask the sound system to play the specified sound.
**
**  @param l  Lua state.
*/
static int CclPlaySound(lua_State *l)
{
	const int args = lua_gettop(l);
	if (args < 1 || args > 2) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	CSound *id = CclGetSound(l);
	lua_pop(l, 1);
	bool always = false;
	if (args == 2) {
		always = LuaToBoolean(l, 2);
	}
	PlayGameSound(id, MaxSampleVolume, always);
	return 0;
}

static void SetSoundConfigRace(lua_State *l, int j, SoundConfig soundConfigs[])
{
	if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
		LuaError(l, "incorrect argument");
	}
	const char *civilization_ident = LuaToString(l, j + 1, 1);
	const CCivilization *civilization = CCivilization::Get(civilization_ident);
	if (!civilization) {
		return;
	}
	lua_rawgeti(l, j + 1, 2);
	LuaUserData *data = nullptr;
	if (!lua_isuserdata(l, -1)
		|| (data = (LuaUserData *)lua_touserdata(l, -1))->Type != LuaSoundType) {
		LuaError(l, "Sound id expected");
	}
	lua_pop(l, 1);
	soundConfigs[civilization->GetIndex()].Sound = (CSound *)data->Data;
}

/**
**  Glue between c and scheme. Allows to specify some global game sounds
**  in a ccl file.
**
**  @param l  Lua state.
*/
static int CclDefineGameSounds(lua_State *l)
{
	//FIXME: should allow to define ALL the game sounds

	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		LuaUserData *data = nullptr;

		// let's handle now the different cases
		if (!strcmp(value, "click")) {
			if (!lua_isuserdata(l, j + 1)
				|| (data = (LuaUserData *)lua_touserdata(l, j + 1))->Type != LuaSoundType) {
				LuaError(l, "Sound id expected");
			}
			GameSounds.Click.Sound = (CSound *)data->Data;
		} else if (!strcmp(value, "transport-docking")) {
			if (!lua_isuserdata(l, j + 1)
				|| (data = (LuaUserData *)lua_touserdata(l, j + 1))->Type != LuaSoundType) {
				LuaError(l, "Sound id expected");
			}
			GameSounds.Docking.Sound = (CSound *)data->Data;
		} else if (!strcmp(value, "placement-error")) {
			SetSoundConfigRace(l, j, GameSounds.PlacementError);
		} else if (!strcmp(value, "placement-success")) {
			SetSoundConfigRace(l, j, GameSounds.PlacementSuccess);
		} else if (!strcmp(value, "work-complete")) {
			SetSoundConfigRace(l, j, GameSounds.WorkComplete);
		} else if (!strcmp(value, "research-complete")) {
			SetSoundConfigRace(l, j, GameSounds.ResearchComplete);
		} else if (!strcmp(value, "not-enough-res")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const char *resName = LuaToString(l, j + 1, 1);
			const int resId = GetResourceIdByName(l, resName);
			const char *civilization_ident = LuaToString(l, j + 1, 2);
			const CCivilization *civilization = CCivilization::Get(civilization_ident);
			if (!civilization) {
				continue;
			}
			lua_rawgeti(l, j + 1, 3);
			if (!lua_isuserdata(l, -1)
				|| (data = (LuaUserData *)lua_touserdata(l, -1))->Type != LuaSoundType) {
				LuaError(l, "Sound id expected");
			}
			lua_pop(l, 1);
			GameSounds.NotEnoughRes[civilization->GetIndex()][resId].Sound = (CSound *)data->Data;
		} else if (!strcmp(value, "not-enough-food")) {
			SetSoundConfigRace(l, j, GameSounds.NotEnoughFood);
		} else if (!strcmp(value, "rescue")) {
			SetSoundConfigRace(l, j, GameSounds.Rescue);
		} else if (!strcmp(value, "building-construction")) {
			SetSoundConfigRace(l, j, GameSounds.BuildingConstruction);
		} else if (!strcmp(value, "chat-message")) {
			if (!lua_isuserdata(l, j + 1)
				|| (data = (LuaUserData *)lua_touserdata(l, j + 1))->Type != LuaSoundType) {
				LuaError(l, "Sound id expected");
			}
			GameSounds.ChatMessage.Sound = (CSound *)data->Data;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

/**
**  Set the cut off distance.
**
**  @param l  Lua state.
*/
static int CclSetGlobalSoundRange(lua_State *l)
{
	LuaCheckArgs(l, 1);

	// FIXME: check for errors
	int d = LuaToNumber(l, 1);
	if (d > 0) {
		DistanceSilent = d;
	}
	return 0;
}

/**
**  Set the range of a given sound.
**
**  @param l  Lua state.
*/
static int CclSetSoundRange(lua_State *l)
{

	LuaCheckArgs(l, 2);

	int tmp = LuaToNumber(l, 2);
	clamp(&tmp, 0, 255);
	const unsigned char theRange = static_cast<unsigned char>(tmp);

	lua_pushvalue(l, 1);
	CSound *id = CclGetSound(l);
	SetSoundRange(id, theRange);
	return 1;
}

//Wyrmgus start
/**
**  Set the volume percent of a given sound (useful for easily correcting how loud a sound is).
**
**  @param l  Lua state.
*/
static int CclSetSoundVolumePercent(lua_State *l)
{

	LuaCheckArgs(l, 2);

	lua_pushvalue(l, 1);
	CSound *id = CclGetSound(l);
	SetSoundVolumePercent(id, LuaToNumber(l, 2));
	return 1;
}

/**
**  Get sound idents
**
**  @param l  Lua state.
*/
static int CclGetSounds(lua_State *l)
{
	lua_createtable(l, SoundMap.size(), 0);
	std::map<std::string, CSound *>::iterator i;
	int j = 1;
	for (i = SoundMap.begin(); i != SoundMap.end(); ++i) {
		lua_pushstring(l, (*i).first.c_str());
		lua_rawseti(l, -2, j);
		++j;
	}
	
	return 1;
}
//Wyrmgus end

/**
**  Register CCL features for sound.
*/
void SoundCclRegister()
{
	lua_register(Lua, "SetGlobalSoundRange", CclSetGlobalSoundRange);
	lua_register(Lua, "DefineGameSounds", CclDefineGameSounds);
	lua_register(Lua, "MapSound", CclMapSound);
	lua_register(Lua, "SoundForName", CclSoundForName);
	lua_register(Lua, "SetSoundRange", CclSetSoundRange);
	//Wyrmgus start
	lua_register(Lua, "SetSoundVolumePercent", CclSetSoundVolumePercent);
	lua_register(Lua, "GetSounds", CclGetSounds);
	//Wyrmgus end
	lua_register(Lua, "MakeSound", CclMakeSound);
	lua_register(Lua, "MakeSoundGroup", CclMakeSoundGroup);
	lua_register(Lua, "PlaySound", CclPlaySound);
}
