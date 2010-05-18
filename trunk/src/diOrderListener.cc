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

#include <diOrderListener.h>

const int diOrderListener::DEFAULT_PORT = 3190;

diOrderListener::diOrderListener(QObject *parent):
	QObject(parent), server(this)
{
	connect(&server, SIGNAL(newConnection()),
	    this, SLOT(newClientConnection()));
}

diOrderListener::~diOrderListener()
{
	mutex.lock();
	while (!clients.empty()) {
		delete *clients.begin();
		clients.erase(clients.begin());
	}
	mutex.unlock();
}

bool
diOrderListener::listen(quint16 port)
{
	return (listen(QHostAddress::Any, port));
}

bool
diOrderListener::listen(const QString &addr, quint16 port)
{
	QHostAddress qha;
	if (!qha.setAddress(addr))
		return (false);
	return (listen(qha, port));
}

bool
diOrderListener::listen(const QHostAddress &addr, quint16 port)
{
	bool ok;
	mutex.lock();
	if (server.isListening())
		ok = false;
	else
		ok = server.listen(addr, port);
	mutex.unlock();
	if (ok)
		std::cerr << "listening on " <<
		    server.serverAddress().toString().toStdString() <<
		    " port " << port << std::endl;
	return (ok);
}

void
diOrderListener::clientConnectionClosed()
{
	diOrderClient *client = (diOrderClient *)sender();
	mutex.lock();
	clients.erase(clients.find(client));
	mutex.unlock();
	client->deleteLater();
}

void
diOrderListener::newClientConnection()
{
	mutex.lock();
	QTcpSocket *socket = server.nextPendingConnection();
	mutex.unlock();
	if (socket == NULL)
		return;
	std::cerr << "new client connection from " <<
	    socket->peerAddress().toString().toStdString() << std::endl;
	diOrderClient *client = new diOrderClient(this, socket);
	if (client == NULL) {
		std::cerr << "failed to allocate new client" << std::endl;
		return;
	}
	mutex.lock();
	clients.insert(client);
	connect(client, SIGNAL(connectionClosed()),
	    this, SLOT(clientConnectionClosed()));
	mutex.unlock();
}
