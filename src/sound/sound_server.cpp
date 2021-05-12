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
/**@name sound_server.cpp - The sound server (hardware layer and so on) */
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

#include "stratagus.h"

#include "sound/sound_server.h"

#include "civilization.h"
#include "dialogue.h"
#include "faction.h"
#include "iocompat.h"
#include "iolib.h"
//Wyrmgus start
#include "grand_strategy.h" //for playing faction music
#include "player.h" //for playing faction music
//Wyrmgus end
#include "sound/music.h"
#include "sound/music_player.h"
#include "sound/music_type.h"
#include "sound/sample.h"
#include "sound/sound.h"
#include "sound/unit_sound_type.h"
//Wyrmgus start
#include "ui/interface.h" //for player faction music
//Wyrmgus end
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_manager.h"
//Wyrmgus end
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/queue_util.h"
#include "util/qunique_ptr.h"

#include <QAudioDeviceInfo>
#include <QAudioFormat>

#include <SDL_mixer.h>

static bool SoundInitialized;    /// is sound initialized

static int EffectsVolume = 128;  /// effects sound volume
static int MusicVolume = 128;    /// music volume

static bool MusicEnabled = true;
static bool EffectsEnabled = true;

/// Channels for sound effects and unit speech
struct SoundChannel {
	std::unique_ptr<Origin> Unit;          /// pointer to unit, who plays the sound, if any
	wyrmgus::unit_sound_type Voice;  /// Voice group of this channel (for identifying voice types)
	void (*FinishedCallback)(int channel); /// Callback for when a sample finishes playing
};

static constexpr int MaxChannels = 64; //how many channels are supported

static SoundChannel Channels[MaxChannels];

static void ChannelFinished(int channel);

/*----------------------------------------------------------------------------
--  Effects
----------------------------------------------------------------------------*/

/**
**  Check if this sound is already playing
*/
bool SampleIsPlaying(const wyrmgus::sample *sample)
{
	for (int i = 0; i < MaxChannels; ++i) {
		if (Mix_GetChunk(i) == sample->get_chunk() && Mix_Playing(i)) {
			return true;
		}
	}
	return false;
}

bool UnitSoundIsPlaying(Origin *origin)
{
	for (int i = 0; i < MaxChannels; ++i) {
		//Wyrmgus start
//		if (origin != nullptr && Channels[i].Unit && origin->Id && Channels[i].Unit->Id
//			&& origin->Id == Channels[i].Unit->Id && Mix_Playing(i)) {
		if (
			origin != nullptr && Mix_Playing(i)
			&& Channels[i].Voice != wyrmgus::unit_sound_type::none
			&& wyrmgus::is_voice_unit_sound_type(Channels[i].Voice)
			&& Channels[i].Unit && origin->Id && Channels[i].Unit->Id
			&& origin->Id == Channels[i].Unit->Id
		) {
		//Wyrmgus end
			return true;
		}
	}
	return false;
}

/**
**  A channel is finished playing
*/
static void ChannelFinished(int channel)
{
	if (Channels[channel].FinishedCallback) {
		Channels[channel].FinishedCallback(channel);
	}

	Channels[channel].Unit.reset();
	Channels[channel].Voice = unit_sound_type::none;

	if (dialogue::has_sound_channel(channel)) {
		dialogue::remove_sound_channel(channel);
	}
}

/**
**  Set the channel volume
**
**  @param channel  Channel to set
**  @param volume   New volume, <0 will not set the volume
**
**  @return         Current volume of the channel, -1 for error
*/
int SetChannelVolume(int channel, int volume)
{
	if (channel < 0 || channel >= MaxChannels) {
		return -1;
	}

	//apply the effects volume to the channel volume
	volume *= GetEffectsVolume();
	volume /= MaxVolume;

	//ensure the volume is within proper bounds
	volume = std::max(0, volume);
	volume = std::min(MaxVolume, volume);

	Mix_Volume(channel, volume * MIX_MAX_VOLUME / MaxVolume);

	return volume;
}

/**
**  Set the channel stereo
**
**  @param channel  Channel to set
**  @param stereo   -128 to 127, out of range will not set the stereo
**
**  @return         Current stereo of the channel, -1 for error
*/
int SetChannelStereo(int channel, int stereo)
{
	if (channel < 0 || channel >= MaxChannels) {
		return -1;
	}

	if (Preference.StereoSound == false) {
		stereo = 0;
	}

	int left = 0;
	int right = 0;
	if (stereo == 0) {
		left = 255;
		right = 255;
	} else if (stereo > 0) {
		left = 255 - stereo;
		right = 255;
	} else {
		left = 255;
		right = 255 + stereo;
	}

	Mix_SetPanning(channel, left, right);
	return stereo;
}

//Wyrmgus start
/**
**  Set the channel voice group
**
**  @param channel  Channel to set
*/
void SetChannelVoiceGroup(int channel, const wyrmgus::unit_sound_type voice)
{
	if (channel < 0 || channel >= MaxChannels) {
		return;
	}

	Channels[channel].Voice = voice;
}
//Wyrmgus end

/**
**  Set the channel's callback for when a sound finishes playing
**
**  @param channel   Channel to set
**  @param callback  Callback to call when the sound finishes
*/
void SetChannelFinishedCallback(int channel, void (*callback)(int channel))
{
	if (channel < 0 || channel >= MaxChannels) {
		return;
	}
	Channels[channel].FinishedCallback = callback;
}

/**
**  Stop a channel
**
**  @param channel  Channel to stop
*/
void StopChannel(int channel)
{
	Mix_HaltChannel(channel);
}

/**
**  Stop all channels
*/
void StopAllChannels()
{
	Mix_HaltChannel(-1);
}

/**
**  Load a sample
**
**  @param name  File name of sample (short version).
**
**  @return      General sample loaded from file into memory.
**
**  @todo  Add streaming, caching support.
*/
std::unique_ptr<wyrmgus::sample> LoadSample(const std::filesystem::path &filepath)
{
	const std::string filename = LibraryFileName(filepath.string().c_str());
	auto sample = std::make_unique<wyrmgus::sample>(filename);
	return sample;
}

/**
**  Play a sound sample
**
**  @param sample  Sample to play
**
**  @return        Channel number, -1 for error
*/
int PlaySample(wyrmgus::sample *sample, Origin *origin)
{
	int channel = -1;

	if (SoundEnabled() && EffectsEnabled && sample != nullptr) {
		try {
			if (!sample->is_loaded()) {
				sample->load();
			}
		} catch (const std::exception &exception) {
			exception::report(exception);
			return -1;
		}

		channel = Mix_PlayChannel(-1, sample->get_chunk(), 0);
		Mix_Volume(channel, EffectsVolume * MIX_MAX_VOLUME / MaxVolume);

		Channels[channel].FinishedCallback = nullptr;
		Channels[channel].Voice = wyrmgus::unit_sound_type::none;
		Channels[channel].Unit.reset();

		if (origin && origin->Base) {
			auto source = std::make_unique<Origin>();
			source->Base = origin->Base;
			source->Id = origin->Id;
			Channels[channel].Unit = std::move(source);
		}
	}

	return channel;
}

/**
**  Set the global sound volume.
**
**  @param volume  the sound volume 0-255
*/
void SetEffectsVolume(int volume)
{
	volume = std::clamp(volume, 0, MaxVolume);
	EffectsVolume = volume;
}

/**
**  Get effects volume
*/
int GetEffectsVolume()
{
	return EffectsVolume;
}

/**
**  Set effects enabled
*/
void SetEffectsEnabled(bool enabled)
{
	EffectsEnabled = enabled;
}

/**
**  Check if effects are enabled
*/
bool IsEffectsEnabled()
{
	return EffectsEnabled;
}

/*----------------------------------------------------------------------------
--  Music
----------------------------------------------------------------------------*/

/**
**  Set the music finished callback
*/
void SetMusicFinishedCallback(void (*callback)())
{
	Mix_HookMusicFinished(callback);
}

void play_menu_music()
{
	music_player::get()->play_music_type(music_type::menu);
}

void play_credits_music()
{
	music_player::get()->play_music_type(music_type::credits);
}

void play_loading_music()
{
	music_player::get()->play_music_type(music_type::loading);
}

void play_map_music()
{
	music_player::get()->play_music_type(music_type::map);
}

void play_victory_music()
{
	music_player::get()->play_music_type(music_type::victory);
}

void play_defeat_music()
{
	music_player::get()->play_music_type(music_type::defeat);
}

/**
**  Stop the current playing music.
*/
void StopMusic()
{
	Mix_FadeOutMusic(200);
}

/**
**  Set the music volume.
**
**  @param volume  the music volume 0-255
*/
void SetMusicVolume(int volume)
{
	volume = std::clamp(volume, 0, MaxVolume);
	MusicVolume = volume;

	Mix_VolumeMusic(volume * wyrmgus::music_player::get()->get_current_volume_modifier() / 100 * MIX_MAX_VOLUME / MaxVolume);
}

/**
**  Get music volume
*/
int GetMusicVolume()
{
	return MusicVolume;
}

/**
**  Set music enabled
*/
void SetMusicEnabled(bool enabled)
{
	if (enabled) {
		MusicEnabled = true;
		wyrmgus::music_player::get()->play();
	} else {
		MusicEnabled = false;
		StopMusic();
	}
}

/**
**  Check if music is enabled
*/
bool IsMusicEnabled()
{
	return MusicEnabled;
}

/**
**  Add tension to music
*/
void AddMusicTension(int value)
{
	Q_UNUSED(value)
	//FIXME: keep a counter with music tension to use for scripted conditions?
}

/*----------------------------------------------------------------------------
--  Init
----------------------------------------------------------------------------*/

/**
**  Check if sound is enabled
*/
bool SoundEnabled()
{
	return SoundInitialized;
}

/**
**  Initialize sound card hardware part with SDL.
**
**  @param freq  Sample frequency (44100,22050,11025 hz).
**  @param size  Sample size (8bit, 16bit)
**
**  @return      True if failure, false if everything ok.
*/
static void InitSdlSound()
{
	static constexpr int init_flags = MIX_INIT_OGG;
	int result = Mix_Init(init_flags);
	if (result != init_flags) {
		throw std::runtime_error("Error in Mix_Init: " + std::string(Mix_GetError()));
	}

	const QAudioDeviceInfo device_info = QAudioDeviceInfo::defaultOutputDevice();
	const QAudioFormat format = device_info.preferredFormat();

	//open the audio device, using the desired format
	int frequency = format.sampleRate();
	if (frequency == -1) {
		log::log_error("Default output device's preferred format sample rate is -1, defaulting to MIX_DEFAULT_FREQUENCY.");
		frequency = MIX_DEFAULT_FREQUENCY;
	}

	uint16_t sdl_audio_format = 0;

	try {
		switch (format.sampleSize()) {
			case 8:
				sdl_audio_format |= 0x0008;
				break;
			case 16:
				sdl_audio_format |= 0x0010;

				switch (format.byteOrder()) {
					case QAudioFormat::LittleEndian:
						break;
					case QAudioFormat::BigEndian:
						sdl_audio_format |= 0x1000;
						break;
					default:
						throw std::runtime_error("Unexpected byte order: " + std::to_string(format.byteOrder()));
				}
				break;
			default:
				throw std::runtime_error("Unexpected sample size: " + std::to_string(format.sampleSize()));
		}

		switch (format.sampleType()) {
			case QAudioFormat::UnSignedInt:
				break;
			case QAudioFormat::SignedInt:
				sdl_audio_format |= 0x8000;
				break;
			default:
				throw std::runtime_error("Unexpected sample type: " + std::to_string(format.sampleType()));
		}
	} catch (const std::exception &exception) {
		exception::report(exception);
		//use the default format if we failed to derive information for one from the QAudioFormat
		sdl_audio_format = MIX_DEFAULT_FORMAT;
	}

	int channel_count = format.channelCount();
	if (channel_count == -1) {
		log::log_error("Default output device's preferred format channel count is -1, defaulting to MIX_DEFAULT_CHANNELS.");
		channel_count = MIX_DEFAULT_CHANNELS;
	}

	result = Mix_OpenAudio(frequency, sdl_audio_format, channel_count, 1024);
	if (result == -1) {
		throw std::runtime_error("Error in Mix_OpenAudio: " + std::string(Mix_GetError()));
	}
}

/**
**  Initialize sound.
**
**  @return  True on success, or false otherwise.
*/
bool InitSound()
{
	//
	// Open sound device, 16-bit samples, stereo.
	//
	try {
		InitSdlSound();
		SoundInitialized = true;
	} catch (const std::exception &exception) {
		exception::report(exception);
		SoundInitialized = false;
		return false;
	}

	// ARI: The following must be done here to allow sound to work in
	// pre-start menus!
	// initialize channels
	Mix_AllocateChannels(MaxChannels);

	Mix_ChannelFinished(ChannelFinished);

	for (int i = 0; i < MaxChannels; ++i) {
		Channels[i].Unit.reset();
		Channels[i].Voice = wyrmgus::unit_sound_type::none;
	}

	//now we're ready for the callback to run
	Mix_ResumeMusic();
	Mix_Resume(-1);

	return true;
}

/**
**  Cleanup sound server.
*/
void QuitSound()
{
	if (!SoundInitialized) {
		return;
	}

	wyrmgus::sound::unload_all();
	wyrmgus::music::unload_all();

	Mix_CloseAudio();
	Mix_Quit();

	// Mustn't call SDL_CloseAudio here, it'll be called again from SDL_Quit
	SoundInitialized = false;
}
