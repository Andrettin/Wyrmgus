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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon, Joris Dauphin and Andrettin
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

#include <csignal>

#include "stratagus.h"

#include "script.h"

//Wyrmgus start
#include "actions.h"
//Wyrmgus end
#include "animation/animation_setplayervar.h"
#include "civilization.h"
#include "config.h"
//Wyrmgus start
#include "editor.h"
//Wyrmgus end
#include "faction.h"
#include "font.h"
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iocompat.h"
#include "iolib.h"
//Wyrmgus start
#include "item.h"
//Wyrmgus end
#include "map/map.h"
#include "map/site.h"
#include "parameters.h"
//Wyrmgus start
#include "player.h"
#include "spells.h"
//Wyrmgus end
#include "time/timeline.h"
#include "translate.h"
#include "trigger.h"
#include "ui/button_action.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
//Wyrmgus start
#include "unit/unit_manager.h" //for checking units of a custom unit type and deleting them if the unit type has been removed
#include "unit/unit_type.h"
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

lua_State *Lua;                       /// Structure to work with lua files.

int CclInConfigFile;                  /// True while config file parsing

NumberDesc *Damage;                   /// Damage calculation for missile.

static int NumberCounter = 0; /// Counter for lua function.
static int StringCounter = 0; /// Counter for lua function.

/// Useful for getComponent.
enum UStrIntType {
	USTRINT_STR, USTRINT_INT
};
struct UStrInt {
	union {const char *s; int i;};
	UStrIntType type;
};

/// Get component for unit variable.
extern UStrInt GetComponent(const CUnit &unit, int index, EnumVariable e, int t);
/// Get component for unit type variable.
extern UStrInt GetComponent(const CUnitType &type, int index, EnumVariable e, int t);

//Wyrmgus start
std::map<std::string, std::string> DLCFileEquivalency;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
		const char *msg = lua_tostring(Lua, -1);
		if (msg == nullptr) {
			msg = "(error with no message)";
		}
		fprintf(stderr, "%s\n", msg);
		if (exitOnError) {
			::exit(1);
		}
		lua_pop(Lua, 1);
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
		DebugPrint("Can't open file '%s\n" _C_ file.c_str());
		fprintf(stderr, "Can't open file '%s': %s\n", file.c_str(), strerror(errno));
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

// ////////////////////

/**
**  Parse binary operation with number.
**
**  @param l       lua state.
**  @param binop   Where to stock info (must be malloced)
*/
static void ParseBinOp(lua_State *l, BinOp *binop)
{
	Assert(l);
	Assert(binop);
	Assert(lua_istable(l, -1));
	Assert(lua_rawlen(l, -1) == 2);

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

	Assert(l);
	Assert(s);
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
	Assert(res); // Must check for error.
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
static const CUnitType **Str2TypeRef(lua_State *l, const char *s)
{
	const CUnitType **res = nullptr; // Result.

	Assert(l);
	if (!strcmp(s, "Type")) {
		res = &TriggerData.Type;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	Assert(res); // Must check for error.
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
static CUpgrade **Str2UpgradeRef(lua_State *l, const char *s)
{
	CUpgrade **res = nullptr; // Result.

	Assert(l);
	if (!strcmp(s, "Upgrade")) {
		res = &TriggerData.Upgrade;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	Assert(res); // Must check for error.
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
static int **Str2ResourceRef(lua_State *l, const char *s)
{
	int **res = nullptr; // Result.

	Assert(l);
	if (!strcmp(s, "Resource")) {
		res = &TriggerData.Resource;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	Assert(res); // Must check for error.
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
static stratagus::faction **Str2FactionRef(lua_State *l, const char *s)
{
	stratagus::faction **res = nullptr; // Result.

	Assert(l);
	if (!strcmp(s, "Faction")) {
		res = &TriggerData.Faction;
	} else {
		LuaError(l, "Invalid type reference '%s'\n" _C_ s);
	}
	Assert(res); // Must check for error.
	return res;
}
//Wyrmgus end

/**
**  Return unit referernce definition.
**
**  @param l  lua state.
**
**  @return   unit referernce definition.
*/
UnitDesc *CclParseUnitDesc(lua_State *l)
{
	UnitDesc *res;  // Result

	res = new UnitDesc;
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
const CUnitType **CclParseTypeDesc(lua_State *l)
{
	const CUnitType **res = nullptr;

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
CUpgrade **CclParseUpgradeDesc(lua_State *l)
{
	CUpgrade **res = nullptr;

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
int **CclParseResourceDesc(lua_State *l)
{
	int **res = nullptr;

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
stratagus::faction **CclParseFactionDesc(lua_State *l)
{
	stratagus::faction **res = nullptr;

	if (lua_isstring(l, -1)) {
		res = Str2FactionRef(l, LuaToString(l, -1));
		lua_pop(l, 1);
	} else {
		LuaError(l, "Parse Error in ParseFaction\n");
	}
	return res;
}
//Wyrmgus end

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
**  Return number.
**
**  @param l  lua state.
**
**  @return   number.
*/
NumberDesc *CclParseNumberDesc(lua_State *l)
{
	NumberDesc *res = new NumberDesc;

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
			Assert(lua_istable(l, -1));

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
					res->D.UnitStat.Component = Str2EnumVariable(l, LuaToString(l, -1));
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
			Assert(lua_istable(l, -1));

			res->e = ENumber_TypeStat;
			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				key = LuaToString(l, -2);
				if (!strcmp(key, "Type")) {
					res->D.TypeStat.Type = CclParseTypeDesc(l);
					lua_pushnil(l);
				} else if (!strcmp(key, "Component")) {
					res->D.TypeStat.Component = Str2EnumVariable(l, LuaToString(l, -1));
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
			Assert(lua_istable(l, -1));
			res->e = ENumber_VideoTextLength;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				key = LuaToString(l, -2);
				if (!strcmp(key, "Text")) {
					res->D.VideoTextLength.String = CclParseStringDesc(l);
					lua_pushnil(l);
				} else if (!strcmp(key, "Font")) {
					res->D.VideoTextLength.Font = CFont::Get(LuaToString(l, -1));
					if (!res->D.VideoTextLength.Font) {
						LuaError(l, "Bad Font name :'%s'" _C_ LuaToString(l, -1));
					}
				} else {
					LuaError(l, "Bad param %s for VideoTextLength" _C_ key);
				}
			}
			lua_pop(l, 1); // pop the table.
		} else if (!strcmp(key, "StringFind")) {
			Assert(lua_istable(l, -1));
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
		} else {
			lua_pop(l, 1);
			LuaError(l, "unknow condition '%s'" _C_ key);
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
StringDesc *CclParseStringDesc(lua_State *l)
{
	StringDesc *res = new StringDesc;

	if (lua_isstring(l, -1)) {
		res->e = EString_Dir;
		res->D.Val = new_strdup(LuaToString(l, -1));
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
			int i; // iterator.

			res->e = EString_Concat;
			res->D.Concat.n = lua_rawlen(l, -1);
			if (res->D.Concat.n < 1) {
				LuaError(l, "Bad number of args in Concat\n");
			}
			res->D.Concat.Strings = new StringDesc *[res->D.Concat.n];
			for (i = 0; i < res->D.Concat.n; ++i) {
				lua_rawgeti(l, -1, 1 + i);
				res->D.Concat.Strings[i] = CclParseStringDesc(l);
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
				res->D.Line.Font = CFont::Get(LuaToString(l, -1));
				if (!res->D.Line.Font) {
					LuaError(l, "Bad Font name :'%s'" _C_ LuaToString(l, -1));
				}
				lua_pop(l, 1); // font name.
			}
			lua_pop(l, 1); // table.
		} else if (!strcmp(key, "PlayerName")) {
			res->e = EString_PlayerName;
			res->D.PlayerName = CclParseNumberDesc(l);
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
	Assert(unitdesc);

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
	const CUnitType **type;
	std::string s;
	int a;
	int b;

	Assert(number);
	switch (number->e) {
		case ENumber_Lua :     // a lua function.
			return CallLuaNumberFunction(number->D.Index);
		case ENumber_Dir :     // directly a number.
			return number->D.Val;
		case ENumber_Add :     // a + b.
			return EvalNumber(number->D.binOp.Left) + EvalNumber(number->D.binOp.Right);
		case ENumber_Sub :     // a - b.
			return EvalNumber(number->D.binOp.Left) - EvalNumber(number->D.binOp.Right);
		case ENumber_Mul :     // a * b.
			return EvalNumber(number->D.binOp.Left) * EvalNumber(number->D.binOp.Right);
		case ENumber_Div :     // a / b.
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			if (!b) { // FIXME : manage better this.
				return 0;
			}
			return a / b;
		case ENumber_Min :     // a <= b ? a : b
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return std::min(a, b);
		case ENumber_Max :     // a >= b ? a : b
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return std::max(a, b);
		case ENumber_Gt  :     // a > b  ? 1 : 0
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return (a > b ? 1 : 0);
		case ENumber_GtEq :    // a >= b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return (a >= b ? 1 : 0);
		case ENumber_Lt  :     // a < b  ? 1 : 0
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return (a < b ? 1 : 0);
		case ENumber_LtEq :    // a <= b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return (a <= b ? 1 : 0);
		case ENumber_Eq  :     // a == b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return (a == b ? 1 : 0);
		case ENumber_NEq  :    // a != b ? 1 : 0
			a = EvalNumber(number->D.binOp.Left);
			b = EvalNumber(number->D.binOp.Right);
			return (a != b ? 1 : 0);

		case ENumber_Rand :    // random(a) [0..a-1]
			a = EvalNumber(number->D.N);
			return SyncRand() % a;
		case ENumber_UnitStat : // property of unit.
			unit = EvalUnit(number->D.UnitStat.Unit);
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
				&& !(s = EvalString(number->D.VideoTextLength.String)).empty()) {
				return number->D.VideoTextLength.Font->Width(s);
			} else { // ERROR.
				return 0;
			}
		case ENumber_StringFind : // s.find(c)
			if (number->D.StringFind.String != nullptr
				&& !(s = EvalString(number->D.StringFind.String)).empty()) {
				size_t pos = s.find(number->D.StringFind.C);
				return pos != std::string::npos ? (int)pos : -1;
			} else { // ERROR.
				return 0;
			}
		case ENumber_NumIf : // cond ? True : False;
			if (EvalNumber(number->D.NumIf.Cond)) {
				return EvalNumber(number->D.NumIf.BTrue);
			} else if (number->D.NumIf.BFalse) {
				return EvalNumber(number->D.NumIf.BFalse);
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
		case ENumber_PlayerData : // getplayerdata(player, data, res);
			int player = EvalNumber(number->D.PlayerData.Player);
			std::string data = EvalString(number->D.PlayerData.DataType);
			//Wyrmgus start
//			std::string res = EvalString(number->D.PlayerData.ResType);
			std::string res;
			if (number->D.PlayerData.ResType != nullptr) {
				res = EvalString(number->D.PlayerData.ResType);
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
	const CUnitType **type;	// Temporary unit type
	CUpgrade **upgrade;	// Temporary upgrade
	int **resource;		// Temporary resource
	stratagus::faction **faction;	// Temporary faction
	//Wyrmgus end

	Assert(s);
	switch (s->e) {
		case EString_Lua :     // a lua function.
			return CallLuaStringFunction(s->D.Index);
		case EString_Dir :     // directly a string.
			return std::string(s->D.Val);
		case EString_Concat :     // a + b -> "ab"
			res = EvalString(s->D.Concat.Strings[0]);
			for (int i = 1; i < s->D.Concat.n; i++) {
				res += EvalString(s->D.Concat.Strings[i]);
			}
			return res;
		case EString_String : {   // 42 -> "42".
			char buffer[16]; // Should be enough ?
			sprintf(buffer, "%d", EvalNumber(s->D.Number));
			return std::string(buffer);
		}
		case EString_InverseVideo : // "a" -> "~<a~>"
			tmp1 = EvalString(s->D.String);
			// FIXME replace existing "~<" by "~>" in tmp1.
			res = std::string("~<") + tmp1 + "~>";
			return res;
		case EString_UnitName : // name of the UnitType
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr) {
				//Wyrmgus start
//				return unit->Type->Name;
				if (!unit->GetName().empty() && unit->Identified) {
					return unit->GetName();
				} else if (!unit->Identified) {
					return unit->GetTypeName() + " (Unidentified)";
				} else {
					return unit->GetTypeName();
				}
				//Wyrmgus end
			} else { // ERROR.
				return std::string("");
			}
		//Wyrmgus start
		case EString_UnitTypeName : // name of the UnitType
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr && !unit->GetName().empty() && ((unit->Prefix == nullptr && unit->Suffix == nullptr && unit->Spell == nullptr) || unit->Unique || unit->Work != nullptr || unit->Elixir != nullptr)) { //items with affixes use their type name in their given name, so there's no need to repeat their type name
				return unit->GetTypeName();
			} else { // only return a unit type name if the unit has a personal name (otherwise the unit type name would be returned as the unit name)
				return std::string("");
			}
		case EString_UnitTrait : // name of the unit's trait
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr && unit->Trait != nullptr) {
				return _(unit->Trait->get_name().c_str());
			} else {
				return std::string("");
			}
		case EString_UnitSpell : // name of the unit's spell
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr && unit->Spell != nullptr) {
				std::string spell_description = unit->Spell->Description;
				spell_description[0] = tolower(spell_description[0]);
				return unit->Spell->Name + " (" + spell_description + ")";
			} else {
				return std::string("");
			}
		case EString_UnitQuote : // unit's quote
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr) {
				if (!unit->Unique) {
					if (unit->Work != nullptr) {
						return unit->Work->get_quote();
					} else if (unit->Elixir != nullptr) {
						return unit->Elixir->get_quote();
					} else {
						return unit->Type->Quote;
					}
				} else {
					return unit->Unique->Quote;
				}
			} else {
				return std::string("");
			}
		case EString_UnitSettlementName : // name of the unit's settlement
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr && unit->settlement != nullptr && unit->settlement->get_site_unit() != nullptr) {
				int civilization = unit->settlement->get_site_unit()->Type->civilization;
				if (civilization != -1 && unit->settlement->get_site_unit()->Player->Faction != -1 && (unit->settlement->get_site_unit()->Player->Race == civilization || unit->settlement->get_site_unit()->Type == stratagus::faction::get_all()[unit->settlement->get_site_unit()->Player->Faction]->get_class_unit_type(unit->settlement->get_site_unit()->Type->get_unit_class()))) {
					civilization = unit->settlement->get_site_unit()->Player->Race;
				}
				return unit->settlement->GetCulturalName(civilization != -1 ? stratagus::civilization::get_all()[civilization] : nullptr);
			} else {
				return std::string("");
			}
		case EString_UnitUniqueSet : // name of the unit's unique item set
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr && unit->Unique && unit->Unique->Set) {
				return unit->Unique->Set->get_name();
			} else {
				return std::string("");
			}
		case EString_UnitUniqueSetItems : // names of the unit's unique item set's items
			unit = EvalUnit(s->D.Unit);
			if (unit != nullptr && unit->Unique && unit->Unique->Set) {
				std::string set_items_string;
				bool first = true;
				for (size_t i = 0; i < unit->Unique->Set->UniqueItems.size(); ++i) {
					if (!first) {
						set_items_string += "\n";
					} else {
						first = false;
					}
					bool item_equipped = unit->Container && unit->Container->IsUniqueItemEquipped(unit->Unique->Set->UniqueItems[i]);
					if (!item_equipped) {
						set_items_string += "~<";
					}
					set_items_string += unit->Unique->Set->UniqueItems[i]->Name;
					if (!item_equipped) {
						set_items_string += "~>";
					}
				}
				return set_items_string;
			} else {
				return std::string("");
			}
		case EString_TypeName : // name of the unit type
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).get_name();
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeIdent : // name of the unit type
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).Ident;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeClass : // name of the unit type's class
			type = s->D.Type;
			if (type != nullptr) {
				std::string str;
				if ((**type).BoolFlag[ITEM_INDEX].value) {
					str = GetItemClassNameById((**type).ItemClass).c_str();
				} else if ((**type).get_unit_class() != nullptr) {
					str = (**type).get_unit_class()->get_identifier().c_str();
				}
				str[0] = toupper(str[0]);
				size_t loc;
				while ((loc = str.find("-")) != std::string::npos) {
					str.replace(loc, 1, " ");
					str[loc + 1] = toupper(str[loc + 1]);
				}
				return _(str.c_str());
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeDescription : // name of the unit type's description
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).Description;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeQuote : // name of the unit type's quote
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).Quote;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeRequirementsString : // name of the unit type's requirements string
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).RequirementsString;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeExperienceRequirementsString : // name of the unit type's experience requirements string
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).ExperienceRequirementsString;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeBuildingRulesString : // name of the unit type's building rules string
			type = s->D.Type;
			if (type != nullptr) {
				return (**type).BuildingRulesString;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeImproveIncomes : // unit type's processing bonuses
			type = s->D.Type;
			if (type != nullptr) {
				std::string improve_incomes;
				bool first = true;
				for (int res = 1; res < MaxCosts; ++res) {
					if ((**type).Stats[CPlayer::GetThisPlayer()->Index].ImproveIncomes[res] > CResource::Resources[res]->DefaultIncome) {
						if (!first) {
							improve_incomes += "\n";
						} else {
							first = false;
						}
						improve_incomes += IdentToName(DefaultResourceNames[res]);
						improve_incomes += " Processing Bonus: +";
						improve_incomes += std::to_string((long long) (**type).Stats[CPlayer::GetThisPlayer()->Index].ImproveIncomes[res] - CResource::Resources[res]->DefaultIncome);
						improve_incomes += "%";
					}
				}
				return improve_incomes;
			} else { // ERROR.
				return std::string("");
			}
		case EString_TypeLuxuryDemand : // unit type's luxury demand
			type = s->D.Type;
			if (type != nullptr) {
				std::string luxury_demand;
				bool first = true;
				for (int res = 1; res < MaxCosts; ++res) {
					if ((**type).Stats[CPlayer::GetThisPlayer()->Index].ResourceDemand[res]) {
						if (!first) {
							luxury_demand += "\n";
						} else {
							first = false;
						}
						luxury_demand += IdentToName(DefaultResourceNames[res]);
						luxury_demand += " Demand: ";
						luxury_demand += std::to_string((long long) (**type).Stats[CPlayer::GetThisPlayer()->Index].ResourceDemand[res]);
					}
				}
				return luxury_demand;
			} else { // ERROR.
				return std::string("");
			}
		case EString_UpgradeCivilization : // name of the upgrade's civilization
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				if ((**upgrade).get_civilization() != nullptr) {
					return (**upgrade).get_civilization()->get_name();
				} else {
					return std::string("");
				}
			} else { // ERROR.
				return std::string("");
			}
		case EString_UpgradeEffectsString : // upgrade's effects string
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				return (**upgrade).get_effects_string();
			} else { // ERROR.
				return std::string("");
			}
		case EString_UpgradeRequirementsString : // upgrade's effects string
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				return (**upgrade).get_requirements_string();
			} else { // ERROR.
				return std::string("");
			}
		case EString_UpgradeMaxLimit : // upgrade's max limit
			upgrade = s->D.Upgrade;
			if (upgrade != nullptr) {
				return std::to_string((long long) (**upgrade).MaxLimit);
			} else { // ERROR.
				return std::string("");
			}
		case EString_FactionCivilization : // name of the faction's civilization
			faction = s->D.Faction;
			
			if (faction != nullptr) {
				return (**faction).civilization->get_name();
			} else {
				return std::string("");
			}
		case EString_FactionType : // the faction's type
			faction = s->D.Faction;
			
			if (faction != nullptr) {
				return IdentToName(GetFactionTypeNameById((**faction).Type));
			} else {
				return std::string("");
			}
		case EString_FactionCoreSettlements : // the faction's core settlements
			faction = s->D.Faction;
			
			if (faction != nullptr) {
				std::string settlements_string;
				bool first = true;
				for (size_t i = 0; i < (**faction).Cores.size(); ++i) {
					if (!first) {
						settlements_string += "\n";
					} else {
						first = false;
					}
					bool has_settlement = (**faction).Cores[i]->get_site_unit() && (**faction).Cores[i]->get_site_unit()->Player == CPlayer::GetThisPlayer() && (**faction).Cores[i]->get_site_unit()->CurrentAction() != UnitAction::Built;
					if (!has_settlement) {
						settlements_string += "~<";
					}
					settlements_string += (**faction).Cores[i]->GetCulturalName(CPlayer::GetThisPlayer()->Race != -1 ? stratagus::civilization::get_all()[CPlayer::GetThisPlayer()->Race] : nullptr);
					if (!has_settlement) {
						settlements_string += "~>";
					}
				}
				return settlements_string;
			} else {
				return std::string("");
			}
		case EString_ResourceIdent : // resource ident
			resource = s->D.Resource;
			if (resource != nullptr) {
				return DefaultResourceNames[(**resource)];
			} else { // ERROR.
				return std::string("");
			}
		case EString_ResourceName : // resource ident
			resource = s->D.Resource;
			if (resource != nullptr) {
				return IdentToName(DefaultResourceNames[(**resource)]);
			} else { // ERROR.
				return std::string("");
			}
		case EString_ResourceConversionRates : // unit type's processing bonuses
			resource = s->D.Resource;
			if (resource != nullptr) {
				std::string conversion_rates;
				bool first = true;
				for (const CResource *child_resource : CResource::Resources[(**resource)]->ChildResources) {
					if (child_resource->ID == TradeCost || child_resource->Hidden) {
						continue;
					}
					if (!first) {
						conversion_rates += "\n";
					} else {
						first = false;
					}
					conversion_rates += child_resource->Name;
					conversion_rates += " to ";
					conversion_rates += CResource::Resources[(**resource)]->Name;
					conversion_rates += " Conversion Rate: ";
					conversion_rates += std::to_string((long long) child_resource->FinalResourceConversionRate);
					conversion_rates += "%";
				}
				return conversion_rates;
			} else { // ERROR.
				return std::string("");
			}
		case EString_ResourceImproveIncomes : // unit type's processing bonuses
			resource = s->D.Resource;
			if (resource != nullptr) {
				std::string improve_incomes;
				bool first = true;
				if (CPlayer::GetThisPlayer()->Incomes[(**resource)] > CResource::Resources[(**resource)]->DefaultIncome) {
					first = false;
					improve_incomes += CResource::Resources[(**resource)]->Name;
					improve_incomes += " Processing Bonus: +";
					improve_incomes += std::to_string((long long) CPlayer::GetThisPlayer()->Incomes[(**resource)] - CResource::Resources[(**resource)]->DefaultIncome);
					improve_incomes += "%";
				}
				for (const CResource *child_resource : CResource::Resources[(**resource)]->ChildResources) {
					if (child_resource->ID == TradeCost || child_resource->Hidden) {
						continue;
					}
					if (CPlayer::GetThisPlayer()->Incomes[child_resource->ID] > child_resource->DefaultIncome) {
						if (!first) {
							improve_incomes += "\n";
						} else {
							first = false;
						}
						improve_incomes += child_resource->Name;
						improve_incomes += " Processing Bonus: +";
						improve_incomes += std::to_string((long long) CPlayer::GetThisPlayer()->Incomes[child_resource->ID] - child_resource->DefaultIncome);
						improve_incomes += "%";
					}
				}
				return improve_incomes;
			} else { // ERROR.
				return std::string("");
			}
		//Wyrmgus end
		case EString_If : // cond ? True : False;
			if (EvalNumber(s->D.If.Cond)) {
				return EvalString(s->D.If.BTrue);
			} else if (s->D.If.BFalse) {
				return EvalString(s->D.If.BFalse);
			} else {
				return std::string("");
			}
		case EString_SubString : // substring(s, begin, end)
			if (s->D.SubString.String != nullptr
				&& !(tmp1 = EvalString(s->D.SubString.String)).empty()) {
				int begin;
				int end;

				begin = EvalNumber(s->D.SubString.Begin);
				if ((unsigned) begin > tmp1.size() && begin > 0) {
					return std::string("");
				}
				res = tmp1.c_str() + begin;
				if (s->D.SubString.End) {
					end = EvalNumber(s->D.SubString.End);
				} else {
					end = -1;
				}
				if ((unsigned)end < res.size() && end >= 0) {
					res[end] = '\0';
				}
				return res;
			} else { // ERROR.
				return std::string("");
			}
		case EString_Line : // line n of the string
			if (s->D.Line.String == nullptr || (tmp1 = EvalString(s->D.Line.String)).empty()) {
				return std::string(""); // ERROR.
			} else {
				int line;
				int maxlen;
				CFont *font;

				line = EvalNumber(s->D.Line.Line);
				if (line <= 0) {
					return std::string("");
				}
				if (s->D.Line.MaxLen) {
					maxlen = EvalNumber(s->D.Line.MaxLen);
					maxlen = std::max(maxlen, 0);
				} else {
					maxlen = 0;
				}
				font = s->D.Line.Font;
				res = GetLineFont(line, tmp1, maxlen, font);
				return res;
			}
		case EString_PlayerName : // player name
			return std::string(CPlayer::Players[EvalNumber(s->D.PlayerName)]->Name);
	}
	return std::string("");
}


/**
**  Free the unit expression content. (not the pointer itself).
**
**  @param unitdesc  struct to free
*/
void FreeUnitDesc(UnitDesc *)
{
#if 0 // Nothing to free mow.
	if (!unitdesc) {
		return;
	}
#endif
}

/**
**  Free the number expression content. (not the pointer itself).
**
**  @param number  struct to free
*/
void FreeNumberDesc(NumberDesc *number)
{
	if (number == 0) {
		return;
	}
	switch (number->e) {
		case ENumber_Lua :     // a lua function.
		// FIXME: when lua table should be freed ?
		case ENumber_Dir :     // directly a number.
			break;
		case ENumber_Add :     // a + b.
		case ENumber_Sub :     // a - b.
		case ENumber_Mul :     // a * b.
		case ENumber_Div :     // a / b.
		case ENumber_Min :     // a <= b ? a : b
		case ENumber_Max :     // a >= b ? a : b
		case ENumber_Gt  :     // a > b  ? 1 : 0
		case ENumber_GtEq :    // a >= b ? 1 : 0
		case ENumber_Lt  :     // a < b  ? 1 : 0
		case ENumber_LtEq :    // a <= b ? 1 : 0
		case ENumber_NEq  :    // a <> b ? 1 : 0
		case ENumber_Eq  :     // a == b ? 1 : 0
			FreeNumberDesc(number->D.binOp.Left);
			FreeNumberDesc(number->D.binOp.Right);
			delete number->D.binOp.Left;
			delete number->D.binOp.Right;
			break;
		case ENumber_Rand :    // random(a) [0..a-1]
			FreeNumberDesc(number->D.N);
			delete number->D.N;
			break;
		case ENumber_UnitStat : // property of unit.
			FreeUnitDesc(number->D.UnitStat.Unit);
			delete number->D.UnitStat.Unit;
			break;
		case ENumber_TypeStat : // property of unit type.
			delete *number->D.TypeStat.Type;
			break;
		case ENumber_VideoTextLength : // VideoTextLength(font, s)
			FreeStringDesc(number->D.VideoTextLength.String);
			delete number->D.VideoTextLength.String;
			break;
		case ENumber_StringFind : // strchr(s, c) - s.
			FreeStringDesc(number->D.StringFind.String);
			delete number->D.StringFind.String;
			break;
		case ENumber_NumIf : // cond ? True : False;
			FreeNumberDesc(number->D.NumIf.Cond);
			delete number->D.NumIf.Cond;
			FreeNumberDesc(number->D.NumIf.BTrue);
			delete number->D.NumIf.BTrue;
			FreeNumberDesc(number->D.NumIf.BFalse);
			delete number->D.NumIf.BFalse;
			break;
		case ENumber_PlayerData : // getplayerdata(player, data, res);
			FreeNumberDesc(number->D.PlayerData.Player);
			delete number->D.PlayerData.Player;
			FreeStringDesc(number->D.PlayerData.DataType);
			delete number->D.PlayerData.DataType;
			FreeStringDesc(number->D.PlayerData.ResType);
			delete number->D.PlayerData.ResType;
			break;
		//Wyrmgus start
		case ENumber_TypeTrainQuantity : // Class of the unit type
			delete *number->D.Type;
			break;
		//Wyrmgus end
	}
}

/**
**  Free the String expression content. (not the pointer itself).
**
**  @param s  struct to free
*/
void FreeStringDesc(StringDesc *s)
{
	int i;

	if (s == 0) {
		return;
	}
	switch (s->e) {
		case EString_Lua :     // a lua function.
			// FIXME: when lua table should be freed ?
			break;
		case EString_Dir :     // directly a string.
			delete[] s->D.Val;
			break;
		case EString_Concat :  // "a" + "b" -> "ab"
			for (i = 0; i < s->D.Concat.n; i++) {
				FreeStringDesc(s->D.Concat.Strings[i]);
				delete s->D.Concat.Strings[i];
			}
			delete[] s->D.Concat.Strings;

			break;
		case EString_String : // 42 -> "42"
			FreeNumberDesc(s->D.Number);
			delete s->D.Number;
			break;
		case EString_InverseVideo : // "a" -> "~<a~>"
			FreeStringDesc(s->D.String);
			delete s->D.String;
			break;
		case EString_UnitName : // Name of the UnitType
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		//Wyrmgus start
		case EString_UnitTypeName : // Name of the UnitType
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_UnitTrait : // Trait of the unit
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_UnitSpell : // Spell of the unit
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_UnitQuote : // Quote of the unit
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_UnitSettlementName : // Settlement name of the unit
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_UnitUniqueSet : // Unique item set name of the unit
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_UnitUniqueSetItems : // Unique item names for the unit's unique item set name
			FreeUnitDesc(s->D.Unit);
			delete s->D.Unit;
			break;
		case EString_TypeName : // Name of the unit type
		case EString_TypeIdent : // Ident of the unit type
		case EString_TypeClass : // Class of the unit type
		case EString_TypeDescription : // Description of the unit type
		case EString_TypeQuote : // Quote of the unit type
		case EString_TypeRequirementsString : // Requirements string of the unit type
		case EString_TypeExperienceRequirementsString : // Experience requirements string of the unit type
		case EString_TypeBuildingRulesString : // Building rules string of the unit type
		case EString_TypeImproveIncomes : // Income improvements of the unit type
		case EString_TypeLuxuryDemand : // Luxury demand of the unit type
			delete *s->D.Type;
			break;
		case EString_UpgradeCivilization : // Civilization of the upgrade
		case EString_UpgradeEffectsString : // Effects string of the upgrade
		case EString_UpgradeRequirementsString : // Requirements string of the upgrade
		case EString_UpgradeMaxLimit : // Requirements string of the upgrade
			delete *s->D.Upgrade;
			break;
		case EString_ResourceIdent : // Ident of the resource
		case EString_ResourceName : // Name of the resource
		case EString_ResourceConversionRates : // Conversion rates of the resource
		case EString_ResourceImproveIncomes : // Income improvements of the resource
			delete *s->D.Resource;
			break;
		case EString_FactionCivilization : // Civilization of the faction
		case EString_FactionType : // Type of the faction
		case EString_FactionCoreSettlements : // Core settlements of the faction
			delete *s->D.Faction;
			break;
		//Wyrmgus end
		case EString_If : // cond ? True : False;
			FreeNumberDesc(s->D.If.Cond);
			delete s->D.If.Cond;
			FreeStringDesc(s->D.If.BTrue);
			delete s->D.If.BTrue;
			FreeStringDesc(s->D.If.BFalse);
			delete s->D.If.BFalse;
			break;
		case EString_SubString : // substring(s, begin, end)
			FreeStringDesc(s->D.SubString.String);
			delete s->D.SubString.String;
			FreeNumberDesc(s->D.SubString.Begin);
			delete s->D.SubString.Begin;
			FreeNumberDesc(s->D.SubString.End);
			delete s->D.SubString.End;
			break;
		case EString_Line : // line n of the string
			FreeStringDesc(s->D.Line.String);
			delete s->D.Line.String;
			FreeNumberDesc(s->D.Line.Line);
			delete s->D.Line.Line;
			FreeNumberDesc(s->D.Line.MaxLen);
			delete s->D.Line.MaxLen;
			break;
		case EString_PlayerName : // player name
			FreeNumberDesc(s->D.PlayerName);
			delete s->D.PlayerName;
			break;
	}
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
	Assert(0 < lua_gettop(l) && lua_gettop(l) <= 3);
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

	Assert(0 < lua_gettop(l) && lua_gettop(l) <= 3);
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
	Assert(narg);
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
	lua_register(Lua, "ResourceIdent", CclResourceIdent);
	lua_register(Lua, "ResourceName", CclResourceName);
	lua_register(Lua, "ResourceConversionRates", CclResourceConversionRates);
	lua_register(Lua, "ResourceImproveIncomes", CclResourceImproveIncomes);
	//Wyrmgus end
	lua_register(Lua, "SubString", CclSubString);
	lua_register(Lua, "Line", CclLine);
	lua_register(Lua, "GameInfo", CclGameInfo);
	lua_register(Lua, "PlayerName", CclPlayerName);
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
	lua_pushstring(l, StratagusLibPath.c_str());
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
	if ((strpbrk(userdir, ":*?\"<>|") != 0 || strstr(userdir, "..") != 0) && (strstr(userdir, "workshop") == 0 || strstr(userdir, "content") == 0 || strstr(userdir, "370070") == 0)) { //special case for Wyrmsun's workshop folder
	//Wyrmgus end
		LuaError(l, "Forbidden directory");
	}
	char directory[PATH_MAX];

	if (pathtype == 1) {
		++userdir;
		std::string dir(Parameters::Instance.GetUserDirectory());
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
		snprintf(directory, sizeof(directory), "%s/%s", StratagusLibPath.c_str(), userdir);
		#else
		snprintf(directory, sizeof(directory), "%s",  userdir);
		#endif
	}
	lua_pop(l, 1);
	lua_newtable(l);
	std::vector<FileList> flp;
	n = ReadDataDirectory(directory, flp, sortmode);
	int j = 0;
	for (int i = 0; i < n; ++i) {
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
	Assert(l);
	if (Damage) {
		FreeNumberDesc(Damage);
		delete Damage;
	}
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
	const int status = luaL_loadbuffer(Lua, command.c_str(), command.size(), command.c_str());

	if (!status) {
		LuaCall(0, 1, exitOnError);
	} else {
		report(status, exitOnError);
	}
	return status;
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
	//Assert(!is_root || !lua_gettop(l));
	if (is_root) {
		lua_getglobal(l, "_G");// global table in lua.
	}
	std::string res;
	const std::string tablesName = ConcatTableString(blockTableNames);

	if (blockTableNames.empty() == false) {
		res = "if (" + tablesName + " == nil) then " + tablesName + " = {} end\n";
	}
	Assert(lua_istable(l, -1));

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
	//Assert(!is_root || !lua_gettop(l));
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
		std::string path = Parameters::Instance.GetUserDirectory();

		if (!GameName.empty()) {
			path += "/";
			path += GameName;
		}
		path += "/preferences.lua";

		FILE *fd = fopen(path.c_str(), "w");
		if (!fd) {
			DebugPrint("Cannot open file %s for writing\n" _C_ path.c_str());
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
	stratagus::faction::remove(faction_name);
}

void DeleteModUnitType(const std::string &unit_type_ident)
{
	CUnitType *unit_type = CUnitType::get(unit_type_ident.c_str());
	
	if (Editor.Running == EditorEditing) {
		std::vector<CUnit *> units_to_remove;

		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit *unit = *it;

			if (unit->Type == unit_type) {
				units_to_remove.push_back(unit);
			}
		}
		
		for (size_t i = 0; i < units_to_remove.size(); ++i) {
			EditorActionRemoveUnit(*units_to_remove[i], false);
		}
		Editor.UnitTypes.erase(std::remove(Editor.UnitTypes.begin(), Editor.UnitTypes.end(), unit_type->Ident), Editor.UnitTypes.end());
		RecalculateShownUnits();
	}
	for (stratagus::civilization *civilization : stratagus::civilization::get_all()) {
		civilization->remove_class_unit_type(unit_type);
	}
	for (stratagus::faction *faction : stratagus::faction::get_all()) {
		faction->remove_class_unit_type(unit_type);
	}
	for (CUnitType *other_unit_type : CUnitType::get_all()) { //remove this unit from the "Trains", "TrainedBy", "Drops" and "AiDrops" vectors of other unit types
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
	int buttons_size = UnitButtonTable.size();
	for (int j = (buttons_size - 1); j >= 0; --j) {
		if (UnitButtonTable[j]->UnitMask.find(unit_type->Ident) != std::string::npos) { //remove this unit from the "ForUnit" array of buttons
			UnitButtonTable[j]->UnitMask = FindAndReplaceString(UnitButtonTable[j]->UnitMask, unit_type->Ident + ",", "");
		}
		if (UnitButtonTable[j]->Value == unit_type->Slot && UnitButtonTable[j]->ValueStr == unit_type->Ident) {
			delete UnitButtonTable[j];
			UnitButtonTable.erase(std::remove(UnitButtonTable.begin(), UnitButtonTable.end(), UnitButtonTable[j]), UnitButtonTable.end());
		}
	}
	CUnitType::remove(unit_type);
}

void DisableMod(const std::string &mod_file)
{
	const int unit_types_size = CUnitType::get_all().size();
	for (int i = (unit_types_size - 1); i >= 0; --i) {
		
		if (CUnitType::get_all()[i]->Mod == mod_file) {
			DeleteModUnitType(CUnitType::get_all()[i]->Ident);
		}
	}
		
	for (CUnitType *unit_type : CUnitType::get_all()) {
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
	
	std::vector<stratagus::faction *> factions_to_remove;
	for (stratagus::faction *faction : stratagus::faction::get_all()) {
		if (faction->Mod == mod_file) {
			factions_to_remove.push_back(faction);
		}
	}

	for (stratagus::faction *faction : factions_to_remove) {
		stratagus::faction::remove(faction);
	}
}

void SetDLCFileEquivalency(const std::string dlc_file, const std::string replacement_file)
{
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
		fprintf(stderr, "Maybe you need to specify another gamepath with '-d /path/to/datadir'?\n");
		ExitFatal(-1);
	}

	ShowLoadProgress(_("Loading Script \"%s\"\n"), name.c_str());
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
