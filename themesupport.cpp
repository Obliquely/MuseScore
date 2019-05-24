//
//  themesupport.cpp
//  mscore
//
//  Created by Matthew Elton on 23/05/2019.
//

#include <stdio.h>
#include "musescore.h"
#include <QStyleFactory>
#include "preferences.h"
#include "icons.h"
#include "shortcut.h"


namespace Ms {
void logPaletteItem(QPalette palette, QPalette::ColorRole role)
      {
      QMetaEnum metaEnum = QMetaEnum::fromType<QPalette::ColorRole>();
      QString roleName = metaEnum.valueToKey(role);
      QString label = roleName.rightJustified(16, ' ');
      qDebug()<<label<<": "<<palette.color(QPalette::Active, role).name()<<"  ("<<palette.color(QPalette::Disabled, role).name()<<")";
      }


void logPalette()
      {
      QPalette palette = QPalette(QApplication::palette());

      logPaletteItem(palette, QPalette::Button);
      logPaletteItem(palette, QPalette::ButtonText);
      logPaletteItem(palette, QPalette::Highlight);
      qDebug(); // blank line
}

void MuseScore::experimentalUpdateUiStyleAndTheme()
      {
      QApplication::setStyle(QStyleFactory::create("Fusion"));

      qDebug()<<"Before any theme work at all, i.e. defaults, the palette is:";
      logPalette();

      QString userCustomizingFilesPath = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QCoreApplication::applicationName());

      QString paletteFilename = preferences.isThemeDark() ? "palette_fresh_fusion.json" : "palette_fresh_fusion.json";
      qDebug()<<"paletteFilename"<<QString(":/themes/%1").arg(paletteFilename);
      QFile paletteFile(QString(":/themes/%1").arg(paletteFilename));

      if (QFile::exists(QString("%1/%2").arg(userCustomizingFilesPath, "palette_fresh_fusion.json")))
                  paletteFile.setFileName(QString("%1/%2").arg(userCustomizingFilesPath, "palette_fresh_fusion.json"));

      QPalette palette = QPalette(QApplication::palette());

      if (paletteFile.open(QFile::ReadOnly | QFile::Text)) {
            QJsonDocument themeDocument = QJsonDocument::fromJson(paletteFile.readAll());

            // the top level of the (new format) palette settings is s set of
            // color group keys
            QJsonObject colorGroupSettings = themeDocument.object();

            QMetaEnum colorGroups = QMetaEnum::fromType<QPalette::ColorGroup>();

            // iterate through all the available color group keys and whenever
            // there is a match, get the colorRoles object and add to palette
            for (int colorGroup = 0; colorGroup < colorGroups.keyCount(); ++colorGroup) {
                  QJsonValue value = colorGroupSettings.value(colorGroups.valueToKey(colorGroup));
                  if (value.isUndefined())
                        continue;
                  qDebug()<<"Found colorGroup: "<<colorGroups.valueToKey(colorGroup)<<" - "<<value;

                  QJsonObject colorRoleSettings = value.toObject(); //NOT SURE about this line - it's a guess
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
      }
      QApplication::setPalette(palette);

      qDebug()<<"After theme work the palette is:";
      logPalette();


      QString css;
      QString styleFilename = preferences.isThemeDark() ? "style_dark_fusion.css" : "style_light_fusion.css";
      QFile fstyle(QString(":/themes/%1").arg(styleFilename));

      if (fstyle.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&fstyle);
            css = in.readAll();
      }

      css.replace("$voice1-bgcolor", MScore::selectColor[0].name(QColor::HexRgb));
      css.replace("$voice2-bgcolor", MScore::selectColor[1].name(QColor::HexRgb));
      css.replace("$voice3-bgcolor", MScore::selectColor[2].name(QColor::HexRgb));
      css.replace("$voice4-bgcolor", MScore::selectColor[3].name(QColor::HexRgb));
      qApp->setStyleSheet(css);

      QString style = QString("*, QSpinBox { font: %1pt \"%2\" } ")
      .arg(QString::number(preferences.getInt(PREF_UI_THEME_FONTSIZE)), preferences.getString(PREF_UI_THEME_FONTFAMILY))
      + qApp->styleSheet();
      qApp->setStyleSheet(style);

      genIcons();
      Shortcut::refreshIcons();
      }
}
