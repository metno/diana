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

#define MILOGGER_CATEGORY "diana.OrderQueue"
#include <miLogger/miLogging.h>

#include <diOrderQueue.h>

diOrderQueue::diOrderQueue(QObject *parent):
	QObject(parent)
{
}

diOrderQueue::~diOrderQueue()
{
	mutex.lock();
	if (!orders.empty())
		METLIBS_LOG_INFO("deleting diOrderQueue with " <<
		    orders.size() << " orders pending");
	while (!orders.empty())
		orders.dequeue()->deleteLater();
	mutex.unlock();
}

bool
diOrderQueue::hasQueuedOrders()
{
	mutex.lock();
	bool ret = !orders.empty();
	mutex.unlock();
	return ret;
}

uint
diOrderQueue::numQueuedOrders()
{
	mutex.lock();
	uint size = orders.size();
	mutex.unlock();
	return size;
}

bool
diOrderQueue::insertOrder(diWorkOrder *order)
{
	mutex.lock();
	orders.enqueue(order);
	condvar.wakeOne();
	mutex.unlock();
	emit newOrderAvailable();
	return true;
}

diWorkOrder *
diOrderQueue::getNextOrder()
{
	mutex.lock();
	if (orders.empty()) {
		mutex.unlock();
		return NULL;
	}
	diWorkOrder *wo = orders.dequeue();
	mutex.unlock();
	return wo;
}

diWorkOrder *
diOrderQueue::getNextOrderWait(uint msec)
{
	mutex.lock();
	while (orders.empty()) {
		if (!condvar.wait(&mutex, msec ? msec : ULONG_MAX)) {
			// timed out
			mutex.unlock();
			return NULL;
		}
	}
	diWorkOrder *wo = orders.dequeue();
	mutex.unlock();
	return wo;
}
