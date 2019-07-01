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

#include "libmscore/excerpt.h"
#include "libmscore/score.h"
#include "libmscore/part.h"
#include "mixer.h"
#include "seq.h"
#include "libmscore/undo.h"
#include "synthcontrol.h"
#include "synthesizer/msynthesizer.h"
#include "preferences.h"

#include <accessibletoolbutton.h>

#include "mixerdetails.h"
#include "mixertrackchannel.h"
#include "mixertrackitem.h"
#include "mixeroptions.h"
#include "mixeroptionsbutton.h"

namespace Ms {

#define _setValue(__x, __y) \
      __x->blockSignals(true); \
      __x->setValue(__y); \
      __x->blockSignals(false);

#define _setChecked(__x, __y) \
      __x->blockSignals(true); \
      __x->setChecked(__y); \
      __x->blockSignals(false);


// initialise the static
MixerOptions* Mixer::options = new MixerOptions(); // will read from settings

//---------------------------------------------------------
//   Mixer
//---------------------------------------------------------
//MARK:- create and setup
Mixer::Mixer(QWidget* parent)
      : QDockWidget("Mixer", parent)
      {

      setupUi(this);

      setWindowFlags(Qt::Tool);
      setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

      setupAdditionalUi();

      gridLayout = new QGridLayout(dockWidgetContents);
      mixerDetails = new MixerDetails(this);

      showDetails(options->showingDetails());

      keyboardFilter = new MixerKeyboardControlFilter(this);
      this->installEventFilter(keyboardFilter);
      mixerTreeWidget->installEventFilter(keyboardFilter);

      savedSelectionTopLevelIndex = -1;   // no saved selection (bit of a magic number :( )

      enablePlay = new EnablePlayForWidget(this);
      setupSlotsAndSignals();
      updateTracks();
      updateUiOptions();
      retranslate(true);

      shiftKeyMonitorTimer = new QTimer(this);
      connect(shiftKeyMonitorTimer, SIGNAL(timeout()), this, SLOT(shiftKeyMonitor()));
      shiftKeyMonitorTimer->start(100);
      }

      void Mixer::shiftKeyMonitor() {

            bool focus = hasFocus();

            if (!focus) {
                  for (QWidget* child : findChildren<QWidget*>()) {
                        if (child->hasFocus()) {
                              focus = true;
                              break;
                        }
                  }
            }


            if (!focus) {
                  if (options->secondaryModeOn) {
                        qDebug()<<"Exiting secondary mode because we don't have the focus.";
                        options->secondaryModeOn = false;
                        mixerTreeWidget->setHeaderLabels({tr("Instrument"), tr("Volume")});
                        return;
                  }
                  else {
                        return;
                  }
            }


            if (QApplication::queryKeyboardModifiers() & Qt::KeyboardModifier::ShiftModifier) {
                  if (options->secondaryModeOn)
                        return;

                  qDebug()<<"Entering secondary mode";
                  options->secondaryModeOn = true;
                  mixerTreeWidget->setHeaderLabels({tr("Instrument"), tr("Pan")});
                  return;
            }

            if (options->secondaryModeOn) {
                  options->secondaryModeOn = false;
                  qDebug()<<"Exiting secondary mode";
                  mixerTreeWidget->setHeaderLabels({tr("Instrument"), tr("Volume")});
            }
      }

void Mixer::setupSlotsAndSignals()
      {
      connect(mixerTreeWidget,SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),SLOT(currentMixerTreeItemChanged()));
      connect(synti,SIGNAL(gainChanged(float)),SLOT(synthGainChanged(float)));

      connect(mixerTreeWidget->header(), SIGNAL(geometriesChanged()), this, SLOT(adjustHeaderWidths()));
      connect(mixerTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(itemCollapsedOrExpanded(QTreeWidgetItem*)));
      connect(mixerTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(itemCollapsedOrExpanded(QTreeWidgetItem*)));
      }

void Mixer::setupAdditionalUi()
      {
      //setup the master channel widget (volume control and Play and Loop button)

      masterChannelWidget = new MixerMasterChannel();
      masterVolumeTreeWidget->clear();
      QTreeWidgetItem* masterVolumeItem = new QTreeWidgetItem(masterVolumeTreeWidget);
      masterVolumeItem->setText(0, "Master");
      masterVolumeTreeWidget->addTopLevelItem(masterVolumeItem);
      masterVolumeTreeWidget->setItemWidget(masterVolumeItem, 1, masterChannelWidget);

      masterVolumeTreeWidget->setColumnCount(2);
      masterVolumeTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Fixed);
      masterVolumeTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
      masterVolumeTreeWidget->setSelectionMode(QAbstractItemView::NoSelection);

      // configure the main mixer tree
      mixerTreeWidget->setAlternatingRowColors(true);
      mixerTreeWidget->setColumnCount(2);
      mixerTreeWidget->setHeaderLabels({tr("Instrument"), tr("Volume")});
      mixerTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Fixed);
      mixerTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);

      mixerTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
      adjustHeaderWidths();

      showDetailsButton->setTarget(this);
      }


void Mixer::adjustHeaderWidths()
      {
      int width = mixerTreeWidget->width();

      int firstColumnMaximumWidth = 300;
      double ratio = 0.4; // instrument names take up 2/5 and controls 3/5
      int margin = 2;   // factor to avoid triggering horizontal scrolling

      int column0 = int(double(width) * ratio);
      int column1 = int(double(width) * (1-ratio) - margin);

      if (column0 > firstColumnMaximumWidth) {
            column0 = firstColumnMaximumWidth;
            column1 = (width - firstColumnMaximumWidth) - margin;
            }

      mixerTreeWidget->header()->resizeSection(0, column0);
      mixerTreeWidget->header()->resizeSection(1, column1);
      masterVolumeTreeWidget->header()->resizeSection(0, column0);
      masterVolumeTreeWidget->header()->resizeSection(1, column1);
      }

//---------------------------------------------------------
//   retranslate
//---------------------------------------------------------

void Mixer::retranslate(bool firstTime)
      {
      setWindowTitle(tr("Mixer"));
      if (firstTime)
            return;

      retranslateUi(this);
      mixerDetails->retranslateUi(mixerDetails);
      //TODO: retranslate section headers
      //TODO: retranslate the "slider options" button (if it's still called that)
      //TODO: retranslate instrument names (but do they have translations)
      }

//MARK:- main interface

void MuseScore::showMixer(bool visible)
      {
      QAction* toggleMixerAction = getAction("toggle-mixer");
      if (mixer == 0) {
            mixer = new Mixer(this);
            mscore->stackUnder(mixer);
            if (synthControl)
                  connect(synthControl, SIGNAL(soundFontChanged()), mixer, SLOT(updateTrack()));
            connect(synti, SIGNAL(soundFontChanged()), mixer, SLOT(updateTracks()));
            connect(mixer, SIGNAL(closed(bool)), toggleMixerAction, SLOT(setChecked(bool)));
            mixer->setFloating(false);
            addDockWidget(Qt::RightDockWidgetArea, mixer);
            }
      reDisplayDockWidget(mixer, visible);
      toggleMixerAction->setChecked(visible);
      mixer->setScore(cs);
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



void Mixer::updateUiOptions()
      {

      // track colors and what is shown in the details list
      mixerDetails->updateUiOptions();

      // track colors in the main mixer
      for (int topLevelIndex = 0; topLevelIndex < mixerTreeWidget->topLevelItemCount(); topLevelIndex++) {
            QTreeWidgetItem* topLevelItem = mixerTreeWidget->topLevelItem(topLevelIndex);
            MixerTrackChannel* itemWidget = static_cast<MixerTrackChannel*>(mixerTreeWidget->itemWidget(topLevelItem, 1));
            itemWidget->updateUiControls();

            for (int childIndex = 0; childIndex < topLevelItem->childCount(); childIndex++) {
                  QTreeWidgetItem* childItem = topLevelItem->child(childIndex);
                  MixerTrackChannel* itemWidget = static_cast<MixerTrackChannel*>(mixerTreeWidget->itemWidget(childItem, 1));
                  itemWidget->updateUiControls();
                  }
            }

      // layout of master volume (is affected by presence or absence or track color
      masterChannelWidget->updateUiControls();

      showDetails(options->showingDetails());

      bool showMasterVol = options->showMasterVolume();

      if (options->showDetailsOnTheSide()) {
            // show TO THE SIDE case

            // addWidget(row, column, rowSpan, columnSpan, [Qt::Alignment])
            gridLayout->addWidget(partOnlyCheckBox, 0, 1, 1, 1, Qt::AlignRight);
            gridLayout->addWidget(showDetailsButton, 0, 0, 1, 1);
            gridLayout->addWidget(mixerTreeWidget, 1, 0, 1, 2);
            if (showMasterVol) {
                  gridLayout->addWidget(masterVolumeTreeWidget, 2, 0, 1, 2);
                  masterVolumeTreeWidget->setVisible(true);
                  }
            else {
                  masterVolumeTreeWidget->setVisible(false);
                  }

            gridLayout->addWidget(mixerDetails, 0, 2, showMasterVol ? 3 : 2, 1, Qt::AlignTop);
            }
      else {
            // show BELOW case

            // addWidget(row, column, rowSpan, columnSpan, [Qt::Alignment])
            gridLayout->addWidget(partOnlyCheckBox, 0, 1, 1, 1, Qt::AlignRight);
            gridLayout->addWidget(showDetailsButton, 0, 0, 1, 1);
            gridLayout->addWidget(mixerTreeWidget, 1, 0, 1, 2);
            if (showMasterVol) {
                  gridLayout->addWidget(masterVolumeTreeWidget, 2, 0, 1, 2);
                  masterVolumeTreeWidget->setVisible(true);
                  }
            else {
                  masterVolumeTreeWidget->setVisible(false);
                  }

            gridLayout->addWidget(mixerDetails, showMasterVol ? 3 : 2, 0, 1 , 2, Qt::AlignTop);

            gridLayout->setRowStretch(1,10);
            }
      }


void Mixer::showDetails(bool visible)
      {
      QSize currentTreeWidgetSize = mixerTreeWidget->size();
      QSize minTreeWidgetSize = mixerTreeWidget->minimumSize();   // respect settings from QT Creator / QT Designer
      QSize maxTreeWidgetSize = mixerTreeWidget->maximumSize();   // respect settings from QT Creator / QT Designer

      if (!isFloating() && visible) {
            // Special case - make the mixerTreeView as narrow as possible before showing the
            // detailsView. Without this step, mixerTreeView will be as fully wide as the dock
            // and when the detailsView is added it will get even wider. (And if the user toggles QT
            // will keep making the dock / mainWindow wider and wider, which is highly undesirable.)
            mixerTreeWidget->setMaximumSize(minTreeWidgetSize);
            mixerDetails->setVisible(visible);
            dockWidgetContents->adjustSize();
            mixerTreeWidget->setMaximumSize(maxTreeWidgetSize);
            return;
            }

      // Pin the size of the mixerView when either showing or hiding the details view.
      // This ensures that the mixer window (when undocked) will shrink or grow as
      // appropriate.
      mixerTreeWidget->setMinimumSize(currentTreeWidgetSize);
      mixerTreeWidget->setMaximumSize(currentTreeWidgetSize);
      mixerDetails->setVisible(visible);
      mixerTreeWidget->adjustSize();
      dockWidgetContents->adjustSize();
      this->adjustSize(); // All three adjustSize() calls (appear) to be required
      mixerTreeWidget->setMinimumSize(minTreeWidgetSize);
      mixerTreeWidget->setMaximumSize(maxTreeWidgetSize);
      }

//---------------------------------------------------------
//   on_partOnlyCheckBox_toggled
//---------------------------------------------------------

void Mixer::on_partOnlyCheckBox_toggled(bool checked)
      {

      if (!_activeScore || !_activeScore->excerpt())
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


//MARK:- listen to changes from elsewhere
//---------------------------------------------------------
//   synthGainChanged
//---------------------------------------------------------

void Mixer::synthGainChanged(float)
      {
      masterChannelWidget->volumeChanged(synti->gain());
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
//   midiPrefsChanged
//---------------------------------------------------------

void Mixer::midiPrefsChanged(bool)
      {
      qDebug()<<"Mixer::midiPrefsChanged";
      updateTracks();
      }


//MARK:- event handling

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
      getAction("toggle-mixer")->setChecked(true);
      }


//---------------------------------------------------------
//   hideEvent
//---------------------------------------------------------

void Mixer::hideEvent(QHideEvent* e)
      {
      QWidget::hideEvent(e);
      getAction("toggle-mixer")->setChecked(false);
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



//MARK:- manage the mixer tree

//---------------------------------------------------------
//   updateTracks
//---------------------------------------------------------
void Mixer::updateTracks()
      {
      qDebug()<<"Mixer::updateTracks()";
      const QSignalBlocker blockTreeWidgetSignals(mixerTreeWidget);  // block during this method

      mixerTreeWidget->clear();

      if (!_score) {
            disableMixer();
            return;
            }

      for (Part* localPart : _score->parts()) {
            Part* part = localPart->masterPart();
            // When it's created the item will also reate any children and setup their widgets
            MixerTreeWidgetItem* item = new MixerTreeWidgetItem(part, _score, mixerTreeWidget);
            mixerTreeWidget->addTopLevelItem(item);
            mixerTreeWidget->setItemWidget(item, 1, new MixerTrackChannel(item));

            qDebug()<<"Part: "<<part->partName()<<" is expanded: "<<part->isExpanded();
            // TODO: test re-implementing remembering expansion state
            item->setExpanded(part->isExpanded());
            }

      if (savedSelectionTopLevelIndex == -1 && mixerTreeWidget->topLevelItemCount() > 0) {
            mixerTreeWidget->setCurrentItem(mixerTreeWidget->itemAt(0,0));
            currentMixerTreeItemChanged();
            }
      }

void Mixer::itemCollapsedOrExpanded(QTreeWidgetItem* item) {
      qDebug()<<"item "<<item->text(0)<<" is expanded = "<<item->isExpanded();

      MixerTreeWidgetItem* mixerTreeWidgetItem = static_cast<MixerTreeWidgetItem*>(item);
      MixerTrackItem* mixerTrackItem = mixerTreeWidgetItem->mixerTrackItem();
      if (mixerTrackItem && mixerTrackItem->isPart()) {
            mixerTrackItem->part()->setExpanded(item->isExpanded());
      }
}

void Mixer::disableMixer()
      {
      mixerDetails->setEnabled(false);
      mixerDetails->resetControls();
      }


// Used to save the item currently selected in the tree when performing operations
// such as changing the patch. The way changing patches is implemented is that it
// triggers a new setScore() method on the mixer which, in turn, and of necessity,
// forces the channel strip to built again from scratch. Not clear patch changes
// have to do this, but, currently, they do. This works around that.
      
void Mixer::saveTreeSelection()
      {
      QTreeWidgetItem* item = mixerTreeWidget->currentItem();

      if (!item) {
            savedSelectionTopLevelIndex = -1;
            return;
            }

      savedSelectionTopLevelIndex = mixerTreeWidget->indexOfTopLevelItem(item);
      if (savedSelectionTopLevelIndex != -1) {
            // current selection is a top level item
            savedSelectionChildIndex = -1;
            return;
            }

      QTreeWidgetItem* parentOfCurrentItem = item->parent();

      savedSelectionTopLevelIndex = mixerTreeWidget->indexOfTopLevelItem(parentOfCurrentItem);
      savedSelectionChildIndex = parentOfCurrentItem->indexOfChild(item);
      }


void Mixer::restoreTreeSelection()
      {
      int topLevel = savedSelectionTopLevelIndex;
      savedSelectionTopLevelIndex = -1;   // indicates no selection currently saved

      if (topLevel < 0 || mixerTreeWidget->topLevelItemCount() == 0) {
            qDebug()<<"asked to restore tree selection, but it's not possible (1)";
            return;
            }

      if (topLevel >=  mixerTreeWidget->topLevelItemCount()) {
            qDebug()<<"asked to restore tree selection, but it's not possible (2)";
            return;
            }

      QTreeWidgetItem* itemOrItsParent = mixerTreeWidget->topLevelItem(topLevel);

      if (!itemOrItsParent) {
            qDebug()<<"asked to restore tree selection, but it's not possible (3)";
            return;
            }

      if (savedSelectionChildIndex == -1) {
            mixerTreeWidget->setCurrentItem(itemOrItsParent);
            return;
            }

      if (savedSelectionChildIndex >= itemOrItsParent->childCount()) {
            qDebug()<<"asked to restore tree selection, but it's not possible (4)";
            return;
            }

      mixerTreeWidget->setCurrentItem(itemOrItsParent->child(savedSelectionChildIndex));
      }

// - listen for changes to current item so that the details view can be updated
// also called directly by updateTracks (while signals are disabled)
void Mixer::currentMixerTreeItemChanged()
      {
      if (mixerTreeWidget->topLevelItemCount() == 0 || !mixerTreeWidget->currentItem()) {
            qDebug()<<"Mixer::currentMixerTreeItemChanged called with no items in tree - ignoring)";
            return;
            }

      MixerTreeWidgetItem* item = static_cast<MixerTreeWidgetItem*>(mixerTreeWidget->currentItem());
      mixerDetails->updateDetails(item->mixerTrackItem());
      }

//MARK:- support classes

MixerKeyboardControlFilter::MixerKeyboardControlFilter(Mixer* mixer) : mixer(mixer)
      {
      }



} // namespace Ms
