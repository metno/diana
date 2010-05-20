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

#ifndef DIORDERBOOK_H_INCLUDED
#define DIORDERBOOK_H_INCLUDED

#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QThread>
#include <QWaitCondition>

#include "diWorkOrder.h"
#include "diOrderListener.h"

class diOrderBook: public QThread {
	Q_OBJECT;
public:
	diOrderBook(QObject *parent = NULL);
	~diOrderBook();

	bool addListener(quint16 port = diOrderListener::DEFAULT_PORT);
	bool addListener(const QString &addr, quint16 port = diOrderListener::DEFAULT_PORT);
	bool addListener(const QHostAddress &addr, quint16 port = diOrderListener::DEFAULT_PORT);

	bool hasQueuedOrders();
	uint numQueuedOrders();
	diWorkOrder *getNextOrder();
	diWorkOrder *getNextOrderWait(uint msec = 0);

signals:
	void newOrder();

public slots:
	void start(Priority priority = InheritPriority);

private slots:
	void listenerHasNewOrder();

private:
	bool addListener(diOrderListener *listener);
	void run();
	QMutex mutex;
	diOrderQueue orders;
	QSet<diOrderListener *> listeners;
};

#endif
