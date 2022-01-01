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
/**@name button_cmd.h - The button command header file. */
//
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#pragma once

/// Button Commands that need target selection
enum class ButtonCmd {
	None = -1,
	Move,           /// order move
	Attack,         /// order attack
	Repair,         /// order repair
	Harvest,        /// order harvest
	Build,          /// order build
	BuildClass, //build order (unit class)
	Patrol,         /// order patrol
	AttackGround,   /// order attack ground
	SpellCast,      /// order cast spell
	Unload,         /// order unload unit
	Stop,           /// order stop
	Button,         /// choose other button set
	Train,          /// order train
	TrainClass, //train order (unit class)
	StandGround,    /// order stand ground
	Return,         /// order return goods
	Research, //research order
	ResearchClass, //research order (upgrade class)
	LearnAbility,   /// order learn ability
	ExperienceUpgradeTo,   /// order upgrade (experience)
	UpgradeTo,      //upgrade order
	UpgradeToClass, //upgrade order (unit class)
	RallyPoint,		/// set rally point
	Faction,		/// change faction
	Dynasty,		/// change dynasty
	Quest,			/// receive quest
	Buy,				/// buy item
	ProduceResource,	/// produce a resource
	SellResource,		/// sell a resource
	BuyResource,		/// buy a resource
	Salvage,			/// salvage a building
	EnterMapLayer,	/// enter a map layer
	Unit,			/// used to display popups for inventory items, units in transporters and the unit under the cursor
	Tile,			/// used to display popups for the tile under the cursor
	Player,			/// used to display player-related popups
	EditorUnit,		/// used to display popups for editor unit type buttons
	Cancel,         /// cancel
	CancelUpgrade,  /// cancel upgrade
	CancelTrain,    /// cancel training
	CancelBuild,    /// cancel building
	CallbackAction
};
