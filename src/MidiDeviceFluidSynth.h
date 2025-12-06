/*********************************************************************************/
/*!
@file           MidiDeviceFluidSynth.h

@brief          xxxxxx.

@author         L. J. Barman

    Copyright (c)   2008-2020, L. J. Barman, all rights reserved

    This file is part of the PianoBooster application

    PianoBooster is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PianoBooster is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PianoBooster.  If not, see <http://www.gnu.org/licenses/>.

*/
/*********************************************************************************/

#ifndef __MIDI_DEVICE_FLUIDSYNTH_H__
#define __MIDI_DEVICE_FLUIDSYNTH_H__

#include "MidiDeviceBase.h"

#include <fluidsynth.h>
#include <memory>

#define FLUID_DEFAULT_GAIN 80

class CMidiDeviceFluidSynth : public CMidiDeviceBase
{
    void init() override;
    //! add a midi event to be played immediately
    void playMidiEvent(const CMidiEvent & event) override;
    int checkMidiInput() override;
    CMidiEvent readMidiInput() override;
    QStringList getMidiPortList(midiType_t type) override;

    bool openMidiPort(midiType_t type, const QString &portName) override;
    void closeMidiPort(midiType_t type, int index) override;

    bool validMidiConnection() override {return m_validConnection;}

    // based on the fluid synth settings
    int     midiSettingsSetStr(const QString &name, const QString &str) override;
    int     midiSettingsSetNum(const QString &name, double val) override;
    int     midiSettingsSetInt(const QString &name, int val) override;
    QString midiSettingsGetStr(const QString &name) override;
    double  midiSettingsGetNum(const QString &name) override;
    int     midiSettingsGetInt(const QString &name) override;

public:
    CMidiDeviceFluidSynth();
    ~CMidiDeviceFluidSynth() override;

    static QString getFluidInternalName()
    {
        return QString(tr("Internal Sound") + " " + FLUID_NAME );
    }

private:
    struct FluidSettingsDeleter {
        void operator()(fluid_settings_t* settings) const { if (settings) delete_fluid_settings(settings); }
    };
    struct FluidSynthDeleter {
        void operator()(fluid_synth_t* synth) const { if (synth) delete_fluid_synth(synth); }
    };
    struct FluidAudioDeleter {
        void operator()(fluid_audio_driver_t* driver) const { if (driver) delete_fluid_audio_driver(driver); }
    };

    static constexpr const char* FLUID_NAME = "(FluidSynth)";
    unsigned char m_savedRawBytes[40]; // Raw data is used for used for a SYSTEM_EVENT
    unsigned int m_rawDataIndex;

    std::unique_ptr<fluid_settings_t, FluidSettingsDeleter> m_fluidSettings;
    std::unique_ptr<fluid_synth_t, FluidSynthDeleter> m_synth;
    std::unique_ptr<fluid_audio_driver_t, FluidAudioDeleter> m_audioDriver;
    int m_soundFontId;
    bool m_validConnection;
};

#endif //__MIDI_DEVICE_FLUIDSYNTH_H__
