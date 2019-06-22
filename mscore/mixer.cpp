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
//#include "parteditbase.h"

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
    : QDockWidget("Mixer", parent)
      {
      setupUi(this);

      setWindowFlags(Qt::Tool);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

      mixerTreeWidget->setAlternatingRowColors(true);
      mixerTreeWidget->setColumnCount(2);
      mixerTreeWidget->setHeaderLabels({tr("Instrument"), tr("Volume")});
      mixerTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
      mixerTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
      mixerTreeWidget->header()->setDefaultSectionSize(168);
      mixerTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

      mixerDetails = new MixerDetails(this);
      contextMenu = new MixerContextMenu(this);

      showingTrackColors = false;      // grab this from a saved preference!

      // default layout - and we can re-organise in code
      //TODO: implement layout re-org from context menu
      gridLayout = new QGridLayout(dockWidgetContents);
      gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
      gridLayout->addWidget(mixerDetails, 0, 2, 1, 1);
      gridLayout->addWidget(partOnlyCheckBox, 1, 0, 1, 1);
      gridLayout->addWidget(showDetailsButton, 1, 1, 1, 1);
      gridLayout->addWidget(mixerTreeWidget, 0, 0, 1, 2);

      keyboardFilter = new MixerKeyboardControlFilter(this);
      this->installEventFilter(keyboardFilter);
      mixerTreeWidget->installEventFilter(keyboardFilter);
      
      enablePlay = new EnablePlayForWidget(this);
      retranslate(true);
      setupSlotsAndSignals();
      }

MixerKeyboardControlFilter::MixerKeyboardControlFilter(Mixer* mixer) : mixer(mixer)
      {
      }

MixerContextMenu::MixerContextMenu(Mixer* mixer) : mixer(mixer)
      {
      detailToSide = new QAction("Show Details Below");
      detailToSide->setCheckable(true);
      detailToSide->setChecked(true);     // default (for testing)
      detailBelow = new QAction("Show Details to the Side");
      detailBelow->setCheckable(true);
      panSliderInMixer = new QAction("Show Pan Slider in Mixer");
      overallVolumeOverrideMode = new QAction("Overall volume: override");
      overallVolumeRatioMode = new QAction("Overall volume: ratio");
      overallVolumeFirstMode = new QAction("Overall volume: first channel");
      showTrackColors = new QAction(tr("Track Colors"));
      showTrackColors->setCheckable(true);

      detailToSide->setStatusTip(tr("Detailed options shown below the mixer"));
      connect(detailToSide, &QAction::triggered, mixer, &Mixer::verticalStacking);
      connect(showTrackColors, SIGNAL(changed()), mixer, SLOT(showTrackColors()));
      }

void MixerContextMenu::contextMenuEvent(QContextMenuEvent *event)
{
      QMenu menu(mixer);
      menu.addSection("Geometry");

      QActionGroup* geometryGroup = new QActionGroup(mixer);

      detailToSide->setActionGroup(geometryGroup);
      detailBelow->setActionGroup(geometryGroup);
      geometryGroup->setExclusive(true);

      geometryGroup->addAction(detailBelow);
      menu.addAction(detailToSide);
      menu.addAction(detailBelow);


      menu.addSection("Controls");
      menu.addAction(panSliderInMixer);
      menu.addAction(showTrackColors);
      menu.addSection("Overall volume");
      menu.addAction(overallVolumeRatioMode);
      menu.addAction(overallVolumeFirstMode);
      menu.addAction(overallVolumeFirstMode);
      menu.exec(event->globalPos());
}

void Mixer::verticalStacking()
      {
      qDebug()<<"Vertical stacking menu item triggered.";
      }

void Mixer::showTrackColors()
      {
      showingTrackColors = contextMenu->showTrackColors->isChecked();
      qDebug()<<"showTrackColors: "<<showingTrackColors;

      mixerDetails->updateUiOptions();

      for (int topLevelIndex = 0; topLevelIndex < mixerTreeWidget->topLevelItemCount(); topLevelIndex++) {
            QTreeWidgetItem* topLevelItem = mixerTreeWidget->topLevelItem(topLevelIndex);
            MixerTrackChannel* itemWidget = static_cast<MixerTrackChannel*>(mixerTreeWidget->itemWidget(topLevelItem, 1));
            itemWidget->updateUiControls(this);

            for (int childIndex = 0; childIndex < topLevelItem->childCount(); childIndex++) {
                  QTreeWidgetItem* childItem = topLevelItem->child(childIndex);
                  MixerTrackChannel* itemWidget = static_cast<MixerTrackChannel*>(mixerTreeWidget->itemWidget(childItem, 1));
                  itemWidget->updateUiControls(this);
                  }
            }
      }


void Mixer::contextMenuEvent(QContextMenuEvent *event)
{
      contextMenu->contextMenuEvent(event);
}

void Mixer::setupSlotsAndSignals()
      {
      connect(mixerTreeWidget,SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),SLOT(currentItemChanged()));
      connect(synti,SIGNAL(gainChanged(float)),SLOT(synthGainChanged(float)));
      connect(showDetailsButton,SIGNAL(clicked()),SLOT(showDetailsClicked()));
      }

void Mixer::showDetailsClicked()
{
      QSize currentTreeWidgetSize = mixerTreeWidget->size();
      QSize minTreeWidgetSize = mixerTreeWidget->minimumSize();   // respect settings from QT Creator / QT Designer
      QSize maxTreeWidgetSize = mixerTreeWidget->maximumSize();   // respect settings from QT Creator / QT Designer

      qDebug()<<"treeWidget current size: "<<currentTreeWidgetSize;

      mixerTreeWidget->setMinimumSize(currentTreeWidgetSize);
      mixerTreeWidget->setMaximumSize(currentTreeWidgetSize);
      mixerDetails->setVisible(!mixerDetails->isVisible());
      mixerTreeWidget->adjustSize();
      dockWidgetContents->adjustSize();
      this->adjustSize();
      mixerTreeWidget->setMinimumSize(minTreeWidgetSize);
      mixerTreeWidget->setMaximumSize(maxTreeWidgetSize);
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
//      if (!firstTime) {
//            for (int i = 0; i < trackAreaLayout->count(); i++) {
//                  PartEdit* p = getPartAtIndex(i);
//                  if (p) p->retranslateUi(p);
//                  }
//            }
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

bool Mixer::eventFilter(QObject* object, QEvent* event)
      {
      if (enablePlay->eventFilter(object, event))
            return true;

      if (object == mixerDetails->panSlider) {
            if (event->type() == QEvent::MouseButtonDblClick) {
                  QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
                  qDebug() << "Ate Double click on pan slider" << keyEvent->key();
                  mixerDetails->resetPanToCentre();
                  return true;
                  }
      }

      return QWidget::eventFilter(object, event);
      }

      
bool MixerKeyboardControlFilter::eventFilter(QObject *obj, QEvent *event)
      {
      
      MixerTrackItem* selectedMixerTrackItem = mixer->mixerDetails->getSelectedMixerTrackItem();
      
      if (event->type() != QEvent::KeyPress) {
            return QObject::eventFilter(obj, event);
      }
      
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
      
      if (keyEvent->key() == Qt::Key_Period && keyEvent->modifiers() == Qt::NoModifier) {
            qDebug()<<"Volume up keyboard command";
            if (selectedMixerTrackItem && int(selectedMixerTrackItem->getVolume()) < 128) {
                  selectedMixerTrackItem->setVolume(selectedMixerTrackItem->getVolume() + 1);
            }
            return true;
      }
      
      if (keyEvent->key() == Qt::Key_Comma && keyEvent->modifiers() == Qt::NoModifier) {
            qDebug()<<"Volume down keyboard command";
            if (selectedMixerTrackItem && int(selectedMixerTrackItem->getVolume()) >0) {
                  selectedMixerTrackItem->setVolume(selectedMixerTrackItem->getVolume() - 1);
            }
            return true;
      }
      
      if (keyEvent->key() == Qt::Key_Less && keyEvent->modifiers() == Qt::ShiftModifier) {
            qDebug()<<"Pan left keyboard command";
            if (selectedMixerTrackItem && int(selectedMixerTrackItem->getPan()) < 128) {
                  selectedMixerTrackItem->setPan(selectedMixerTrackItem->getPan() + 1);
            }
            return true;
      }
      
      if (keyEvent->key() == Qt::Key_Greater && keyEvent->modifiers() == Qt::ShiftModifier) {
            qDebug()<<"Pan right keyboard command";
            if (selectedMixerTrackItem && int(selectedMixerTrackItem->getPan()) >0) {
                  selectedMixerTrackItem->setPan(selectedMixerTrackItem->getPan() - 1);
            }
            return true;
      }
      
      if (keyEvent->key() == Qt::Key_M && keyEvent->modifiers() == Qt::NoModifier) {
            qDebug()<<"Mute (M) keyboard command";
            if (selectedMixerTrackItem) {
                  selectedMixerTrackItem->setMute(!selectedMixerTrackItem->getMute());
            }
            return true;
      }
      
      if (keyEvent->key() == Qt::Key_S && keyEvent->modifiers() == Qt::NoModifier) {
            qDebug()<<"Solo (S) keyboard command";
            if (selectedMixerTrackItem) {
                  selectedMixerTrackItem->setSolo(!selectedMixerTrackItem->getSolo());
            }
            return true;
      }
      
      return QObject::eventFilter(obj, event);
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
//
//PartEdit* Mixer::getPartAtIndex(int)
//      {
//      return 0;
//      }

//---------------------------------------------------------
//   setPlaybackScore
//---------------------------------------------------------

void Mixer::setPlaybackScore(Score* score)
      {
      qDebug()<<"Mixer::setPlayBackScore";
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


void Mixer::disableMixer()
      {
      //TODO: no parts or tracks present, so grey everything out
      }


void Mixer::updateTreeSelection()
      {
      if (mixerTreeWidget->topLevelItemCount() == 0) {
            disableMixer();
            return;
      }

      if (!mixerTreeWidget->currentItem()) {
            mixerTreeWidget->setCurrentItem(mixerTreeWidget->topLevelItem(0));
      }

      MixerTrackChannel* itemWidget = static_cast<MixerTrackChannel*>(mixerTreeWidget->itemWidget(mixerTreeWidget->currentItem(), 1));

      MixerTrackItem* mixerTrackItem = itemWidget->getMixerTrackItem();
      if (mixerTrackItem) {
            mixerDetails->updateDetails(mixerTrackItem);
      }
      else {
            qDebug()<<"**MIXER WARNING** Unexpected missing mixerTrackItem **. Non-fatal.";
            }
      //older code

//      Part* selPart = 0;
//      Channel* selChan = 0;
//
//      if (_score) {
//            //If nothing selected, select first available track
//            if (!_score->parts().isEmpty())
//            {
//                  selPart = _score->parts()[0]->masterPart();
//                  selChan = selPart->instrument(Fraction(0,1))->playbackChannel(0, _score->masterScore());
//            }
//      }
      }


MixerTrackItem* Mixer::mixerTrackItemFromPart(Part* part)
      {
      const InstrumentList* instrumenList = part->instruments();

      // why is it called a proxy instrument here
      // is it because, well, it IS and this represents the part
      // but we somehow tuck in (second) to get the first instrument in the part??
      Instrument* proxyInstr = nullptr;
      Channel* proxyChan = nullptr;
      if (!instrumenList->empty()) {
            instrumenList->begin();
            proxyInstr = instrumenList->begin()->second;
            proxyChan = proxyInstr->playbackChannel(0, _score->masterScore());
            }

      MixerTrackItem* mixerTrackItem = new MixerTrackItem(MixerTrackItem::TrackType::PART, part, proxyInstr, proxyChan);

      return mixerTrackItem;
      }

//---------------------------------------------------------
//   updateTracks
//---------------------------------------------------------

void Mixer::updateTracks()
      {
      qDebug()<<"Mixer::updateTracks()";

      mixerTreeWidget->clear();
      trackList.clear();

      if (!_score) {
            disableMixer();
            return;
      }

      for (Part* localPart : _score->parts()) {
            Part* part = localPart->masterPart();
            //qDebug()<<"Mixer::updateTracks() - outer loop - part->name()"<<part->longName();

            MixerTrackItem* mixerTrackItem = mixerTrackItemFromPart(part);

            QTreeWidgetItem* item = new QTreeWidgetItem(mixerTreeWidget);
            item->setText(0, part->partName());
            item->setToolTip(0, part->partName());
            mixerTreeWidget->addTopLevelItem(item);

            MixerTrackChannel* mixerTrackWidget = new MixerTrackChannel(item, mixerTrackItem);
            mixerTreeWidget->setItemWidget(item, 1, mixerTrackWidget);

            //Add per channel tracks
            const InstrumentList* partInstrumentList = part->instruments();

            // Note sure I understand the code here - will this loop ever iterate more
            // than once?
            for (auto partInstrumentListItem = partInstrumentList->begin(); partInstrumentListItem != partInstrumentList->end(); ++partInstrumentListItem) {

                  Instrument* instrument = partInstrumentListItem->second;
                  //qDebug()<<"    Mixer::updateTracks() - inner loop - instrument->trackName()"<<instrument->trackName();

                  if (instrument->channel().size() <= 1)
                        continue;

                  // add an item for each channel used by a given instrument
                  for (int i = 0; i < instrument->channel().size(); ++i) {
                        Channel* channel = instrument->playbackChannel(i, _score->masterScore());
                        //qDebug()<<"        Mixer::updateTracks() - inner loop, inner loop - channel->name()()"<<channel->name();


                        QTreeWidgetItem* child = new QTreeWidgetItem(0);
                        child->setText(0, channel->name());
                        child->setToolTip(0, QString("%1 - %2").arg(part->partName()).arg(channel->name()));
                        item->addChild(child);

                        MixerTrackItem* mixerTrackItem = new MixerTrackItem(MixerTrackItem::TrackType::CHANNEL, part, instrument, channel);
                        mixerTreeWidget->setItemWidget(child, 1, new MixerTrackChannel(child, mixerTrackItem));
                        }
                  }
            }
            mixerTreeWidget->expandAll(); // force expansion - TODO: hack to avoid issue with unwanted wigdets displaying when un-expanded
            updateTreeSelection();
      }

//---------------------------------------------------------
//   midiPrefsChanged
//---------------------------------------------------------

void Mixer::midiPrefsChanged(bool)
      {
      qDebug()<<"Mixer::midiPrefsChanged";
      updateTracks();
      }


void Mixer::currentItemChanged()
      {
      QWidget* treeItemWidget = mixerTreeWidget->itemWidget(mixerTreeWidget->currentItem(), 1);
      
      if (!treeItemWidget) {
            mixerDetails->resetDetails();
            mixerDetails->enableDetails(false);
            return;
            }

      MixerTrackChannel* itemWidget = static_cast<MixerTrackChannel*>(treeItemWidget);
      mixerDetails->updateDetails(itemWidget->getMixerTrackItem());
      qDebug()<<"about to call takeSelection";
      itemWidget->takeSelection();
      }


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
