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

#include <diOrderClient.h>

diOrderClient::diOrderClient(QObject *parent, QTcpSocket *socket): 
	QObject(parent), socket(socket)
{
	connect(socket, SIGNAL(readyRead()),
	    this, SLOT(clientReadyRead()));
	connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	    this, SLOT(clientStateChanged(QAbstractSocket::SocketState)));
}

diOrderClient::~diOrderClient()
{
	delete socket;
}

void
diOrderClient::clientReadyRead()
{
	if (socket->canReadLine()) {
		QByteArray line = socket->readLine();
		/*
		 * Since canReadLine() was true, we know that an empty
		 * array results from an error.
		 */
		if (line.isEmpty()) {
			socket->close(); // should trigger a state change
			return;
		}
		std::cerr << "line: " << line.data(); // \n already there
		// unfinished:
		// check for keywords etc.
		// accumulate work order in buffer
		// when entire order has been received, emit signal
	}
}

void
diOrderClient::clientStateChanged(QAbstractSocket::SocketState state)
{
	if (state == 0) {
		// buh-bye
		emit connectionClosed();
	}
}
