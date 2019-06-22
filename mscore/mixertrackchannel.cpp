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

#include "mixertrackchannel.h"

#include "musescore.h"

#include "libmscore/score.h"
#include "libmscore/part.h"
#include "mixer.h"
#include "mixertrackitem.h"
#include "seq.h"
#include "libmscore/undo.h"
#include "synthcontrol.h"
#include "synthesizer/msynthesizer.h"
#include "preferences.h"

namespace Ms {

MixerTrackChannel::MixerTrackChannel(QTreeWidgetItem* treeWidgetItem, MixerTrackItem* mixerTrackItem) :
      treeWidgetItem(treeWidgetItem), mixerTrackItem(mixerTrackItem)
      {
      setupUi(this);
      setupAdditionalUi();
      setupSlotsAndSignals();
      update();

      Channel* channel = mixerTrackItem->chan();
      channel->addListener(this);
      }


void MixerTrackChannel::setupSlotsAndSignals()
      {
      connect(muteButton,     SIGNAL(toggled(bool)),        SLOT(stripMuteToggled(bool)));
      connect(soloButton,     SIGNAL(toggled(bool)),        SLOT(stripSoloToggled(bool)));
      connect(volumeSlider,   SIGNAL(valueChanged(int)),    SLOT(stripVolumeSliderMoved(int)));
      connect(volumeSlider,   SIGNAL(sliderPressed()),      SLOT(takeSelection()));
      }

void MixerTrackChannel::takeSelection()
      {
      treeWidgetItem->treeWidget()->setCurrentItem(treeWidgetItem);
      }

void MixerTrackChannel::setupAdditionalUi()
      {
      //TODO: a more responsible approach to styling that's also light/dark theme respectful
      QString basicButton = "QToolButton{background: white; color: black; font-weight: bold; border: 1px solid gray;}";
      QString colorTemplate = "QToolButton:checked, QToolButton:pressed { color: white; background: %1;}";
      muteButton->setStyleSheet(basicButton + colorTemplate.arg("red"));
      soloButton->setStyleSheet(basicButton + colorTemplate.arg("green"));
      }

void MixerTrackChannel::updateUiControls(Mixer* mixer)  // fudge for now!
      {
      bool showTrackColors = mixer->isShowingTrackColors();
      colorLabel->setVisible(showTrackColors);
      }

void MixerTrackChannel::update()
      {
      const QSignalBlocker blockVolumeSignals(volumeSlider);
      const QSignalBlocker blockMuteSignals(muteButton);
      const QSignalBlocker blockSoloSignals(soloButton);

      volumeSlider->setValue(mixerTrackItem->getVolume());
      volumeSlider->setToolTip(tr("Volume: %1").arg(QString::number(mixerTrackItem->getVolume())));
      
      muteButton->setChecked(mixerTrackItem->getMute());
      soloButton->setChecked(mixerTrackItem->getSolo());

      Channel* channel = mixerTrackItem->chan();
      MidiPatch* midiPatch = synti->getPatchInfo(channel->synti(), channel->bank(), channel->program());
      Part* part = mixerTrackItem->part();
      Instrument* instrument = mixerTrackItem->instrument();

      QColor channelColor = channel->color();
      if (colorLabel)
            colorLabel->setStyleSheet(QString("QLabel{background: %1;padding-top: 2px; padding-bottom: 2px; border-radius: 3px;}").arg(channelColor.name()));

      QString tooltip = tr("Part Name: %1\n"
                           "Instrument: %2\n"
                           "Channel: %3\n"
                           "Bank: %4\n"
                           "Program: %5\n"
                           "Patch: %6")
                        .arg(part->partName(),
                             instrument->trackName(),
                             qApp->translate("InstrumentsXML", channel->name().toUtf8().data()),
                             QString::number(channel->bank()),
                             QString::number(channel->program()),
                             midiPatch ? midiPatch->name : tr("~no patch~"));
      setToolTip(tooltip);
      }


void MixerTrackChannel::propertyChanged(Channel::Prop property)
      {
      update();
      }

      
void MixerTrackChannel::stripVolumeSliderMoved(int value)
      {
      mixerTrackItem->setVolume(value);
      volumeSlider->setToolTip(tr("Volume: %1").arg(QString::number(value)));
      takeSelection();
      }

      
void MixerTrackChannel::stripSoloToggled(bool val)
      {
      mixerTrackItem->setSolo(val);
      takeSelection();
      }

      
void MixerTrackChannel::stripMuteToggled(bool val)
      {
      mixerTrackItem->setMute(val);
      takeSelection();
      }

}
