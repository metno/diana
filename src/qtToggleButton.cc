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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtToggleButton.h"

#include <QMouseEvent>
#include <QPixmap>


ToggleButton::ToggleButton(QWidget* parent, const QString& name, QColor* color)
  : QPushButton(name, parent)
{
  init(color);
}

ToggleButton::ToggleButton(QWidget* parent, const QString& nameIn, const QString& nameOut, QColor* color)
  : QPushButton(nameOut, parent)
  , m_outName(nameOut)
  , m_inName(nameIn)
{
  init(color);
  NameChange = true;
}

ToggleButton::ToggleButton(QWidget* parent, const QPixmap& pixmap, QColor* color)
  : QPushButton(pixmap, QString(), parent)
{
  init(color);
}

void ToggleButton::init(QColor* color)
{
  NameChange = false;

  if (color && &color[0] && &color[1]) {
    inPalette = QPalette(color[0], color[1]);
    outPalette = palette();
    setPalette(outPalette);
    usePalette = true;
  } else {
    usePalette = false;
  }

  setCheckable(true);
  connect(this, SIGNAL(toggled(bool)), this, SLOT(Toggled(bool)));
}

void ToggleButton::Toggled(bool on)
{
  if (usePalette)
    setPalette(on ? inPalette : outPalette);
  if (NameChange)
    setText(on ? m_inName : m_outName);
}

void ToggleButton::mouseReleaseEvent( QMouseEvent *e )
{
  if (e->button() == Qt::RightButton) {
    /*emit*/ rightButtonClicked(this);
  }
  QPushButton::mouseReleaseEvent(e);
}
