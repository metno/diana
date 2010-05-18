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

class diOrderClient: public QObject {
	Q_OBJECT;
public:
	diOrderClient(QObject *parent, QTcpSocket *socket);
	~diOrderClient();

signals:
	void connectionClosed();

private slots:
	void clientReadyRead();
	void clientStateChanged(QAbstractSocket::SocketState state);

private:
	void message(const QString &kw, const QString &msg);
	void hello();
	void error(const QString &msg);

	enum OrderState {
		idle = 0,
		reading = 1,
		pending = 2,
		complete = 3,
	};

	static const QString kw_hello;
	static const QString kw_error;
	static const QString kw_start_order_text;
	static const QString kw_start_order_base64;
	static const QString kw_end_order;

	QTcpSocket *socket;
	QByteArray orderbuf;
	OrderState state;
	bool base64;
};

#endif
