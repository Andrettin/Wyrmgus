#               _       _
#              ( |     / )  _ ____  __  __ ____ __   _ ____
#              | | _  / /(_/ / _  )/  |/  / __ \ /  / / ___)
#              | |/ |/ /\  // /  // /(// / (_  |(__/ /\__ \
#              (__/|__/ /_//_/ \_\_/  (_/\___ /_____/(_____)
#                                       (____/
#  ______________________                           ______________________
#                        T H E   W A R   B E G I N S
#         Wyrmgus - A free fantasy real time strategy game engine
#
#    CMakeLists.txt
#    Copyright (C) 2011-2021  Pali Rohár <pali.rohar@gmail.com>,
#    cybermind <cybermindid@gmail.com> and Andrettin
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#

cmake_minimum_required(VERSION 3.7.0)

project(wyrmgus VERSION 4.1.5)
add_compile_definitions(
	StratagusMajorVersion=${CMAKE_PROJECT_VERSION_MAJOR}
	StratagusMinorVersion=${CMAKE_PROJECT_VERSION_MINOR}
	StratagusPatchLevel=${CMAKE_PROJECT_VERSION_PATCH}
)

set(STRATAGUS_VERSION ${CMAKE_PROJECT_VERSION})

# Stratagus sources

include_directories(
	src
	src/include
	src/guichan/include
	src/guichan/include/guichan
	src/stratagus
	src/xbrz/include
	${CMAKE_CURRENT_BINARY_DIR}
)

set(action_SRCS
	src/action/action_attack.cpp
	src/action/action_board.cpp
	src/action/action_build.cpp
	src/action/action_built.cpp
	src/action/action_defend.cpp
	src/action/action_die.cpp
	src/action/action_follow.cpp
	src/action/action_move.cpp
	src/action/action_pickup.cpp
	src/action/action_patrol.cpp
	src/action/action_repair.cpp
	src/action/action_research.cpp
	src/action/action_resource.cpp
	src/action/action_spellcast.cpp
	src/action/action_still.cpp
	#Wyrmgus start
	src/action/action_trade.cpp
	#Wyrmgus end
	src/action/action_train.cpp
	src/action/action_unload.cpp
	src/action/action_upgradeto.cpp
	src/action/action_use.cpp
	src/action/actions.cpp
	src/action/command.cpp
)
source_group(action FILES ${action_SRCS})

set(animation_SRCS
	src/animation/animation.cpp
	src/animation/animation_attack.cpp
	src/animation/animation_die.cpp
	src/animation/animation_exactframe.cpp
	src/animation/animation_frame.cpp
	src/animation/animation_goto.cpp
	src/animation/animation_ifvar.cpp
	src/animation/animation_label.cpp
	src/animation/animation_move.cpp
	src/animation/animation_randomgoto.cpp
	src/animation/animation_randomrotate.cpp
	src/animation/animation_randomsound.cpp
	src/animation/animation_randomwait.cpp
	src/animation/animation_rotate.cpp
	src/animation/animation_setvar.cpp
	src/animation/animation_sound.cpp
	src/animation/animation_spawnmissile.cpp
	src/animation/animation_unbreakable.cpp
	src/animation/animation_wait.cpp
)
source_group(animation FILES ${animation_SRCS})

set(ai_SRCS
	src/ai/ai_building.cpp
	src/ai/ai.cpp
	src/ai/ai_force.cpp
	src/ai/ai_force_template.cpp
	src/ai/ai_magic.cpp
	src/ai/ai_plan.cpp
	src/ai/ai_resource.cpp
	src/ai/script_ai.cpp
)
source_group(ai FILES ${ai_SRCS})

set(beos_SRCS
	src/beos/beos.cpp
)
source_group(beos FILES ${beos_SRCS})

set(database_SRCS
	src/database/basic_data_entry.cpp
	src/database/data_entry.cpp
	src/database/data_entry_history.cpp
	src/database/data_module.cpp
	src/database/data_module_container.cpp
	src/database/database.cpp
	src/database/defines.cpp
	src/database/detailed_data_entry.cpp
	src/database/named_data_entry.cpp
	src/database/predefines.cpp
	src/database/preferences.cpp
	src/database/sml_data.cpp
	src/database/sml_parser.cpp
	src/database/sml_property.cpp
)
source_group(database FILES ${database_SRCS})

set(economy_SRCS
	src/economy/resource.cpp
	src/economy/resource_container.cpp
)
source_group(economy FILES ${economy_SRCS})

set(editor_SRCS
	src/editor/editloop.cpp
	src/editor/editor.cpp
	src/editor/edmap.cpp
	src/editor/script_editor.cpp
)
source_group(editor FILES ${editor_SRCS})

set(game_SRCS
	src/game/game.cpp
	src/game/loadgame.cpp
	src/game/replay.cpp
	src/game/savegame.cpp
)
source_group(game FILES ${game_SRCS})

set(guichan_SRCS
	src/guichan/cliprectangle.cpp
	src/guichan/defaultfont.cpp
	src/guichan/exception.cpp
	src/guichan/focushandler.cpp
	src/guichan/gcn_color.cpp
	src/guichan/gfont.cpp
	src/guichan/graphics.cpp
	src/guichan/gui.cpp
	src/guichan/guichan.cpp
	src/guichan/image.cpp
	src/guichan/imagefont.cpp
	src/guichan/key.cpp
	src/guichan/keyinput.cpp
	src/guichan/mouseinput.cpp
#	src/guichan/opengl/opengl.cpp
#	src/guichan/opengl/openglgraphics.cpp
#	src/guichan/opengl/openglimageloader.cpp
	src/guichan/rectangle.cpp
	src/guichan/sdl/gsdl.cpp
	src/guichan/sdl/sdlinput.cpp
	src/guichan/widget.cpp
	src/guichan/widgets/container.cpp
	src/guichan/widgets/dropdown.cpp
	src/guichan/widgets/checkbox.cpp
	src/guichan/widgets/gcn_button.cpp
	src/guichan/widgets/gcn_icon.cpp
	src/guichan/widgets/label.cpp
	src/guichan/widgets/listbox.cpp
	src/guichan/widgets/radiobutton.cpp
	src/guichan/widgets/scrollarea.cpp
	src/guichan/widgets/slider.cpp
	src/guichan/widgets/textbox.cpp
	src/guichan/widgets/textfield.cpp
	src/guichan/widgets/window.cpp
)
source_group(guichan FILES ${guichan_SRCS})

set(item_SRCS
	src/item/item_class.cpp
	src/item/persistent_item.cpp
	src/item/recipe.cpp
	src/item/script_item.cpp
	src/item/unique_item.cpp
)
source_group(item FILES ${item_SRCS})

set(language_SRCS
	src/language/language.cpp
	src/language/word.cpp
)
source_group(language FILES ${language_SRCS})

set(map_SRCS
	src/map/character_substitution.cpp
	src/map/character_unit.cpp
	src/map/historical_location.cpp
	src/map/landmass.cpp
	src/map/landmass_container.cpp
	src/map/map.cpp
	src/map/map_draw.cpp
	src/map/map_fog.cpp
	src/map/map_layer.cpp
	src/map/map_projection.cpp
	src/map/map_radar.cpp
	src/map/map_template.cpp
	src/map/map_template_container.cpp
	src/map/map_wall.cpp
	src/map/minimap.cpp
	src/map/plane.cpp
	src/map/region.cpp
	src/map/script_map.cpp
	src/map/script_tileset.cpp
	src/map/site.cpp
	src/map/site_container.cpp
	src/map/site_game_data.cpp
	src/map/site_history.cpp
	src/map/terrain_feature.cpp
	src/map/terrain_geodata_map.cpp
	src/map/terrain_type.cpp
	src/map/tile.cpp
	src/map/tile_flag.cpp
	src/map/tile_image_provider.cpp
	src/map/tileset.cpp
	src/map/world.cpp
	src/map/world_game_data.cpp
)
source_group(map FILES ${map_SRCS})

set(missile_SRCS
	src/missile/missile.cpp
	src/missile/missile_cliptotarget.cpp
	src/missile/missile_continuous.cpp
	src/missile/missile_cycleonce.cpp
	src/missile/missile_deathcoil.cpp
	src/missile/missile_fire.cpp
	src/missile/missile_flameshield.cpp
	src/missile/missile_hit.cpp
	src/missile/missile_landmine.cpp
	src/missile/missile_none.cpp
	src/missile/missile_parabolic.cpp
	src/missile/missile_pointotpointwithhit.cpp
	src/missile/missile_pointtopoint.cpp
	src/missile/missile_pointtopointbounce.cpp
	src/missile/missile_pointtopointcycleonce.cpp
	src/missile/missile_stay.cpp
	src/missile/missile_straightfly.cpp
	src/missile/missile_tracer.cpp
	src/missile/missile_whirlwind.cpp
	src/missile/missileconfig.cpp
	src/missile/script_missile.cpp
)
source_group(missile FILES ${missile_SRCS})

set(network_SRCS
	src/network/commands.cpp
	src/network/net_lowlevel.cpp
	src/network/net_message.cpp
	src/network/master.cpp
	src/network/netconnect.cpp
	src/network/network.cpp
	src/network/netsockets.cpp
)
source_group(network FILES ${network_SRCS})

set(particle_SRCS
	src/particle/graphicanimation.cpp
	src/particle/chunkparticle.cpp
	src/particle/particlemanager.cpp
	src/particle/radialparticle.cpp
	src/particle/smokeparticle.cpp
	src/particle/staticparticle.cpp
)
source_group(particle FILES ${particle_SRCS})

set(pathfinder_SRCS
	src/pathfinder/astar.cpp
	src/pathfinder/pathfinder.cpp
	src/pathfinder/script_pathfinder.cpp
)
source_group(pathfinder FILES ${pathfinder_SRCS})

set(quest_SRCS
	src/quest/achievement.cpp
	src/quest/campaign.cpp
	src/quest/player_quest_objective.cpp
	src/quest/quest.cpp
	src/quest/script_quest.cpp
)
source_group(quest FILES ${quest_SRCS})

set(quest_objective_SRCS
	src/quest/objective/quest_objective.cpp
)
source_group(quest\\objective FILES ${quest_objective_SRCS})

set(religion_SRCS
	src/religion/deity.cpp
	src/religion/religion.cpp
)
source_group(religion FILES ${religion_SRCS})

set(script_SRCS
	src/script/cheat.cpp
	src/script/context.cpp
	src/script/factor_modifier.cpp
	src/script/trigger.cpp
)
source_group(script FILES ${script_SRCS})

set(script_condition_SRCS
	src/script/condition/condition.cpp
	src/script/condition/scripted_condition.cpp
)
source_group(script\\condition FILES ${script_condition_SRCS})

set(script_effect_SRCS
	src/script/effect/delayed_effect_instance.cpp
	src/script/effect/effect.cpp
	src/script/effect/effect_list.cpp
)
source_group(script\\effect FILES ${script_effect_SRCS})

set(sound_SRCS
	src/sound/game_sound_set.cpp
	src/sound/music.cpp
	src/sound/music_player.cpp
	src/sound/script_sound.cpp
	src/sound/sound.cpp
	src/sound/sound_id.cpp
	src/sound/sound_server.cpp
	src/sound/unitsound.cpp
	src/sound/unit_sound_type.cpp
)
source_group(sound FILES ${sound_SRCS})

set(species_SRCS
	src/species/species.cpp
	src/species/taxon.cpp
	src/species/taxon_base.cpp
)
source_group(species FILES ${species_SRCS})

set(spell_SRCS
	src/spell/script_spell.cpp
	src/spell/spell.cpp
	src/spell/spell_action.cpp
	src/spell/spell_action_adjust_variable.cpp
	src/spell/spell_action_adjust_vitals.cpp
	src/spell/spell_action_spawn_missile.cpp
	src/spell/spell_action_summon.cpp
	src/spell/spell_areaadjustvital.cpp
	src/spell/spell_areabombardment.cpp
	src/spell/spell_capture.cpp
	src/spell/spell_container.cpp
	src/spell/spell_demolish.cpp
	src/spell/spell_polymorph.cpp
	src/spell/spell_retrain.cpp
	src/spell/spell_spawnportal.cpp
	src/spell/spell_teleport.cpp
)
source_group(spell FILES ${spell_SRCS})

set(wyrmgus_base_SRCS
	src/stratagus/age.cpp
	src/stratagus/character.cpp
	src/stratagus/character_history.cpp
	src/stratagus/civilization.cpp
	src/stratagus/civilization_base.cpp
	src/stratagus/civilization_group.cpp
	src/stratagus/civilization_history.cpp
	src/stratagus/config.cpp
	src/stratagus/currency.cpp
	src/stratagus/dialogue.cpp
	src/stratagus/dialogue_node.cpp
	src/stratagus/dialogue_option.cpp
	src/stratagus/dynasty.cpp
	src/stratagus/engine_interface.cpp
	src/stratagus/faction.cpp
	src/stratagus/faction_history.cpp
	src/stratagus/fallback_name_generator.cpp
	src/stratagus/grand_strategy.cpp
	src/stratagus/groups.cpp
	src/stratagus/iolib.cpp
	src/stratagus/literary_text.cpp
	src/stratagus/luacallback.cpp
	src/stratagus/magic_domain.cpp
	src/stratagus/mainloop.cpp
	src/stratagus/mod.cpp
	src/stratagus/name_generator.cpp
	src/stratagus/parameters.cpp
	src/stratagus/player.cpp
	src/stratagus/player_color.cpp
	src/stratagus/province.cpp
	src/stratagus/script.cpp
	src/stratagus/script_character.cpp
	src/stratagus/script_grand_strategy.cpp
	src/stratagus/script_literary_text.cpp
	src/stratagus/script_player.cpp
	src/stratagus/script_province.cpp
	src/stratagus/selection.cpp
	src/stratagus/stratagus.cpp
	src/stratagus/text_processor.cpp
	src/stratagus/title.cpp
	src/stratagus/translate.cpp
)
source_group(wyrmgus FILES ${wyrmgus_base_SRCS})

set(time_SRCS
	src/time/calendar.cpp
	src/time/date.cpp
	src/time/season.cpp
	src/time/season_schedule.cpp
	src/time/time_of_day.cpp
	src/time/time_of_day_schedule.cpp
	src/time/time_period_schedule.cpp
)
source_group(time FILES ${time_SRCS})

set(ui_SRCS
	src/ui/botpanel.cpp
	src/ui/button.cpp
	src/ui/button_checks.cpp
	src/ui/button_style.cpp
	src/ui/contenttype.cpp
	src/ui/cursor.cpp
	src/ui/icon.cpp
	src/ui/icon_base.cpp
	src/ui/interface.cpp
	src/ui/interface_image_provider.cpp
	src/ui/interface_style.cpp
	src/ui/mainscr.cpp
	src/ui/uibuttons_proc.cpp
	src/ui/mouse.cpp
	src/ui/popup.cpp
	src/ui/script_ui.cpp
	src/ui/statusline.cpp
	src/ui/ui.cpp
	src/ui/uitimer.cpp
	src/ui/widgets.cpp
)
source_group(ui FILES ${ui_SRCS})

set(unit_SRCS
	src/unit/build.cpp
	src/unit/construction.cpp
	src/unit/historical_unit.cpp
	src/unit/historical_unit_history.cpp
	src/unit/script_unit.cpp
	src/unit/script_unit_type.cpp
	src/unit/unit_cache.cpp
	src/unit/unit.cpp
	src/unit/unit_class.cpp
	src/unit/unit_class_container.cpp
	src/unit/unit_draw.cpp
	src/unit/unit_find.cpp
	src/unit/unit_manager.cpp
	src/unit/unit_ref.cpp
	src/unit/unit_save.cpp
	src/unit/unit_type_container.cpp
	src/unit/unit_type_variation.cpp
	src/unit/unit_type.cpp
)
source_group(unit FILES ${unit_SRCS})

set(upgrade_SRCS
	src/upgrade/upgrade.cpp
	src/upgrade/upgrade_class.cpp
	src/upgrade/upgrade_container.cpp
	src/upgrade/upgrade_modifier.cpp
)
source_group(upgrade FILES ${upgrade_SRCS})

set(util_SRCS
	src/util/angle_util.cpp
	src/util/astronomy_util.cpp
	src/util/color_container.cpp
	src/util/date_util.cpp
	src/util/exception_util.cpp
	src/util/geocoordinate.cpp
	src/util/geojson_util.cpp
	src/util/geopath_util.cpp
	src/util/georectangle_util.cpp
	src/util/geoshape_util.cpp
	src/util/image_util.cpp
	src/util/number_util.cpp
	src/util/point_container.cpp
	src/util/point_util.cpp
	src/util/random.cpp
	src/util/string_conversion_util.cpp
	src/util/string_util.cpp
	src/util/thread_pool.cpp
	src/util/time_util.cpp
	src/util/util.cpp
)
source_group(util FILES ${util_SRCS})

set(video_SRCS
	src/video/color.cpp
	src/video/font.cpp
	src/video/font_color.cpp
	src/video/frame_buffer_object.cpp
	src/video/graphic.cpp
	src/video/linedraw.cpp
	src/video/png.cpp
	src/video/render_context.cpp
	src/video/renderer.cpp
	src/video/sdl.cpp
	src/video/sprite.cpp
	src/video/video.cpp
)
source_group(video FILES ${video_SRCS})

set(xbrz_SRCS
	src/xbrz/xbrz.cpp
)
source_group(xbrz FILES ${xbrz_SRCS})

set(win32_SRCS
	src/win32/SetupConsole_win32.cpp
	src/win32/stratagus.rc
)
source_group(win32 FILES ${win32_SRCS})

set(tolua_FILES
	src/tolua/ai.pkg
	src/tolua/campaign.pkg
	src/tolua/editor.pkg
	src/tolua/font.pkg
	src/tolua/game.pkg
	#Wyrmgus start
	src/tolua/grand_strategy.pkg
	#Wyrmgus end
	src/tolua/map.pkg
	src/tolua/minimap.pkg
	src/tolua/mod.pkg
	src/tolua/network.pkg
	src/tolua/pathfinder.pkg
	src/tolua/player.pkg
	src/tolua/sound.pkg
	src/tolua/stratagus.pkg
	src/tolua/translate.pkg
	src/tolua/trigger.pkg
	src/tolua/ui.pkg
	src/tolua/unit.pkg
	src/tolua/unit_type.pkg
	src/tolua/upgrade.pkg
	src/tolua/video.pkg
	src/tolua/stratagus.lua
)
source_group(tolua FILES ${tolua_FILES})

set(wyrmgus_SRCS
	${action_SRCS}
	${animation_SRCS}
	${ai_SRCS}
	${database_SRCS}
	${economy_SRCS}
	${editor_SRCS}
	${game_SRCS}
	${guichan_SRCS}
	${item_SRCS}
	${language_SRCS}
	${map_SRCS}
	${missile_SRCS}
	${network_SRCS}
	${particle_SRCS}
	${pathfinder_SRCS}
	${quest_SRCS}
	${quest_objective_SRCS}
	${religion_SRCS}
	${script_SRCS}
	${script_condition_SRCS}
	${script_effect_SRCS}
	${sound_SRCS}
	${species_SRCS}
	${spell_SRCS}
	${wyrmgus_base_SRCS}
	${time_SRCS}
	${ui_SRCS}
	${unit_SRCS}
	${upgrade_SRCS}
	${util_SRCS}
	${video_SRCS}
	${xbrz_SRCS}
	${CMAKE_CURRENT_BINARY_DIR}/tolua.cpp
)

set(wyrmgus_guichan_HDRS
	src/guichan/include/guichan/actionlistener.h
	src/guichan/include/guichan/allegro.h
	src/guichan/include/guichan/basiccontainer.h
	src/guichan/include/guichan/cliprectangle.h
	src/guichan/include/guichan/color.h
	src/guichan/include/guichan/defaultfont.h
	src/guichan/include/guichan/exception.h
	src/guichan/include/guichan/focushandler.h
	src/guichan/include/guichan/font.h
	src/guichan/include/guichan/graphics.h
	src/guichan/include/guichan/gsdl.h
	src/guichan/include/guichan/gui.h
	src/guichan/include/guichan.h
	src/guichan/include/guichan/imagefont.h
	src/guichan/include/guichan/image.h
	src/guichan/include/guichan/imageloader.h
	src/guichan/include/guichan/input.h
	src/guichan/include/guichan/key.h
	src/guichan/include/guichan/keyinput.h
	src/guichan/include/guichan/keylistener.h
	src/guichan/include/guichan/listmodel.h
	src/guichan/include/guichan/mouseinput.h
	src/guichan/include/guichan/mouselistener.h
	src/guichan/include/guichan/platform.h
	src/guichan/include/guichan/rectangle.h
	src/guichan/include/guichan/sdl/sdlinput.h
	src/guichan/include/guichan/widget.h
	src/guichan/include/guichan/widgets/button.h
	src/guichan/include/guichan/widgets/container.h
	src/guichan/include/guichan/widgets/dropdown.h
	src/guichan/include/guichan/widgets/checkbox.h
	src/guichan/include/guichan/widgets/icon.h
	src/guichan/include/guichan/widgets/label.h
	src/guichan/include/guichan/widgets/listbox.h
	src/guichan/include/guichan/widgets/radiobutton.h
	src/guichan/include/guichan/widgets/scrollarea.h
	src/guichan/include/guichan/widgets/slider.h
	src/guichan/include/guichan/widgets/textbox.h
	src/guichan/include/guichan/widgets/textfield.h
	src/guichan/include/guichan/widgets/window.h
	src/guichan/include/guichan/x.h
)

set(wyrmgus_action_HDRS
	src/include/action/action_attack.h
	src/include/action/action_board.h
	src/include/action/action_build.h
	src/include/action/action_built.h
	src/include/action/action_defend.h
	src/include/action/action_die.h
	src/include/action/action_follow.h
	src/include/action/action_move.h
	src/include/action/action_patrol.h
	src/include/action/action_pickup.h
	src/include/action/action_repair.h
	src/include/action/action_research.h
	src/include/action/action_resource.h
	src/include/action/action_spellcast.h
	src/include/action/action_still.h
	#Wyrmgus start
	src/include/action/action_trade.h
	#Wyrmgus end
	src/include/action/action_train.h
	src/include/action/action_unload.h
	src/include/action/action_upgradeto.h
	src/include/action/action_use.h
)

set(wyrmgus_ai_HDRS
	src/ai/ai_force_template.h
	src/ai/ai_force_type.h
	src/ai/ai_local.h
)

set(wyrmgus_animation_HDRS
	src/include/animation/animation_attack.h
	src/include/animation/animation_die.h
	src/include/animation/animation_exactframe.h
	src/include/animation/animation_frame.h
	src/include/animation/animation_goto.h
	src/include/animation/animation_ifvar.h
	src/include/animation/animation_label.h
	src/include/animation/animation_move.h
	src/include/animation/animation_randomgoto.h
	src/include/animation/animation_randomrotate.h
	src/include/animation/animation_randomsound.h
	src/include/animation/animation_randomwait.h
	src/include/animation/animation_rotate.h
	src/include/animation/animation_setvar.h
	src/include/animation/animation_sound.h
	src/include/animation/animation_spawnmissile.h
	src/include/animation/animation_unbreakable.h
	src/include/animation/animation_wait.h
)

set(wyrmgus_database_HDRS
	src/database/basic_data_entry.h
	src/database/data_entry.h
	src/database/data_entry_history.h
	src/database/data_module.h
	src/database/data_module_container.h
	src/database/data_type.h
	src/database/data_type_metadata.h
	src/database/database.h
	src/database/defines.h
	src/database/detailed_data_entry.h
	src/database/named_data_entry.h
	src/database/predefines.h
	src/database/preferences.h
	src/database/sml_data.h
	src/database/sml_data_visitor.h
	src/database/sml_element_visitor.h
	src/database/sml_operator.h
	src/database/sml_parser.h
	src/database/sml_property.h
	src/database/sml_property_visitor.h
)

set(wyrmgus_economy_HDRS
	src/economy/resource.h
	src/economy/resource_container.h
	src/economy/resource_storage_type.h
)

set(wyrmgus_item_HDRS
	src/item/item_class.h
	src/item/item_slot.h
	src/item/persistent_item.h
	src/item/recipe.h
	src/item/unique_item.h
)

set(wyrmgus_language_HDRS
	src/language/grammatical_gender.h
	src/language/language.h
	src/language/language_family.h
	src/language/word.h
	src/language/word_type.h
)

set(wyrmgus_map_HDRS
	src/map/character_substitution.h
	src/map/character_unit.h
	src/map/equirectangular_map_projection.h
	src/map/historical_location.h
	src/map/landmass.h
	src/map/landmass_container.h
	src/map/map.h
	src/map/map_layer.h
	src/map/map_projection.h
	src/map/map_template.h
	src/map/map_template_container.h
	src/map/map_template_history.h
	src/map/minimap.h
	src/map/minimap_mode.h
	src/map/plane.h
	src/map/region.h
	src/map/site.h
	src/map/site_container.h
	src/map/site_game_data.h
	src/map/site_history.h
	src/map/terrain_feature.h
	src/map/terrain_geodata_map.h
	src/map/terrain_type.h
	src/map/tile.h
	src/map/tile_flag.h
	src/map/tile_image_provider.h
	src/map/tileset.h
	src/map/world.h
	src/map/world_game_data.h
)

set(wyrmgus_missile_HDRS
	src/missile/missile_class.h
)

set(wyrmgus_quest_HDRS
	src/quest/achievement.h
	src/quest/campaign.h
	src/quest/objective_type.h
	src/quest/player_quest_objective.h
	src/quest/quest.h
)

set(wyrmgus_quest_objective_HDRS
	src/quest/objective/build_units_objective.h
	src/quest/objective/destroy_faction_objective.h
	src/quest/objective/destroy_hero_objective.h
	src/quest/objective/destroy_unique_objective.h
	src/quest/objective/destroy_unit_objective_base.h
	src/quest/objective/destroy_units_objective.h
	src/quest/objective/gather_resource_objective.h
	src/quest/objective/have_resource_objective.h
	src/quest/objective/hero_must_survive_objective.h
	src/quest/objective/quest_objective.h
	src/quest/objective/recruit_hero_objective.h
	src/quest/objective/research_upgrade_objective.h
)

set(wyrmgus_religion_HDRS
	src/include/religion/deity.h
	src/include/religion/pantheon.h
	src/include/religion/religion.h
)

set(wyrmgus_script_HDRS
	src/script/cheat.h
	src/script/context.h
	src/script/factor_modifier.h
	src/script/trigger.h
)

set(wyrmgus_script_condition_HDRS
	src/script/condition/age_condition.h
	src/script/condition/and_condition.h
	src/script/condition/any_other_player_condition.h
	src/script/condition/any_player_condition.h
	src/script/condition/any_unit_condition.h
	src/script/condition/any_unit_of_class_condition.h
	src/script/condition/any_unit_of_type_condition.h
	src/script/condition/can_accept_quest_condition.h
	src/script/condition/character_condition.h
	src/script/condition/character_exists_condition.h
	src/script/condition/civilization_condition.h
	src/script/condition/civilization_group_condition.h
	src/script/condition/coastal_condition.h
	src/script/condition/completed_quest_condition.h
	src/script/condition/condition.h
	src/script/condition/dynasty_condition.h
	src/script/condition/equipment_condition.h
	src/script/condition/faction_condition.h
	src/script/condition/location_condition.h
	src/script/condition/neutral_player_condition.h
	src/script/condition/not_condition.h
	src/script/condition/or_condition.h
	src/script/condition/quest_condition.h
	src/script/condition/random_condition.h
	src/script/condition/real_day_condition.h
	src/script/condition/real_month_condition.h
	src/script/condition/scope_condition.h
	src/script/condition/scope_condition_base.h
	src/script/condition/scripted_condition.h
	src/script/condition/scripted_condition_condition.h
	src/script/condition/season_condition.h
	src/script/condition/settlement_condition.h
	src/script/condition/terrain_condition.h
	src/script/condition/time_of_day_condition.h
	src/script/condition/trigger_condition.h
	src/script/condition/unit_class_condition.h
	src/script/condition/unit_type_condition.h
	src/script/condition/upgrade_class_condition.h
	src/script/condition/upgrade_condition.h
	src/script/condition/war_condition.h
)

set(wyrmgus_script_effect_HDRS
	src/script/effect/accept_quest_effect.h
	src/script/effect/any_unit_of_type_effect.h
	src/script/effect/call_dialogue_effect.h
	src/script/effect/create_unit_effect.h
	src/script/effect/delayed_effect.h
	src/script/effect/delayed_effect_instance.h
	src/script/effect/effect.h
	src/script/effect/effect_list.h
	src/script/effect/hidden_effect.h
	src/script/effect/if_effect.h
	src/script/effect/last_created_unit_effect.h
	src/script/effect/neutral_player_effect.h
	src/script/effect/random_effect.h
	src/script/effect/random_list_effect.h
	src/script/effect/remove_character_effect.h
	src/script/effect/remove_unit_effect.h
	src/script/effect/resource_effect.h
	src/script/effect/scope_effect.h
	src/script/effect/scope_effect_base.h
	src/script/effect/scripted_effect.h
	src/script/effect/scripted_effect_effect.h
)

set(wyrmgus_sound_HDRS
	src/sound/game_sound_set.h
	src/sound/music.h
	src/sound/music_player.h
	src/sound/music_sample.h
	src/sound/music_type.h
	src/sound/sample.h
	src/sound/script_sound.h
	src/sound/sound.h
	src/sound/sound_server.h
	src/sound/unitsound.h
	src/sound/unit_sound_type.h
)

set(wyrmgus_species_HDRS
	src/species/geological_era.h
	src/species/species.h
	src/species/taxon.h
	src/species/taxon_base.h
	src/species/taxonomic_rank.h
)

set(wyrmgus_spell_HDRS
	src/spell/spell.h
	src/spell/spell_action.h
	src/spell/spell_action_adjust_variable.h
	src/spell/spell_action_adjust_vitals.h
	src/spell/spell_action_spawn_missile.h
	src/spell/spell_action_summon.h
	src/spell/spell_areaadjustvital.h
	src/spell/spell_areabombardment.h
	src/spell/spell_capture.h
	src/spell/spell_container.h
	src/spell/spell_demolish.h
	src/spell/spell_polymorph.h
	src/spell/spell_retrain.h
	src/spell/spell_spawnportal.h
	src/spell/spell_target_type.h
	src/spell/spell_teleport.h
)

set(wyrmgus_time_HDRS
	src/time/calendar.h
	src/time/date.h
	src/time/season.h
	src/time/season_schedule.h
	src/time/time_of_day.h
	src/time/time_of_day_schedule.h
	src/time/time_period_schedule.h
	src/time/timeline.h
)

set(wyrmgus_ui_HDRS
	src/ui/button.h
	src/ui/button_cmd.h
	src/ui/button_level.h
	src/ui/button_state.h
	src/ui/button_style.h
	src/ui/contenttype.h
	src/ui/cursor.h
	src/ui/cursor_type.h
	src/ui/icon.h
	src/ui/icon_base.h
	src/ui/interface.h
	src/ui/interface_element_type.h
	src/ui/interface_image_provider.h
	src/ui/interface_style.h
	src/ui/popup.h
	src/ui/resource_icon.h
	src/ui/statusline.h
	src/ui/ui.h
	src/ui/uitimer.h
)

set(wyrmgus_unit_HDRS
	src/unit/construction.h
	src/unit/historical_unit.h
	src/unit/historical_unit_history.h
	src/unit/unit.h
	src/unit/unit_cache.h
	src/unit/unit_class.h
	src/unit/unit_class_container.h
	src/unit/unit_find.h
	src/unit/unit_manager.h
	src/unit/unit_ref.h
	src/unit/unit_type_variation.h
	src/unit/unit_type.h
	src/unit/unit_type_container.h
	src/unit/unit_type_type.h
	src/unit/unit_variable.h
)

set(wyrmgus_upgrade_HDRS
	src/include/upgrade/upgrade.h
	src/upgrade/upgrade_class.h
	src/upgrade/upgrade_container.h
	src/include/upgrade/upgrade_modifier.h
	src/include/upgrade/upgrade_structs.h
)

set(wyrmgus_util_HDRS
	src/util/angle_util.h
	src/util/astronomy_util.h
	src/util/char_util.h
	src/util/color_container.h
	src/util/container_util.h
	src/util/date_util.h
	src/util/enum_util.h
	src/util/exception_util.h
	src/util/fractional_int.h
	src/util/geocoordinate.h
	src/util/geocoordinate_util.h
	src/util/geojson_util.h
	src/util/geopath_util.h
	src/util/georectangle_util.h
	src/util/geoshape_util.h
	src/util/grid_model.h
	src/util/image_util.h
	src/util/list_util.h
	src/util/log_util.h
	src/util/map_util.h
	src/util/number_util.h
	src/util/point_container.h
	src/util/point_operators.h
	src/util/point_util.h
	src/util/queue_util.h
	src/util/qunique_ptr.h
	src/util/random.h
	src/util/rect_util.h
	src/util/set_util.h
	src/util/singleton.h
	src/util/size_operators.h
	src/util/size_util.h
	src/util/string_conversion_util.h
	src/util/string_util.h
	src/util/string_view_util.h
	src/util/thread_pool.h
	src/util/time_util.h
	src/util/type_traits.h
	src/util/util.h
	src/util/vector_random_util.h
	src/util/vector_util.h
)

set(wyrmgus_video_HDRS
	src/video/font.h
	src/video/font_color.h
	src/video/frame_buffer_object.h
	src/video/intern_video.h
	src/video/render_context.h
	src/video/renderer.h
	src/video/video.h
)

set(wyrmgus_xbrz_HDRS
	src/xbrz/include/config.h
	src/xbrz/include/xbrz.h
)

set(wyrmgus_generic_HDRS
	src/include/actions.h
	src/include/age.h
	src/include/ai.h
	src/include/animation.h
	src/include/character.h
	src/include/civilization.h
	src/include/color.h
	src/include/commands.h
	src/include/config.h
	src/include/currency.h
	src/include/data_type.h
	src/include/editor.h
	src/include/game.h
	src/include/grand_strategy.h
	src/include/iocompat.h
	src/include/iolib.h
	src/include/luacallback.h
	src/include/master.h
	src/include/menus.h
	src/include/missile.h
	src/include/missileconfig.h
	src/include/mod.h
	src/include/myendian.h
	src/include/net_lowlevel.h
	src/include/net_message.h
	src/include/netconnect.h
	src/include/network.h
	src/include/network/netsockets.h
	src/include/particle.h
	src/include/pathfinder.h
	src/include/player.h
	src/include/player_color.h
	src/include/province.h
	src/include/replay.h
	src/include/results.h
	src/include/script.h
	src/include/settings.h
	src/include/stratagus.h
	src/include/title.h
	src/include/translate.h
	src/include/vec2i.h
	src/include/version.h
	src/include/viewport.h
	src/include/widgets.h
	src/include/SetupConsole_win32.h
	src/stratagus/civilization_base.h
	src/stratagus/character_history.h
	src/stratagus/civilization_history.h
	src/stratagus/civilization_group.h
	src/stratagus/civilization_group_rank.h
	src/stratagus/dialogue.h
	src/stratagus/dialogue_node.h
	src/stratagus/dialogue_option.h
	src/stratagus/diplomacy_state.h
	src/stratagus/dynasty.h
	src/stratagus/engine_interface.h
	src/stratagus/faction.h
	src/stratagus/faction_history.h
	src/stratagus/faction_tier.h
	src/stratagus/fallback_name_generator.h
	src/stratagus/gender.h
	src/stratagus/government_type.h
	src/stratagus/literary_text.h
	src/stratagus/magic_domain.h
	src/stratagus/name_generator.h
	src/stratagus/parameters.h
	src/stratagus/player_container.h
	src/stratagus/text_processing_context.h
	src/stratagus/text_processor.h
	src/stratagus/vassalage_type.h
)

source_group(wyrmgus FILES ${wyrmgus_generic_HDRS})
source_group(action FILES ${wyrmgus_action_HDRS})
source_group(ai FILES ${wyrmgus_ai_HDRS})
source_group(animation FILES ${wyrmgus_animation_HDRS})
source_group(database FILES ${wyrmgus_database_HDRS})
source_group(guichan FILES ${wyrmgus_guichan_HDRS})
source_group(economy FILES ${wyrmgus_economy_HDRS})
source_group(item FILES ${wyrmgus_item_HDRS})
source_group(language FILES ${wyrmgus_language_HDRS})
source_group(map FILES ${wyrmgus_map_HDRS})
source_group(missile FILES ${wyrmgus_missile_HDRS})
source_group(quest FILES ${wyrmgus_quest_HDRS})
source_group(quest\\objective FILES ${wyrmgus_quest_objective_HDRS})
source_group(religion FILES ${wyrmgus_religion_HDRS})
source_group(script FILES ${wyrmgus_script_HDRS})
source_group(script\\condition FILES ${wyrmgus_script_condition_HDRS})
source_group(script\\effect FILES ${wyrmgus_script_effect_HDRS})
source_group(sound FILES ${wyrmgus_sound_HDRS})
source_group(species FILES ${wyrmgus_species_HDRS})
source_group(spell FILES ${wyrmgus_spell_HDRS})
source_group(time FILES ${wyrmgus_time_HDRS})
source_group(ui FILES ${wyrmgus_ui_HDRS})
source_group(unit FILES ${wyrmgus_unit_HDRS})
source_group(upgrade FILES ${wyrmgus_upgrade_HDRS})
source_group(util FILES ${wyrmgus_util_HDRS})
source_group(video FILES ${wyrmgus_video_HDRS})
source_group(xbrz FILES ${wyrmgus_xbrz_HDRS})

set(wyrmgus_HDRS
	${wyrmgus_generic_HDRS}
	${wyrmgus_action_HDRS}
	${wyrmgus_ai_HDRS}
	${wyrmgus_animation_HDRS}
	${wyrmgus_database_HDRS}
	${wyrmgus_economy_HDRS}
	${wyrmgus_guichan_HDRS}
	${wyrmgus_item_HDRS}
	${wyrmgus_language_HDRS}
	${wyrmgus_map_HDRS}
	${wyrmgus_missile_HDRS}
	${wyrmgus_quest_HDRS}
	${wyrmgus_quest_objective_HDRS}
	${wyrmgus_religion_HDRS}
	${wyrmgus_script_HDRS}
	${wyrmgus_script_condition_HDRS}
	${wyrmgus_script_effect_HDRS}
	${wyrmgus_sound_HDRS}
	${wyrmgus_species_HDRS}
	${wyrmgus_spell_HDRS}
	${wyrmgus_time_HDRS}
	${wyrmgus_ui_HDRS}
	${wyrmgus_unit_HDRS}
	${wyrmgus_upgrade_HDRS}
	${wyrmgus_util_HDRS}
	${wyrmgus_video_HDRS}
	${wyrmgus_xbrz_HDRS}
)

set(wyrmgus_main_SRCS
	src/stratagus/main.cpp
)

set(economy_test_SRCS
	test/economy/resource_test.cpp
)
source_group(economy FILES ${economy_test_SRCS})

set(util_test_SRCS
	test/util/angle_test.cpp
	test/util/astronomy_test.cpp
	test/util/geocoordinate_test.cpp
	test/util/image_test.cpp
	test/util/number_test.cpp
	test/util/string_conversion_test.cpp
	test/util/time_test.cpp
)
source_group(util FILES ${util_test_SRCS})

set(wyrmgus_test_SRCS
	${economy_test_SRCS}
	${util_test_SRCS}
	test/main.cpp
)

# Configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

# Additional platform checks

if(UNIX AND CMAKE_SYSTEM_NAME MATCHES BSD)
	set(BSD true)
endif()

if(UNIX AND CMAKE_SYSTEM_NAME MATCHES Linux)
	set(LINUX true)
endif()

if(WIN32 AND MSVC AND NOT CMAKE_PREFIX_PATH)
	# use a default
	set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies)
endif()

# Find all libraries

if(WIN32)
	option(ENABLE_STATIC "Compile Stratagus as static executable" ON)
else()
	option(ENABLE_STATIC "Compile Stratagus as static executable" OFF)
endif()

if(ENABLE_STATIC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib;.a")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

find_package(Boost 1.69.0 REQUIRED)

#Qt5 modules
#different modules have different licenses, make sure all modules used here are compatible with the GPL 2.0
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.12 COMPONENTS Core REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.12 COMPONENTS Gui REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.12 COMPONENTS Widgets REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.12 COMPONENTS Multimedia REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.12 COMPONENTS Location REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.12 COMPONENTS Qml REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.12 COMPONENTS Quick REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)

find_package(Lua51 REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(Tolua++ REQUIRED)
find_package(ZLIB REQUIRED)

find_package(BZip2)

find_package(X11)
find_package(OpenGL)
find_package(OpenGLES)

find_package(Sqlite)
find_package(Doxygen)
find_package(SelfPackers)

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckSymbolExists)

# Windows RC compiler definitions

if(WIN32)
	enable_language(RC)
	include(CMakeDetermineRCCompiler)

	if(MINGW)
		set(CMAKE_RC_COMPILER_INIT windres)
		set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
	endif()
endif()

# Options for compiling

if(WIN32)
	option(ENABLE_STDIO_REDIRECT "Redirect Stratagus console output to files on Windows" ON)
endif()

option(ENABLE_METASERVER "Build Stratagus metaserver (requires Sqlite3)" OFF)
option(ENABLE_TOUCHSCREEN "Use touchscreen input" OFF)

option(WITH_BZIP2 "Compile Stratagus with BZip2 compression support" OFF)

option(WITH_X11 "Compile Stratagus with X11 clipboard pasting support" ON)

option(ENABLE_DOC "Generate Stratagus source code documentation with Doxygen" OFF)
option(ENABLE_DEV "Install Stratagus game development headers files" OFF)
option(ENABLE_UPX "Compress Stratagus executable binary with UPX packer" OFF)
option(ENABLE_STRIP "Strip all symbols from executables" OFF)
option(ENABLE_USEGAMEDIR "Place all files created by Stratagus (logs, savegames) in the game directory, otherwise place everything in the user directory" ON)
option(ENABLE_MULTIBUILD "Compile Stratagus on all CPU cores simultaneously in MSVC" ON)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
	#unity builds using defined groups require CMake 3.18
	option(ENABLE_UNITY_BUILD "Use unity builds for compilation" ON)
endif()

option(WITH_GEOJSON "Compile with support for generating map data from GeoJSON files" ON)
option(WITH_TEST "Compile the test project" ON)

if(NOT WITH_RENDERER)
	if(OPENGL_FOUND)
		set(WITH_RENDERER "OpenGL")
	else()
		set(WITH_RENDERER "NativeSDL")
	endif()
endif()

# Binary name
set(BINARY_NAME "wyrmgus" CACHE PATH "Sets the name of the binary.")

add_definitions(-DBINARY_NAME=\"${BINARY_NAME}\")

# Install paths
set(BINDIR "bin" CACHE PATH "Where to install user binaries")
set(SBINDIR "sbin" CACHE PATH "Where to install system binaries")
set(GAMEDIR "games" CACHE PATH "Where to install games binaries")
set(DOCDIR "share/doc/stratagus" CACHE PATH "Sets the doc directory to a non-default location.")
set(MANDIR "share/man/man6" CACHE PATH "Sets the man directory to a non-default location.")
set(PIXMAPSDIR "share/pixmaps" CACHE PATH "Sets the pixmaps directory to a non-default location.")
set(STRATAGUS_HEADERS "include" CACHE PATH "Where to install stratagus headers.")

if(NOT IS_ABSOLUTE "${PIXMAPSDIR}")
	set(PIXMAPSDIRABS "${CMAKE_INSTALL_PREFIX}/${PIXMAPSDIR}")
else()
	set(PIXMAPSDIRABS "${PIXMAPSDIR}")
endif()

# Stratagus definitions

add_definitions(-DUSE_ZLIB -DPIXMAPS=\"${PIXMAPSDIRABS}\")
include_directories(
	${LUA_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
	${SDL_MIXER_INCLUDE_DIR}
	${TOLUA++_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
)

set(QT_LIBRARIES
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::Multimedia
	Qt5::Location
	Qt5::Qml
	Qt5::Quick
)

if(WITH_GEOJSON)
	add_definitions(-DUSE_GEOJSON)
	set(QT_LIBRARIES ${QT_LIBRARIES} Qt5::LocationPrivate)
endif()

set(wyrmgus_LIBS
	${LUA_LIBRARIES}
	${SDL_LIBRARY}
	${SDL_MIXER_LIBRARY}
	${TOLUA++_LIBRARY}
	${ZLIB_LIBRARIES}
	${QT_LIBRARIES}
)

if(WIN32 AND NOT ENABLE_STDIO_REDIRECT)
	add_definitions(-DNO_STDIO_REDIRECT)
endif()

if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

if(WITH_BZIP2 AND BZIP2_FOUND)
	add_definitions(-DUSE_BZ2LIB ${BZIP2_DEFINITIONS})
	include_directories(${BZIP2_INCLUDE_DIR})
	set(wyrmgus_LIBS ${wyrmgus_LIBS} ${BZIP2_LIBRARIES})
endif()

if(WITH_X11 AND X11_FOUND)
	add_definitions(-DUSE_X11)
	include_directories(${X11_INCLUDE_DIR})
	set(wyrmgus_LIBS ${wyrmgus_LIBS} ${X11_X11_LIB})
endif()

if(WITH_RENDERER STREQUAL "OpenGL" AND OPENGL_FOUND)
	add_definitions(-DUSE_OPENGL)
	include_directories(${OPENGL_INCLUDE_DIR})
	set(wyrmgus_LIBS ${wyrmgus_LIBS} ${OPENGL_LIBRARIES})
	message(STATUS "Using OpenGL")
elseif(NOT WITH_RENDERER STREQUAL "NativeSDL")
	message(FATAL_ERROR "Specified rendering support was not found in system.\nChoose: NativeSDL OpenGL OpenGLES")
endif()

# Platform definitions

if(WIN32)
	add_definitions(-DUSE_WIN32)
	set(wyrmgus_main_SRCS ${wyrmgus_main_SRCS} ${win32_SRCS})
	set(wyrmgus_LIBS ${wyrmgus_LIBS} dsound winmm ws2_32 dbghelp)
endif()

if (WIN32 AND MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE=1 -DNOMINMAX)
endif()

if (WIN32 AND MINGW)
	set(wyrmgus_LIBS ${wyrmgus_LIBS} dxguid)
endif()

if(APPLE)
	add_definitions(-DUSE_MAC)
	set(SDL_LIBRARY ${SDL_LIBRARY} SDLmain)	
endif()

if(BSD)
	add_definitions(-DUSE_BSD)
endif()

if(LINUX)
	add_definitions(-DUSE_LINUX)
endif()

if(BEOS)
	add_definitions(-DUSE_BEOS)
	set(wyrmgus_SRCS ${wyrmgus_SRCS} ${beos_SRCS})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Stratagus needs to have char by default signed
# No idea how to tell this to other compilers
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
endif()
if(CMAKE_COMPILER_IS_GNUC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
endif()
# TODO: Add support for other compilers

check_type_size(errno_t ERRNOT)
check_function_exists("strcpy_s" HAVE_STRCPYS)
check_function_exists("strncpy_s" HAVE_STRNCPYS)
check_function_exists("strcasestr" HAVE_STRCASESTR)
check_function_exists("strnlen" HAVE_STRNLEN)
check_function_exists("getopt" HAVE_GETOPT)

# mingw-w64 does not have strcat_s in any include file, but function symbol in library exists
# so rather check if we have strcat_s in string.h file
check_symbol_exists("strcat_s" "string.h" HAVE_STRCATS)

if(HAVE_ERRNOT)
	add_definitions(-DHAVE_ERRNOT)
endif()

if(HAVE_STRCPYS)
	add_definitions(-DHAVE_STRCPYS)
endif()

if(HAVE_STRNCPYS)
	add_definitions(-DHAVE_STRNCPYS)
endif()

if(HAVE_STRCATS)
	add_definitions(-DHAVE_STRCATS)
endif()

if(HAVE_STRCASESTR)
	add_definitions(-DHAVE_STRCASESTR)
endif()

if(HAVE_STRNLEN)
	add_definitions(-DHAVE_STRNLEN)
endif()

if(HAVE_GETOPT)
	add_definitions(-DHAVE_GETOPT)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

if(ENABLE_STRIP)
	if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
	endif()
	# TODO: Add support for other compilers
endif()

if(ENABLE_USEGAMEDIR)
	add_definitions(-DUSE_GAME_DIR)
endif()

if(ENABLE_TOUCHSCREEN)
	add_definitions(-DUSE_TOUCHSCREEN)
endif()

if(ENABLE_MULTIBUILD)
	if(WIN32 AND MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		message(STATUS "Added parallel build arguments to CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
	endif()
endif()


if(ENABLE_UNITY_BUILD)
	set(CMAKE_UNITY_BUILD ON)
endif()

# Print compile info

message("==================================")

macro(log_package PACKAGE_NAME PACKAGE)
	if(NOT WITH_${PACKAGE})
		message("${PACKAGE_NAME}: Disabled (Enable by param -DWITH_${PACKAGE}=ON)")
	elseif(WITH_${PACKAGE} AND ${PACKAGE}_FOUND)
		message("${PACKAGE_NAME}: Found and enabled (Disable by param -DWITH_${PACKAGE}=OFF)")
	elseif(WITH_${PACKAGE} AND ${PACKAGE}_EMBEDDED)
		message("${PACKAGE_NAME}: Building embedded library (Disable by param -DWITH_${PACKAGE}=OFF, or install system library)")
	else()
		message("${PACKAGE_NAME}: Not found")
	endif()
endmacro()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message("Debug mode: Yes (Disable by param -DCMAKE_BUILD_TYPE=Release)")
else()
	message("Debug mode: No (Enable by param -DCMAKE_BUILD_TYPE=Debug)")
endif()

if(ENABLE_STRIP)
	message("Strip executables: Yes (Disable by param -DENABLE_STRIP=OFF)")
else()
	message("Strip executables: No (Enable by param -DENABLE_STRIP=ON)")
endif()

if(ENABLE_STATIC)
	message("Static linking: Yes (Disable by param -DENABLE_STATIC=OFF)")
else()
	message("Static linking: No (Enable by param -DENABLE_STATIC=ON)")
endif()

if(ENABLE_USEGAMEDIR)
	message("Place game files in: game directory (Place in user directory with -DENABLE_USEGAMEDIR=OFF)")
else()
	message("Place game files in: user directory (Place in game directory with -DENABLE_USEGAMEDIR=ON)")
endif()

if(ENABLE_MULTIBUILD)
	message("Parallel building in MSVC: Yes (Disable by param -DENABLE_MULTIBUILD=OFF)")
else()
	message("Parallel building in MSVC: No (Enable by param -DENABLE_MULTIBUILD=ON)")
endif()

if(WIN32 AND MSVC)
	message("Platform: Windows (MSVC)")
elseif(WIN32 AND MINGW)
	message("Platform: Windows (MinGW)")
elseif(WIN32)
	message("Platform: Windows (Other)")
elseif(APPLE)
	message("Platform: Mac")
elseif(BSD)
	message("Platform: BSD")
elseif(BEOS)
	message("Platform: BEOS")
elseif(LINUX)
	message("Platform: Linux")
else()
	message("Platform: Other/No platform code")
endif()

if(WIN32 AND ENABLE_STDIO_REDIRECT)
	message("Redirect stdio: Yes (Disable by param -DENABLE_STDIO_REDIRECT=OFF)")
elseif(NOT WIN32)
	message("Redirect stdio: Unsupported on this platform")
else()
	message("Redirect stdio: No (Enable by param -DENABLE_STDIO_REDIRECT=ON)")
endif()

if(ENABLE_TOUCHSCREEN)
	message("Touchscreen input: Yes (Disable by param -DENABLE_TOUCHSCREEN=OFF)")
else()
	message("Touchscreen input: No (Enable by param -DENABLE_TOUCHSCREEN=ON)")
endif()

if(ENABLE_METASERVER)
	if(SQLITE_FOUND)
		message("Metaserver: Yes (Disable by param -DENABLE_METASERVER=OFF)")
	else()
		message("Metaserver: No (Sqlite3 dependency not found)")
	endif()
else()
	message("Metaserver: No (Enable by param -DENABLE_METASERVER=ON)")
endif()

if(ENABLE_DOC AND DOXYGEN_FOUND)
	message("Doxygen documentation: Yes (Disable by param -DENABLE_DOC=OFF)")
else()
	message("Doxygen documentation: No (Enable by param -DENABLE_DOC=ON)")
endif()

if(ENABLE_DEV)
	message("Game development files: Yes (Disable by param -DENABLE_DEV=OFF)")
else()
	message("Game development files: No (Enable by param -DENABLE_DEV=ON)")
endif()

if(ENABLE_UPX AND SELF_PACKER_FOR_EXECUTABLE)
	message("Upx packer: Yes (Disable by param -DENABLE_UPX=OFF)")
else()
	message("Upx packer: No (Enable by param -DENABLE_UPX=ON)")
endif()

log_package("Bzip2" "BZIP2")
log_package("X11" "X11")

if(WITH_RENDERER STREQUAL "OpenGL" AND OPENGL_FOUND)
	message("Renderer: OpenGL (Disable by param -DWITH_RENDERER=NativeSDL)")
else()
	message("Renderer: NativeSDL (Enable OpenGL by param -DWITH_RENDERER=OpenGL)")
endif()

message("==================================")

# Compile Stratagus

add_custom_command(OUTPUT tolua.cpp
	COMMAND ${TOLUA++_APP} ARGS -L stratagus.lua -o ${CMAKE_CURRENT_BINARY_DIR}/tolua.cpp stratagus.pkg
	DEPENDS ${tolua_FILES}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/tolua
	VERBATIM
)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/tolua.cpp PROPERTY SKIP_AUTOMOC ON)

add_library(wyrmgus STATIC ${wyrmgus_SRCS} ${wyrmgus_HDRS})
add_executable(wyrmgus_main WIN32 ${wyrmgus_main_SRCS})

if(WITH_TEST)
	add_executable(wyrmgus_test ${wyrmgus_test_SRCS})
	add_test(wyrmgus_test wyrmgus_test)
	enable_testing()
endif()

if (MSVC)
	target_compile_options(wyrmgus PRIVATE /W4 /w44800 /wd4458)
	target_compile_options(wyrmgus_main PRIVATE /W4 /w44800 /wd4458)
	if(WITH_TEST)
		target_compile_options(wyrmgus_test PRIVATE /W4 /w44800 /wd4458)
	endif()
endif()

target_precompile_headers(wyrmgus PRIVATE src/pch.h)

if(ENABLE_UNITY_BUILD)
	set_target_properties(wyrmgus PROPERTIES UNITY_BUILD_MODE GROUP)
	
	set_source_files_properties(${action_SRCS} PROPERTIES UNITY_GROUP "action")
	set_source_files_properties(${animation_SRCS} PROPERTIES UNITY_GROUP "animation")
	set_source_files_properties(${ai_SRCS} PROPERTIES UNITY_GROUP "ai")
	set_source_files_properties(${beos_SRCS} PROPERTIES UNITY_GROUP "beos")
	set_source_files_properties(${database_SRCS} PROPERTIES UNITY_GROUP "database")
	set_source_files_properties(${economy_SRCS} PROPERTIES UNITY_GROUP "economy")
	set_source_files_properties(${editor_SRCS} PROPERTIES UNITY_GROUP "editor")
	set_source_files_properties(${game_SRCS} PROPERTIES UNITY_GROUP "game")
	set_source_files_properties(${guichan_SRCS} PROPERTIES UNITY_GROUP "guichan")
	set_source_files_properties(${item_SRCS} PROPERTIES UNITY_GROUP "item")
	set_source_files_properties(${language_SRCS} PROPERTIES UNITY_GROUP "language")
	set_source_files_properties(${map_SRCS} PROPERTIES UNITY_GROUP "map")
	set_source_files_properties(${missile_SRCS} PROPERTIES UNITY_GROUP "missile")
	set_source_files_properties(${network_SRCS} PROPERTIES UNITY_GROUP "network")
	set_source_files_properties(${particle_SRCS} PROPERTIES UNITY_GROUP "particle")
	set_source_files_properties(${pathfinder_SRCS} PROPERTIES UNITY_GROUP "pathfinder")
	set_source_files_properties(${quest_SRCS} ${quest_objective_SRCS} PROPERTIES UNITY_GROUP "quest")
	set_source_files_properties(${religion_SRCS} PROPERTIES UNITY_GROUP "religion")
	set_source_files_properties(${script_SRCS} PROPERTIES UNITY_GROUP "script")
	set_source_files_properties(${script_condition_SRCS} PROPERTIES UNITY_GROUP "script_condition")
	set_source_files_properties(${script_effect_SRCS} PROPERTIES UNITY_GROUP "script_effect")
	set_source_files_properties(${sound_SRCS} PROPERTIES UNITY_GROUP "sound")
	set_source_files_properties(${species_SRCS} PROPERTIES UNITY_GROUP "species")
	set_source_files_properties(${spell_SRCS} PROPERTIES UNITY_GROUP "spell")
	set_source_files_properties(${wyrmgus_base_SRCS} PROPERTIES UNITY_GROUP "wyrmgus")
	set_source_files_properties(${time_SRCS} PROPERTIES UNITY_GROUP "time")
	set_source_files_properties(${ui_SRCS} PROPERTIES UNITY_GROUP "ui")
	set_source_files_properties(${unit_SRCS} PROPERTIES UNITY_GROUP "unit")
	set_source_files_properties(${upgrade_SRCS} PROPERTIES UNITY_GROUP "upgrade")
	set_source_files_properties(${util_SRCS} PROPERTIES UNITY_GROUP "util")
	set_source_files_properties(${video_SRCS} PROPERTIES UNITY_GROUP "video")
	set_source_files_properties(${xbrz_SRCS} PROPERTIES UNITY_GROUP "xbrz")
	
	if(WITH_TEST)
		set_target_properties(wyrmgus_test PROPERTIES UNITY_BUILD_MODE GROUP)
		set_source_files_properties(${economy_test_SRCS} PROPERTIES UNITY_GROUP "economy_test")
		set_source_files_properties(${util_test_SRCS} PROPERTIES UNITY_GROUP "util_test")
	endif()
endif()

target_link_libraries(wyrmgus ${wyrmgus_LIBS})
if(wyrmgus_LIBS_RELEASE)
	target_link_libraries(wyrmgus ${wyrmgus_LIBS_RELEASE})
endif()
if(wyrmgus_LIBS_DEBUG)
	target_link_libraries(wyrmgus ${wyrmgus_LIBS_DEBUG})
endif()

if(WIN32 AND MINGW AND ENABLE_STATIC)
	set_target_properties(wyrmgus PROPERTIES LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++ -Wl,--stack,10485760")
	set_target_properties(wyrmgus_main PROPERTIES LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++ -Wl,--stack,10485760")
endif()

if(APPLE)
	set_target_properties(wyrmgus PROPERTIES LINK_FLAGS "${LINK_FLAGS} -lSDLmain")
endif()

target_link_libraries(wyrmgus ${wyrmgus_LIBS})

target_precompile_headers(wyrmgus_main REUSE_FROM wyrmgus)
if(WITH_TEST)
	target_precompile_headers(wyrmgus_test REUSE_FROM wyrmgus)
endif()

set_target_properties(wyrmgus_main PROPERTIES OUTPUT_NAME ${BINARY_NAME})

if (MSVC)
	#ignore linker warning due to missing .pdb files, as otherwise a stream of warnings comes from linking external libraries
	set_target_properties(wyrmgus_main PROPERTIES LINK_FLAGS "/ignore:4099")
	if(WITH_TEST)
		set_target_properties(wyrmgus_test PROPERTIES LINK_FLAGS "/ignore:4099")
	endif()
endif()

target_link_libraries(wyrmgus_main LINK_PUBLIC wyrmgus)
if(WITH_TEST)
	target_link_libraries(wyrmgus_test LINK_PUBLIC wyrmgus)
endif()

########### next target ###############

set(metaserver_SRCS
	metaserver/cmd.cpp
	metaserver/db.cpp
	metaserver/games.cpp
	metaserver/main.cpp
	metaserver/netdriver.cpp
	src/network/net_lowlevel.cpp
)

set(metaserver_HDRS
	metaserver/cmd.h
	metaserver/db.h
	metaserver/games.h
	metaserver/netdriver.h
)

source_group(metaserver FILES ${metaserver_SRCS} ${metaserver_HDRS})

if(NOT SQLITE_FOUND)
	set(ENABLE_METASERVER OFF)
endif()

if(ENABLE_METASERVER)
	add_executable(metaserver ${metaserver_SRCS} ${metaserver_HDRS})
	target_link_libraries(metaserver ${SDL_LIBRARY} ${SQLITE_LIBRARIES})
	
	if(WIN32)
		target_link_libraries(metaserver winmm ws2_32)
	endif()
	
	if(WIN32 AND MINGW)
		target_link_libraries(metaserver dxguid)
	endif()

	if(WIN32 AND MINGW AND ENABLE_STATIC)
		set_target_properties(metaserver PROPERTIES LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++")
	endif()
endif()

########### next target ###############

set(gameheaders_HDRS
	gameheaders/stratagus-game-installer.nsi
	gameheaders/stratagus-game-launcher.h
)
source_group(gameheaders FILES ${gameheaders_HDRS})

########### next target ###############

set(doxygen_FILES
	${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	doc/Doxyfile-footer.html
	doc/Doxyfile-header.html
	${wyrmgus_SRCS}
	${wyrmgus_HDRS}
	${metaserver_SRCS}
	${metaserver_HDRS}
	${gameheaders_HDRS}
)

if(ENABLE_DOC AND DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
	add_custom_command(OUTPUT doxygen/index.html
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		DEPENDS ${doxygen_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating Stratagus source code documentation with Doxygen"
		VERBATIM
	)
	add_custom_target(doc ALL DEPENDS doxygen/index.html)
	list(APPEND CLEAN_FILES doxygen)
endif()

########### next target ###############

macro(self_packer PACKER_TARGET)
	get_target_property(PACKER_NAME ${PACKER_TARGET} OUTPUT_NAME)
	if(NOT PACKER_NAME)
		set(PACKER_NAME ${PACKER_TARGET})
	endif()
	add_custom_command(TARGET ${PACKER_TARGET} POST_BUILD
		COMMAND ${SELF_PACKER_FOR_EXECUTABLE}
		ARGS ${SELF_PACKER_FOR_EXECUTABLE_FLAGS} ${PACKER_NAME}${CMAKE_EXECUTABLE_SUFFIX}
		VERBATIM
	)
endmacro()

if(ENABLE_UPX AND SELF_PACKER_FOR_EXECUTABLE)
	self_packer(wyrmgus)
	if(ENABLE_METASERVER)
		self_packer(metaserver)
	endif()
endif()

########### install files ###############

install(TARGETS wyrmgus_main DESTINATION ${GAMEDIR})

if(ENABLE_METASERVER)
	install(TARGETS metaserver DESTINATION ${SBINDIR})
endif()

if(ENABLE_DOC AND DOXYGEN_FOUND)
	install(FILES doc/stratagus.6 DESTINATION ${MANDIR})
	install(FILES
		doc/development.html
		doc/faq.html
		doc/gpl.html
		doc/guichan-copyright.txt
		doc/changelog.html
		doc/index.html
		doc/install.html
		doc/media.html
		doc/metaserver_protocol.txt
		doc/README-SDL.txt
		DESTINATION share/doc/wyrmgus
	)
	install(DIRECTORY doc/graphics doc/scripts ${CMAKE_CURRENT_BINARY_DIR}/doxygen DESTINATION ${DOCDIR})
endif(ENABLE_DOC AND DOXYGEN_FOUND)

if(ENABLE_DEV)
	install(FILES ${gameheaders_HDRS} DESTINATION ${STRATAGUS_HEADERS})
endif()

install(
  FILES "doc/copyright-deb.txt"
  TYPE DOC
  RENAME "${CMAKE_PROJECT_NAME}/copyright"
  OPTIONAL
  COMPONENT deb-package)

########### packages ##################

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_CONTACT "Anton Karmanov <techie@bastardi.net>")
set(
  CPACK_PACKAGE_DESCRIPTION
  "Modified Stratagus engine for Wyrmsun
Wyrmsun is an open-source real-time strategy game which features elements of
mythology, history and fiction.
In the Wyrmsun universe a myriad of inhabited planets exist. Humans dwell on
Earth, while dwarves inhabit Nidavellir and elves nourish the world of Alfheim.
These peoples struggle to carve a place for themselves with their tools of
stone, bronze and iron. And perhaps one day they will meet one another, beyond
the stars...")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION games)
set(CPACK_DEBIAN_PACKAGE_SUGGESTS wyrmsun)

include(CPack)

########### clean files ###############

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEAN_FILES}")
