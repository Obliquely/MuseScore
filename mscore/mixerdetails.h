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

      Mixer* mixer;
      MixerTrackItem* selectedMixerTrackItem = nullptr;
      void setupSlotsAndSignals();
      QWidget* mutePerVoiceHolder = nullptr;
      QGridLayout* mutePerVoiceGrid;
      QList<QWidget*> voiceButtons; // used for dynamically updating tabOrder

      void updateName();
      void updateTrackColor();
      void updatePatch(MixerTrackItem* mixerTrackItem);
      void updateVolume();
      void updatePan();
      void updateMutePerVoice(MixerTrackItem* mixerTrackItem);
      void updateReverb();
      void updateChorus();
      void updateMidiChannel();

      void blockDetailsSignals(bool);
      void updateTabOrder();
            
   public slots:
      void partNameEdited();
      void drumsetCheckboxToggled(bool);
      void patchComboEdited(int);
      void volumeSliderMoved(int);
      void volumeSpinBoxEdited(double);
      void panSliderMoved(int);
      void panSpinBoxEdited(double);
      void midiChannelOrPortEdited(int);
      void reverbSliderMoved(int);
      void reverbSpinBoxEdited(double);
      void chorusSliderMoved(int);
      void chorusSpinBoxEdited(double);
      void trackColorEdited(QColor);

   public:
      MixerDetails(Mixer *mixer);
      void updateDetails(MixerTrackItem*);
      void propertyChanged(Channel::Prop property) override;

      void enableDetails(bool);
      void resetDetails(); // apply default (0 or empty) values for when no track is selected
      void setVoiceMute(int staffIndex, int voiceIndex, bool shouldMute);
      void resetPanToCentre();
      void updateUiOptions();

      MixerTrackItem* getSelectedMixerTrackItem() { return selectedMixerTrackItem; };
      };

class MixerDetails;

class MixerVoiceMuteButtonHandler : public QObject
      {
      Q_OBJECT

      MixerDetails* mixerDetails;
      int staffIndex;
      int voiceIndex;

   public:
      MixerVoiceMuteButtonHandler(MixerDetails* mixerDetails, int staffIndex, int voiceIndex, QObject* parent = nullptr)
            : QObject(parent),
              mixerDetails(mixerDetails),
              staffIndex(staffIndex),
              voiceIndex(voiceIndex)
            {}

public slots:
      void setVoiceMute(bool checked)
            {
            mixerDetails->setVoiceMute(staffIndex, voiceIndex, checked);
            }
   };
}
#endif // __MIXERDETAILS_H__
