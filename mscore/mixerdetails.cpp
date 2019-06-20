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

#include "mixerdetails.h"

#include "musescore.h"

#include "libmscore/score.h"
#include "libmscore/part.h"
#include "mixer.h"
#include "mixertrack.h"
#include "mixertrackitem.h"
#include "seq.h"
#include "libmscore/undo.h"
#include "synthcontrol.h"
#include "synthesizer/msynthesizer.h"
#include "preferences.h"

namespace Ms {

void Mixer::updateDetails(MixerTrackItem* mixerTrackItem)
      {
      detailsMixerTrackItem = mixerTrackItem;

      if (!detailsMixerTrackItem) {
            resetDetails();         // return controls to default / unset state
            enableDetails(false);   // disable controls
            setNotifier(nullptr);
            return;
            }

      // setNotifier(channel) zaps previous notifiers and then calls addListener(this).
      // As a listener, this object receives propertyChanged() calls when the channel is
      // changed. This ensures the details view is synced with changes in the tree view.

      setNotifier(detailsMixerTrackItem->chan());
      enableDetails(true);

      blockDetailsSignals(true);

      updateVolume();
      updatePan();
      updateReverb();
      updateChorus();
      updateName();
      updateMidiChannel();
      updatePatch(mixerTrackItem);
      updateMutePerVoice(mixerTrackItem);

      blockDetailsSignals(false);
      }


void Mixer::enableDetails(bool enable)
      {
      drumkitCheck->setEnabled(enable);
      patchCombo->setEnabled(enable);
      partNameLineEdit->setEnabled(enable);
      volumeSlider->setEnabled(enable);
      volumeSpinBox->setEnabled(enable);
      panSlider->setEnabled(enable);
      panSpinBox->setEnabled(enable);
      reverbSlider->setEnabled(enable);
      reverbSpinBox->setEnabled(enable);
      chorusSlider->setEnabled(enable);
      chorusSpinBox->setEnabled(enable);
      portSpinBox->setEnabled(enable);
      channelSpinBox->setEnabled(enable);
      //trackColorLabel->setEnabled(enable);

      // is this meaningful to disable the labels or just redundant?
      labelName->setEnabled(enable);
      labelChannel->setEnabled(enable);
      labelChannel_2->setEnabled(enable);
      labelChorus->setEnabled(enable);
      labelPan->setEnabled(enable);
      labelPatch->setEnabled(enable);
      labelPort->setEnabled(enable);
      labelReverb->setEnabled(enable);
      labelVolume->setEnabled(enable);
      }

      
void Mixer::resetDetails()
      {
      drumkitCheck->setChecked(false);
      patchCombo->clear();
      partNameLineEdit->setText("");
      channelLabel->setText("");
      volumeSlider->setValue(0);
      volumeSpinBox->setValue(0);
      panSlider->setValue(0);
      panSpinBox->setValue(0);
      reverbSlider->setValue(0);
      reverbSpinBox->setValue(0);
      chorusSlider->setValue(0);
      chorusSpinBox->setValue(0);
      portSpinBox->setValue(0);
      channelSpinBox->setValue(0);
      //trackColorLabel->setColor(QColor());
      }

      
void Mixer::blockDetailsSignals(bool block)
      {
      //trackColorLabel->blockSignals(block);
      volumeSlider->blockSignals(block);
      volumeSpinBox->blockSignals(block);
      panSlider->blockSignals(block);
      panSpinBox->blockSignals(block);
      reverbSlider->blockSignals(block);
      reverbSpinBox->blockSignals(block);
      chorusSlider->blockSignals(block);
      chorusSpinBox->blockSignals(block);
      portSpinBox->blockSignals(block);
      channelSpinBox->blockSignals(block);
      }

// updatePatch - is there a missing case here - can the patch
// be updated outwith the mixer - and if it is are we listening
// for that change? - not clear that we are
void Mixer::updatePatch(MixerTrackItem* mixerTrackItem)
      {
      Channel* channel = mixerTrackItem->chan();
      MidiMapping* midiMap = mixerTrackItem->midiMap();
      
      //Check if drumkit
      const bool drum = midiMap->part()->instrument()->useDrumset();
      drumkitCheck->setChecked(drum);
      
      //Populate patch combo
      patchCombo->clear();
      const auto& pl = synti->getPatchInfo();
      int patchIndex = 0;
      
      // Order by program number instead of bank, so similar instruments
      // appear next to each other, but ordered primarily by soundfont
      std::map<int, std::map<int, std::vector<const MidiPatch*>>> orderedPl;
      
      for (const MidiPatch* p : pl)
            orderedPl[p->sfid][p->prog].push_back(p);
      
      std::vector<QString> usedNames;
      for (auto const& sf : orderedPl) {
            for (auto const& pn : sf.second) {
                  for (const MidiPatch* p : pn.second) {
                        if (p->drum == drum || p->synti != "Fluid") {
                              QString pName = p->name;
                              if (std::find(usedNames.begin(), usedNames.end(), p->name) != usedNames.end()) {
                                    QString addNum = QString(" (%1)").arg(p->sfid);
                                    pName.append(addNum);
                              }
                              else
                                    usedNames.push_back(p->name);
                              
                              patchCombo->addItem(pName, QVariant::fromValue<void*>((void*)p));
                              if (p->synti == channel->synti() &&
                                  p->bank == channel->bank() &&
                                  p->prog == channel->program())
                                    patchIndex = patchCombo->count() - 1;
                        }
                  }
            }
      }
      patchCombo->setCurrentIndex(patchIndex);
      }
      
void Mixer::updateMutePerVoice(MixerTrackItem* mixerTrackItem)
      {
       if (mutePerVoiceHolder) {
            // deleteLater() deletes object after the current event loop completes
             mutePerVoiceHolder->deleteLater();
             mutePerVoiceHolder = nullptr;
       }

      Part* part = mixerTrackItem->part();

      //Set up mute per voice buttons
      mutePerVoiceHolder = new QWidget();
      mutePerVoiceArea->addWidget(mutePerVoiceHolder);

      mutePerVoiceGrid = new QGridLayout();
      mutePerVoiceHolder->setLayout(mutePerVoiceGrid);
      mutePerVoiceGrid->setContentsMargins(0, 0, 0, 0);
      mutePerVoiceGrid->setSpacing(7);

      for (int staffIdx = 0; staffIdx < (*part->staves()).length(); ++staffIdx) {
            Staff* staff = (*part->staves())[staffIdx];
            for (int voice = 0; voice < VOICES; ++voice) {
                  QPushButton* tb = new QPushButton;
                  tb->setStyleSheet(
                        QString("QPushButton{padding: 4px 8px 4px 8px;}QPushButton:checked{background-color:%1}")
                        .arg(MScore::selectColor[voice].name()));
                  tb->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                  tb->setText(QString("%1").arg(voice + 1));
                  tb->setCheckable(true);
                  tb->setChecked(!staff->playbackVoice(voice));
                  tb->setToolTip(QString(tr("Staff #%1")).arg(staffIdx + 1));

                  mutePerVoiceGrid->addWidget(tb, staffIdx, voice);
                  MixerDetailsVoiceButtonHandler* handler =
                              new MixerDetailsVoiceButtonHandler(this, staffIdx, voice, tb);
                  connect(tb, SIGNAL(toggled(bool)), handler, SLOT(setVoiceMute(bool)));
                  }
            }
      }

//---------------------------------------------------------
//   setVoiceMute
//---------------------------------------------------------

void Mixer::setVoiceMute(int staffIdx, int voice, bool shouldMute)
      {
      Part* part = detailsMixerTrackItem->part();
      Staff* staff = part->staff(staffIdx);
      switch (voice) {
            case 0:
                  staff->undoChangeProperty(Pid::PLAYBACK_VOICE1, !shouldMute);
                  break;
            case 1:
                  staff->undoChangeProperty(Pid::PLAYBACK_VOICE2, !shouldMute);
                  break;
            case 2:
                  staff->undoChangeProperty(Pid::PLAYBACK_VOICE3, !shouldMute);
                  break;
            case 3:
                  staff->undoChangeProperty(Pid::PLAYBACK_VOICE4, !shouldMute);
                  break;
            }
      }


//---------------------------------------------------------
//   partNameChanged
//---------------------------------------------------------

void Mixer::partNameChanged()
      {
      if (!detailsMixerTrackItem)
            return;

      QString text = partNameLineEdit->text();
      Part* part = detailsMixerTrackItem->part();
      if (part->partName() == text) {
            return;
            }

      Score* score = part->score();
      if (score) {
            score->startCmd();
            score->undo(new ChangePart(part, part->instrument(), text));
            score->endCmd();
            }
      }

//---------------------------------------------------------
//   trackColorChanged
//---------------------------------------------------------

/*void Mixer::trackColorChanged(QColor col)
      {
      if (trackColorLabel->color() != col) {
            trackColorLabel->blockSignals(true);
            trackColorLabel->setColor(col);
            trackColorLabel->blockSignals(false);
            }

      _mti->setColor(col.rgb());
      }
*/
/*
 trackColorLabel->setColor(QColor(_mti->color() | 0xff000000));
 */
       



void Mixer::updateVolume()
      {
      Channel* channel = detailsMixerTrackItem->chan();
      volumeSlider->setValue((int)channel->volume());
      volumeSpinBox->setValue(channel->volume());
      }

void Mixer::updatePan()
      {
      Channel* channel = detailsMixerTrackItem->chan();
      panSlider->setValue(channel->pan()-63);
      panSlider->setToolTip(tr("Pan: %1").arg(QString::number(channel->pan()-63)));
      }


void Mixer::updateReverb()
      {
      Channel* channel = detailsMixerTrackItem->chan();
      reverbSlider->setValue((int)channel->reverb());
      reverbSpinBox->setValue(channel->reverb());
      }


void Mixer::updateChorus()
      {
      Channel* channel = detailsMixerTrackItem->chan();
      reverbSlider->setValue((int)channel->reverb());
      reverbSpinBox->setValue(channel->reverb());
      }


void Mixer::updateName()
      {
      Part* part = detailsMixerTrackItem->part();
      Channel* channel = detailsMixerTrackItem->chan();
      QString partName = part->partName();
      if (!channel->name().isEmpty())
            channelLabel->setText(qApp->translate("InstrumentsXML", channel->name().toUtf8().data()));
      else
            channelLabel->setText("");
      partNameLineEdit->setText(partName);
      partNameLineEdit->setToolTip(partName);
      }

void Mixer::updateMidiChannel()
      {
      Part* part = detailsMixerTrackItem->part();
      Channel* channel = detailsMixerTrackItem->chan();
      portSpinBox->setValue(part->masterScore()->midiMapping(channel->channel())->port() + 1);
      channelSpinBox->setValue(part->masterScore()->midiMapping(channel->channel())->channel() + 1);
      }
      
// propertyChanged - we're listening to changes to the channel
// When they occur, this method is called so that we can update
// the UI. Signals sent by the UI control are blocked during the
// update to prevent getting caught in an update loop.
void Mixer::propertyChanged(Channel::Prop property)
      {
      if (!detailsMixerTrackItem)
            return;

      blockDetailsSignals(true);

      switch (property) {
            case Channel::Prop::VOLUME: {
                  updateVolume();
                  break;
                  }
            case Channel::Prop::PAN: {
                  updatePan();
                  break;
                  }
            case Channel::Prop::CHORUS: {
                  updateChorus();
                  break;
                  }
            case Channel::Prop::REVERB: {
                  updateReverb();
                  break;
                  }
            case Channel::Prop::COLOR: {
                  //trackColorChanged(chan->color());
                  break;
                  }
            case Channel::Prop::NAME: {
                  updateName();
                  break;
                  }
            default:
                  break;
            }

            blockDetailsSignals(false);
      }


// volumeChanged - process signal from volumeSlider
void Mixer::volumeChanged(double value)
      {
      qDebug()<<"volumeChanged(double "<<value<<")";
      if (!detailsMixerTrackItem)
            return;
      detailsMixerTrackItem->setVolume(value);
      }

// volumeChanged - process signal from volumeSpinBox
void Mixer::volumeChanged(int value)
{
      qDebug()<<"volumeChanged(double "<<value<<")";
      if (!detailsMixerTrackItem)
            return;
      detailsMixerTrackItem->setVolume(value);
}


// panChanged - process signal from panSlider
void Mixer::panChanged(double value)
      {
      if (detailsMixerTrackItem)
            return;
      detailsMixerTrackItem->setPan(value + 63);
      }

// panChanged - process signal from panSpinBox
void Mixer::panChanged(int value)
{
      if (!detailsMixerTrackItem)
            return;
      detailsMixerTrackItem->setPan(value + 63);
}

// reverbChanged - process signal from reverbSlider

void Mixer::reverbChanged(double v)
      {
      if (!detailsMixerTrackItem)
            return;
      detailsMixerTrackItem->setReverb(v);
      }

//  chorusChanged - process signal from chorusSlider
void Mixer::chorusChanged(double v)
      {
      if (!detailsMixerTrackItem)
            return;
      detailsMixerTrackItem->setChorus(v);
      }

//  patchChanged - process signal from patchCombo
void Mixer::patchChanged(int n)
{
      qDebug()<<"Mixer::patchChanged('"<<n<<")";
      if (!detailsMixerTrackItem)
            return;

      const MidiPatch* p = (MidiPatch*)patchCombo->itemData(n, Qt::UserRole).value<void*>();
      if (p == 0) {
            qDebug("PartEdit::patchChanged: no patch");
            return;
      }

      Part* part = detailsMixerTrackItem->midiMap()->part();
      Channel* channel = detailsMixerTrackItem->midiMap()->articulation();
      Score* score = part->score();
      if (score) {
            score->startCmd();
            score->undo(new ChangePatch(score, channel, p));
            score->undo(new SetUserBankController(channel, true));
            score->setLayoutAll();
            score->endCmd();
      }
}

// drumkitToggled - process signal from drumkitCheck
void Mixer::drumkitToggled(bool val)
      {
      if (!detailsMixerTrackItem)
            return;

      Part* part = detailsMixerTrackItem->part();
      Channel* channel = detailsMixerTrackItem->chan();

      Instrument *instr;
      if (detailsMixerTrackItem->trackType() == MixerTrackItem::TrackType::CHANNEL)
            instr = detailsMixerTrackItem->instrument();
      else
            instr = part->instrument(Fraction(0,1));

      if (instr->useDrumset() == val)
            return;

      const MidiPatch* newPatch = 0;
      const QList<MidiPatch*> pl = synti->getPatchInfo();
      for (const MidiPatch* p : pl) {
            if (p->drum == val) {
                  newPatch = p;
                  break;
                  }
            }

      Score* score = part->score();
      if (newPatch) {
            score->startCmd();
            part->undoChangeProperty(Pid::USE_DRUMSET, val);
            score->undo(new ChangePatch(score, channel, newPatch));
            score->setLayoutAll();
            score->endCmd();
            }
      blockDetailsSignals(true);
      updatePatch(detailsMixerTrackItem);
      blockDetailsSignals(false);
      }

// midiChannelChanged - process signal from either portSpinBox
// or channelSpinBox, i.e. MIDI port or channel change
void Mixer::midiChannelChanged(int)
      {
      if (!detailsMixerTrackItem)
            return;

      Part* part = detailsMixerTrackItem->part();
      Channel* channel = detailsMixerTrackItem->chan();

      seq->stopNotes(channel->channel());
      int p =    portSpinBox->value() - 1;
      int c = channelSpinBox->value() - 1;

      MidiMapping* midiMap = detailsMixerTrackItem->midiMap();
      part->masterScore()->updateMidiMapping(midiMap->articulation(), part, p, c);

      part->score()->setInstrumentsChanged(true);
      part->score()->setLayoutAll();
      seq->initInstruments();

      // Update MIDI Out ports
      int maxPort = max(p, part->score()->masterScore()->midiPortCount());
      part->score()->masterScore()->setMidiPortCount(maxPort);
      if (seq->driver() && (preferences.getBool(PREF_IO_JACK_USEJACKMIDI) || preferences.getBool(PREF_IO_ALSA_USEALSAAUDIO)))
            seq->driver()->updateOutPortCount(maxPort + 1);
      }

}


