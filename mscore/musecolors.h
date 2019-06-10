//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2019 Werner Schweer and others
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

#ifndef __MUSECOLORS_H__
#define __MUSECOLORS_H__


#include "globals.h"

namespace Ms {
      
enum class ColorRole : char {
            CUSTOM1, CUSTOM2, CUSTOM3
      };
      
class MuseColor {
            
      public:
            QColor color(Qt::ColorGroup, Qt::ColorRole);    // access Qt Palette
            QColor color(Qt::ColorGroup, Ms::ColorRole);    // access non-Qt palette colors
            QColor color(Qt::ColorRole role) { return QColor(Qt::Active, role ); }
            QColor color(Qt::ColorRole role) { return QColor(Qt::Active, role ); }
      };

}
#endif /* __MUSECOLORS_H__ */
