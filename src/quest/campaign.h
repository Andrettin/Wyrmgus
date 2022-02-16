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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "include/data_type.h"
#include "vec2i.h"

class LuaCallback;
struct lua_State;

static int CclDefineCampaign(lua_State *l);
static int CclGetCampaignData(lua_State *l);

namespace wyrmgus {

class calendar;
class faction;
class icon;
class map_presets;
class map_template;
class player_color;
class quest;
class species;
class timeline;

class campaign final : public detailed_data_entry, public data_type<campaign>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(wyrmgus::player_color* player_color MEMBER player_color NOTIFY changed)
	Q_PROPERTY(QDateTime start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(wyrmgus::calendar* start_date_calendar MEMBER start_date_calendar)
	Q_PROPERTY(wyrmgus::timeline* timeline MEMBER timeline NOTIFY changed)
	Q_PROPERTY(wyrmgus::faction* faction MEMBER faction NOTIFY changed)
	Q_PROPERTY(wyrmgus::quest* quest MEMBER quest NOTIFY changed)
	Q_PROPERTY(QVariantList map_templates READ get_map_templates_qvariant_list)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(bool available READ is_available NOTIFY available_changed)
	Q_PROPERTY(wyrmgus::map_presets* map_presets MEMBER map_presets NOTIFY changed)
	Q_PROPERTY(wyrmgus::campaign* tree_parent MEMBER tree_parent NOTIFY changed)

public:
	static constexpr const char *class_identifier = "campaign";
	static constexpr const char *database_folder = "campaigns";

	static void initialize_all();

	static std::vector<const campaign *> get_all_visible()
	{
		std::vector<const campaign *> campaigns;

		for (const campaign *campaign : campaign::get_all()) {
			if (campaign->is_hidden()) {
				continue;
			}

			campaigns.push_back(campaign);
		}

		return campaigns;
	}

public:
	explicit campaign(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	
	const QDateTime &get_start_date() const
	{
		return this->start_date;
	}

	const wyrmgus::timeline *get_timeline() const
	{
		return this->timeline;
	}
	
	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

	wyrmgus::quest *get_quest() const
	{
		return this->quest;
	}

	const species *get_species() const;

	bool is_hidden() const
	{
		if (this->tree_parent != nullptr && this->tree_parent->is_hidden()) {
			return true;
		}

		return this->hidden;
	}

	bool is_available() const;

	bool contains_timeline_date(const wyrmgus::timeline *timeline, const QDateTime &date) const;

	const std::vector<wyrmgus::quest *> &get_required_quests() const
	{
		return this->required_quests;
	}

	const std::vector<map_template *> &get_map_templates() const
	{
		return this->map_templates;
	}

	QVariantList get_map_templates_qvariant_list() const;

	Q_INVOKABLE void add_map_template(map_template *map_template)
	{
		this->map_templates.push_back(map_template);
	}

	Q_INVOKABLE void remove_map_template(map_template *map_template);

	bool is_required_map_template(const map_template *map_template) const
	{
		return this->required_map_templates.contains(map_template);
	}

	const wyrmgus::map_presets *get_map_presets() const
	{
		return this->map_presets;
	}

	virtual named_data_entry *get_tree_parent() const override
	{
		return this->tree_parent;
	}

	virtual bool is_hidden_in_tree() const override
	{
		return this->is_hidden();
	}

	virtual std::vector<const named_data_entry *> get_top_tree_elements() const override
	{
		std::vector<const named_data_entry *> top_tree_elements;
		const std::vector<const campaign *> visible_campaigns = campaign::get_all_visible();
		top_tree_elements.insert(top_tree_elements.end(), visible_campaigns.begin(), visible_campaigns.end());
		return top_tree_elements;
	}

signals:
	void available_changed();
	void changed();

private:
	wyrmgus::icon *icon = nullptr;
	wyrmgus::player_color *player_color = nullptr; //the player color used for the campaign's icon
	QDateTime start_date; //the starting date for the campaign
	calendar *start_date_calendar = nullptr; //the calendar for the start date
	wyrmgus::timeline *timeline = nullptr; //the timeline in which the campaign is set
	bool hidden = false; //whether the campaign is hidden
	bool Sandbox = false;			/// Whether the campaign is a sandbox one
	std::vector<wyrmgus::quest *> required_quests;		/// Quests required by the campaign
	wyrmgus::faction *faction = nullptr;	//which faction the player plays as in the campaign
	wyrmgus::quest *quest = nullptr; //the quest which is acquired when the campaign starts, and which when completed means that the campaign has been completed as well
	std::vector<map_template *> map_templates; //map templates used by the campaign
	std::set<const map_template *> required_map_templates; //required map subtemplates for the campaign
public:
	std::vector<Vec2i> MapSizes;				/// Map sizes
	std::vector<Vec2i> MapTemplateStartPos;		/// Map template position the map will start on
private:
	wyrmgus::map_presets *map_presets = nullptr;
	campaign *tree_parent = nullptr;
	
	friend int ::CclDefineCampaign(lua_State *l);
	friend int ::CclGetCampaignData(lua_State *l);
};

}

extern std::string GetCurrentCampaign();
