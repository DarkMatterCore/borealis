/*
    Copyright 2021 natinusala
    Copyright 2020-2021 p-sam

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/logger.hpp>
#include <borealis/platforms/switch/switch_audio.hpp>

#include <nxdt_bfsar.h>

namespace brls
{

const std::string SOUNDS_MAP[_SOUND_MAX] = {
    "", // SOUND_NONE
    "SeBtnFocus", // SOUND_FOCUS_CHANGE
    "SeKeyErrorCursor", // SOUND_FOCUS_ERROR
    "SeBtnDecide", // SOUND_CLICK
    "SeNaviFocus", // SOUND_FOCUS_SIDEBAR
    "SeKeyError", // SOUND_CLICK_ERROR
    "SeUnlockKeyZR", // SOUND_HONK
    "SeNaviDecide", // SOUND_CLICK_SIDEBAR
};

SwitchAudioPlayer::SwitchAudioPlayer()
{
    // Init the sounds array
    for (size_t sound = 0; sound < _SOUND_MAX; sound++)
        this->sounds[sound] = PLSR_PLAYER_INVALID_SOUND;

    // Get BFSAR file path
    const char *bfsarPath = bfsarGetFilePath();
    if (!bfsarPath)
    {
        Logger::error("Failed to get BRSAR file path");
        return;
    }

    // Init pulsar player
    PLSR_RC rc = plsrPlayerInit();
    if (PLSR_RC_FAILED(rc))
    {
        Logger::error("Unable to init Pulsar player: {:#x}", rc);
        return;
    }

    // Open qlaunch BFSAR
    rc = plsrBFSAROpen(bfsarPath, &this->qlaunchBfsar);
    if (PLSR_RC_FAILED(rc))
    {
        Logger::error("Unable to open qlaunch BFSAR: {:#x}", rc);
        plsrPlayerExit();
        return;
    }

    // Good to go~
    this->init = true;
}

bool SwitchAudioPlayer::load(enum Sound sound)
{
    if (!this->init)
        return false;

    if (sound == SOUND_NONE)
        return true;

    if (this->sounds[sound] != PLSR_PLAYER_INVALID_SOUND)
        return true;

    std::string soundName = SOUNDS_MAP[sound];

    if (soundName == "")
        return false; // unimplemented sound

    Logger::debug("Loading sound {}: {}", sound, soundName);

    PLSR_RC rc = plsrPlayerLoadSoundByName(&this->qlaunchBfsar, soundName.c_str(), &this->sounds[sound]);
    if (PLSR_RC_FAILED(rc))
    {
        Logger::warning("Unable to load sound {}: {:#x}", soundName, rc);
        this->sounds[sound] = PLSR_PLAYER_INVALID_SOUND;
        return false;
    }

    return true;
}

bool SwitchAudioPlayer::play(enum Sound sound)
{
    if (!this->init)
        return false;

    if (sound == SOUND_NONE)
        return true;

    // Load the sound if needed
    if (this->sounds[sound] == PLSR_PLAYER_INVALID_SOUND)
    {
        if (!this->load(sound))
            return false;
    }

    // Play the sound
    PLSR_RC rc = plsrPlayerPlay(this->sounds[sound]);
    if (PLSR_RC_FAILED(rc))
    {
        Logger::error("Unable to play sound {}: {:#x}", sound, rc);
        return false;
    }

    return true;
}

SwitchAudioPlayer::~SwitchAudioPlayer()
{
    if (!this->init)
        return;

    // Unload all sounds
    for (size_t sound = 0; sound < _SOUND_MAX; sound++)
    {
        if (this->sounds[sound] != PLSR_PLAYER_INVALID_SOUND)
            plsrPlayerFree(this->sounds[sound]);
    }

    // Close the archive and exit player
    plsrBFSARClose(&this->qlaunchBfsar);
    plsrPlayerExit();
}

} // namespace brls
