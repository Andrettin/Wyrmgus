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
/**@name action_attack.h - The actions headerfile. */
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

class COrder_Attack final : public COrder
{
	friend std::unique_ptr<COrder> COrder::NewActionAttack(const CUnit &attacker, CUnit &target);
	//Wyrmgus start
//	friend std::unique_ptr<COrder> COrder::NewActionAttack(const CUnit &attacker, const Vec2i &dest);
//	friend std::unique_ptr<COrder> COrder::NewActionAttackGround(const CUnit &attacker, const Vec2i &dest);
	friend std::unique_ptr<COrder> COrder::NewActionAttack(const CUnit &attacker, const Vec2i &dest, int z);
	friend std::unique_ptr<COrder> COrder::NewActionAttackGround(const CUnit &attacker, const Vec2i &dest, int z);
	//Wyrmgus end
public:
	explicit COrder_Attack(const bool ground) : COrder(ground ? UnitAction::AttackGround : UnitAction::Attack)
	{
	}

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_Attack>(*this);
	}

	virtual bool IsValid() const override;
	virtual void Save(CFile &file, const CUnit &unit) const override;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) override;

	virtual void Execute(CUnit &unit) override;
	virtual void OnAnimationAttack(CUnit &unit) override;
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos) const override;
	virtual void UpdatePathFinderData(PathFinderInput &input) override;
	virtual bool OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/) override;

	virtual const Vec2i GetGoalPos() const override { return goalPos; }
	//Wyrmgus start
	virtual const int GetGoalMapLayer() const override { return MapLayer; }
	//Wyrmgus end
	bool IsWeakTargetSelected() const;

private:
	bool CheckForDeadGoal(CUnit &unit);
	bool CheckForTargetInRange(CUnit &unit);
	void MoveToTarget(CUnit &unit);
	void AttackTarget(CUnit &unit);

private:
	int State = 0;
	int MinRange = 0;
	int Range = 0;
	Vec2i goalPos = Vec2i(-1, -1);
	//Wyrmgus start
	int MapLayer = 0;
	//Wyrmgus end
};
