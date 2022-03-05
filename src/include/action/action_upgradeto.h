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
/**@name action_upgradeto.h - The actions headerfile. */
//
//      (c) Copyright 1998-2012 by Lutz Sammer and Jimmy Salmon
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

#include "actions.h"

class COrder_TransformInto final : public COrder
{
	friend std::unique_ptr<COrder>COrder::NewActionTransformInto(const unit_type &type);

public:
	COrder_TransformInto() : COrder(UnitAction::TransformInto)
	{
	}

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_TransformInto>(*this);
	}

	virtual bool IsValid() const;

	virtual void Save(CFile &file, const CUnit &unit) const;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit);

	virtual void Execute(CUnit &unit);
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const;
	virtual void UpdatePathFinderData(PathFinderInput &input) { UpdatePathFinderData_NotCalled(input); }

	//Wyrmgus start
	void ConvertUnitType(const CUnit &unit, wyrmgus::unit_type &newType);
	const wyrmgus::unit_type &GetUnitType() const { return *Type; }
	//Wyrmgus end

private:
	const wyrmgus::unit_type *Type = nullptr; /// Transform unit into this unit-type
};

class COrder_UpgradeTo final : public COrder
{
	friend std::unique_ptr<COrder> COrder::NewActionUpgradeTo(CUnit &unit, const wyrmgus::unit_type &type);

public:
	COrder_UpgradeTo() : COrder(UnitAction::UpgradeTo)
	{
	}

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_UpgradeTo>(*this);
	}

	virtual bool IsValid() const override;

	virtual void Save(CFile &file, const CUnit &unit) const override;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) override;

	virtual void Execute(CUnit &unit) override;
	virtual void Cancel(CUnit &unit) override;
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void UpdatePathFinderData(PathFinderInput &input) override { UpdatePathFinderData_NotCalled(input); }

	virtual void UpdateUnitVariables(CUnit &unit) const override;
	
	//Wyrmgus start
	void ConvertUnitType(const CUnit &unit, wyrmgus::unit_type &newType);
	//Wyrmgus end
	
	const wyrmgus::unit_type &GetUnitType() const { return *Type; }

private:
	const wyrmgus::unit_type *Type = nullptr; /// upgrade to this unit-type
	int Ticks = 0;       /// Ticks to complete
};

extern int TransformUnitIntoType(CUnit &unit, const wyrmgus::unit_type &newtype);
