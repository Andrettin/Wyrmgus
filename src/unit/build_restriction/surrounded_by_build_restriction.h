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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
//		and Andrettin
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

#include "unit/build_restriction/build_restriction.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

class surrounded_by_build_restriction final : public build_restriction
{
public:
	virtual std::unique_ptr<build_restriction> duplicate() const override
	{
		auto b = std::make_unique<surrounded_by_build_restriction>();
		b->Distance = this->Distance;
		b->DistanceType = this->DistanceType;
		b->Count = this->Count;
		b->CountType = this->CountType;
		b->RestrictTypeName = this->RestrictTypeName;
		b->RestrictTypeOwner = this->RestrictTypeOwner;
		b->RestrictType = this->RestrictType;
		b->CheckBuilder = this->CheckBuilder;
		return b;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const gsml_operator property_operator = property.get_operator();
		const std::string &value = property.get_value();

		if (key == "type") {
			this->RestrictTypeName = value;
		} else if (key == "count") {
			this->Count = std::stoi(value);

			switch (property_operator) {
				case gsml_operator::equality:
					this->CountType = DistanceTypeType::Equal;
					break;
				case gsml_operator::inequality:
					this->CountType = DistanceTypeType::NotEqual;
					break;
				case gsml_operator::less_than:
					this->CountType = DistanceTypeType::LessThan;
					break;
				case gsml_operator::less_than_or_equality:
					this->CountType = DistanceTypeType::LessThanEqual;
					break;
				case gsml_operator::greater_than:
					this->CountType = DistanceTypeType::GreaterThan;
					break;
				case gsml_operator::assignment:
				case gsml_operator::greater_than_or_equality:
					this->CountType = DistanceTypeType::GreaterThanEqual;
					break;
				default:
					assert_throw(false);
			}
		} else if (key == "distance") {
			this->Distance = std::stoi(value);

			switch (property_operator) {
				case gsml_operator::equality:
					this->DistanceType = DistanceTypeType::Equal;
					break;
				case gsml_operator::inequality:
					this->DistanceType = DistanceTypeType::NotEqual;
					break;
				case gsml_operator::less_than:
					this->DistanceType = DistanceTypeType::LessThan;
					break;
				case gsml_operator::less_than_or_equality:
					this->DistanceType = DistanceTypeType::LessThanEqual;
					break;
				case gsml_operator::greater_than:
					this->DistanceType = DistanceTypeType::GreaterThan;
					break;
				case gsml_operator::assignment:
				case gsml_operator::greater_than_or_equality:
					this->DistanceType = DistanceTypeType::GreaterThanEqual;
					break;
				default:
					assert_throw(false);
			}
		} else if (key == "owner") {
			this->RestrictTypeOwner = value;
		} else if (key == "check_builder") {
			this->CheckBuilder = string::to_bool(value);
		} else {
			build_restriction::process_gsml_property(property);
		}
	}

	virtual void Init() override
	{
		this->RestrictType = unit_type::get(this->RestrictTypeName);
	}

	virtual bool Check(const CUnit *builder, const unit_type &type, const QPoint &pos, CUnit *&ontoptarget, const int z) const override
	{
		Q_UNUSED(ontoptarget);

		Vec2i pos1(0, 0);
		Vec2i pos2(0, 0);
		int distance = 0;
		int count = 0;

		if (this->DistanceType == DistanceTypeType::LessThanEqual
			|| this->DistanceType == DistanceTypeType::GreaterThan
			|| this->DistanceType == DistanceTypeType::Equal
			|| this->DistanceType == DistanceTypeType::NotEqual) {
			pos1.x = std::max<int>(pos.x() - this->Distance, 0);
			pos1.y = std::max<int>(pos.y() - this->Distance, 0);
			pos2.x = std::min<int>(pos.x() + type.get_tile_width() + this->Distance, CMap::get()->Info->MapWidths[z]);
			pos2.y = std::min<int>(pos.y() + type.get_tile_height() + this->Distance, CMap::get()->Info->MapHeights[z]);
			distance = this->Distance;
		} else if (this->DistanceType == DistanceTypeType::LessThan || this->DistanceType == DistanceTypeType::GreaterThanEqual) {
			pos1.x = std::max<int>(pos.x() - this->Distance - 1, 0);
			pos1.y = std::max<int>(pos.y() - this->Distance - 1, 0);
			pos2.x = std::min<int>(pos.x() + type.get_tile_width() + this->Distance + 1, CMap::get()->Info->MapWidths[z]);
			pos2.y = std::min<int>(pos.y() + type.get_tile_height() + this->Distance + 1, CMap::get()->Info->MapHeights[z]);
			distance = this->Distance - 1;
		}
		std::vector<CUnit *> table;
		//Wyrmgus start
	//	Select(pos1, pos2, table);
		Select(pos1, pos2, table, z);
		//Wyrmgus end

		for (size_t i = 0; i != table.size(); ++i) {
			if ((builder != table[i] || this->CheckBuilder) &&
				// unit has RestrictType or no RestrictType was set, but a RestrictTypeOwner
				(this->RestrictType == table[i]->Type || (!this->RestrictType && this->RestrictTypeOwner.size() > 0)) &&
				// RestrictTypeOwner is not set or unit belongs to a suitable player
				(this->RestrictTypeOwner.size() == 0 ||
					(!this->RestrictTypeOwner.compare("self") && builder->Player == table[i]->Player) ||
					(!this->RestrictTypeOwner.compare("allied") && (builder->Player == table[i]->Player || builder->Player->is_allied_with(*table[i]->Player))) ||
					//Wyrmgus start
	//				(!this->RestrictTypeOwner.compare("enemy") && builder->Player->is_enemy_of(*table[i]->Player)))) {
					(!this->RestrictTypeOwner.compare("enemy") && builder->Player->is_enemy_of(*table[i])))) {
				//Wyrmgus end

				switch (this->DistanceType) {
				case DistanceTypeType::GreaterThan:
				case DistanceTypeType::GreaterThanEqual:
					break;
				case DistanceTypeType::LessThan:
				case DistanceTypeType::LessThanEqual:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) <= distance) {
						count++;
					}
					break;
				case DistanceTypeType::Equal:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) == distance) {
						count++;
					}
					break;
				case DistanceTypeType::NotEqual:
					if (MapDistanceBetweenTypes(type, pos, z, *table[i]->Type, table[i]->tilePos, table[i]->MapLayer->ID) == distance) {
						count++;
					}
					break;
				}
			}
		}

		switch (this->CountType) {
		case DistanceTypeType::LessThan: return count < this->Count;
		case DistanceTypeType::LessThanEqual: return count <= this->Count;
		case DistanceTypeType::Equal: return count == this->Count;
		case DistanceTypeType::NotEqual: return count != this->Count;
		case DistanceTypeType::GreaterThanEqual: return count >= this->Count;
		case DistanceTypeType::GreaterThan: return count > this->Count;
		default: return false;
		}
	}

	int Distance = 0;
	DistanceTypeType DistanceType = DistanceTypeType::Equal;
	int Count = 0;
	DistanceTypeType CountType = DistanceTypeType::Equal;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	unit_type *RestrictType = nullptr;
	bool CheckBuilder = false;
};

}
