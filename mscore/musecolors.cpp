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

#include "musecolors.h"
#include "libmscore/mscore.h"

// access Qt Palette
QColor color(Qt::ColorGroup, Qt::ColorRole)
      {
      return QColor(Qt::red);
      }
// access non-Qt palette colors
QColor color(Qt::ColorGroup, Ms::ColorRole)
      {
      return QColor(Qt::red);
      }
