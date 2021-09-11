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
//      (c) Copyright 1998-2021 by Lutz Sammer, Fabrice Rossi,
//		Jimmy Salmon and Andrettin
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

#include "stratagus.h"

#include "sound/sound.h"

#include "character.h"
#include "database/defines.h"
#include "iolib.h"
#include "missile.h"
#include "player/civilization.h"
#include "sound/sample.h"
#include "sound/sound_server.h"
#include "sound/unit_sound_type.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "util/container_util.h"
#include "util/vector_util.h"
#include "video/video.h"

/**
**  Selection handling
*/
struct SelectionHandling {
	Origin Source;         /// origin of the sound
	wyrmgus::sound *sound;         /// last sound played by this unit
	unsigned char HowMany; /// number of sound played in this group
};

/// FIXME: docu
SelectionHandling SelectionHandler;

static int ViewPointOffset;      /// Distance to Volume Mapping
int DistanceSilent;              /// silent distance

/**
**  "Randomly" choose a sample from a sound group.
*/
static wyrmgus::sample *SimpleChooseSample(const wyrmgus::sound &sound)
{
	if (sound.Number == ONE_SOUND) {
		return sound.get_samples().front().get();
	} else {
		//FIXME: check for errors
		//FIXME: valid only in shared memory context (FrameCounter)
		return sound.get_samples()[FrameCounter % sound.Number].get();
	}
}

/**
**  Choose the sample to play
*/
static wyrmgus::sample *ChooseSample(const wyrmgus::sound *sound, bool selection, Origin &source)
{
	wyrmgus::sample *result = nullptr;

	if (sound == nullptr || !SoundEnabled()) {
		return nullptr;
	}

	if (sound->Number == TWO_GROUPS) {
		// handle a special sound (selection)
		if (SelectionHandler.sound != nullptr && (SelectionHandler.Source.Base == source.Base && SelectionHandler.Source.Id == source.Id)) {
			if (SelectionHandler.sound == sound->get_first_sound()) {
				result = SimpleChooseSample(*SelectionHandler.sound);
				SelectionHandler.HowMany++;
				if (SelectionHandler.HowMany >= 3) {
					SelectionHandler.HowMany = 0;
					SelectionHandler.sound = sound->get_second_sound();
				}
			} else {
				//FIXME: checks for error
				// check whether the second group is really a group
				if (SelectionHandler.sound->Number > 1) {
					//Wyrmgus start
//					result = SelectionHandler.Sound->Sound.OneGroup[SelectionHandler.HowMany];
					result = SimpleChooseSample(*SelectionHandler.sound);
					//Wyrmgus end
					SelectionHandler.HowMany++;
					if (SelectionHandler.HowMany >= SelectionHandler.sound->Number) {
						SelectionHandler.HowMany = 0;
						SelectionHandler.sound = sound->get_first_sound();
					}
				} else {
					result = SelectionHandler.sound->get_samples().front().get();
					SelectionHandler.HowMany = 0;
					SelectionHandler.sound = sound->get_first_sound();
				}
			}
		} else {
			SelectionHandler.Source = source;
			SelectionHandler.sound = sound->get_first_sound();
			result = SimpleChooseSample(*SelectionHandler.sound);
			SelectionHandler.HowMany = 1;
		}
	} else {
		// normal sound/sound group handling
		result = SimpleChooseSample(*sound);
		if (SelectionHandler.Source.Base == source.Base && SelectionHandler.Source.Id == source.Id) {
			SelectionHandler.HowMany = 0;
			SelectionHandler.sound = nullptr;
		}
		if (selection) {
			SelectionHandler.Source = source;
		}
	}

	return result;
}

/**
**  Maps a unit_sound_type to a sound*.
**
**  @param unit    Sound initiator
**  @param voice   Type of sound wanted
**
**  @return        Sound identifier
*/
static const wyrmgus::sound *ChooseUnitVoiceSound(const CUnit *unit, const wyrmgus::unit_sound_type unit_sound_type)
{
	const wyrmgus::sound *sound = nullptr;

	if (unit->get_character() != nullptr && unit->get_character()->get_sound_set() != nullptr) {
		sound = unit->get_character()->get_sound_set()->get_sound_for_unit(unit_sound_type, unit);

		if (sound != nullptr) {
			return sound;
		}
	}

	sound = unit->Type->MapSound->get_sound_for_unit(unit_sound_type, unit);

	if (sound != nullptr) {
		return sound;
	}

	const civilization_base *civilization_base = unit->get_civilization_base();

	if (civilization_base != nullptr && civilization_base->get_unit_sound_set() != nullptr) {
		switch (unit_sound_type) {
			case wyrmgus::unit_sound_type::acknowledging:
			case wyrmgus::unit_sound_type::attack:
			case wyrmgus::unit_sound_type::build:
			case wyrmgus::unit_sound_type::ready:
			case wyrmgus::unit_sound_type::selected:
			case wyrmgus::unit_sound_type::dying:
			case wyrmgus::unit_sound_type::repairing:
			case wyrmgus::unit_sound_type::harvesting:
				if (unit->Type->BoolFlag[ORGANIC_INDEX].value) {
					return civilization_base->get_unit_sound_set()->get_sound_for_unit(unit_sound_type, unit);
				}
				break;
			case wyrmgus::unit_sound_type::help:
				if (unit->Type->BoolFlag[BUILDING_INDEX].value && civilization_base->get_help_town_sound() != nullptr) {
					return civilization_base->get_help_town_sound();
				}

				return civilization_base->get_unit_sound_set()->get_sound_for_unit(unit_sound_type, unit);
			default:
				break;
		}
	}

	return nullptr;
}

/**
**  Compute a suitable volume for something taking place at a given
**  distance from the current view point.
**
**  @param d      distance
**  @param range  range
**
**  @return       volume for given distance (0..??)
*/
unsigned char VolumeForDistance(unsigned short d, unsigned char range)
{
	// FIXME: THIS IS SLOW!!!!!!!
	if (d <= ViewPointOffset || range == wyrmgus::sound::infinite_range) {
		return MaxVolume;
	} else {
		if (range) {
			d -= ViewPointOffset;
			int d_tmp = d * wyrmgus::sound::max_range;
			int range_tmp = DistanceSilent * range;
			if (d_tmp > range_tmp) {
				return 0;
			} else {
				return (unsigned char)((range_tmp - d_tmp) * wyrmgus::sound::max_range / range_tmp);
			}
		} else {
			return 0;
		}
	}
}

/**
**  Calculate the volume associated with a request, either by clipping the
**  range parameter of this request, or by mapping this range to a volume.
*/
unsigned char CalculateVolume(bool isVolume, int power, unsigned char range)
{
	if (isVolume) {
		return std::min(MaxVolume, power);
	} else {
		// map distance to volume
		return VolumeForDistance(power, range);
	}
}

/**
**  Calculate the stereo value for a unit
*/
static char CalculateStereo(const CUnit &unit)
{
	int stereo = ((unit.tilePos.x * wyrmgus::defines::get()->get_scaled_tile_width() + unit.Type->get_tile_width() * wyrmgus::defines::get()->get_scaled_tile_width() / 2 +
				   unit.get_scaled_pixel_offset().x() - UI.SelectedViewport->MapPos.x * wyrmgus::defines::get()->get_scaled_tile_width()) * 256 /
				  ((UI.SelectedViewport->MapWidth - 1) * wyrmgus::defines::get()->get_scaled_tile_width())) - 128;
	stereo = std::clamp(stereo, -128, 127);
	return stereo;
}

/**
**  Ask to the sound server to play a sound attached to a unit. The
**  sound server may discard the sound if needed (e.g., when the same
**  unit is already speaking).
**
**  @param unit   Sound initiator, unit speaking
**  @param voice  Type of sound wanted (Ready,Die,Yes,...)
*/
void PlayUnitSound(const CUnit &unit, const wyrmgus::unit_sound_type unit_sound_type)
{
	if (!UI.CurrentMapLayer || unit.MapLayer != UI.CurrentMapLayer) {
		return;
	}
	
	if (unit.Variable[STUN_INDEX].Value > 0 && wyrmgus::is_voice_unit_sound_type(unit_sound_type)) { //don't speak if stunned
		return;
	}
	
	const wyrmgus::sound *sound = ChooseUnitVoiceSound(&unit, unit_sound_type);
	if (!sound) {
		return;
	}

	const bool selection = (unit_sound_type == wyrmgus::unit_sound_type::selected || unit_sound_type == wyrmgus::unit_sound_type::construction);
	Origin source = {&unit, unsigned(UnitNumber(unit))};
	
	//don't speak if already speaking
	if (wyrmgus::is_voice_unit_sound_type(unit_sound_type) && UnitSoundIsPlaying(&source)) {
		return;
	}

	const int volume = CalculateVolume(false, ViewPointDistanceToUnit(unit), wyrmgus::get_unit_sound_type_range(unit_sound_type)) * sound->VolumePercent / 100;

	if (volume == 0) {
		return;
	}

	int channel = PlaySample(ChooseSample(sound, selection, source), &source);
	if (channel == -1) {
		return;
	}
	SetChannelVolume(channel, volume);
	SetChannelStereo(channel, CalculateStereo(unit));
	SetChannelVoiceGroup(channel, unit_sound_type);
}

/**
**  Ask to the sound server to play a sound attached to a unit. The
**  sound server may discard the sound if needed (e.g., when the same
**  unit is already speaking).
**
**  @param unit   Sound initiator, unit speaking
**  @param sound  Sound to be generated
*/
void PlayUnitSound(const CUnit &unit, wyrmgus::sound *sound)
{
	//Wyrmgus start
	if (!&unit) {
		fprintf(stderr, "Error in PlayUnitSound: unit is null.\n");
		return;
	}
	
	if (!sound) {
		return;
	}

	if (unit.MapLayer != UI.CurrentMapLayer) {
		return;
	}

	Origin source = {&unit, unsigned(UnitNumber(unit))};

	const int volume = CalculateVolume(false, ViewPointDistanceToUnit(unit), sound->get_range()) * sound->VolumePercent / 100;

	if (volume == 0) {
		return;
	}

	int channel = PlaySample(ChooseSample(sound, false, source));
	if (channel == -1) {
		return;
	}
	SetChannelVolume(channel, volume);
	SetChannelStereo(channel, CalculateStereo(unit));
}

/**
**  Ask the sound server to play a sound for a missile.
**
**  @param missile  Sound initiator, missile exploding
**  @param sound    Sound to be generated
*/
void PlayMissileSound(const Missile &missile, wyrmgus::sound *sound)
{
	if (!sound) {
		return;
	}
	int stereo = ((missile.position.x + (missile.Type->G ? missile.Type->G->Width / 2 : 0) +
				   UI.SelectedViewport->MapPos.x * wyrmgus::defines::get()->get_tile_width()) * 256 /
				  ((UI.SelectedViewport->MapWidth - 1) * wyrmgus::defines::get()->get_tile_width())) - 128;
	stereo = std::clamp(stereo, -128, 127);

	Origin source = {nullptr, 0};
	const int volume = CalculateVolume(false, ViewPointDistanceToMissile(missile), sound->get_range()) * sound->VolumePercent / 100;

	if (volume == 0) {
		return;
	}

	int channel = PlaySample(ChooseSample(sound, false, source));
	if (channel == -1) {
		return;
	}
	SetChannelVolume(channel, volume);
	SetChannelStereo(channel, stereo);
}

/**
**  Play a game sound
**
**  @param sound   Sound to play
**  @param volume  Volume level to play the sound
*/
int PlayGameSound(const wyrmgus::sound *sound, unsigned char volume, const bool always)
{
	if (!sound) {
		return -1;
	}

	if (!SoundEnabled()) {
		return -1;
	}

	Origin source = {nullptr, 0};

	wyrmgus::sample *sample = ChooseSample(sound, false, source);

	if (!always && SampleIsPlaying(sample)) {
		return -1;
	}

	volume = CalculateVolume(true, volume, sound->get_range()) * sound->VolumePercent / 100;
	if (volume == 0) {
		return -1;
	}

	int channel = PlaySample(sample);
	if (channel == -1) {
		return -1;
	}

	SetChannelVolume(channel, volume);

	return channel;
}

/**
**  Ask the sound server to change the range of a sound.
**
**  @param sound  the id of the sound to modify.
**  @param range  the new range for this sound.
*/
void SetSoundRange(wyrmgus::sound *sound, unsigned char range)
{
	if (sound != nullptr) {
		sound->range = range;
	}
}

//Wyrmgus start
/**
**  Ask the sound server to change the volume percent of a sound.
**
**  @param sound  the id of the sound to modify.
**  @param volume_percent  the new volume percent for this sound.
*/
void SetSoundVolumePercent(wyrmgus::sound *sound, int volume_percent)
{
	if (sound != nullptr) {
		sound->VolumePercent = volume_percent;
	}
}
//Wyrmgus end

/**
**  Ask the sound server to register a sound (and currently to load it)
**  and to return an unique identifier for it. The unique identifier is
**  memory pointer of the server.
**
**  @param files   An array of wav files.
**  @param number  Number of files belonging together.
**
**  @return        the sound unique identifier
**
**  @todo FIXME: Must handle the errors better.
*/
sound *RegisterSound(const std::string &identifier, const std::vector<std::filesystem::path> &files)
{
	sound *id = sound::add(identifier, nullptr);

	for (const std::filesystem::path &filepath : files) {
		id->add_file(filepath);
	}

	return id;
}

/**
**  Ask the sound server to put together two sounds to form a special sound.
**
**  @param first   first part of the group
**  @param second  second part of the group
**
**  @return        the special sound unique identifier
*/
sound *RegisterTwoGroups(const std::string &identifier, sound *first, sound *second)
{
	if (first == nullptr || second == nullptr) {
		return nullptr;
	}
	sound *id = sound::add(identifier, nullptr);
	id->Number = TWO_GROUPS;
	id->first_sound = first;
	id->second_sound = second;
	id->range = sound::max_range;
	//Wyrmgus start
	id->VolumePercent = first->VolumePercent + second->VolumePercent / 2;
	//Wyrmgus end

	return id;
}

/**
**  Lookup the sound id's for the game sounds.
*/
void InitSoundClient()
{
	if (!SoundEnabled()) { // No sound enabled
		return;
	}

	const int MapWidth = (UI.MapArea.EndX - UI.MapArea.X + wyrmgus::defines::get()->get_scaled_tile_width()) / wyrmgus::defines::get()->get_scaled_tile_width();
	const int MapHeight = (UI.MapArea.EndY - UI.MapArea.Y + wyrmgus::defines::get()->get_scaled_tile_height()) / wyrmgus::defines::get()->get_scaled_tile_height();
	DistanceSilent = 3 * std::max<int>(MapWidth, MapHeight);
	ViewPointOffset = std::max<int>(MapWidth / 2, MapHeight / 2);
}

namespace wyrmgus {

sound::sound(const std::string &identifier) : data_entry(identifier)
{
}

sound::~sound()
{
}

void sound::initialize()
{
	const size_t file_count = this->get_files().size();
	if (file_count > 1) { // sound group
		this->Number = file_count;
	} else if (file_count == 1) { // unique sound
		this->Number = ONE_SOUND;
	} else if (this->get_first_sound() != nullptr && this->get_second_sound() != nullptr) {
		this->Number = TWO_GROUPS;
	} else {
		throw std::runtime_error("Sound \"" + this->get_identifier() + "\" is neither a sound group, nor does it have any files.");
	}

	for (const std::filesystem::path &filepath : this->get_files()) {
		auto sample = std::make_unique<wyrmgus::sample>(filepath);
		this->samples.push_back(std::move(sample));
	}

	data_entry::initialize();
}

void sound::unload()
{
	for (const std::unique_ptr<sample> &sample : this->samples) {
		if (sample->is_loaded()) {
			sample->unload();
		}
	}
}

QVariantList sound::get_files_qvariant_list() const
{
	return container::to_qvariant_list(this->get_files());
}

void sound::add_file(const std::filesystem::path &filepath)
{
	this->files.push_back(database::get()->get_sounds_path(this->get_module()) / filepath);
}

void sound::remove_file(const std::filesystem::path &filepath)
{
	vector::remove(this->files, filepath);
}

}