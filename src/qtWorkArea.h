/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2017 met.no

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
#ifndef WORKAREA_H
#define WORKAREA_H

#include "diEventResult.h"

#include <QWidget>

class Controller;
class MainUiEventHandler;
class MainPaintable;

/**
    \brief the main workarea
    provides a wrapper around the MainPaintable
*/
class WorkArea : public QWidget
{
  Q_OBJECT

public:
  WorkArea(Controller *c,  QWidget* parent = 0);
  ~WorkArea();

  MainPaintable* Glw() { return glw; }

  MainUiEventHandler* Gli() { return gli; }

  void setGlwCursor(const QCursor& c)
    { qw->setCursor(c); }

  void unsetGlwCursor()
    { qw->unsetCursor(); }

  void setGlwFocus()
    { qw->setFocus(); }

public Q_SLOTS:
  void updateGL();

private Q_SLOTS:
  void onRepaintRequested(bool updateBackgroundBuffer);
  void changeCursor(cursortype);

protected:
  MainPaintable* glw;
  MainUiEventHandler* gli;
  QWidget* qw;
  cursortype currentCursor;
};

#endif
