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
/**@name sound.h - The sound header file. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer, Fabrice Rossi, and Jimmy Salmon
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

#ifndef __SOUND_H__
#define __SOUND_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "unitsound.h"

//Wyrmgus start
#include <string>
#include <map>
//Wyrmgus end

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

#define MaxSampleVolume 255  /// Maximum sample volume
#define NO_SOUND 0           /// No valid sound ID

/**
**  Voice groups for a unit
*/
enum UnitVoiceGroup {
	VoiceSelected,          /// If selected
	VoiceAcknowledging,     /// Acknowledge command
	VoiceReady,             /// Command completed
	VoiceHelpMe,            /// If attacked
	VoiceDying,             /// If killed
	VoiceWorkCompleted,     /// only worker, work completed
	VoiceBuilding,          /// only for building under construction
	VoiceDocking,           /// only for transport reaching coast
	VoiceRepairing,         /// repairing
	VoiceHarvesting,        /// harvesting
	VoiceAttack,            /// Attack command
	//Wyrmgus start
	VoiceIdle,				/// Idle sound
	VoiceHit,				/// Hit another unit
	VoiceMiss,				/// Attacked another unit, but missed
	VoiceFireMissile,		/// Fire a missile at another unit
	VoiceStep,				/// Stepped
	VoiceUsed,				/// Used (for items)
	//Wyrmgus end
	VoiceBuild              /// worker goes to build a building
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
**  Sound definition.
*/
class CSound
{
public:
	CSound()
	{
		memset(&Sound, 0, sizeof(Sound));
	}
	
	~CSound();
	
	static void ProcessConfigData(const CConfigData *config_data);
		
	unsigned int Mapref = 0;
	/**
	**  Range is a multiplier for ::DistanceSilent.
	**  255 means infinite range of the sound.
	*/
	unsigned char Range = 0;	/// Range is a multiplier for DistanceSilent
	//Wyrmgus start
//	unsigned char Number = 0;	/// single, group, or table of sounds.
	unsigned int Number = 0;	/// single, group, or table of sounds.
	//Wyrmgus end
	//Wyrmgus start
	int VolumePercent = 0;
	//Wyrmgus end
	union {
		CSample *OneSound;       /// if it's only a simple sound
		CSample **OneGroup;      /// when it's a simple group
		struct {
			CSound *First;       /// first group: selected sound
			CSound *Second;      /// second group: annoyed sound
		} TwoGroups;             /// when it's a double group
	} Sound;
};

/**
** A possible value for Number in the Sound struct: means a simple sound
*/
#define ONE_SOUND 0
/**
** A possible value for Number in the Sound struct: means a double group (for
** selection/annoyed sounds)
*/
#define TWO_GROUPS 1

/**
** the range value that makes a sound volume distance independent
*/
#define INFINITE_SOUND_RANGE 255
/**
** the maximum range value
*/
#define MAX_SOUND_RANGE 254

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

//Wyrmgus start
extern std::map<std::string, CSound *> SoundMap;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Calculates volume level
extern unsigned char CalculateVolume(bool isVolume, int power, unsigned char range);
/// Play a unit sound
extern void PlayUnitSound(const CUnit &unit, UnitVoiceGroup unit_voice_group);
/// Play a unit sound
extern void PlayUnitSound(const CUnit &unit, CSound *sound);
/// Play a missile sound
extern void PlayMissileSound(const Missile &missile, CSound *sound);
/// Play a game sound
extern void PlayGameSound(CSound *sound, unsigned char volume, bool always = false);

/// Play a sound file
extern int PlayFile(const std::string &name, LuaActionListener *listener = nullptr);

/// Modify the range of a given sound.
extern void SetSoundRange(CSound *sound, unsigned char range);

//Wyrmgus start
/// Modify the volume percent of a given sound.
extern void SetSoundVolumePercent(CSound *sound, int volume_percent);
//Wyrmgus end

/// Register a sound (can be a simple sound or a group)
extern CSound *RegisterSound(const std::vector<std::string> &files);

///  Create a special sound group with two sounds
extern CSound *RegisterTwoGroups(CSound *first, CSound *second);

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

/// Turn music stopped callback on
#define CallbackMusicOn() \
	CallbackMusic = true;
/// Turn music stopped callback off
#define CallbackMusicOff() \
	CallbackMusic = false;


// sound_id.cpp

/// Map sound to identifier
extern void MapSound(const std::string &sound_name, CSound *id);
/// Get the sound id bound to an identifier
extern CSound *SoundForName(const std::string &sound_name);
/// Make a sound bound to identifier
extern CSound *MakeSound(const std::string &sound_name, const std::vector<std::string> &files);
/// Make a sound group bound to identifier
extern CSound *MakeSoundGroup(const std::string &name, CSound *first, CSound *second);

extern void FreeSounds();

// script_sound.cpp

/// register ccl features
extern void SoundCclRegister();

#endif
