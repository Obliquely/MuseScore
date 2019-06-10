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
      
enum class ColorRole;

class MuseColors {
   public:
      QColor color(QPalette::ColorGroup, QPalette::ColorRole);    // access Qt Palette
      QColor color(QPalette::ColorGroup, Ms::ColorRole);          // access non-Qt palette colors
      QColor color(QPalette::ColorRole role);
      QColor color(Ms::ColorRole);
      };
} // namespace Ms
#endif /* __MUSECOLORS_H__ */
