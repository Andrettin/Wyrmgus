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
/**@name sound_server.h - The sound server header file. */
//
//      (c) Copyright 1998-2020 by Lutz Sammer, Fabrice Rossi,
//                                 Jimmy Salmon and Andrettin
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

#include "sound/sound.h"

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

static constexpr int MaxVolume = 255;
static constexpr int SOUND_BUFFER_SIZE = 65536;

/**
**  RAW samples.
*/
class CSample
{
public:
	virtual ~CSample() {}

	virtual int Read(void *buf, int len) { return 0; }

	unsigned char *Buffer = nullptr; //sample buffer
	int Len = 0; //length of filled buffer

	const QAudioFormat &get_format() const
	{
		return this->format;
	}

	void set_format(const QAudioFormat &format)
	{
		if (format == this->get_format()) {
			return;
		}

		this->format = format;
	}

private:
	QAudioFormat format;
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Set the channel volume
extern int SetChannelVolume(int channel, int volume);
/// Set the channel stereo
extern int SetChannelStereo(int channel, int stereo);
//Wyrmgus start
/// Set the channel voice group
extern void SetChannelVoiceGroup(int channel, UnitVoiceGroup voice);
//Wyrmgus end
/// Set the channel's callback for when a sound finishes playing
extern void SetChannelFinishedCallback(int channel, void (*callback)(int channel));
/// Get the sample playing on a channel
extern CSample *GetChannelSample(int channel);
/// Stop a channel
extern void StopChannel(int channel);
/// Stop all channels
extern void StopAllChannels();

/// Check if this unit plays some sound
extern bool UnitSoundIsPlaying(Origin *origin);
/// Check, if this sample is already playing
extern bool SampleIsPlaying(CSample *sample);
/// Load a sample
extern CSample *LoadSample(const std::filesystem::path &filepath);
/// Play a sample
extern int PlaySample(CSample *sample, Origin *origin = nullptr);
/// Play a sound file
extern int PlaySoundFile(const std::string &name);

/// Set effects volume
extern void SetEffectsVolume(int volume);
/// Get effects volume
extern int GetEffectsVolume();
/// Set effects enabled
extern void SetEffectsEnabled(bool enabled);
/// Check if effects are enabled
extern bool IsEffectsEnabled();

/// Set the music finished callback
void SetMusicFinishedCallback(void (*callback)());
/// Play a music file
extern int PlayMusic(CSample *sample);
/// Play a music file
extern int PlayMusic(const std::string &file);
/// Play a music track
extern void PlayMusicName(const std::string &name);
/// Play a music track
extern void PlayMusicByGroupRandom(const std::string &group);
/// Play a music track
extern void PlayMusicByGroupAndSubgroupRandom(const std::string &group, const std::string &subgroup);
/// Play a music track
extern void PlayMusicByGroupAndFactionRandom(const std::string &group, const std::string &civilization_name, const std::string &faction_name);
/// Set a condition for music
extern void SetMusicCondition(int id, int value);
/// Increase tension value for the music
extern void AddMusicTension(int value);
/// Set gain of a music layer
extern void SetMusicLayerGain(const std::string &layer, float gain);
/// Stop music playing
extern void StopMusic();
/// Set music volume
extern void SetMusicVolume(int volume);
/// Get music volume
extern int GetMusicVolume();
/// Set music enabled
extern void SetMusicEnabled(bool enabled);
/// Check if music is enabled
extern bool IsMusicEnabled();
/// Check if music is playing
extern bool IsMusicPlaying();

/// Check if sound is enabled
extern bool SoundEnabled();
/// Initialize the sound card.
extern int InitSound();
///  Cleanup sound.
extern void QuitSound();
