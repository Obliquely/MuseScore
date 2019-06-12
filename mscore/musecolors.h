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
      
enum class ColorRole {
      SCORE_VOICE1_COLOR = 1,             //    "#1259d0"
      SCORE_VOICE2_COLOR = 2,             //    "#009234"
      SCORE_VOICE3_COLOR = 3,             //    "#c04400"
      SCORE_VOICE4_COLOR = 4,             //    "#70167a"
      CANVAS_BACKGROUND_COLOR,            //    "#dddddd"   // excluded from Advanced Prefs
      CANVAS_FOREGROUND_COLOUR,           //    "#f9f9f9"   // excluded from Advanced Prefs
      PIANO_HIGHLIGHTCOLOR,               //    "#1259d0"
      SCORE_NOTE_DROPCOLOR,               //    "#1778db"
      SCORE_DEFAULTCOLOR,                 //    "#000000"
      SCORE_FRAMEMARGINCOLOR,             //    "#5999db"
      SCORE_LAYOUTBREAKCOLOR,             //    "#5999db"

      PIANOROLL_DARK_SELECTION_BOX_COLOR, //    "#0cebff"
      PIANOROLL_DARK_NOTE_UNSEL_COLOR,    //    "#1dcca0"
      PIANOROLL_DARK_NOTE_SEL_COLOR,      //    "#ffff00"
      PIANOROLL_DARK_BG_BASE_COLOR,       //    "#3a3a3a"
      PIANOROLL_DARK_BG_KEY_WHITE_COLOR,  //    "#3a3a3a"
      PIANOROLL_DARK_BG_KEY_BLACK_COLOR,  //    "#262626"
      PIANOROLL_DARK_BG_GRIDLINE_COLOR,   //    "#111111"
      PIANOROLL_DARK_BG_TEXT_COLOR,       //    "#999999"
      PIANOROLL_LIGHT_SELECTION_BOX_COLOR,//    "#2085c3"
      PIANOROLL_LIGHT_NOTE_UNSEL_COLOR,   //    "#1dcca0"
      PIANOROLL_LIGHT_NOTE_SEL_COLOR,     //    "#ffff00"
      PIANOROLL_LIGHT_BG_BASE_COLOR,      //    "#e0e0e7"
      PIANOROLL_LIGHT_BG_KEY_WHITE_COLOR, //    "#ffffff"
      PIANOROLL_LIGHT_BG_KEY_BLACK_COLOR, //    "#e6e6e6"
      PIANOROLL_LIGHT_BG_GRIDLINE_COLOR,  //    "#a2a2a6"
      PIANOROLL_LIGHT_BG_TEXT_COLOR,      //    "#111111"
};
Q_ENUM_NS(ColorRole)

      //    enum ColorGroup { Active, Disabled, Inactive, NColorGroups, Current, All, Normal = Active };


class ThemeColorDescriptor {
      ColorRole msRole;
      QPalette::ColorRole qtRole = QPalette::NoRole;
      QString key;                              // for JSON files working mixed list of QT and MS colors
      QString legacyPreferenceKey;              // for reading old prefs to preserve user customization
      QColor active;
      QColor disabled;
      QColor inactive;
      QColor current;
      QColor nsColor;
      QString description;                      // friendly description shown in the theme editor
      QList<QPalette::ColorGroup> groups;       // group variations that can be customised
   public:
      ThemeColorDescriptor(ColorRole role, QString key, QString legacyPreferenceKey, QList<QPalette::ColorGroup> customOptions, QString description);
      ThemeColorDescriptor(QPalette::ColorRole role, QString key, QList<QPalette::ColorGroup> groups, QString description);
      };

class MuseColors {
      
   public:
      QColor color(QPalette::ColorGroup, QPalette::ColorRole);    // access Qt Palette
      QColor color(QPalette::ColorGroup, Ms::ColorRole);          // access non-Qt palette colors
      QColor color(QPalette::ColorRole role);
      QColor color(Ms::ColorRole);
      MuseColors();
      QList<ThemeColorDescriptor> themeColorSlots;

   private:
      using ColorList = QList<QColor>;
      QMap<QPalette::ColorGroup, ColorList> museColorGroups;

      void setColorGroup(QPalette::ColorGroup, QPalette&, QJsonDocument);
      void setPaletteFromFile(QString);
      void refresh();
      void populateSlots();

      };

// singleton
extern MuseColors museColors;

} // namespace Ms
#endif /* __MUSECOLORS_H__ */
