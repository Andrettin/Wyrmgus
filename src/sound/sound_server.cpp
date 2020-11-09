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

#include "SDL.h"

#ifdef USE_OAML
#include <oaml.h>
#endif

static bool SoundInitialized;    /// is sound initialized
static bool MusicPlaying;        /// flag true if playing music

static int EffectsVolume = 128;  /// effects sound volume
static int MusicVolume = 128;    /// music volume

static bool MusicEnabled = true;
static bool EffectsEnabled = true;

/// Channels for sound effects and unit speech
struct SoundChannel {
	wyrmgus::sample *Sample;       /// sample to play
	std::unique_ptr<Origin> Unit;          /// pointer to unit, who plays the sound, if any
	unsigned char Volume;  /// Volume of this channel
	signed char Stereo;    /// stereo location of sound (-128 left, 0 center, 127 right)
	//Wyrmgus start
	wyrmgus::unit_sound_type Voice;  /// Voice group of this channel (for identifying voice types)
	//Wyrmgus end

	bool Playing;          /// channel is currently playing
	int Point;             /// point in sample if playing or next free channel

	void (*FinishedCallback)(int channel); /// Callback for when a sample finishes playing
};

static constexpr int MaxChannels = 64; //how many channels are supported

static SoundChannel Channels[MaxChannels];
static int NextFreeChannel;

static struct {
	std::unique_ptr<wyrmgus::sample> Sample;       /// Music sample
	void (*FinishedCallback)(); /// Callback for when music finishes playing
} MusicChannel;

static void ChannelFinished(int channel);

static struct {
	SDL_AudioSpec Format;
	SDL_mutex *Lock;
	SDL_cond *Cond;
	SDL_Thread *Thread;

	std::unique_ptr<int[]> MixerBuffer;
	std::unique_ptr<Uint8[]> Buffer;
	bool Running;
} Audio;

#ifdef USE_OAML
#ifndef SDL_AUDIO_BITSIZE
#define SDL_AUDIO_BITSIZE(x) (x&0xFF)
#endif
#endif

/*----------------------------------------------------------------------------
--  Mixers
----------------------------------------------------------------------------*/

/**
**  Convert RAW sound data to 44100 hz, Stereo, 16 bits per channel
**
**  @param src        Source buffer
**  @param dest       Destination buffer
**  @param frequency  Frequency of source
**  @param chansize   Bitrate in bytes per channel of source
**  @param channels   Number of channels of source
**  @param bytes      Number of compressed bytes to read
**
**  @return           Number of bytes written in 'dest'
*/
static int ConvertToStereo32(const char *src, char *dest, int frequency,
							 int chansize, int channels, int bytes)
{
	SDL_AudioCVT acvt;
	Uint16 format;

	if (chansize == 1) {
		format = AUDIO_U8;
	} else {
		format = AUDIO_S16SYS;
	}
	SDL_BuildAudioCVT(&acvt, format, channels, frequency, AUDIO_S16SYS, 2, 44100);

	acvt.buf = (Uint8 *)dest;
	memcpy(dest, src, bytes);
	acvt.len = bytes;

	SDL_ConvertAudio(&acvt);

	return acvt.len_mult * bytes;
}

/**
**  Mix music to stereo 32 bit.
**
**  @param buffer  Buffer for mixed samples.
**  @param size    Number of samples that fits into buffer.
**
**  @todo this functions can be called from inside the SDL audio callback,
**  which is bad, the buffer should be precalculated.
*/
static void MixMusicToStereo32(int *buffer, int size)
{
	if (MusicPlaying) {
		Assert(MusicChannel.Sample);

		auto buf = std::make_unique<short[]>(size);
		int len = size * sizeof(short);
		auto tmp = std::make_unique<char[]>(len);

		int div = 176400 / (MusicChannel.Sample->get_format().sampleRate() * (MusicChannel.Sample->get_format().sampleSize() / 8) * MusicChannel.Sample->get_format().channelCount());

		size = MusicChannel.Sample->Read(tmp.get(), len / div);

		int n = ConvertToStereo32(tmp.get(), reinterpret_cast<char *>(buf.get()), MusicChannel.Sample->get_format().sampleRate(), MusicChannel.Sample->get_format().sampleSize() / 8, MusicChannel.Sample->get_format().channelCount(), size);

		for (int i = 0; i < n / static_cast<int>(sizeof(*buf.get())); ++i) {
			// Add to our samples
			// FIXME: why taking out '/ 2' leads to distortion
			buffer[i] += buf[i] * MusicVolume / MaxVolume / 2;
		}

		if (n < len) { // End reached
			MusicPlaying = false;
			MusicChannel.Sample.reset();

			if (MusicChannel.FinishedCallback) {
				MusicChannel.FinishedCallback();
			}
		}
	}
}

/**
**  Mix sample to buffer.
**
**  The input samples are adjusted by the local volume and resampled
**  to the output frequence.
**
**  @param sample  Input sample
**  @param index   Position into input sample
**  @param volume  Volume of the input sample
**  @param stereo  Stereo (left/right) position of sample
**  @param buffer  Output buffer
**  @param size    Size of output buffer (in samples per channel)
**
**  @return        The number of bytes used to fill buffer
**
**  @todo          Can mix faster if signed 8 bit buffers are used.
*/
static int MixSampleToStereo32(wyrmgus::sample *sample, int index, unsigned char volume,
							   char stereo, int *buffer, int size)
{
	static int buf[SOUND_BUFFER_SIZE / 2];
	unsigned char left;
	unsigned char right;

	int div = 176400 / (sample->get_format().sampleRate() * (sample->get_format().sampleSize() / 8) * sample->get_format().channelCount());
	int local_volume = (int)volume * EffectsVolume / MaxVolume;

	if (stereo < 0) {
		left = 128;
		right = 128 + stereo;
	} else {
		left = 128 - stereo;
		right = 128;
	}

	Assert(!(index & 1));

	size = std::min((sample->get_length() - index) * div / 2, size);

	size = ConvertToStereo32((char *)(sample->get_buffer() + index), (char *)buf, sample->get_format().sampleRate(), sample->get_format().sampleSize() / 8, sample->get_format().channelCount(), size * 2 / div);

	size /= 2;
	for (int i = 0; i < size; i += 2) {
		// FIXME: why taking out '/ 2' leads to distortion
		buffer[i] += ((short *)buf)[i] * local_volume * left / 128 / MaxVolume / 2;
		buffer[i + 1] += ((short *)buf)[i + 1] * local_volume * right / 128 / MaxVolume / 2;
	}

	return 2 * size / div;
}

/**
**  Mix channels to stereo 32 bit.
**
**  @param buffer  Buffer for mixed samples.
**  @param size    Number of samples that fits into buffer.
**
**  @return        How many channels become free after mixing them.
*/
static int MixChannelsToStereo32(int *buffer, const int size)
{
	int new_free_channels = 0;

	for (int channel = 0; channel < MaxChannels; ++channel) {
		if (Channels[channel].Playing && Channels[channel].Sample) {
			//Wyrmgus start
			if ((Channels[channel].Point & 1)) {
				fprintf(stderr, "Sound effect error; Index: %d, Voice: %d, Origin: \"%s\", Sample Length: %d\n", Channels[channel].Point, Channels[channel].Voice, (Channels[channel].Unit && Channels[channel].Unit->Base) ? wyrmgus::unit_manager::get()->GetSlotUnit(Channels[channel].Unit->Id).Type->Ident.c_str() : "", Channels[channel].Sample->get_length());
			}
			//Wyrmgus end
			int i = MixSampleToStereo32(Channels[channel].Sample,
										Channels[channel].Point, Channels[channel].Volume,
										Channels[channel].Stereo, buffer, size);
			Channels[channel].Point += i;
			Assert(Channels[channel].Point <= Channels[channel].Sample->get_length());

			if (Channels[channel].Point == Channels[channel].Sample->get_length()) {
				ChannelFinished(channel);
				++new_free_channels;
			}
		}
	}
	return new_free_channels;
}

/**
**  Clip mix to output stereo 16 signed bit.
**
**  @param mix     signed 32 bit input.
**  @param size    number of samples in input.
**  @param output  clipped 16 signed bit output buffer.
*/
static void ClipMixToStereo16(const int *mix, int size, short *output)
{
	const int *end = mix + size;

	while (mix < end) {
		int s = (*mix++);
		clamp(&s, SHRT_MIN, SHRT_MAX);
		*output++ = s;
	}
}

/**
**  Mix into buffer.
**
**  @param buffer   Buffer to be filled with samples. Buffer must be big enough.
**  @param samples  Number of samples.
*/
static void MixIntoBuffer(void *buffer, int samples)
{
	// FIXME: can save the memset here, if first channel sets the values
	memset(Audio.MixerBuffer.get(), 0, samples * sizeof(*Audio.MixerBuffer.get()));

	if (EffectsEnabled) {
		// Add channels to mixer buffer
		MixChannelsToStereo32(Audio.MixerBuffer.get(), samples);
	}
	if (MusicEnabled) {
		// Add music to mixer buffer
		MixMusicToStereo32(Audio.MixerBuffer.get(), samples);
	}
	ClipMixToStereo16(Audio.MixerBuffer.get(), samples, (short *)buffer);

#ifdef USE_OAML
	if (enableOAML && oaml) {
		oaml->SetAudioFormat(Audio.Format.freq, Audio.Format.channels, SDL_AUDIO_BITSIZE(Audio.Format.format) / 8);
		oaml->MixToBuffer(buffer, samples);
	}
#endif
}

/**
**  Fill buffer for the sound card.
**
**  @see SDL_OpenAudio
**
**  @param udata   the pointer stored in userdata field of SDL_AudioSpec.
**  @param stream  pointer to buffer you want to fill with information.
**  @param len     is length of audio buffer in bytes.
*/
static void FillAudio(void *, Uint8 *stream, int len)
{
	Assert((len/2) != static_cast<int>(Audio.Format.size));

	if (Audio.Running == false)
		return;

	SDL_memset(stream, 0, len);

	SDL_LockMutex(Audio.Lock);
	SDL_MixAudio(stream, Audio.Buffer.get(), len, SDL_MIX_MAXVOLUME);

	// Signal our FillThread, we can fill the Audio.Buffer again
	SDL_CondSignal(Audio.Cond);
	SDL_UnlockMutex(Audio.Lock);
}

/**
**  Fill audio thread.
*/
static int FillThread(void *)
{
	while (Audio.Running == true) {
		SDL_LockMutex(Audio.Lock);
#ifdef USE_WIN32
		if (SDL_CondWaitTimeout(Audio.Cond, Audio.Lock, 1000) == 0) {
#else
		if (SDL_CondWaitTimeout(Audio.Cond, Audio.Lock, 100) == 0) {
#endif
			MixIntoBuffer(Audio.Buffer.get(), Audio.Format.samples * Audio.Format.channels);
		}
		SDL_UnlockMutex(Audio.Lock);

#ifdef USE_OAML
		if (enableOAML && oaml) {
			oaml->Update();
		}
#endif
	}

	return 0;
}

/*----------------------------------------------------------------------------
--  Effects
----------------------------------------------------------------------------*/

/**
**  Check if this sound is already playing
*/
bool SampleIsPlaying(wyrmgus::sample *sample)
{
	for (int i = 0; i < MaxChannels; ++i) {
		if (Channels[i].Sample == sample && Channels[i].Playing) {
			return true;
		}
	}
	return false;
}

bool UnitSoundIsPlaying(Origin *origin)
{
	for (int i = 0; i < MaxChannels; ++i) {
		//Wyrmgus start
//		if (origin && Channels[i].Unit && origin->Id && Channels[i].Unit->Id
//			&& origin->Id == Channels[i].Unit->Id && Channels[i].Playing) {
		if (
			origin && Channels[i].Playing
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
	
	//Wyrmgus start
	Channels[channel].Voice = wyrmgus::unit_sound_type::none;
	//Wyrmgus end
	Channels[channel].Playing = false;
	Channels[channel].Point = NextFreeChannel;
	NextFreeChannel = channel;
}

/**
**  Put a sound request in the next free channel.
*/
static int FillChannel(wyrmgus::sample *sample, unsigned char volume, char stereo, Origin *origin)
{
	Assert(NextFreeChannel < MaxChannels);

	int old_free = NextFreeChannel;
	int next_free = Channels[NextFreeChannel].Point;

	Channels[NextFreeChannel].Volume = volume;
	Channels[NextFreeChannel].Point = 0;
	//Wyrmgus start
	Channels[NextFreeChannel].Voice = wyrmgus::unit_sound_type::none;
	//Wyrmgus end
	Channels[NextFreeChannel].Playing = true;
	Channels[NextFreeChannel].Sample = sample;
	Channels[NextFreeChannel].Stereo = stereo;
	Channels[NextFreeChannel].FinishedCallback = nullptr;
	//Wyrmgus start
	Channels[NextFreeChannel].Unit.reset();
	//Wyrmgus end
	if (origin && origin->Base) {
		auto source = std::make_unique<Origin>();
		source->Base = origin->Base;
		source->Id = origin->Id;
		Channels[NextFreeChannel].Unit = std::move(source);
	}
	NextFreeChannel = next_free;

	return old_free;
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

	if (volume < 0) {
		volume = Channels[channel].Volume;
	} else {
		SDL_LockMutex(Audio.Lock);

		volume = std::min(MaxVolume, volume);
		Channels[channel].Volume = volume;

		SDL_UnlockMutex(Audio.Lock);
	}
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
	if (Preference.StereoSound == false) {
		stereo = 0;
	}
	if (channel < 0 || channel >= MaxChannels) {
		return -1;
	}

	if (stereo < -128 || stereo > 127) {
		stereo = Channels[channel].Stereo;
	} else {
		SDL_LockMutex(Audio.Lock);
		Channels[channel].Stereo = stereo;
		SDL_UnlockMutex(Audio.Lock);
	}
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

	SDL_LockMutex(Audio.Lock);
	Channels[channel].Voice = voice;
	SDL_UnlockMutex(Audio.Lock);
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
**  Get the sample playing on a channel
*/
wyrmgus::sample *GetChannelSample(int channel)
{
	if (channel < 0 || channel >= MaxChannels) {
		return nullptr;
	}
	return Channels[channel].Sample;
}

/**
**  Stop a channel
**
**  @param channel  Channel to stop
*/
void StopChannel(int channel)
{
	SDL_LockMutex(Audio.Lock);
	if (channel >= 0 && channel < MaxChannels) {
		if (Channels[channel].Playing) {
			ChannelFinished(channel);
		}
	}
	SDL_UnlockMutex(Audio.Lock);
}

/**
**  Stop all channels
*/
void StopAllChannels()
{
	SDL_LockMutex(Audio.Lock);
	for (int i = 0; i < MaxChannels; ++i) {
		if (Channels[i].Playing) {
			ChannelFinished(i);
		}
	}
	SDL_UnlockMutex(Audio.Lock);
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

namespace wyrmgus {

void sample::decrement_decoding_loop_counter()
{
	sample::decoding_loop_counter--;

	if (sample::decoding_loop_counter < sample::max_concurrent_decoders && !sample::queued_decoders.empty()) {
		std::unique_ptr<QAudioDecoder> decoder = queue::take(sample::queued_decoders);
		sample::start_decoder(std::move(decoder));
		return;
	}

	if (sample::decoding_loop_counter == 0) {
		sample::decoding_loop->quit();
	}
}


sample::sample(const std::filesystem::path &filepath)
{
	if (!std::filesystem::exists(filepath)) {
		throw std::runtime_error("Sound file \"" + filepath.string() + "\" does not exist.");
	}

	this->decode(filepath);
}

void sample::decode(const std::filesystem::path &filepath)
{
	auto decoder = std::make_unique<QAudioDecoder>();
	decoder->setSourceFilename(QString::fromStdString(filepath.string()));

	QAudioDecoder *decoder_ptr = decoder.get();

	sample::decoding_loop->connect(decoder_ptr, &QAudioDecoder::bufferReady, [this, decoder_ptr]() {
		const QAudioBuffer buffer = decoder_ptr->read();
		this->read_audio_buffer(buffer);
	});

	sample::decoding_loop->connect(decoder_ptr, &QAudioDecoder::finished, [this, decoder_ptr]() {
		this->format = decoder_ptr->audioFormat();
		sample::decrement_decoding_loop_counter();
	});

	sample::decoding_loop->connect(decoder_ptr, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), [this, decoder_ptr]() {
		throw std::runtime_error(decoder_ptr->errorString().toStdString());
	});

	sample::queue_decoder(std::move(decoder));
}

void sample::read_audio_buffer(const QAudioBuffer &buffer)
{
	if (buffer.byteCount() == 0) {
		return;
	}

	this->buffer.resize(this->get_length() + buffer.byteCount());
	std::copy_n(buffer.constData<unsigned char>(), buffer.byteCount(), this->buffer.data() + this->get_length());
	this->length += buffer.byteCount();
}

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

	SDL_LockMutex(Audio.Lock);
	if (SoundEnabled() && EffectsEnabled && sample && NextFreeChannel != MaxChannels) {
		channel = FillChannel(sample, EffectsVolume, 0, origin);
	}
	SDL_UnlockMutex(Audio.Lock);
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
	MusicChannel.FinishedCallback = callback;
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
		MusicPlaying = true;
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
		MusicPlaying = true;
		return 0;
	} else {
		DebugPrint("Could not play %s\n" _C_ file.c_str());
		return -1;
	}
}

void PlayMusicName(const std::string &name) {
	if (!IsMusicEnabled()) {
		return;
	}
	
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	SDL_LockMutex(Audio.Lock);
	oaml->PlayTrack(name.c_str());
	SDL_UnlockMutex(Audio.Lock);
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

	SDL_LockMutex(Audio.Lock);
	oaml->PlayTrackByGroupRandom(group.c_str());
	SDL_UnlockMutex(Audio.Lock);
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

	SDL_LockMutex(Audio.Lock);
	if (oaml->PlayTrackByGroupAndSubgroupRandom(group.c_str(), subgroup.c_str()) != OAML_OK) {
		oaml->PlayTrackByGroupRandom(group.c_str());
	}
	SDL_UnlockMutex(Audio.Lock);
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

	SDL_LockMutex(Audio.Lock);
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
	SDL_UnlockMutex(Audio.Lock);
#endif
}

void SetMusicCondition(int id, int value) {
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	SDL_LockMutex(Audio.Lock);
	oaml->SetCondition(id, value);
	SDL_UnlockMutex(Audio.Lock);
#endif
}

void SetMusicLayerGain(const std::string &layer, float gain) {
#ifdef USE_OAML
	if (enableOAML == false || oaml == nullptr) {
		return;
	}

	SDL_LockMutex(Audio.Lock);
	oaml->SetLayerGain(layer.c_str(), gain);
	SDL_UnlockMutex(Audio.Lock);
#endif
}

/**
**  Stop the current playing music.
*/
void StopMusic()
{
#ifdef USE_OAML
	if (enableOAML && oaml) {
		SDL_LockMutex(Audio.Lock);
		oaml->StopPlaying();
		SDL_UnlockMutex(Audio.Lock);
	}
#endif

	if (MusicPlaying) {
		MusicPlaying = false;
		if (MusicChannel.Sample) {
			SDL_LockMutex(Audio.Lock);
			MusicChannel.Sample.reset();
			SDL_UnlockMutex(Audio.Lock);
		}
	}
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
		SDL_LockMutex(Audio.Lock);
		oaml->SetVolume(MusicVolume / 255.f);
		SDL_UnlockMutex(Audio.Lock);
	}
#endif
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

	return MusicPlaying;
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

	SDL_LockMutex(Audio.Lock);
	oaml->AddTension(value);
	SDL_UnlockMutex(Audio.Lock);
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
static int InitSdlSound(int freq, int size)
{
	SDL_AudioSpec wanted;

	wanted.freq = freq;
	if (size == 8) {
		wanted.format = AUDIO_U8;
	} else if (size == 16) {
		wanted.format = AUDIO_S16SYS;
	} else {
		DebugPrint("Unexpected sample size %d\n" _C_ size);
		wanted.format = AUDIO_S16SYS;
	}
	wanted.channels = 2;
	wanted.samples = 4096;
	wanted.callback = FillAudio;
	wanted.userdata = nullptr;

	//  Open the audio device, forcing the desired format
	if (SDL_OpenAudio(&wanted, &Audio.Format) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return -1;
	}
	SDL_PauseAudio(0);
	return 0;
}

/**
**  Initialize sound card.
**
**  @return  True if failure, false if everything ok.
*/
int InitSound()
{
	//
	// Open sound device, 8bit samples, stereo.
	//
	if (InitSdlSound(44100, 16)) {
		SoundInitialized = false;
		return 1;
	}
	SoundInitialized = true;

	// ARI: The following must be done here to allow sound to work in
	// pre-start menus!
	// initialize channels
	for (int i = 0; i < MaxChannels; ++i) {
		Channels[i].Point = i + 1;
		//Wyrmgus start
		Channels[i].Sample = nullptr;
		Channels[i].Unit.reset();
		Channels[i].Volume = 0;
		Channels[i].Stereo = 0;
		Channels[i].Voice = wyrmgus::unit_sound_type::none;
		Channels[i].Playing = false;
		//Wyrmgus end
	}

	// Create mutex and cond for FillThread
	Audio.MixerBuffer = std::make_unique<int[]>(Audio.Format.samples * Audio.Format.channels);
	memset(Audio.MixerBuffer.get(), 0, Audio.Format.samples * Audio.Format.channels * sizeof(int));
	Audio.Buffer = std::make_unique<Uint8[]>(Audio.Format.size);
	memset(Audio.Buffer.get(), 0, Audio.Format.size);
	Audio.Lock = SDL_CreateMutex();
	Audio.Cond = SDL_CreateCond();
	Audio.Running = true;

	// Create thread to fill sdl audio buffer
	Audio.Thread = SDL_CreateThread(FillThread, nullptr);
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

	Audio.Running = false;
	SDL_WaitThread(Audio.Thread, nullptr);

	SDL_DestroyCond(Audio.Cond);
	SDL_DestroyMutex(Audio.Lock);

	// Mustn't call SDL_CloseAudio here, it'll be called again from SDL_Quit
	SoundInitialized = false;
	Audio.MixerBuffer.reset();
	Audio.Buffer.reset();
}
