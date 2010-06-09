/*-*- c++ -*-

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

#ifndef DIWORKORDER_H_INCLUDED
#define DIWORKORDER_H_INCLUDED

#include <string>

#include <QObject>

/*
 * In an ideal world, a diWorkOrder would be a far more active object,
 * with its own identity and a certain amount of functionality, including
 * handling not just the input but also the *output* of the job.  It would
 * also be responsible for parsing the order text itself into some sort of
 * structured representation.
 *
 * For now, however, a diWorkOrder is simply a container for the order
 * text, with a signal that a diOrderListener can connect to so it can
 * notify the client that the job is complete.  Since the diWorkOrder is
 * not directly involved in processing the order, we have to expose a
 * signalCompletion() method that emits the workCompleted() signal.
 */
class diWorkOrder: public QObject {
	Q_OBJECT;
public:
	diWorkOrder(QObject *, int, const char *);
	diWorkOrder(int, const char *);
	~diWorkOrder();

	int getSerial() const;
	const char *getText() const;
	bool isComplete() const;
	void signalCompletion();

signals:
	void workComplete();

private:
	int serial;
	std::string text;
	bool complete;
};

#endif
