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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sstream>

#include <diWorkOrder.h>

diWorkOrder::diWorkOrder(QObject *parent, int serial, const char *text):
	QObject(parent), serial(serial), text(text), complete(false)
{
}

diWorkOrder::diWorkOrder(int serial, const char *text):
	QObject(), serial(serial), text(text), complete(false)
{
}

diWorkOrder::~diWorkOrder()
{
}

int
diWorkOrder::getSerial() const
{
	return serial;
}

const char *
diWorkOrder::getText() const
{
	return text.c_str();
}

bool
diWorkOrder::isComplete() const
{
	return complete;
}

void
diWorkOrder::signalCompletion()
{
	if (!complete) {
		emit workComplete();
		complete = true;
	}
}
