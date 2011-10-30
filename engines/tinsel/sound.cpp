/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 * sound functionality
 */

#include "tinsel/sound.h"

#include "tinsel/dw.h"
#include "tinsel/config.h"
#include "tinsel/music.h"
#include "tinsel/strres.h"
#include "tinsel/tinsel.h"
#include "tinsel/sysvar.h"
#include "tinsel/background.h"

#include "common/endian.h"
#include "common/file.h"
#include "common/system.h"

#include "sound/mixer.h"
#include "sound/adpcm.h"

#include "gui/message.h"

namespace Tinsel {

extern LANGUAGE sampleLanguage;

//--------------------------- General data ----------------------------------

SoundManager::SoundManager(TinselEngine *vm) :
	//_vm(vm),	// TODO: Enable this once global _vm var is gone
	_sampleIndex(0), _sampleIndexLen(0) {

	for (int i = 0; i < kNumChannels; i++)
		_channels[i].sampleNum = _channels[i].subSample = -1;
}

SoundManager::~SoundManager() {
	free(_sampleIndex);
}

/**
 * Plays the specified sample through the sound driver.
 * @param id			Identifier of sample to be played
 * @param type			type of sound (voice or sfx)
 * @param handle		sound handle
 */
bool SoundManager::playSample(int id, Audio::Mixer::SoundType type, Audio::SoundHandle *handle) {
	// Floppy version has no sample file
	if (_vm->getFeatures() & GF_FLOPPY)
		return false;

	// no sample driver?
	if (!_vm->_mixer->isReady())
		return false;

	Channel &curChan = _channels[kChannelTinsel1];

	// stop any currently playing sample
	_vm->_mixer->stopHandle(curChan.handle);

	// make sure id is in range
	assert(id > 0 && id < _sampleIndexLen);

	curChan.sampleNum = id;
	curChan.subSample = 0;

	// get file offset for this sample
	uint32 dwSampleIndex = _sampleIndex[id];

	// move to correct position in the sample file
	_sampleStream.seek(dwSampleIndex);
	if (_sampleStream.ioFailed() || (uint32)_sampleStream.pos() != dwSampleIndex)
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

	// read the length of the sample
	uint32 sampleLen = _sampleStream.readUint32LE();
	if (_sampleStream.ioFailed())
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

	// allocate a buffer
	void *sampleBuf = malloc(sampleLen);
	assert(sampleBuf);

	// read all of the sample
	if (_sampleStream.read(sampleBuf, sampleLen) != sampleLen)
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

	// FIXME: Should set this in a different place ;)
	_vm->_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, volSound);
	//_vm->_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, soundVolumeMusic);
	_vm->_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, volVoice);


	// play it
	_vm->_mixer->playRaw(type, &curChan.handle, sampleBuf, sampleLen, 22050,
						 Audio::Mixer::FLAG_AUTOFREE | Audio::Mixer::FLAG_UNSIGNED);

	if (handle)
		*handle = curChan.handle;

	return true;
}

bool SoundManager::playSample(int id, int sub, bool bLooped, int x, int y, int priority,
		Audio::Mixer::SoundType type, Audio::SoundHandle *handle) {

	// Floppy version has no sample file
	if (_vm->getFeatures() & GF_FLOPPY)
		return false;

	// no sample driver?
	if (!_vm->_mixer->isReady())
		return false;

	Channel *curChan;

	uint8 sndVol = 255;

	// Sample on screen?
	if (!offscreenChecks(x, y))
		return false;

	// If that sample is already playing, stop it
	stopSpecSample(id, sub);

	if (type == Audio::Mixer::kSpeechSoundType) {
		curChan = &_channels[kChannelTalk];
	} else if (type == Audio::Mixer::kSFXSoundType) {
		uint32 oldestTime = g_system->getMillis();
		int	oldestChan = kChannelSFX;

		int chan;
		for (chan = kChannelSFX; chan < kNumChannels; chan++) {
			if (!_vm->_mixer->isSoundHandleActive(_channels[chan].handle))
				break;

			if ((_channels[chan].lastStart <  oldestTime) &&
			    (_channels[chan].priority  <= priority)) {

				oldestTime = _channels[chan].lastStart;
				oldestChan = chan;
			}
		}

		if (chan == kNumChannels) {
			if (_channels[oldestChan].priority > priority) {
				warning("playSample: No free channel");
				return false;
			}

			chan = oldestChan;
		}

		if (_vm->_pcmMusic->isDimmed() && SysVar(SYS_SceneFxDimFactor))
			sndVol = 255 - 255/SysVar(SYS_SceneFxDimFactor);

		curChan = &_channels[chan];
	} else {
		warning("playSample: Unknown SoundType");
		return false;
	}

	// stop any currently playing sample
	_vm->_mixer->stopHandle(curChan->handle);

	// make sure id is in range
	assert(id > 0 && id < _sampleIndexLen);

	// get file offset for this sample
	uint32 dwSampleIndex = _sampleIndex[id];

	if (dwSampleIndex == 0) {
		warning("Tinsel2 playSample, non-existant sample %d", id);
		return false;
	}

	// move to correct position in the sample file
	_sampleStream.seek(dwSampleIndex);
	if (_sampleStream.ioFailed() || (uint32)_sampleStream.pos() != dwSampleIndex)
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

	// read the length of the sample
	uint32 sampleLen = _sampleStream.readUint32LE();
	if (_sampleStream.ioFailed())
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

	if (sampleLen & 0x80000000) {
		// Has sub samples

		int32 numSubs = sampleLen & ~0x80000000;

		assert(sub >= 0 && sub < numSubs);

		// Skipping
		for (int32 i = 0; i < sub; i++) {
			sampleLen = _sampleStream.readUint32LE();
			_sampleStream.skip(sampleLen);
			if (_sampleStream.ioFailed())
				error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));
		}
		sampleLen = _sampleStream.readUint32LE();
		if (_sampleStream.ioFailed())
			error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));
	}

	debugC(DEBUG_DETAILED, kTinselDebugSound, "Playing sound %d.%d, %d bytes at %d (pan %d)", id, sub, sampleLen,
			_sampleStream.pos(), getPan(x));

	// allocate a buffer
	byte *sampleBuf = (byte *) malloc(sampleLen);
	assert(sampleBuf);

	// read all of the sample
	if (_sampleStream.read(sampleBuf, sampleLen) != sampleLen)
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

	Common::MemoryReadStream *sampleStream =
		new Common::MemoryReadStream(sampleBuf, sampleLen, true);
	Audio::AudioStream *_stream =
		makeADPCMStream(sampleStream, true, sampleLen, Audio::kADPCMTinsel6, 22050, 1, 24);

	// FIXME: Should set this in a different place ;)
	_vm->_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, volSound);
	//_vm->_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, soundVolumeMusic);
	_vm->_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, volVoice);

	curChan->sampleNum = id;
	curChan->subSample = sub;
	curChan->looped = bLooped;
	curChan->x = x;
	curChan->y = y;
	curChan->priority = priority;
	curChan->lastStart = g_system->getMillis();
	//                         /---Compression----\    Milis   BytesPerSecond
	curChan->timeDuration = (((sampleLen * 64) / 25) * 1000) / (22050 * 2);

	// Play it
	_vm->_mixer->playInputStream(type, &curChan->handle, _stream);
	_vm->_mixer->setChannelVolume(curChan->handle, sndVol);
	_vm->_mixer->setChannelBalance(curChan->handle, getPan(x));

	if (handle)
		*handle = curChan->handle;

	return true;
}

/**
 * Returns FALSE if sample doesn't need playing
 */
bool SoundManager::offscreenChecks(int x, int &y)
{
	// No action if no x specification
	if (x == -1)
		return true;

	// convert x to offset from screen centre
	x -= PlayfieldGetCentreX(FIELD_WORLD);

	if (x < -SCREEN_WIDTH || x > SCREEN_WIDTH) {
		// A long way offscreen, ignore it
		return false;
	} else if (x < -SCREEN_WIDTH/2 || x > SCREEN_WIDTH/2) {
		// Off-screen, attennuate it

		y = (y > 0) ? (y / 2) : 50;

		return true;
	} else
		return true;
}

int8 SoundManager::getPan(int x) {

	if (x == -1)
		return 0;

	x -= PlayfieldGetCentreX(FIELD_WORLD);

	if (x == 0)
		return 0;

	if (x < 0) {
		if (x < (-SCREEN_WIDTH / 2))
			return -127;

		x = (-x * 127) / (SCREEN_WIDTH / 2);

		return 0 - x;
	}

	if (x > (SCREEN_WIDTH / 2))
		return 127;

	x = (x * 127) / (SCREEN_WIDTH / 2);

	return x;
}

/**
 * Returns TRUE if there is a sample for the specified sample identifier.
 * @param id			Identifier of sample to be checked
 */
bool SoundManager::sampleExists(int id) {
	if (_vm->_mixer->isReady()) 	{
		// make sure id is in range
		if (id > 0 && id < _sampleIndexLen) {
			// check for a sample index
			if (_sampleIndex[id])
				return true;
		}
	}

	// no sample driver or no sample
	return false;
}

/**
 * Returns true if a sample is currently playing.
 */
bool SoundManager::sampleIsPlaying(int id) {
	if (!TinselV2)
		return _vm->_mixer->isSoundHandleActive(_channels[kChannelTinsel1].handle);

	for (int i = 0; i < kNumChannels; i++)
		if (_channels[i].sampleNum == id)
			if (_vm->_mixer->isSoundHandleActive(_channels[i].handle))
				return true;

	return false;
}

/**
 * Stops any currently playing sample.
 */
void SoundManager::stopAllSamples(void) {
	// stop currently playing sample

	if (!TinselV2) {
		_vm->_mixer->stopHandle(_channels[kChannelTinsel1].handle);
		return;
	}

	for (int i = 0; i < kNumChannels; i++)
		_vm->_mixer->stopHandle(_channels[i].handle);
}

void SoundManager::stopSpecSample(int id, int sub) {
	debugC(DEBUG_DETAILED, kTinselDebugSound, "stopSpecSample(%d, %d)", id, sub);

	if (!TinselV2) {
		if (_channels[kChannelTinsel1].sampleNum == id)
			_vm->_mixer->stopHandle(_channels[kChannelTinsel1].handle);
		return;
	}

	for (int i = kChannelTalk; i < kNumChannels; i++) {
		if ((_channels[i].sampleNum == id) && (_channels[i].subSample == sub))
			_vm->_mixer->stopHandle(_channels[i].handle);
	}
}

void SoundManager::setSFXVolumes(uint8 volume) {
	if (!TinselV2)
		return;

	for (int i = kChannelSFX; i < kNumChannels; i++)
		_vm->_mixer->setChannelVolume(_channels[i].handle, volume);
}

/**
 * Opens and inits all sound sample files.
 */
void SoundManager::openSampleFiles(void) {
	// Floppy and demo versions have no sample files
	if (_vm->getFeatures() & GF_FLOPPY || _vm->getFeatures() & GF_DEMO)
		return;

	Common::File f;

	if (_sampleIndex)
		// already allocated
		return;

	// open sample index file in binary mode
	if (f.open(_vm->getSampleIndex(sampleLanguage))) 	{
		// get length of index file
		f.seek(0, SEEK_END);		// move to end of file
		_sampleIndexLen = f.pos();	// get file pointer
		f.seek(0, SEEK_SET);		// back to beginning

		if (_sampleIndex == NULL) {
			// allocate a buffer for the indices
			_sampleIndex = (uint32 *)malloc(_sampleIndexLen);

			// make sure memory allocated
			if (_sampleIndex == NULL) {
				// disable samples if cannot alloc buffer for indices
				// TODO: Disabled sound if we can't load the sample index?
				return;
			}
		}

		// load data
		if (f.read(_sampleIndex, _sampleIndexLen) != (uint32)_sampleIndexLen)
			// file must be corrupt if we get to here
			error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));

#ifdef SCUMM_BIG_ENDIAN
		// Convert all ids from LE to native format
		for (uint i = 0; i < _sampleIndexLen / sizeof(uint32); ++i) {
			_sampleIndex[i] = READ_LE_UINT32(_sampleIndex + i);
		}
#endif

		// close the file
		f.close();

		// convert file size to size in DWORDs
		_sampleIndexLen /= sizeof(uint32);
	} else {
		char buf[50];
		sprintf(buf, CANNOT_FIND_FILE, _vm->getSampleIndex(sampleLanguage));
		GUI::MessageDialog dialog(buf, "OK");
 	 	dialog.runModal();

		error(CANNOT_FIND_FILE, _vm->getSampleIndex(sampleLanguage));
	}

	// open sample file in binary mode
	if (!_sampleStream.open(_vm->getSampleFile(sampleLanguage))) {
		char buf[50];
		sprintf(buf, CANNOT_FIND_FILE, _vm->getSampleFile(sampleLanguage));
		GUI::MessageDialog dialog(buf, "OK");
 	 	dialog.runModal();

		error(CANNOT_FIND_FILE, _vm->getSampleFile(sampleLanguage));
	}

/*
	// gen length of the largest sample
	sampleBuffer.size = _sampleStream.readUint32LE();
	if (_sampleStream.ioFailed())
		error(FILE_IS_CORRUPT, _vm->getSampleFile(sampleLanguage));
*/
}

void SoundManager::closeSampleStream(void) {
	_sampleStream.close();
	free(_sampleIndex);
	_sampleIndex = 0;
	_sampleIndexLen = 0;
}

} // end of namespace Tinsel
