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
/**@name script.h - The clone configuration language header file. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer and Jimmy Salmon
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

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>
//Wyrmgus start
#include <map>
//Wyrmgus end
#ifdef __cplusplus
extern "C" {
#endif
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CDate;
//Wyrmgus start
class CFaction;
//Wyrmgus end
class CUnit;
class CUnitType;
//Wyrmgus start
class CUpgrade;
//Wyrmgus end
class CFile;
class CFont;

struct LuaUserData {
	int Type;
	void *Data;
};

enum {
	LuaUnitType = 100,
	LuaSoundType
};

extern lua_State *Lua;

extern int LuaLoadFile(const std::string &file, const std::string &strArg = "");
extern int LuaCall(int narg, int clear, bool exitOnError = true);

#define LuaError(l, args) \
	do { \
		PrintFunction(); \
		fprintf(stderr, args); \
		fprintf(stderr, "\n"); \
	} while (0)

#define LuaCheckArgs(l, args) \
	do { \
		if (lua_gettop(l) != args) { \
			LuaError(l, "incorrect argument"); \
		} \
	} while (0)

#if LUA_VERSION_NUM <= 501

inline size_t lua_rawlen(lua_State *l, int index)
{
	return luaL_getn(l, index);
}

#endif

/// All possible value for a number.
enum ENumber {
	ENumber_Lua,         /// a lua function.
	ENumber_Dir,         /// directly a number.
	ENumber_Add,         /// a + b.
	ENumber_Sub,         /// a - b.
	ENumber_Mul,         /// a * b.
	ENumber_Div,         /// a / b.
	ENumber_Min,         /// Min(a, b).
	ENumber_Max,         /// Max(a, b).
	ENumber_Rand,        /// Rand(a) : number in [0..a-1].

	ENumber_Gt,          /// a  > b.
	ENumber_GtEq,        /// a >= b.
	ENumber_Lt,          /// a  < b.
	ENumber_LtEq,        /// a <= b.
	ENumber_Eq,          /// a == b.
	ENumber_NEq,         /// a <> b.

	ENumber_VideoTextLength, /// VideoTextLength(font, string).
	ENumber_StringFind,      /// strchr(string, char) - s.

	ENumber_UnitStat,    /// Property of Unit.
	ENumber_TypeStat,    /// Property of UnitType.
	//Wyrmgus start
	ENumber_TypeTrainQuantity,	/// Unit type's trained quantity
	//Wyrmgus end

	ENumber_NumIf,       /// If cond then Number1 else Number2.

	ENumber_PlayerData   /// Numeric Player Data
};

/// All possible value for a unit.
enum EUnit {
	EUnit_Ref           /// Unit direct reference.
	// FIXME: add others.
};

/// All possible value for a string.
enum EString {
	EString_Lua,          /// a lua function.
	EString_Dir,          /// directly a string.
	EString_Concat,       /// a + b [+ c ...].
	EString_String,       /// Convert number in string.
	EString_InverseVideo, /// Inverse video for the string ("a" -> "~<a~>").
	EString_If,           /// If cond then String1 else String2.
	EString_UnitName,     /// UnitType Name.
	//Wyrmgus start
	EString_UnitTypeName,		/// UnitType Name, if EString_UnitName is occupied by the unit's personal name.
	EString_UnitTrait,			/// Unit Trait
	EString_UnitSpell,			/// Unit spell
	EString_UnitQuote,			/// Unit quote
	EString_UnitSettlementName,	/// Unit Settlement Name
	EString_UnitUniqueSet,		/// Unit Unique Item Set
	EString_UnitUniqueSetItems,	/// Unit Unique Item Set Items
	EString_TypeName,			/// Unit type's name
	EString_TypeIdent,			/// Unit type's ident
	EString_TypeClass,			/// Unit type's class
	EString_TypeDescription,	/// Unit type's description
	EString_TypeQuote,			/// Unit type's quote
	EString_TypeRequirementsString,	/// Unit type's requirements string
	EString_TypeExperienceRequirementsString,	/// Unit type's experience requirements string
	EString_TypeBuildingRulesString,	/// Unit type's building rules string
	EString_TypeImproveIncomes,			/// Unit type's processing bonuses
	EString_TypeLuxuryDemand,			/// Unit type's luxury demand
	EString_UpgradeCivilization,	/// Upgrade's civilization
	EString_UpgradeEffectsString,	/// Upgrade's effects string
	EString_UpgradeRequirementsString,	/// Upgrade's requirements string
	EString_UpgradeMaxLimit,	/// Upgrade's max limit
	EString_FactionCivilization,	/// Faction's civilization
	EString_FactionType,		/// Faction's type
	EString_FactionCoreSettlements,	/// Core settlements of the faction
	EString_ResourceIdent,	/// Resource's ident
	EString_ResourceName,	/// Resource's name
	EString_ResourceConversionRates,		/// Resource's conversion rates
	EString_ResourceImproveIncomes,			/// Resource's processing bonuses
	//Wyrmgus end
	EString_SubString,    /// SubString.
	EString_Line,         /// line n of the string.
	EString_PlayerName    /// player name.
	// add more...
};

/// All possible value for a game info string.
enum ES_GameInfo {
	ES_GameInfo_Objectives       /// All Objectives of the game.
};

/**
**  Enumeration to know which variable to be selected.
*/
enum EnumVariable {
	VariableValue = 0,  /// Value of the variable.
	VariableMax,        /// Max of the variable.
	VariableIncrease,   /// Increase value of the variable.
	VariableDiff,       /// (Max - Value)
	VariablePercent,    /// (100 * Value / Max)
	//Wyrmgus start
//	VariableName        /// Name of the variable.
	VariableName,		/// Name of the variable.
	VariableChange,		/// Change of the variable.
	VariableIncreaseChange,	/// Change of the variable's increase.
	//Wyrmgus end
};

/**
**  Enumeration of unit
*/
enum EnumUnit {
	UnitRefItSelf = 0,      /// unit.
	UnitRefInside,          /// unit->Inside.
	UnitRefContainer,       /// Unit->Container.
	UnitRefWorker,          /// unit->Data.Built.Worker
	UnitRefGoal             /// unit->Goal
};

/**
**  Number description.
**  Use to describe complex number in script to use when game running.
** [Deprecated]: give access to lua.
*/
struct NumberDesc;

/**
** Unit description
**  Use to describe complex unit in script to use when game running.
*/
struct UnitDesc;

/**
** String description
**  Use to describe complex string in script to use when game running.
*/
struct StringDesc;

/// for Bin operand  a ?? b
struct BinOp {
	NumberDesc *Left;           /// Left operand.
	NumberDesc *Right;          /// Right operand.
};

/**
**  Number description.
*/
struct NumberDesc {
	ENumber e;       /// which number.
	union {
		unsigned int Index; /// index of the lua function.
		int Val;       /// Direct value.
		NumberDesc *N; /// Other number.
		//Wyrmgus start
		CUnitType **Type;           /// Which unit type.
		CUpgrade **Upgrade;         /// Which upgrade.
		CFaction **Faction;			/// Which faction.
		int **Resource;				/// Which resource
		//Wyrmgus end
		BinOp binOp;   /// For binary operand.
		struct {
			UnitDesc *Unit;            /// Which unit.
			int Index;                 /// Which index variable.
			EnumVariable Component;    /// Which component.
			int Loc;                   /// Location of Variables[].
		} UnitStat;
		struct {
			CUnitType **Type;           /// Which unit type.
			int Index;                 /// Which index variable.
			EnumVariable Component;    /// Which component.
			int Loc;                   /// Location of Variables[].
		} TypeStat;
		struct {
			StringDesc *String; /// String.
			CFont *Font;        /// Font.
		} VideoTextLength;
		struct {
			StringDesc *String; /// String.
			char C;             /// Char.
		} StringFind;
		struct {
			NumberDesc *Cond;   /// Branch condition.
			NumberDesc *BTrue;  /// Number if Cond is true.
			NumberDesc *BFalse; /// Number if Cond is false.
		} NumIf; /// conditional string.
		struct {
			NumberDesc *Player;   /// Number of player
			StringDesc *DataType; /// Player's data
			StringDesc *ResType;  /// Resource type
		} PlayerData; /// conditional string.
	} D;
};

/**
**  Unit description.
*/
struct UnitDesc {
	EUnit e;       /// which unit;
	union {
		CUnit **AUnit; /// Address of the unit.
	} D;
};

/**
**  String description.
*/
struct StringDesc {
	EString e;       /// which number.
	union {
		unsigned int Index; /// index of the lua function.
		char *Val;       /// Direct value.
		struct {
			StringDesc **Strings;  /// Array of operands.
			int n;                 /// number of operand to concat
		} Concat; /// for Concat two string.
		NumberDesc *Number;  /// Number.
		StringDesc *String;  /// String.
		UnitDesc *Unit;      /// Unit desciption.
		//Wyrmgus start
		CUnitType **Type;           /// Which unit type.
		CUpgrade **Upgrade;         /// Which upgrade.
		CFaction **Faction;			/// Which faction.
		int **Resource;				/// Which resource
		//Wyrmgus end
		struct {
			NumberDesc *Cond;  /// Branch condition.
			StringDesc *BTrue;  /// String if Cond is true.
			StringDesc *BFalse; /// String if Cond is false.
		} If; /// conditional string.
		struct {
			StringDesc *String;  /// Original string.
			NumberDesc *Begin;   /// Begin of result string.
			NumberDesc *End;     /// End of result string.
		} SubString; /// For extract a substring
		struct {
			StringDesc *String;  /// Original string.
			NumberDesc *Line;    /// Line number.
			NumberDesc *MaxLen;  /// Max length of line.
			CFont *Font;         /// Font to consider (else (-1) consider just char).
		} Line; /// For specific line.
		ES_GameInfo GameInfoType;
		NumberDesc *PlayerName;  /// Player name.
	} D;
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern int CclInConfigFile;        /// True while config file parsing

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern const char *LuaToString(lua_State *l, int narg);
extern int LuaToNumber(lua_State *l, int narg);
extern unsigned int LuaToUnsignedNumber(lua_State *l, int narg);
extern bool LuaToBoolean(lua_State *l, int narg);

extern const char *LuaToString(lua_State *l, int index, int subIndex);
extern int LuaToNumber(lua_State *l, int index, int subIndex);
extern unsigned int LuaToUnsignedNumber(lua_State *l, int index, int subIndex);
extern bool LuaToBoolean(lua_State *l, int index, int subIndex);

extern void LuaGarbageCollect();  /// Perform garbage collection
extern void InitLua();                /// Initialise Lua
extern void LoadCcl(const std::string &filename, const std::string &luaArgStr = "");  /// Load ccl config file
extern void SavePreferences();        /// Save user preferences
//Wyrmgus start
extern void DeleteModFaction(const std::string &faction_name);
extern void DeleteModUnitType(const std::string &unit_type_ident);
extern void DisableMod(const std::string &mod_file);
extern void SetDLCFileEquivalency(const std::string dlc_file, const std::string replacement_file);
extern std::map<std::string, std::string> DLCFileEquivalency;
//Wyrmgus end
extern int CclCommand(const std::string &command, bool exitOnError = true);

extern void ScriptRegister();

extern std::string SaveGlobal(lua_State *l); /// For saving lua state

CUnit *CclGetUnitFromRef(lua_State *l);

/**
**  Get a position from lua state
**
**  @param l  Lua state.
**  @param x  pointer to output x position.
**  @param y  pointer to output y position.
*/
template <typename T>
static void CclGetPos(lua_State *l, T *x , T *y, const int offset = -1)
{
	if (!lua_istable(l, offset) || lua_rawlen(l, offset) != 2) {
		LuaError(l, "incorrect argument");
	}
	*x = LuaToNumber(l, offset, 1);
	*y = LuaToNumber(l, offset, 2);
}

//Wyrmgus start
extern void CclGetDate(lua_State *l, CDate *d, const int offset = -1);
//Wyrmgus end

extern NumberDesc *Damage;  /// Damage calculation for missile.

/// transform string in corresponding index.
extern EnumVariable Str2EnumVariable(lua_State *l, const char *s);
extern NumberDesc *CclParseNumberDesc(lua_State *l); /// Parse a number description.
extern UnitDesc *CclParseUnitDesc(lua_State *l);     /// Parse a unit description.
extern CUnitType **CclParseTypeDesc(lua_State *l);   /// Parse a unit type description.
//Wyrmgus start
extern CUpgrade **CclParseUpgradeDesc(lua_State *l);   /// Parse an upgrade description.
extern int **CclParseResourceDesc(lua_State *l);   /// Parse a resource description.
extern CFaction **CclParseFactionDesc(lua_State *l);   /// Parse a faction description.
//Wyrmgus end
StringDesc *CclParseStringDesc(lua_State *l);        /// Parse a string description.

extern int EvalNumber(const NumberDesc *numberdesc); /// Evaluate the number.
extern CUnit *EvalUnit(const UnitDesc *unitdesc);    /// Evaluate the unit.
std::string EvalString(const StringDesc *s);         /// Evaluate the string.

void FreeNumberDesc(NumberDesc *number);  /// Free number description content. (no pointer itself).
void FreeUnitDesc(UnitDesc *unitdesc);    /// Free unit description content. (no pointer itself).
void FreeStringDesc(StringDesc *s);       /// Frre string description content. (no pointer itself).

extern void ProcessLuaCommandQueue();
extern void QueueLuaCommand(const std::string &command);

#endif
