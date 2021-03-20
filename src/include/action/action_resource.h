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
/**@name actionresource.h - The actions headerfile. */
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

namespace wyrmgus {
	class resource;
}

class COrder_Resource final : public COrder
{
public:
	explicit COrder_Resource(CUnit &harvester);
	virtual ~COrder_Resource() override;

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_Resource>(*this);
	}

	virtual bool IsValid() const override;

	virtual void Save(CFile &file, const CUnit &unit) const override;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) override;

	virtual void Execute(CUnit &unit) override;
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const override;
	virtual void UpdatePathFinderData(PathFinderInput &input) override;
	virtual bool OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/) override;

	const resource *get_current_resource() const;
	int GetCurrentResource() const { return CurrentResource; }
	Vec2i GetHarvestLocation() const;
	//Wyrmgus start
	int GetHarvestMapLayer() const;
	//Wyrmgus end
	bool IsGatheringStarted() const;
	bool IsGatheringFinished() const;
	bool IsGatheringWaiting() const;
private:
	int MoveToResource_Terrain(CUnit &unit);
	int MoveToResource_Unit(CUnit &unit);
	int MoveToResource(CUnit &unit);
	void UnitGotoGoal(CUnit &unit, CUnit *const goal, int state);
	int StartGathering(CUnit &unit);
	void LoseResource(CUnit &unit, CUnit &source);
	int GatherResource(CUnit &unit);
	bool StopGathering(CUnit &unit);
	int MoveToDepot(CUnit &unit);
	bool WaitInDepot(CUnit &unit);
	void DropResource(CUnit &unit);
	void ResourceGiveUp(CUnit &unit);
	bool FindAnotherResource(CUnit &unit);
	bool ActionResourceInit(CUnit &unit);

	CUnit *get_worker() const;
	CUnit *get_depot() const;

private:
	std::shared_ptr<wyrmgus::unit_ref> worker; /// unit that own this order.
	unsigned char CurrentResource = 0;
	struct Resource {
		CUnit *get_mine() const;

		Vec2i Pos = Vec2i(-1, -1); /// position for terrain resource.
		//Wyrmgus start
		int MapLayer = -1;
		//Wyrmgus end
		std::shared_ptr<wyrmgus::unit_ref> Mine;
	} Resource;
	std::shared_ptr<wyrmgus::unit_ref> Depot;
	int State = 0;
	int TimeToHarvest = 0;          /// how much time until we harvest some more.
	bool DoneHarvesting = false;  /// Harvesting done, wait for action to break.
	int Range = 0;
	// duplicate of Resource.Pos ?
	Vec2i goalPos = Vec2i(-1, -1);
	//Wyrmgus start
	int MapLayer = 0;
	//Wyrmgus end

	//Wyrmgus start
//	friend std::unique_ptr<COrder> COrder::NewActionResource(CUnit &harvester, const Vec2i &pos);
	friend std::unique_ptr<COrder> COrder::NewActionResource(CUnit &harvester, const Vec2i &pos, int z);
	//Wyrmgus end
	friend std::unique_ptr<COrder> COrder::NewActionResource(CUnit &harvester, CUnit &mine);
	friend std::unique_ptr<COrder> COrder::NewActionReturnGoods(CUnit &harvester, CUnit *depot);
};
