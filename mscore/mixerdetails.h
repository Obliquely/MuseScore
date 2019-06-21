//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2016 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __MIXERDETAILS_H__
#define __MIXERDETAILS_H__

#include "ui_mixerdetails.h"
#include "libmscore/instrument.h"
#include "mixertrackitem.h"
#include "mixer.h"

#include <functional>
#include <QPushButton>

class Mixer;

namespace Ms {

//---------------------------------------------------------
//   MixerDetailsVoiceButtonHandler
//---------------------------------------------------------

class MixerDetails : public QWidget, public Ui::MixerDetails, public ChannelListener
      {
      Q_OBJECT

      MixerTrackItem* selectedMixerTrackItem = nullptr;
      void setupSlotsAndSignals();
      QWidget* mutePerVoiceHolder = nullptr;
      QGridLayout* mutePerVoiceGrid;
      QList<QPushButton*> voiceButtons;

      void updatePatch(MixerTrackItem* mixerTrackItem);
      void updateMutePerVoice(MixerTrackItem* mixerTrackItem);
      void updateVolume();
      void updatePan();
      void updateReverb();
      void updateChorus();
      void updateName();
      void updateMidiChannel();

      void blockDetailsSignals(bool);
            
   public slots:
      void partNameChanged();
      // void trackColorChanged(QColor);
      void volumeSpinBoxEdited(double);
      void volumeSliderMoved(int);
      void panSliderMoved(int);
      void panSpinBoxEdited(double);
      void chorusSliderMoved(double);
      void reverbSliderMoved(double);
      void drumsetCheckboxToggled(bool);
      void midiChannelOrPortEdited(int);
      void patchComboEdited(int);

   public:
      MixerDetails(Mixer *mixer);
      void updateDetails(MixerTrackItem*);
      void enableDetails(bool);
      void resetDetails();          // default values (for when not detail selected)
      MixerTrackItem* getSelectedMixerTrackItem() { return selectedMixerTrackItem; };
      void setVoiceMute(int staffIdx, int voice, bool shouldMute);
      void resetPanToCentre();
      void propertyChanged(Channel::Prop property) override;
      };

class MixerDetails;

class MixerDetailsVoiceButtonHandler : public QObject
      {
      Q_OBJECT

      MixerDetails* mixerDetails;
      int staff;
      int voice;

   public:
      MixerDetailsVoiceButtonHandler(MixerDetails* mixerDetails, int staff, int voice, QObject* parent = nullptr)
            : QObject(parent),
              mixerDetails(mixerDetails),
              staff(staff),
              voice(voice)
            {}

public slots:
      void setVoiceMute(bool checked)
            {
            mixerDetails->setVoiceMute(staff, voice, checked);
            }
   };
}
#endif // __MIXERDETAILS_H__
