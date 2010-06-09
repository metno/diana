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

#ifndef DIORDERCLIENT_H_INCLUDED
#define DIORDERCLIENT_H_INCLUDED

#include <QTcpSocket>

#include "diOrderQueue.h"

class diOrderClient: public QObject {
	Q_OBJECT;
public:
	diOrderClient(QObject *parent, QTcpSocket *socket);
	~diOrderClient();

	bool hasOrder();
	diWorkOrder *getOrder();

signals:
	void connectionClosed();
	void newOrder();

private slots:
	void clientReadyRead();
	void clientStateChanged(QAbstractSocket::SocketState state);
	void workOrderCompleted();

private:
	void readCommands();
	void message(const QString &kw, const QString &msg);
	void message(const QString &kw);
	void hello();
	void error(const QString &msg);

	enum OrderState {
		idle = 0,
		reading = 1,
		pending = 2,
		complete = 3,
	};

	static const char *kw_hello;
	static const char *kw_error;
	static const char *kw_start_order_text;
	static const char *kw_start_order_base64;
	static const char *kw_end_order;
	static const char *kw_running;
	static const char *kw_complete;
	static const char *kw_goodbye;

	QTcpSocket *socket;
	std::string orderbuf;
	OrderState state;
	bool base64;
	int serial;

	diWorkOrder *order;
};

#endif
