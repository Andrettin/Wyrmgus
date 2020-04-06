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
//      (c) Copyright 1998-2020 by Lutz Sammer, Fabrice Rossi, Jimmy Salmon
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
//

#pragma once

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database/data_entry.h"
#include "database/data_type.h"
#include "sound/unitsound.h"
#include "stratagus.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CSample;
class CUnit;
class LuaActionListener;
class Missile;

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

static constexpr int MaxSampleVolume = 255;  /// Maximum sample volume

/**
**  Voice groups for a unit
*/
enum class UnitVoiceGroup {
	None = -1,
	Selected,          /// If selected
	Acknowledging,     /// Acknowledge command
	Ready,             /// Command completed
	HelpMe,            /// If attacked
	Dying,             /// If killed
	WorkCompleted,     /// only worker, work completed
	Building,          /// only for building under construction
	Docking,           /// only for transport reaching coast
	Repairing,         /// repairing
	Harvesting,        /// harvesting
	Attack,            /// Attack command
	//Wyrmgus start
	Idle,				/// Idle sound
	Hit,				/// Hit another unit
	Miss,				/// Attacked another unit, but missed
	FireMissile,		/// Fire a missile at another unit
	Step,				/// Stepped
	Used,				/// Used (for items)
	//Wyrmgus end
	Build              /// worker goes to build a building
};


/**
**  Global game sounds, not associated to any unit-type
*/
class GameSound
{
public:
	SoundConfig PlacementError[MAX_RACES];        /// used by ui
	SoundConfig PlacementSuccess[MAX_RACES];      /// used by ui
	SoundConfig Click;                            /// used by ui
	SoundConfig Docking;                          /// ship reaches coast
	SoundConfig BuildingConstruction[MAX_RACES];  /// building under construction
	SoundConfig WorkComplete[MAX_RACES];          /// building ready
	SoundConfig Rescue[MAX_RACES];                /// rescue units
	SoundConfig ChatMessage;                      /// chat message
	SoundConfig ResearchComplete[MAX_RACES];      /// research complete message
	SoundConfig NotEnoughRes[MAX_RACES][MaxCosts];/// not enough resources message
	SoundConfig NotEnoughFood[MAX_RACES];         /// not enough food message
};

/**
** A possible value for Number in the Sound struct: means a simple sound
*/
static constexpr int ONE_SOUND = 0;
/**
** A possible value for Number in the Sound struct: means a double group (for
** selection/annoyed sounds)
*/
static constexpr int TWO_GROUPS = 1;

/**
** the range value that makes a sound volume distance independent
*/
static constexpr int INFINITE_SOUND_RANGE = 255;
/**
** the maximum range value
*/
static constexpr int MAX_SOUND_RANGE = 254;

/// Register a sound (can be a simple sound or a group)
extern stratagus::sound *RegisterSound(const std::string &identifier, const std::vector<std::filesystem::path> &files);

namespace stratagus {

class sound final : public data_entry, public data_type<sound>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList files READ get_files_qvariant_list)
	Q_PROPERTY(int range MEMBER range)

public:
	static constexpr const char *class_identifier = "sound";
	static constexpr const char *database_folder = "sounds";

	sound(const std::string &identifier) : data_entry(identifier)
	{
		memset(&Sound, 0, sizeof(Sound));
	}

	~sound();

	virtual void initialize() override;

	static void ProcessConfigData(const CConfigData *config_data);

	const std::vector<std::filesystem::path> &get_files() const
	{
		return this->files;
	}

	QVariantList get_files_qvariant_list() const;

	Q_INVOKABLE void add_file(const std::filesystem::path &filepath);
	Q_INVOKABLE void remove_file(const std::filesystem::path &filepath);
		
	/**
	**  Range is a multiplier for ::DistanceSilent.
	**  255 means infinite range of the sound.
	*/
	int range = MAX_SOUND_RANGE; //range is a multiplier for DistanceSilent
	//Wyrmgus start
//	unsigned char Number = 0;       /// single, group, or table of sounds.
	unsigned int Number = 0;       /// single, group, or table of sounds.
	//Wyrmgus end
	//Wyrmgus start
	int VolumePercent = 100;
	//Wyrmgus end
	union {
		CSample *OneSound;       /// if it's only a simple sound
		CSample **OneGroup;      /// when it's a simple group
		struct {
			sound *First;       /// first group: selected sound
			sound *Second;      /// second group: annoyed sound
		} TwoGroups;             /// when it's a double group
	} Sound;

private:
	std::vector<std::filesystem::path> files; //the paths to the sound files

	friend sound *::RegisterSound(const std::string &identifier, const std::vector<std::filesystem::path> &files);
};

}

/**
**  Origin of a sound
*/
struct Origin {
	//Wyrmgus start
//	const void *Base;   /// pointer on a Unit
	const CUnit *Base;   /// pointer on a Unit
	//Wyrmgus end
	unsigned Id;        /// unique identifier (if the pointer has been shared)
};


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern GameSound GameSounds;  /// Game sound configuration

extern bool CallbackMusic;  /// flag true callback ccl if stops

/// global range control (max cut off distance for sound)
extern int DistanceSilent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Calculates volume level
extern unsigned char CalculateVolume(bool isVolume, int power, unsigned char range);
/// Play a unit sound
extern void PlayUnitSound(const CUnit &unit, UnitVoiceGroup unit_voice_group);
/// Play a unit sound
extern void PlayUnitSound(const CUnit &unit, stratagus::sound *sound);
/// Play a missile sound
extern void PlayMissileSound(const Missile &missile, stratagus::sound *sound);
/// Play a game sound
extern void PlayGameSound(stratagus::sound *sound, unsigned char volume, bool always = false);

/// Play a sound file
extern int PlayFile(const std::string &name, LuaActionListener *listener = nullptr);

/// Modify the range of a given sound.
extern void SetSoundRange(stratagus::sound *sound, unsigned char range);

//Wyrmgus start
/// Modify the volume percent of a given sound.
extern void SetSoundVolumePercent(stratagus::sound *sound, int volume_percent);
//Wyrmgus end

///  Create a special sound group with two sounds
extern stratagus::sound *RegisterTwoGroups(const std::string &identifier, stratagus::sound *first, stratagus::sound *second);

/// Initialize client side of the sound layer.
extern void InitSoundClient();


// music.cpp

/// Check if music is finished and play the next song
extern void CheckMusicFinished(bool force = false);

/// Initialize music
extern void InitMusic();

/// Initialize adaptive music
extern void InitMusicOAML();

/// Load adaptive music definitions file
extern void LoadOAMLDefinitionsFile(const std::string &file_path);

/// Shutdown adaptive music
extern void ShutdownMusicOAML();

/// Turn music stopped callback on
#define CallbackMusicOn() \
	CallbackMusic = true;
/// Turn music stopped callback off
#define CallbackMusicOff() \
	CallbackMusic = false;


// sound_id.cpp

/// Map sound to identifier
extern void MapSound(const std::string &sound_name, stratagus::sound *id);
/// Make a sound bound to identifier
extern stratagus::sound *MakeSound(const std::string &sound_name, const std::vector<std::filesystem::path> &files);
/// Make a sound group bound to identifier
extern stratagus::sound *MakeSoundGroup(const std::string &name, stratagus::sound *first, stratagus::sound *second);

// script_sound.cpp

/// register ccl features
extern void SoundCclRegister();
