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

#ifndef __ILEDIT_H__
#define __ILEDIT_H__

//#include "ui_parteditbase.h"
#include "ui_mixer.h"
#include "libmscore/instrument.h"
#include "enableplayforwidget.h"
#include "mixertrackgroup.h"
#include "mixertrackchannel.h"
#include "mixerdetails.h"
#include <QWidget>
#include <QDockWidget>
#include <QScrollArea>
#include <QList>

namespace Ms {

class Score;
class Channel;
class Part;
//class PartEdit;
class MixerTrack;
class MixerDetails;
class MidiMapping;
class MixerKeyboardControlFilter;
//class MixerOptions;
class MixerContextMenu;
class MixerOptions;

double volumeToUserRange(char v);
double panToUserRange(char v);
double chorusToUserRange(char v);
double reverbToUserRange(char v);

//0 to 100
char userRangeToVolume(double v);
//-180 to 180
char userRangeToPan(double v);
//0 to 100
char userRangeToChorus(double v);
//0 to 100
char userRangeToReverb(double v);


//---------------------------------------------------------
//   Mixer
//---------------------------------------------------------

/* obq-notes
      MixerTrackGroup provides,
      mmm, a sort of PROTOCOL allowing things other than the Mixer to be the
      target for cetain signals??

      Not very clear when/how GROUP is used. Maybe the mixer IS the only group?
      Might have something to do with PARTS etc.
 */

enum class MixerVolumeMode : int
      {
            Override,
            Ratio,
            PrimaryInstrument
      };

class Mixer : public QDockWidget, public Ui::Mixer
      {
      Q_OBJECT

      Score* _score = nullptr;                        // playback score
      Score* _activeScore = nullptr;                  // may be a _score itself or its excerpt;
      MixerContextMenu* contextMenu;                  // context menu
      QGridLayout* gridLayout;                        // main layout - used to show/hide & position details panel
      MixerOptions* options;                          // UI options, e.g. show/hide track colors

      EnablePlayForWidget* enablePlay;

      QSet<Part*> expandedParts;                      //TOD: expandedParts - from old mixer code - re-implement
      QList<MixerTrack*> trackList;                   //TO:  trackLIst - from old mixer code - may re-use
      int savedSelectionTopLevelIndex;
      int savedSelectionChildIndex;

      MixerKeyboardControlFilter* keyboardFilter;     // process key presses for the mixer AND the details panel
      virtual void closeEvent(QCloseEvent*) override;
      virtual void showEvent(QShowEvent*) override;
      virtual bool eventFilter(QObject*, QEvent*) override;
      virtual void keyPressEvent(QKeyEvent*) override;

      void setupSlotsAndSignals();
      void disableMixer();                            // gray out everything when no score or part is open
      void setPlaybackScore(Score*);

      MixerTrackItem* mixerTrackItemFromPart(Part* part);

   private slots:
      void on_partOnlyCheckBox_toggled(bool checked);

   public slots:
      void updateTracks();
      void midiPrefsChanged(bool showMidiControls);
      void masterVolumeChanged(double val);
      void currentMixerTreeItemChanged();
      void synthGainChanged(float val);
      void showDetailsClicked();
      void showDetailsBelow();
      void showMidiOptions();
      void showTrackColors();

      void saveTreeSelection();
      void restoreTreeSelection();
            
   signals:
      void closed(bool);

   protected:
      virtual void changeEvent(QEvent *event) override;
      void retranslate(bool firstTime = false);

   public:
      Mixer(QWidget* parent);
      void setScore(Score*);
      //PartEdit* getPartAtIndex(int index);                      // from old mixer code (appears redundant)
      void contextMenuEvent(QContextMenuEvent *event) override;   // TODO: contextMenuEvent - does it need to be public?
      MixerDetails* mixerDetails;                                 // TODO: mixerDetails - does it NEED to be public?
      void updateUiOptions();
      MixerOptions* getOptions() { return options; };
            
      };

class MixerKeyboardControlFilter : public QObject
      {
      Q_OBJECT
      Mixer* mixer;
   protected:
      bool eventFilter(QObject *obj, QEvent *event) override;
      
   public:
      MixerKeyboardControlFilter(Mixer* mixer);
      };

class MixerContextMenu : public QObject
      {
      Q_OBJECT
      Mixer* mixer;

public:
      MixerContextMenu(Mixer* mixer);
      void contextMenuEvent(QContextMenuEvent *event);
      
      QAction* detailToSide;
      QAction* detailBelow;
      QAction* showMidiOptions;
      QAction* panSliderInMixer;
      QAction* overallVolumeOverrideMode;
      QAction* overallVolumeRatioMode;
      QAction* overallVolumeFirstMode;
      QAction* showTrackColors;
      };

      
class MixerOptions
      {
   public:
      MixerOptions(bool showTrackColors, bool detailsOnTheSide, bool showMidiOptions, MixerVolumeMode mode);
      MixerOptions();
      bool showTrackColors;
      bool detailsOnTheSide;
      bool showMidiOptions;
      MixerVolumeMode mode;
      };
      
} // namespace Ms
#endif

