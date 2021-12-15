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
//      (c) Copyright 1998-2021 by Lutz Sammer, Fabrice Rossi,
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

#pragma once

struct Origin;

namespace wyrmgus {
	class sample;
	enum class unit_sound_type;
}

constexpr int MaxVolume = 255;
constexpr int SOUND_BUFFER_SIZE = 65536;

/// Set the channel volume
extern int SetChannelVolume(int channel, int volume);
/// Set the channel stereo
extern int SetChannelStereo(int channel, int stereo);
//Wyrmgus start
/// Set the channel voice group
extern void SetChannelVoiceGroup(int channel, const wyrmgus::unit_sound_type unit_sound_type);
//Wyrmgus end
/// Set the channel's callback for when a sound finishes playing
extern void SetChannelFinishedCallback(int channel, void (*callback)(int channel));
/// Stop a channel
extern void StopChannel(int channel);
/// Stop all channels
extern void StopAllChannels();

/// Check if this unit plays some sound
extern bool UnitSoundIsPlaying(Origin *origin);
/// Check, if this sample is already playing
extern bool SampleIsPlaying(const wyrmgus::sample *sample);
/// Load a sample
extern std::unique_ptr<wyrmgus::sample> LoadSample(const std::filesystem::path &filepath);
/// Play a sample
extern int PlaySample(wyrmgus::sample *sample, Origin *origin = nullptr);

/// Increase tension value for the music
extern void AddMusicTension(int value);

/// Check if sound is enabled
extern bool SoundEnabled();
/// Initialize the sound card.
extern void InitSound();
///  Cleanup sound.
extern void QuitSound();
