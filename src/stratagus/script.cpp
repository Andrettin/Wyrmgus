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
/**@name script.cpp - The configuration language. */
//
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon, Joris Dauphin and Andrettin
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

#include <csignal>

#include "stratagus.h"

#include "script.h"

//Wyrmgus start
#include "actions.h"
//Wyrmgus end
#include "config.h"
#include "economy/resource_storage_type.h"
//Wyrmgus start
#include "editor.h"
//Wyrmgus end
#include "game/game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iocompat.h"
#include "iolib.h"
#include "item/unique_item.h"
#include "map/map.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_feature.h"
#include "map/tile.h"
#include "map/world.h"
#include "parameters.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/faction_type.h"
#include "player/player.h"
#include "script/trigger.h"
#include "spell/spell.h"
#include "time/timeline.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
//Wyrmgus start
#include "unit/unit_manager.h" //for checking units of a custom unit type and deleting them if the unit type has been removed
#include "unit/unit_type.h"
//Wyrmgus end
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/number_util.h"
#include "util/util.h"
#include "util/vector_util.h"
#include "video/font.h"

lua_State *Lua;                       /// Structure to work with lua files.

int CclInConfigFile;                  /// True while config file parsing

std::unique_ptr<NumberDesc> Damage;                   /// Damage calculation for missile.

static int NumberCounter = 0; /// Counter for lua function.
static int StringCounter = 0; /// Counter for lua function.

//Wyrmgus start
std::map<std::string, std::string> DLCFileEquivalency;
//Wyrmgus end

/**
**  FIXME: docu
*/
static void lstop(lua_State *l, lua_Debug *ar)
{
	(void)ar;  // unused arg.
	lua_sethook(l, nullptr, 0, 0);
	luaL_error(l, "interrupted!");
}

/**
**  FIXME: docu
*/
static void laction(int i)
{
	// if another SIGINT happens before lstop,
	// terminate process (default action)
	signal(i, SIG_DFL);
	lua_sethook(Lua, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

/**
**  Check error status, and print error message and exit
**  if status is different of 0.
**
**  @param status       status of the last lua call. (0: success)
**  @param exitOnError  exit the program on error
**
**  @return             0 in success, else exit.
*/
static int report(int status, bool exitOnError)
{
	if (status) {
		std::string msg = lua_tostring(Lua, -1);

		if (msg.empty()) {
			msg = "(error with no message)";
		}

		msg = "Lua error: " + msg;

		lua_pop(Lua, 1);

		if (exitOnError) {
			throw std::runtime_error(msg);
		} else {
			log::log_error(msg);
		}
	}
	return status;
}

static int luatraceback(lua_State *L)
{
	lua_getglobal(L, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_pushliteral(L, "traceback");
	lua_gettable(L, -2);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  // pass error message
	lua_pushnumber(L, 2);  // skip this function and traceback
	lua_call(L, 2, 1);  // call debug.traceback
	return 1;
}

/**
**  Call a lua function
**
**  @param narg         Number of arguments
**  @param clear        Clear the return value(s)
**  @param exitOnError  Exit the program when an error occurs
**
**  @return             0 in success, else exit.
*/
int LuaCall(int narg, int clear, bool exitOnError)
{
	const int base = lua_gettop(Lua) - narg;  // function index
	lua_pushcfunction(Lua, luatraceback);  // push traceback function
	lua_insert(Lua, base);  // put it under chunk and args
	signal(SIGINT, laction);
	const int status = lua_pcall(Lua, narg, (clear ? 0 : LUA_MULTRET), base);
	signal(SIGINT, SIG_DFL);
	lua_remove(Lua, base);  // remove traceback function

	return report(status, exitOnError);
}

/**
**  Get the (uncompressed) content of the file into a string
*/
static bool GetFileContent(const std::string &file, std::string &content)
{
	CFile fp;

	content.clear();
	if (fp.open(file.c_str(), CL_OPEN_READ) == -1) {
		log::log_error("Can't open file \"" + file + "\": " + strerror(errno));
		return false;
	}

	const int size = 10000;
	std::vector<char> buf;
	buf.resize(size);
	int location = 0;
	for (;;) {
		int read = fp.read(&buf[location], size);
		if (read != size) {
			location += read;
			break;
		}
		location += read;
		buf.resize(buf.size() + size);
	}
	fp.close();
	content.assign(&buf[0], location);
	return true;
}

/**
**  Load a file and execute it
**
**  @param file  File to load and execute
**
**  @return      0 for success, else exit.
*/
int LuaLoadFile(const std::string &file, const std::string &strArg)
{
	DebugPrint("Loading '%s'\n" _C_ file.c_str());

	std::string content;
	if (GetFileContent(file, content) == false) {
		return -1;
	}
	const int status = luaL_loadbuffer(Lua, content.c_str(), content.size(), file.c_str());

	if (!status) {
		if (!strArg.empty()) {
			lua_pushstring(Lua, strArg.c_str());
			LuaCall(1, 1);
		} else {
			LuaCall(0, 1);
		}
	} else {
		report(status, true);
	}
	return status;
}

/**
**  Save preferences
**
**  @param l  Lua state.
*/
static int CclSavePreferences(lua_State *l)
{
	LuaCheckArgs(l, 0);
	SavePreferences();
	return 0;
}

/**
**  Load a file and execute it.
**
**  @param l  Lua state.
**
**  @return   0 in success, else exit.
*/
static int CclLoad(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const std::string filename = LibraryFileName(LuaToString(l, 1));

	if (LuaLoadFile(filename) == -1) {
		DebugPrint("Load failed: %s\n" _C_ filename.c_str());
	}
	return 0;
}

/**
**	@brief	Load a config file.
**
**	@param	l	Lua state.
**
**	@return	0 in success, else exit.
*/
static int CclLoadConfigFile(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const std::string filename = LibraryFileName(LuaToString(l, 1));
	
	try {
		CConfigData::ParseConfigData(filename, DefiningData);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error parsing config file \"" + filename + "\"."));
	}

	return 0;
}

/**
**  Load a file into a buffer and return it.
**
**  @param l  Lua state.
**
**  @return   buffer or nil on failure
*/
static int CclLoadBuffer(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const std::string file = LibraryFileName(LuaToString(l, 1));
	DebugPrint("Loading '%s'\n" _C_ file.c_str());
	std::string content;
	if (GetFileContent(file, content) == false) {
		return 0;
	}
	lua_pushstring(l, content.c_str());
	return 1;
}

/**
**  Convert lua string in char*.
**  It checks also type and exit in case of error.
**
**  @note char* could be invalidated with lua garbage collector.
**
**  @param l     Lua state.
**  @param narg  Argument number.
**
**  @return      char* from lua.
*/
const char *LuaToString(lua_State *l, int narg)
{
	luaL_checktype(l, narg, LUA_TSTRING);
	return lua_tostring(l, narg);
}

/**
**  Convert lua number in C number.
**  It checks also type and exit in case of error.
**
**  @param l     Lua state.
**  @param narg  Argument number.
**
**  @return      C number from lua.
*/
int LuaToNumber(lua_State *l, int narg)
{
	luaL_checktype(l, narg, LUA_TNUMBER);
	return static_cast<int>(lua_tonumber(l, narg));
}

/**
**  Convert lua number in C unsigned int.
**  It checks also type and exit in case of error.
**
**  @param l     Lua state.
**  @param narg  Argument number.
**
**  @return      C number from lua.
*/
unsigned int LuaToUnsignedNumber(lua_State *l, int narg)
{
	luaL_checktype(l, narg, LUA_TNUMBER);
	return static_cast<unsigned int>(lua_tonumber(l, narg));
}

/**
**  Convert lua boolean to bool.
**  It also checks type and exits in case of error.
**
**  @param l     Lua state.
**  @param narg  Argument number.
**
**  @return      1 for true, 0 for false from lua.
*/
bool LuaToBoolean(lua_State *l, int narg)
{
	luaL_checktype(l, narg, LUA_TBOOLEAN);
	return lua_toboolean(l, narg) != 0;
}

const char *LuaToString(lua_State *l, int index, int subIndex)
{
	luaL_checktype(l, index, LUA_TTABLE);
	lua_rawgeti(l, index, subIndex);
	const char *res = LuaToString(l, -1);
	lua_pop(l, 1);
	return res;
}

int LuaToNumber(lua_State *l, int index, int subIndex)
{
	luaL_checktype(l, index, LUA_TTABLE);
	lua_rawgeti(l, index, subIndex);
	const int res = LuaToNumber(l, -1);
	lua_pop(l, 1);
	return res;
}

unsigned int LuaToUnsignedNumber(lua_State *l, int index, int subIndex)
{
	luaL_checktype(l, index, LUA_TTABLE);
	lua_rawgeti(l, index, subIndex);
	const unsigned int res = LuaToUnsignedNumber(l, -1);
	lua_pop(l, 1);
	return res;
}

bool LuaToBoolean(lua_State *l, int index, int subIndex)
{
	luaL_checktype(l, index, LUA_TTABLE);
	lua_rawgeti(l, index, subIndex);
	const bool res = LuaToBoolean(l, -1);
	lua_pop(l, 1);
	return res;
}


/**
**  Perform lua garbage collection
*/
void LuaGarbageCollect()
{
#if LUA_VERSION_NUM >= 501
	DebugPrint("Garbage collect (before): %d\n" _C_ lua_gc(Lua, LUA_GCCOUNT, 0));
	lua_gc(Lua, LUA_GCCOLLECT, 0);
	DebugPrint("Garbage collect (after): %d\n" _C_ lua_gc(Lua, LUA_GCCOUNT, 0));
#else
	DebugPrint("Garbage collect (before): %d/%d\n" _C_  lua_getgccount(Lua) _C_ lua_getgcthreshold(Lua));
	lua_setgcthreshold(Lua, 0);
	DebugPrint("Garbage collect (after): %d/%d\n" _C_ lua_getgccount(Lua) _C_ lua_getgcthreshold(Lua));
#endif
}

/**
**  Parse binary operation with number.
**
**  @param l       lua state.
**  @param binop   Where to stock info (must be malloced)
*/
static void ParseBinOp(lua_State *l, BinOp *binop)
{
	assert_throw(l != nullptr);
	assert_throw(binop != nullptr);
	assert_throw(lua_istable(l, -1));
	assert_throw(lua_rawlen(l, -1) == 2);

	lua_rawgeti(l, -1, 1); // left
	binop->Left = CclParseNumberDesc(l);
	lua_rawgeti(l, -1, 2); // right
	binop->Right = CclParseNumberDesc(l);
	lua_pop(l, 1); // table.
}

/**
**  Convert the string to the corresponding data (which is a unit).
**
**  @param l   lua state.
**  @param s   Ident.
**
**  @return    The reference of the unit.
**
**  @todo better check for error (restrict param).
*/
static CUnit **Str2UnitRef(lua_State *l, const char *s)
{
	CUnit **res; // Result.

	assert_throw(l != nullptr);
	assert_throw(s != nullptr);
	res = nullptr;
	if (!strcmp(s, "Attacker")) {
		res = &TriggerData.Attacker;
	} else if (!strcmp(s, "Defender")) {
		res = &TriggerData.Defender;
	} else if (!strcmp(s, "Active")) {
		res = &TriggerData.Active;
	//Wyrmgus start
	} else if (!strcmp(s, "Unit")) {
		res = &TriggerData.Unit;
	//Wyrmgus end
	} else {
		LuaError(l, "Invalid unit reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); // Must check for error.
	return res;
}

/**
**  Convert the string to the corresponding data (which is a unit type).
**
**  @param l   lua state.
**  @param s   Ident.
**
**  @return    The reference of the unit type.
**
**  @todo better check for error (restrict param).
*/
static const wyrmgus::unit_type **Str2TypeRef(lua_State *l, const char *s)
{
	const wyrmgus::unit_type **res = nullptr; // Result.

	assert_throw(l != nullptr);
	if (!strcmp(s, "Type")) {
		res = &TriggerData.Type;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); // Must check for error.
	return res;
}

//Wyrmgus start
/**
**  Convert the string to the corresponding data (which is an upgrade).
**
**  @param l   lua state.
**  @param s   Ident.
**
**  @return    The reference of the upgrade.
**
**  @todo better check for error (restrict param).
*/
static const CUpgrade **Str2UpgradeRef(lua_State *l, const char *s)
{
	const CUpgrade **res = nullptr; // Result.

	assert_throw(l != nullptr);
	if (!strcmp(s, "Upgrade")) {
		res = &TriggerData.Upgrade;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); // Must check for error.
	return res;
}

/**
**  Convert the string to the corresponding index (which is a resource index).
**
**  @param l   lua state.
**  @param s   Ident.
**
**  @return    The index of the resource.
**
**  @todo better check for error (restrict param).
*/
static const wyrmgus::resource **Str2ResourceRef(lua_State *l, const char *s)
{
	const wyrmgus::resource **res = nullptr; // Result.

	assert_throw(l != nullptr);
	if (!strcmp(s, "Resource")) {
		res = &TriggerData.resource;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); // Must check for error.
	return res;
}

/**
**  Convert the string to the corresponding data (which is a faction).
**
**  @param l   lua state.
**  @param s   Ident.
**
**  @return    The reference of the faction.
**
**  @todo better check for error (restrict param).
*/
static const wyrmgus::faction **Str2FactionRef(lua_State *l, const char *s)
{
	const wyrmgus::faction **res = nullptr; // Result.

	assert_throw(l != nullptr);
	if (!strcmp(s, "Faction")) {
		res = &TriggerData.faction;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); // Must check for error.
	return res;
}
//Wyrmgus end

static const CPlayer **Str2PlayerRef(lua_State *l, const char *s)
{
	const CPlayer **res = nullptr; // Result.

	assert_throw(l != nullptr);
	if (!strcmp(s, "Player")) {
		res = &TriggerData.player;
	} else {
		LuaError(l, "Invalid player reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); // Must check for error.
	return res;
}

static const tile **Str2TileRef(lua_State *l, const char *s)
{
	const tile **res = nullptr; //result.

	assert_throw(l != nullptr);
	if (!strcmp(s, "Tile")) {
		res = &TriggerData.tile;
	} else {
		LuaError(l, "Invalid tile reference '%s'\n" _C_ s);
	}
	assert_throw(res != nullptr); //check for error.
	return res;
}

/**
**  Return unit referernce definition.
**
**  @param l  lua state.
**
**  @return   unit referernce definition.
*/
std::unique_ptr<UnitDesc> CclParseUnitDesc(lua_State *l)
{
	auto res = std::make_unique<UnitDesc>();  // Result

	if (lua_isstring(l, -1)) {
		res->e = EUnit_Ref;
		res->D.AUnit = Str2UnitRef(l, LuaToString(l, -1));
	} else {
		LuaError(l, "Parse Error in ParseUnit\n");
	}
	lua_pop(l, 1);

	return res;
}

/**
**  Return unit type referernce definition.
**
**  @param l  lua state.
**
**  @return   unit type referernce definition.
*/
const wyrmgus::unit_type **CclParseTypeDesc(lua_State *l)
{
	const wyrmgus::unit_type **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2TypeRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseUnit\n");
	}
	return res;
}

//Wyrmgus start
/**
**  Return upgrade reference definition.
**
**  @param l  lua state.
**
**  @return   upgrade reference definition.
*/
const CUpgrade **CclParseUpgradeDesc(lua_State *l)
{
	const CUpgrade **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2UpgradeRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseUpgrade\n");
	}
	return res;
}

/**
**  Return resource index.
**
**  @param l  lua state.
**
**  @return   resource index.
*/
const wyrmgus::resource **CclParseResourceDesc(lua_State *l)
{
	const wyrmgus::resource **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2ResourceRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseResource\n");
	}
	return res;
}

/**
**  Return faction reference definition.
**
**  @param l  lua state.
**
**  @return   faction reference definition.
*/
const wyrmgus::faction **CclParseFactionDesc(lua_State *l)
{
	const wyrmgus::faction **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2FactionRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseFaction\n");
	}
	return res;
}
//Wyrmgus end

const tile **CclParseTileDesc(lua_State *l)
{
	const tile **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2TileRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseTile\n");
	}

	return res;
}

const CPlayer **CclParsePlayerDesc(lua_State *l)
{
	const CPlayer **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2PlayerRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseFaction\n");
	}
	return res;
}

/**
**  Add a Lua handler
**
**  @param l          lua state.
**  @param tablename  name of the lua table.
**  @param counter    Counter for the handler
**
**  @return handle of the function.
*/
static int ParseLuaFunction(lua_State *l, const char *tablename, int *counter)
{
	lua_getglobal(l, tablename);
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_newtable(l);
		lua_setglobal(l, tablename);
		lua_getglobal(l, tablename);
	}
	lua_pushvalue(l, -2);
	lua_rawseti(l, -2, *counter);
	lua_pop(l, 1);
	return (*counter)++;
}

/**
**  Call a Lua handler
**
**  @param handler  handler of the lua function to call.
**
**  @return  lua function result.
*/
static int CallLuaNumberFunction(unsigned int handler)
{
	const int narg = lua_gettop(Lua);

	lua_getglobal(Lua, "_numberfunction_");
	lua_rawgeti(Lua, -1, handler);
	LuaCall(0, 0);
	if (lua_gettop(Lua) - narg != 2) {
		LuaError(Lua, "Function must return one value.");
	}
	const int res = LuaToNumber(Lua, -1);
	lua_pop(Lua, 2);
	return res;
}

/**
**  Call a Lua handler
**
**  @param handler  handler of the lua function to call.
**
**  @return         lua function result.
*/
static std::string CallLuaStringFunction(unsigned int handler)
{
	const int narg = lua_gettop(Lua);
	lua_getglobal(Lua, "_stringfunction_");
	lua_rawgeti(Lua, -1, handler);
	LuaCall(0, 0);
	if (lua_gettop(Lua) - narg != 2) {
		LuaError(Lua, "Function must return one value.");
	}
	std::string res = LuaToString(Lua, -1);
	lua_pop(Lua, 2);
	return res;
}

/**
**  Gets the player data.
**
**  @param player  Player number.
**  @param prop    Player's property.
**  @param arg     Additional argument (for resource and unit).
**
**  @return  Returning value (only integer).
*/
static int GetPlayerData(const int player_index, const char *prop, const char *arg)
{
	const CPlayer *player = CPlayer::Players[player_index].get();

	if (!strcmp(prop, "RaceName")) {
		return player->Race;
	} else if (!strcmp(prop, "Resources")) {
		const resource *resource = resource::get(arg);
		return player->get_resource(resource, resource_storage_type::both);
	} else if (!strcmp(prop, "StoredResources")) {
		const resource *resource = resource::get(arg);
		return player->get_stored_resource(resource);
	} else if (!strcmp(prop, "MaxResources")) {
		const resource *resource = resource::get(arg);
		return player->get_max_resource(resource);
	} else if (!strcmp(prop, "Incomes")) {
		const resource *resource = resource::get(arg);
		return player->get_income(resource);
		//Wyrmgus start
	} else if (!strcmp(prop, "Prices")) {
		const resource *resource = resource::get(arg);
		return player->get_resource_price(resource);
	} else if (!strcmp(prop, "ResourceDemand")) {
		const resource *resource = resource::get(arg);
		return player->get_resource_demand(resource);
	} else if (!strcmp(prop, "StoredResourceDemand")) {
		const resource *resource = resource::get(arg);
		return player->get_stored_resource_demand(resource);
	} else if (!strcmp(prop, "EffectiveResourceDemand")) {
		const resource *resource = resource::get(arg);
		return player->get_effective_resource_demand(resource);
	} else if (!strcmp(prop, "EffectiveResourceBuyPrice")) {
		const resource *resource = resource::get(arg);
		return player->get_effective_resource_buy_price(resource);
	} else if (!strcmp(prop, "EffectiveResourceSellPrice")) {
		const resource *resource = resource::get(arg);
		return player->get_effective_resource_sell_price(resource);
	} else if (!strcmp(prop, "TradeCost")) {
		return player->TradeCost;
		//Wyrmgus end
	} else if (!strcmp(prop, "UnitTypesCount")) {
		const std::string unit(arg);
		wyrmgus::unit_type *type = wyrmgus::unit_type::get(unit);
		return player->GetUnitTypeCount(type);
	} else if (!strcmp(prop, "UnitTypesUnderConstructionCount")) {
		const std::string unit(arg);
		wyrmgus::unit_type *type = wyrmgus::unit_type::get(unit);
		return player->GetUnitTypeUnderConstructionCount(type);
	} else if (!strcmp(prop, "UnitTypesAiActiveCount")) {
		const std::string unit(arg);
		wyrmgus::unit_type *type = wyrmgus::unit_type::get(unit);
		return player->GetUnitTypeAiActiveCount(type);
	} else if (!strcmp(prop, "AiEnabled")) {
		return player->AiEnabled;
	} else if (!strcmp(prop, "TotalNumUnits")) {
		return player->GetUnitCount();
	} else if (!strcmp(prop, "NumBuildings")) {
		return player->NumBuildings;
		//Wyrmgus start
	} else if (!strcmp(prop, "NumBuildingsUnderConstruction")) {
		return player->NumBuildingsUnderConstruction;
		//Wyrmgus end
	} else if (!strcmp(prop, "Supply")) {
		return player->Supply;
	} else if (!strcmp(prop, "Demand")) {
		return player->Demand;
	} else if (!strcmp(prop, "UnitLimit")) {
		return player->UnitLimit;
	} else if (!strcmp(prop, "BuildingLimit")) {
		return player->BuildingLimit;
	} else if (!strcmp(prop, "TotalUnitLimit")) {
		return player->TotalUnitLimit;
	} else if (!strcmp(prop, "Score")) {
		return player->Score;
	} else if (!strcmp(prop, "TotalUnits")) {
		return player->TotalUnits;
	} else if (!strcmp(prop, "TotalBuildings")) {
		return player->TotalBuildings;
	} else if (!strcmp(prop, "TotalResources")) {
		const resource *resource = resource::get(arg);
		return player->get_resource_total(resource);
	} else if (!strcmp(prop, "TotalRazings")) {
		return player->TotalRazings;
	} else if (!strcmp(prop, "TotalKills")) {
		return player->TotalKills;
	} else if (!strcmp(prop, "Population")) {
		return player->get_population();
	} else if (!strcmp(prop, "Overlord")) {
		if (player->get_overlord() != nullptr) {
			return player->get_overlord()->get_index();
		}
		return -1;
	} else if (!strcmp(prop, "TopOverlord")) {
		if (player->get_overlord() != nullptr) {
			return player->get_top_overlord()->get_index();
		}
		return -1;
	} else {
		throw std::runtime_error("Invalid field: \"" + std::string(prop) + "\".");
	}
}

/**
**  Return number.
**
**  @param l  lua state.
**
**  @return   number.
*/
std::unique_ptr<NumberDesc> CclParseNumberDesc(lua_State *l)
{
	auto res = std::make_unique<NumberDesc>();

	if (lua_isnumber(l, -1)) {
		res->e = ENumber_Dir;
		res->D.Val = LuaToNumber(l, -1);
	} else if (lua_isfunction(l, -1)) {
		res->e = ENumber_Lua;
		res->D.Index = ParseLuaFunction(l, "_numberfunction_", &NumberCounter);
	} else if (lua_istable(l, -1)) {
		const int nargs = lua_rawlen(l, -1);
		if (nargs != 2) {
			LuaError(l, "Bad number of args in parse Number table\n");
		}
		lua_rawgeti(l, -1, 1); // key
		const char *key = LuaToString(l, -1);
		lua_pop(l, 1);
		lua_rawgeti(l, -1, 2); // table
		if (!strcmp(key, "Add")) {
			res->e = ENumber_Add;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Sub")) {
			res->e = ENumber_Sub;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Mul")) {
			res->e = ENumber_Mul;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Div")) {
			res->e = ENumber_Div;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Min")) {
			res->e = ENumber_Min;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Max")) {
			res->e = ENumber_Max;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Rand")) {
			res->e = ENumber_Rand;
			res->D.N = CclParseNumberDesc(l);
		} else if (!strcmp(key, "GreaterThan")) {
			res->e = ENumber_Gt;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "GreaterThanOrEq")) {
			res->e = ENumber_GtEq;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "LessThan")) {
			res->e = ENumber_Lt;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "LessThanOrEq")) {
			res->e = ENumber_LtEq;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "Equal")) {
			res->e = ENumber_Eq;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "NotEqual")) {
			res->e = ENumber_NEq;
			ParseBinOp(l, &res->D.binOp);
		} else if (!strcmp(key, "UnitVar")) {
			assert_throw(lua_istable(l, -1));

			res->e = ENumber_UnitStat;
			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				key = LuaToString(l, -2);
				if (!strcmp(key, "Unit")) {
					res->D.UnitStat.Unit = CclParseUnitDesc(l);
					lua_pushnil(l);
				} else if (!strcmp(key, "Variable")) {
					const char *const name = LuaToString(l, -1);
					res->D.UnitStat.Index = UnitTypeVar.VariableNameLookup[name];
					if (res->D.UnitStat.Index == -1) {
						LuaError(l, "Bad variable name :'%s'" _C_ LuaToString(l, -1));
					}
				} else if (!strcmp(key, "Component")) {
					res->D.UnitStat.Component = Str2VariableAttribute(l, LuaToString(l, -1));
				} else if (!strcmp(key, "Loc")) {
					res->D.UnitStat.Loc = LuaToNumber(l, -1);
					if (res->D.UnitStat.Loc < 0 || 2 < res->D.UnitStat.Loc) {
						LuaError(l, "Bad Loc number :'%d'" _C_ LuaToNumber(l, -1));
					}
				} else {
					LuaError(l, "Bad param %s for Unit" _C_ key);
				}
			}
			lua_pop(l, 1); // pop the table.
		} else if (!strcmp(key, "TypeVar")) {
			assert_throw(lua_istable(l, -1));

			res->e = ENumber_TypeStat;
			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				key = LuaToString(l, -2);
				if (!strcmp(key, "Type")) {
					res->D.TypeStat.Type = CclParseTypeDesc(l);
					lua_pushnil(l);
				} else if (!strcmp(key, "Component")) {
					res->D.TypeStat.Component = Str2VariableAttribute(l, LuaToString(l, -1));
				} else if (!strcmp(key, "Variable")) {
					const char *const name = LuaToString(l, -1);
					res->D.TypeStat.Index = UnitTypeVar.VariableNameLookup[name];
					if (res->D.TypeStat.Index == -1) {
						LuaError(l, "Bad variable name :'%s'" _C_ LuaToString(l, -1));
					}
				} else if (!strcmp(key, "Loc")) {
					res->D.TypeStat.Loc = LuaToNumber(l, -1);
					if (res->D.TypeStat.Loc < 0 || 2 < res->D.TypeStat.Loc) {
						LuaError(l, "Bad Loc number :'%d'" _C_ LuaToNumber(l, -1));
					}
				} else {
					LuaError(l, "Bad param %s for Unit" _C_ key);
				}
			}
			lua_pop(l, 1); // pop the table.
		} else if (!strcmp(key, "VideoTextLength")) {
			assert_throw(lua_istable(l, -1));
			res->e = ENumber_VideoTextLength;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				key = LuaToString(l, -2);
				if (!strcmp(key, "Text")) {
					res->D.VideoTextLength.String = CclParseStringDesc(l);
					lua_pushnil(l);
				} else if (!strcmp(key, "Font")) {
					res->D.VideoTextLength.Font = wyrmgus::font::get(LuaToString(l, -1));
					if (!res->D.VideoTextLength.Font) {
						LuaError(l, "Bad Font name :'%s'" _C_ LuaToString(l, -1));
					}
				} else {
					LuaError(l, "Bad param %s for VideoTextLength" _C_ key);
				}
			}
			lua_pop(l, 1); // pop the table.
		} else if (!strcmp(key, "StringFind")) {
			assert_throw(lua_istable(l, -1));
			res->e = ENumber_StringFind;
			if (lua_rawlen(l, -1) != 2) {
				LuaError(l, "Bad param for StringFind");
			}
			lua_rawgeti(l, -1, 1); // left
			res->D.StringFind.String = CclParseStringDesc(l);

			lua_rawgeti(l, -1, 2); // right
			res->D.StringFind.C = *LuaToString(l, -1);
			lua_pop(l, 1); // pop the char

			lua_pop(l, 1); // pop the table.
		} else if (!strcmp(key, "NumIf")) {
			res->e = ENumber_NumIf;
			if (lua_rawlen(l, -1) != 2 && lua_rawlen(l, -1) != 3) {
				LuaError(l, "Bad number of args in NumIf\n");
			}
			lua_rawgeti(l, -1, 1); // Condition.
			res->D.NumIf.Cond = CclParseNumberDesc(l);
			lua_rawgeti(l, -1, 2); // Then.
			res->D.NumIf.BTrue = CclParseNumberDesc(l);
			if (lua_rawlen(l, -1) == 3) {
				lua_rawgeti(l, -1, 3); // Else.
				res->D.NumIf.BFalse = CclParseNumberDesc(l);
			}
			lua_pop(l, 1); // table.
		} else if (!strcmp(key, "PlayerData")) {
			res->e = ENumber_PlayerData;
			if (lua_rawlen(l, -1) != 2 && lua_rawlen(l, -1) != 3) {
				LuaError(l, "Bad number of args in PlayerData\n");
			}
			lua_rawgeti(l, -1, 1); // Player.
			res->D.PlayerData.Player = CclParseNumberDesc(l);
			lua_rawgeti(l, -1, 2); // DataType.
			res->D.PlayerData.DataType = CclParseStringDesc(l);
			if (lua_rawlen(l, -1) == 3) {
				lua_rawgeti(l, -1, 3); // Res type.
				res->D.PlayerData.ResType = CclParseStringDesc(l);
			//Wyrmgus start
			} else {
				res->D.PlayerData.ResType = nullptr;
			//Wyrmgus end
			}
			lua_pop(l, 1); // table.
		//Wyrmgus start
		} else if (!strcmp(key, "TypeTrainQuantity")) {
			res->e = ENumber_TypeTrainQuantity;
			res->D.Type = CclParseTypeDesc(l);
		//Wyrmgus end
		} else if (!strcmp(key, "ButtonPlayer")) {
			res->e = ENumber_ButtonPlayer;
			res->D.player = CclParsePlayerDesc(l);
		} else {
			lua_pop(l, 1);
			LuaError(l, "unknown condition '%s'" _C_ key);
		}
	} else {
		LuaError(l, "Parse Error in ParseNumber");
	}
	lua_pop(l, 1);
	return res;
}

/**
**  Return String description.
**
**  @param l  lua state.
**
**  @return   String description.
*/
std::unique_ptr<StringDesc> CclParseStringDesc(lua_State *l)
{
	auto res = std::make_unique<StringDesc>();

	if (lua_isstring(l, -1)) {
		res->e = EString_Dir;
		res->D.Val = LuaToString(l, -1);
	} else if (lua_isfunction(l, -1)) {
		res->e = EString_Lua;
		res->D.Index = ParseLuaFunction(l, "_stringfunction_", &StringCounter);
	} else if (lua_istable(l, -1)) {
		const int nargs = lua_rawlen(l, -1);
		if (nargs != 2) {
			LuaError(l, "Bad number of args in parse String table\n");
		}
		lua_rawgeti(l, -1, 1); // key
		const char *key = LuaToString(l, -1);
		lua_pop(l, 1);
		lua_rawgeti(l, -1, 2); // table
		if (!strcmp(key, "Concat")) {
			res->e = EString_Concat;
			const size_t size = lua_rawlen(l, -1);
			if (size < 1) {
				LuaError(l, "Bad number of args in Concat\n");
			}
			for (size_t i = 0; i < size; ++i) {
				lua_rawgeti(l, -1, 1 + i);
				res->D.Concat.Strings.push_back(CclParseStringDesc(l));
			}
			lua_pop(l, 1); // table.
		} else if (!strcmp(key, "String")) {
			res->e = EString_String;
			res->D.Number = CclParseNumberDesc(l);
		} else if (!strcmp(key, "InverseVideo")) {
			res->e = EString_InverseVideo;
			res->D.String = CclParseStringDesc(l);
		} else if (!strcmp(key, "UnitName")) {
			res->e = EString_UnitName;
			res->D.Unit = CclParseUnitDesc(l);
		//Wyrmgus start
		} else if (!strcmp(key, "UnitTypeName")) {
			res->e = EString_UnitTypeName;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitTrait")) {
			res->e = EString_UnitTrait;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitSpell")) {
			res->e = EString_UnitSpell;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitQuote")) {
			res->e = EString_UnitQuote;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitSettlementName")) {
			res->e = EString_UnitSettlementName;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitSiteName")) {
			res->e = EString_UnitSiteName;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitUniqueSet")) {
			res->e = EString_UnitUniqueSet;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "UnitUniqueSetItems")) {
			res->e = EString_UnitUniqueSetItems;
			res->D.Unit = CclParseUnitDesc(l);
		} else if (!strcmp(key, "TypeName")) {
			res->e = EString_TypeName;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeIdent")) {
			res->e = EString_TypeIdent;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeClass")) {
			res->e = EString_TypeClass;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeDescription")) {
			res->e = EString_TypeDescription;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeQuote")) {
			res->e = EString_TypeQuote;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeRequirementsString")) {
			res->e = EString_TypeRequirementsString;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeExperienceRequirementsString")) {
			res->e = EString_TypeExperienceRequirementsString;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeBuildingRulesString")) {
			res->e = EString_TypeBuildingRulesString;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeImproveIncomes")) {
			res->e = EString_TypeImproveIncomes;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "TypeLuxuryDemand")) {
			res->e = EString_TypeLuxuryDemand;
			res->D.Type = CclParseTypeDesc(l);
		} else if (!strcmp(key, "UpgradeCivilization")) {
			res->e = EString_UpgradeCivilization;
			res->D.Upgrade = CclParseUpgradeDesc(l);
		} else if (!strcmp(key, "UpgradeEffectsString")) {
			res->e = EString_UpgradeEffectsString;
			res->D.Upgrade = CclParseUpgradeDesc(l);
		} else if (!strcmp(key, "UpgradeRequirementsString")) {
			res->e = EString_UpgradeRequirementsString;
			res->D.Upgrade = CclParseUpgradeDesc(l);
		} else if (!strcmp(key, "UpgradeMaxLimit")) {
			res->e = EString_UpgradeMaxLimit;
			res->D.Upgrade = CclParseUpgradeDesc(l);
		} else if (!strcmp(key, "FactionCivilization")) {
			res->e = EString_FactionCivilization;
			res->D.Faction = CclParseFactionDesc(l);
		} else if (!strcmp(key, "FactionType")) {
			res->e = EString_FactionType;
			res->D.Faction = CclParseFactionDesc(l);
		} else if (!strcmp(key, "FactionCoreSettlements")) {
			res->e = EString_FactionCoreSettlements;
			res->D.Faction = CclParseFactionDesc(l);
		} else if (!strcmp(key, "ResourceIdent")) {
			res->e = EString_ResourceIdent;
			res->D.Resource = CclParseResourceDesc(l);
		} else if (!strcmp(key, "ResourceName")) {
			res->e = EString_ResourceName;
			res->D.Resource = CclParseResourceDesc(l);
		} else if (!strcmp(key, "ResourceConversionRates")) {
			res->e = EString_ResourceConversionRates;
			res->D.Resource = CclParseResourceDesc(l);
		} else if (!strcmp(key, "ResourceImproveIncomes")) {
			res->e = EString_ResourceImproveIncomes;
			res->D.Resource = CclParseResourceDesc(l);
		//Wyrmgus end
		} else if (!strcmp(key, "If")) {
			res->e = EString_If;
			if (lua_rawlen(l, -1) != 2 && lua_rawlen(l, -1) != 3) {
				LuaError(l, "Bad number of args in If\n");
			}
			lua_rawgeti(l, -1, 1); // Condition.
			res->D.If.Cond = CclParseNumberDesc(l);
			lua_rawgeti(l, -1, 2); // Then.
			res->D.If.BTrue = CclParseStringDesc(l);
			if (lua_rawlen(l, -1) == 3) {
				lua_rawgeti(l, -1, 3); // Else.
				res->D.If.BFalse = CclParseStringDesc(l);
			}
			lua_pop(l, 1); // table.
		} else if (!strcmp(key, "SubString")) {
			res->e = EString_SubString;
			if (lua_rawlen(l, -1) != 2 && lua_rawlen(l, -1) != 3) {
				LuaError(l, "Bad number of args in SubString\n");
			}
			lua_rawgeti(l, -1, 1); // String.
			res->D.SubString.String = CclParseStringDesc(l);
			lua_rawgeti(l, -1, 2); // Begin.
			res->D.SubString.Begin = CclParseNumberDesc(l);
			if (lua_rawlen(l, -1) == 3) {
				lua_rawgeti(l, -1, 3); // End.
				res->D.SubString.End = CclParseNumberDesc(l);
			}
			lua_pop(l, 1); // table.
		} else if (!strcmp(key, "Line")) {
			res->e = EString_Line;
			if (lua_rawlen(l, -1) < 2 || lua_rawlen(l, -1) > 4) {
				LuaError(l, "Bad number of args in Line\n");
			}
			lua_rawgeti(l, -1, 1); // Line.
			res->D.Line.Line = CclParseNumberDesc(l);
			lua_rawgeti(l, -1, 2); // String.
			res->D.Line.String = CclParseStringDesc(l);
			if (lua_rawlen(l, -1) >= 3) {
				lua_rawgeti(l, -1, 3); // Length.
				res->D.Line.MaxLen = CclParseNumberDesc(l);
			}
			res->D.Line.Font = nullptr;
			if (lua_rawlen(l, -1) >= 4) {
				lua_rawgeti(l, -1, 4); // Font.
				res->D.Line.Font = wyrmgus::font::get(LuaToString(l, -1));
				if (!res->D.Line.Font) {
					LuaError(l, "Bad Font name :'%s'" _C_ LuaToString(l, -1));
				}
				lua_pop(l, 1); // font name.
			}
			lua_pop(l, 1); // table.
		} else if (!strcmp(key, "PlayerName")) {
			res->e = EString_PlayerName;
			res->D.PlayerName = CclParseNumberDesc(l);
		} else if (!strcmp(key, "PlayerFullName")) {
			res->e = EString_PlayerFullName;
			res->D.PlayerName = CclParseNumberDesc(l);
		} else if (!strcmp(key, "TileTerrainFeatureName")) {
			res->e = EString_TileTerrainFeatureName;
			res->D.tile = CclParseTileDesc(l);
		} else if (!strcmp(key, "TileWorldName")) {
			res->e = EString_TileWorldName;
			res->D.tile = CclParseTileDesc(l);
		} else {
			lua_pop(l, 1);
			LuaError(l, "unknow condition '%s'" _C_ key);
		}
	} else {
		LuaError(l, "Parse Error in ParseString");
	}
	lua_pop(l, 1);

	return res;
}

/**
**  compute the Unit expression
**
**  @param unitdesc  struct with definition of the calculation.
**
**  @return          the result unit.
*/
CUnit *EvalUnit(const UnitDesc *unitdesc)
{
	assert_throw(unitdesc != nullptr);

	if (!Selected.empty()) {
		TriggerData.Active = Selected[0];
	} else {
		TriggerData.Active = UnitUnderCursor;
	}
	switch (unitdesc->e) {
		case EUnit_Ref :
			return *unitdesc->D.AUnit;
	}
	return nullptr;
}

/**
**  compute the number expression
**
**  @param number  struct with definition of the calculation.
**
**  @return        the result number.
**
**  @todo Manage better the error (div/0, unit==null, ...).
*/
int EvalNumber(const NumberDesc *number)
{
	CUnit *unit;
	const wyrmgus::unit_type **type;
	std::string s;
	int a;
	int b;

	assert_throw(number != nullptr);
	switch (number->e) {
		case ENumber_Lua :     // a lua function.
			return CallLuaNumberFunction(number->D.Index);
		case ENumber_Dir :     // directly a number.
			return number->D.Val;
		case ENumber_Add :     // a + b.
			return EvalNumber(number->D.binOp.Left.get()) + EvalNumber(number->D.binOp.Right.get());
		case ENumber_Sub :     // a - b.
			return EvalNumber(number->D.binOp.Left.get()) - EvalNumber(number->D.binOp.Right.get());
		case ENumber_Mul :     // a * b.
			return EvalNumber(number->D.binOp.Left.get()) * EvalNumber(number->D.binOp.Right.get());
		case ENumber_Div :     // a / b.
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			if (!b) { // FIXME : manage better this.
				return 0;
			}
			return a / b;
		case ENumber_Min :     // a <= b ? a : b
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return std::min(a, b);
		case ENumber_Max :     // a >= b ? a : b
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return std::max(a, b);
		case ENumber_Gt  :     // a > b  ? 1 : 0
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return (a > b ? 1 : 0);
		case ENumber_GtEq :    // a >= b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return (a >= b ? 1 : 0);
		case ENumber_Lt  :     // a < b  ? 1 : 0
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return (a < b ? 1 : 0);
		case ENumber_LtEq :    // a <= b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return (a <= b ? 1 : 0);
		case ENumber_Eq  :     // a == b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return (a == b ? 1 : 0);
		case ENumber_NEq  :    // a != b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left.get());
			b = EvalNumber(number->D.binOp.Right.get());
			return (a != b ? 1 : 0);

		case ENumber_Rand :    // random(a) [0..a-1]
			a = EvalNumber(number->D.N.get());
			return SyncRand(a);
		case ENumber_UnitStat : // property of unit.
			unit = EvalUnit(number->D.UnitStat.Unit.get());
			if (unit != nullptr) {
				return GetComponent(*unit, number->D.UnitStat.Index,
									number->D.UnitStat.Component, number->D.UnitStat.Loc).i;
			} else { // ERROR.
				return 0;
			}
		case ENumber_TypeStat : // property of unit type.
			type = number->D.TypeStat.Type;
			if (type != nullptr) {
				return GetComponent(**type, number->D.TypeStat.Index,
									number->D.TypeStat.Component, number->D.TypeStat.Loc).i;
			} else { // ERROR.
				return 0;
			}
		case ENumber_VideoTextLength : // VideoTextLength(font, s)
			if (number->D.VideoTextLength.String != nullptr
				&& !(s = EvalString(number->D.VideoTextLength.String.get())).empty()) {
				return number->D.VideoTextLength.Font->Width(s);
			} else { // ERROR.
				return 0;
			}
		case ENumber_StringFind : // s.find(c)
			if (number->D.StringFind.String != nullptr
				&& !(s = EvalString(number->D.StringFind.String.get())).empty()) {
				size_t pos = s.find(number->D.StringFind.C);
				return pos != std::string::npos ? (int)pos : -1;
			} else { // ERROR.
				return 0;
			}
		case ENumber_NumIf : // cond ? True : False;
			if (EvalNumber(number->D.NumIf.Cond.get())) {
				return EvalNumber(number->D.NumIf.BTrue.get());
			} else if (number->D.NumIf.BFalse) {
				return EvalNumber(number->D.NumIf.BFalse.get());
			} else {
				return 0;
			}
		//Wyrmgus start
		case ENumber_TypeTrainQuantity : // name of the unit type's class
			type = number->D.Type;
			if (type != nullptr) {
				return (**type).TrainQuantity;
			} else { // ERROR.
				return 0;
			}
		//Wyrmgus end
		case ENumber_ButtonPlayer: // name of the unit type's class
			if (number->D.player != nullptr) {
				return (**number->D.player).get_index();
			} else { // ERROR.
				return 0;
			}
		case ENumber_PlayerData : // getplayerdata(player, data, res);
			int player = EvalNumber(number->D.PlayerData.Player.get());
			std::string data = EvalString(number->D.PlayerData.DataType.get());
			//Wyrmgus start
//			std::string res = EvalString(number->D.PlayerData.ResType);
			std::string res;
			if (number->D.PlayerData.ResType != nullptr) {
				res = EvalString(number->D.PlayerData.ResType.get());
			}
			//Wyrmgus end
			return GetPlayerData(player, data.c_str(), res.c_str());
	}
	return 0;
}

/**
**  compute the string expression
**
**  @param s  struct with definition of the calculation.
**
**  @return   the result string.
**
**  @todo Manage better the error.
*/
std::string EvalString(const StringDesc *s)
{
	std::string res;    // Result string.
	std::string tmp1;   // Temporary string.
	const CUnit *unit;  // Temporary unit
	//Wyrmgus start
	const wyrmgus::unit_type **type;	// Temporary unit type
	const CUpgrade **upgrade;	// Temporary upgrade
	const wyrmgus::resource **resource;		// Temporary resource
	const wyrmgus::faction **faction;	// Temporary faction
	//Wyrmgus end
	int player_index;

	assert_throw(s != nullptr);
	switch (s->e) {
		case EString_Lua :     // a lua function.
			return CallLuaStringFunction(s->D.Index);
		case EString_Dir :     // directly a string.
			return std::string(s->D.Val);
		case EString_Concat :     // a + b -> "ab"
			res = EvalString(s->D.Concat.Strings.front().get());
			for (size_t i = 1; i < s->D.Concat.Strings.size(); ++i) {
				res += EvalString(s->D.Concat.Strings[i].get());
			}
			return res;
		case EString_String : {   // 42 -> "42".
			return wyrmgus::number::to_formatted_string(EvalNumber(s->D.Number.get()));
		}
		case EString_InverseVideo : // "a" -> "~<a~>"
			tmp1 = EvalString(s->D.String.get());
			// FIXME replace existing "~<" by "~>" in tmp1.
			res = std::string("~<") + tmp1 + "~>";
			return res;
		case EString_UnitName : // name of the UnitType
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr) {
				//Wyrmgus start
//				return unit->Type->Name;
				if (!unit->get_name().empty() && unit->Identified) {
					return unit->get_name();
				} else if (!unit->Identified) {
					return unit->get_type_name() + " (Unidentified)";
				} else {
					return unit->get_type_name();
				}
				//Wyrmgus end
			} else { // ERROR.
				return std::string();
			}
		//Wyrmgus start
		case EString_UnitTypeName : // name of the UnitType
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && !unit->get_name().empty() && ((unit->Prefix == nullptr && unit->Suffix == nullptr && unit->Spell == nullptr) || unit->get_unique() != nullptr || unit->Work != nullptr || unit->Elixir != nullptr)) { //items with affixes use their type name in their given name, so there's no need to repeat their type name
				return unit->get_type_name();
			} else { // only return a unit type name if the unit has a personal name (otherwise the unit type name would be returned as the unit name)
				return std::string();
			}
		case EString_UnitTrait : // name of the unit's trait
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && unit->Trait != nullptr) {
				return _(unit->Trait->get_name().c_str());
			} else {
				return std::string();
			}
		case EString_UnitSpell : // name of the unit's spell
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && unit->Spell != nullptr) {
				std::string spell_description = unit->Spell->get_effects_string();
				spell_description[0] = tolower(spell_description[0]);
				return unit->Spell->get_name() + " (" + spell_description + ")";
			} else {
				return std::string();
			}
		case EString_UnitQuote : //unit's quote
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr) {
				if (unit->get_unique() == nullptr) {
					if (unit->Work != nullptr) {
						return unit->Work->get_quote();
					} else if (unit->Elixir != nullptr) {
						return unit->Elixir->get_quote();
					} else {
						return unit->Type->get_quote();
					}
				} else {
					return unit->get_unique()->get_quote();
				}
			} else {
				return std::string();
			}
		case EString_UnitSettlementName: //name of the unit's settlement
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && unit->settlement != nullptr) {
				return unit->settlement->get_game_data()->get_current_cultural_name();
			} else {
				return std::string();
			}
		case EString_UnitSiteName: //name of the unit's site
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && unit->get_site() != nullptr) {
				return unit->get_site()->get_game_data()->get_current_cultural_name();
			} else {
				return std::string();
			}
		case EString_UnitUniqueSet : //name of the unit's unique item set
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && unit->get_unique() != nullptr && unit->get_unique()->get_set() != nullptr) {
				return unit->get_unique()->get_set()->get_name();
			} else {
				return std::string();
			}
		case EString_UnitUniqueSetItems : // names of the unit's unique item set's items
			unit = EvalUnit(s->D.Unit.get());
			if (unit != nullptr && unit->get_unique() != nullptr && unit->get_unique()->get_set() != nullptr) {
				std::string set_items_string;
				bool first = true;
				for (const wyrmgus::unique_item *unique_item : unit->get_unique()->get_set()->UniqueItems) {
					if (!first) {
						set_items_string += "\n";
					} else {
						first = false;
					}
					const bool item_equipped = unit->Container && unit->Container->IsUniqueItemEquipped(unique_item);
					if (!item_equipped) {
						set_items_string += "~<";
					}
					set_items_string += unique_item->get_name();
					if (!item_equipped) {
						set_items_string += "~>";
					}
				}
				return set_items_string;
			} else {
				return std::string();
			}
		case EString_TypeName : // name of the unit type
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).get_name();
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeIdent : // name of the unit type
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).Ident;
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeClass : // name of the unit type's class
			type = s->D.Type;
			if (type != nullptr) {
				std::string str;
				if ((**type).BoolFlag[ITEM_INDEX].value) {
					str = wyrmgus::item_class_to_string((**type).get_item_class());
					str[0] = toupper(str[0]);
					size_t loc;
					while ((loc = str.find("_")) != std::string::npos) {
						str.replace(loc, 1, " ");
						str[loc + 1] = toupper(str[loc + 1]);
					}
				} else if ((**type).get_unit_class() != nullptr) {
					str = (**type).get_unit_class()->get_name().c_str();
				}
				return _(str.c_str());
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeDescription : // name of the unit type's description
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).get_description();
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeQuote : // name of the unit type's quote
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).get_quote();
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeRequirementsString : // name of the unit type's requirements string
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).RequirementsString;
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeExperienceRequirementsString : // name of the unit type's experience requirements string
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).ExperienceRequirementsString;
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeBuildingRulesString : // name of the unit type's building rules string
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).BuildingRulesString;
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeImproveIncomes : // unit type's processing bonuses
			type = s->D.Type;
			if (type != nullptr) {
				std::string improve_incomes;
				bool first = true;
				for (const auto &[loop_resource, quantity] : (**type).Stats[CPlayer::GetThisPlayer()->get_index()].get_improve_incomes()) {
					if (loop_resource->get_index() == TimeCost) {
						continue;
					}

					if (quantity > loop_resource->get_default_income()) {
						if (!first) {
							improve_incomes += "\n";
						} else {
							first = false;
						}
						improve_incomes += loop_resource->get_name();
						improve_incomes += " Processing Bonus: +";
						improve_incomes += std::to_string(quantity - loop_resource->get_default_income());
						improve_incomes += "%";
					}
				}
				return improve_incomes;
			} else { // ERROR.
				return std::string();
			}
		case EString_TypeLuxuryDemand : // unit type's luxury demand
			type = s->D.Type;
			if (type != nullptr) {
				std::string luxury_demand;
				bool first = true;
				for (const auto &[loop_resource, quantity] : (**type).Stats[CPlayer::GetThisPlayer()->get_index()].get_resource_demands()) {
					if (loop_resource->get_index() == TimeCost) {
						continue;
					}

					if (!first) {
						luxury_demand += "\n";
					} else {
						first = false;
					}
					luxury_demand += loop_resource->get_name();
					luxury_demand += " Demand: ";
					luxury_demand += std::to_string(quantity);
				}
				return luxury_demand;
			} else { // ERROR.
				return std::string();
			}
		case EString_UpgradeCivilization : // name of the upgrade's civilization
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				if ((**upgrade).get_civilization() != nullptr) {
					return (**upgrade).get_civilization()->get_name();
				} else {
					return std::string();
				}
			} else { // ERROR.
				return std::string();
			}
		case EString_UpgradeEffectsString : // upgrade's effects string
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				return (**upgrade).get_effects_string();
			} else { // ERROR.
				return std::string();
			}
		case EString_UpgradeRequirementsString : // upgrade's effects string
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				return (**upgrade).get_requirements_string();
			} else { // ERROR.
				return std::string();
			}
		case EString_UpgradeMaxLimit : // upgrade's max limit
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				return std::to_string((**upgrade).MaxLimit);
			} else { // ERROR.
				return std::string();
			}
		case EString_FactionCivilization : // name of the faction's civilization
			faction = s->D.Faction;
			
			if (faction != nullptr) {
				return (**faction).get_civilization()->get_name();
			} else {
				return std::string();
			}
		case EString_FactionType : // the faction's type
			faction = s->D.Faction;
			
			if (faction != nullptr) {
				return IdentToName(wyrmgus::faction_type_to_string((**faction).get_type()));
			} else {
				return std::string();
			}
		case EString_FactionCoreSettlements : // the faction's core settlements
			faction = s->D.Faction;
			
			if (faction != nullptr) {
				std::string settlements_string;
				bool first = true;
				for (const wyrmgus::site *core_settlement : (**faction).get_core_settlements()) {
					const wyrmgus::site_game_data *core_settlement_game_data = core_settlement->get_game_data();
					if (!first) {
						settlements_string += "\n";
					} else {
						first = false;
					}

					const CUnit *site_unit = core_settlement_game_data->get_site_unit();

					const bool has_settlement = site_unit != nullptr && site_unit->Player == CPlayer::GetThisPlayer() && site_unit->CurrentAction() != UnitAction::Built;

					if (!has_settlement) {
						settlements_string += "~<";
					}

					settlements_string += core_settlement_game_data->get_current_cultural_name();

					if (!has_settlement) {
						settlements_string += "~>";
					}
				}
				return settlements_string;
			} else {
				return std::string();
			}
		case EString_ResourceIdent : // resource ident
			resource = s->D.Resource;
			if (resource != nullptr) {
				return (*resource)->get_identifier();
			} else { // ERROR.
				return std::string();
			}
		case EString_ResourceName : // resource ident
			resource = s->D.Resource;
			if (resource != nullptr) {
				return (*resource)->get_name();
			} else { // ERROR.
				return std::string();
			}
		case EString_ResourceConversionRates : // unit type's processing bonuses
			resource = s->D.Resource;
			if (resource != nullptr) {
				std::string conversion_rates;
				bool first = true;
				for (const wyrmgus::resource *child_resource : (*resource)->ChildResources) {
					if (child_resource->get_index() == TradeCost || child_resource->Hidden) {
						continue;
					}
					if (!first) {
						conversion_rates += "\n";
					} else {
						first = false;
					}
					conversion_rates += child_resource->get_name();
					conversion_rates += " to ";
					conversion_rates += (*resource)->get_name();
					conversion_rates += " Conversion Rate: ";
					conversion_rates += std::to_string(child_resource->get_final_resource_conversion_rate());
					conversion_rates += "%";
				}
				return conversion_rates;
			} else { // ERROR.
				return std::string();
			}
		case EString_ResourceImproveIncomes : // unit type's processing bonuses
			resource = s->D.Resource;
			if (resource != nullptr) {
				std::string improve_incomes;
				bool first = true;
				if (CPlayer::GetThisPlayer()->get_income((*resource)) > (*resource)->get_default_income()) {
					first = false;
					improve_incomes += (*resource)->get_name();
					improve_incomes += " Processing Bonus: +";
					improve_incomes += std::to_string(CPlayer::GetThisPlayer()->get_income((*resource)) - (*resource)->get_default_income());
					improve_incomes += "%";
				}
				for (const wyrmgus::resource *child_resource : (*resource)->ChildResources) {
					if (child_resource->get_index() == TradeCost || child_resource->Hidden) {
						continue;
					}

					if (CPlayer::GetThisPlayer()->get_income(child_resource) > child_resource->get_default_income()) {
						if (!first) {
							improve_incomes += "\n";
						} else {
							first = false;
						}
						improve_incomes += child_resource->get_name();
						improve_incomes += " Processing Bonus: +";
						improve_incomes += std::to_string(CPlayer::GetThisPlayer()->get_income(child_resource) - child_resource->get_default_income());
						improve_incomes += "%";
					}
				}
				return improve_incomes;
			} else { // ERROR.
				return std::string();
			}
		//Wyrmgus end
		case EString_If : // cond ? True : False;
			if (EvalNumber(s->D.If.Cond.get())) {
				return EvalString(s->D.If.BTrue.get());
			} else if (s->D.If.BFalse) {
				return EvalString(s->D.If.BFalse.get());
			} else {
				return std::string();
			}
		case EString_SubString : // substring(s, begin, end)
			if (s->D.SubString.String != nullptr
				&& !(tmp1 = EvalString(s->D.SubString.String.get())).empty()) {
				int begin;
				int end;

				begin = EvalNumber(s->D.SubString.Begin.get());
				if ((unsigned) begin > tmp1.size() && begin > 0) {
					return std::string();
				}
				res = tmp1.c_str() + begin;
				if (s->D.SubString.End) {
					end = EvalNumber(s->D.SubString.End.get());
				} else {
					end = -1;
				}
				if ((unsigned)end < res.size() && end >= 0) {
					res[end] = '\0';
				}
				return res;
			} else { // ERROR.
				return std::string();
			}
		case EString_Line : // line n of the string
			if (s->D.Line.String == nullptr || (tmp1 = EvalString(s->D.Line.String.get())).empty()) {
				return std::string(); // ERROR.
			} else {
				int line;
				int maxlen;
				wyrmgus::font *font;

				line = EvalNumber(s->D.Line.Line.get());
				if (line <= 0) {
					return std::string();
				}
				if (s->D.Line.MaxLen) {
					maxlen = EvalNumber(s->D.Line.MaxLen.get());
					maxlen = std::max(maxlen, 0);
				} else {
					maxlen = 0;
				}
				font = s->D.Line.Font;
				res = GetLineFont(line, tmp1, maxlen, font);
				return res;
			}
		case EString_PlayerName: // player name
			player_index = EvalNumber(s->D.PlayerName.get());
			try {
				return CPlayer::Players.at(player_index)->get_name();
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Error getting the player name for index " + std::to_string(player_index) + "."));
			}
		case EString_PlayerFullName: // player full name
			player_index = EvalNumber(s->D.PlayerName.get());
			try {
				return CPlayer::Players.at(player_index)->get_full_name();
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Error getting the full player name for index " + std::to_string(player_index) + "."));
			}
		case EString_TileTerrainFeatureName: {
			if (s->D.tile != nullptr) {
				const tile *tile = *s->D.tile;
				if (tile != nullptr && tile->get_terrain_feature() != nullptr) {
					const civilization *tile_owner_civilization = tile->get_owner() ? tile->get_owner()->get_civilization() : nullptr;
					return tile->get_terrain_feature()->get_cultural_name(tile_owner_civilization);
				}
			}

			return std::string();
		}
		case EString_TileWorldName: {
			if (s->D.tile != nullptr) {
				const tile *tile = *s->D.tile;
				if (tile != nullptr && tile->get_world() != nullptr) {
					return tile->get_world()->get_name();
				}
			}

			return std::string();
		}
	}

	return std::string();
}

/*............................................................................
..  Aliases
............................................................................*/

/**
**  Make alias for some unit type Variable function.
**
**  @param l  lua State.
**  @param s  FIXME: docu
**
**  @return   the lua table {"TypeVar", {Variable = arg1,
**                           Component = "Value" or arg2}
*/
static int AliasTypeVar(lua_State *l, const char *s)
{
	assert_throw(0 < lua_gettop(l) && lua_gettop(l) <= 3);
	int nargs = lua_gettop(l); // number of args in lua.
	lua_newtable(l);
	lua_pushnumber(l, 1);
	lua_pushstring(l, "TypeVar");
	lua_rawset(l, -3);
	lua_pushnumber(l, 2);
	lua_newtable(l);

	lua_pushstring(l, "Type");
	lua_pushstring(l, s);
	lua_rawset(l, -3);
	lua_pushstring(l, "Variable");
	lua_pushvalue(l, 1);
	lua_rawset(l, -3);
	lua_pushstring(l, "Component");
	if (nargs >= 2) {
		lua_pushvalue(l, 2);
	} else {
		lua_pushstring(l, "Value");
	}
	lua_rawset(l, -3);
	lua_pushstring(l, "Loc");
	if (nargs >= 3) {
		//  Warning: type is for unit->Stats->Var...
		//           and Initial is for unit->Type->Var... (no upgrade modification)
		const char *sloc[] = {"Unit", "Initial", "Type", nullptr};
		int i;
		const char *key;
		
		key = LuaToString(l, 3);
		for (i = 0; sloc[i] != nullptr; i++) {
			if (!strcmp(key, sloc[i])) {
				lua_pushnumber(l, i);
				break ;
			}
		}
		if (sloc[i] == nullptr) {
			LuaError(l, "Bad loc :'%s'" _C_ key);
		}
	} else {
		lua_pushnumber(l, 0);
	}
	lua_rawset(l, -3);

	lua_rawset(l, -3);
	return 1;
}

/**
**  Make alias for some unit Variable function.
**
**  @param l  lua State.
**  @param s  FIXME: docu
**
**  @return   the lua table {"UnitVar", {Unit = s, Variable = arg1,
**                           Component = "Value" or arg2, Loc = [012]}
*/
static int AliasUnitVar(lua_State *l, const char *s)
{
	int nargs; // number of args in lua.

	assert_throw(0 < lua_gettop(l) && lua_gettop(l) <= 3);
	nargs = lua_gettop(l);
	lua_newtable(l);
	lua_pushnumber(l, 1);
	lua_pushstring(l, "UnitVar");
	lua_rawset(l, -3);
	lua_pushnumber(l, 2);
	lua_newtable(l);

	lua_pushstring(l, "Unit");
	lua_pushstring(l, s);
	lua_rawset(l, -3);
	lua_pushstring(l, "Variable");
	lua_pushvalue(l, 1);
	lua_rawset(l, -3);
	lua_pushstring(l, "Component");
	if (nargs >= 2) {
		lua_pushvalue(l, 2);
	} else {
		lua_pushstring(l, "Value");
	}
	lua_rawset(l, -3);
	lua_pushstring(l, "Loc");
	if (nargs >= 3) {
		//  Warning: type is for unit->Stats->Var...
		//           and Initial is for unit->Type->Var... (no upgrade modification)
		const char *sloc[] = {"Unit", "Initial", "Type", nullptr};
		int i;
		const char *key;

		key = LuaToString(l, 3);
		for (i = 0; sloc[i] != nullptr; i++) {
			if (!strcmp(key, sloc[i])) {
				lua_pushnumber(l, i);
				break ;
			}
		}
		if (sloc[i] == nullptr) {
			LuaError(l, "Bad loc :'%s'" _C_ key);
		}
	} else {
		lua_pushnumber(l, 0);
	}
	lua_rawset(l, -3);

	lua_rawset(l, -3);
	return 1;
}

/**
**  Return equivalent lua table for .
**  {"Unit", {Unit = "Attacker", Variable = arg1, Component = "Value" or arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitAttackerVar(lua_State *l)
{
	if (lua_gettop(l) == 0 || lua_gettop(l) > 3) {
		LuaError(l, "Bad number of arg for AttackerVar()\n");
	}
	return AliasUnitVar(l, "Attacker");
}

/**
**  Return equivalent lua table for .
**  {"Unit", {Unit = "Defender", Variable = arg1, Component = "Value" or arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitDefenderVar(lua_State *l)
{
	if (lua_gettop(l) == 0 || lua_gettop(l) > 3) {
		LuaError(l, "Bad number of arg for DefenderVar()\n");
	}
	return AliasUnitVar(l, "Defender");
}

/**
**  Return equivalent lua table for .
**  {"Unit", {Unit = "Active", Variable = arg1, Component = "Value" or arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclActiveUnitVar(lua_State *l)
{
	if (lua_gettop(l) == 0 || lua_gettop(l) > 3) {
		LuaError(l, "Bad number of arg for ActiveUnitVar()\n");
	}
	return AliasUnitVar(l, "Active");
}

/**
**  Return equivalent lua table for .
**  {"Type", {Type = "Active", Variable = arg1, Component = "Value" or arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclActiveTypeVar(lua_State *l)
{
	if (lua_gettop(l) == 0 || lua_gettop(l) > 3) {
		LuaError(l, "Bad number of arg for ActiveTypeVar()\n");
	}
	return AliasTypeVar(l, "Type");
}

//Wyrmgus start
/**
**  Return equivalent lua table for .
**  {"Unit", {Unit = "Unit", Variable = arg1, Component = "Value" or arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitVar(lua_State *l)
{
	if (lua_gettop(l) == 0 || lua_gettop(l) > 3) {
		LuaError(l, "Bad number of arg for ActiveUnitVar()\n");
	}
	return AliasUnitVar(l, "Unit");
}
//Wyrmgus end

/**
**  Make alias for some function.
**
**  @param l  lua State.
**  @param s  FIXME: docu
**
**  @return the lua table {s, {arg1, arg2, ..., argn}} or {s, arg1}
*/
static int Alias(lua_State *l, const char *s)
{
	const int narg = lua_gettop(l);
	assert_throw(narg > 0);
	lua_newtable(l);
	lua_pushnumber(l, 1);
	lua_pushstring(l, s);
	lua_rawset(l, -3);
	lua_pushnumber(l, 2);
	if (narg > 1) {
		lua_newtable(l);
		for (int i = 1; i <= narg; i++) {
			lua_pushnumber(l, i);
			lua_pushvalue(l, i);
			lua_rawset(l, -3);
		}
	} else {
		lua_pushvalue(l, 1);
	}
	lua_rawset(l, -3);
	return 1;
}

/**
**  Return equivalent lua table for add.
**  {"Add", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclAdd(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Add");
}

/**
**  Return equivalent lua table for add.
**  {"Div", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclSub(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Sub");
}
/**
**  Return equivalent lua table for add.
**  {"Mul", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclMul(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Mul");
}
/**
**  Return equivalent lua table for add.
**  {"Div", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclDiv(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Div");
}
/**
**  Return equivalent lua table for add.
**  {"Min", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclMin(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Min");
}
/**
**  Return equivalent lua table for add.
**  {"Max", {arg1, arg2, argn}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclMax(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Max");
}
/**
**  Return equivalent lua table for add.
**  {"Rand", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclRand(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "Rand");
}
/**
**  Return equivalent lua table for GreaterThan.
**  {"GreaterThan", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclGreaterThan(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "GreaterThan");
}
/**
**  Return equivalent lua table for GreaterThanOrEq.
**  {"GreaterThanOrEq", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclGreaterThanOrEq(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "GreaterThanOrEq");
}
/**
**  Return equivalent lua table for LessThan.
**  {"LessThan", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclLessThan(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "LessThan");
}
/**
**  Return equivalent lua table for LessThanOrEq.
**  {"LessThanOrEq", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclLessThanOrEq(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "LessThanOrEq");
}
/**
**  Return equivalent lua table for Equal.
**  {"Equal", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclEqual(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "Equal");
}
/**
**  Return equivalent lua table for NotEqual.
**  {"NotEqual", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclNotEqual(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "NotEqual");
}



/**
**  Return equivalent lua table for Concat.
**  {"Concat", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclConcat(lua_State *l)
{
	if (lua_gettop(l) < 1) { // FIXME do extra job for 1.
		LuaError(l, "Bad number of arg for Concat()\n");
	}
	return Alias(l, "Concat");
}

/**
**  Return equivalent lua table for String.
**  {"String", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclString(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "String");
}
/**
**  Return equivalent lua table for InverseVideo.
**  {"InverseVideo", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclInverseVideo(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "InverseVideo");
}
/**
**  Return equivalent lua table for UnitName.
**  {"UnitName", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitName");
}
//Wyrmgus start
/**
**  Return equivalent lua table for UnitTypeName.
**  {"UnitTypeName", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitTypeName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitTypeName");
}

/**
**  Return equivalent lua table for UnitTrait.
**  {"UnitTrait", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitTrait(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitTrait");
}

/**
**  Return equivalent lua table for UnitSpell.
**  {"UnitSpell", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitSpell(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitSpell");
}

/**
**  Return equivalent lua table for UnitQuote.
**  {"UnitQuote", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitQuote(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitQuote");
}

/**
**  Return equivalent lua table for UnitSettlementName.
**  {"UnitSettlementName", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitSettlementName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitSettlementName");
}

static int CclUnitSiteName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitSiteName");
}

/**
**  Return equivalent lua table for UnitUniqueSet.
**  {"UnitUniqueSet", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitUniqueSet(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitUniqueSet");
}

/**
**  Return equivalent lua table for UnitUniqueSetItems.
**  {"UnitUniqueSetItems", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUnitUniqueSetItems(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "UnitUniqueSetItems");
}

/**
**  Return equivalent lua table for TypeName.
**  {"TypeName", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclTypeName(lua_State *l)
{
	return Alias(l, "TypeName");
}

/**
**  Return equivalent lua table for TypeIdent.
**  {"TypeIdent", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclTypeIdent(lua_State *l)
{
	return Alias(l, "TypeIdent");
}

/**
**  Return equivalent lua table for TypeClass.
**  {"TypeClass", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclTypeClass(lua_State *l)
{
	return Alias(l, "TypeClass");
}

/**
**  Return equivalent lua table for TypeDescription.
**  {"TypeDescription", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclTypeDescription(lua_State *l)
{
	return Alias(l, "TypeDescription");
}

/**
**  Return equivalent lua table for TypeQuote.
**  {"TypeQuote", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclTypeQuote(lua_State *l)
{
	return Alias(l, "TypeQuote");
}

static int CclTypeRequirementsString(lua_State *l)
{
	return Alias(l, "TypeRequirementsString");
}

static int CclTypeExperienceRequirementsString(lua_State *l)
{
	return Alias(l, "TypeExperienceRequirementsString");
}

static int CclTypeBuildingRulesString(lua_State *l)
{
	return Alias(l, "TypeBuildingRulesString");
}

static int CclTypeImproveIncomes(lua_State *l)
{
	return Alias(l, "TypeImproveIncomes");
}

static int CclTypeLuxuryDemand(lua_State *l)
{
	return Alias(l, "TypeLuxuryDemand");
}

static int CclTypeTrainQuantity(lua_State *l)
{
	return Alias(l, "TypeTrainQuantity");
}

/**
**  Return equivalent lua table for UpgradeCivilization.
**  {"UpgradeCivilization", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUpgradeCivilization(lua_State *l)
{
	return Alias(l, "UpgradeCivilization");
}

/**
**  Return equivalent lua table for UpgradeEffectsString.
**  {"UpgradeEffectsString", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUpgradeEffectsString(lua_State *l)
{
	return Alias(l, "UpgradeEffectsString");
}

/**
**  Return equivalent lua table for UpgradeRequirementsString.
**  {"UpgradeRequirementsString", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUpgradeRequirementsString(lua_State *l)
{
	return Alias(l, "UpgradeRequirementsString");
}

/**
**  Return equivalent lua table for UpgradeMaxLimit.
**  {"UpgradeMaxLimit", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclUpgradeMaxLimit(lua_State *l)
{
	return Alias(l, "UpgradeMaxLimit");
}

/**
**  Return equivalent lua table for FactionCivilization.
**  {"FactionCivilization", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclFactionCivilization(lua_State *l)
{
	return Alias(l, "FactionCivilization");
}

/**
**  Return equivalent lua table for FactionType.
**  {"FactionType", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclFactionType(lua_State *l)
{
	return Alias(l, "FactionType");
}

/**
**  Return equivalent lua table for FactionCoreSettlements.
**  {"FactionCoreSettlements", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclFactionCoreSettlements(lua_State *l)
{
	return Alias(l, "FactionCoreSettlements");
}

static int CclButtonPlayer(lua_State *l)
{
	return Alias(l, "ButtonPlayer");
}

/**
**  Return equivalent lua table for ResourceIdent.
**  {"ResourceIdent", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclResourceIdent(lua_State *l)
{
	return Alias(l, "ResourceIdent");
}

/**
**  Return equivalent lua table for ResourceName.
**  {"ResourceName", {}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclResourceName(lua_State *l)
{
	return Alias(l, "ResourceName");
}

static int CclResourceConversionRates(lua_State *l)
{
	return Alias(l, "ResourceConversionRates");
}

static int CclResourceImproveIncomes(lua_State *l)
{
	return Alias(l, "ResourceImproveIncomes");
}
//Wyrmgus end

static int CclTileTerrainFeatureName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "TileTerrainFeatureName");
}

static int CclTileWorldName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "TileWorldName");
}

/**
**  Return equivalent lua table for If.
**  {"If", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclIf(lua_State *l)
{
	if (lua_gettop(l) != 2 && lua_gettop(l) != 3) {
		LuaError(l, "Bad number of arg for If()\n");
	}
	return Alias(l, "If");
}

/**
**  Return equivalent lua table for SubString.
**  {"SubString", {arg1, arg2, arg3}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclSubString(lua_State *l)
{
	if (lua_gettop(l) != 2 && lua_gettop(l) != 3) {
		LuaError(l, "Bad number of arg for SubString()\n");
	}
	return Alias(l, "SubString");
}

/**
**  Return equivalent lua table for Line.
**  {"Line", {arg1, arg2[, arg3]}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclLine(lua_State *l)
{
	if (lua_gettop(l) < 2 || lua_gettop(l) > 4) {
		LuaError(l, "Bad number of arg for Line()\n");
	}
	return Alias(l, "Line");
}

/**
**  Return equivalent lua table for Line.
**  {"Line", "arg1"}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclGameInfo(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "GameInfo");
}

/**
**  Return equivalent lua table for VideoTextLength.
**  {"VideoTextLength", {Text = arg1, Font = arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclVideoTextLength(lua_State *l)
{
	LuaCheckArgs(l, 2);
	lua_newtable(l);
	lua_pushnumber(l, 1);
	lua_pushstring(l, "VideoTextLength");
	lua_rawset(l, -3);
	lua_pushnumber(l, 2);

	lua_newtable(l);
	lua_pushstring(l, "Font");
	lua_pushvalue(l, 1);
	lua_rawset(l, -3);
	lua_pushstring(l, "Text");
	lua_pushvalue(l, 2);
	lua_rawset(l, -3);

	lua_rawset(l, -3);
	return 1;
}

/**
**  Return equivalent lua table for StringFind.
**  {"StringFind", {arg1, arg2}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclStringFind(lua_State *l)
{
	LuaCheckArgs(l, 2);
	return Alias(l, "StringFind");
}

/**
**  Return equivalent lua table for NumIf.
**  {"NumIf", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclNumIf(lua_State *l)
{
	if (lua_gettop(l) != 2 && lua_gettop(l) != 3) {
		LuaError(l, "Bad number of arg for NumIf()\n");
	}
	return Alias(l, "NumIf");
}

/**
**  Return equivalent lua table for PlayerData.
**  {"PlayerData", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclPlayerData(lua_State *l)
{
	if (lua_gettop(l) != 2 && lua_gettop(l) != 3) {
		LuaError(l, "Bad number of arg for PlayerData()\n");
	}
	return Alias(l, "PlayerData");
}

/**
**  Return equivalent lua table for PlayerName.
**  {"PlayerName", {arg1}}
**
**  @param l  Lua state.
**
**  @return   equivalent lua table.
*/
static int CclPlayerName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "PlayerName");
}

static int CclPlayerFullName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	return Alias(l, "PlayerFullName");
}

static void AliasRegister()
{
	// Number.
	lua_register(Lua, "Add", CclAdd);
	lua_register(Lua, "Sub", CclSub);
	lua_register(Lua, "Mul", CclMul);
	lua_register(Lua, "Div", CclDiv);
	lua_register(Lua, "Min", CclMin);
	lua_register(Lua, "Max", CclMax);
	lua_register(Lua, "Rand", CclRand);

	lua_register(Lua, "GreaterThan", CclGreaterThan);
	lua_register(Lua, "LessThan", CclLessThan);
	lua_register(Lua, "Equal", CclEqual);
	lua_register(Lua, "GreaterThanOrEq", CclGreaterThanOrEq);
	lua_register(Lua, "LessThanOrEq", CclLessThanOrEq);
	lua_register(Lua, "NotEqual", CclNotEqual);
	lua_register(Lua, "VideoTextLength", CclVideoTextLength);
	lua_register(Lua, "StringFind", CclStringFind);


	// Unit
	lua_register(Lua, "AttackerVar", CclUnitAttackerVar);
	lua_register(Lua, "DefenderVar", CclUnitDefenderVar);
	lua_register(Lua, "ActiveUnitVar", CclActiveUnitVar);
	//Wyrmgus start
	lua_register(Lua, "UnitVar", CclUnitVar);
	//Wyrmgus end

	// Type
	lua_register(Lua, "TypeVar", CclActiveTypeVar);

	// String.
	lua_register(Lua, "Concat", CclConcat);
	lua_register(Lua, "String", CclString);
	lua_register(Lua, "InverseVideo", CclInverseVideo);
	lua_register(Lua, "UnitName", CclUnitName);
	//Wyrmgus start
	lua_register(Lua, "UnitTypeName", CclUnitTypeName);
	lua_register(Lua, "UnitTrait", CclUnitTrait);
	lua_register(Lua, "UnitSpell", CclUnitSpell);
	lua_register(Lua, "UnitQuote", CclUnitQuote);
	lua_register(Lua, "UnitSettlementName", CclUnitSettlementName);
	lua_register(Lua, "UnitSiteName", CclUnitSiteName);
	lua_register(Lua, "UnitUniqueSet", CclUnitUniqueSet);
	lua_register(Lua, "UnitUniqueSetItems", CclUnitUniqueSetItems);
	lua_register(Lua, "TypeName", CclTypeName);
	lua_register(Lua, "TypeIdent", CclTypeIdent);
	lua_register(Lua, "TypeClass", CclTypeClass);
	lua_register(Lua, "TypeDescription", CclTypeDescription);
	lua_register(Lua, "TypeQuote", CclTypeQuote);
	lua_register(Lua, "TypeRequirementsString", CclTypeRequirementsString);
	lua_register(Lua, "TypeExperienceRequirementsString", CclTypeExperienceRequirementsString);
	lua_register(Lua, "TypeBuildingRulesString", CclTypeBuildingRulesString);
	lua_register(Lua, "TypeImproveIncomes", CclTypeImproveIncomes);
	lua_register(Lua, "TypeLuxuryDemand", CclTypeLuxuryDemand);
	lua_register(Lua, "TypeTrainQuantity", CclTypeTrainQuantity);
	lua_register(Lua, "UpgradeCivilization", CclUpgradeCivilization);
	lua_register(Lua, "UpgradeEffectsString", CclUpgradeEffectsString);
	lua_register(Lua, "UpgradeRequirementsString", CclUpgradeRequirementsString);
	lua_register(Lua, "UpgradeMaxLimit", CclUpgradeMaxLimit);
	lua_register(Lua, "FactionCivilization", CclFactionCivilization);
	lua_register(Lua, "FactionType", CclFactionType);
	lua_register(Lua, "FactionCoreSettlements", CclFactionCoreSettlements);
	lua_register(Lua, "ButtonPlayer", CclButtonPlayer);
	lua_register(Lua, "ResourceIdent", CclResourceIdent);
	lua_register(Lua, "ResourceName", CclResourceName);
	lua_register(Lua, "ResourceConversionRates", CclResourceConversionRates);
	lua_register(Lua, "ResourceImproveIncomes", CclResourceImproveIncomes);
	//Wyrmgus end
	lua_register(Lua, "TileTerrainFeatureName", CclTileTerrainFeatureName);
	lua_register(Lua, "TileWorldName", CclTileWorldName);
	lua_register(Lua, "SubString", CclSubString);
	lua_register(Lua, "Line", CclLine);
	lua_register(Lua, "GameInfo", CclGameInfo);
	lua_register(Lua, "PlayerName", CclPlayerName);
	lua_register(Lua, "PlayerFullName", CclPlayerFullName);
	lua_register(Lua, "PlayerData", CclPlayerData);

	lua_register(Lua, "If", CclIf);
	lua_register(Lua, "NumIf", CclNumIf);
}

/*............................................................................
..  Config
............................................................................*/

/**
**  Return the stratagus library path.
**
**  @param l  Lua state.
**
**  @return   Current libray path.
*/
static int CclStratagusLibraryPath(lua_State *l)
{
	lua_pushstring(l, database::get()->get_root_path().string().c_str());
	return 1;
}

/**
**  Return a table with the filtered items found in the subdirectory.
*/
static int CclFilteredListDirectory(lua_State *l, int type, int mask, int sortmode = 0)
{
	const int args = lua_gettop(l);
	if (args < 1 || args > 2) {
		LuaError(l, "incorrect argument");
	}
	const char *userdir = lua_tostring(l, 1);
	const int rel = args > 1 ? lua_toboolean(l, 2) : 0;
	int n = strlen(userdir);

	int pathtype = 0; // path relative to stratagus dir
	if (n > 0 && *userdir == '~') {
		// path relative to user preferences directory
		pathtype = 1;
	}

	// security: disallow all special characters
	//Wyrmgus start
//	if (strpbrk(userdir, ":*?\"<>|") != 0 || strstr(userdir, "..") != 0) {
	if ((strpbrk(userdir, ":*?\"<>|") != 0 || strstr(userdir, "..") != 0) && (strstr(userdir, "workshop") == 0 || strstr(userdir, "content") == 0 || strstr(userdir, "370070") == 0) && strstr(userdir, get_user_maps_path().c_str()) == 0) { //special case for Wyrmsun's workshop folder
	//Wyrmgus end
		LuaError(l, "Forbidden directory");
	}
	char directory[PATH_MAX];

	if (pathtype == 1) {
		++userdir;
		std::string dir(parameters::get()->GetUserDirectory());
		if (!GameName.empty()) {
			dir += "/";
			dir += GameName;
		}
		snprintf(directory, sizeof(directory), "%s/%s", dir.c_str(), userdir);
	} else if (rel) {
		std::string dir = LibraryFileName(userdir);
		snprintf(directory, sizeof(directory), "%s", dir.c_str());
		lua_pop(l, 1);
	} else {
		#ifndef __MORPHOS__
		snprintf(directory, sizeof(directory), "%s/%s", wyrmgus::database::get()->get_root_path().string().c_str(), userdir);
		#else
		snprintf(directory, sizeof(directory), "%s",  userdir);
		#endif
	}
	lua_pop(l, 1);
	lua_newtable(l);
	const std::vector<FileList> flp = ReadDataDirectory(directory, sortmode);
	int j = 0;
	for (size_t i = 0; i < flp.size(); ++i) {
		if ((flp[i].type & mask) == type) {
			lua_pushnumber(l, j + 1);
			lua_pushstring(l, flp[i].name.c_str());
			lua_settable(l, 1);
			++j;
		}
	}
	return 1;
}

/**
**  Return a table with the files or directories found in the subdirectory.
*/
static int CclListDirectory(lua_State *l)
{
	return CclFilteredListDirectory(l, 0, 0);
}

/**
**  Return a table with the files found in the subdirectory.
*/
static int CclListFilesInDirectory(lua_State *l)
{
	return CclFilteredListDirectory(l, 0x1, 0x1);
}

/**
**  Return a table with the files found in the subdirectory, ordered by modified time.
*/
static int CclListFilesInDirectorySortByTime(lua_State *l)
{
	return CclFilteredListDirectory(l, 0x1, 0x1, 1);
}

/**
**  Return a table with the files found in the subdirectory.
*/
static int CclListDirsInDirectory(lua_State *l)
{
	return CclFilteredListDirectory(l, 0x0, 0x1);
}

/**
**  Set damage computation method.
**
**  @param l  Lua state.
*/
static int CclSetDamageFormula(lua_State *l)
{
	assert_throw(l != nullptr);
	Damage = CclParseNumberDesc(l);
	return 0;
}

/**
**  Print debug message with info about current script name, line number and function.
**
**  @see DebugPrint
**
**  @param l  Lua state.
*/
static int CclDebugPrint(lua_State *l)
{
	LuaCheckArgs(l, 1);

#ifdef DEBUG
	lua_Debug ar;
	lua_getstack(l, 1, &ar);
	lua_getinfo(l, "nSl", &ar);
	fprintf(stdout, "%s:%d: %s: %s", ar.source, ar.currentline, ar.what, LuaToString(l, 1));
#endif

	return 1;
}

//Wyrmgus start
/**
**  Print message to sdout.txt
**
**  @param l  Lua state.
*/
static int CclStdOutPrint(lua_State *l)
{
	LuaCheckArgs(l, 1);

	fprintf(stdout, "%s", LuaToString(l, 1));

	return 1;
}

/**
**  Get a date from lua state
**
**  @param l  Lua state.
**  @param d  pointer to output date.
*/
void CclGetDate(lua_State *l, CDate *d, const int offset)
{
	if (!lua_istable(l, offset)) {
		d->Year = LuaToNumber(l, offset);
	} else {
		d->Year = LuaToNumber(l, offset, 1);
		d->Month = LuaToNumber(l, offset, 2);
		d->Day = LuaToNumber(l, offset, 3);
	}
}
//Wyrmgus end

/*............................................................................
..  Commands
............................................................................*/

/**
**  Send command to ccl.
**
**  @param command  Zero terminated command string.
*/
int CclCommand(const std::string &command, bool exitOnError)
{
	try {
		const int status = luaL_loadbuffer(Lua, command.c_str(), command.size(), command.c_str());

		if (!status) {
			LuaCall(0, 1, exitOnError);
		} else {
			report(status, exitOnError);
		}

		return status;
	} catch (...) {
		std::throw_with_nested("Error calling Lua command: \"" + command + "\"");
	}
}

/*............................................................................
..  Setup
............................................................................*/

extern int tolua_stratagus_open(lua_State *tolua_S);

/**
**  Initialize Lua
*/
void InitLua()
{
	// For security we don't load all libs
	static const luaL_Reg lualibs[] = {
		{"", luaopen_base},
		//{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
		//{LUA_IOLIBNAME, luaopen_io},
		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_DBLIBNAME, luaopen_debug},
		{nullptr, nullptr}
	};

	Lua = luaL_newstate();

	for (const luaL_Reg *lib = lualibs; lib->func; ++lib) {
		lua_pushcfunction(Lua, lib->func);
		lua_pushstring(Lua, lib->name);
		lua_call(Lua, 1, 0);
	}
	tolua_stratagus_open(Lua);
	lua_settop(Lua, 0);  // discard any results
}

/*
static char *LuaEscape(const char *str)
{
	const unsigned char *src;
	char *dst;
	char *escapedString;
	int size = 0;

	for (src = (const unsigned char *)str; *src; ++src) {
		if (*src == '"' || *src == '\\') { // " -> \"
			size += 2;
		} else if (*src < 32 || *src > 127) { // 0xA -> \010
			size += 4;
		} else {
			++size;
		}
	}

	escapedString = new char[size + 1];
	for (src = (const unsigned char *)str, dst = escapedString; *src; ++src) {
		if (*src == '"' || *src == '\\') { // " -> \"
			*dst++ = '\\';
			*dst++ = *src;
		} else if (*src < 32 || *src > 127) { // 0xA -> \010
			*dst++ = '\\';
			snprintf(dst, (size + 1) - (dst - escapedString), "%03d", *src);
			dst += 3;
		} else {
			*dst++ = *src;
		}
	}
	*dst = '\0';

	return escapedString;
}
*/

static std::string ConcatTableString(const std::vector<std::string> &blockTableNames)
{
	if (blockTableNames.empty()) {
		return "";
	}
	std::string res(blockTableNames[0]);
	for (size_t i = 1; i != blockTableNames.size(); ++i) {
		if (blockTableNames[i][0] != '[') {
			res += ".";
		}
		res += blockTableNames[i];
	}
	return res;
}

static bool IsAValidTableName(const std::string &key)
{
	return key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789") == std::string::npos;
}

static bool ShouldGlobalTableBeSaved(const std::string &key)
{
	if (IsAValidTableName(key) == false) {
		return false;
	}
	const std::string forbiddenNames[] = {
		"assert", "gcinfo", "getfenv", "unpack", "tostring", "tonumber",
		"setmetatable", "require", "pcall", "rawequal", "collectgarbage", "type",
		"getmetatable", "math", "next", "print", "xpcall", "rawset", "setfenv",
		"rawget", "newproxy", "ipairs", "loadstring", "dofile", "_TRACEBACK",
		"_VERSION", "pairs", "__pow", "error", "loadfile", "arg",
		"_LOADED", "loadlib", "string", "os", "io", "debug",
		"coroutine", "Icons", "Upgrades", "Fonts", "FontColors", "expansion",
		"CMap", "CPlayer", "Graphics", "Vec2i", "_triggers_"
	}; // other string to protected ?
	const int size = sizeof(forbiddenNames) / sizeof(*forbiddenNames);

	return std::find(forbiddenNames, forbiddenNames + size, key) == forbiddenNames + size;
}

static bool ShouldLocalTableBeSaved(const std::string &key)
{
	if (IsAValidTableName(key) == false) {
		return false;
	}
	const std::string forbiddenNames[] = { "tolua_ubox" }; // other string to protected ?
	const int size = sizeof(forbiddenNames) / sizeof(*forbiddenNames);

	return std::find(forbiddenNames, forbiddenNames + size, key) == forbiddenNames + size;
}

static bool LuaValueToString(lua_State *l, std::string &value)
{
	const int type_value = lua_type(l, -1);

	switch (type_value) {
		case LUA_TNIL:
			value = "nil";
			return true;
		case LUA_TNUMBER:
			value = lua_tostring(l, -1); // let lua do the conversion
			return true;
		case LUA_TBOOLEAN: {
			const bool b = (lua_toboolean(l, -1) != 0);
			value = b ? "true" : "false";
			return true;
		}
		case LUA_TSTRING: {
			const std::string s = lua_tostring(l, -1);
			value = "";
			
			if ((s.find('\n') != std::string::npos)) {
				value = std::string("[[") + s + "]]";
			} else {
				for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
					if (*it == '\"') {
						value.push_back('\\');
					}
					value.push_back(*it);
				}
				value = std::string("\"") + value + "\"";
			}
			return true;
		}
		case LUA_TTABLE:
			value = "";
			return false;
		case LUA_TFUNCTION:
			// Could be done with string.dump(function)
			// and debug.getinfo(function).name (could be nil for anonymous function)
			// But not useful yet.
			value = "";
			return false;
		case LUA_TUSERDATA:
		case LUA_TTHREAD:
		case LUA_TLIGHTUSERDATA:
		case LUA_TNONE:
		default : // no other cases
			value = "";
			return false;
	}
}

/**
**  For saving lua state (table, number, string, bool, not function).
**
**  @param l        lua_State to save.
**  @param is_root  true for the main call, 0 for recursif call.
**
**  @return  "" if nothing could be saved.
**           else a string that could be executed in lua to restore lua state
**  @todo    do the output prettier (adjust indentation, newline)
*/
static std::string SaveGlobal(lua_State *l, bool is_root, std::vector<std::string> &blockTableNames)
{
	//assert_throw(!is_root || !lua_gettop(l));
	if (is_root) {
		lua_getglobal(l, "_G");// global table in lua.
	}
	std::string res;
	const std::string tablesName = ConcatTableString(blockTableNames);

	if (blockTableNames.empty() == false) {
		res = "if (" + tablesName + " == nil) then " + tablesName + " = {} end\n";
	}
	assert_throw(lua_istable(l, -1));

	lua_pushnil(l);
	while (lua_next(l, -2)) {
		const int type_key = lua_type(l, -2);
		std::string key = (type_key == LUA_TSTRING) ? lua_tostring(l, -2) : "";
		if ((key == "_G")
			|| (is_root && ShouldGlobalTableBeSaved(key) == false)
			|| (!is_root && ShouldLocalTableBeSaved(key) == false)) {
			lua_pop(l, 1); // pop the value
			continue;
		}
		std::string lhsLine;
		if (tablesName.empty() == false) {
			if (type_key == LUA_TSTRING) {
				lhsLine = tablesName + "." + key;
			} else if (type_key == LUA_TNUMBER) {
				lua_pushvalue(l, -2);
				lhsLine = tablesName + "[" + lua_tostring(l, -1) + "]";
				lua_pop(l, 1);
			}
		} else {
			lhsLine = key;
		}

		std::string value;
		const bool b = LuaValueToString(l, value);
		if (b) {
			res += lhsLine + " = " + value + "\n";
		} else {
			const int type_value = lua_type(l, -1);
			if (type_value == LUA_TTABLE) {
				if (key == "") {
					lua_pushvalue(l, -2);
					key = key + "[" + lua_tostring(l, -1) + "]";
					lua_pop(l, 1);
				}
				lua_pushvalue(l, -1);
				//res += "if (" + lhsLine + " == nil) then " + lhsLine + " = {} end\n";
				blockTableNames.push_back(key);
				res += SaveGlobal(l, false, blockTableNames);
				blockTableNames.pop_back();
			}
		}
		lua_pop(l, 1); /* pop the value */
	}
	lua_pop(l, 1); // pop the table
	//assert_throw(!is_root || !lua_gettop(l));
	return res;
}

std::string SaveGlobal(lua_State *l)
{
	std::vector<std::string> blockTableNames;

	return SaveGlobal(l, true, blockTableNames);
}

/**
**  Save user preferences
*/
void SavePreferences()
{
	std::vector<std::string> blockTableNames;

	if (!GameName.empty()) {
		lua_getglobal(Lua, GameName.c_str());
		if (lua_type(Lua, -1) == LUA_TTABLE) {
			blockTableNames.push_back(GameName);
			lua_pushstring(Lua, "preferences");
			lua_gettable(Lua, -2);
		} else {
			lua_getglobal(Lua, "preferences");
		}
	} else {
		lua_getglobal(Lua, "preferences");
	}
	blockTableNames.push_back("preferences");
	if (lua_type(Lua, -1) == LUA_TTABLE) {
		std::string path = parameters::get()->GetUserDirectory();

		if (!GameName.empty()) {
			path += "/";
			path += GameName;
		}
		path += "/preferences.lua";

		FILE *fd = fopen(path.c_str(), "w");
		if (!fd) {
			log::log_error("Cannot open file \"" + path + "\" for writing.");
			return;
		}

		std::string s = SaveGlobal(Lua, false, blockTableNames);
		if (!GameName.empty()) {
			fprintf(fd, "if (%s == nil) then %s = {} end\n", GameName.c_str(), GameName.c_str());
		}
		fprintf(fd, "%s\n", s.c_str());
		fclose(fd);
	}
}

//Wyrmgus start
void DeleteModFaction(const std::string &faction_name)
{
	wyrmgus::faction::remove(faction_name);
}

void DeleteModUnitType(const std::string &unit_type_ident)
{
	wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
	
	if (CEditor::get()->is_running()) {
		std::vector<CUnit *> units_to_remove;

		for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
			if (unit->Type == unit_type) {
				units_to_remove.push_back(unit);
			}
		}
		
		for (size_t i = 0; i < units_to_remove.size(); ++i) {
			EditorActionRemoveUnit(*units_to_remove[i], false);
		}
		vector::remove(CEditor::get()->UnitTypes, unit_type->Ident);
		RecalculateShownUnits();
	}
	for (wyrmgus::civilization *civilization : wyrmgus::civilization::get_all()) {
		civilization->remove_class_unit_type(unit_type);
	}
	for (wyrmgus::faction *faction : wyrmgus::faction::get_all()) {
		faction->remove_class_unit_type(unit_type);
	}
	for (wyrmgus::unit_type *other_unit_type : wyrmgus::unit_type::get_all()) { //remove this unit from the "Trains", "TrainedBy", "Drops" and "AiDrops" vectors of other unit types
		if (std::find(other_unit_type->Trains.begin(), other_unit_type->Trains.end(), unit_type) != other_unit_type->Trains.end()) {
			other_unit_type->Trains.erase(std::remove(other_unit_type->Trains.begin(), other_unit_type->Trains.end(), unit_type), other_unit_type->Trains.end());
		}
		if (std::find(other_unit_type->TrainedBy.begin(), other_unit_type->TrainedBy.end(), unit_type) != other_unit_type->TrainedBy.end()) {
			other_unit_type->TrainedBy.erase(std::remove(other_unit_type->TrainedBy.begin(), other_unit_type->TrainedBy.end(), unit_type), other_unit_type->TrainedBy.end());
		}
		if (std::find(other_unit_type->Drops.begin(), other_unit_type->Drops.end(), unit_type) != other_unit_type->Drops.end()) {
			other_unit_type->Drops.erase(std::remove(other_unit_type->Drops.begin(), other_unit_type->Drops.end(), unit_type), other_unit_type->Drops.end());
		}
		if (std::find(other_unit_type->AiDrops.begin(), other_unit_type->AiDrops.end(), unit_type) != other_unit_type->AiDrops.end()) {
			other_unit_type->AiDrops.erase(std::remove(other_unit_type->AiDrops.begin(), other_unit_type->AiDrops.end(), unit_type), other_unit_type->AiDrops.end());
		}
	}
	const int buttons_size = wyrmgus::button::get_all().size();
	for (int j = (buttons_size - 1); j >= 0; --j) {
		if (wyrmgus::button::get_all()[j]->UnitMask.find(unit_type->Ident) != std::string::npos) { //remove this unit from the "ForUnit" array of buttons
			wyrmgus::button::get_all()[j]->UnitMask = FindAndReplaceString(wyrmgus::button::get_all()[j]->UnitMask, unit_type->Ident + ",", "");
		}
		if (wyrmgus::button::get_all()[j]->Value == unit_type->Slot && wyrmgus::button::get_all()[j]->ValueStr == unit_type->Ident) {
			wyrmgus::button::remove(wyrmgus::button::get_all()[j]);
		}
	}
	wyrmgus::unit_type::remove(unit_type);
}

void DisableMod(const std::string &mod_file)
{
	const int unit_types_size = wyrmgus::unit_type::get_all().size();
	for (int i = (unit_types_size - 1); i >= 0; --i) {
		
		if (wyrmgus::unit_type::get_all()[i]->Mod == mod_file) {
			DeleteModUnitType(wyrmgus::unit_type::get_all()[i]->Ident);
		}
	}
		
	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->ModTrains.find(mod_file) != unit_type->ModTrains.end()) {
			unit_type->ModTrains.erase(mod_file);
			unit_type->RemoveButtons(ButtonCmd::None, mod_file);
		}
		if (unit_type->ModTrainedBy.find(mod_file) != unit_type->ModTrainedBy.end()) {
			unit_type->ModTrainedBy.erase(mod_file);
			unit_type->RemoveButtons(ButtonCmd::None, mod_file);
		}
		if (unit_type->ModAiDrops.find(mod_file) != unit_type->ModAiDrops.end()) {
			unit_type->ModAiDrops.erase(mod_file);
		}
		if (unit_type->ModDefaultStats.find(mod_file) != unit_type->ModDefaultStats.end()) {
			unit_type->ModDefaultStats.erase(mod_file);
		}
	}
	
	std::vector<wyrmgus::faction *> factions_to_remove;
	for (wyrmgus::faction *faction : wyrmgus::faction::get_all()) {
		if (faction->Mod == mod_file) {
			factions_to_remove.push_back(faction);
		}
	}

	for (wyrmgus::faction *faction : factions_to_remove) {
		wyrmgus::faction::remove(faction);
	}
}

void SetDLCFileEquivalency(const std::string dlc_file, const std::string replacement_file)
{
	if (!std::filesystem::exists(LibraryFileName(replacement_file.c_str()))) {
		throw std::runtime_error("DLC replacement file \"" + replacement_file + "\" does not exist.");
	}

	DLCFileEquivalency[dlc_file] = replacement_file;
}
//Wyrmgus end

/**
**  Load stratagus config file.
*/
void LoadCcl(const std::string &filename, const std::string &luaArgStr)
{
	//  Load and evaluate configuration file
	CclInConfigFile = 1;
	const std::string name = LibraryFileName(filename.c_str());
	if (CanAccessFile(name.c_str()) == 0) {
		throw std::runtime_error("Maybe you need to specify another gamepath with '-d /path/to/datadir'?");
	}

	ShowLoadProgress(_("Loading Script \"%s\"..."), name.c_str());
	LuaLoadFile(name, luaArgStr);
	CclInConfigFile = 0;
	LuaGarbageCollect();
}

void ScriptRegister()
{
	AliasRegister();

	lua_register(Lua, "LibraryPath", CclStratagusLibraryPath);
	lua_register(Lua, "ListDirectory", CclListDirectory);
	lua_register(Lua, "ListFilesInDirectory", CclListFilesInDirectory);
	lua_register(Lua, "ListFilesInDirectorySortByTime", CclListFilesInDirectorySortByTime);
	lua_register(Lua, "ListDirsInDirectory", CclListDirsInDirectory);

	lua_register(Lua, "SetDamageFormula", CclSetDamageFormula);

	lua_register(Lua, "SavePreferences", CclSavePreferences);
	lua_register(Lua, "Load", CclLoad);
	lua_register(Lua, "LoadConfigFile", CclLoadConfigFile);
	lua_register(Lua, "LoadBuffer", CclLoadBuffer);

	lua_register(Lua, "DebugPrint", CclDebugPrint);
	//Wyrmgus start
	lua_register(Lua, "StdOutPrint", CclStdOutPrint);
	//Wyrmgus end
}
