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

#include "ui_parteditbase.h"
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
class PartEdit;
class MixerTrack;
class MixerDetails;
class MidiMapping;
class MixerKeyboardControlFilter;

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

class Mixer : public QDockWidget, public Ui::Mixer
      {
      Q_OBJECT

      Score* _score = nullptr;            // playback score
      Score* _activeScore = nullptr;      // may be a _score itself or its excerpt;
      MixerKeyboardControlFilter* keyboardFilter;
      QGridLayout* gridLayout;

      EnablePlayForWidget* enablePlay;

      QSet<Part*> expandedParts;
      QList<MixerTrack*> trackList;

      virtual void closeEvent(QCloseEvent*) override;
      virtual void showEvent(QShowEvent*) override;
      virtual bool eventFilter(QObject*, QEvent*) override;
      virtual void keyPressEvent(QKeyEvent*) override;

      void setPlaybackScore(Score*);
      void setupSlotsAndSignals();

      void updateTreeSelection();                     // go to first item OR disable mixer

      void disableMixer();
      MixerTrackItem* mixerTrackItemFromPart(Part* part);

   private slots:
      void on_partOnlyCheckBox_toggled(bool checked);

   public slots:
      void updateTracks();
      void midiPrefsChanged(bool showMidiControls);
      void masterVolumeChanged(double val);
      
      void currentItemChanged(); // obq

      void synthGainChanged(float val);

      void showDetailsClicked();
            
   signals:
      void closed(bool);

   protected:
      virtual void changeEvent(QEvent *event) override;
      void retranslate(bool firstTime = false);

   public:
      Mixer(QWidget* parent);
      void setScore(Score*);
      PartEdit* getPartAtIndex(int index);
      void contextMenuEvent(QContextMenuEvent *event) override;
      MixerDetails* mixerDetails;
      QAction* act1;
      QAction* act2;
      QAction* act3;
      QAction* act4;
      QAction* act5;
      QAction* act6;
      void createActions();
      void verticalStacking();
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

      
      
} // namespace Ms
#endif

