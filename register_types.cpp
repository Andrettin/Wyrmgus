/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "register_types.h"

#include "age.h"
#include "animation/animation.h"
#include "character.h"
#include "civilization.h"
#include "conversible_color.h"
#include "dynasty.h"
#include "economy/currency.h"
#include "economy/resource.h"
#include "faction.h"
#include "game/trigger.h"
#include "hair_color.h"
#include "item/item_class.h"
#include "language/language.h"
#include "language/word.h"
#include "literary_text.h"
#include "literary_text_page.h"
#include "map/map_template.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "missile/missile_type.h"
#include "player.h"
#include "player_color.h"
#include "quest/achievement.h"
#include "quest/campaign.h"
#include "quest/dialogue.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "school_of_magic.h"
#include "skin_color.h"
#include "species/species.h"
#include "species/species_category.h"
#include "species/species_category_rank.h"
#include "spell/spells.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/timeline.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "time/time_period_schedule.h"
#include "ui/button_level.h"
#include "ui/icon.h"
#include "unit/historical_unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_structs.h"
#include "world/plane.h"
#include "world/world.h"
#include "wyrmgus.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void register_wyrmgus_types()
{
	ClassDB::register_class<Wyrmgus>();
	
	ClassDB::register_virtual_class<DataElement>();
	ClassDB::register_virtual_class<DetailedDataElement>();
	
	ClassDB::register_class<CAchievement>();
	ClassDB::register_class<CAge>();
	ClassDB::register_class<CAnimations>();
	ClassDB::register_class<CButtonLevel>();
	ClassDB::register_class<CCalendar>();
	ClassDB::register_class<CCampaign>();
	ClassDB::register_class<CCharacter>();
	ClassDB::register_class<CCivilization>();
	ClassDB::register_class<Currency>();
	ClassDB::register_class<CDeity>();
	ClassDB::register_class<CDeityDomain>();
	ClassDB::register_class<CDialogue>();
	ClassDB::register_class<CDynasty>();
	ClassDB::register_class<CFaction>();
	ClassDB::register_class<CHistoricalUnit>();
	ClassDB::register_class<CIcon>();
	ClassDB::register_class<CLanguage>();
	ClassDB::register_class<CLiteraryText>();
	ClassDB::register_class<CLiteraryTextPage>();
	ClassDB::register_class<CMapTemplate>();
	ClassDB::register_class<CPantheon>();
	ClassDB::register_class<CPlane>();
	ClassDB::register_class<CPlayer>();
	ClassDB::register_class<CReligion>();
	ClassDB::register_class<CResource>();
	ClassDB::register_class<CSchoolOfMagic>();
	ClassDB::register_class<CSeason>();
	ClassDB::register_class<CSite>();
	ClassDB::register_class<CSpecies>();
	ClassDB::register_class<CSpeciesCategory>();
	ClassDB::register_class<CSpeciesCategoryRank>();
	ClassDB::register_class<CSpell>();
	ClassDB::register_class<CTerrainType>();
	ClassDB::register_class<CTimeline>();
	ClassDB::register_class<CTimeOfDay>();
	ClassDB::register_class<CTrigger>();
	ClassDB::register_class<CUnitType>();
	ClassDB::register_class<CUpgrade>();
	ClassDB::register_class<CWord>();
	ClassDB::register_class<CWorld>();
	ClassDB::register_class<ItemClass>();
	ClassDB::register_class<MissileType>();
	ClassDB::register_class<UnitClass>();
	
	ClassDB::register_virtual_class<CConversibleColor>();
	ClassDB::register_class<CHairColor>();
	ClassDB::register_class<CPlayerColor>();
	ClassDB::register_class<CSkinColor>();
	
	ClassDB::register_virtual_class<CTimePeriodSchedule>();
	ClassDB::register_class<CSeasonSchedule>();
	ClassDB::register_class<CTimeOfDaySchedule>();
}

void unregister_wyrmgus_types()
{
}
