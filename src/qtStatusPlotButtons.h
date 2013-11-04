/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _qtStatusPlotButtons_h
#define _qtStatusPlotButtons_h

#include <diCommonTypes.h>

#include <QWidget>
#include <QToolButton>

#include <vector>

class QHBoxLayout;
class QAction;
class QMenu;
class QFocusEvent;
class QKeyEvent;

/**
   \brief Data layer button

   One PlotButton for each data layer on the map, for quick toggle of layer
*/
class PlotButton : public QToolButton {
  Q_OBJECT
public:
  PlotButton(QWidget * parent, PlotElement& pe);

  /// Link this button to a specific PlotElement
  void setPlotElement(const PlotElement& pe);
  /// Return PlotElement for this button
  PlotElement getPlotElement(){return plotelement_;}
  /// tooltip string
  QString tipText(){return tipstr_;}

public slots:
  void togg(bool);      ///< toggle on/off
  void highlight(bool); ///< toggle highlighting

protected:
  PlotElement plotelement_;
  QString tipstr_;

signals:
  void enabled(PlotElement);
};


/**
   \brief Row of PlotButton

   Resides in status bar, gives quick access to enable/disable plotting a specific data layer.

*/

class StatusPlotButtons : public QWidget {
  Q_OBJECT
public:
  StatusPlotButtons(QWidget* parent = 0);

public slots:
  /// add several buttons
  void setPlotElements(const std::vector<PlotElement>& vpe);
  void enabled(PlotElement pe);
  void reset();
  void setfocus();
  void showText(const QString&);

protected:
  enum { MAXBUTTONS=50};
  int numbuttons;
//   QScrollArea* sv;
  PlotButton* buttons[MAXBUTTONS];
  QMenu* showtip;
  QPoint tip_pos;
  int activebutton;
  QAction * plotButtonsAction;
  void releasefocus();
  void calcTipPos();
  void showActiveButton(bool b);
  void focusInEvent ( QFocusEvent * );
  void focusOutEvent ( QFocusEvent * );
  void keyPressEvent ( QKeyEvent * e );

signals:
  void toggleElement(PlotElement);
  void releaseFocus();
};

#endif
