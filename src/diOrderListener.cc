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

#include <iostream>

#define MILOGGER_CATEGORY "diana.OrderListener"
#include <miLogger/miLogging.h>

#include <diOrderListener.h>

const int diOrderListener::DEFAULT_PORT = 3190;

diOrderListener::diOrderListener(QObject *parent):
	QObject(parent), server(this), orders(this)
{
	connect(&server, SIGNAL(newConnection()),
	    this, SLOT(newClientConnection()));
}

diOrderListener::~diOrderListener()
{
	mutex.lock();
	while (!clients.empty()) {
		(*clients.begin())->deleteLater();
		clients.erase(clients.begin());
	}
	mutex.unlock();
}

bool
diOrderListener::listen(const QHostAddress &addr, quint16 port)
{
	bool ok;
	mutex.lock();
	if ((ok = !server.isListening())) {
		if ((ok = server.listen(addr, port))) {
			METLIBS_LOG_INFO(this << QString("listening on %1:%2").
			    arg(addr.toString()).arg(port).toStdString());
		} else {
			METLIBS_LOG_INFO(this << QString("failed to listen on %1:%2: %3").
			    arg(addr.toString()).arg(port).arg(server.errorString()).toStdString());
		}
	}
	mutex.unlock();
	return ok;
}

bool
diOrderListener::hasQueuedOrders()
{
	return orders.hasQueuedOrders();
}

uint
diOrderListener::numQueuedOrders()
{
	return orders.numQueuedOrders();
}

diWorkOrder *
diOrderListener::getNextOrder()
{
	return orders.getNextOrder();
}

diWorkOrder *
diOrderListener::getNextOrderWait(uint msec)
{
	return orders.getNextOrderWait(msec);
}

void
diOrderListener::clientConnectionClosed()
{
	diOrderClient *client =
	    static_cast<diOrderClient *>(sender());
	mutex.lock();
	clients.erase(clients.find(client));
	client->deleteLater();
	mutex.unlock();
}

void
diOrderListener::newClientConnection()
{
	QTcpServer *server =
	    static_cast<QTcpServer *>(sender());
	QTcpSocket *socket = server->nextPendingConnection();
	if (socket == NULL)
		return;
	METLIBS_LOG_INFO("new client connection from " <<
	    socket->peerAddress().toString().toStdString());
	diOrderClient *client = new diOrderClient(this, socket);
	if (client == NULL) {
		METLIBS_LOG_INFO("failed to allocate new client");
		return;
	}
	mutex.lock();
	clients.insert(client);
	connect(client, SIGNAL(connectionClosed()),
	    this, SLOT(clientConnectionClosed()));
	connect(client, SIGNAL(newOrder()),
	    this, SLOT(clientHasNewOrder()));
	mutex.unlock();
}

void
diOrderListener::clientHasNewOrder()
{
	mutex.lock();
	diOrderClient *client =
	    static_cast<diOrderClient *>(sender());
	diWorkOrder *order = client->getOrder();
	mutex.unlock();
	if (order != NULL)
		orders.insertOrder(order);
	emit newOrder();
}
