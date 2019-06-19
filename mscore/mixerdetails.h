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

#include "mixer.h"

namespace Ms {

//---------------------------------------------------------
//   MixerDetailsVoiceButtonHandler
//---------------------------------------------------------

class MixerDetailsVoiceButtonHandler : public QObject
      {
      Q_OBJECT

      Mixer* _mixer;
      int _staff;
      int _voice;
public:
      MixerDetailsVoiceButtonHandler(Mixer* mixer, int staff, int voice, QObject* parent = nullptr)
            : QObject(parent),
              _mixer(mixer),
              _staff(staff),
              _voice(voice)
            {}

public slots:
      void setVoiceMute(bool checked)
            {
            _mixer->setVoiceMute(_staff, _voice, checked);
            }
   };
}
