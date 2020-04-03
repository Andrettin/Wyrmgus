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
//
//      (c) Copyright 1998-2020 by Lutz Sammer, Fabrice Rossi,
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
//

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "sound.h"

#include "action/action_resource.h"
#include "civilization.h"
#include "config.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "mod.h"
#include "missile.h"
#include "sound_server.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "video.h"
#include "widgets.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/**
**  Various sounds used in game.
*/
GameSound GameSounds;

/**
**  Selection handling
*/
struct SelectionHandling {
	Origin Source;         /// origin of the sound
	CSound *Sound;         /// last sound played by this unit
	unsigned char HowMany; /// number of sound played in this group
};

/// FIXME: docu
SelectionHandling SelectionHandler;

static int ViewPointOffset;      /// Distance to Volume Mapping
int DistanceSilent;              /// silent distance

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  "Randomly" choose a sample from a sound group.
*/
static CSample *SimpleChooseSample(const CSound &sound)
{
	if (sound.Number == ONE_SOUND) {
		return sound.Sound.OneSound;
	} else {
		//FIXME: check for errors
		//FIXME: valid only in shared memory context (FrameCounter)
		return sound.Sound.OneGroup[FrameCounter % sound.Number];
	}
}

/**
**  Choose the sample to play
*/
static CSample *ChooseSample(CSound *sound, bool selection, Origin &source)
{
	CSample *result = nullptr;

	if (!sound || !SoundEnabled()) {
		return nullptr;
	}

	if (sound->Number == TWO_GROUPS) {
		// handle a special sound (selection)
		if (SelectionHandler.Sound != nullptr && (SelectionHandler.Source.Base == source.Base && SelectionHandler.Source.Id == source.Id)) {
			if (SelectionHandler.Sound == sound->Sound.TwoGroups.First) {
				result = SimpleChooseSample(*SelectionHandler.Sound);
				SelectionHandler.HowMany++;
				if (SelectionHandler.HowMany >= 3) {
					SelectionHandler.HowMany = 0;
					SelectionHandler.Sound = sound->Sound.TwoGroups.Second;
				}
			} else {
				//FIXME: checks for error
				// check whether the second group is really a group
				if (SelectionHandler.Sound->Number > 1) {
					//Wyrmgus start
//					result = SelectionHandler.Sound->Sound.OneGroup[SelectionHandler.HowMany];
					result = SimpleChooseSample(*SelectionHandler.Sound);
					//Wyrmgus end
					SelectionHandler.HowMany++;
					if (SelectionHandler.HowMany >= SelectionHandler.Sound->Number) {
						SelectionHandler.HowMany = 0;
						SelectionHandler.Sound = sound->Sound.TwoGroups.First;
					}
				} else {
					result = SelectionHandler.Sound->Sound.OneSound;
					SelectionHandler.HowMany = 0;
					SelectionHandler.Sound = sound->Sound.TwoGroups.First;
				}
			}
		} else {
			SelectionHandler.Source = source;
			SelectionHandler.Sound = sound->Sound.TwoGroups.First;
			result = SimpleChooseSample(*SelectionHandler.Sound);
			SelectionHandler.HowMany = 1;
		}
	} else {
		// normal sound/sound group handling
		result = SimpleChooseSample(*sound);
		if (SelectionHandler.Source.Base == source.Base && SelectionHandler.Source.Id == source.Id) {
			SelectionHandler.HowMany = 0;
			SelectionHandler.Sound = nullptr;
		}
		if (selection) {
			SelectionHandler.Source = source;
		}
	}

	return result;
}

/**
**  Maps a UnitVoiceGroup to a CSound*.
**
**  @param unit    Sound initiator
**  @param voice   Type of sound wanted
**
**  @return        Sound identifier
*/
static CSound *ChooseUnitVoiceSound(const CUnit &unit, UnitVoiceGroup voice)
{
	//Wyrmgus start
	const CMapField &mf = *unit.MapLayer->Field(unit.tilePos);
	//Wyrmgus end
	switch (voice) {
		case UnitVoiceGroup::Acknowledging:
			//Wyrmgus start
//			return unit.Type->MapSound.Acknowledgement.Sound;
			if (unit.Type->MapSound.Acknowledgement.Sound) {
				return unit.Type->MapSound.Acknowledgement.Sound;
			} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				return CCivilization::Civilizations[civilization]->UnitSounds.Acknowledgement.Sound;
			} else {
				return nullptr;
			}
			//Wyrmgus end
		case UnitVoiceGroup::Attack:
			if (unit.Type->MapSound.Attack.Sound) {
				return unit.Type->MapSound.Attack.Sound;
			//Wyrmgus start
			} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				if (CCivilization::Civilizations[civilization]->UnitSounds.Attack.Sound) {
					return CCivilization::Civilizations[civilization]->UnitSounds.Attack.Sound;
				} else {
					return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
				}
			//Wyrmgus end
			} else {
				//Wyrmgus start
//				return unit.Type->MapSound.Acknowledgement.Sound;
				return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
				//Wyrmgus end
			}
		//Wyrmgus start
		case UnitVoiceGroup::Idle:
			return unit.Type->MapSound.Idle.Sound;
		case UnitVoiceGroup::Hit:
			return unit.Type->MapSound.Hit.Sound;
		case UnitVoiceGroup::Miss:
			if (unit.Type->MapSound.Miss.Sound) {
				return unit.Type->MapSound.Miss.Sound;
			} else {
				return unit.Type->MapSound.Hit.Sound;
			}
		case UnitVoiceGroup::FireMissile:
			return unit.Type->MapSound.FireMissile.Sound;
		case UnitVoiceGroup::Step:
			if (unit.Type->MapSound.StepMud.Sound && ((mf.getFlag() & MapFieldMud) || (mf.getFlag() & MapFieldSnow))) {
				return unit.Type->MapSound.StepMud.Sound;
			} else if (unit.Type->MapSound.StepDirt.Sound && ((mf.getFlag() & MapFieldDirt) || (mf.getFlag() & MapFieldIce))) {
				return unit.Type->MapSound.StepDirt.Sound;
			} else if (unit.Type->MapSound.StepGravel.Sound && mf.getFlag() & MapFieldGravel) {
				return unit.Type->MapSound.StepGravel.Sound;
			} else if (unit.Type->MapSound.StepGrass.Sound && ((mf.getFlag() & MapFieldGrass) || (mf.getFlag() & MapFieldStumps))) {
				return unit.Type->MapSound.StepGrass.Sound;
			} else if (unit.Type->MapSound.StepStone.Sound && mf.getFlag() & MapFieldStoneFloor) {
				return unit.Type->MapSound.StepStone.Sound;
			} else {
				return unit.Type->MapSound.Step.Sound;
			}
		case UnitVoiceGroup::Used:
			return unit.Type->MapSound.Used.Sound;
		//Wyrmgus end
		case UnitVoiceGroup::Build:
			//Wyrmgus start
//			return unit.Type->MapSound.Build.Sound;
			if (unit.Type->MapSound.Build.Sound) {
				return unit.Type->MapSound.Build.Sound;
			//Wyrmgus start
			} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				if (CCivilization::Civilizations[civilization]->UnitSounds.Build.Sound) {
					return CCivilization::Civilizations[civilization]->UnitSounds.Build.Sound;
				} else {
					return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
				}
			//Wyrmgus end
			} else {
				return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
			}
			//Wyrmgus end
		case UnitVoiceGroup::Ready:
			//Wyrmgus start
//			return unit.Type->MapSound.Ready.Sound;
			if (unit.Type->MapSound.Ready.Sound) {
				return unit.Type->MapSound.Ready.Sound;
			} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				return CCivilization::Civilizations[civilization]->UnitSounds.Ready.Sound;
			} else {
				return nullptr;
			}
			//Wyrmgus end
		case UnitVoiceGroup::Selected:
			//Wyrmgus start
//			return unit.Type->MapSound.Selected.Sound;
			if (unit.Type->MapSound.Selected.Sound) {
				return unit.Type->MapSound.Selected.Sound;
			} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				return CCivilization::Civilizations[civilization]->UnitSounds.Selected.Sound;
			} else {
				return nullptr;
			}
			//Wyrmgus end
		case UnitVoiceGroup::HelpMe:
			//Wyrmgus start
//			return unit.Type->MapSound.Help.Sound;
			if (unit.Type->MapSound.Help.Sound) {
				return unit.Type->MapSound.Help.Sound;
			} else if (unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				if (unit.Type->BoolFlag[BUILDING_INDEX].value && CCivilization::Civilizations[civilization]->UnitSounds.HelpTown.Sound) {
					return CCivilization::Civilizations[civilization]->UnitSounds.HelpTown.Sound;
				} else {
					return CCivilization::Civilizations[civilization]->UnitSounds.Help.Sound;
				}
			} else {
				return nullptr;
			}
			//Wyrmgus end
		case UnitVoiceGroup::Dying:
			if (unit.Type->MapSound.Dead[unit.DamagedType].Sound) {
				return unit.Type->MapSound.Dead[unit.DamagedType].Sound;
			} else {
				return unit.Type->MapSound.Dead[ANIMATIONS_DEATHTYPES].Sound;
			}
		case UnitVoiceGroup::WorkCompleted:
			return GameSounds.WorkComplete[CPlayer::GetThisPlayer()->Race].Sound;
		case UnitVoiceGroup::Building:
			return GameSounds.BuildingConstruction[CPlayer::GetThisPlayer()->Race].Sound;
		case UnitVoiceGroup::Docking:
			return GameSounds.Docking.Sound;
		case UnitVoiceGroup::Repairing:
			if (unit.Type->MapSound.Repair.Sound) {
				return unit.Type->MapSound.Repair.Sound;
			//Wyrmgus start
			} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
				int civilization = unit.Type->Civilization;
				if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
					civilization = unit.Player->Race;
				}
				if (CCivilization::Civilizations[civilization]->UnitSounds.Repair.Sound) {
					return CCivilization::Civilizations[civilization]->UnitSounds.Repair.Sound;
				} else {
					return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
				}
			//Wyrmgus end
			} else {
				//Wyrmgus start
//				return unit.Type->MapSound.Acknowledgement.Sound;
				return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
				//Wyrmgus end
			}
		case UnitVoiceGroup::Harvesting:
			for (size_t i = 0; i != unit.Orders.size(); ++i) {
				if (unit.Orders[i]->Action == UnitAction::Resource) {
					COrder_Resource &order = dynamic_cast<COrder_Resource &>(*unit.Orders[i]);
					if (unit.Type->MapSound.Harvest[order.GetCurrentResource()].Sound) {
						return unit.Type->MapSound.Harvest[order.GetCurrentResource()].Sound;
					//Wyrmgus start
					} else if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->Civilization != -1) {
						int civilization = unit.Type->Civilization;
						if (unit.Player->Race != -1 && unit.Player->Race != civilization && unit.Player->Faction != -1 && unit.Type->Slot == PlayerRaces.GetFactionClassUnitType(unit.Player->Faction, unit.Type->Class)) {
							civilization = unit.Player->Race;
						}
						if (CCivilization::Civilizations[civilization]->UnitSounds.Harvest[order.GetCurrentResource()].Sound) {
							return CCivilization::Civilizations[civilization]->UnitSounds.Harvest[order.GetCurrentResource()].Sound;
						} else {
							return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
						}
					//Wyrmgus end
					} else {
						//Wyrmgus start
//						return unit.Type->MapSound.Acknowledgement.Sound;
						return ChooseUnitVoiceSound(unit, UnitVoiceGroup::Acknowledging);
						//Wyrmgus end
					}
				}
			}
	}

	return NO_SOUND;
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
	if (d <= ViewPointOffset || range == INFINITE_SOUND_RANGE) {
		return MaxVolume;
	} else {
		if (range) {
			d -= ViewPointOffset;
			int d_tmp = d * MAX_SOUND_RANGE;
			int range_tmp = DistanceSilent * range;
			if (d_tmp > range_tmp) {
				return 0;
			} else {
				return (unsigned char)((range_tmp - d_tmp) * MAX_SOUND_RANGE / range_tmp);
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
	int stereo = ((unit.tilePos.x * CMap::Map.GetCurrentPixelTileSize().x + unit.Type->TileSize.x * CMap::Map.GetCurrentPixelTileSize().x / 2 +
				   unit.IX - UI.SelectedViewport->MapPos.x * CMap::Map.GetCurrentPixelTileSize().x) * 256 /
				  ((UI.SelectedViewport->MapWidth - 1) * CMap::Map.GetCurrentPixelTileSize().x)) - 128;
	clamp(&stereo, -128, 127);
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
void PlayUnitSound(const CUnit &unit, UnitVoiceGroup voice)
{
	if (!UI.CurrentMapLayer || unit.MapLayer != UI.CurrentMapLayer) {
		return;
	}
	
	if (unit.Variable[STUN_INDEX].Value > 0 && voice != UnitVoiceGroup::Hit && voice != UnitVoiceGroup::Miss && voice != UnitVoiceGroup::FireMissile && voice != UnitVoiceGroup::Step && voice != UnitVoiceGroup::Dying) { //don't speak if stunned
		return;
	}
	
	CSound *sound = ChooseUnitVoiceSound(unit, voice);
	if (!sound) {
		return;
	}

	bool selection = (voice == UnitVoiceGroup::Selected || voice == UnitVoiceGroup::Building);
	Origin source = {&unit, unsigned(UnitNumber(unit))};
	
	//Wyrmgus start
//	if (UnitSoundIsPlaying(&source)) {
	if (voice != UnitVoiceGroup::Hit && voice != UnitVoiceGroup::Miss && voice != UnitVoiceGroup::FireMissile && voice != UnitVoiceGroup::Step && UnitSoundIsPlaying(&source)) {
	//Wyrmgus end
		return;
	}

	int channel = PlaySample(ChooseSample(sound, selection, source), &source);
	if (channel == -1) {
		return;
	}
	//Wyrmgus start
//	SetChannelVolume(channel, CalculateVolume(false, ViewPointDistanceToUnit(unit), sound->Range));
	SetChannelVolume(channel, CalculateVolume(false, ViewPointDistanceToUnit(unit), sound->Range) * sound->VolumePercent / 100);
	//Wyrmgus end
	SetChannelStereo(channel, CalculateStereo(unit));
	//Wyrmgus start
	SetChannelVoiceGroup(channel, voice);
	//Wyrmgus end
}

/**
**  Ask to the sound server to play a sound attached to a unit. The
**  sound server may discard the sound if needed (e.g., when the same
**  unit is already speaking).
**
**  @param unit   Sound initiator, unit speaking
**  @param sound  Sound to be generated
*/
void PlayUnitSound(const CUnit &unit, CSound *sound)
{
	//Wyrmgus start
	if (!&unit) {
		fprintf(stderr, "Error in PlayUnitSound: unit is null.\n");
		return;
	}
	
	if (!sound) {
		return;
	}
	//Wyrmgus start
	if (unit.MapLayer != UI.CurrentMapLayer) {
		return;
	}
	//Wyrmgus end
	Origin source = {&unit, unsigned(UnitNumber(unit))};
	//Wyrmgus start
//	unsigned char volume = CalculateVolume(false, ViewPointDistanceToUnit(unit), sound->Range);
	unsigned char volume = CalculateVolume(false, ViewPointDistanceToUnit(unit), sound->Range) * sound->VolumePercent / 100;
	//Wyrmgus end
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
void PlayMissileSound(const Missile &missile, CSound *sound)
{
	if (!sound) {
		return;
	}
	int stereo = ((missile.position.x + (missile.Type->G ? missile.Type->G->Width / 2 : 0) +
				   UI.SelectedViewport->MapPos.x * CMap::Map.GetCurrentPixelTileSize().x) * 256 /
				  ((UI.SelectedViewport->MapWidth - 1) * CMap::Map.GetCurrentPixelTileSize().x)) - 128;
	clamp(&stereo, -128, 127);

	Origin source = {nullptr, 0};
	//Wyrmgus start
//	unsigned char volume = CalculateVolume(false, ViewPointDistanceToMissile(missile), sound->Range);
	unsigned char volume = CalculateVolume(false, ViewPointDistanceToMissile(missile), sound->Range) * sound->VolumePercent / 100;
	//Wyrmgus end
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
void PlayGameSound(CSound *sound, unsigned char volume, bool always)
{
	if (!sound) {
		return;
	}
	Origin source = {nullptr, 0};

	CSample *sample = ChooseSample(sound, false, source);

	if (!always && SampleIsPlaying(sample)) {
		return;
	}

	int channel = PlaySample(sample);
	if (channel == -1) {
		return;
	}
	//Wyrmgus start
//	SetChannelVolume(channel, CalculateVolume(true, volume, sound->Range));
	SetChannelVolume(channel, CalculateVolume(true, volume, sound->Range) * sound->VolumePercent / 100);
	//Wyrmgus end
}

static std::map<int, LuaActionListener *> ChannelMap;

/**
**  Callback for PlaySoundFile
*/
static void PlaySoundFileCallback(int channel)
{
	LuaActionListener *listener = ChannelMap[channel];
	if (listener != nullptr) {
		listener->action("");
		ChannelMap[channel] = nullptr;
	}
	delete GetChannelSample(channel);
}

/**
**  Play a sound file
**
**  @param name      Filename of a sound to play
**  @param listener  Optional lua callback
**
**  @return          Channel number the sound is playing on, -1 for error
*/
int PlayFile(const std::string &name, LuaActionListener *listener)
{
	int channel = -1;
	CSample *sample = LoadSample(name);

	if (sample) {
		channel = PlaySample(sample);
		if (channel != -1) {
			SetChannelVolume(channel, MaxVolume);
			SetChannelFinishedCallback(channel, PlaySoundFileCallback);
			ChannelMap[channel] = listener;
		}
	}
	return channel;
}

/**
**  Ask the sound server to change the range of a sound.
**
**  @param sound  the id of the sound to modify.
**  @param range  the new range for this sound.
*/
void SetSoundRange(CSound *sound, unsigned char range)
{
	if (sound != NO_SOUND) {
		sound->Range = range;
	}
}

//Wyrmgus start
/**
**  Ask the sound server to change the volume percent of a sound.
**
**  @param sound  the id of the sound to modify.
**  @param volume_percent  the new volume percent for this sound.
*/
void SetSoundVolumePercent(CSound *sound, int volume_percent)
{
	if (sound != NO_SOUND) {
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
CSound *RegisterSound(const std::vector<std::string> &files)
{
	CSound *id = new CSound;
	size_t number = files.size();

	if (number > 1) { // load a sound group
		id->Sound.OneGroup = new CSample *[number];
		memset(id->Sound.OneGroup, 0, sizeof(CSample *) * number);
		id->Number = number;
		for (unsigned int i = 0; i < number; ++i) {
			id->Sound.OneGroup[i] = LoadSample(files[i]);
			if (!id->Sound.OneGroup[i]) {
				//delete[] id->Sound.OneGroup;
				delete id;
				return NO_SOUND;
			}
		}
	} else { // load a unique sound
		id->Sound.OneSound = LoadSample(files[0]);
		if (!id->Sound.OneSound) {
			delete id;
			return NO_SOUND;
		}
		id->Number = ONE_SOUND;
	}
	id->Range = MAX_SOUND_RANGE;
	//Wyrmgus start
	id->VolumePercent = 100;
	//Wyrmgus end
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
CSound *RegisterTwoGroups(CSound *first, CSound *second)
{
	if (first == NO_SOUND || second == NO_SOUND) {
		return NO_SOUND;
	}
	CSound *id = new CSound;
	id->Number = TWO_GROUPS;
	id->Sound.TwoGroups.First = first;
	id->Sound.TwoGroups.Second = second;
	id->Range = MAX_SOUND_RANGE;
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
	// let's map game sounds, look if already setup in ccl.

	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.PlacementError[i].Sound) {
			GameSounds.PlacementError[i].MapSound();
		}
	}

	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.PlacementSuccess[i].Sound) {
			GameSounds.PlacementSuccess[i].MapSound();
		}
	}

	if (!GameSounds.Click.Sound) {
		GameSounds.Click.MapSound();
	}
	if (!GameSounds.Docking.Sound) {
		GameSounds.Docking.MapSound();
	}

	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.BuildingConstruction[i].Sound) {
			GameSounds.BuildingConstruction[i].MapSound();
		}
	}
	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.WorkComplete[i].Sound) {
			GameSounds.WorkComplete[i].MapSound();
		}
	}
	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.ResearchComplete[i].Sound) {
			GameSounds.ResearchComplete[i].MapSound();
		}
	}
	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		for (unsigned int j = 0; j < MaxCosts; ++j) {
			if (!GameSounds.NotEnoughRes[i][j].Sound) {
				GameSounds.NotEnoughRes[i][j].MapSound();
			}
		}
	}
	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.NotEnoughFood[i].Sound) {
			GameSounds.NotEnoughFood[i].MapSound();
		}
	}
	for (size_t i = 0; i < CCivilization::Civilizations.size(); ++i) {
		if (!GameSounds.Rescue[i].Sound) {
			GameSounds.Rescue[i].MapSound();
		}
	}
	if (!GameSounds.ChatMessage.Sound) {
		GameSounds.ChatMessage.MapSound();
	}

	int MapWidth = (UI.MapArea.EndX - UI.MapArea.X + CMap::Map.GetCurrentPixelTileSize().x) / CMap::Map.GetCurrentPixelTileSize().x;
	int MapHeight = (UI.MapArea.EndY - UI.MapArea.Y + CMap::Map.GetCurrentPixelTileSize().y) / CMap::Map.GetCurrentPixelTileSize().y;
	DistanceSilent = 3 * std::max<int>(MapWidth, MapHeight);
	ViewPointOffset = std::max<int>(MapWidth / 2, MapHeight / 2);
}

CSound::~CSound()
{
	if (this->Number == ONE_SOUND) {
		delete Sound.OneSound;
	} else if (this->Number == TWO_GROUPS) {
	} else {
		//Wyrmgus start
//		for (int i = 0; i < this->Number; ++i) {
		for (unsigned int i = 0; i < this->Number; ++i) {
		//Wyrmgus end
			delete this->Sound.OneGroup[i];
			this->Sound.OneGroup[i] = nullptr;
		}
		delete[] this->Sound.OneGroup;
	}
}

void CSound::ProcessConfigData(const CConfigData *config_data)
{
	std::string ident = config_data->Ident;
	ident = FindAndReplaceString(ident, "_", "-");
	std::vector<std::string> files;
	std::vector<CSound *> group_sounds; //sounds for sound group
	unsigned char range = 0;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "file") {
			std::string file = CMod::GetCurrentModPath() + value;
			files.push_back(file);
		} else if (key == "group_sound") {
			value = FindAndReplaceString(value, "_", "-");
			CSound *group_sound = SoundForName(value);
			if (group_sound) {
				group_sounds.push_back(group_sound);
			} else {
				fprintf(stderr, "Invalid sound: \"%s\".\n", value.c_str());
			}
		} else if (key == "range") {
			range = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid sound property: \"%s\".\n", key.c_str());
		}
	}
	
	CSound *sound = nullptr;
	if (group_sounds.size() >= 2) {
		sound = MakeSoundGroup(ident, group_sounds[0], group_sounds[1]);
	} else {
		sound = MakeSound(ident, files);
	}
	
	if (range != 0) {
		SetSoundRange(sound, range);
	}
}
