/*********************************************************************************/
/*!
@file           Song.cpp

@brief          xxxxx.

@author         L. J. Barman

    Copyright (c)   2008-2020, L. J. Barman and others, all rights reserved

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

#include "Song.h"
#include "Score.h"
#include "TrackList.h"
#include "MidiFile.h"

CSong::CSong()
    : m_midiFile(new CMidiFile),
      m_findChord(),
      m_reachedMidiEof(false),
      m_fakeChord(),
      m_trackList(new CTrackList),
      m_songTitle(),
      m_maxSongBar(0)
{
    CStavePos::setKeySignature(NOT_USED, 0);
    reset();
}

CSong::~CSong()
{
    delete m_midiFile;
    delete m_trackList;
}

void CSong::reset()
{
    m_reachedMidiEof = false;
    m_findChord.reset();
}

void CSong::init2(CScore * scoreWin, CSettings* settings)
{

    CNote::reset();

    this->CConductor::init2(scoreWin, settings);

    setActiveHand(PB_PART_both);
    setPlayMode(PB_PLAY_MODE_followYou);
    setSpeed(1.0);
    setSkill(3);
}

void CSong::loadSong(const QString & filename)
{
    CNote::reset();

    m_songTitle = filename;
    int index = m_songTitle.lastIndexOf("/");
    if (index >= 0)
        m_songTitle = m_songTitle.right( m_songTitle.length() - index - 1);

    QString fn = filename;
#ifdef _WIN32
     fn = fn.replace('/','\\');
#endif
    m_midiFile->setLogLevel(3);
    m_midiFile->openMidiFile(string(fn.toLocal8Bit().data()));
    ppLogInfo("Opening song %s",  fn.toLocal8Bit().data());
    transpose(0);
    midiFileInfo();
    m_maxSongBar = computeMaxSongBar();
    m_midiFile->setLogLevel(99);
    playMusic(false);
    rewind();
    setPlayFromBar(0.0);
    setLoopingBars(0.0);
    setEventBits(EVENT_BITS_loadSong);
    if (!m_midiFile->getSongTitle().isEmpty())
        m_songTitle = m_midiFile->getSongTitle();

}

void CSong::setPlayFromBar(double bar)
{
    double clampedBar = bar;
    if (m_maxSongBar > 0 && bar > m_maxSongBar)
        clampedBar = static_cast<double>(m_maxSongBar);
    this->CConductor::setPlayFromBar(clampedBar);
}

// read the file ahead to collect info about the song first
void CSong::midiFileInfo()
{
    m_trackList->reset(m_midiFile->numberOfTracks());
    for (int i = 0; i < m_midiFile->numberOfTracks(); ++i)
        m_trackList->setTrackName(i, m_midiFile->getTrackName(i));
    setTimeSig(0,0);
    CStavePos::setKeySignature( NOT_USED, 0 );

    // Read the next events to find the active channels
    CMidiEvent event;
    while ( true )
    {
        event = m_midiFile->readMidiEvent();
        m_trackList->examineMidiEvent(event);

        if (event.type() == MIDI_PB_timeSignature)
        {
            setTimeSig(event.data1(),event.data2());
        }

        if (event.type() == MIDI_PB_EOF)
            break;
    }
}

int CSong::computeMaxSongBar()
{
    if (!m_midiFile)
        return 0;

    m_midiFile->rewind();
    CBar barProbe;
    barProbe.setTimeSig(0, 0);

    while (true)
    {
        CMidiEvent event = m_midiFile->readMidiEvent();
        if (event.type() == MIDI_PB_timeSignature)
            barProbe.setTimeSig(event.data1(), event.data2());

        barProbe.addDeltaTime(static_cast<qint64>(event.deltaTime()) * SPEED_ADJUST_FACTOR);

        if (event.type() == MIDI_PB_EOF)
            break;
    }

    return barProbe.getBarNumber();
}

void CSong::rewind()
{
    if (m_maxSongBar == 0)
        m_maxSongBar = computeMaxSongBar();
    m_midiFile->rewind();
    this->CConductor::rewind();
    m_scoreWin->reset();
    reset();
    forceScoreRedraw();
}

void CSong::setActiveHand(whichPart_t hand)
{
    if (hand < PB_PART_both)
        hand = PB_PART_both;
    if (hand > PB_PART_left)
        hand = PB_PART_left;

    this->CConductor::setActiveHand(hand);
    regenerateChordQueue();

    m_scoreWin->setDisplayHand(hand);
}

void CSong::setActiveChannel(int chan)
{
    this->CConductor::setActiveChannel(chan);
    m_scoreWin->setActiveChannel(chan);
    if (m_trackList)
    {
        const int program = m_trackList->getChannelPatch(chan);
        if (program >= 0)
            setPianistProgram(program);
    }
    regenerateChordQueue();
}

void  CSong::setPlayMode(playMode_t mode)
{
    regenerateChordQueue();
    this->CConductor::setPlayMode(mode);
    forceScoreRedraw();
}

void CSong::regenerateChordQueue()
{
    clearWantedChordQueue();
    m_findChord.reset();

    const int length = songEventQueueLength();

    for (int i = 0; i < length; i++)
    {
        const CMidiEvent event = songEventAt(i);
        insertChordIfFound(event);
    }
    resetWantedChord();
}

void CSong::refreshScroll()
{
    m_scoreWin->refreshScroll();
    forceScoreRedraw();
}

bool CSong::seekToPlayFromBar()
{
    const bool resumePlaying = playingMusic();
    int desiredBar = playFromBarTarget();

    // First find the furthest reachable bar, then clamp the desired start.
    {
        m_midiFile->rewind();
        CBar barProbe;
        int top = 0, bottom = 0;
        getTimeSig(&top, &bottom);
        if (top > 0 && bottom > 0)
            barProbe.setTimeSig(top, bottom);
        while (true)
        {
            CMidiEvent event = m_midiFile->readMidiEvent();
            if (event.type() == MIDI_PB_timeSignature)
                barProbe.setTimeSig(event.data1(), event.data2());
            barProbe.addDeltaTime(static_cast<qint64>(event.deltaTime()) * SPEED_ADJUST_FACTOR);
            if (event.type() == MIDI_PB_EOF)
                break;
        }
        const int maxReachableBar = barProbe.getBarNumber();
        if (desiredBar > maxReachableBar)
        {
            desiredBar = maxReachableBar;
            setPlayFromBar(desiredBar);
        }
    }

    rewind();

    bool reachedTarget = (desiredBar == 0);
    auto applySkippedEventState = [this](const CMidiEvent &event) {
        const int type = event.type();
        if (type == MIDI_PB_tempo)
        {
            applyTempoEvent(event);
            return;
        }
        if (type == MIDI_PB_timeSignature)
        {
            applyTimeSignatureEvent(event);
            return;
        }

        switch (type)
        {
        case MIDI_PROGRAM_CHANGE:
        case MIDI_CONTROL_CHANGE:
        case MIDI_PITCH_BEND:
        case MIDI_CHANNEL_PRESSURE:
        case MIDI_NOTE_PRESSURE:
            sendImmediateSetupEvent(event);
            break;
        default:
            break;
        }
    };

    while (!m_reachedMidiEof)
    {
        if (midiEventSpace() <= 10 || chordEventSpace() <= 10)
            break;

        if (m_scoreWin->midiEventSpace() <= 100)
            break;

        CMidiEvent event = m_midiFile->readMidiEvent();
        qint64 deltaTicks = static_cast<qint64>(event.deltaTime()) * SPEED_ADJUST_FACTOR;

        if (!reachedTarget)
        {
            qint64 leftover = deltaTicks;
            while (leftover > 0 && currentBarNumberRaw() < desiredBar)
            {
                const qint64 toNextBar = ticksToNextBarStart();
                if (toNextBar <= 0)
                    break;

                if (leftover < toNextBar)
                {
                    advanceBarPosition(leftover);
                    leftover = 0;
                    break;
                }

                advanceBarPosition(toNextBar);
                leftover -= toNextBar;
            }

            if (currentBarNumberRaw() < desiredBar)
            {
                applySkippedEventState(event);
                continue;
            }

            reachedTarget = true;
            deltaTicks = leftover;
            jumpToBarNumber();
        }

        const int adjustedDelta = static_cast<int>(deltaTicks / SPEED_ADJUST_FACTOR);
        event.setDeltaTime(adjustedDelta);

        insertChordIfFound(event);
        m_scoreWin->midiEventInsert(event);
        midiEventInsert(event);

        if (event.type() == MIDI_PB_EOF)
        {
            m_reachedMidiEof = true;
            break;
        }
    }

    if (!reachedTarget)
    {
        jumpToBarNumber();
        reachedTarget = true;
    }

    const eventBits_t barBits = readBarEventBits();
    setEventBits(barBits);
    if (barBits & EVENT_BITS_newBarNumber)
        emit barChanged(currentBarNumberRaw());

    if (resumePlaying)
        playMusic(true);

    return reachedTarget;
}

eventBits_t CSong::task(qint64 ticks)
{
    Q_UNUSED(ticks);
    if (seekingBarNumber() && playingMusic())
        seekToPlayFromBar();

    while (true)
    {
        if (m_reachedMidiEof == true)
            break;

        // Check that there is space
        if (midiEventSpace() <= 10 || chordEventSpace() <= 10)
            break;

        // and that the Score has space also
        if (m_scoreWin->midiEventSpace() <= 100)
            break;

        // Read the next events
        CMidiEvent event = m_midiFile->readMidiEvent();

        insertChordIfFound(event);

        m_scoreWin->midiEventInsert(event);

        midiEventInsert(event);

        if (event.type() == MIDI_PB_EOF)
        {
            m_reachedMidiEof = true;
            break;
        }
    }

    return takePendingEventBits();
}

static const struct pcNote_s
{
    int key;
    int note;
} pcNoteLookup[] =
{
    { 'a', PC_KEY_LOWEST_NOTE },
    { 'z', 59 }, // B
    { 'x', 60 }, // Middle C
    { 'd', 61 },
    { 'c', 62 }, // D
    { 'f', 63 },
    { 'v', 64 }, // E
    { 'b', 65 }, // F
    { 'h', 66 },
    { 'n', 67 }, // G
    { 'j', 68 },
    { 'm', 69 }, // A
    { 'k', 70 },
    { ',', 71 }, // B
    { '.', 72 }, // C
    { ';', 73 },
    { '/', 74 }, // D
    { '\'', PC_KEY_HIGHEST_NOTE },
};

// Fakes a midi piano keyboard using the PC keyboard
bool CSong::pcKeyPress(int key, bool down)
{
    int i;
    CMidiEvent midi;
    const int cfg_pcKeyVolume = 64;
    const int cfg_pcKeyChannel = 1-1;

    if (key == 't') // the tab key on the PC fakes good notes
    {
        if (down)
            m_fakeChord = getWantedChord();
        for (i = 0; i < m_fakeChord.length(); i++)
        {
            if (down)
                midi.noteOnEvent(0, cfg_pcKeyChannel, m_fakeChord.getNote(i).pitch() + getTranspose(), cfg_pcKeyVolume);
            else
                midi.noteOffEvent(0, cfg_pcKeyChannel, m_fakeChord.getNote(i).pitch() + getTranspose(), cfg_pcKeyVolume);
            expandPianistInput(midi);
        }
        return true;
    }

    for (const auto &pcNote : pcNoteLookup)
    {
        if (key == pcNote.key)
        {
            if (down)
                midi.noteOnEvent(0, cfg_pcKeyChannel, pcNote.note, cfg_pcKeyVolume);
            else
                midi.noteOffEvent(0, cfg_pcKeyChannel, pcNote.note, cfg_pcKeyVolume);

            expandPianistInput(midi);
            return true;
        }
    }
    //printf("pcKeyPress %d %d\n", m_pcNote, key);
    return false;
}

void CSong::insertChordIfFound(const CMidiEvent &event)
{
    if (m_findChord.findChord(event, getActiveChannel(), PB_PART_both) == true)
        chordEventInsert(m_findChord.getChord()); // give the Conductor the chord event
}
