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
/**@name luacallback.cpp -  Lua callback. */
//
//      (c) Copyright 2008 by Francois Beerten
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

#include "stratagus.h"

#include "luacallback.h"

#include "script.h"

/**
**  LuaCallback constructor
**
**  @param l  Lua state
**  @param f  Listener function
*/
LuaCallback::LuaCallback(lua_State *l, lua_Object f) :
	luastate(l), arguments(0), rescount(0)
{
	if (!lua_isfunction(l, f)) {
		LuaError(l, "Argument isn't a function");
		Assert(0);
	}
	lua_pushvalue(l, f);
	luaref = luaL_ref(l, LUA_REGISTRYINDEX);
}

/**
**  Push the preamble on the stack to call the callback.
**  Call this function before pushing the arguments on the lua stack.
*/
void LuaCallback::pushPreamble()
{
	base = lua_gettop(luastate);
	lua_getglobal(luastate, "_TRACEBACK");
	lua_rawgeti(luastate, LUA_REGISTRYINDEX, luaref);
	arguments = 0;
}

/**
**  Push a string argument for the callback on the stack.
**
**  @param value  the integer to push on the stack
*/
void LuaCallback::pushInteger(int value)
{
	lua_pushnumber(luastate, value);
	arguments++;
}

/**
**  Push a array of integers from callback to stack
**
**  @param value  the integer to push on the stack
*/
void LuaCallback::pushIntegers(const std::vector<int> &values)
{
	lua_newtable(luastate);
	for (size_t i = 0; i < values.size(); ++i) {
		lua_pushnumber(luastate, i + 1);
		lua_pushnumber(luastate, values[i]);
		lua_settable(luastate, -3);
	}
	arguments++;
}

/**
**  Push a string argument for the callback on the stack.
**
**  @param s  the string to push on the stack
*/
void LuaCallback::pushString(const std::string &s)
{
	lua_pushstring(luastate, s.c_str());
	arguments++;
}

/**
**  Pops a boolean value for the callback on the stack.
**
*/
bool LuaCallback::popBoolean()
{
	if (rescount) {
		--rescount;
		luaL_checktype(luastate, -1, LUA_TBOOLEAN);
		bool result = lua_toboolean(luastate, -1) != 0;
		lua_pop(luastate, 1);
		return result;
	}
	LuaError(luastate, "No results left");
	return false;
}

/**
**  Pops an integer value for the callback on the stack.
**
*/
int LuaCallback::popInteger()
{
	if (rescount) {
		--rescount;
		luaL_checktype(luastate, -1, LUA_TNUMBER);
		int result = lua_tonumber(luastate, -1);
		lua_pop(luastate, 1);
		return result;
	}
	LuaError(luastate, "No results left");
	return false;
}

/**
**  Called when an action is received from a Widget. It is used
**  to be able to receive a notification that an action has
**  occurred.
**
**  @param results  the number of results to be expected in call
*/
void LuaCallback::run(int results)
{
	//FIXME call error reporting function
	int status = lua_pcall(luastate, arguments, results, base);

	if (status) {
		const char *msg = lua_tostring(luastate, -1);

		if (msg == nullptr) {
			msg = "(error with no message)";
		}
		fprintf(stderr, "%s\n", msg);
		lua_pop(luastate, 1);
	}
	rescount = results;
}

/**
**  LuaActionListener destructor
*/
LuaCallback::~LuaCallback()
{
	if (rescount) {
		fprintf(stderr, "There are still some results that weren't popped from stack\n");
	}
	luaL_unref(luastate, LUA_REGISTRYINDEX, luaref);
}
