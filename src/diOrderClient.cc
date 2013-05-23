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

#define MILOGGER_CATEGORY "diana.OrderClient"
#include <miLogger/miLogging.h>

#include <diOrderClient.h>

/*
 * Protocol description:
 *
 * Sentences:
 *
 * - A sentence is a sequence of keywords and parameters ending in a
 *   newline.
 *
 * - Whitespace is not significant; leading and trailing whitespace is
 *   ignored, internal whitespace is reduced to single spaces.
 *
 * - A sentence can be followed by a comment, which starts with "#" and
 *   ends at the end of the line.
 *
 * - Blank lines (including lines consisting only of a comment) are
 *   ignored.
 *
 * Upon accepting a connection, we send the sentence "hello" and wait for
 * input from the client.
 *
 * The client sends the sentence "start order text" or "start order
 * base64", then the work order in either plaintext or base64, then the
 * sentence "end order".
 *
 * TODO how do we report the results?
 */
const char *diOrderClient::kw_hello = "hello";
const char *diOrderClient::kw_error = "error";
const char *diOrderClient::kw_start_order_text = "start order text";
const char *diOrderClient::kw_start_order_base64 = "start order base64";
const char *diOrderClient::kw_end_order = "end order";
const char *diOrderClient::kw_running = "running";
const char *diOrderClient::kw_complete = "complete";
const char *diOrderClient::kw_goodbye = "goodbye";

diOrderClient::diOrderClient(QObject *parent, QTcpSocket *socket):
	QObject(parent), socket(socket), state(idle), base64(false), serial(0), order(NULL)
{
	connect(socket, SIGNAL(readyRead()),
	    this, SLOT(clientReadyRead()));
	connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	    this, SLOT(clientStateChanged(QAbstractSocket::SocketState)));
	hello();
}

diOrderClient::~diOrderClient()
{
	socket->deleteLater();
	if (order != NULL)
		order->deleteLater();
}

bool
diOrderClient::hasOrder()
{
	return (state == pending);
}

diWorkOrder *
diOrderClient::getOrder()
{
	if (state == pending) {
		METLIBS_LOG_INFO(__func__ << "(): order retrieved");
		return order;
	}
	return NULL;
}

void
diOrderClient::readCommands()
{
//	METLIBS_LOG_DEBUG(__func__ << "(): " <<
//	    socket->bytesAvailable() << " bytes available");
	while (socket->canReadLine()) {
		if (state != idle && state != reading) {
			/*
			 * We are not in a state in which we will accept
			 * input; defer it until later.
			 */
//			METLIBS_LOG_DEBUG(__func__ << "(): " <<
//			    socket->bytesAvailable() << " bytes deferred");
			break;
		}

		QByteArray line = socket->readLine();
		/*
		 * Since canReadLine() was true, we know that an empty
		 * array results from an error.
		 */
		if (line.isEmpty()) {
			socket->close();
			return;
		}
		if (line.contains('#'))
			line.truncate(line.indexOf('#'));
		line = line.simplified();
		if (state != reading && line.isEmpty())
			return;
		std::string sline = line.constData();
		// METLIBS_LOG_DEBUG("[" << sline.toStdString() << "]");
		if (state == idle) {
			if (sline == kw_goodbye) {
				message(kw_goodbye);
				socket->close();
				return;
			} else if (sline == kw_start_order_text) {
				state = reading;
				base64 = false;
			} else if (sline == kw_start_order_base64) {
				state = reading;
				base64 = true;
			} else {
				error(QString("unrecognized command"));
			}
		} else if (state == reading) {
			if (sline == kw_end_order) {
				++serial;
				if (base64)
					order = new diWorkOrder(this, serial,
					    QByteArray::fromBase64(orderbuf.c_str()).constData());
				else
					order = new diWorkOrder(this, serial,
					    orderbuf.c_str());
				connect(order, SIGNAL(workComplete()),
				    this, SLOT(workOrderCompleted()));
				state = pending;
				emit newOrder();
				message(kw_running, QString("%1").arg(serial));
			} else {
				orderbuf += sline;
				orderbuf += "\n";
			}
		} else {
			error(QString("internal error"));
		}
	}
}

void
diOrderClient::clientReadyRead()
{
	readCommands();
}

void
diOrderClient::clientStateChanged(QAbstractSocket::SocketState state)
{
	if (state == 0)
		emit connectionClosed();
}

void
diOrderClient::workOrderCompleted()
{
	if (state != pending) {
		METLIBS_LOG_WARN("Received completion signal from work order " <<
		    order->getSerial() << " while not in pending state");
		return;
	}
	if (static_cast<diWorkOrder *>(sender()) != order) {
		METLIBS_LOG_WARN("Received completion signal from unknown work order ");
		return;
	}
	message(kw_complete, QString("%1").arg(order->getSerial()));
	order->deleteLater();
	order = NULL;
	state = idle;
	/*
	 * If the client has been "typing ahead", so to speak, there may
	 * be commands waiting in our input buffer.  Call readCommands()
	 * to check, since otherwise they won't be processed until some
	 * more input arrives to trigger the socket's readyRead().
	 */
	readCommands();
}

void
diOrderClient::message(const QString &kw, const QString &msg)
{
	QByteArray line;
	line += kw;
	line += " // ";
	line += msg.simplified();
	line += "\n";
	socket->write(line);
}

void
diOrderClient::message(const QString &kw)
{
	QByteArray line;
	line += kw;
	line += "\n";
	socket->write(line);
}

void
diOrderClient::hello()
{
	message(kw_hello, "bdiana " PACKAGE_VERSION);
}

void
diOrderClient::error(const QString &msg)
{
	message(kw_error, msg);
	socket->close();
}
