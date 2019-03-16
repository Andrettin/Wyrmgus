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

#ifndef __ACTION_RESOURCE_H__
#define __ACTION_RESOURCE_H__

#include "action/actions.h"

class COrder_Resource : public COrder
{
	//Wyrmgus start
//	friend COrder *COrder::NewActionResource(CUnit &harvester, const Vec2i &pos);
	friend COrder *COrder::NewActionResource(CUnit &harvester, const Vec2i &pos, int z);
	//Wyrmgus end
	friend COrder *COrder::NewActionResource(CUnit &harvester, CUnit &mine);
	friend COrder *COrder::NewActionReturnGoods(CUnit &harvester, CUnit *depot);

public:
	COrder_Resource(CUnit &harvester) : COrder(UnitActionResource), worker(&harvester)
	{
		Resource.Pos.x = Resource.Pos.y = -1;
		//Wyrmgus start
		Resource.MapLayer = -1;
		//Wyrmgus end
	}

	~COrder_Resource();

	virtual COrder_Resource *Clone() const { return new COrder_Resource(*this); }

	virtual bool IsValid() const;

	virtual void Save(CFile &file, const CUnit &unit) const;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit);

	virtual void Execute(CUnit &unit);
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos) const;
	virtual void UpdatePathFinderData(PathFinderInput &input);
	virtual bool OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/);


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
	int StopGathering(CUnit &unit);
	int MoveToDepot(CUnit &unit);
	bool WaitInDepot(CUnit &unit);
	void DropResource(CUnit &unit);
	void ResourceGiveUp(CUnit &unit);
	bool FindAnotherResource(CUnit &unit);
	bool ActionResourceInit(CUnit &unit);
	
private:
	CUnitPtr worker;	/// unit that own this order.
	unsigned char CurrentResource = 0;
	struct {
		Vec2i Pos;	/// position for terrain resource.
		//Wyrmgus start
		int MapLayer;
		//Wyrmgus end
		CUnitPtr Mine;
	} Resource;
	CUnitPtr Depot;
	int State = 0;
	int TimeToHarvest = 0;			/// how much time until we harvest some more.
	bool DoneHarvesting = false;	/// Harvesting done, wait for action to break.
	int Range = 0;
#if 1
	// duplicate of Resource.Pos ?
	Vec2i goalPos = Vec2i(-1, -1);
	//Wyrmgus start
	int MapLayer = 0;
	//Wyrmgus end
#endif
};

#endif
