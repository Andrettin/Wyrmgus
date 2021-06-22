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
//      (c) Copyright 1998-2021 by Lutz Sammer, Fabrice Rossi, Jimmy Salmon
//      and Andrettin
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
#include "sound/unitsound.h"
#include "stratagus.h"

class CConfigData;
class CUnit;
class LuaActionListener;
class Missile;

extern wyrmgus::sound *RegisterTwoGroups(const std::string &identifier, wyrmgus::sound *first, wyrmgus::sound *second);
extern void SetSoundRange(wyrmgus::sound *sound, unsigned char range);

constexpr int MaxSampleVolume = 255;  /// Maximum sample volume

/**
** A possible value for Number in the Sound struct: means a simple sound
*/
constexpr int ONE_SOUND = 0;
/**
** A possible value for Number in the Sound struct: means a double group (for
** selection/annoyed sounds)
*/
constexpr int TWO_GROUPS = 1;

/// Register a sound (can be a simple sound or a group)
extern wyrmgus::sound *RegisterSound(const std::string &identifier, const std::vector<std::filesystem::path> &files);

namespace wyrmgus {

class sample;
enum class unit_sound_type;

class sound final : public data_entry, public data_type<sound>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList files READ get_files_qvariant_list)
	Q_PROPERTY(int range MEMBER range READ get_range)
	Q_PROPERTY(wyrmgus::sound* first_sound MEMBER first_sound READ get_first_sound)
	Q_PROPERTY(wyrmgus::sound* second_sound MEMBER second_sound READ get_second_sound)

public:
	static constexpr const char *class_identifier = "sound";
	static constexpr const char *database_folder = "sounds";
	static constexpr int max_range = 254;
	static constexpr int infinite_range = 255; //the range value that makes a sound volume distance independent

	static void unload_all()
	{
		for (sound *sound : sound::get_all()) {
			sound->unload();
		}
	}

	explicit sound(const std::string &identifier);
	virtual ~sound() override;

	virtual void initialize() override;

	void unload();

	const std::vector<std::filesystem::path> &get_files() const
	{
		return this->files;
	}

	QVariantList get_files_qvariant_list() const;

	Q_INVOKABLE void add_file(const std::filesystem::path &filepath);
	Q_INVOKABLE void remove_file(const std::filesystem::path &filepath);

	const std::vector<std::unique_ptr<sample>> &get_samples() const
	{
		return this->samples;
	}

	int get_range() const
	{
		return this->range;
	}

	sound *get_first_sound() const
	{
		return this->first_sound;
	}

	sound *get_second_sound() const
	{
		return this->second_sound;
	}

	/**
	**  Range is a multiplier for ::DistanceSilent.
	**  255 means infinite range of the sound.
	*/
private:
	int range = sound::max_range; //range is a multiplier for DistanceSilent
public:
	//Wyrmgus start
//	unsigned char Number = 0;       /// single, group, or table of sounds.
	unsigned int Number = 0;       /// single, group, or table of sounds.
	//Wyrmgus end
	//Wyrmgus start
	int VolumePercent = 100;
	//Wyrmgus end

private:
	std::vector<std::filesystem::path> files; //the paths to the sound files
	std::vector<std::unique_ptr<sample>> samples; //the sound's samples, one for each file
	sound *first_sound = nullptr; //selected sound
	sound *second_sound = nullptr; //annoyed sound

	friend sound *::RegisterTwoGroups(const std::string &identifier, sound *first, sound *second);
	friend void ::SetSoundRange(wyrmgus::sound *sound, unsigned char range);
};

}

/**
**  Origin of a sound
*/
struct Origin {
	const CUnit *Base;  /// pointer on a Unit
	unsigned Id;        /// unique identifier (if the pointer has been shared)
};

extern bool CallbackMusic;  /// flag true callback ccl if stops

/// global range control (max cut off distance for sound)
extern int DistanceSilent;

/// Calculates volume level
extern unsigned char CalculateVolume(bool isVolume, int power, unsigned char range);
/// Play a unit sound
extern void PlayUnitSound(const CUnit &unit, const wyrmgus::unit_sound_type unit_sound_type);
/// Play a unit sound
extern void PlayUnitSound(const CUnit &unit, wyrmgus::sound *sound);
/// Play a missile sound
extern void PlayMissileSound(const Missile &missile, wyrmgus::sound *sound);
/// Play a game sound
extern int PlayGameSound(const wyrmgus::sound *sound, unsigned char volume, const bool always = false);

/// Modify the range of a given sound.
extern void SetSoundRange(wyrmgus::sound *sound, unsigned char range);

//Wyrmgus start
/// Modify the volume percent of a given sound.
extern void SetSoundVolumePercent(wyrmgus::sound *sound, int volume_percent);
//Wyrmgus end

///  Create a special sound group with two sounds
extern wyrmgus::sound *RegisterTwoGroups(const std::string &identifier, wyrmgus::sound *first, wyrmgus::sound *second);

/// Initialize client side of the sound layer.
extern void InitSoundClient();

// music.cpp

/// Check if music is finished and play the next song
extern void CheckMusicFinished();

/// Initialize music
extern void InitMusic();

/// Turn music stopped callback on
#define CallbackMusicOn() \
	CallbackMusic = true;
/// Turn music stopped callback off
#define CallbackMusicOff() \
	CallbackMusic = false;


// sound_id.cpp

/// Map sound to identifier
extern void MapSound(const std::string &sound_name, wyrmgus::sound *id);
/// Make a sound bound to identifier
extern wyrmgus::sound *MakeSound(const std::string &sound_name, const std::vector<std::filesystem::path> &files);
/// Make a sound group bound to identifier
extern wyrmgus::sound *MakeSoundGroup(const std::string &name, wyrmgus::sound *first, wyrmgus::sound *second);

// script_sound.cpp

/// register ccl features
extern void SoundCclRegister();
