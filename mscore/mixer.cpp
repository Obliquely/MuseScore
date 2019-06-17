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

#include "musescore.h"
#include "parteditbase.h"

#include "libmscore/excerpt.h"
#include "libmscore/score.h"
#include "libmscore/part.h"
#include "mixer.h"
#include "seq.h"
#include "libmscore/undo.h"
#include "synthcontrol.h"
#include "synthesizer/msynthesizer.h"
#include "preferences.h"
#include <QtGlobal>
#include <qmessagebox.h>
#include <accessibletoolbutton.h>
#include "mixerdetails.h"
#include "mixertrack.h"
#include "mixertrackchannel.h"
#include "mixertrackpart.h"
#include "mixertrackitem.h"

namespace Ms {

#define _setValue(__x, __y) \
      __x->blockSignals(true); \
      __x->setValue(__y); \
      __x->blockSignals(false);

#define _setChecked(__x, __y) \
      __x->blockSignals(true); \
      __x->setChecked(__y); \
      __x->blockSignals(false);


double volumeToUserRange(char v) { return v * 100.0 / 128.0; }
double panToUserRange(char v) { return (v / 128.0) * 360.0; }
double chorusToUserRange(char v) { return v * 100.0 / 128.0; }
double reverbToUserRange(char v) { return v * 100.0 / 128.0; }

const float minDecibels = -3;

//0 to 100
char userRangeToVolume(double v) { return (char)qBound(0, (int)(v / 100.0 * 128.0), 127); }
//-180 to 180
char userRangeToPan(double v) { return (char)qBound(0, (int)((v / 360.0) * 128.0), 127); }
//0 to 100
char userRangeToChorus(double v) { return (char)qBound(0, (int)(v / 100.0 * 128.0), 127); }
//0 to 100
char userRangeToReverb(double v) { return (char)qBound(0, (int)(v / 100.0 * 128.0), 127); }

//---------------------------------------------------------
//   Mixer
//---------------------------------------------------------

Mixer::Mixer(QWidget* parent)
    : QDockWidget("Mixer", parent),
      showDetails(true),
      trackHolder(nullptr)
      {
      setupUi(this);

      setWindowFlags(Qt::Tool);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

      connect(mixerTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(currentItemChanged()));
      connect(synti, SIGNAL(gainChanged(float)), SLOT(synthGainChanged(float)));

      enablePlay = new EnablePlayForWidget(this);
      retranslate(true);
      }



//---------------------------------------------------------
//   synthGainChanged
//---------------------------------------------------------

void Mixer::synthGainChanged(float)
      {
      float decibels = qBound(minDecibels, log10f(synti->gain()), 0.0f);
      qDebug()<<"Used to update master slider with value: "<<decibels;
      }




//---------------------------------------------------------
//   masterVolumeChanged
//---------------------------------------------------------

void Mixer::masterVolumeChanged(double decibels)
      {
      float gain = qBound(0.0f, powf(10, (float)decibels), 1.0f);
      synti->setGain(gain);

      }

//---------------------------------------------------------
//   on_partOnlyCheckBox_toggled
//---------------------------------------------------------

void Mixer::on_partOnlyCheckBox_toggled(bool checked)
      {
      if (!_activeScore->excerpt())
            return;

      mscore->setPlayPartOnly(checked);
      setPlaybackScore(_activeScore->masterScore()->playbackScore());

      // Prevent muted channels from sounding
      for (const MidiMapping& mm : _activeScore->masterScore()->midiMapping()) {
            const Channel* ch = mm.articulation();
            if (ch && (ch->mute() || ch->soloMute()))
                  seq->stopNotes(ch->channel());
            }
      }

//---------------------------------------------------------
//   retranslate
//---------------------------------------------------------

void Mixer::retranslate(bool firstTime)
      {
      setWindowTitle(tr("Mixer"));
      if (!firstTime) {
            for (int i = 0; i < trackAreaLayout->count(); i++) {
                  PartEdit* p = getPartAtIndex(i);
                  if (p) p->retranslateUi(p);
                  }
            }
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void Mixer::closeEvent(QCloseEvent* ev)
      {
      emit closed(false);
      QWidget::closeEvent(ev);
      }

//---------------------------------------------------------
//   showEvent
//---------------------------------------------------------

void Mixer::showEvent(QShowEvent* e)
      {
      enablePlay->showEvent(e);
      QWidget::showEvent(e);
      activateWindow();
      setFocus();
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool Mixer::eventFilter(QObject* obj, QEvent* e)
      {
      if (enablePlay->eventFilter(obj, e))
            return true;
      return QWidget::eventFilter(obj, e);
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void Mixer::keyPressEvent(QKeyEvent* ev) {
      if (ev->key() == Qt::Key_Escape && ev->modifiers() == Qt::NoModifier) {
            close();
            return;
            }
      QWidget::keyPressEvent(ev);
      }

//---------------------------------------------------------
//   changeEvent
//---------------------------------------------------------

void Mixer::changeEvent(QEvent *event)
      {
      QWidget::changeEvent(event);
      if (event->type() == QEvent::LanguageChange)
            retranslate();
      }

//---------------------------------------------------------
//   partEdit
//---------------------------------------------------------

PartEdit* Mixer::getPartAtIndex(int)
      {
      return 0;
      }

//---------------------------------------------------------
//   setPlaybackScore
//---------------------------------------------------------

void Mixer::setPlaybackScore(Score* score)
      {
      if (_score != score) {
            _score = score;
            //mixerDetails->setTrack(0);
            }
      updateTracks();
      }

//---------------------------------------------------------
//   setScore
//---------------------------------------------------------

void Mixer::setScore(Score* score)
      {
      // No equality check, this function seems to need to cause
      // mixer update every time it gets called.
      _activeScore = score;
      setPlaybackScore(_activeScore ? _activeScore->masterScore()->playbackScore() : nullptr);

      partOnlyCheckBox->setChecked(mscore->playPartOnly());
      partOnlyCheckBox->setEnabled(_activeScore && !_activeScore->isMaster());
      }

MixerTrackChannel* Mixer::mixerRowWidget(MixerTrackItem* mixerTrackItem)
      {
      //QLabel* labelWidget = new QLabel(name);

      QSlider* slider = new QSlider(Qt::Horizontal);
      slider->setValue(50);

      QHBoxLayout* sliderLayout = new QHBoxLayout();
      sliderLayout->setContentsMargins(0, 0, 10, 0);
      sliderLayout->addWidget(slider);

      QWidget* itemWidget = new QWidget();

      QVBoxLayout *layout = new QVBoxLayout;
      layout->setAlignment(Qt::AlignVCenter);
      //layout->addWidget(labelWidget);
      layout->addLayout(sliderLayout);
      layout->setMargin(0);
      itemWidget->setLayout(layout);
      itemWidget->setMinimumHeight(25);
      return new MixerTrackChannel(this, mixerTrackItem);
      }


      //TODO: panning tooltip
      /*
       panSlider->setValue(channel->pan());
       panSlider->setToolTip(tr("Pan: %1").arg(QString::number(channel->pan())));
       panSlider->setMaxValue(127);
       panSlider->setMinValue(0);
       */

      /*
       case Channel::Prop::PAN: {
       //                  panSlider->blockSignals(true);
       //                  panSlider->setValue(channel->pan());
       //                  panSlider->setToolTip(tr("Pan: %1").arg(QString::number(channel->pan())));
       //                  panSlider->blockSignals(false);
       break;
       }
       */
      //void MixerTrackChannel::panChanged(double value)
      //      {
      //      _mixerTrackItem->setPan(value);
      //      panSlider->setToolTip(tr("Pan: %1").arg(QString::number(value)));
      //      }


//---------------------------------------------------------
//   updateTracks
//---------------------------------------------------------

void Mixer::updateTracks()
      {
      //MixerTrackItem* oldSel = mixerDetails->track().get();

      Part* selPart = 0; //oldSel ? oldSel->part() : 0;
      Channel* selChan = 0; //oldSel ? oldSel->chan() : 0;

      if (_score && !selPart) {
            //If nothing selected, select first available track
            if (!_score->parts().isEmpty())
                  {
                  selPart = _score->parts()[0]->masterPart();
                  selChan = selPart->instrument(Fraction(0,1))->playbackChannel(0, _score->masterScore());
                  }
            }

      mixerTreeWidget->clear();
      trackList.clear();

      if (!_score)
            return;

      mixerTreeWidget->setAlternatingRowColors(true);
      //mixerTreeWidget->headerItem()->setHidden(true);
      mixerTreeWidget->setColumnCount(2);
      mixerTreeWidget->setHeaderLabels({"Part", "Volume"});

      for (Part* localPart : _score->parts()) {
            Part* part = localPart->masterPart();

            const InstrumentList* instrumenList = part->instruments();

            Instrument* proxyInstr = nullptr;
            Channel* proxyChan = nullptr;
            if (!instrumenList->empty()) {
                  instrumenList->begin();
                  proxyInstr = instrumenList->begin()->second;
                  proxyChan = proxyInstr->playbackChannel(0, _score->masterScore());
                  }

            //MixerTrackItem::MixerTrackItem(TrackType trackType, Part* part, Instrument* instrument, Channel *channel)


            MixerTrackItem* mixerTrackItem = new MixerTrackItem(MixerTrackItem::TrackType::PART, part, proxyInstr, proxyChan);
            // we want to use the ITEM to set up the itemWidget
            // at time of writing this comment, the itemWidget is hand-rolled in a function
            // need to refactor it into a class AND think about the model...
            // AND the key details are, where, in  _score->parts()->mastetPart->instruments ??
            // the MixerTrackItem keeps a track of all this, so it is a good (good-ish) MODEL object
            // where do I want to KEEP the mixerTrackItem - could have it as an ivar on
            // the itemWidget in the tree view... OR maintain a LIST of them in the mixer class?
            // IS this updateTracks method called a lot? Does it have any particular overhead?



            QStringList columns = QStringList(part->partName());

            char volume = mixerTrackItem->getVolume();
            qDebug()<<"Volume for "<<part->partName()<<" is:"<<int(volume);
            // there is no getVolume - how, on current setup, does the mixer control get the current value
            // need to dig into the code more to find out...

            columns.append("");
            QTreeWidgetItem* item = new QTreeWidgetItem((QTreeWidget*)0, columns);
            mixerTreeWidget->addTopLevelItem(item);
            mixerTreeWidget->setItemWidget(item, 1, mixerRowWidget(mixerTrackItem));


            //Add per channel tracks
            const InstrumentList* il1 = part->instruments();
            for (auto it = instrumenList->begin(); it != il1->end(); ++it) {
                  Instrument* instr = it->second;

                  if (instr->channel().size() <= 1)
                        continue;

                  for (int i = 0; i < instr->channel().size(); ++i) {
                        Channel* chan = instr->playbackChannel(i, _score->masterScore());

                        QStringList columns = QStringList(chan->name());
                        columns.append("");
                        QTreeWidgetItem* child = new QTreeWidgetItem((QTreeWidget*)0, columns);
                        item->addChild(child);

                        //TODO: this needs to get its own mixerTrackItem for the sub-part
                        mixerTreeWidget->setItemWidget(child, 1, mixerRowWidget(mixerTrackItem));

                        }
                  }
            }

      }

//---------------------------------------------------------
//   midiPrefsChanged
//---------------------------------------------------------

void Mixer::midiPrefsChanged(bool)
      {
      updateTracks();
      }



//---------------------------------------------------------
//   notifyTrackSelected
//---------------------------------------------------------

/*void Mixer::notifyTrackSelected(MixerTrack* track)
      {
      for (MixerTrack *mt: trackList) {
            if (!(mt->mti()->part() == track->mti()->part() &&
                  mt->mti()->chan() == track->mti()->chan() &&
                  mt->mti()->trackType() == track->mti()->trackType())) {
                  mt->setSelected(false);
                  }
            }
      //mixerDetails->setTrack(track->mti());
      } */

void Mixer::currentItemChanged()
      {
            qDebug()<<"Row change in tree view (and maybe column change too)";
            qDebug()<<mixerTreeWidget->currentItem();
      }


//---------------------------------------------------------
//   showMixer
//---------------------------------------------------------

void MuseScore::showMixer(bool val)
      {
      if (!cs)
            return;

      QAction* a = getAction("toggle-mixer");
      if (mixer == 0) {
            mixer = new Mixer(this);
            mscore->stackUnder(mixer);
            if (synthControl)
                  connect(synthControl, SIGNAL(soundFontChanged()), mixer, SLOT(updateTrack()));
            connect(synti, SIGNAL(soundFontChanged()), mixer, SLOT(updateTracks()));
            connect(mixer, SIGNAL(closed(bool)), a, SLOT(setChecked(bool)));
            }
      mixer->setScore(cs);
      mixer->setVisible(val);
      }

}
