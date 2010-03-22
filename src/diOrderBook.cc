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

#include <diOrderBook.h>

diOrderBook::diOrderBook(QObject *parent):
	QThread(parent)
{
}

diOrderBook::~diOrderBook()
{
	mutex.lock();
	if (hasQueuedOrders())
		std::cerr << "deleting diOrderBook with " <<
		    numQueuedOrders() << " orders pending" << std::endl;
	while (!orders.empty())
		delete orders.dequeue();
	while (!listeners.empty()) {
		delete *listeners.begin();
		listeners.erase(listeners.begin());
	}
	mutex.unlock();
}

bool
diOrderBook::addListener(quint16 port)
{
	diOrderListener *listener =
	    new diOrderListener(); // see comment in start()
	if (listener == NULL)
		return (false);
	if (!listener->listen(port) || !addListener(listener)) {
		delete listener;
		return (false);
	}
	return (true);
}

bool
diOrderBook::addListener(const QString &addr, quint16 port)
{
	diOrderListener *listener =
	    new diOrderListener(); // see comment in start()
	if (listener == NULL)
		return (false);
	if (!listener->listen(addr, port) || !addListener(listener)) {
		delete listener;
		return (false);
	}
	return (true);
}

bool
diOrderBook::addListener(const QHostAddress &addr, quint16 port)
{
	diOrderListener *listener =
	    new diOrderListener(); // see comment in start()
	if (listener == NULL)
		return (false);
	if (!listener->listen(addr, port) || !addListener(listener)) {
		delete listener;
		return (false);
	}
	return (true);
}

bool
diOrderBook::addListener(diOrderListener *listener)
{
	// see comment in start()
	if (isRunning())
		listener->moveToThread(this);
	mutex.lock();
	listeners.insert(listener);
	mutex.unlock();
	return (true);
}

bool
diOrderBook::hasQueuedOrders()
{
	mutex.lock();
	bool ret = !orders.empty();
	mutex.unlock();
	return (ret);
}

uint
diOrderBook::numQueuedOrders()
{
	mutex.lock();
	uint size = orders.size();
	mutex.unlock();
	return (size);
}

diWorkOrder *
diOrderBook::getNextOrder()
{
	mutex.lock();
	if (orders.empty()) {
		mutex.unlock();
		return (NULL);
	}
	diWorkOrder *wo = orders.dequeue();
	mutex.unlock();
	return (wo);
}

diWorkOrder *
diOrderBook::getNextOrderWait(uint msec)
{
	mutex.lock();
	while (orders.empty()) {
		if (!condvar.wait(&mutex, msec ? msec : ULONG_MAX)) {
			// timed out
			mutex.unlock();
			return (NULL);
		}
	}
	diWorkOrder *wo = orders.dequeue();
	mutex.unlock();
	return (wo);
}

void
diOrderBook::start(Priority priority)
{
	if (isRunning())
		return;
	QThread::start(priority);
	/*
	 * Listeners will normally run in whichever thread started them.
	 * This defeats the purpose of running the order book in a
	 * separate thread...  so when the thread starts, we move all
	 * existing listeners over.  In addition, if a listener is added
	 * while the thread is running, we immediately move it over.
	 *
	 * Note that calling moveToThread() *before* the thread starts has
	 * no effect; neither does calling moveToThread() from any thread
	 * other than that which created the listener in the first place.
	 *
	 * Finally, it is not possible to move an object that has a
	 * parent, even if that parent is the target thread, so all
	 * variants of addListener() create listeners with no parent.  It
	 * probably does not really matter, since our destructor destroys
	 * all our listeners.
	 */
	QSet<diOrderListener *>::iterator it;
	for (it = listeners.begin(); it != listeners.end(); ++it)
		(*it)->moveToThread(this);
}

void
diOrderBook::newOrder(diWorkOrder *order)
{
	mutex.lock();
	orders.enqueue(order);
	condvar.wakeOne();
	mutex.unlock();
}

void
diOrderBook::run()
{
	exec();
}
