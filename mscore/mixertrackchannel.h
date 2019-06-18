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

#ifndef __MIXERTRACKCHANNEL_H__
#define __MIXERTRACKCHANNEL_H__

#include "ui_mixertrackchannel.h"
#include "mixertrackgroup.h"
#include "mixertrack.h"
#include "mixertrackitem.h"
#include "libmscore/instrument.h"

// obq-note - re-purposing this for thw new mixer design

namespace Ms {

class MidiMapping;
class MixerTrackItem;

//---------------------------------------------------------
//   MixerTrack
//---------------------------------------------------------

class MixerTrackChannel : public QWidget, public Ui::MixerTrackChannel, public ChannelListener
      {
      Q_OBJECT

      MixerTrackItem* mixerTrackItem;

      void setupAdditionalUi();
      void setupSlotsAndSignals();
      void update();

public slots:
      void updateSolo(bool);
      void updateMute(bool);
      void volumeChanged(int);

protected:
      void propertyChanged(Channel::Prop property) override;      // ChannelListener method
            
public:
      explicit MixerTrackChannel(QWidget *parent, MixerTrackItem* mixerTrackItem);
      MixerTrackItem* getMixerTrackItem() { return mixerTrackItem; }    //TODO: obq-note is this used?
      };
}

#endif // __MIXERTRACKCHANNEL_H__
