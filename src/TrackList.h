/*********************************************************************************/
/*!
@file           TrackList.h

@brief          xxxx.

@author         L. J. Barman

    Copyright (c)   2008-2013, L. J. Barman, all rights reserved

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

#ifndef __TRACK_LIST_H__
#define __TRACK_LIST_H__

#include <QString>
#include <QStringList>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QVector>

#include <cstring>

#include "MidiEvent.h"
#include "Chord.h"

#define CONVENTION_LEFT_HAND_CHANNEL (3-1)
#define CONVENTION_RIGHT_HAND_CHANNEL (4-1)

class CSong;
class CSettings;
class QListWidgetItem;

class AnalyseItem
{
public:

    AnalyseItem() = default;

    AnalyseItem(int numberOfTracks) :
        m_noteCount(0),
        m_firstPatch(-1)
    {
        for (int i = 0; i < numberOfTracks; ++i) {
             QSharedPointer<int> notePtr (new int[MAX_MIDI_NOTES]);
             std::memset(notePtr.data(), 0, sizeof(int) * MAX_MIDI_NOTES );
             m_noteFrequencyByTrack.append(notePtr) ;
             m_noteCountByTrack.append(0);
        }
     }

   void addNoteEvent(CMidiEvent event){
       m_noteCount++;
       int trackNo = event.track();
       m_noteCountByTrack[trackNo]++;
       int *noteFrequency = m_noteFrequencyByTrack[trackNo].data();
       int note = event.note();
       // count each note so we can guess the key signature
       if (note >= 0 && note < MAX_MIDI_NOTES) {
           (*(noteFrequency + note))++;
       }
       // If we have a note and no patch then default to grand piano patch
       if (m_firstPatch == -1 && event.channel() != MIDI_DRUM_CHANNEL) {
           m_firstPatch = GM_PIANO_PATCH;
       }
   }

   int noteCount() const {return m_noteCount;}
   int active() const {return m_noteCount >0;}

   void addPatch(int patch){
        if (m_firstPatch == -1) {
            m_firstPatch = patch;
        }
   }

   int firstPatch() const {return m_firstPatch;}

   int trackCount() const {
       int acitveTracks = 0;
       for (int track = 0; track < m_noteCountByTrack.size(); ++track) {
           if (m_noteCountByTrack[track] > 0) {
               acitveTracks++;
           }
       }
       return acitveTracks;
   }

   int rightHandTrack() const {
       if (trackCount() <= 1) {
           return -1;
       }
       double highestAveragePitch = 0.0;
       int rightHandTrackIndex = 1;
       for (int track = 0; track < m_noteCountByTrack.size(); ++track) {
           if (m_noteCountByTrack[track] > 0) {
               double averagePitch = averageNotePitch(track);
               if (averagePitch > highestAveragePitch ) {
                   highestAveragePitch = averagePitch;
                   rightHandTrackIndex = track;
               }
           }
       }
       return rightHandTrackIndex;
   }

   double averageNotePitch(int trackNo) const {
       int totalNoteCount = 0;
       double sumOffAllPitches = 0.0;
       int *noteFrequency = m_noteFrequencyByTrack[trackNo].data();

       for (int note = 0; note < MAX_MIDI_NOTES; ++note) {
           int frequency =  *(noteFrequency + note);
           totalNoteCount += frequency;
           sumOffAllPitches += frequency * note;
       }
       return sumOffAllPitches / totalNoteCount;
   }

private:
    int m_noteCount = 0;
    int m_firstPatch = -1;
    QVector<QSharedPointer<int>> m_noteFrequencyByTrack;
    QVector<int> m_noteCountByTrack;
};

class CTrackListItem
{
public:
    CTrackListItem(int midiChannel, int trackIndex = -1, const QString &trackName = QString()) :
    m_midiChannel(midiChannel),
    m_trackIndex(trackIndex),
    m_trackName(trackName)
    {
    }

    int midiChannel() const {return m_midiChannel;}
    int trackIndex() const {return m_trackIndex;}
    QString trackName() const {return m_trackName;}

private:
    int m_midiChannel;
    int m_trackIndex;
    QString m_trackName;
};

class CTrackList : public QObject
{
Q_OBJECT
public:
    CTrackList() :
        QObject(nullptr),
        m_song(nullptr),
        m_settings(nullptr)
    {
        reset(0);
    }

    void init(CSong* songObj, CSettings* settings);

    void refresh();
    void reset(int numberOfTracks);

    // Find an unused channel
    int findFreeChannel(int startChannel);

    void currentRowChanged(int currentRow);
    void examineMidiEvent(CMidiEvent event);
    bool pianoPartConvetionTest();
    bool findLeftAndRightPianoParts();
    int guessKeySignature(int chanA, int chanB);

    // The programme name now starts at 1 with 0 = "(none)"
    static QString getProgramName(int program);
    QStringList getAllChannelProgramNames(bool raw=false);
    int getActiveItemIndex();
    int getActiveHandIndex(whichPart_t whichPart);

    // set the midi channels to use for the left and right hand piano parts
    void setActiveHandsIndex(int leftIndex, int rightIndex);

    int getChannelPatch(int channel) const;

    int getHandTrackIndex(whichPart_t whichPart);

    void changeListWidgetItemView(int index, QListWidgetItem* listWidgetItem);
    bool isChannelActive(int chan) const;
    void setTrackName(int trackIndex, const QString &name);
    QString trackName(int trackIndex) const;
    int channelTrackIndex(int channel) const;

    double averageNotePitch(int chan) {
        int totalNoteCount = 0;
        double sumOffAllPitches = 0.0;

        for (int note = 0; note < MAX_MIDI_NOTES; ++note) {
            int frequency = m_noteFrequency[chan][note];
            totalNoteCount += frequency;
            sumOffAllPitches += frequency * note;
        }
        return sumOffAllPitches / totalNoteCount;
    }

private:
    QString getChannelProgramName(int chan) const;

    CSong* m_song;
    CSettings* m_settings;
    QList<CTrackListItem> m_partsList;
    QVector<AnalyseItem> m_midiChannels;
    int m_noteFrequency[MAX_MIDI_CHANNELS][MAX_MIDI_NOTES];
    QVector<QString> m_trackNames;
    int m_channelTrackIndex[MAX_MIDI_CHANNELS];

};

#endif //__TRACK_LIST_H__
