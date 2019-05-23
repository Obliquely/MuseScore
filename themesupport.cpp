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
      logPaletteItem(palette, QPalette::Highlight);
      logPaletteItem(palette, QPalette::HighlightedText);
      qDebug(); // blank line
}

void MuseScore::experimentalUpdateUiStyleAndTheme()
      {
      QApplication::setStyle(QStyleFactory::create("Fusion"));

      qDebug()<<"Before any theme work at all, i.e. defaults, the palette is:";
      logPalette();

      QString paletteFilename = preferences.isThemeDark() ? "ex_palette_dark_fusion.json" : "ex_palette_light_fusion.json";;
      QFile paletteFile(QString(":/themes/%1").arg(paletteFilename));

      QPalette palette = QPalette(QApplication::palette());

      if (paletteFile.open(QFile::ReadOnly | QFile::Text)) {
            QJsonDocument d = QJsonDocument::fromJson(paletteFile.readAll());
            QJsonObject o = d.object();
            QMetaEnum metaEnum = QMetaEnum::fromType<QPalette::ColorRole>();
            for (int i = 0; i < metaEnum.keyCount(); ++i) {
                  QJsonValue v = o.value(metaEnum.valueToKey(i));
                  if (!v.isUndefined())
                        palette.setColor(static_cast<QPalette::ColorRole>(metaEnum.value(i)), QColor(v.toString()));
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
