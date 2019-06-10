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

#include "libmscore/mscore.h"
#include "musecolors.h"

namespace Ms {

MuseColors museColors;  // global for the MuseScore color palette

enum class ColorRole {
      CUSTOM1, CUSTOM2, CUSTOM3
};

void MuseColors::setColorGroup(QPalette::ColorGroup colorGroup, QPalette &palette, QJsonDocument themeDocument)
      {
      QJsonObject colorGroupSettings = themeDocument.object();
      QStringList keys = colorGroupSettings.keys();

      QMetaEnum colorGroups = QMetaEnum::fromType<QPalette::ColorGroup>();
      QString key = colorGroups.valueToKey(colorGroup);

      if (!keys.contains(key)) {
            qDebug()<<"setColorGroup() - JSON file does not have the requested key: "<<key;
            return;
            }

      QJsonValue value = colorGroupSettings.value(colorGroups.valueToKey(colorGroup));

      if (value.isUndefined()) {
            qDebug()<<"setColorGroup() - JSON file does not have a value for the key:"<<key;
            return;
      }

      qDebug()<<"setColorGroup() -  extracting colors for key: "<<key;

      QJsonObject colorRoleSettings = value.toObject();
      QMetaEnum colorRoles = QMetaEnum::fromType<QPalette::ColorRole>();

      for (int colorRole = 0; colorRole < colorRoles.keyCount(); ++colorRole) {
            QJsonValue value = colorRoleSettings.value(colorRoles.valueToKey(colorRole));
            if (value.isUndefined())
                  continue;

            QString colorRoleFormatted = colorRoles.valueToKey(colorRole);
            QString colorGroupFormatted = colorGroups.valueToKey(colorGroup);
            colorRoleFormatted = QString(colorRoleFormatted.rightJustified(16, ' '));
            colorGroupFormatted = colorGroupFormatted.rightJustified(8, ' ');

            qDebug()<<"Setting "<<colorRoleFormatted<<" ("<<colorGroupFormatted<<") to "<<value;

            palette.setColor(static_cast<QPalette::ColorGroup>(colorGroups.value(colorGroup)), static_cast<QPalette::ColorRole>(colorRoles.value(colorRole)), QColor(value.toString()));
            }
      }


void MuseColors::setPaletteFromFile(QString paletteFilePath)
      {
      if (!QFile::exists(paletteFilePath)) {
            qDebug()<<"Palette file '"<<paletteFilePath<<"' not present.";
            return;
            }
      qDebug()<<"Detected and applying palette from: '"<<paletteFilePath<<"'";

      QFile paletteFile(paletteFilePath);

      if (!paletteFile.open(QFile::ReadOnly | QFile::Text)) {
            qDebug()<<"Could not open the palette file.";
            return;
            }

      QJsonDocument themeDocument = QJsonDocument::fromJson(paletteFile.readAll());

      // the top level of the palette settings is a set of color group keys
      QJsonObject colorGroupSettings = themeDocument.object();

      QPalette palette = QPalette(QApplication::palette());

      // set the "All" group first, other groups can then
      // provide overrides where necessary
      setColorGroup(QPalette::All, palette, themeDocument);
      setColorGroup(QPalette::Active, palette, themeDocument);
      setColorGroup(QPalette::Disabled, palette, themeDocument);
      setColorGroup(QPalette::Inactive, palette, themeDocument);

      QApplication::setPalette(palette);
      }


void MuseColors::refresh()
      {
      // get the default from QT
      QApplication::setStyle(QStyleFactory::create("Fusion"));

      bool dark = preferences.isThemeDark();

      // always apply the built-in palette for the theme. This means the user
      // palette settings only need to include the setting they want to override
      QString paletteFilePath = QString(":/themes/%1"). arg(dark ? "palette_dark_fusion.json" : "palette_light_fusion.json");
      setPaletteFromFile(paletteFilePath);

      // if present, use the user-provided palette
      // TODO: add a check for legacy ms_palette.json and show user a warning/ some options
      QString userSettingsPath = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QCoreApplication::applicationName());
      QString userPaletteFilePath = QString("%1/%2").arg(userSettingsPath, dark ? "ms_dark_palette.json" : "ms_light_palette.json");

      // will no-op if the user file is not present
      setPaletteFromFile(userPaletteFilePath);


}
// access Qt Palette colors
QColor MuseColors::color(QPalette::ColorGroup group, QPalette::ColorRole role)
      {
      return QPalette(QApplication::palette()).color(group, role);
      }

// access MuseScore specific colors
QColor MuseColors::color(QPalette::ColorGroup group, Ms::ColorRole role)
      {
      // how to implement this
      // fallback to ACTIVE if have an empty color value


            switch (group) {
                  case QPalette::Active:
                  case QPalette::Disabled:
                  case QPalette::Inactive:
                  case QPalette::Current:
                  default:
                  return QColor(Qt::red);
            }
      }


// access Qt Palette colors with the active role
QColor MuseColors::color(QPalette::ColorRole role)
      {
      return color(QPalette::Active, role);
      }

// access MuseScore specific colors, with the active role
QColor MuseColors::color(Ms::ColorRole role)
      {
      return color(QPalette::Active, role);
      }

} // namespace Ms
