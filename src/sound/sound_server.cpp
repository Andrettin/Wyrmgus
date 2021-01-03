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

#include "stratagus.h"

#include "sound/sound_server.h"

#include "civilization.h"
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
#include "util/queue_util.h"
#include "util/qunique_ptr.h"

#ifdef USE_OAML
#include <oaml.h>
#endif

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

static struct {
	std::unique_ptr<wyrmgus::sample> Sample;       /// Music sample
	void (*FinishedCallback)(); /// Callback for when music finishes playing
} MusicChannel;

static void ChannelFinished(int channel);

#ifdef USE_OAML
#ifndef SDL_AUDIO_BITSIZE
#define SDL_AUDIO_BITSIZE(x) (x&0xFF)
#endif
#endif

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
	Channels[channel].Voice = wyrmgus::unit_sound_type::none;
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
		if (!sample->is_loaded()) {
			sample->load();
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
	clamp(&volume, 0, MaxVolume);
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

/**
**  Play a music file.
**
**  @param sample  Music sample.
**
**  @return        0 if music is playing, -1 if not.
*/
int PlayMusic(std::unique_ptr<wyrmgus::sample> &&sample)
{
	if (sample) {
		StopMusic();
		MusicChannel.Sample = std::move(sample);
		return 0;
	} else {
		DebugPrint("Could not play sample\n");
		return -1;
	}
}

/**
**  Play a music file.
**
**  @param file  Name of music file, format is automatically detected.
**
**  @return      0 if music is playing, -1 if not.
*/
int PlayMusic(const std::string &file)
{
	if (!SoundEnabled() || !IsMusicEnabled()) {
		return -1;
	}
	const std::string name = LibraryFileName(file.c_str());
	DebugPrint("play music %s\n" _C_ name.c_str());
	std::unique_ptr<wyrmgus::sample> sample = LoadSample(name);

	if (sample != nullptr) {
		StopMusic();
		MusicChannel.Sample = std::move(sample);
		return 0;
	} else {
		DebugPrint("Could not play %s\n" _C_ file.c_str());
		return -1;
	}
}

void play_menu_music()
{
	wyrmgus::music_player::get()->play_music_type(wyrmgus::music_type::menu);
}

void play_credits_music()
{
	wyrmgus::music_player::get()->play_music_type(wyrmgus::music_type::credits);
}

void play_loading_music()
{
	wyrmgus::music_player::get()->play_music_type(wyrmgus::music_type::loading);
}

void play_map_music()
{
	wyrmgus::music_player::get()->play_music_type(wyrmgus::music_type::map);
}

void play_victory_music()
{
	wyrmgus::music_player::get()->play_music_type(wyrmgus::music_type::victory);
}

void play_defeat_music()
{
	wyrmgus::music_player::get()->play_music_type(wyrmgus::music_type::defeat);
}

void PlayMusicName(const std::string &name) {
	if (!IsMusicEnabled()) {
		return;
	}
	
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	oaml->PlayTrack(name.c_str());
#endif
}

void PlayMusicByGroupRandom(const std::string &group) {
	if (!IsMusicEnabled()) {
		return;
	}
	
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	oaml->PlayTrackByGroupRandom(group.c_str());
#endif
}

void PlayMusicByGroupAndSubgroupRandom(const std::string &group, const std::string &subgroup) {
	if (!IsMusicEnabled()) {
		return;
	}
	
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	if (oaml->PlayTrackByGroupAndSubgroupRandom(group.c_str(), subgroup.c_str()) != OAML_OK) {
		oaml->PlayTrackByGroupRandom(group.c_str());
	}
#endif
}

void PlayMusicByGroupAndFactionRandom(const std::string &group, const std::string &civilization_name, const std::string &faction_name) {
	if (!IsMusicEnabled()) {
		return;
	}

#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	if (oaml->PlayTrackByGroupAndSubgroupRandom(group.c_str(), faction_name.c_str()) != OAML_OK) {
		const wyrmgus::civilization *civilization = wyrmgus::civilization::try_get(civilization_name);
		const wyrmgus::faction *faction = wyrmgus::faction::try_get(faction_name);
		const wyrmgus::faction *parent_faction = nullptr;
		bool found_music = false;
		if (faction != nullptr) {
			while (true) {
				parent_faction = faction->get_parent_faction();
				if (parent_faction == nullptr) {
					break;
				}
				faction = parent_faction;
				
				if (oaml->PlayTrackByGroupAndSubgroupRandom(group.c_str(), faction->get_identifier().c_str()) == OAML_OK) {
					found_music = true;
					break;
				}
			}
		}
		if (!found_music && oaml->PlayTrackByGroupAndSubgroupRandom(group.c_str(), civilization_name.c_str()) != OAML_OK) {
			const wyrmgus::civilization *parent_civilization = nullptr;
			if (civilization) {
				while (true) {
					parent_civilization = civilization->get_parent_civilization();
					if (!parent_civilization) {
						break;
					}
					civilization = parent_civilization;
					
					if (oaml->PlayTrackByGroupAndSubgroupRandom(group.c_str(), civilization->get_identifier().c_str()) == OAML_OK) {
						found_music = true;
						break;
					}
				}
			}
			if (!found_music) {
				oaml->PlayTrackByGroupRandom(group.c_str());
			}
		}
	}
#endif
}

void SetMusicCondition(int id, int value) {
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	oaml->SetCondition(id, value);
#endif
}

void SetMusicLayerGain(const std::string &layer, float gain) {
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	oaml->SetLayerGain(layer.c_str(), gain);
#endif
}

/**
**  Stop the current playing music.
*/
void StopMusic()
{
#ifdef USE_OAML
	if (enableOAML && oaml) {
		oaml->StopPlaying();
	}
#endif

	Mix_FadeOutMusic(200);
}

/**
**  Set the music volume.
**
**  @param volume  the music volume 0-255
*/
void SetMusicVolume(int volume)
{
	clamp(&volume, 0, MaxVolume);
	MusicVolume = volume;

#ifdef USE_OAML
	if (enableOAML && oaml) {
		oaml->SetVolume(MusicVolume / 255.f);
	}
#endif

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
**  Check if music is playing
*/
bool IsMusicPlaying()
{
#ifdef USE_OAML
	if (enableOAML && oaml) {
		if (oaml->IsPlaying()) {
			return true;
		}
	}
#endif

	return Mix_PlayingMusic() == 1;
}

/**
**  Add tension to music
*/
void AddMusicTension(int value)
{
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	oaml->AddTension(value);
#endif
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
	static constexpr int init_flags = 0;
	int result = Mix_Init(init_flags);
	if (result != init_flags) {
		throw std::runtime_error("Error in Mix_Init: " + std::string(Mix_GetError()));
	}

	//open the audio device, forcing the desired format
	uint16_t format = 0;
	switch (wyrmgus::sample::sample_size) {
		case 8:
			format = AUDIO_U8;
			break;
		case 16:
			format = AUDIO_S16SYS;
			break;
		default:
			throw std::runtime_error("Unexpected sample size: " + std::to_string(wyrmgus::sample::sample_size));
	}

	result = Mix_OpenAudio(wyrmgus::sample::frequency, format, wyrmgus::sample::channel_count, 1024);
	if (result == -1) {
		throw std::runtime_error("Error in Mix_OpenAudio: " + std::string(Mix_GetError()));
	}
}

/**
**  Initialize sound card.
**
**  @return  True if failure, false if everything ok.
*/
int InitSound()
{
	//
	// Open sound device, 16-bit samples, stereo.
	//
	InitSdlSound();
	SoundInitialized = true;

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

	return 0;
}

/**
**  Cleanup sound server.
*/
void QuitSound()
{
#ifdef USE_OAML
	if (oaml) {
		oaml->Shutdown();
		oaml.reset();
	}
#endif

	for (wyrmgus::sound *sound : wyrmgus::sound::get_all()) {
		sound->clear_samples();
	}

	for (wyrmgus::music *music : wyrmgus::music::get_all()) {
		music->clear_sample();
	}

	Mix_CloseAudio();
	Mix_Quit();

	// Mustn't call SDL_CloseAudio here, it'll be called again from SDL_Quit
	SoundInitialized = false;
}
