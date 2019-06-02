//=============================================================================
//  Awl
//  Audio Widget Library
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#include "slider.h"

namespace Awl {

//---------------------------------------------------------
//   Slider
//---------------------------------------------------------

Slider::Slider(QWidget* parent)
   : AbstractSlider(parent), orient(Qt::Vertical), _sliderSize(14,14)
      {
      init();
      }

//---------------------------------------------------------
//   Slider
//---------------------------------------------------------

Slider::Slider(Qt::Orientation orientation, QWidget* parent)
   : AbstractSlider(parent), orient(orientation), _sliderSize(14,14)
      {
      init();
      }

//---------------------------------------------------------
//   Slider
//---------------------------------------------------------

void Slider::init()
      {
      if (orient == Qt::Vertical)
      	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      else
      	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      dragMode = false;
      points  = 0;
      updateKnob();
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize Slider::sizeHint() const
  	{
      int w = _sliderSize.width() + scaleWidth();
 	return orient == Qt::Vertical ? QSize(w, 200) : QSize(200, w);
      }

//---------------------------------------------------------
//   Slider
//---------------------------------------------------------

Slider::~Slider()
      {
      if (points)
      	delete points;
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void Slider::setOrientation(Qt::Orientation o)
      {
      orient = o;
      updateKnob();
      update();
      }

//---------------------------------------------------------
//   updateKnob
//---------------------------------------------------------

void Slider::updateKnob()
      {
	if (points)
      	delete points;
	points = new QPainterPath;
      int kh  = _sliderSize.height();
    	int kw  = _sliderSize.width();
      points->moveTo(0.0, 0.0);
      if (orient == Qt::Vertical) {
      	   int kh1  = _sliderSize.height();
            int kh2  = kh1 / 2;
            points->lineTo(kw, -kh2);
            points->lineTo(kw, kh2);
            }
      else {
            int kw2 = kw/2;
            points->lineTo(-kw2, kh);
            points->lineTo(kw2, kh);
            }
      points->lineTo(0.0, 0.0);
      }

//---------------------------------------------------------
//   setInvertedAppearance
//---------------------------------------------------------

void Slider::setInvertedAppearance(bool val)
      {
      AbstractSlider::setInvertedAppearance(val);
      update();
      }

//---------------------------------------------------------
//   setSliderSize
//---------------------------------------------------------

void Slider::setSliderSize(const QSize& s)
      {
      _sliderSize = s;
      update();
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Slider::mousePressEvent(QMouseEvent* ev)
      {
      startDrag = ev->pos();
            emit sliderPressed(__id);
            dragMode = true;
            int pixel = (orient == Qt::Vertical) ? height() - _sliderSize.height() : width() - _sliderSize.width();
            dragppos = int(pixel * (_value - minValue()) / (maxValue() - minValue()));
            if (_invert)
                  dragppos = pixel - dragppos;
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void Slider::mouseReleaseEvent(QMouseEvent*)
      {
      if (dragMode) {
            emit sliderReleased(__id);
            dragMode = false;
            }
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void Slider::mouseMoveEvent(QMouseEvent* ev)
      {
      if (!dragMode)
            return;
      int delta = orient == Qt::Horizontal ? (startDrag.x() - ev->x()) : (startDrag.y() - ev->y());

      if (orient == Qt::Horizontal)
            delta = -delta;
      int ppos = dragppos + delta;
      if (ppos < 0)
            ppos = 0;

      int pixel = (orient == Qt::Vertical) ? height() - _sliderSize.height() : width() - _sliderSize.width();
      if (ppos > pixel)
            ppos = pixel;
      int pos = _invert ? (pixel - ppos) : ppos;
      _value  = (pos * (maxValue() - minValue()) / pixel) + minValue() - 0.000001;
      valueChange();
      }

//---------------------------------------------------------
//   paint
//    r - phys coord system
//---------------------------------------------------------

void Slider::paintEvent(QPaintEvent* /*ev*/)
      {
      int h   = height();
      int w   = width();
      int kw  = _sliderSize.width();
      int kh  = _sliderSize.height();
      int pixel = (orient == Qt::Vertical) ? h - kh : w - kw;
      double range = maxValue() - minValue();
      int ppos = int(pixel * (_value - minValue()) / range);
      if ((orient == Qt::Vertical && _invert) || (orient == Qt::Horizontal && !_invert))
            ppos = pixel - ppos;

      qDebug()<<"height = "<<height()<<" (_sliderSize.height = "<<kh;
      qDebug()<<"width = "<<width()<<" (_sliderSize.width = "<<kh;

      qDebug()<<"ppos = "<<ppos;

      QPainter painter(this);

      painter.setRenderHint(QPainter::Antialiasing, true);

      _scaleColor = QApplication::palette().color(QPalette::AlternateBase);

      QColor scaleBackgroundColor(isEnabled() ? _scaleColor : Qt::gray);
      QColor scaleValueColor(isEnabled() ? _scaleValueColor : Qt::gray);
      painter.setBrush(scaleValueColor);

      int kh2 = kh/2;

      //---------------------------------------------------
      //    draw scale
      //---------------------------------------------------

      if (orient == Qt::Vertical) {
            int xMid = (w - _scaleWidth - _sliderSize.height()) / 2;
            int y1 = kh2;
            int y2 = h - (ppos + y1);
            int y3 = h - y1;

            int roundedness = 5;
            int grooveWidth = 5; // or _scaleWidth

            // Draw the un-filled part of the slider
            QPen outlinePen = QPen(QColor(Qt::gray).darker()); // get this from palette too?
            outlinePen.setWidth(0);
            painter.setPen(outlinePen);

            painter.setBrush(QBrush(Qt::gray));
            painter.drawRoundRect(xMid, y1, grooveWidth, y2-y1, roundedness, roundedness);

            // Draw the filled part of the slider

            // Get the HIGHLIGHT colour from the palette. This ensure that the fill color
            // of this hand-crafted control matches those of the QT supplied ones. And
            // ensures color changes when theme changes.

            QColor scaleValueFillColor = QApplication::palette().color(QPalette::Highlight);

            int red = scaleValueFillColor.red();
            int green = scaleValueFillColor.blue();
            int blue = scaleValueFillColor.green();

            red = red / 2;
            green = green / 2;
            scaleValueFillColor = QColor(red, green, blue);


            // the default QSlider is related to Highlight but not in a very obvious
            // way. Setting the highlight to, say, HARD RED gives close to HARD RED
            // THe default highlight varies in some subtle ways. There is also a gradient
            // on the slider that makes it non-obvious what's going on.
            //
            // An investigative strategy would be to:
            // - make the default slider WIDER (with a stylesheet)
            // - put in HARD RED, HARD BLUE, HARD GREEN
            // - see how they were softened
            // - try and figure out the function that is applied
            // - is it TO DO with a diff between the HIGHLIGHT color and some
            // - other palette color? Could test that by changing other palette
            // - values and seeing if the SHIFT was different.

            outlinePen = QPen(scaleValueFillColor.darker());
            outlinePen.setWidth(1);
            painter.setPen(outlinePen);
            painter.setBrush(scaleValueFillColor);

            painter.drawRoundRect(xMid, y2, grooveWidth, y3-y2, roundedness, roundedness);

            painter.translate(QPointF(xMid + _scaleWidth/2, y2));
      }
      else {
            // horizontal case (not revised!)
            int ym = (h - _scaleWidth - _sliderSize.height()) / 2;
            int x1 = kh2;
            int x2 = w - (ppos + x1);
            int x3 = w - x1;
            painter.fillRect(x1, ym, x2-x1, _scaleWidth, _invert ? scaleBackgroundColor : scaleValueColor);
            painter.fillRect(x2, ym, x3-x2, _scaleWidth, _invert ? scaleValueColor : scaleBackgroundColor);
            painter.translate(QPointF(x2, ym + _scaleWidth/2));
            }

      //---------------------------------------------------
      //    draw slider
      //---------------------------------------------------
      painter.setBrush(QBrush(QApplication::palette().color(QPalette::BrightText).darker()));
      painter.drawPath(*points);
      }
}

