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


//---------------------------------------------------------
//   MixerTrack
//---------------------------------------------------------

MixerTrackChannel::MixerTrackChannel(QWidget *parent, MixerTrackItem* mixerTrackItem) :
      QWidget(parent), _mixerTrackItem(mixerTrackItem), _group(0)
      {
      setupUi(this);

      muteBn->setStyleSheet("QToolButton{background: white; color: black; font-weight: bold; border: 1px solid gray;} QToolButton:checked, QToolButton:pressed { color: white; background: red;}");
      soloBn->setStyleSheet("QToolButton{background: white; color: black; font-weight: bold; border: 1px solid gray;} QToolButton:checked, QToolButton:pressed { color: white; background: green; }");

      connect(soloBn, SIGNAL(toggled(bool)), SLOT(updateSolo(bool)));
      connect(muteBn, SIGNAL(toggled(bool)), SLOT(updateMute(bool)));

      // updateNameLabel();

      Channel* channel = mixerTrackItem->chan();
      soloBn->setChecked(channel->solo());
      muteBn->setChecked(channel->mute());

      channel->addListener(this);

      volumeSlider->setValue(channel->volume());
      volumeSlider->setToolTip(tr("Volume: %1").arg(QString::number(channel->volume())));
//      volumeSlider->setMaxValue(127);
//      volumeSlider->setNumMajorTicks(10);
//      volumeSlider->setNumMinorTicks(5);

      /*
      panSlider->setValue(channel->pan());
      panSlider->setToolTip(tr("Pan: %1").arg(QString::number(channel->pan())));
      panSlider->setMaxValue(127);
      panSlider->setMinValue(0);
*/
      connect(volumeSlider, SIGNAL(valueChanged(double)),      SLOT(volumeChanged(double)));
//    connect(panSlider,    SIGNAL(valueChanged(double, int)), SLOT(panChanged(double)));
      connect(muteBn, SIGNAL(toggled(bool)),    SLOT(updateMute(bool)));
      connect(soloBn, SIGNAL(toggled(bool)),    SLOT(updateSolo(bool)));

      applyStyle();
      }


void MixerTrackChannel::applyStyle()
      {
      switch (preferences.globalStyle()){
            case MuseScoreStyleType::DARK_FUSION:
                  // NO OP - placeholder
                  break;
            case MuseScoreStyleType::LIGHT_FUSION:
                  // NO OP - placeholder
                  break;
            }
      //setStyleSheet(style);
      }

//---------------------------------------------------------
//   updateNameLabel
//---------------------------------------------------------

void MixerTrackChannel::updateNameLabel()
      {
      Part* part = _mixerTrackItem->part();
      Instrument* instrument = _mixerTrackItem->instrument();
      Channel* chan = _mixerTrackItem->chan();

      QString shortName;
      if (instrument->shortNames().count())
            shortName = instrument->shortNames().first().name() + "-";
      else
            shortName = "";
      QString text = QString("%1%2").arg(shortName, qApp->translate("InstrumentsXML", chan->name().toUtf8().data()));
      //trackLabel->setText(text);

      MidiPatch* mp = synti->getPatchInfo(chan->synti(), chan->bank(), chan->program());

      QString tooltip = tr("Part Name: %1\n"
                           "Instrument: %2\n"
                           "Channel: %3\n"
                           "Bank: %4\n"
                           "Program: %5\n"
                           "Patch: %6")
                  .arg(part->partName(),
                       instrument->trackName(),
                       qApp->translate("InstrumentsXML", chan->name().toUtf8().data()),
                       QString::number(chan->bank()),
                       QString::number(chan->program()),
                       mp ? mp->name : tr("~no patch~"));

      //trackLabel->setToolTip(tooltip);

      /* track color support
       QColor bgCol((QRgb)chan->color());   // for parts -   QColor bgPartCol((QRgb)part->color());

      QString trackColorName = bgCol.name();
      int val = bgCol.value();

      QString trackStyle = QString(".QLabel {"
                 "border: 2px solid black;"
                 "background: %1;"
                 "color: %2;"
                 "padding: 6px 0px;"
             "}").arg(trackColorName, val > 128 ? "black" : "white");

      trackLabel->setStyleSheet(trackStyle);

      QColor bgPartCol((QRgb)part->color());
      QString partColorName = bgPartCol.name();
      val = bgPartCol.value();
      */

      }


//---------------------------------------------------------
//   propertyChanged
//---------------------------------------------------------

void MixerTrackChannel::propertyChanged(Channel::Prop property)
      {
      Channel* channel = _mixerTrackItem->chan();

      switch (property) {
            case Channel::Prop::VOLUME: {
                  volumeSlider->blockSignals(true);
                  volumeSlider->setValue(channel->volume());
                  volumeSlider->setToolTip(tr("Volume: %1").arg(QString::number(channel->volume())));
                  volumeSlider->blockSignals(false);
                  break;
                  }
            case Channel::Prop::PAN: {
//                  panSlider->blockSignals(true);
//                  panSlider->setValue(channel->pan());
//                  panSlider->setToolTip(tr("Pan: %1").arg(QString::number(channel->pan())));
//                  panSlider->blockSignals(false);
                  break;
                  }
            case Channel::Prop::MUTE: {
                  muteBn->blockSignals(true);
                  muteBn->setChecked(channel->mute());
                  muteBn->blockSignals(false);
                  break;
                  }
            case Channel::Prop::SOLO: {
                  soloBn->blockSignals(true);
                  soloBn->setChecked(channel->solo());
                  soloBn->blockSignals(false);
                  break;
                  }
            case Channel::Prop::COLOR: {
                  updateNameLabel();
                  break;
                  }
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void MixerTrackChannel::volumeChanged(double value)
      {
      _mixerTrackItem->setVolume(value);
      volumeSlider->setToolTip(tr("Volume: %1").arg(QString::number(value)));
      }

////---------------------------------------------------------
////   panChanged
////---------------------------------------------------------
//
//void MixerTrackChannel::panChanged(double value)
//      {
//      _mixerTrackItem->setPan(value);
//      panSlider->setToolTip(tr("Pan: %1").arg(QString::number(value)));
//      }

//---------------------------------------------------------
//   updateSolo
//---------------------------------------------------------

void MixerTrackChannel::updateSolo(bool val)
      {
      _mixerTrackItem->setSolo(val);
      }

//---------------------------------------------------------
//   udpateMute
//---------------------------------------------------------

void MixerTrackChannel::updateMute(bool val)
      {
      _mixerTrackItem->setMute(val);
      }

}
