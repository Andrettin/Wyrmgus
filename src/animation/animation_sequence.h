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
//      (c) Copyright 2005-2022 by Jimmy Salmon and Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"

class CAnimation;

struct LabelsStruct final
{
	const CAnimation *Anim = nullptr;
	std::string Name;
};

struct LabelsLaterStruct final
{
	const CAnimation **Anim = nullptr;
	std::string Name;
};

namespace wyrmgus {

class animation_sequence final : public data_entry, public data_type<animation_sequence>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "animation_sequence";
	static constexpr const char property_class_identifier[] = "wyrmgus::animation_sequence*";
	static constexpr const char *database_folder = "animation_sequences";

	static void AddAnimationToArray(CAnimation *anim);

	explicit animation_sequence(const std::string &identifier);
	~animation_sequence();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void initialize() override;
	virtual void check() const override;

	void set_animations(std::vector<std::unique_ptr<CAnimation>> &&animations);

	const CAnimation *get_first_animation() const
	{
		return this->animations.front().get();
	}

	void add_label(const CAnimation *anim, const std::string &name);
	const CAnimation *find_label(const std::string &name) const;
	void find_label_later(const CAnimation **anim, const std::string &name);
	void fix_labels();

private:
	std::vector<std::unique_ptr<CAnimation>> animations;
	std::vector<LabelsStruct> labels;
	std::vector<LabelsLaterStruct> labels_later;
};

}
