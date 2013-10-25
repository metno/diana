/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#ifndef _togglebutton_h
#define _togglebutton_h

#include <qpushbutton.h>
#include <qpalette.h>

class QMouseEvent;
class QPixmap;

/**
   \brief Toggle button
   Button changes colour ( and optionally text ) when toggled
*/
class ToggleButton : public QPushButton
{
  Q_OBJECT

public:
  ToggleButton(QWidget* parent, const QString& name, QColor* color = 0);
  ToggleButton(QWidget* parent, const QString& nameIn, const QString& nameOut, QColor* color = 0);

  ToggleButton(QWidget* parent, const QPixmap& pixmap, QColor* color = 0);

Q_SIGNALS:
  void rightButtonClicked(ToggleButton*);

public Q_SLOTS:
  void Toggled(bool on);

protected:
  virtual void mouseReleaseEvent(QMouseEvent *);

private:
  void init(QColor* color);
 
private:
  bool usePalette;
  bool NameChange;
  QString m_outName;
  QString m_inName;
  QPalette outPalette;
  QPalette inPalette;
};

#endif
